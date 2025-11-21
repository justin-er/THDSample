#ifndef DHT_READER_H
#define DHT_READER_H

#include <freertos/queue.h>

typedef struct {
    float humidity;
    float temperature;
} DHTData;

DHTData readDHT(void);
QueueHandle_t dht_init(void);

#endif
