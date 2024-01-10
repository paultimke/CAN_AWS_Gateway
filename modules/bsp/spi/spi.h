// ***************************************************** //
/// @file spi.h
/// @brief Serial Peripheral Inerface (SPI) driver
/// @version 0.1
// ***************************************************** //

#ifndef _SPI_H_
#define _SPI_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include <stdbool.h>

// --------------------------------------------------------
// Public functions
// --------------------------------------------------------
bool SPI_init(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _SPI_H_
