
/* Includes ------------------------------------------------------------------------------- */
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

/* Function Prototypes -------------------------------------------------------------------- */
static void DHT22_ControlTask(void *pvParameter);

/* Notes ---------------------------------------------------------------------------------- */
/*
TODO:
    o Init
        o Create run time task
    
    o Control
        o start and received update a predefined amount of time

SIGNAL FORM

                    |              |                               |                   |                      |
                    | Start: Host  |                               |                   |                      |
        ____________                _________             ___________           ________          _____________
                    \    1ms       / 20-40us \   80us    /  80us   | \  50us  / 26-28us \  50us  /   70us      \
                    |\____________/|          \_________/          |  \______/         | \______/             | \
                    |              |                               |                   |                      |
                    |              | Sensor Response               | Bit 1 (0)         | Bit 2 (1)            |


    1. Start signal from MCU
        o Pull data low for > 1ms
    2. Start response from sensor
        o After 20-40us, pull low for 80us
        o Pull up for 80us
    3. "1" Bit data from sensor
        o Pull low for 50us
        o Release for 70us
    4. "0" Bit data from sensor
        o Pull low for 50us
        p Release for 26-28us

DATA FORMAT
    o Data transmitted big-endian
        1. Humidity (1 byte)
        2. Humidity decimal (1 byte)
        3. Temp (1 byte)
        4. Temp decimal (1 byte) 
        o Checksum (1 byte) (low byte of 1 + 2 + 3 + 4)


*/

/* ---------------------------------------------------------------------------------------- */
/* @details Initialize the DHT22 control system
 * @param   None
 * @result  None
 */
void DHT22_Init(void)
{
    xTaskCreate(&DHT22_ControlTask, "DHT22_ControlTask", 2048, NULL, 1, NULL);
}

/* ---------------------------------------------------------------------------------------- */
/* @details The DHT22 control task routine
 * @param   pvParameter: pointer to argument passed from the task
 * @result  None
 */
static void DHT22_ControlTask(void *pvParameter)
{
    printf("Task 'DHT22_ControlTask' start.\n");
    while (1) 
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
        printf("Task 'DHT22_ControlTask' run.\n");

        //TODO: Start the DHT22 read 
    }
    fflush(stdout);
}