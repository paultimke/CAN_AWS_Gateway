// ****************************************************************************
// CAN-WIFI Gateway to AWS cloud
// ****************************************************************************
// This file is meant to be edited to add user-specific Wifi credentials
// before compilation of the project.

#ifndef _WIFI_CREDENTIALS_H_
#define _WIFI_CREDENTIALS_H_

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

// --------------------------------------------------------
// Constants and variables
// --------------------------------------------------------

/// @brief EDIT TO ENTER NETWORK SSID
const char WIFI_SSID[]        = "*************************"; // change this

/// @brief EDIT TO ENTER NETWORK WIFI PASSWORD
const char WIFI_PASSWORD[]    = "*************************"; // change this


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _WIFI_CREDENTIALS_H_
