// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include "wifi.h"
#include "wifi_credentials.h"

// Standard Includes
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ESP Includes
#include "esp_err.h"
#include "esp_interface.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

// FreeRTOS Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// --------------------------------------------------------
// Local private variables
// --------------------------------------------------------
typedef enum
{
    WIFI_SUCCESS,
    WIFI_FAILURE
} Wifi_outcomes_e;

static const char* HOST_NAME = "CAN-WIFI Gateway";
static const char* TAG = "WIFI";

static uint16_t retry_num = 0;
static const uint16_t MAX_FAILURES = 10;

// --------------------------------------------------------
// Local private functions
// --------------------------------------------------------
static void event_handler(
        void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Connecting to Access Point ...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (retry_num < MAX_FAILURES)
        {
            ESP_LOGI(TAG, 
                     "Attempting reconnection to AP. Retry num=%d/%d", 
                     retry_num,
                     MAX_FAILURES);

            retry_num++;
            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* ipEvent = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP obtained: " IPSTR, IP2STR(&ipEvent->ip_info.ip));
        retry_num = 0;
    }
    else
    {
        ESP_LOGW(TAG, "Unhandled event: %s:%d", event_base, event_id);
    }
}

static void wifi_connect()
{
    esp_netif_t*  netif = NULL;
    wifi_config_t wifi_config = {0};
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();

    // Initialize ESP network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    // Create event handler
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, 
                                               ESP_EVENT_ANY_ID, 
                                               &event_handler, 
                                               NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, 
                                               IP_EVENT_STA_GOT_IP, 
                                               &event_handler, 
                                               NULL));

    // Start the wifi interface
    strncpy((char*)wifi_config.sta.ssid, CRED_WIFI_SSID, CRED_WIFI_MAX_SSID_LEN);
    strncpy((char*)wifi_config.sta.password, CRED_WIFI_PASSWORD, CRED_WIFI_MAX_PASS_LEN);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_LOGI(TAG, "Starting Wifi STA. SSID = \"%s\"", CRED_WIFI_SSID);
    netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_set_hostname(netif, HOST_NAME));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE) );
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "STA Initialization complete");
}

// --------------------------------------------------------
// Public functions 
// --------------------------------------------------------
void wifi_init()
{
    // Initialize Non-volatile storage to save Wifi config
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES
        || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Try to connect to an Access point
    wifi_connect();
    return;
}
 
