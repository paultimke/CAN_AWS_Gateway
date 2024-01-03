#ifndef _CLIENT_MQTT_H_
#define _CLIENT_MQTT_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// --------------------------------------------------------
// Includes
// --------------------------------------------------------

// --------------------------------------------------------
// Public definitions
// --------------------------------------------------------
void mqtt_start_thread(void);
bool mqtt_publish(const char *msg, const char *topic);
bool mqtt_subscribe(const char *topic);
bool mqtt_unsubscribe(const char *topic);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _CLIENT_MQTT_H_
