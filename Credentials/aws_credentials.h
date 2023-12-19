// ****************************************************************************
// CAN-WIFI Gateway to AWS cloud
// ****************************************************************************
// This file is meant to be edited to add user-specific AWS credentials
// before compilation of the project.

#ifndef _AWS_CREDENTIALS_H_
#define _AWS_CREDENTIALS_H_

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include <pgmspace.h>

// --------------------------------------------------------
// Constants and variables
// --------------------------------------------------------

/// @brief EDIT TO ENTER AWS IoT Core thing name
const char THING_NAME[]       = "*************************"; // change this

/// @brief 
const char AWS_IOT_ENDPOINT[] = "*************************"; // change this

/// @brief EDIT TO ENTER Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
 
-----END CERTIFICATE-----
)EOF";

/// @brief EDIT TO ENTER Device Certificate                 // change this
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
 
-----END CERTIFICATE-----


)KEY";

/// @brief EDIT TO ENTER Device Private Key                 // change this
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
 
-----END RSA PRIVATE KEY-----
 
 
)KEY";

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _AWS_CREDENTIALS_H_
