/* Includes ------------------------------------------------------------------------------- */
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "DHT22.h"

/* Definitions ---------------------------------------------------------------------------- */
#define THP_UPDATEPERIOD                      10000        //Update every 10 seconds

/* Types ---------------------------------------------------------------------------------- */
/* Variables ------------------------------------------------------------------------------ */
static DHT22_TypeDef *dht22 = NULL;

/* Function Prototypes -------------------------------------------------------------------- */
static void THP_ControlTask(void *pvParameter);
static void THP_DHTUpdateCallback(void *pDHT);

/* ---------------------------------------------------------------------------------------- */

/* @details Initialize the DHT22 control system
 * @param   None
 * @result  None
 */
void THP_init(void)
{
    //Initialize the sensors
    dht22 = DHT22_Init(RMT_CHANNEL_0, 4);

    //Start the task
    xTaskCreate(&THP_ControlTask, "THP_ControlTask", 2048, NULL, 5, NULL);
}

/* ---------------------------------------------------------------------------------------- */
/* @details The DHT22 control task routine
 * @param   pvParameter: pointer to argument passed from the task
 * @result  None
 */
static void THP_ControlTask(void *pvParameter)
{
    while (1) 
    {
        //DHT22
        DHT22_StartUpdateTemperatureAndHumidity(dht22, THP_DHTUpdateCallback);

        vTaskDelay(THP_UPDATEPERIOD / portTICK_PERIOD_MS);
    }
    fflush(stdout);
}

/* ---------------------------------------------------------------------------------------- */
/* @details The handler for the DHT22 update callback
 * @param   pvParameter: pointer to DHT passed
 * @result  None
 */
static void THP_DHTUpdateCallback(void *pDHT)
{
    DHT22_TypeDef *pDHT22 = (DHT22_TypeDef *)pDHT;

    //TODO: Handle DHT22 update
}