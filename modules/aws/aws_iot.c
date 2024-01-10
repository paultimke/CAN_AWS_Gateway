// ***************************************************** //
/// @file aws_iot.c
/// @brief Handles communication with the AWS IoT MQTT Platform
/// @version 0.1
// ***************************************************** //

/// This module is based on the subscribe_publish_sample.c file from one
/// of the esp-aws-iot examples on release/v3.1.x
/// https://github.com/espressif/esp-aws-iot/tree/release/v3.1.x/examples/subscribe_publish

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include "aws_iot.h"
#include "application.h"
#include "rtos_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

// --------------------------------------------------------
// Local private variables
// --------------------------------------------------------
static const char *TAG = "AWS_IOT";

static AWS_IoT_Client client;

/// @brief Certificates for AWS. These are read from the files on certs directory 
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");

/// @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
char HostAddress[255] = "a2s77tqeeh4a21-ats.iot.us-east-1.amazonaws.com";

/// @brief Default MQTT port is pulled from the aws_iot_config.h
uint32_t port = AWS_IOT_MQTT_PORT;

// --------------------------------------------------------
// Local private functions
// --------------------------------------------------------

static void iot_subscribe_callback_handler(
    AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
    IoT_Publish_Message_Params *params, void *pData) 
{
    ESP_LOGI(TAG, "Subscribe callback");
    ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);

    main_app_event_t event;
    event.Type = EVENT_AWS_TOPIC_MSG;
    event.Data = NULL; // TODO put actual data here.
    application_sendEventFromIsr(event);
}

static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) 
{
    ESP_LOGW(TAG, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    // Notify the application of disconnection
    main_app_event_t event;
    event.Type = EVENT_AWS_DISCONNECTED;
    application_sendEvent(event);

    if(NULL == pClient) 
    {
        return;
    }

    if(aws_iot_is_autoreconnect_enabled(pClient)) 
    {
        ESP_LOGI(TAG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    } 
    else 
    {
        ESP_LOGW(TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if(NETWORK_RECONNECTED == rc) {
            ESP_LOGW(TAG, "Manual Reconnect Successful");
        } else {
            ESP_LOGW(TAG, "Manual Reconnect Failed - %d", rc);
        }
    }
}

static void aws_iot_task(void *param) 
{
    IoT_Error_t rc = FAILURE;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    ESP_LOGI(TAG, "AWS IoT task running ...");
    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;

    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        // TODO: Delete abort, and instead return from task and delete it
        abort();
    }

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = CONFIG_AWS_EXAMPLE_CLIENT_ID;
    connectParams.clientIDLen = (uint16_t) strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(TAG, "Connecting to AWS...");
    do {
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if(SUCCESS != rc) {
            ESP_LOGE(TAG, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while(SUCCESS != rc);

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
        abort();
    }

    ESP_LOGI(TAG, "Subscribing...");
    rc = aws_iot_mqtt_subscribe(&client, 
                                TOPIC_SUB, 
                                strlen(TOPIC_SUB), 
                                QOS0, 
                                iot_subscribe_callback_handler, 
                                NULL);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Error subscribing : %d ", rc);
        abort();
    }

    // Notify the application that connection was successful
    main_app_event_t event;
    event.Type = EVENT_AWS_CONNECTED;
    application_sendEvent(event);

    while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {

        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client, 100);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// --------------------------------------------------------
// Public functions
// --------------------------------------------------------
void aws_iot_publish(const char* payload)
{
    // If this doesn't work, try with QOS0
    IoT_Error_t rc = FAILURE;
    IoT_Publish_Message_Params paramsQOS1;

    // TODO: Payload must be a json-formated string. 
    // The application will be responsible to convert the CAN
    // messages into a JSON format before calling this function.

    paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *) payload;
    paramsQOS1.isRetained = 0;
    paramsQOS1.payloadLen = strlen(payload);

    rc = aws_iot_mqtt_publish(&client, TOPIC_PUB, strlen(TOPIC_PUB), &paramsQOS1);
    if (rc == MQTT_REQUEST_TIMEOUT_ERROR) 
    {
        ESP_LOGW(TAG, "QOS1 publish ack not received.");
        rc = SUCCESS;
    }
}

void aws_iot_task_start()
{
    xTaskCreate(&aws_iot_task, 
                "aws_iot_task", 
                AWS_TASK_STACK_SIZE, 
                NULL, 
                AWS_TASK_PRIORITY,
                NULL);
}
