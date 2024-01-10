// ***************************************************** //
/// @file bsp_config.h
/// @brief Contains all pin definitions used by the application 
/// @version 0.1
// ***************************************************** //

#include <string.h>
#include "driver/gpio.h"

// General SPI bus pins
#define SPI_PIN_MISO           GPIO_NUM_19
#define SPI_PIN_MOSI           GPIO_NUM_23
#define SPI_PIN_CLK            GPIO_NUM_18

// MCP2515 specific pins
#define MCP_SPI_PIN_CS         GPIO_NUM_5
#define MCP_SPI_PIN_INTERRUPT  GPIO_NUM_21