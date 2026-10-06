/**
 * @file    epdif.c
 * @brief   E-paper panel hardware interface for the xmc4500-relax-lite board.
 *
 * Hardware SPI on USIC2 channel 0, mode 0, 2 MHz. USIC2 is held in the SCU
 * peripheral-reset domain, so its reset must be released before the registers
 * take effect - the same trap as USIC1 for the console. P2.6 is an analog pad,
 * so its PDISC bit is cleared too. CS is held low continuously: toggling it in
 * software races the USIC shift register and drops bytes (confirmed on
 * hardware - a per-byte CS gave a sheared image).
 */

#include "epdif.h"

#include "board.h"

#include "XMC4500.h"
#include "xmc_gpio.h"
#include "xmc_spi.h"

#define EPD_CS_PIN   P2_6
#define EPD_DC_PIN   P5_7
#define EPD_RST_PIN  P3_3
#define EPD_BUSY_PIN P3_4
#define EPD_SCK_PIN  P5_2
#define EPD_MOSI_PIN P5_0

#define EPD_SPI_CH   XMC_SPI2_CH0
#define EPD_SPI_BAUD (2000000UL)

static const XMC_SPI_CH_CONFIG_t epd_spi_config =
{
    .baudrate       = EPD_SPI_BAUD,
    .bus_mode       = XMC_SPI_CH_BUS_MODE_MASTER,
    .selo_inversion = XMC_SPI_CH_SLAVE_SEL_INV_TO_MSLS,
    .parity_mode    = XMC_USIC_CH_PARITY_MODE_NONE,
};

void epdif_init(void)
{
    /* Release USIC2 from the SCU peripheral reset (bit 8). */
    SCU_RESET->PRCLR1 = SCU_RESET_PRSTAT1_USIC2RS_Msk;

    /* P2.6 (CS) is an analog-capable pad: enable its digital pad. */
    XMC_GPIO_PORT2->PDISC &= ~(1U << 6);

    XMC_GPIO_SetMode(EPD_CS_PIN, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
    XMC_GPIO_SetMode(EPD_DC_PIN, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
    XMC_GPIO_SetMode(EPD_RST_PIN, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
    XMC_GPIO_SetMode(EPD_BUSY_PIN, XMC_GPIO_MODE_INPUT_TRISTATE);
    XMC_GPIO_SetOutputHigh(EPD_RST_PIN);

    XMC_GPIO_SetMode(EPD_SCK_PIN, XMC_GPIO_MODE_OUTPUT_PUSH_PULL | P5_2_AF_U2C0_SCLKOUT);
    XMC_GPIO_SetMode(EPD_MOSI_PIN, XMC_GPIO_MODE_OUTPUT_PUSH_PULL | P5_0_AF_U2C0_DOUT0);

    XMC_SPI_CH_Init(EPD_SPI_CH, &epd_spi_config);
    XMC_SPI_CH_SetWordLength(EPD_SPI_CH, 8U);
    XMC_SPI_CH_SetFrameLength(EPD_SPI_CH, 8U);
    XMC_SPI_CH_SetBitOrderMsbFirst(EPD_SPI_CH);
    XMC_SPI_CH_ConfigureShiftClockOutput(EPD_SPI_CH,
                                         XMC_SPI_CPOL_0_CPHA_0,
                                         XMC_SPI_CH_BRG_SHIFT_CLOCK_OUTPUT_SCLK);
    XMC_SPI_CH_Start(EPD_SPI_CH);

    /* The panel is write-only and single-slave: hold CS low for the session. */
    XMC_GPIO_SetOutputLow(EPD_CS_PIN);
}

void epdif_write_pin(epdif_pin_t pin, int value)
{
    switch (pin)
    {
        case EPDIF_PIN_RST:
            if (value != 0) { XMC_GPIO_SetOutputHigh(EPD_RST_PIN); } else { XMC_GPIO_SetOutputLow(EPD_RST_PIN); }
            break;
        case EPDIF_PIN_DC:
            if (value != 0) { XMC_GPIO_SetOutputHigh(EPD_DC_PIN); } else { XMC_GPIO_SetOutputLow(EPD_DC_PIN); }
            break;
        case EPDIF_PIN_CS:
            if (value != 0) { XMC_GPIO_SetOutputHigh(EPD_CS_PIN); } else { XMC_GPIO_SetOutputLow(EPD_CS_PIN); }
            break;
        default:
            break;
    }
}

int epdif_read_pin(epdif_pin_t pin)
{
    if (pin == EPDIF_PIN_BUSY)
    {
        return (int)XMC_GPIO_GetInput(EPD_BUSY_PIN);
    }

    return 0;
}

int epdif_busy_pullup_read(void)
{
    int level;

    XMC_GPIO_SetMode(EPD_BUSY_PIN, XMC_GPIO_MODE_INPUT_PULL_UP);
    level = (int)XMC_GPIO_GetInput(EPD_BUSY_PIN);
    XMC_GPIO_SetMode(EPD_BUSY_PIN, XMC_GPIO_MODE_INPUT_TRISTATE);

    return level;
}

void epdif_delay_ms(uint32_t ms)
{
    board_delay_ms(ms);
}

void epdif_spi_transfer(uint8_t data)
{
    XMC_SPI_CH_ClearStatusFlag(EPD_SPI_CH,
                               XMC_SPI_CH_STATUS_FLAG_TRANSMIT_SHIFT_INDICATION);

    XMC_SPI_CH_Transmit(EPD_SPI_CH, (uint16_t)data, XMC_SPI_CH_MODE_STANDARD);

    while ((XMC_SPI_CH_GetStatusFlag(EPD_SPI_CH) &
            XMC_SPI_CH_STATUS_FLAG_TRANSMIT_SHIFT_INDICATION) == 0U)
    {
        /* wait for the shift register to drain before the next byte */
    }
    XMC_SPI_CH_ClearStatusFlag(EPD_SPI_CH,
                               XMC_SPI_CH_STATUS_FLAG_TRANSMIT_SHIFT_INDICATION);
}

uint32_t epdif_spi_hz(void)
{
    uint32_t fdr = EPD_SPI_CH->FDR;
    uint32_t brg = EPD_SPI_CH->BRG;
    uint32_t step = fdr & 0x3FFU;
    uint32_t dctq = (brg >> USIC_CH_BRG_DCTQ_Pos) & 0xFU;
    uint32_t pdiv = (brg >> USIC_CH_BRG_PDIV_Pos) & 0x1FU;
    uint32_t pclk = board_peripheral_clock_hz();

    /* Fractional divider: fSCLK = fPCLK * (STEP+1) / (1024 * (DCTQ+1) * (PDIV+1)). */
    return (uint32_t)(((uint64_t)pclk * (step + 1U)) /
                      (1024ULL * (dctq + 1U) * (pdiv + 1U)));
}
