/**
 * @file    console.c
 * @brief   Polled UART console on USIC1 channel 0 (TXD = P0.5, RXD = P0.4).
 *
 * P0.4/P0.5 are the U1C0 pads wired to the external COM bridge. The register
 * sequence mirrors the Infineon XMClib UART init
 * (XMC_UART_CH_InitEx + XMC_UART_CH_SetInputSource + XMC_UART_CH_Start) so the
 * behaviour matches the vendor reference without pulling in the whole library.
 */

#include "console.h"

#include "board.h"

#include "XMC4500.h"

/* PORT0 layout: OUT at +0x00, IOCR0 at +0x10, one 8-bit pin lane per pin. */
#define PORT_OUT_OFFSET   (0x00U)
#define PORT_IOCR0_OFFSET (0x10U)
#define PORT_IOCR_PC_Msk  (0xF8UL)

#define GPIO_MODE_INPUT_TRISTATE         (0x00UL)
#define GPIO_MODE_OUTPUT_PUSH_PULL_ALT2  (0x90UL)

#define UART_OVERSAMPLING (16UL)

/* USIC CH FDR.DM = 2 selects the fractional baudrate divider mode. */
#define USIC_FDR_DIVIDER_MODE_FRACTIONAL (0x8000UL)

/* CCR.MODE = 2 selects UART (ASC) mode. */
#define USIC_CCR_MODE_UART (0x2UL)

#define P0_4_PIN (4U)
#define P0_5_PIN (5U)

/* USIC1_C0_DX0_P0_4 from xmc4_usic_map.h: P0.4 is the RXD input (DX0A). */
#define USIC1_C0_DX0_P0_4 (0U)

#define USIC1_CH0_PTR (USIC1_CH0)

static void port0_pin_set_mode(uint32_t pin, uint32_t mode)
{
    volatile uint32_t *iocr =
        (volatile uint32_t *)((uintptr_t)PORT0 + PORT_IOCR0_OFFSET + ((pin >> 2U) * 4UL));
    uint32_t shift = 8UL * (pin & 3UL);

    *iocr = (*iocr & ~(PORT_IOCR_PC_Msk << shift)) | (mode << shift);
}

/** Switch the USIC1 channel 0 on and hold it in IDLE until programmed. */
static void usic1_channel_enable(void)
{
    /* Release USIC1 from the SCU peripheral reset domain (SCU_RESET.PRCLR1).
     * Without this the USIC1 register block reads back as 0xFFFFFFFF and the
     * transmit/baud logic never runs. */
    SCU_RESET->PRCLR1 = SCU_RESET_PRSTAT1_USIC1RS_Msk;

    USIC1_CH0_PTR->KSCFG = USIC_CH_KSCFG_MODEN_Msk | USIC_CH_KSCFG_BPMODEN_Msk;
    while ((USIC1_CH0_PTR->KSCFG & USIC_CH_KSCFG_MODEN_Msk) == 0UL)
    {
        /* wait until the channel leaves reset */
    }

    /* hold the channel in IDLE mode until the frame format is programmed */
    USIC1_CH0_PTR->CCR &= ~USIC_CH_CCR_MODE_Msk;
}

/**
 * @brief Program the baudrate generator for @p baud using the fractional divider.
 *
 * Same search as XMClib's XMC_USIC_CH_SetBaudrate(): find the prescaler value
 * whose fractional part of PDIV is smallest, then combine it with FDR.STEP.
 *
 * The clock fed to the search is the peripheral bus clock (fPB = SystemCoreClock
 * when PBCLKDIV = 0), matching XMC_SCU_CLOCK_GetPeripheralClockFrequency().
 */
static void usic1_set_baudrate(uint32_t baud, uint32_t oversampling)
{
    uint32_t peripheral_clock_100 = board_peripheral_clock_hz() / 100UL;
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

    USIC1_CH0_PTR->FDR = USIC_FDR_DIVIDER_MODE_FRACTIONAL |
                        ((best_prescaler - 1UL) << USIC_CH_FDR_STEP_Pos);

    USIC1_CH0_PTR->BRG =
        (USIC1_CH0_PTR->BRG & ~(USIC_CH_BRG_DCTQ_Msk | USIC_CH_BRG_PDIV_Msk)) |
        ((oversampling - 1UL) << USIC_CH_BRG_DCTQ_Pos) |
        ((best_pdiv_int - 1UL) << USIC_CH_BRG_PDIV_Pos);
}

void console_init(uint32_t baud)
{
    usic1_channel_enable();
    usic1_set_baudrate(baud, UART_OVERSAMPLING);

    /*
     * PCR[ASC]: 1 stop bit, sampling point at half the oversampling period,
     * sample majority decision, transmitter and receiver status enabled.
     */
    USIC1_CH0_PTR->PCR_ASCMode =
        ((((UART_OVERSAMPLING >> 1UL) + 1UL) << USIC_CH_PCR_ASCMode_SP_Pos) |
         USIC_CH_PCR_ASCMode_SMD_Msk |
         USIC_CH_PCR_ASCMode_RSTEN_Msk |
         USIC_CH_PCR_ASCMode_TSTEN_Msk);

    /*
     * SCTR: passive data level high, transmission mode active high,
     * 8 data bits (WLE = FLE = 7).
     */
    USIC1_CH0_PTR->SCTR =
        ((7UL << USIC_CH_SCTR_WLE_Pos) |
         (7UL << USIC_CH_SCTR_FLE_Pos) |
         (1UL << USIC_CH_SCTR_TRM_Pos) |
         USIC_CH_SCTR_PDL_Msk);

    /* TDEN: enable the transmitter, TDSSM: single shift mode. */
    USIC1_CH0_PTR->TCSR = (1UL << USIC_CH_TCSR_TDEN_Pos) | USIC_CH_TCSR_TDSSM_Msk;

    /* CCR = 0 selects no parity generation or checking. */
    USIC1_CH0_PTR->CCR = 0UL;
    USIC1_CH0_PTR->PSCR = 0xFFFFFFFFUL;

    /* Select P0.4 as the USIC receive input (DX0 stage, DX0A). */
    USIC1_CH0_PTR->DX0CR &= ~USIC_CH_DX0CR_DSEL_Msk;
    USIC1_CH0_PTR->DX0CR |= (USIC1_C0_DX0_P0_4 << USIC_CH_DX0CR_DSEL_Pos);

    USIC1_CH0_PTR->CCR =
        (USIC1_CH0_PTR->CCR & ~USIC_CH_CCR_MODE_Msk) | USIC_CCR_MODE_UART;

    /* P0.5 = USIC1_CH0.DOUT0 on ALT2, P0.4 = RXD input. */
    port0_pin_set_mode(P0_5_PIN, GPIO_MODE_OUTPUT_PUSH_PULL_ALT2);
    port0_pin_set_mode(P0_4_PIN, GPIO_MODE_INPUT_TRISTATE);
}

void console_putc(char c)
{
    /*
     * Wait for the transmit buffer to drain. XMClib's XMC_UART_CH_Transmit()
     * polls TCSR.TDV (data-valid) for the same purpose; PSR.BUSY reflects the
     * shift-register state and can stay clear while TBUF is still occupied.
     */
    while ((USIC1_CH0_PTR->TCSR & USIC_CH_TCSR_TDV_Msk) != 0UL)
    {
        /* wait until the transmit buffer is free */
    }

    USIC1_CH0_PTR->PSCR = USIC_CH_PSCR_CTBIF_Msk;
    USIC1_CH0_PTR->TBUF[0] = (uint16_t)(uint8_t)c;

    /* Wait until the word has been loaded into the shift register, then until
     * the transmitter is idle, so the in-flight frame completes before the next
     * write (or a WFI) can disturb it. */
    while ((USIC1_CH0_PTR->TCSR & USIC_CH_TCSR_TDV_Msk) != 0UL)
    {
    }
    while ((USIC1_CH0_PTR->PSR_ASCMode & USIC_CH_PSR_ASCMode_BUSY_Msk) != 0UL)
    {
    }
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

        if ((USIC1_CH0_PTR->PSR & USIC_CH_PSR_ASCMode_RIF_Msk) == 0UL)
        {
            if ((board_millis() - start) >= timeout_ms)
            {
                break;
            }
            continue;
        }

        c = (char)(uint8_t)USIC1_CH0_PTR->RBUF;
        USIC1_CH0_PTR->PSCR = USIC_CH_PSCR_CRIF_Msk;

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
