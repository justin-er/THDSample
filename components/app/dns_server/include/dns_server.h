#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include "esp_err.h"

/**
 * Start DNS server for captive portal
 * Responds to all DNS queries with ESP32 AP IP
 * 
 * @return ESP_OK on success
 */
esp_err_t dns_server_start(void);

/**
 * Stop DNS server
 * 
 * @return ESP_OK on success
 */
esp_err_t dns_server_stop(void);

/**
 * Check if DNS server is running
 * 
 * @return true if running, false otherwise
 */
bool dns_server_is_running(void);

#endif // DNS_SERVER_H

