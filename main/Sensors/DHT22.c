
/* Includes ------------------------------------------------------------------------------- */
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "DHT22.h"

/* Definitions ---------------------------------------------------------------------------- */
/* Types ---------------------------------------------------------------------------------- */
/* Variables ------------------------------------------------------------------------------ */
/* Function Prototypes -------------------------------------------------------------------- */
static void DHT22_ControlTask(void *pvParameter);
static void DHT22_ReadTemperatureHumidity(DHT22_TypeDef *dht);
static void DHT22_DecodeResults(DHT22_TypeDef *dht, rmt_item32_t *items, uint32_t itemCount);

/* Notes ---------------------------------------------------------------------------------- */
/*
TODO:
    o Upgrade to generic DHT device controlled externally

SIGNAL FORM

                    |              |                               |                   |                      |
                    | Start: Host  |                               |                   |                      |
        ____________                _________             ___________           ________          _____________
                    \    >1ms      / 20-40us \   80us    /  80us   | \  50us  / 26-28us \  50us  /   70us      \
                    |\____________/|          \_________/          |  \______/         | \______/             | \
                    |              |                               |                   |                      |
                    |              | Sensor: Response              | Bit 1 (0)         | Bit 2 (1)            |


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
DHT22_TypeDef* DHT22_Init(rmt_channel_t RMT_CHANNEL, uint8_t GPIO_PIN)
{
    //Initialize the DHT22 device
    DHT22_TypeDef *pDHT = (DHT22_TypeDef *)malloc(sizeof(DHT22_TypeDef));
    if (pDHT == NULL)
        return NULL;

    pDHT->rmt = RMT_CHANNEL;
    pDHT->pin = GPIO_PIN;
    pDHT->buff = NULL;
    pDHT->temperature = 0;
    pDHT->humidity = 0;
    pDHT->task = NULL;
    pDHT->updated = NULL;

    // Setup the rmt
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_RX;
    config.channel = pDHT->rmt;
    config.gpio_num = (gpio_num_t)pDHT->pin;
    config.mem_block_num = 1;
    config.rx_config.filter_en = true;
    config.rx_config.filter_ticks_thresh = 200;
    config.rx_config.idle_threshold = 1000;
    config.clk_div = 80;
    rmt_config(&config);
    rmt_driver_install(pDHT->rmt, 1000, 0);  // 400 words for ringbuffer containing pulse trains from DHT
    rmt_get_ringbuf_handle(pDHT->rmt, &pDHT->buff);

    //Setup GPIO
    gpio_set_direction(pDHT->pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pDHT->pin, 1);

    //Start the control task
    xTaskCreate(&DHT22_ControlTask, "DHT22_ControlTask", 2048, pDHT, 5, &pDHT->task);

    return pDHT;
}

/* ---------------------------------------------------------------------------------------- */
/* @details Start reading the temperature and humidity
 * @param   pDHT: pointer to the temperature struct
 * @param   updateCB: pointer to the update callback routine
 * @result  None
 */
void DHT22_StartUpdateTemperatureAndHumidity(DHT22_TypeDef *pDHT, DHT22_TemperatureAndHumidityUpdateCallback updateCB)
{
    pDHT->updated = updateCB;
    xTaskNotifyGive(pDHT->task);
}

/* ---------------------------------------------------------------------------------------- */
/* @details The DHT22 control task routine
 * @param   pvParameter: pointer to argument passed from the task
 * @result  None
 */
static void DHT22_ControlTask(void *pvParameter)
{
    DHT22_TypeDef *dht = (DHT22_TypeDef *)pvParameter;
    while (1) 
    {
        ulTaskNotifyTake(true, portMAX_DELAY);

        DHT22_ReadTemperatureHumidity(dht);

        //Notify updated
        if(dht->updated != NULL)
            dht->updated(dht);
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
    printf("DHT22: Reading temperature and humidity....\n");

    //Start rmt receive
    printf("DHT22: Start RMT\n");
    rmt_rx_start(dht->rmt, true);

    //Configure GPIO
    printf("DHT22: Pin low\n");
    gpio_set_direction(dht->pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(dht->pin, 0);

    //Start signal
    printf("DHT22: Wait.\n");
    uint32_t tick = xTaskGetTickCount();
    while((xTaskGetTickCount() - tick) < 2)
        vTaskDelay(portTICK_RATE_MS);

    //Configure GPIO
    printf("DHT22: Pin input\n");
    rmt_set_pin(dht->rmt, RMT_MODE_RX, dht->pin);
    gpio_set_direction(dht->pin, GPIO_MODE_INPUT);
    //rmt_set_gpio(dht->rmt, RMT_MODE_RX, (gpio_num_t)dht->pin, false);

    //Wait for completion
    size_t rx_size = 0;
    printf("DHT22: Awaiting completion\n");
    rmt_item32_t* items = (rmt_item32_t*)xRingbufferReceive(dht->buff, &rx_size, 1000);

    //Handle results
    printf("DHT22: Results: %u...\n", rx_size);
    if (items != NULL)
    {
        printf("Results: Decode\n");
        DHT22_DecodeResults(dht, items, rx_size/sizeof(rmt_item32_t));
        vRingbufferReturnItem(dht->buff, items);
    }

    //Clean up peripherals
    printf("RX: Reset peripherals\n");
    rmt_rx_stop(dht->rmt);
    gpio_set_direction(dht->pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(dht->pin, 1);
    
    printf("Reading temperature and humidity: Complete\n");
}

/* ---------------------------------------------------------------------------------------- */
/* @details Decode the RMT items to DHT22 values
 * @param   dht: Pointer to the DHT22 device
 * @param   items: Pointer to the items to decode
 * @param   itemCount: Number of items
 * @result  None
 */
static void DHT22_DecodeResults(DHT22_TypeDef *dht, rmt_item32_t* items, uint32_t itemCount) 
{
    printf("Decode: %u items\n", itemCount);
}