// ***************************************************** //
/// @file can.h
/// @brief Controller Area Network Interface
/// @version 0.1
// ***************************************************** //

#ifndef _CAN_H_
#define _CAN_H_

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include <stdbool.h>
#include "mcp2515.h"

// --------------------------------------------------------
// Interface
// --------------------------------------------------------
typedef MCP_CAN_frame CAN_frame_t;

/// @brief Initializes CAN communication and configures
/// interrupt to send application events on each received 
/// message
/// @param None  
/// @return true if successful, false otherwise 
bool CAN_init(void);

/// @brief Reads message from MCP2515 controller. Use this
/// function when receiving an interrupt indicating a new 
/// message arrives. Otherwise, it will read old messages
/// @param frame Pointer to a CAN frame to place data
/// @return true if successful, false otherwise 
bool CAN_receive(CAN_frame_t* frame);

#endif // _CAN_H_
