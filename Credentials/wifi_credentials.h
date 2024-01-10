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
#define CRED_WIFI_MAX_SSID_LEN (26)
#define CRED_WIFI_MAX_PASS_LEN (26)

/// @brief EDIT TO ENTER NETWORK SSID
const char CRED_WIFI_SSID[CRED_WIFI_MAX_SSID_LEN] = "";

/// @brief EDIT TO ENTER NETWORK WIFI PASSWORD
const char CRED_WIFI_PASSWORD[CRED_WIFI_MAX_PASS_LEN] = "";


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _WIFI_CREDENTIALS_H_
