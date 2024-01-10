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
#include "aws_iot.h"
#include "can.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

// --------------------------------------------------
// Local private variables and functions 
// --------------------------------------------------
#define APP_QUEUE_SIZE              (10)

static const char *TAG = "APP";

static TaskHandle_t  mainAppTask   = NULL;
static QueueHandle_t mainAppQueue  = NULL;

/// Locals function prototypes
static void application_task_function(void* pvParams);

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

            case EVENT_AWS_TOPIC_MSG:
                // Serialize and convert AWS json message to a format appropiate
                // to send to can device
                break;

            case EVENT_CAN_MSG:
            {
                // Deserialize and convert CAJ message to JSON and send to AWS
                CAN_frame frame;
                if (CAN_receive(&frame))
                {
                    ESP_LOGI(TAG, "[CAN MSG] ID=%d DLC=%d", frame.can_id, frame.can_dlc);
                    ESP_LOG_BUFFER_HEX(TAG, frame.data, frame.can_dlc);
                }

                break;
            }

            default:
                break;
        }
    }
}