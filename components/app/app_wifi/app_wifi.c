#include "app_wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tasks.h"
#include "app_nvs.h"

static const char *TAG = "wifi_app";

// netif objects for the Station and Access Point
esp_netif_t *esp_netif_sta = NULL;
esp_netif_t *esp_netif_ap = NULL;

// FreeRTOS queue and task handles
static QueueHandle_t wifi_app_queue_handle = NULL;
static TaskHandle_t wifi_app_task_handle = NULL;

// WiFi application callback
static wifi_connected_event_callback_t wifi_connected_cb = NULL;

// WiFi configuration
static wifi_config_t wifi_config = {0};

// Retry counter for STA connection
static int g_retry_number = 0;

/**
 * WiFi application event handler
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "Access Point started");
            // Send message to start HTTP server (Phase 2)
            wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);
            break;

        case WIFI_EVENT_AP_STACONNECTED:
        {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }

        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Station mode started");
            break;

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to AP");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGI(TAG, "Disconnected from AP, reason: %d", event->reason);
            
            if (g_retry_number < MAX_CONNECTION_RETRIES)
            {
                esp_wifi_connect();
                g_retry_number++;
                ESP_LOGI(TAG, "Retrying connection... (%d/%d)", g_retry_number, MAX_CONNECTION_RETRIES);
            }
            else
            {
                ESP_LOGI(TAG, "Failed to connect after %d retries", MAX_CONNECTION_RETRIES);
                wifi_app_send_message(WIFI_APP_MSG_STA_DISCONNECTED);
            }
            break;
        }

        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
            g_retry_number = 0;
            wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);
            break;
        }

        default:
            break;
        }
    }
}

/**
 * Initializes the WiFi application event handler
 */
static void wifi_app_event_handler_init(void)
{
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_app_event_handler, NULL, NULL));
}

/**
 * Initializes the TCP/IP stack and WiFi
 */
static void wifi_app_default_wifi_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize event handler
    wifi_app_event_handler_init();

    // Create default WiFi AP and STA interfaces
    esp_netif_ap = esp_netif_create_default_wifi_ap();
    esp_netif_sta = esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_LOGI(TAG, "WiFi initialized");
}

/**
 * Configures the WiFi Access Point
 */
static void wifi_app_soft_ap_config(void)
{
    // Configure AP with static IP
    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0, sizeof(ap_ip_info));

    // Stop DHCP server
    esp_netif_dhcps_stop(esp_netif_ap);

    // Set static IP
    inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));

    // Start DHCP server
    esp_netif_dhcps_start(esp_netif_ap);

    // Configure WiFi AP
    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASSWORD,
            .channel = WIFI_AP_CHANNEL,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = WIFI_AP_SSID_HIDDEN,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .beacon_interval = WIFI_AP_BEACON_INTERVAL,
        },
    };

    // Set WiFi mode to AP+STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));

    ESP_LOGI(TAG, "Access Point configured: SSID=%s, Channel=%d", WIFI_AP_SSID, WIFI_AP_CHANNEL);
}

/**
 * Connects to an external WiFi Access Point (STA mode)
 * This will be used in Phase 2 when HTTP server sends connection request
 */
static void wifi_app_connect_sta(void)
{
    ESP_LOGI(TAG, "Connecting to AP: %s", wifi_config.sta.ssid);
    g_retry_number = 0;
    ESP_ERROR_CHECK(esp_wifi_connect());
}

/**
 * Main WiFi application task
 */
static void wifi_app_task(void *pvParameters)
{
    wifi_app_queue_message_t msg;

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    for (;;)
    {
        if (xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
            case WIFI_APP_MSG_START_HTTP_SERVER:
                ESP_LOGI(TAG, "Received: START_HTTP_SERVER (will be implemented in Phase 2)");
                break;

            case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
                ESP_LOGI(TAG, "Received: CONNECTING_FROM_HTTP_SERVER");
                wifi_app_connect_sta();
                break;

            case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                ESP_LOGI(TAG, "Received: STA_CONNECTED_GOT_IP");
                // Call callback if set
                if (wifi_connected_cb)
                {
                    wifi_connected_cb();
                }
                // TODO: Initialize SNTP in Phase 2
                break;

            case WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT:
                ESP_LOGI(TAG, "Received: USER_REQUESTED_STA_DISCONNECT");
                g_retry_number = MAX_CONNECTION_RETRIES;
                ESP_ERROR_CHECK(esp_wifi_disconnect());
                break;

            case WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS:
                ESP_LOGI(TAG, "Received: LOAD_SAVED_CREDENTIALS");
                if (app_nvs_load_sta_creds())
                {
                    ESP_LOGI(TAG, "Loaded saved credentials, attempting connection");
                    wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);
                }
                else
                {
                    ESP_LOGI(TAG, "No saved credentials found");
                }
                break;

            case WIFI_APP_MSG_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Received: STA_DISCONNECTED");
                break;

            default:
                ESP_LOGW(TAG, "Unknown message ID: %d", msg.msgID);
                break;
            }
        }
    }
}

/**
 * Sends a message to the queue
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
    wifi_app_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

/**
 * Starts the WiFi RTOS task
 */
void wifi_app_start(void)
{
    ESP_LOGI(TAG, "Starting WiFi application");

    // Initialize WiFi
    wifi_app_default_wifi_init();

    // Configure Access Point
    wifi_app_soft_ap_config();

    // Create message queue
    wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

    // Create WiFi application task
    xTaskCreatePinnedToCore(
        &wifi_app_task,
        "wifi_app_task",
        WIFI_APP_TASK_STACK_SIZE,
        NULL,
        WIFI_APP_TASK_PRIORITY,
        &wifi_app_task_handle,
        WIFI_APP_TASK_CORE_ID);
}

/**
 * Gets the WiFi configuration
 */
wifi_config_t *wifi_app_get_wifi_config(void)
{
    return &wifi_config;
}

/**
 * Sets the callback function
 */
void wifi_app_set_callback(wifi_connected_event_callback_t cb)
{
    wifi_connected_cb = cb;
}

/**
 * Calls the callback function
 */
void wifi_app_call_callback(void)
{
    if (wifi_connected_cb)
    {
        wifi_connected_cb();
    }
}

/**
 * Gets the RSSI value of the WiFi connection
 * @return current RSSI level (0 if not connected)
 */
int8_t wifi_app_get_rssi(void)
{
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    
    if (ret == ESP_OK)
    {
        return ap_info.rssi;
    }
    
    return 0;
}
