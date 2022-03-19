/**
* Company: 
* Engineer:      Leandro Santos
* Create Date:   12/03/2021 
* Design Name:   SNTP_Time_sync

* Target Devices: ESP32
* Tool versions:  ESP-IDF(v4.3.1) 
* Description: SNTP Time Synchronization implementation according ESP-IDF Programming Guide
*
* Dependencies: WiFi
*
* Revision: 
* Revision 0.01 - File Created
* Additional Comments: 
 **/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "nvs_flash.h"
#include "esp_spi_flash.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "wifi_app.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define strftime_buf_size (64)

/********************************************************************************************
*                           Application data definition section                             *
*********************************************************************************************/
static const char *TAG = "MAIN_APP";

//Tasks
TaskHandle_t dSNTP_get_time_task;

// wifi connection constants and variables
uint8_t base_mac_addr[6];
char base_mac_addr_value[128];

// SNTP Time syncronization variables
time_t now;
struct tm timeinfo;
char strftime_buf[strftime_buf_size];


/********************************************************************************************
*                               Handles and Function prototypes                             *
*********************************************************************************************/ 
/**
 * @brief SNTP internal callback function
 * 
 * @param tv - LWIP timeval private structure (tv_sec, tv_usec)
 */
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

/**
 * @brief Initialize SNTP service
 * 
 */
void sntp_init_app_main(void)
{
    int retry = 0;
    const int retry_count = 10;

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();

    // wait for time to be set
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Get the sntp time object
 * 
 * @param strftime_buf - full char array with current date / time for Sao Paulo(BRA) timezone  
 * @param now          - Time structure based ESP-IDF device 
 * @param timeinfo     - SNTP time data structure
 */
void get_sntp_time(char *strftime_buf, time_t *now, struct tm *timeinfo)
{
    time(now);

    // Set timezone to São Paulo Standard Time
    setenv("TZ", "BRST+3BRDT+2,M10.3.0,M2.3.0", 1);
    tzset();
 
    localtime_r(now, timeinfo);
    strftime(strftime_buf, strftime_buf_size, "%c", timeinfo);
    ESP_LOGI(TAG, "The current date/time in Sao Paulo(BRA) is: %s", strftime_buf);
}

/**
 * @brief SNTP get Time Synchronization task
 * 
 * @param pvParameter - FreeRTOS default void variable
 */
void vSNTP_get_time_task(void *pvParameter)
{
    while (true)
    {    
        get_sntp_time(&strftime_buf[strftime_buf_size], &now, &timeinfo);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/********************************************************************************************
*                                   MAIN APPLICATION                                        *
*********************************************************************************************/ 
void app_main(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);

    ESP_LOGI(TAG, "%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
        
    // Get MAC address for WiFi Station interface 
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(base_mac_addr));
    
    memset(base_mac_addr_value, 0, 128);
    snprintf (base_mac_addr_value, 128, "%X%X%X%X%X%X",base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);

    ESP_LOGI(TAG, "Using \"0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\" as base MAC address\n",
             base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);
    
    ESP_LOGI(TAG, "base_mac_addr_value => %s\n",base_mac_addr_value);
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    
    // Initialize WiFi connection
    wifi_init_app_main();
    
    // Initialize and sync data / time by ESP-IDF System Time (SNTP)
    sntp_init_app_main();

    if (xTaskCreate(&vSNTP_get_time_task, "vSNTP_get_time_task", configMINIMAL_STACK_SIZE + 2048, NULL, 1, dSNTP_get_time_task) != pdTRUE)
    {
        ESP_LOGE("ERROR", "*** vSNTP_get_time_task taskCreate error process ***\n");
    }  
}
