/**
 * @file    interface.c
 * @brief   ST7735 bus primitives for the magsensor-2go (XMC1100).
 *
 * Two busses, switchable at runtime with lcd_bus_select():
 *   - "soft": bit-banged GPIO, SPI mode 3 (SCL idles high, SDA changes while
 *     SCL is low, panel samples on the rising edge) — the proven path.
 *   - "hw": USIC0_CH1 SPI master, mode 3, 8 MHz.
 * SCL/SDA/DC/RES/CS are on PORT0 (no PDISC required), BL on PORT2.0.
 */

#include "interface.h"

#include "lcd.h"

#include <stdio.h>

#include "system_xmc1100.h"

#include "XMC1100.h"

/* XMC1 port register offsets (XMC1100.h PORT0_Type). */
#define PORT_OUT_OFFSET (0x00U)
#define PORT_IOCR0_OFFSET (0x10U)
#define PORT_PDISC_OFFSET (0x60U)

#define PORT_IOCR_PC_Msk (0xF8UL)

/* Pin-control lane encoding (bits [7:3] of each 8-bit IOCR lane). */
#define GPIO_MODE_OUTPUT_PUSH_PULL (0x80UL)
#define GPIO_MODE_AF7              (0xB8UL)  /* P0.7 DOUT0 / P0.8 SCLKOUT */

/* USIC0_CH1 SPI master, mode 3. Two details matter (verified on hardware and
 * against the XMCLib SPI example):
 *  - the FDR must be in FRACTIONAL divider mode (DM=10); the normal divider
 *    mode does not clock the protocol pre-processor on this part,
 *  - only DX0 (data) may use INSW=1; DX1 (SCLK) and DX2 (MSLS) must take the
 *    protocol pre-processor outputs, i.e. INSW=0.
 * fSCLK = fPCLK * (STEP+1) / (1024 * (DCTQ+1) * (PDIV+1)). STEP=1023 (max)
 * with DCTQ=1, PDIV=0 gives the fastest SPI clock (~16 MHz at fPCLK=32 MHz).
 * SCLKCFG=0b11 = passive level 1 + half-period delay (SPI mode 3). */
#define HW_FDR_STEP  (1023U)
#define HW_DCTQ      (1U)                       /* oversampling - 1 */
#define HW_PDIV      (0U)
#define HW_FDR     (0x8000UL | HW_FDR_STEP)
#define HW_BRG     ((HW_DCTQ << USIC_CH_BRG_DCTQ_Pos) | \
                    (3UL << USIC_CH_BRG_SCLKCFG_Pos))
#define HW_SCTR    (USIC_CH_SCTR_PDL_Msk | (1UL << USIC_CH_SCTR_TRM_Pos) | \
                    (0x3FUL << USIC_CH_SCTR_FLE_Pos) | \
                    (0x07UL << USIC_CH_SCTR_WLE_Pos) | \
                    USIC_CH_SCTR_SDIR_Msk)
#define HW_TCSR    (USIC_CH_TCSR_HPCMD_Msk | \
                    (1UL << USIC_CH_TCSR_TDEN_Pos) | \
                    USIC_CH_TCSR_TDSSM_Msk)
#define HW_PCR     (USIC_CH_PCR_SSCMode_MSLSEN_Msk | \
                    USIC_CH_PCR_SSCMode_SELCTR_Msk | \
                    USIC_CH_PCR_SSCMode_SELINV_Msk | \
                    USIC_CH_PCR_SSCMode_FEM_Msk | \
                    (1UL << USIC_CH_PCR_SSCMode_SELO_Pos))

/* Configured HW-SPI shift clock, in Hz (from the FDR/BRG settings above). */
uint32_t lcd_hw_spi_hz(void)
{
    return (uint32_t)(((uint64_t)SystemCoreClock * (HW_FDR_STEP + 1U)) /
                      (1024ULL * (HW_DCTQ + 1U) * (HW_PDIV + 1U)));
}

static volatile uint32_t *port_ptr(uint8_t port)
{
    static volatile uint32_t *const ports[3] = {
        (volatile uint32_t *)PORT0,
        (volatile uint32_t *)PORT1,
        (volatile uint32_t *)PORT2,
    };

    return ports[port];
}

static void pin_set_mode(uint8_t port, uint8_t pin, uint32_t mode)
{
    volatile uint32_t *iocr;
    uint32_t shift = 8UL * (pin & 3UL);

    iocr = (volatile uint32_t *)((uintptr_t)port_ptr(port) +
                                 PORT_IOCR0_OFFSET + ((pin >> 2U) * 4UL));
    *iocr = (*iocr & ~(PORT_IOCR_PC_Msk << shift)) | (mode << shift);
}

/* Only PORT2 pads are analog-capable, so only PORT2 has a writable PDISC
 * (resets to 0xFFFF, i.e. pads disabled). PORT0/PORT1 PDISC is read-only. */
static void pad_enable(uint8_t port, uint8_t pin)
{
    volatile uint32_t *pdisc;

    if (port != 2U)
    {
        return;
    }

    pdisc = (volatile uint32_t *)((uintptr_t)port_ptr(port) + PORT_PDISC_OFFSET);
    *pdisc &= ~(1UL << pin);
}

void lcd_io_init_out(uint8_t port, uint8_t pin)
{
    pad_enable(port, pin);
    pin_set_mode(port, pin, GPIO_MODE_OUTPUT_PUSH_PULL);
    lcd_io_clr(port, pin);
}

void lcd_io_bind_af(uint8_t port, uint8_t pin, uint32_t af_lane)
{
    pad_enable(port, pin);
    pin_set_mode(port, pin, af_lane);
}

void lcd_io_set(uint8_t port, uint8_t pin)
{
    volatile uint32_t *out =
        (volatile uint32_t *)((uintptr_t)port_ptr(port) + PORT_OUT_OFFSET);

    *out |= (1UL << pin);
}

void lcd_io_clr(uint8_t port, uint8_t pin)
{
    volatile uint32_t *out =
        (volatile uint32_t *)((uintptr_t)port_ptr(port) + PORT_OUT_OFFSET);

    *out &= ~(1UL << pin);
}

/* ---- soft SPI: mode 3, MSB first, idle high ---- */
static void soft_send_byte(uint8_t dat)
{
    uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
        if ((dat & 0x80U) != 0U)
        {
            LCD_SPI_SDA_SET;
        }
        else
        {
            LCD_SPI_SDA_CLR;
        }
        dat <<= 1;
        LCD_SPI_SCL_CLR;   /* SDA settles while SCL is low */
        LCD_SPI_SCL_SET;   /* panel samples on the rising edge */
    }
}

/* ---- hardware SPI: USIC0_CH1, master, mode 3 ---- */
static void hw_spi_apply(uint32_t fdr)
{
    USIC_CH_TypeDef *ch = USIC0_CH1;
    uint32_t guard;

    /* channel enabled + forced to IDLE before reconfiguring */
    ch->KSCFG = USIC_CH_KSCFG_MODEN_Msk | USIC_CH_KSCFG_BPMODEN_Msk;
    guard = 100000UL;
    while ((ch->KSCFG & USIC_CH_KSCFG_MODEN_Msk) == 0U)
    {
        if (--guard == 0U)
        {
            puts("[LCD] hw: MODEN stuck");
            break;
        }
    }
    ch->CCR &= ~USIC_CH_CCR_MODE_Msk;

    ch->FDR = fdr;
    ch->BRG = HW_BRG;
    ch->SCTR = HW_SCTR;
    ch->TCSR = HW_TCSR;
    ch->PCR_SSCMode = HW_PCR;
    ch->PSCR = 0xFFFFFFFFUL;
    ch->CCR = 0UL;   /* parity disabled */

    /* SSC master input stages: only DIN0 (data, P2.6) comes from a pin
     * (INSW=1); DX1 (SCLK) and DX2 (MSLS) must be fed from the protocol
     * pre-processor (INSW=0). Leaving INSW=1 here stalls the shift unit. */
    ch->DX0CR = USIC_CH_DX0CR_INSW_Msk | (6UL << USIC_CH_DX0CR_DSEL_Pos);
    ch->DX1CR = 0UL;
    ch->DX2CR = 0UL;
    ch->DX3CR = 0UL;

    /* SCLK/SDA pins onto USIC0_CH1 (ALT7) */
    lcd_io_bind_af(LCD_SDA_PORT, LCD_SDA_PIN, GPIO_MODE_AF7);
    lcd_io_bind_af(LCD_SCL_PORT, LCD_SCL_PIN, GPIO_MODE_AF7);

    /* operating mode: SPI. CCR.MODE = 1 on XMC1 (2 would be UART). */
    ch->CCR = (ch->CCR & ~USIC_CH_CCR_MODE_Msk) |
              (1UL << USIC_CH_CCR_MODE_Pos);
}

static void hw_spi_init(void)
{
    hw_spi_apply(HW_FDR);
}

/* Queue one byte once TBUF is free. The PSCR TBIF-clear before each write
 * mirrors the console UART (console.c) and XMC_SPI_CH_Transmit() FIFO-off
 * path. */
static void hw_send_byte(uint8_t dat)
{
    uint32_t guard = 400000UL;
    while ((USIC0_CH1->TCSR & USIC_CH_TCSR_TDV_Msk) != 0U)
    {
        if (--guard == 0U)
        {
            puts("[LCD] hw_send_byte: TDV stuck");
            break;
        }
    }
    USIC0_CH1->PSCR = USIC_CH_PSCR_CTBIF_Msk;
    USIC0_CH1->TBUF[0] = dat;
}

/* Wait until the queued byte has drained from the shift register. Waiting on
 * TSIF races at high SCLK (the word can complete before we clear the flag and
 * then the wait never ends), so use a short bounded spin: one word takes
 * 8/fSCLK (<1 us at 16 MHz). CS/DC must not change before that or the panel
 * latches a truncated byte. */
static void hw_wait_idle(void)
{
    uint32_t guard = 400000UL;
    while ((USIC0_CH1->TCSR & USIC_CH_TCSR_TDV_Msk) != 0U)
    {
        if (--guard == 0U)
        {
            puts("[LCD] hw_wait_idle: TDV stuck");
            break;
        }
    }
    for (volatile uint32_t d = 0U; d < 64U; d++)
    {
        /* drain the final word */
    }
    USIC0_CH1->PSCR = USIC_CH_PSR_SSCMode_TSIF_Msk;
}

/* ---- active bus + byte dispatch ---- */
static lcd_bus_t s_bus;

void lcd_bus_select(lcd_bus_t bus)
{
    if (bus == LCD_BUS_HW)
    {
        hw_spi_init();
    }
    else
    {
        lcd_io_init_out(LCD_SDA_PORT, LCD_SDA_PIN);
        lcd_io_init_out(LCD_SCL_PORT, LCD_SCL_PIN);
    }
    s_bus = bus;
}

static void bus_send_byte(uint8_t dat)
{
    if (s_bus == LCD_BUS_HW)
    {
        hw_send_byte(dat);
    }
    else
    {
        soft_send_byte(dat);
    }
}

/* ---- byte-level transfers ---- */

void WriteComm(uint16_t data)
{
    LCD_CS_CLR;
    LCD_RS_CLR;                 /* command */
    bus_send_byte((uint8_t)data);
    if (s_bus == LCD_BUS_HW)
    {
        hw_wait_idle();
    }
    LCD_CS_SET;
}

void WriteData(uint16_t data)
{
    LCD_CS_CLR;
    LCD_RS_SET;                 /* data */
    bus_send_byte((uint8_t)data);
    if (s_bus == LCD_BUS_HW)
    {
        hw_wait_idle();
    }
    LCD_CS_SET;
}

void SendData(uint32_t color)
{
    LCD_CS_CLR;
    LCD_RS_SET;
    bus_send_byte((uint8_t)(color >> 8));
    bus_send_byte((uint8_t)color);
    if (s_bus == LCD_BUS_HW)
    {
        hw_wait_idle();
    }
    LCD_CS_SET;
}

void LCD_WriteDataFast(uint8_t data)
{
    bus_send_byte(data);
}

void LCD_BeginData(void)
{
    LCD_CS_CLR;
    LCD_RS_SET;
}

void LCD_EndData(void)
{
    if (s_bus == LCD_BUS_HW)
    {
        hw_wait_idle();
    }
    LCD_CS_SET;
}