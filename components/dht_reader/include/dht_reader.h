#ifndef DHT_READER_H
#define DHT_READER_H

#include <freertos/queue.h>
#include <esp_err.h>

typedef struct {
    float humidity;
    float temperature;
} dht_data_t;

QueueHandle_t dht_init(void);

#endif
