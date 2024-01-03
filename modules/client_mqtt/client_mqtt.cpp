// --------------------------------------------------------
// Includes
// --------------------------------------------------------

#include "client_mqtt.h"

// Standard Includes
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ESP Includes
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"

// FreeRTOS Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// --------------------------------------------------------
// Local private variables and functions
// --------------------------------------------------------
static esp_mqtt_client_handle_t client;

//static const char *AWS_PUB_TOPIC = "AWS/esp32_pub";
//static const char *AWS_SUB_TOPIC = "AWS/esp32_sub";

static const char *TAG = "CLIENT_MQTT";

/// @brief Log message to STDOUT if error code is not zero
static void log_error_if_nonzero(const char * message, int error_code) 
{
    if (error_code != 0) 
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/// @brief Callback function for MQTT events
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            // TODO: Here I'll put application_send_event() once I write
            // it to notify the application thread of incoming data from
            // subscribed topics
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", 
                                        event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", 
                                        event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  
                                        event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", 
                                        strerror(event->error_handle->esp_transport_sock_errno));

            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(
    void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb((esp_mqtt_event_handle_t) event_data);
}

// --------------------------------------------------------
// Public functions 
// --------------------------------------------------------
void mqtt_start_thread(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://test.mosquitto.org:1883/",
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_LOGI(TAG, "Client pointer val %p", client);

    esp_mqtt_client_register_event(client, 
                                   (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID, 
                                   mqtt_event_handler, 
                                   client);
    ESP_LOGI(TAG, "Client pointer val %p", client);

    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "Client pointer val %p", client);
}

bool mqtt_publish(const char *msg, const char *topic)
{
    const int QOS = 1;
    const int RETAIN = 0;

    int8_t err = 0;
    esp_mqtt_client_publish(client,
                            topic,
                            msg,
                            strlen(msg),
                            QOS,
                            RETAIN);

    log_error_if_nonzero("Publish action FAILED", err);
    return (err != -1);
}

bool mqtt_subscribe(const char *topic)
{
    const int QOS = 1;

    ESP_LOGI(TAG, "Checking if here's the error. Client addr = %p", client);
    int8_t err = esp_mqtt_client_subscribe(client, topic, QOS);

    log_error_if_nonzero("Subscription to topic FAILED", err);
    return (err != -1);
}

bool mqtt_unsubscribe(const char *topic)
{
    int err = esp_mqtt_client_unsubscribe(client, topic);

    log_error_if_nonzero("Unsubscribe action FAILED", err);
    return (err != -1);
} 
