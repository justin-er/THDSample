#include "http_server.h"
#include "app_coordinator.h"
#include "app_wifi.h"
#include "sntp_client.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "http_server";

// HTTP server handle
static httpd_handle_t server = NULL;

// Embedded file declarations (will be populated by CMake)
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t app_css_start[] asm("_binary_app_css_start");
extern const uint8_t app_css_end[] asm("_binary_app_css_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");
extern const uint8_t jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

/**
 * Root handler - serves index.html
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

/**
 * CSS file handler
 */
static esp_err_t css_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);
    return ESP_OK;
}

/**
 * JavaScript file handler
 */
static esp_err_t js_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
    return ESP_OK;
}

/**
 * jQuery library handler
 */
static esp_err_t jquery_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);
    return ESP_OK;
}

/**
 * Favicon handler
 */
static esp_err_t favicon_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);
    return ESP_OK;
}

/**
 * Captive portal detection handler
 * Handles common captive portal detection URLs used by iOS, Android, and Windows devices
 * Redirects to root page to trigger captive portal
 */
static esp_err_t captive_portal_handler(httpd_req_t *req)
{
    // Redirect to root page - this triggers captive portal detection
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/**
 * AP SSID handler - returns ESP32 AP SSID
 */
static esp_err_t ap_ssid_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ssid", WIFI_AP_SSID);
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * DHT sensor handler - returns temperature and humidity
 */
static esp_err_t dht_sensor_handler(httpd_req_t *req)
{
    app_coordinator_sensor_data_t data;
    esp_err_t ret = app_coordinator_get_sensor_data(&data);
    
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Sensor unavailable");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_CreateObject();
    char temp_str[32], humidity_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.1f°C", data.temperature);
    snprintf(humidity_str, sizeof(humidity_str), "%.1f%%", data.humidity);
    
    cJSON_AddStringToObject(root, "temp", temp_str);
    cJSON_AddStringToObject(root, "humidity", humidity_str);
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * Local time handler - returns formatted local time
 */
static esp_err_t local_time_handler(httpd_req_t *req)
{
    char time_str[128] = {0};
    esp_err_t ret = sntp_client_get_time_string(time_str, sizeof(time_str));
    
    if (ret != ESP_OK) {
        strcpy(time_str, "Time not available");
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "time", time_str);
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * System status handler - returns heap, uptime, version
 */
static esp_err_t system_status_handler(httpd_req_t *req)
{
    app_coordinator_system_info_t info;
    esp_err_t ret = app_coordinator_get_system_info(&info);
    
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "System info unavailable");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "heap_free", info.heap_free);
    cJSON_AddNumberToObject(root, "heap_min", info.heap_min);
    cJSON_AddNumberToObject(root, "uptime_seconds", info.uptime_seconds);
    cJSON_AddStringToObject(root, "firmware_version", info.firmware_version);
    cJSON_AddStringToObject(root, "compile_date", info.compile_date);
    cJSON_AddStringToObject(root, "compile_time", info.compile_time);
    cJSON_AddBoolToObject(root, "wifi_sta_connected", info.wifi_sta_connected);
    cJSON_AddNumberToObject(root, "wifi_ap_clients", info.wifi_ap_clients);
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * OTA status handler - returns firmware info and OTA status
 */
static esp_err_t ota_status_handler(httpd_req_t *req)
{
    app_coordinator_ota_status_t status;
    esp_err_t ret = app_coordinator_get_ota_status(&status);
    
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA status unavailable");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ota_update_status", status.status);
    cJSON_AddStringToObject(root, "compile_date", status.compile_date);
    cJSON_AddStringToObject(root, "compile_time", status.compile_time);
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * WiFi connect handler - initiates connection to external WiFi
 */
static esp_err_t wifi_connect_handler(httpd_req_t *req)
{
    char ssid[33] = {0};
    char password[65] = {0};
    
    // Get SSID from header
    size_t ssid_len = httpd_req_get_hdr_value_len(req, "my-connect-ssid");
    if (ssid_len > 0 && ssid_len < sizeof(ssid)) {
        httpd_req_get_hdr_value_str(req, "my-connect-ssid", ssid, sizeof(ssid));
    }
    
    // Get password from header
    size_t pwd_len = httpd_req_get_hdr_value_len(req, "my-connect-pwd");
    if (pwd_len > 0 && pwd_len < sizeof(password)) {
        httpd_req_get_hdr_value_str(req, "my-connect-pwd", password, sizeof(password));
    }
    
    if (strlen(ssid) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "WiFi connect request: SSID=%s", ssid);
    
    // Set credentials
    esp_err_t ret = wifi_app_set_sta_credentials(ssid, password);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set credentials");
        return ESP_FAIL;
    }
    
    // Send connect message to WiFi app
    wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"connecting\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * WiFi connect status handler - returns connection status
 */
static esp_err_t wifi_connect_status_handler(httpd_req_t *req)
{
    wifi_app_connection_status_e status = wifi_app_get_connection_status();
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "wifi_connect_status", status);
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * WiFi connect info handler - returns IP, gateway, netmask
 */
static esp_err_t wifi_connect_info_handler(httpd_req_t *req)
{
    wifi_app_connection_info_t info;
    esp_err_t ret = wifi_app_get_connection_info(&info);
    
    cJSON *root = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddStringToObject(root, "ap", info.ssid);
        cJSON_AddStringToObject(root, "ip", info.ip);
        cJSON_AddStringToObject(root, "netmask", info.netmask);
        cJSON_AddStringToObject(root, "gw", info.gateway);
    } else {
        cJSON_AddStringToObject(root, "ap", "");
        cJSON_AddStringToObject(root, "ip", "");
        cJSON_AddStringToObject(root, "netmask", "");
        cJSON_AddStringToObject(root, "gw", "");
    }
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * WiFi disconnect handler - disconnects from external WiFi
 */
static esp_err_t wifi_disconnect_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "WiFi disconnect request");
    
    // Send disconnect message to WiFi app
    wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"disconnecting\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * WiFi scan handler - returns available networks
 */
static esp_err_t wifi_scan_handler(httpd_req_t *req)
{
    wifi_app_scan_result_t *results = NULL;
    size_t count = 0;
    
    esp_err_t ret = wifi_app_scan_networks(&results, &count);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Scan failed");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_CreateArray();
    
    for (size_t i = 0; i < count; i++) {
        cJSON *network = cJSON_CreateObject();
        cJSON_AddStringToObject(network, "ssid", results[i].ssid);
        cJSON_AddNumberToObject(network, "rssi", results[i].rssi);
        
        const char *auth_str = "Open";
        if (results[i].auth_mode == WIFI_AUTH_WEP) auth_str = "WEP";
        else if (results[i].auth_mode == WIFI_AUTH_WPA_PSK) auth_str = "WPA";
        else if (results[i].auth_mode == WIFI_AUTH_WPA2_PSK) auth_str = "WPA2";
        else if (results[i].auth_mode == WIFI_AUTH_WPA_WPA2_PSK) auth_str = "WPA/WPA2";
        else if (results[i].auth_mode == WIFI_AUTH_WPA3_PSK) auth_str = "WPA3";
        
        cJSON_AddStringToObject(network, "auth", auth_str);
        cJSON_AddItemToArray(root, network);
    }
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    
    if (results != NULL) {
        free(results);
    }
    
    return ESP_OK;
}

/**
 * Config backup handler - exports configuration
 */
static esp_err_t config_backup_handler(httpd_req_t *req)
{
    char *json_out = NULL;
    esp_err_t ret = app_coordinator_backup_config(&json_out);
    
    if (ret != ESP_OK || json_out == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Backup failed");
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"esp32_config.json\"");
    httpd_resp_send(req, json_out, strlen(json_out));
    
    free(json_out);
    return ESP_OK;
}

/**
 * Config restore handler - imports configuration
 */
static esp_err_t config_restore_handler(httpd_req_t *req)
{
    char *buf = malloc(req->content_len + 1);
    if (buf == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }
    
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    
    esp_err_t result = app_coordinator_restore_config(buf);
    free(buf);
    
    if (result != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid configuration");
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"restored\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * OTA update handler - receives firmware binary
 */
static esp_err_t ota_update_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "OTA update request, size: %d bytes", req->content_len);
    
    // This will be fully implemented when ota_update component is created
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA not yet implemented");
    return ESP_FAIL;
}

esp_err_t http_server_start(void)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "HTTP server already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting HTTP server");
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 25;  // Increased to accommodate captive portal handlers
    // max_open_sockets: HTTP server uses 3 sockets internally
    // Current: 7 (works with LWIP_MAX_SOCKETS=10)
    // After 'idf.py reconfigure': Change to 17 (works with LWIP_MAX_SOCKETS=20 from sdkconfig.defaults)
    config.max_open_sockets = 7;  // TODO: After reconfigure, increase to 17 for 5-client support
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }
    
    // Register static file handlers
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root_uri);
    
    httpd_uri_t css_uri = {
        .uri = "/app.css",
        .method = HTTP_GET,
        .handler = css_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &css_uri);
    
    httpd_uri_t js_uri = {
        .uri = "/app.js",
        .method = HTTP_GET,
        .handler = js_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &js_uri);
    
    httpd_uri_t jquery_uri = {
        .uri = "/jquery-3.3.1.min.js",
        .method = HTTP_GET,
        .handler = jquery_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &jquery_uri);
    
    httpd_uri_t favicon_uri = {
        .uri = "/favicon.ico",
        .method = HTTP_GET,
        .handler = favicon_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &favicon_uri);
    
    // Register captive portal detection handlers (iOS, Android, Windows)
    httpd_uri_t captive_portal_uri = {
        .uri = "/hotspot-detect.html",
        .method = HTTP_GET,
        .handler = captive_portal_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &captive_portal_uri);
    
    httpd_uri_t captive_portal_uri2 = {
        .uri = "/generate_204",
        .method = HTTP_GET,
        .handler = captive_portal_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &captive_portal_uri2);
    
    httpd_uri_t captive_portal_uri3 = {
        .uri = "/gen_204",
        .method = HTTP_GET,
        .handler = captive_portal_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &captive_portal_uri3);
    
    httpd_uri_t captive_portal_uri4 = {
        .uri = "/connecttest.txt",
        .method = HTTP_GET,
        .handler = captive_portal_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &captive_portal_uri4);
    
    httpd_uri_t captive_portal_uri5 = {
        .uri = "/success.txt",
        .method = HTTP_GET,
        .handler = captive_portal_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &captive_portal_uri5);
    
    // Register API handlers
    httpd_uri_t ap_ssid_uri = {
        .uri = "/apSSID.json",
        .method = HTTP_GET,
        .handler = ap_ssid_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &ap_ssid_uri);
    
    httpd_uri_t dht_uri = {
        .uri = "/dhtSensor.json",
        .method = HTTP_GET,
        .handler = dht_sensor_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &dht_uri);
    
    httpd_uri_t time_uri = {
        .uri = "/localTime.json",
        .method = HTTP_GET,
        .handler = local_time_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &time_uri);
    
    httpd_uri_t system_uri = {
        .uri = "/systemStatus.json",
        .method = HTTP_GET,
        .handler = system_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &system_uri);
    
    httpd_uri_t ota_status_uri = {
        .uri = "/OTAstatus",
        .method = HTTP_POST,
        .handler = ota_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &ota_status_uri);
    
    httpd_uri_t ota_update_uri = {
        .uri = "/OTAupdate",
        .method = HTTP_POST,
        .handler = ota_update_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &ota_update_uri);
    
    httpd_uri_t wifi_connect_uri = {
        .uri = "/wifiConnect.json",
        .method = HTTP_POST,
        .handler = wifi_connect_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &wifi_connect_uri);
    
    httpd_uri_t wifi_status_uri = {
        .uri = "/wifiConnectStatus",
        .method = HTTP_POST,
        .handler = wifi_connect_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &wifi_status_uri);
    
    httpd_uri_t wifi_info_uri = {
        .uri = "/wifiConnectInfo.json",
        .method = HTTP_GET,
        .handler = wifi_connect_info_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &wifi_info_uri);
    
    httpd_uri_t wifi_disconnect_uri = {
        .uri = "/wifiDisconnect.json",
        .method = HTTP_DELETE,
        .handler = wifi_disconnect_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &wifi_disconnect_uri);
    
    httpd_uri_t wifi_scan_uri = {
        .uri = "/wifiScan.json",
        .method = HTTP_GET,
        .handler = wifi_scan_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &wifi_scan_uri);
    
    httpd_uri_t config_backup_uri = {
        .uri = "/configBackup.json",
        .method = HTTP_GET,
        .handler = config_backup_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &config_backup_uri);
    
    httpd_uri_t config_restore_uri = {
        .uri = "/configRestore.json",
        .method = HTTP_POST,
        .handler = config_restore_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &config_restore_uri);
    
    ESP_LOGI(TAG, "HTTP server started successfully");
    return ESP_OK;
}

esp_err_t http_server_stop(void)
{
    if (server == NULL) {
        ESP_LOGW(TAG, "HTTP server not running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping HTTP server");
    esp_err_t ret = httpd_stop(server);
    server = NULL;
    return ret;
}

bool http_server_is_running(void)
{
    return server != NULL;
}

