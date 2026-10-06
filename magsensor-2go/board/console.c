/**
 * @file    console.c
 * @brief   Polled UART console on USIC0 channel 0 (TXD = P2.1, RXD = P2.2).
 *
 * The register sequence mirrors the Infineon XMClib UART init
 * (XMC_UART_CH_InitEx + XMC_UART_CH_SetInputSource + XMC_UART_CH_Start) so the
 * behaviour matches the vendor reference without pulling in the whole library.
 */

#include "console.h"

#include "board.h"
#include "system_xmc1100.h"

#include "XMC1100.h"

/* PORT2 layout: OUT at +0x00, IOCR0 at +0x10, one 8-bit pin lane per pin. */
#define PORT_OUT_OFFSET   (0x00U)
#define PORT_IOCR0_OFFSET (0x10U)
#define PORT_IOCR_PC_Msk  (0xF8UL)

#define GPIO_MODE_INPUT_TRISTATE        (0x00UL)
#define GPIO_MODE_OUTPUT_PUSH_PULL_ALT6 (0xB0UL)

#define UART_OVERSAMPLING (16UL)

/* USIC CH FDR.DM = 2 selects the fractional baudrate divider mode. */
#define USIC_FDR_DIVIDER_MODE_FRACTIONAL (0x8000UL)

/* CCR.MODE = 2 selects UART (ASC) mode. */
#define USIC_CCR_MODE_UART (0x2UL)

#define P2_1_PIN (1U)
#define P2_2_PIN (2U)

/*
 * USIC0_C0_DX3_P2_2 from xmc1_usic_map.h: P2.2 is the only USIC0 channel 0 input
 * reachable through the DX3 stage.
 */
#define USIC0_C0_DX3_P2_2 (0U)

/*
 * USIC0_C0_DX0_DX3INS from xmc1_usic_map.h: the RX input is taken from the
 * output of the DX3 stage rather than directly from an input pad. This matches
 * RTE_UART0_RX_INPUT for P2.2 in the XMC_2Go RTE_Device.h.
 */
#define USIC0_C0_DX0_DX3INS (6U)

#define USIC0_CH0_PTR (USIC0_CH0)

static void port2_pin_set_mode(uint32_t pin, uint32_t mode)
{
    volatile uint32_t *iocr =
        (volatile uint32_t *)((uintptr_t)PORT2 + PORT_IOCR0_OFFSET + ((pin >> 2U) * 4UL));
    uint32_t shift = 8UL * (pin & 3UL);

    *iocr = (*iocr & ~(PORT_IOCR_PC_Msk << shift)) | (mode << shift);
}

/** Ungate the USIC0 peripheral clock and switch the channel on. */
static void usic0_channel_enable(void)
{
    SCU_GENERAL->PASSWD = 0x000000C0UL;
    SCU_CLK->CGATCLR0 = SCU_CLK_CGATSTAT0_USIC0_Msk;
    SCU_GENERAL->PASSWD = 0x000000C3UL;

    while ((SCU_CLK->CGATSTAT0 & SCU_CLK_CGATSTAT0_USIC0_Msk) != 0UL)
    {
        /* wait until the USIC0 clock is running again */
    }

    USIC0_CH0_PTR->KSCFG = USIC_CH_KSCFG_MODEN_Msk | USIC_CH_KSCFG_BPMODEN_Msk;
    while ((USIC0_CH0_PTR->KSCFG & USIC_CH_KSCFG_MODEN_Msk) == 0UL)
    {
        /* wait until the channel leaves reset */
    }

    /* hold the channel in IDLE mode until the frame format is programmed */
    USIC0_CH0_PTR->CCR &= ~USIC_CH_CCR_MODE_Msk;
}

/**
 * @brief Program the baudrate generator for @p baud using the fractional divider.
 *
 * Same search as XMClib's XMC_USIC_CH_SetBaudrate(): find the prescaler value
 * whose fractional part of PDIV is smallest, then combine it with FDR.STEP.
 *
 * The clock fed to the search is SystemCoreClock, not PCLK. XMClib derives it
 * from XMC_SCU_CLOCK_GetPeripheralClockFrequency(), which returns SystemCoreClock
 * unchanged on XMC1; only "fast" peripherals (GTM, SC) run at 2 x MCLK. Using
 * PCLK here doubles every divider and halves the achieved baudrate.
 */
static void usic0_set_baudrate(uint32_t baud, uint32_t oversampling)
{
    uint32_t peripheral_clock_100 = SystemCoreClock / 100UL;
    uint32_t baud_100 = baud / 100UL;
    uint32_t best_prescaler = 1024UL;
    uint32_t best_pdiv_int = 1UL;
    uint32_t best_pdiv_frac = 0x3FFUL;
    uint32_t prescaler;

    for (prescaler = 1024UL; prescaler > 0UL; --prescaler)
    {
        uint32_t pdiv = (peripheral_clock_100 * prescaler) / (baud_100 * oversampling);
        uint32_t pdiv_int = pdiv >> 10U;
        uint32_t pdiv_frac = pdiv & 0x3FFUL;

        if ((pdiv_int <= 1024UL) && (pdiv_frac < best_pdiv_frac))
        {
            best_pdiv_frac = pdiv_frac;
            best_pdiv_int = pdiv_int;
            best_prescaler = prescaler;
        }
    }

    USIC0_CH0_PTR->FDR = USIC_FDR_DIVIDER_MODE_FRACTIONAL |
                        ((best_prescaler - 1UL) << USIC_CH_FDR_STEP_Pos);

    USIC0_CH0_PTR->BRG =
        (USIC0_CH0_PTR->BRG & ~(USIC_CH_BRG_DCTQ_Msk | USIC_CH_BRG_PDIV_Msk)) |
        ((oversampling - 1UL) << USIC_CH_BRG_DCTQ_Pos) |
        ((best_pdiv_int - 1UL) << USIC_CH_BRG_PDIV_Pos);
}

void console_init(uint32_t baud)
{
    usic0_channel_enable();
    usic0_set_baudrate(baud, UART_OVERSAMPLING);

    /*
     * PCR[ASC]: 1 stop bit, sampling point at half the oversampling period,
     * sample majority decision, transmitter and receiver status enabled.
     */
    USIC0_CH0_PTR->PCR_ASCMode =
        ((((UART_OVERSAMPLING >> 1UL) + 1UL) << USIC_CH_PCR_ASCMode_SP_Pos) |
         USIC_CH_PCR_ASCMode_SMD_Msk |
         USIC_CH_PCR_ASCMode_RSTEN_Msk |
         USIC_CH_PCR_ASCMode_TSTEN_Msk);

    /*
     * SCTR: passive data level high, transmission mode active high,
     * 8 data bits (WLE = FLE = 7).
     */
    USIC0_CH0_PTR->SCTR =
        ((7UL << USIC_CH_SCTR_WLE_Pos) |
         (7UL << USIC_CH_SCTR_FLE_Pos) |
         (1UL << USIC_CH_SCTR_TRM_Pos) |
         USIC_CH_SCTR_PDL_Msk);

    /* TDEN: enable the transmitter, TDSSM: single shift mode. */
    USIC0_CH0_PTR->TCSR = (1UL << USIC_CH_TCSR_TDEN_Pos) | USIC_CH_TCSR_TDSSM_Msk;

    /* CCR = 0 selects no parity generation or checking. */
    USIC0_CH0_PTR->CCR = 0UL;
    USIC0_CH0_PTR->PSCR = 0xFFFFFFFFUL;

    /* Route P2.2 into the DX3 input stage used as RXD. */
    USIC0_CH0_PTR->DX3CR &= ~USIC_CH_DX3CR_DSEL_Msk;
    USIC0_CH0_PTR->DX3CR |= (USIC0_C0_DX3_P2_2 << USIC_CH_DX3CR_DSEL_Pos);

    /* Select the DX3 stage output as the USIC receive input. */
    USIC0_CH0_PTR->DX0CR &= ~USIC_CH_DX0CR_DSEL_Msk;
    USIC0_CH0_PTR->DX0CR |= (USIC0_C0_DX0_DX3INS << USIC_CH_DX0CR_DSEL_Pos);

    USIC0_CH0_PTR->CCR =
        (USIC0_CH0_PTR->CCR & ~USIC_CH_CCR_MODE_Msk) | USIC_CCR_MODE_UART;

    /* P2.1 = USIC0_CH0.DOUT0 on ALT6, P2.2 = RXD input. */
    port2_pin_set_mode(P2_1_PIN, GPIO_MODE_OUTPUT_PUSH_PULL_ALT6);
    port2_pin_set_mode(P2_2_PIN, GPIO_MODE_INPUT_TRISTATE);

    /*
     * PORT2_PDISC keeps its reset value (every pin disabled) unless it is
     * cleared. XMC_GPIO_Init() in the vendor driver clears the bit for each pin
     * it configures; skipping it leaves the pads disconnected, so USIC0 keeps
     * shifting bits at the correct rate but nothing reaches the wire.
     */
    PORT2->PDISC &= ~((1UL << P2_1_PIN) | (1UL << P2_2_PIN));
}

void console_putc(char c)
{
    /*
     * Wait for the transmit buffer to drain. XMClib's XMC_UART_CH_Transmit()
     * polls TCSR.TDV (data-valid) for the same purpose; PSR.BUSY reflects the
     * shift-register state and can stay clear while TBUF is still occupied.
     */
    while ((USIC0_CH0_PTR->TCSR & USIC_CH_TCSR_TDV_Msk) != 0UL)
    {
        /* wait until the transmit buffer is free */
    }

    USIC0_CH0_PTR->PSCR = USIC_CH_PSCR_CTBIF_Msk;
    USIC0_CH0_PTR->TBUF[0] = (uint16_t)(uint8_t)c;
}

void console_write(const char *data, size_t len)
{
    size_t i;

    for (i = 0U; i < len; ++i)
    {
        console_putc(data[i]);
    }
}

void console_puts(const char *s)
{
    console_write(s, __builtin_strlen(s));
}

size_t console_readline(char *buf, size_t len, uint32_t timeout_ms)
{
    size_t count = 0U;
    uint32_t start = board_millis();

    if ((buf == NULL) || (len == 0U))
    {
        return 0U;
    }

    for (;;)
    {
        char c;

        if ((USIC0_CH0_PTR->PSR & USIC_CH_PSR_ASCMode_RIF_Msk) == 0UL)
        {
            if ((board_millis() - start) >= timeout_ms)
            {
                break;
            }
            continue;
        }

        c = (char)(uint8_t)USIC0_CH0_PTR->RBUF;
        USIC0_CH0_PTR->PSCR = USIC_CH_PSCR_CRIF_Msk;

        if ((c == '\r') || (c == '\n'))
        {
            break;
        }

        if ((count + 1U) < len)
        {
            buf[count++] = c;
        }
    }

    buf[count] = '\0';
    return count;
}
