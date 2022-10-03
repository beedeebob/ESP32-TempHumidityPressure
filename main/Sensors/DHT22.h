#ifndef _DHT22_
#define _DHT22_

/* Includes ------------------------------------------------------------------------------- */
#include "driver/rmt.h"

/* Types ---------------------------------------------------------------------------------- */
typedef void (*DHT22_TemperatureAndHumidityUpdateCallback)(void *pDHT);
typedef struct DHT22_TypeDef
{
    rmt_channel_t rmt;
    uint8_t pin;
    RingbufHandle_t buff;
    TaskHandle_t task;
    DHT22_TemperatureAndHumidityUpdateCallback updated;

    //Results
    int32_t temperature;
    int32_t humidity;
}DHT22_TypeDef;

/* Function Prototypes -------------------------------------------------------------------- */
DHT22_TypeDef* DHT22_Init(rmt_channel_t RMT_CHANNEL, uint8_t GPIO_PIN);
void DHT22_StartUpdateTemperatureAndHumidity(DHT22_TypeDef *pDHT, DHT22_TemperatureAndHumidityUpdateCallback updateCB);

#endif /* _DHT22_ */