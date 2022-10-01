
/* Includes ------------------------------------------------------------------------------- */
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/rmt.h"
#include "driver/gpio.h"

/* Definitions ---------------------------------------------------------------------------- */
#define DHT22_UPDATEPERIOD                      10000        //Update every 10 seconds

/* Types ---------------------------------------------------------------------------------- */
typedef struct
{
    rmt_channel_t rmt;
    uint8_t pin;
    RingbufHandle_t buff;

    //Results
    int32_t temperature;
    int32_t humidity;
}DHT22_TypeDef;

/* Variables ------------------------------------------------------------------------------ */
static DHT22_TypeDef dht22 = 
{
    RMT_CHANNEL_0, 4, NULL, 0, 0
};

/* Function Prototypes -------------------------------------------------------------------- */
static void DHT22_ControlTask(void *pvParameter);
static void DHT22_ReadTemperatureHumidity(DHT22_TypeDef *dht);

/* Notes ---------------------------------------------------------------------------------- */
/*
TODO:
    o Upgrade to generic DHT device controlled externally

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
    //Initialize the DHT22 device

    //Setup the rmt
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_RX;
    config.channel = dht22.rmt;
    config.gpio_num = (gpio_num_t)dht22.pin;
    config.mem_block_num = 2;
    config.rx_config.filter_en = 1;
    config.rx_config.filter_ticks_thresh = 10;
    config.rx_config.idle_threshold = 1000;
    config.clk_div = 80;
    rmt_config(&config);
    rmt_driver_install(dht22.rmt, 400, 0);  // 400 words for ringbuffer containing pulse trains from DHT
    rmt_get_ringbuf_handle(dht22.rmt, &dht22.buff);

    //Setup GPIO
    gpio_set_direction(dht22.pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(dht22.pin, 1);

    //Start the control task
    xTaskCreate(&DHT22_ControlTask, "DHT22_ControlTask", 2048, &dht22, 1, NULL);
}

/* ---------------------------------------------------------------------------------------- */
/* @details The DHT22 control task routine
 * @param   pvParameter: pointer to argument passed from the task
 * @result  None
 */
static void DHT22_ControlTask(void *pvParameter)
{
    printf("Task 'DHT22_ControlTask' start.\n");
    DHT22_TypeDef *dht = (DHT22_TypeDef *)pvParameter;
    static uint32_t tickval = 0;
    while (1) 
    {

        //TESTING
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(dht22.pin, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(dht22.pin, 1);
        continue;

        tickval = xTaskGetTickCount();

        DHT22_ReadTemperatureHumidity(dht);

        //Delay till next loop
        /*  
            EQUATION 1
            timeTotal = DHT22_UPDATEPERIOD  //ms

            EQUATION 2
            timePassed = (xTaskGetTickCount() - tickval) * portTICK_RATE_MS  //ms

            EQUATION 3
            time = timeTotal - timePassed   //ms
                time = DHT22_UPDATEPERIOD - ((xTaskGetTickCount() - tickval) * portTICK_RATE_MS)

            EQUATION 4
            delay = time / portTICK_RATE_MS
                delay = (DHT22_UPDATEPERIOD - ((xTaskGetTickCount() - tickval) * portTICK_RATE_MS))/ portTICK_RATE_MS
                delay = (DHT22_UPDATEPERIOD / portTICK_RATE_MS) - xTaskGetTickCount() + tickval
            */
        vTaskDelay((DHT22_UPDATEPERIOD / portTICK_RATE_MS) - xTaskGetTickCount() + tickval);
    }
    fflush(stdout);
}

/* ---------------------------------------------------------------------------------------- */
/* @details Read the temperature and humidity
 * @param   dht: Pointer to the DHT22 device
 * @result  None
 */
static void DHT22_ReadTemperatureHumidity(DHT22_TypeDef *dht) 
{
    printf("Reading temperature and humidity....\n");

    //Configure pin as output

    //Drive pin low for 2ms

    //Configre pin as RMT

    //Configure rmt receive

    //Wait for completion

    //Handle results
}