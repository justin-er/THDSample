#include "dns_server.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "dns_server";

#define DNS_PORT 53
#define DNS_MAX_PACKET_SIZE 512

// DNS server state
static int dns_socket = -1;
static TaskHandle_t dns_task_handle = NULL;
static bool dns_running = false;

/**
 * DNS header structure
 */
typedef struct __attribute__((packed)) {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;

/**
 * Parse DNS query and build response
 * Responds with ESP32 AP IP for all queries
 */
static int build_dns_response(uint8_t *request, int request_len, uint8_t *response)
{
    if (request_len < sizeof(dns_header_t)) {
        return 0;
    }
    
    dns_header_t *resp_header = (dns_header_t *)response;
    
    // Copy request header
    memcpy(response, request, request_len);
    
    // Set response flags
    resp_header->flags = htons(0x8180);  // Standard query response, no error
    resp_header->ancount = htons(1);     // One answer
    resp_header->nscount = 0;
    resp_header->arcount = 0;
    
    int pos = request_len;
    
    // Add answer section (pointer to question + type A + class IN + TTL + IP)
    response[pos++] = 0xC0;  // Pointer to question name
    response[pos++] = 0x0C;
    response[pos++] = 0x00;  // Type A
    response[pos++] = 0x01;
    response[pos++] = 0x00;  // Class IN
    response[pos++] = 0x01;
    response[pos++] = 0x00;  // TTL (60 seconds)
    response[pos++] = 0x00;
    response[pos++] = 0x00;
    response[pos++] = 0x3C;
    response[pos++] = 0x00;  // Data length (4 bytes for IPv4)
    response[pos++] = 0x04;
    
    // IP address (192.168.0.1)
    response[pos++] = 192;
    response[pos++] = 168;
    response[pos++] = 0;
    response[pos++] = 1;
    
    return pos;
}

/**
 * DNS server task
 * Listens for DNS queries and responds with ESP32 IP
 */
static void dns_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    uint8_t rx_buffer[DNS_MAX_PACKET_SIZE];
    uint8_t tx_buffer[DNS_MAX_PACKET_SIZE];
    
    ESP_LOGI(TAG, "DNS server task started");
    
    // Create UDP socket
    dns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (dns_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        dns_running = false;
        vTaskDelete(NULL);
        return;
    }
    
    // Bind to DNS port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DNS_PORT);
    
    if (bind(dns_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket");
        close(dns_socket);
        dns_socket = -1;
        dns_running = false;
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "DNS server listening on port %d", DNS_PORT);
    
    while (dns_running) {
        // Receive DNS query
        int len = recvfrom(dns_socket, rx_buffer, sizeof(rx_buffer), 0,
                          (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (len < 0) {
            if (dns_running) {
                ESP_LOGE(TAG, "recvfrom failed");
            }
            break;
        }
        
        if (len > 0) {
            // Build and send response
            int response_len = build_dns_response(rx_buffer, len, tx_buffer);
            if (response_len > 0) {
                sendto(dns_socket, tx_buffer, response_len, 0,
                      (struct sockaddr *)&client_addr, client_addr_len);
                
                ESP_LOGD(TAG, "DNS query responded");
            }
        }
    }
    
    ESP_LOGI(TAG, "DNS server task ending");
    
    if (dns_socket >= 0) {
        close(dns_socket);
        dns_socket = -1;
    }
    
    vTaskDelete(NULL);
}

esp_err_t dns_server_start(void)
{
    if (dns_running) {
        ESP_LOGW(TAG, "DNS server already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting DNS server");
    
    dns_running = true;
    
    BaseType_t ret = xTaskCreate(
        dns_server_task,
        "dns_server",
        4096,
        NULL,
        5,
        &dns_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create DNS server task");
        dns_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "DNS server started successfully");
    return ESP_OK;
}

esp_err_t dns_server_stop(void)
{
    if (!dns_running) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping DNS server");
    
    dns_running = false;
    
    // Close socket to unblock recvfrom
    if (dns_socket >= 0) {
        close(dns_socket);
        dns_socket = -1;
    }
    
    // Wait for task to finish
    if (dns_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
        dns_task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "DNS server stopped");
    return ESP_OK;
}

bool dns_server_is_running(void)
{
    return dns_running;
}

