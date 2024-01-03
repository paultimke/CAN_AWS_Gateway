#ifndef _AWS_IOT_H_
#define _AWS_IOT_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// --------------------------------------------------------
// Public definitions
// --------------------------------------------------------

#define CONFIG_AWS_EXAMPLE_CLIENT_ID "ESP32_GW"

/// @brief Starts the AWS task and connects to the MQTT broker
void aws_iot_task_start();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _AWS_IOT_H_