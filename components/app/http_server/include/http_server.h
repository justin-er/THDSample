#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * Start HTTP server
 * Serves static files and REST API endpoints
 * 
 * @return ESP_OK on success
 */
esp_err_t http_server_start(void);

/**
 * Stop HTTP server
 * 
 * @return ESP_OK on success
 */
esp_err_t http_server_stop(void);

/**
 * Check if HTTP server is running
 * 
 * @return true if running, false otherwise
 */
bool http_server_is_running(void);

#endif // HTTP_SERVER_H

