// ***************************************************** //
/// @file application.cpp
/// @brief Main application code
/// @version 0.1
/// @date 2023-12-27
// ***************************************************** //

// --------------------------------------------------
// Includes
// --------------------------------------------------
#include "application.h"
#include "rtos_config.h"
#include "wifi.h"
#include "aws_iot.h"
#include "can_bus.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// --------------------------------------------------
// Local private variables and functions 
// --------------------------------------------------
#define MAX_JSON_MSG_LEN            (80)
#define APP_QUEUE_SIZE              (10)

static bool is_AWS_connected = false;

static const char *TAG = "APP";

static TaskHandle_t  mainAppTask   = NULL;
static QueueHandle_t mainAppQueue  = NULL;

/// Locals function prototypes
static void application_task_function(void* pvParams);
static void construct_JSON_CAN_msg(char msg[MAX_JSON_MSG_LEN], const CAN_frame_t& frame);

// --------------------------------------------------
// Public functions 
// --------------------------------------------------
void application_start(void)
{
    ESP_LOGI(TAG, "Creating main application objects");

    // Create queue
    mainAppQueue = xQueueCreate(APP_QUEUE_SIZE, sizeof(main_app_event_t));

    // Create task
    xTaskCreate(application_task_function, 
                "app_task", 
                APP_TASK_STACK_SIZE, 
                NULL, 
                APP_TASK_PRIORITY, 
                &mainAppTask);
}

void application_sendEvent(main_app_event_t event)
{
    xQueueSend(mainAppQueue, &event, 0);
}

void application_sendEventFromIsr(main_app_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(mainAppQueue, &event, &xHigherPriorityTaskWoken);

    // If a lower priority was set to hold, force a context switch
    if (xHigherPriorityTaskWoken != pdFALSE)
    {
        portYIELD_FROM_ISR();
    }
}

// --------------------------------------------------
// Local private functions 
// --------------------------------------------------
static void application_task_function(void* pvParameters)
{
    main_app_event_t event;

    ESP_LOGI(TAG, "Main application thread running.");

    // Initialize modules needed by the application
    wifi_init();
    if (!CAN_init())
    {
        ESP_LOGE(TAG, "Could not initialize CAN module");
    }

    // Main application event loop
    while (true)
    {
        if (xQueueReceive(mainAppQueue, (void*)&event, portMAX_DELAY) != pdTRUE)
        {
            // Failed to receive from queue, should never happen
            continue;
        }

        switch (event.Type)
        {
            case EVENT_START:
                ESP_LOGI(TAG, "Resumed gateway application. Communicating CAN devices with AWS");
                break;

            case EVENT_STOP:
                ESP_LOGI(TAG, "Suspending gateway application. Entering idle state");
                break;
            
            case EVENT_WIFI_CONNECTED:
                ESP_LOGI(TAG, "WIFI Connection successful");
                // Connect to AWS host as soon as Wifi is established
                aws_iot_task_start(); 
                break;

            case EVENT_AWS_CONNECTED:
                ESP_LOGI(TAG, "AWS Connection successful");
                is_AWS_connected = true;
                break;

            case EVENT_AWS_DISCONNECTED:
                ESP_LOGI(TAG, "AWS disconnected");
                is_AWS_connected = false;
                break;

            case EVENT_AWS_TOPIC_MSG:
                // Serialize and convert AWS json message to a format appropiate
                // to send to can device
                break;

            case EVENT_CAN_MSG:
            {
                ESP_LOGD(TAG, "EVENT_CAN_MSG");

                // Convert CAN message to JSON and send to AWS
                CAN_frame_t frame;
                if (CAN_receive(&frame))
                {
                    ESP_LOGD(TAG, "[CAN MSG] ID=%d DLC=%d", frame.can_id, frame.can_dlc);
                    ESP_LOG_BUFFER_HEX_LEVEL(TAG, frame.data, frame.can_dlc, ESP_LOG_DEBUG);

                    if (is_AWS_connected)
                    {
                        char msg[MAX_JSON_MSG_LEN];
                        construct_JSON_CAN_msg(msg, frame);

                        ESP_LOGI(TAG, "Sending to AWS: %s", msg);
                        aws_iot_publish(msg);
                    }
                }
                break;
            }

            default:
                break;
        }
    }
}

static void construct_JSON_CAN_msg(char msg[MAX_JSON_MSG_LEN], const CAN_frame_t& frame)
{
    char data_str[30];

    // Translate the frame's hex data into a string
    int buf_idx = 0;
    for (int i = 0; i < frame.can_dlc; i++) {
        sprintf(&data_str[buf_idx], "%02x ", frame.data[i]);
        buf_idx += 3;
    } 

    // Construct the JSON message
    sprintf(msg, 
            "{\n\t\"id\": \"%d\",\n\t \"dlc\": \"%d\",\n\t\"data\": \"%s\"\n}",
            frame.can_id, frame.can_dlc, data_str);
}