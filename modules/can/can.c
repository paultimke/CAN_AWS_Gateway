// ***************************************************** //
/// @file spi.c
/// @brief Serial Peripheral Inerface (SPI) 
/// @version 0.1
// ***************************************************** //

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include "can.h"
#include "spi.h"
#include "application.h"
#include "bsp_config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"

// --------------------------------------------------------
// Local private functions
// --------------------------------------------------------
static const char* TAG = "CAN";

static void IRAM_ATTR isr_handler(void *args)
{
    main_app_event_t event;
    event.Type = EVENT_CAN_MSG;
    application_sendEventFromIsr(event);
}

/// @brief Initializes GPIO to configure MCP2515 interrupt
/// @param None 
static void GPIO_init(void)
{
	gpio_pad_select_gpio(MCP_SPI_PIN_INTERRUPT);
	gpio_set_direction(MCP_SPI_PIN_INTERRUPT, GPIO_MODE_INPUT);
	gpio_pulldown_en(MCP_SPI_PIN_INTERRUPT);
	gpio_pulldown_dis(MCP_SPI_PIN_INTERRUPT);
	gpio_set_intr_type(MCP_SPI_PIN_INTERRUPT, GPIO_INTR_NEGEDGE);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(MCP_SPI_PIN_INTERRUPT, isr_handler, NULL);
}

// --------------------------------------------------------
// Public functions
// --------------------------------------------------------
bool CAN_init(void)
{
    GPIO_init();
    SPI_init();

    MCP_ERROR_t ret = ERROR_FAIL;
    ret = MCP2515_init();
    if (ERROR_OK != ret)
    {
        return false;
    }

    ret = MCP2515_reset();
    if (ERROR_OK != ret)
    {
        return false;
    }

    ret = MCP2515_setBitrate(CAN_100KBPS, MCP_8MHZ);
    if (ERROR_OK != ret)
    {
        return false;
    }

    ret = MCP2515_setNormalMode();
    if (ERROR_OK != ret)
    {
        return false;
    }

    ESP_LOGI(TAG, "Initialized successfully");
    return true;
}

bool CAN_receive(CAN_frame_t* frame)
{
    MCP_ERROR_t ret = ERROR_FAIL;
    ret = MCP2515_readMessage(RXB0, &frame);

    return (ERROR_OK == ret);
}