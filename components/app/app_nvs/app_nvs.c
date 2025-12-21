#include "app_nvs.h"
#include "esp_log.h"
#include "nvs.h"
#include "string.h"
#include "esp_wifi.h"

static const char *TAG = "app_nvs";

// NVS namespace and keys for WiFi credentials
#define APP_NVS_WIFI_NAMESPACE "wifi_creds"
#define APP_NVS_SSID_KEY "ssid"
#define APP_NVS_PASS_KEY "password"

/**
 * Saves station mode WiFi credentials to NVS
 * @return ESP_OK if successful.
 */
esp_err_t app_nvs_save_sta_creds(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ESP_LOGI(TAG, "Saving WiFi credentials to NVS");

    // Get WiFi configuration
    wifi_config_t wifi_config;
    ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get WiFi config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Open NVS handle
    ret = nvs_open(APP_NVS_WIFI_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }

    // Save SSID
    ret = nvs_set_str(nvs_handle, APP_NVS_SSID_KEY, (const char *)wifi_config.sta.ssid);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save SSID: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    // Save password
    ret = nvs_set_str(nvs_handle, APP_NVS_PASS_KEY, (const char *)wifi_config.sta.password);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save password: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    // Commit changes
    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "WiFi credentials saved successfully");
    return ESP_OK;
}

/**
 * Loads the previously saved credentials from NVS.
 * @return true if previously saved credentials were found.
 */
bool app_nvs_load_sta_creds(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    size_t ssid_len = 0;
    size_t pass_len = 0;

    ESP_LOGI(TAG, "Loading WiFi credentials from NVS");

    // Open NVS handle
    ret = nvs_open(APP_NVS_WIFI_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
    {
        if (ret == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGI(TAG, "No saved WiFi credentials found");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(ret));
        }
        return false;
    }

    // Get SSID length
    ret = nvs_get_str(nvs_handle, APP_NVS_SSID_KEY, NULL, &ssid_len);
    if (ret != ESP_OK)
    {
        ESP_LOGI(TAG, "No SSID found in NVS");
        nvs_close(nvs_handle);
        return false;
    }

    // Get password length
    ret = nvs_get_str(nvs_handle, APP_NVS_PASS_KEY, NULL, &pass_len);
    if (ret != ESP_OK)
    {
        ESP_LOGI(TAG, "No password found in NVS");
        nvs_close(nvs_handle);
        return false;
    }

    // Allocate buffers
    char *ssid = malloc(ssid_len);
    char *password = malloc(pass_len);
    if (ssid == NULL || password == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for credentials");
        free(ssid);
        free(password);
        nvs_close(nvs_handle);
        return false;
    }

    // Read SSID
    ret = nvs_get_str(nvs_handle, APP_NVS_SSID_KEY, ssid, &ssid_len);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read SSID: %s", esp_err_to_name(ret));
        free(ssid);
        free(password);
        nvs_close(nvs_handle);
        return false;
    }

    // Read password
    ret = nvs_get_str(nvs_handle, APP_NVS_PASS_KEY, password, &pass_len);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read password: %s", esp_err_to_name(ret));
        free(ssid);
        free(password);
        nvs_close(nvs_handle);
        return false;
    }

    nvs_close(nvs_handle);

    // Set WiFi configuration
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, ssid_len);
    memcpy(wifi_config.sta.password, password, pass_len);

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        free(ssid);
        free(password);
        return false;
    }

    ESP_LOGI(TAG, "WiFi credentials loaded successfully: SSID=%s", ssid);

    free(ssid);
    free(password);
    return true;
}

/**
 * Clears station mode credentials from NVS
 * @return ESP_OK if successful.
 */
esp_err_t app_nvs_clear_sta_creds(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ESP_LOGI(TAG, "Clearing WiFi credentials from NVS");

    // Open NVS handle
    ret = nvs_open(APP_NVS_WIFI_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }

    // Erase SSID
    ret = nvs_erase_key(nvs_handle, APP_NVS_SSID_KEY);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Failed to erase SSID: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    // Erase password
    ret = nvs_erase_key(nvs_handle, APP_NVS_PASS_KEY);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Failed to erase password: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    // Commit changes
    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "WiFi credentials cleared successfully");
    return ESP_OK;
}
