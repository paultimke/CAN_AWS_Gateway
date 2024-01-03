// Standard includes
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ESP32 includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"

// Application includes
#include "application.h"
#include "wifi.h"
#include "aws_iot.h"

static const char *TAG = "MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    // Set log levels for all modules 
    esp_log_level_set("CLIENT_MQTT", ESP_LOG_DEBUG);
    esp_log_level_set("WIFI", ESP_LOG_DEBUG);
    
    application_start();
    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(10000));
}