// ***************************************************** //
/// @file application.h
/// @brief Main application code
/// @version 0.1
// ***************************************************** //

#ifndef _APP_H_
#define _APP_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// --------------------------------------------------
// Type definitions
// --------------------------------------------------

typedef enum
{
    EVENT_START,
    EVENT_STOP,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_AWS_CONNECTED,
    EVENT_AWS_DISCONNECTED,
    EVENT_AWS_TOPIC_MSG,
    EVENT_CAN_MSG
} event_type_e;

typedef struct
{
    event_type_e Type;
    void*        Data;
} main_app_event_t;

// --------------------------------------------------
// Public functions 
// --------------------------------------------------

void application_start(void);
void application_sendEvent(main_app_event_t event);
void application_sendEventFromIsr(main_app_event_t event);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _APP_H_