#ifndef _AWS_IOT_H_
#define _AWS_IOT_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// --------------------------------------------------------
// Constants
// --------------------------------------------------------
/// AWS IoT 'Thing' name
#define CONFIG_AWS_EXAMPLE_CLIENT_ID "ESP32_GW"

/// @brief MQTT Topics. These topics were created in the policy
/// of the AWS Thing. Changing them could break things.
#define TOPIC_SUB  "AWS/esp32_sub"
#define TOPIC_PUB  "AWS/esp32_pub"

// --------------------------------------------------------
// Public definitions
// --------------------------------------------------------

/// @brief Starts the AWS task and connects to the MQTT broker
void aws_iot_task_start();

/// @brief Publishes message to TOPIC_PUB with MQTT
/// @param payload message to be published
void aws_iot_publish(const char* payload);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _AWS_IOT_H_