#ifndef DHT_READER_H
#define DHT_READER_H

typedef struct {
    float humidity;
    float temperature;
} DHTData;

DHTData readDHT(void);

#endif
