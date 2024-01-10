// ***************************************************** //
/// @file mcp2515.c
/// @brief MCP2515 CAN controller device drvier
/// @version 0.1
// ***************************************************** //

/// This driver is based on the driver by Microver-Electronics
/// https://github.com/Microver-Electronics/mcp2515-esp32-idf/tree/main

// --------------------------------------------------------
// Includes
// --------------------------------------------------------
#include <string.h>
#include "mcp2515.h"
#include "bsp_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/spi_master.h"

// --------------------------------------------------------
// Local private variables
// --------------------------------------------------------
// special address description flags for the CAN_ID 
#define CAN_EFF_FLAG 0x80000000UL // EFF/SFF is set in the MSB 
#define CAN_RTR_FLAG 0x40000000UL // remote transmission request 
#define CAN_ERR_FLAG 0x20000000UL // error message frame 

// valid bits in CAN ID for frame formats 
#define CAN_SFF_MASK 0x000007FFUL // standard frame format (SFF) 
#define CAN_EFF_MASK 0x1FFFFFFFUL // extended frame format (EFF) 
#define CAN_ERR_MASK 0x1FFFFFFFUL // omit EFF, RTR, ERR flags 

#define CAN_SFF_ID_BITS     11
#define CAN_EFF_ID_BITS     29

#define MCP_SPI_PIN_MISO       SPI_PIN_MISO
#define MCP_SPI_PIN_MOSI       SPI_PIN_MOSI
#define MCP_SPI_PIN_CLK        SPI_PIN_CLK

#define CANCTRL_REQOP         (0xE0)
#define CANCTRL_ABAT          (0x10)
#define CANCTRL_OSM           (0x08)
#define CANCTRL_CLKEN         (0x04)
#define CANCTRL_CLKPRE        (0x03)

#define CANSTAT_OPMOD         (0xE0)
#define CANSTAT_ICOD          (0x0E)

#define CNF3_SOF              (0x80)

#define TXB_EXIDE_MASK        (0x08)
#define DLC_MASK              (0x0F)
#define RTR_MASK              (0x40)

#define RXBnCTRL_RXM_STD      (0x20)
#define RXBnCTRL_RXM_EXT      (0x40)
#define RXBnCTRL_RXM_STDEXT   (0x00)
#define RXBnCTRL_RXM_MASK     (0x60)
#define RXBnCTRL_RTR          (0x08)
#define RXB0CTRL_BUKT         (0x04)
#define RXB0CTRL_FILHIT_MASK  (0x03)
#define RXB1CTRL_FILHIT_MASK  (0x07)
#define RXB0CTRL_FILHIT       (0x00)
#define RXB1CTRL_FILHIT       (0x01)

static const uint8_t MCP_SIDH = 0;
static const uint8_t MCP_SIDL = 1;
static const uint8_t MCP_EID8 = 2;
static const uint8_t MCP_EID0 = 3;
static const uint8_t MCP_DLC  = 4;
static const uint8_t MCP_DATA = 5;

static const uint8_t STAT_RXIF_MASK = STAT_RX0IF | STAT_RX1IF;
static const uint8_t EFLG_ERRORMASK = EFLG_RX1OVR
									| EFLG_RX0OVR
									| EFLG_TXBO
									| EFLG_TXEP
									| EFLG_RXEP;

typedef struct MCP2515_s{
	MCP_ERROR_t ERROR;
	MASK_t  MASK;
	RXF_t   RXF;
	RXBn_t  RXBn;
	TXBn_t  TXBn;
	CANINTF_t CANINTF;
	EFLG_t    EFLG_t;
	CANCTRL_REQOP_MODE_t CANCTRL_REQOP_MODE;
	STAT_t STAT;
	TXBnCTRL_t    TXBnCTRL;
	INSTRUCTION_t INSTRUCTION;
	REGISTER_t    REGISTER;

	TXBn_REGS TXB_ptr;
	RXBn_REGS RXB_ptr;

	spi_device_handle_t spi;
} MCP2515_t[1], *MCP2515;

MCP2515 MCP2515_Object = NULL;

static const char* TAG = "MCP2515";

// --------------------------------------------------------
// Local private functions
// --------------------------------------------------------

// Prototypes
MCP_ERROR_t MCP2515_setMode(const CANCTRL_REQOP_MODE_t mode);
uint8_t MCP2515_readRegister(const REGISTER_t reg);
void MCP2515_readRegisters(const REGISTER_t reg, uint8_t values[], const uint8_t n);
void MCP2515_setRegister(const REGISTER_t reg, const uint8_t value);
void MCP2515_setRegisters(const REGISTER_t reg, const uint8_t values[], const uint8_t n);
void MCP2515_modifyRegister(const REGISTER_t reg, const uint8_t mask, const uint8_t data);
void MCP2515_prepareId(uint8_t *buffer, const bool ext, const uint32_t id);

MCP_ERROR_t MCP2515_setMode(const CANCTRL_REQOP_MODE_t mode)
{
	MCP2515_modifyRegister(MCP_CANCTRL, CANCTRL_REQOP, mode);

    bool modeMatch = false;

    for (int i = 0; i < 10; i++) {
        uint8_t newmode = MCP2515_readRegister(MCP_CANSTAT);
        newmode &= CANSTAT_OPMOD;

        modeMatch = newmode == mode;

        if (modeMatch) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return modeMatch? ERROR_OK : ERROR_FAIL;
}


uint8_t MCP2515_readRegister(const REGISTER_t reg)
{
    spi_transaction_t trans = {};

    trans.length = 24;
    trans.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    trans.tx_data[0] = INSTRUCTION_READ;
    trans.tx_data[1] = reg;
    trans.tx_data[2] = 0x00;

    esp_err_t ret = spi_device_transmit(MCP2515_Object->spi, &trans);
    if (ret != ESP_OK) {
        printf("spi_device_transmit failed\n");
    }

    return trans.rx_data[2];
}

void MCP2515_readRegisters(const REGISTER_t reg, uint8_t values[], const uint8_t n)
{
    uint8_t rx_data[n + 2];
    uint8_t tx_data[n + 2];

    tx_data[0] = INSTRUCTION_READ;
    tx_data[1] = reg;

    spi_transaction_t trans = {};

    trans.length = ((2 + ((size_t)n)) * 8);
    trans.rx_buffer = rx_data;
    trans.tx_buffer = tx_data;

    esp_err_t ret = spi_device_transmit(MCP2515_Object->spi, &trans);
    if (ret != ESP_OK) {
        printf("spi_device_transmit failed\n");
    }

    for (uint8_t i = 0; i < n; i++) {
        values[i] = rx_data[i+2];
    }
}

void MCP2515_setRegister(const REGISTER_t reg, const uint8_t value)
{
    spi_transaction_t trans = {};
    trans.length = 24;
    trans.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    trans.tx_data[0] = INSTRUCTION_WRITE;
    trans.tx_data[1] = reg;
    trans.tx_data[2] = value;

    esp_err_t ret = spi_device_transmit(MCP2515_Object->spi, &trans);
    if (ret != ESP_OK) {
        printf("spi_device_transmit failed\n");
    }
}

void MCP2515_setRegisters(const REGISTER_t reg, const uint8_t values[], const uint8_t n)
{
    uint8_t data[n + 2];

    data[0] = INSTRUCTION_WRITE;
    data[1] = reg;

    for (uint8_t i=0; i<n; i++) {
        data[i+2] = values[i];
    }

    spi_transaction_t trans = {};

    trans.length = ((2 + ((size_t)n)) * 8);
    trans.tx_buffer = data;

    esp_err_t ret = spi_device_transmit(MCP2515_Object->spi, &trans);
    if (ret != ESP_OK) {
        printf("spi_device_transmit failed\n");
    }
}

void MCP2515_modifyRegister(const REGISTER_t reg, const uint8_t mask, const uint8_t data)
{
    spi_transaction_t trans = {};

    trans.length = 32;
    trans.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    trans.tx_data[0] = INSTRUCTION_BITMOD;
    trans.tx_data[1] = reg;
    trans.tx_data[2] = mask;
    trans.tx_data[3] = data;

    esp_err_t ret = spi_device_transmit(MCP2515_Object->spi, &trans);
    if (ret != ESP_OK) {
        printf("spi_device_transmit failed\n");
    }
}

void MCP2515_prepareId(uint8_t *buffer, const bool ext, const uint32_t id)
{
    uint16_t canid = (uint16_t)(id & 0x0FFFF);

    if (ext) 
    {
        buffer[MCP_EID0] = (uint8_t) (canid & 0xFF);
        buffer[MCP_EID8] = (uint8_t) (canid >> 8);
        canid = (uint16_t)(id >> 16);
        buffer[MCP_SIDL] = (uint8_t) (canid & 0x03);
        buffer[MCP_SIDL] += (uint8_t) ((canid & 0x1C) << 3);
        buffer[MCP_SIDL] |= TXB_EXIDE_MASK;
        buffer[MCP_SIDH] = (uint8_t) (canid >> 5);
    } 
    else 
    {
        buffer[MCP_SIDH] = (uint8_t) (canid >> 3);
        buffer[MCP_SIDL] = (uint8_t) ((canid & 0x07 ) << 5);
        buffer[MCP_EID0] = 0;
        buffer[MCP_EID8] = 0;
    }
}


// --------------------------------------------------------
// Public functions 
// --------------------------------------------------------

MCP_ERROR_t MCP2515_init()
{
	// MEMORY ALLOCATIONS FOR MCP2515 STRUCTURE
	MCP2515_Object = (MCP2515)malloc(sizeof(MCP2515_t[1]));
	if(MCP2515_Object == NULL)
    {
		ESP_LOGE(TAG, "Cannot initialize MCP2515. No memory available");
		return ERROR_FAIL;
	}

	MCP2515_Object->TXB_ptr = NULL;
	MCP2515_Object->RXB_ptr = NULL;
	MCP2515_Object->TXB_ptr = (TXBn_REGS)malloc(sizeof(TXBn_REGS_t[N_TXBUFFERS]));
	MCP2515_Object->RXB_ptr = (RXBn_REGS)malloc(sizeof(RXBn_REGS_t[N_RXBUFFERS]));

	if(MCP2515_Object->TXB_ptr == NULL || MCP2515_Object->RXB_ptr == NULL)
    {
		ESP_LOGE(TAG, "Couldn't initialize MCP2515");
		return ERROR_FAIL;
	}

	// TXBn and RXBn Register Initialization 
	MCP2515_Object->TXB_ptr[0].CTRL = MCP_TXB0CTRL;
	MCP2515_Object->TXB_ptr[0].DATA = MCP_TXB0DATA;
	MCP2515_Object->TXB_ptr[0].SIDH = MCP_TXB0SIDH;

	MCP2515_Object->TXB_ptr[1].CTRL = MCP_TXB1CTRL;
	MCP2515_Object->TXB_ptr[1].DATA = MCP_TXB1DATA;
	MCP2515_Object->TXB_ptr[1].SIDH = MCP_TXB1SIDH;

	MCP2515_Object->TXB_ptr[2].CTRL = MCP_TXB2CTRL;
	MCP2515_Object->TXB_ptr[2].DATA = MCP_TXB2DATA;
	MCP2515_Object->TXB_ptr[2].SIDH = MCP_TXB2SIDH;

	MCP2515_Object->RXB_ptr[0].CTRL = MCP_RXB0CTRL;
	MCP2515_Object->RXB_ptr[0].DATA = MCP_RXB0DATA;
	MCP2515_Object->RXB_ptr[0].SIDH = MCP_RXB0SIDH;
	MCP2515_Object->RXB_ptr[0].CANINTF_RXnIF = CANINTF_RX0IF;

	MCP2515_Object->RXB_ptr[1].CTRL = MCP_RXB1CTRL;
	MCP2515_Object->RXB_ptr[1].DATA = MCP_RXB1DATA;
	MCP2515_Object->RXB_ptr[1].SIDH = MCP_RXB1SIDH;
	MCP2515_Object->RXB_ptr[1].CANINTF_RXnIF = CANINTF_RX1IF;

    // Define MCP2515 SPI device configuration
	spi_device_interface_config_t dev_cfg = {
		.mode = 0, // (0,0)
		.clock_speed_hz = 10000000, 
		.spics_io_num = MCP_SPI_PIN_CS,
		.queue_size = 1024
	};

    // Add MCP2515 SPI device to the bus
    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &MCP2515_Object->spi);
    if (ESP_OK != ret)
    {
        ESP_LOGE(TAG, "Could not add device to SPI bus. %d %s",
                ret, esp_err_to_name(ret));
    }

	return ERROR_OK;
}

MCP_ERROR_t MCP2515_reset(void)
{
    spi_transaction_t trans = {};

    trans.length = 8;
    trans.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    trans.tx_data[0] = INSTRUCTION_RESET;

    esp_err_t ret = spi_device_transmit(MCP2515_Object->spi, &trans);
    if (ret != ESP_OK) {
        printf("spi_device_transmit failed\n");
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t zeros[14];
    memset(zeros, 0, sizeof(zeros));
    MCP2515_setRegisters(MCP_TXB0CTRL, zeros, 14);
    MCP2515_setRegisters(MCP_TXB1CTRL, zeros, 14);
    MCP2515_setRegisters(MCP_TXB2CTRL, zeros, 14);

    MCP2515_setRegister(MCP_RXB0CTRL, 0);
    MCP2515_setRegister(MCP_RXB1CTRL, 0);

    MCP2515_setRegister(
        MCP_CANINTE, 
        CANINTF_RX0IF | CANINTF_RX1IF | CANINTF_ERRIF | CANINTF_MERRF);

    // receives all valid messages using either Standard or Extended Identifiers that
    // meet filter criteria. RXF0 is applied for RXB0, RXF1 is applied for RXB1
    MCP2515_modifyRegister(MCP_RXB0CTRL,
                   RXBnCTRL_RXM_MASK | RXB0CTRL_BUKT | RXB0CTRL_FILHIT_MASK,
                   RXBnCTRL_RXM_STDEXT | RXB0CTRL_BUKT | RXB0CTRL_FILHIT);
    MCP2515_modifyRegister(MCP_RXB1CTRL,
                   RXBnCTRL_RXM_MASK | RXB1CTRL_FILHIT_MASK,
                   RXBnCTRL_RXM_STDEXT | RXB1CTRL_FILHIT);

    // clear filters and masks
    // do not filter any standard frames for RXF0 used by RXB0
    // do not filter any extended frames for RXF1 used by RXB1
    const RXF_t filters[] = {RXF0, RXF1, RXF2, RXF3, RXF4, RXF5};
    for (int i=0; i<6; i++) {
        const bool ext = (i == 1);
        MCP_ERROR_t result = MCP2515_setFilter(filters[i], ext, 0);
        if (result != ERROR_OK) {
            return result;
        }
    }

    MASK_t masks[] = {MASK0, MASK1};
    for (int i=0; i<2; i++) {
        MCP_ERROR_t result = MCP2515_setFilterMask(masks[i], true, 0);
        if (result != ERROR_OK) {
            return result;
        }
    }

    return ERROR_OK;
}

uint8_t MCP2515_getStatus(void)
{
    spi_transaction_t trans = {};

    trans.length = 16;
    trans.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    trans.tx_data[0] = INSTRUCTION_READ_STATUS;
    trans.tx_data[1] = 0x00;

    esp_err_t ret = spi_device_transmit(MCP2515_Object->spi, &trans);
    if (ret != ESP_OK) {
        printf("spi_device_transmit failed\n");
    }

    return trans.rx_data[1];
}

MCP_ERROR_t MCP2515_setConfigMode()
{
    return MCP2515_setMode(CANCTRL_REQOP_CONFIG);
}

MCP_ERROR_t MCP2515_setListenOnlyMode()
{
    return MCP2515_setMode(CANCTRL_REQOP_LISTENONLY);
}

MCP_ERROR_t MCP2515_setSleepMode()
{
    return MCP2515_setMode(CANCTRL_REQOP_SLEEP);
}

MCP_ERROR_t MCP2515_setLoopbackMode()
{
    return MCP2515_setMode(CANCTRL_REQOP_LOOPBACK);
}

MCP_ERROR_t MCP2515_setOneShotMode(bool set)
{
    uint8_t data = 0;
    if (set) {
        data = 1U << 3;
    }
        
    MCP2515_modifyRegister(MCP_CANCTRL, 1U << 3, data);
    vTaskDelay(pdMS_TO_TICKS(10));

    bool modeMatch = false;
    for (int i = 0; i < 10; i++) {
        uint8_t ctrlR = MCP2515_readRegister(MCP_CANCTRL);
        modeMatch = (ctrlR & (1U << 3)) == data;
        if (modeMatch)
            break;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    return modeMatch? ERROR_OK : ERROR_FAIL;
}

MCP_ERROR_t MCP2515_setNormalMode()
{
    return MCP2515_setMode(CANCTRL_REQOP_NORMAL);
}



MCP_ERROR_t MCP2515_setBitrate(const CAN_SPEED_t canSpeed, CAN_CLOCK_t canClock)
{
    MCP_ERROR_t err = MCP2515_setConfigMode();
    if (err != ERROR_OK) {
        return ERROR_FAIL;
    }

    uint8_t set, cfg1, cfg2, cfg3;
    set = 1;
    switch (canClock)
    {
        case (MCP_8MHZ):
        switch (canSpeed)
        {
            case (CAN_5KBPS):                                               //   5KBPS
            cfg1 = MCP_8MHz_5kBPS_CFG1;
            cfg2 = MCP_8MHz_5kBPS_CFG2;
            cfg3 = MCP_8MHz_5kBPS_CFG3;
            break;

            case (CAN_10KBPS):                                              //  10KBPS
            cfg1 = MCP_8MHz_10kBPS_CFG1;
            cfg2 = MCP_8MHz_10kBPS_CFG2;
            cfg3 = MCP_8MHz_10kBPS_CFG3;
            break;

            case (CAN_20KBPS):                                              //  20KBPS
            cfg1 = MCP_8MHz_20kBPS_CFG1;
            cfg2 = MCP_8MHz_20kBPS_CFG2;
            cfg3 = MCP_8MHz_20kBPS_CFG3;
            break;

            case (CAN_31K25BPS):                                            //  31.25KBPS
            cfg1 = MCP_8MHz_31k25BPS_CFG1;
            cfg2 = MCP_8MHz_31k25BPS_CFG2;
            cfg3 = MCP_8MHz_31k25BPS_CFG3;
            break;

            case (CAN_33KBPS):                                              //  33.333KBPS
            cfg1 = MCP_8MHz_33k3BPS_CFG1;
            cfg2 = MCP_8MHz_33k3BPS_CFG2;
            cfg3 = MCP_8MHz_33k3BPS_CFG3;
            break;

            case (CAN_40KBPS):                                              //  40Kbps
            cfg1 = MCP_8MHz_40kBPS_CFG1;
            cfg2 = MCP_8MHz_40kBPS_CFG2;
            cfg3 = MCP_8MHz_40kBPS_CFG3;
            break;

            case (CAN_50KBPS):                                              //  50Kbps
            cfg1 = MCP_8MHz_50kBPS_CFG1;
            cfg2 = MCP_8MHz_50kBPS_CFG2;
            cfg3 = MCP_8MHz_50kBPS_CFG3;
            break;

            case (CAN_80KBPS):                                              //  80Kbps
            cfg1 = MCP_8MHz_80kBPS_CFG1;
            cfg2 = MCP_8MHz_80kBPS_CFG2;
            cfg3 = MCP_8MHz_80kBPS_CFG3;
            break;

            case (CAN_100KBPS):                                             // 100Kbps
            cfg1 = MCP_8MHz_100kBPS_CFG1;
            cfg2 = MCP_8MHz_100kBPS_CFG2;
            cfg3 = MCP_8MHz_100kBPS_CFG3;
            break;

            case (CAN_125KBPS):                                             // 125Kbps
            cfg1 = MCP_8MHz_125kBPS_CFG1;
            cfg2 = MCP_8MHz_125kBPS_CFG2;
            cfg3 = MCP_8MHz_125kBPS_CFG3;
            break;

            case (CAN_200KBPS):                                             // 200Kbps
            cfg1 = MCP_8MHz_200kBPS_CFG1;
            cfg2 = MCP_8MHz_200kBPS_CFG2;
            cfg3 = MCP_8MHz_200kBPS_CFG3;
            break;

            case (CAN_250KBPS):                                             // 250Kbps
            cfg1 = MCP_8MHz_250kBPS_CFG1;
            cfg2 = MCP_8MHz_250kBPS_CFG2;
            cfg3 = MCP_8MHz_250kBPS_CFG3;
            break;

            case (CAN_500KBPS):                                             // 500Kbps
            cfg1 = MCP_8MHz_500kBPS_CFG1;
            cfg2 = MCP_8MHz_500kBPS_CFG2;
            cfg3 = MCP_8MHz_500kBPS_CFG3;
            break;

            case (CAN_1000KBPS):                                            //   1Mbps
            cfg1 = MCP_8MHz_1000kBPS_CFG1;
            cfg2 = MCP_8MHz_1000kBPS_CFG2;
            cfg3 = MCP_8MHz_1000kBPS_CFG3;
            break;

            default:
            set = 0;
            break;
        }
        break;

        case (MCP_16MHZ):
        switch (canSpeed)
        {
            case (CAN_5KBPS):                                               //   5Kbps
            cfg1 = MCP_16MHz_5kBPS_CFG1;
            cfg2 = MCP_16MHz_5kBPS_CFG2;
            cfg3 = MCP_16MHz_5kBPS_CFG3;
            break;

            case (CAN_10KBPS):                                              //  10Kbps
            cfg1 = MCP_16MHz_10kBPS_CFG1;
            cfg2 = MCP_16MHz_10kBPS_CFG2;
            cfg3 = MCP_16MHz_10kBPS_CFG3;
            break;

            case (CAN_20KBPS):                                              //  20Kbps
            cfg1 = MCP_16MHz_20kBPS_CFG1;
            cfg2 = MCP_16MHz_20kBPS_CFG2;
            cfg3 = MCP_16MHz_20kBPS_CFG3;
            break;

            case (CAN_33KBPS):                                              //  33.333Kbps
            cfg1 = MCP_16MHz_33k3BPS_CFG1;
            cfg2 = MCP_16MHz_33k3BPS_CFG2;
            cfg3 = MCP_16MHz_33k3BPS_CFG3;
            break;

            case (CAN_40KBPS):                                              //  40Kbps
            cfg1 = MCP_16MHz_40kBPS_CFG1;
            cfg2 = MCP_16MHz_40kBPS_CFG2;
            cfg3 = MCP_16MHz_40kBPS_CFG3;
            break;

            case (CAN_50KBPS):                                              //  50Kbps
            cfg1 = MCP_16MHz_50kBPS_CFG1;
            cfg2 = MCP_16MHz_50kBPS_CFG2;
            cfg3 = MCP_16MHz_50kBPS_CFG3;
            break;

            case (CAN_80KBPS):                                              //  80Kbps
            cfg1 = MCP_16MHz_80kBPS_CFG1;
            cfg2 = MCP_16MHz_80kBPS_CFG2;
            cfg3 = MCP_16MHz_80kBPS_CFG3;
            break;

            case (CAN_83K3BPS):                                             //  83.333Kbps
            cfg1 = MCP_16MHz_83k3BPS_CFG1;
            cfg2 = MCP_16MHz_83k3BPS_CFG2;
            cfg3 = MCP_16MHz_83k3BPS_CFG3;
            break;

            case (CAN_100KBPS):                                             // 100Kbps
            cfg1 = MCP_16MHz_100kBPS_CFG1;
            cfg2 = MCP_16MHz_100kBPS_CFG2;
            cfg3 = MCP_16MHz_100kBPS_CFG3;
            break;

            case (CAN_125KBPS):                                             // 125Kbps
            cfg1 = MCP_16MHz_125kBPS_CFG1;
            cfg2 = MCP_16MHz_125kBPS_CFG2;
            cfg3 = MCP_16MHz_125kBPS_CFG3;
            break;

            case (CAN_200KBPS):                                             // 200Kbps
            cfg1 = MCP_16MHz_200kBPS_CFG1;
            cfg2 = MCP_16MHz_200kBPS_CFG2;
            cfg3 = MCP_16MHz_200kBPS_CFG3;
            break;

            case (CAN_250KBPS):                                             // 250Kbps
            cfg1 = MCP_16MHz_250kBPS_CFG1;
            cfg2 = MCP_16MHz_250kBPS_CFG2;
            cfg3 = MCP_16MHz_250kBPS_CFG3;
            break;

            case (CAN_500KBPS):                                             // 500Kbps
            cfg1 = MCP_16MHz_500kBPS_CFG1;
            cfg2 = MCP_16MHz_500kBPS_CFG2;
            cfg3 = MCP_16MHz_500kBPS_CFG3;
            break;

            case (CAN_1000KBPS):                                            //   1Mbps
            cfg1 = MCP_16MHz_1000kBPS_CFG1;
            cfg2 = MCP_16MHz_1000kBPS_CFG2;
            cfg3 = MCP_16MHz_1000kBPS_CFG3;
            break;

            default:
            set = 0;
            break;
        }
        break;

        case (MCP_20MHZ):
        switch (canSpeed)
        {
            case (CAN_33KBPS):                                              //  33.333Kbps
            cfg1 = MCP_20MHz_33k3BPS_CFG1;
            cfg2 = MCP_20MHz_33k3BPS_CFG2;
            cfg3 = MCP_20MHz_33k3BPS_CFG3;
	    break;

            case (CAN_40KBPS):                                              //  40Kbps
            cfg1 = MCP_20MHz_40kBPS_CFG1;
            cfg2 = MCP_20MHz_40kBPS_CFG2;
            cfg3 = MCP_20MHz_40kBPS_CFG3;
            break;

            case (CAN_50KBPS):                                              //  50Kbps
            cfg1 = MCP_20MHz_50kBPS_CFG1;
            cfg2 = MCP_20MHz_50kBPS_CFG2;
            cfg3 = MCP_20MHz_50kBPS_CFG3;
            break;

            case (CAN_80KBPS):                                              //  80Kbps
            cfg1 = MCP_20MHz_80kBPS_CFG1;
            cfg2 = MCP_20MHz_80kBPS_CFG2;
            cfg3 = MCP_20MHz_80kBPS_CFG3;
            break;

            case (CAN_83K3BPS):                                             //  83.333Kbps
            cfg1 = MCP_20MHz_83k3BPS_CFG1;
            cfg2 = MCP_20MHz_83k3BPS_CFG2;
            cfg3 = MCP_20MHz_83k3BPS_CFG3;
	    break;

            case (CAN_100KBPS):                                             // 100Kbps
            cfg1 = MCP_20MHz_100kBPS_CFG1;
            cfg2 = MCP_20MHz_100kBPS_CFG2;
            cfg3 = MCP_20MHz_100kBPS_CFG3;
            break;

            case (CAN_125KBPS):                                             // 125Kbps
            cfg1 = MCP_20MHz_125kBPS_CFG1;
            cfg2 = MCP_20MHz_125kBPS_CFG2;
            cfg3 = MCP_20MHz_125kBPS_CFG3;
            break;

            case (CAN_200KBPS):                                             // 200Kbps
            cfg1 = MCP_20MHz_200kBPS_CFG1;
            cfg2 = MCP_20MHz_200kBPS_CFG2;
            cfg3 = MCP_20MHz_200kBPS_CFG3;
            break;

            case (CAN_250KBPS):                                             // 250Kbps
            cfg1 = MCP_20MHz_250kBPS_CFG1;
            cfg2 = MCP_20MHz_250kBPS_CFG2;
            cfg3 = MCP_20MHz_250kBPS_CFG3;
            break;

            case (CAN_500KBPS):                                             // 500Kbps
            cfg1 = MCP_20MHz_500kBPS_CFG1;
            cfg2 = MCP_20MHz_500kBPS_CFG2;
            cfg3 = MCP_20MHz_500kBPS_CFG3;
            break;

            case (CAN_1000KBPS):                                            //   1Mbps
            cfg1 = MCP_20MHz_1000kBPS_CFG1;
            cfg2 = MCP_20MHz_1000kBPS_CFG2;
            cfg3 = MCP_20MHz_1000kBPS_CFG3;
            break;

            default:
            set = 0;
            break;
        }
        break;

        default:
            set = 0;
        break;
    }

    if (set) {
    	MCP2515_setRegister(MCP_CNF1, cfg1);
    	MCP2515_setRegister(MCP_CNF2, cfg2);
    	MCP2515_setRegister(MCP_CNF3, cfg3);
        return ERROR_OK;
    }
    else {
        return ERROR_FAIL;
    }
}

MCP_ERROR_t MCP2515_setClkOut(const CAN_CLKOUT_t divisor)
{
    if (divisor == CLKOUT_DISABLE) {
	    // Turn off CLKEN 
    	MCP2515_modifyRegister(MCP_CANCTRL, CANCTRL_CLKEN, 0x00);

	    // Turn on CLKOUT for SOF 
        MCP2515_modifyRegister(MCP_CNF3, CNF3_SOF, CNF3_SOF);
        return ERROR_OK;
    }

    // Set the prescaler (CLKPRE) 
    MCP2515_modifyRegister(MCP_CANCTRL, CANCTRL_CLKPRE, divisor);

    // Turn on CLKEN 
    MCP2515_modifyRegister(MCP_CANCTRL, CANCTRL_CLKEN, CANCTRL_CLKEN);

    // Turn off CLKOUT for SOF 
    MCP2515_modifyRegister(MCP_CNF3, CNF3_SOF, 0x00);
    return ERROR_OK;
}

MCP_ERROR_t MCP2515_setFilterMask(const MASK_t mask, const bool ext, const uint32_t ulData)
{
    MCP_ERROR_t res = MCP2515_setConfigMode();
    if (res != ERROR_OK) {
        return res;
    }

    uint8_t tbufdata[4];
    MCP2515_prepareId(tbufdata, ext, ulData);

    REGISTER_t reg;
    switch (mask) {
        case MASK0: 
            reg = MCP_RXM0SIDH; 
            break;
        case MASK1: 
            reg = MCP_RXM1SIDH; 
            break;
        default:
            return ERROR_FAIL;
    }

    MCP2515_setRegisters(reg, tbufdata, 4);

    return ERROR_OK;
}

MCP_ERROR_t MCP2515_setFilter(const RXF_t num, const bool ext, const uint32_t ulData)
{
    MCP_ERROR_t res = MCP2515_setConfigMode();
    if (res != ERROR_OK) {
        return res;
    }

    REGISTER_t reg;

    switch (num) {
        case RXF0: 
            reg = MCP_RXF0SIDH; 
            break;
        case RXF1: 
            reg = MCP_RXF1SIDH; 
            break;
        case RXF2: 
            reg = MCP_RXF2SIDH; 
            break;
        case RXF3: 
            reg = MCP_RXF3SIDH; 
            break;
        case RXF4: 
            reg = MCP_RXF4SIDH; 
            break;
        case RXF5: 
            reg = MCP_RXF5SIDH; 
            break;
        default:
            return ERROR_FAIL;
    }

    uint8_t tbufdata[4];
    MCP2515_prepareId(tbufdata, ext, ulData);
    MCP2515_setRegisters(reg, tbufdata, 4);

    return ERROR_OK;
}

MCP_ERROR_t MCP2515_sendMessage(const TXBn_t txbn, const MCP_CAN_frame* frame)
{
    if (frame->can_dlc > CAN_MAX_DLEN) {
        return ERROR_FAILTX;
    }

    const TXBn_REGS txbuf = &MCP2515_Object->TXB_ptr[txbn];


    uint8_t data[13];

    bool ext = (frame->can_id & CAN_EFF_FLAG);
    bool rtr = (frame->can_id & CAN_RTR_FLAG);
    uint32_t id = (frame->can_id & (ext ? CAN_EFF_MASK : CAN_SFF_MASK));

    MCP2515_prepareId(data, ext, id);

    data[MCP_DLC] = rtr ? (frame->can_dlc | RTR_MASK) : frame->can_dlc;

    memcpy(&data[MCP_DATA], frame->data, frame->can_dlc);

    MCP2515_setRegisters(txbuf->SIDH, data, 5 + frame->can_dlc);

    MCP2515_modifyRegister(txbuf->CTRL, TXB_TXREQ, TXB_TXREQ);

    uint8_t ctrl = MCP2515_readRegister(txbuf->CTRL);
    if ((ctrl & (TXB_ABTF | TXB_MLOA | TXB_TXERR)) != 0) {
        return ERROR_FAILTX;
    }
    return ERROR_OK;
}

MCP_ERROR_t MCP2515_sendMessageAfterCtrlCheck(const MCP_CAN_frame* frame)
{
    if (frame->can_dlc > CAN_MAX_DLEN) {
        return ERROR_FAILTX;
    }

    TXBn_t txBuffers[N_TXBUFFERS] = {TXB0, TXB1, TXB2};

    for (int i=0; i<N_TXBUFFERS; i++) 
    {
        const TXBn_REGS txbuf = &MCP2515_Object->TXB_ptr[txBuffers[i]];

        uint8_t ctrlval = MCP2515_readRegister(txbuf->CTRL);
        if ( (ctrlval & TXB_TXREQ) == 0 ) 
        {
            return MCP2515_sendMessage(txBuffers[i], frame);
        }
    }

    return ERROR_ALLTXBUSY;
}

MCP_ERROR_t MCP2515_readMessage(const RXBn_t rxbn, MCP_CAN_frame* frame)
{
    const RXBn_REGS rxb = &MCP2515_Object->RXB_ptr[rxbn];

    uint8_t tbufdata[5];

    MCP2515_readRegisters(rxb->SIDH, tbufdata, 5);

    uint32_t id = (tbufdata[MCP_SIDH]<<3) + (tbufdata[MCP_SIDL]>>5);

    if ( (tbufdata[MCP_SIDL] & TXB_EXIDE_MASK) ==  TXB_EXIDE_MASK ) {
        id = (id<<2) + (tbufdata[MCP_SIDL] & 0x03);
        id = (id<<8) + tbufdata[MCP_EID8];
        id = (id<<8) + tbufdata[MCP_EID0];
        id |= CAN_EFF_FLAG;
    }

    uint8_t dlc = (tbufdata[MCP_DLC] & DLC_MASK);
    if (dlc > CAN_MAX_DLEN) {
        return ERROR_FAIL;
    }

    uint8_t ctrl = MCP2515_readRegister(rxb->CTRL);
    if (ctrl & RXBnCTRL_RTR) {
        id |= CAN_RTR_FLAG;
    }

    frame->can_id = id;
    frame->can_dlc = dlc;

    MCP2515_readRegisters(rxb->DATA, frame->data, dlc);

    MCP2515_modifyRegister(MCP_CANINTF, rxb->CANINTF_RXnIF, 0);

    return ERROR_OK;
}

MCP_ERROR_t MCP2515_readMessageAfterStatCheck(MCP_CAN_frame* frame)
{
    MCP_ERROR_t rc;
    uint8_t stat = MCP2515_getStatus();

    if ( stat & STAT_RX0IF ) {
        rc = MCP2515_readMessage(RXB0, frame);
    } 
    else if ( stat & STAT_RX1IF ) {
        rc = MCP2515_readMessage(RXB1, frame);
    } 
    else {
        rc = ERROR_NOMSG;
    }

    return rc;
}

bool MCP2515_checkReceive(void)
{
    uint8_t res = MCP2515_getStatus();
    if ( res & STAT_RXIF_MASK ) {
        return true;
    } else {
        return false;
    }
}

bool MCP2515_checkError(void)
{
    uint8_t eflg = MCP2515_getErrorFlags();

    if ( eflg & EFLG_ERRORMASK ) {
        return true;
    } else {
        return false;
    }
}

uint8_t MCP2515_getErrorFlags(void)
{
    return MCP2515_readRegister(MCP_EFLG);
}

void MCP2515_clearRXnOVRFlags(void)
{
	MCP2515_modifyRegister(MCP_EFLG, EFLG_RX0OVR | EFLG_RX1OVR, 0);
}

uint8_t MCP2515_getInterrupts(void)
{
    return MCP2515_readRegister(MCP_CANINTF);
}

void MCP2515_clearInterrupts(void)
{
	MCP2515_setRegister(MCP_CANINTF, 0);
}

uint8_t MCP2515_getInterruptMask(void)
{
    return MCP2515_readRegister(MCP_CANINTE);
}

void MCP2515_clearTXInterrupts(void)
{
	MCP2515_modifyRegister(
        MCP_CANINTF, 
        (CANINTF_TX0IF | CANINTF_TX1IF | CANINTF_TX2IF), 
        0);
}

void MCP2515_clearRXnOVR(void)
{
	uint8_t eflg = MCP2515_getErrorFlags();
	if (eflg != 0) {
		MCP2515_clearRXnOVRFlags();
		MCP2515_clearInterrupts();
	}
}

void MCP2515_clearMERR()
{
	MCP2515_modifyRegister(MCP_CANINTF, CANINTF_MERRF, 0);
}

void MCP2515_clearERRIF()
{
	MCP2515_modifyRegister(MCP_CANINTF, CANINTF_ERRIF, 0);
}
