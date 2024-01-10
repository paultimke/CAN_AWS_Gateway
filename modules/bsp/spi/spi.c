// ***************************************************** //
/// @file spi.c
/// @brief Serial Peripheral Inerface (SPI) 
/// @version 0.1
// ***************************************************** //

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include "spi.h"
#include "bsp_config.h"

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

// --------------------------------------------------------
// Public functions
// --------------------------------------------------------
bool SPI_init(void)
{
	esp_err_t ret;

	//Configuration for the SPI bus
	spi_bus_config_t bus_cfg={
		.miso_io_num = SPI_PIN_MISO,
		.mosi_io_num = SPI_PIN_MOSI,
		.sclk_io_num = SPI_PIN_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 0 // no limit
	};

	// Initialize SPI bus
	ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
	ESP_ERROR_CHECK(ret);

    return true;
}