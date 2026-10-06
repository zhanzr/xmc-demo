/**
 * @file    main.c
 * @brief   magsensor-2go sweep_io: GPIO discovery sweep + chip-ID check.
 *
 * Announces each drive-capable pin on the VQFN24 package once, then toggles that
 * pin alone for 3 s so the board wiring can be identified by watching it.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "board.h"

#include "XMC1100.h"

/* XMC1 port register offsets (XMC1100.h PORT0_Type). */
#define PORT_OUT_OFFSET   (0x00U)
#define PORT_IOCR0_OFFSET (0x10U)
#define PORT_IN_OFFSET    (0x24U)
#define PORT_PDISC_OFFSET (0x60U)

#define PORT_IOCR_PC_Msk (0xF8UL)

/* Pin-control lane encodings (bits [7:3] of each 8-bit lane). */
#define GPIO_MODE_INPUT_TRISTATE   (0x00UL)
#define GPIO_MODE_OUTPUT_PUSH_PULL (0x80UL)

#define TOGGLE_INTERVAL_MS (200UL)
#define PIN_WINDOW_MS      (3000UL)

/* Chip ID of XMC1100-Q024F0064 (datasheet Table 4); -Q040F0032 reads ...42. */
#define IDCHIP_Q024F0064 (0x00011062UL)
#define IDCHIP_Q024_BIT  (0x00000020UL)

/* Flash config sector 0 holds the same ID as its most significant word. */
#define CHIP_ID_FLASH_ADDR (0x10000F00UL)

typedef struct
{
    uint8_t port;
    uint8_t pin;
    uint8_t pad;     /* VQFN24 pad number (datasheet Table 6) */
    bool input_only; /* STD_IN/AN pad: no output driver on this package */
    const char *name;
} sweep_pin_t;

/*
 * Every bonded pad except the debug pins: P2.1/P2.2 = console UART, and
 * P1.2/P1.3 = SWCLK/SWDIO (board manual). P0.1-P0.4, P0.10, P0.11 and P1.4-P1.6
 * are not bonded on VQFN24 (datasheet Table 6).
 *
 * P2.2-P2.9 are STD_IN/AN pads, so P2.6, P2.7/P2.8 and P2.9 carry no output
 * driver and are listed for completeness only; P2.7 and P2.8 share pad 5.
 */
static const sweep_pin_t sweep_pins[] = {
    { 0U,  0U, 15U, false, "P0.0" },  { 0U,  5U, 16U, false, "P0.5" },
    { 0U,  6U, 17U, false, "P0.6" },  { 0U,  7U, 18U, false, "P0.7" },
    { 0U,  8U, 19U, false, "P0.8" },  { 0U,  9U, 20U, false, "P0.9" },
    { 0U, 12U, 21U, false, "P0.12" }, { 0U, 13U, 22U, false, "P0.13" },
    { 0U, 14U, 23U, false, "P0.14" }, { 0U, 15U, 24U, false, "P0.15" },
    { 1U,  0U, 14U, false, "P1.0" },  { 1U,  1U, 13U, false, "P1.1" },
    { 2U,  0U,  1U, false, "P2.0" },  { 2U,  6U,  4U, true,  "P2.6" },
    { 2U,  7U,  5U, true,  "P2.7/P2.8" }, { 2U, 9U, 6U, true, "P2.9" },
    { 2U, 10U,  7U, false, "P2.10" }, { 2U, 11U,  8U, false, "P2.11" },
};

#define SWEEP_PIN_COUNT (sizeof(sweep_pins) / sizeof(sweep_pins[0]))

static volatile uint32_t *port_ptr(uint32_t port)
{
    static volatile uint32_t *const ports[3] = {
        (volatile uint32_t *)PORT0,
        (volatile uint32_t *)PORT1,
        (volatile uint32_t *)PORT2,
    };

    return ports[port];
}

/* Only the P2 pads are analog-capable (STD_IN/AN or STD_INOUT/AN), so PORT2 is
 * the only port with a writable PDISC; it resets to 0xFFFF, i.e. pads disabled.
 * PORT0/PORT1 PDISC reads as 0 and writing it raises a HardFault, which is why
 * the DFP header declares them __I, so leave those ports alone. */
static void pad_enable(uint32_t port, uint32_t pin)
{
    volatile uint32_t *pdisc;

    if (port != 2U)
    {
        return;
    }

    pdisc = (volatile uint32_t *)((uintptr_t)port_ptr(port) + PORT_PDISC_OFFSET);
    *pdisc &= ~(1UL << pin);
}

static void pin_set_mode(uint32_t port, uint32_t pin, uint32_t mode)
{
    volatile uint32_t *iocr;
    uint32_t shift = 8UL * (pin & 3UL);

    iocr = (volatile uint32_t *)((uintptr_t)port_ptr(port) + PORT_IOCR0_OFFSET + ((pin >> 2U) * 4UL));
    *iocr = (*iocr & ~(PORT_IOCR_PC_Msk << shift)) | (mode << shift);
}

static void pin_write(uint32_t port, uint32_t pin, bool level)
{
    volatile uint32_t *out =
        (volatile uint32_t *)((uintptr_t)port_ptr(port) + PORT_OUT_OFFSET);

    if (level)
    {
        *out |= (1UL << pin);
    }
    else
    {
        *out &= ~(1UL << pin);
    }
}

/* Pad level actually present on the pin, to tell a working output driver from an
 * input-only pad that ignores OUT. */
static bool pin_read(uint32_t port, uint32_t pin)
{
    volatile uint32_t *in =
        (volatile uint32_t *)((uintptr_t)port_ptr(port) + PORT_IN_OFFSET);

    return (*in & (1UL << pin)) != 0UL;
}

static void print_chip_id(void)
{
    uint32_t idchip = SCU_GENERAL->IDCHIP;
    uint32_t idflash = *(volatile const uint32_t *)CHIP_ID_FLASH_ADDR;

    printf("  SCU_GENERAL->IDCHIP : 0x%08lX%s\r\n",
           (unsigned long)idchip,
           (idchip == IDCHIP_Q024F0064) ? "  [Q024F0064 as expected]"
                                        : "  [UNEXPECTED, datasheet Table 4 says 00011062]");
    printf("  flash 0x10000F00   : 0x%08lX%s\r\n",
           (unsigned long)idflash,
           (idflash == IDCHIP_Q024F0064) ? "  [Q024F0064 as expected]"
                                          : "  [UNEXPECTED]");
    printf("  package code       : %s\r\n",
           (idchip & IDCHIP_Q024_BIT) ? "-Q024 (24-pin VQFN)"
                                      : "-Q040 (40-pin VQFN/TSSOP)");
}

static uint32_t count_input_only(void)
{
    uint32_t i;
    uint32_t count = 0U;

    for (i = 0U; i < SWEEP_PIN_COUNT; ++i)
    {
        if (sweep_pins[i].input_only)
        {
            ++count;
        }
    }

    return count;
}

static void print_banner(void)
{
    printf("\r\n");
    printf("magsensor-2go / XMC1100 / sweep_io\r\n");
    print_chip_id();
    printf("  package            : VQFN24\r\n");
    printf("  console (excluded) : P2.1 TXD, P2.2 RXD\r\n");
    printf("  SWD (excluded)     : P1.2 SWCLK, P1.3 SWDIO\r\n");
    printf("  pins to sweep      : %lu (%lu of them input-only)\r\n",
           (unsigned long)SWEEP_PIN_COUNT, (unsigned long)count_input_only());
    printf("  timing             : %lu ms per toggle, %lu ms per pin\r\n",
           (unsigned long)TOGGLE_INTERVAL_MS, (unsigned long)PIN_WINDOW_MS);
    printf("\r\n");
}

int main(void)
{
    uint32_t i;

    board_init();
    print_banner();

    for (;;)
    {
        for (i = 0U; i < SWEEP_PIN_COUNT; ++i)
        {
            const sweep_pin_t *p = &sweep_pins[i];
            bool level = false;
            uint32_t elapsed;

            printf("  %-9s (pad %2lu)%s ", p->name, (unsigned long)p->pad,
                   p->input_only ? " input-only" : "");

            pad_enable(p->port, p->pin);
            pin_write(p->port, p->pin, false);
            pin_set_mode(p->port, p->pin, GPIO_MODE_OUTPUT_PUSH_PULL);

            for (elapsed = 0UL; elapsed < PIN_WINDOW_MS; elapsed += TOGGLE_INTERVAL_MS)
            {
                board_delay_ms(TOGGLE_INTERVAL_MS);
                level = !level;
                pin_write(p->port, p->pin, level);
            }

            printf("done");

            if (p->input_only)
            {
                bool actual = pin_read(p->port, p->pin);

                printf(" [OUT=%lu IN=%lu: %s]",
                       (unsigned long)(level ? 1UL : 0UL),
                       (unsigned long)(actual ? 1UL : 0UL),
                       (actual == level) ? "pad follows" : "no output driver");
            }

            printf("\r\n");

            /* Leave the pad high-impedance so nothing stays driven between pins. */
            pin_set_mode(p->port, p->pin, GPIO_MODE_INPUT_TRISTATE);
        }
    }
}