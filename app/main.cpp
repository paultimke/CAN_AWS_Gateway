// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "application.h"

// --------------------------------------------------------
// Main function
// --------------------------------------------------------
static const char *TAG = "MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    // Set log levels for all modules 
    esp_log_level_set("APP", ESP_LOG_DEBUG);
    esp_log_level_set("WIFI", ESP_LOG_DEBUG);
    esp_log_level_set("AWS_IOT", ESP_LOG_DEBUG);
    
    application_start();
}