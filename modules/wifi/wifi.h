#ifndef _WIFI_STA_H_
#define _WIFI_STA_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include <stdbool.h>

// --------------------------------------------------------
// Public definitions
// --------------------------------------------------------

/// @brief Initializes Wifi, connects with the lwIP stack
/// and starts the Wifi thread.
/// @return True if successful, False otherwise
void wifi_init(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _WIFI_STA_H_
