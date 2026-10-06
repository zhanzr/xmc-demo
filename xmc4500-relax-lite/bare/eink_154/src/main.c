/**
 * @file    main.c
 * @brief   xmc4500-relax-lite 1.54" e-paper test page set.
 *
 * LuatOS Eink-1.54 (SSD1608-class, 200x200, 2-level) over USIC2_CH0 hardware
 * SPI. The st7735 bring-up page set is reproduced in black/white, with an info
 * page that also reports the die temperature and the EVR13/EVR33 monitors.
 * Text is the 5x8 font scaled 2x (body) / 3x (titles).
 */

#include <stdio.h>

#include "xmc_scu.h"

#include "board.h"
#include "canvas.h"
#include "epd1in54.h"
#include "epdif.h"

#define DWELL_MS (4000UL)

/* Die temperature, XMCLib formula (xmc4_scu.h): T[degC] = (RESULT - 605) / 2.05. */
#define DTS_OFFSET    (605)
#define DTS_MILLI_DIV (2050)
#define DTS_MIN_MILLI (-40000)  /* -40 C */
#define DTS_MAX_MILLI (125000)  /* 125 C */

static int32_t read_temperature_millidegc(void)
{
    uint32_t raw;
    int32_t millideg;

    while (XMC_SCU_IsTemperatureSensorReady() == false)
    {
        /* wait for the running conversion */
    }
    raw = XMC_SCU_GetTemperatureMeasurement();
    (void)XMC_SCU_StartTemperatureMeasurement();

    millideg = ((int32_t)raw - DTS_OFFSET) * 1000000 / DTS_MILLI_DIV;

    /* The first conversion after enabling the sensor can be garbage; gate it. */
    if ((millideg < DTS_MIN_MILLI) || (millideg > DTS_MAX_MILLI))
    {
        return 25000; /* 25.000 C fallback */
    }

    return millideg;
}

/* Re-init the panel and push the whole canvas. */
static void epd_show(void)
{
    epd_init(EPD_VARIANT_V1);
    epd_set_update_value(0xC4U);
    epd_set_frame_memory_full(canvas_buffer());
    (void)epd_display_frame();
}

static void centered(int y, const char *s, int scale, int black)
{
    int x = (EPD_WIDTH - canvas_text_width_scaled(s, scale)) / 2;
    canvas_text_scaled((x < 0) ? 0 : x, y, s, scale, black);
}

/* --------------------------------------------------------------------- */
static void banner_page(void)
{
    char buf[32];

    canvas_clear(0);
    canvas_rect(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, 1);
    canvas_rect(1, 1, EPD_WIDTH - 2, EPD_HEIGHT - 2, 1);

    centered(20, "xmc4500", 3, 1);
    centered(50, "relax-lite", 3, 1);
    centered(90, "EINK 1.54", 2, 1);
    centered(110, "200x200", 2, 1);
    centered(132, "SSD1608", 2, 1);

    snprintf(buf, sizeof(buf), "SPI %lu MHz",
             (unsigned long)((epdif_spi_hz() + 500000UL) / 1000000UL));
    centered(156, buf, 2, 1);

    epd_show();
    board_delay_ms(DWELL_MS);
}

/* --------------------------------------------------------------------- */
static void info_page(void)
{
    char buf[40];
    int32_t temp;
    int y = 2;
    const int dy = 18;

    canvas_clear(0);

    canvas_text_scaled(4, y, "xmc4500-relax", 2, 1); y += dy;
    canvas_text_scaled(4, y, "eink_154 1.54\"", 2, 1); y += dy;
    canvas_text_scaled(4, y, "SSD1608 HW SPI", 2, 1); y += dy;
    canvas_text_scaled(4, y, "SCK P5.2 MOSI P5.0", 2, 1); y += dy;
    canvas_text_scaled(4, y, "CS P2.6 DC P5.7", 2, 1); y += dy;
    canvas_text_scaled(4, y, "RES P3.3 BUSY P3.4", 2, 1); y += dy;
    y += 4;

    temp = read_temperature_millidegc();
    snprintf(buf, sizeof(buf), "DTS %lu.%03lu C",
             (unsigned long)((uint32_t)temp / 1000UL),
             (unsigned long)((uint32_t)temp % 1000UL));
    canvas_text_scaled(4, y, buf, 2, 1); y += dy;

    snprintf(buf, sizeof(buf), "EVR13 %lu mV",
             (unsigned long)(XMC_SCU_POWER_GetEVR13Voltage() * 1000.0f));
    canvas_text_scaled(4, y, buf, 2, 1); y += dy;

    snprintf(buf, sizeof(buf), "EVR33 %lu mV",
             (unsigned long)(XMC_SCU_POWER_GetEVR33Voltage() * 1000.0f));
    canvas_text_scaled(4, y, buf, 2, 1); y += dy;

    snprintf(buf, sizeof(buf), "CPU %lu MHz",
             (unsigned long)(board_cpu_frequency_hz() / 1000000UL));
    canvas_text_scaled(4, y, buf, 2, 1);

    epd_show();
    board_delay_ms(DWELL_MS);
}

/* --------------------------------------------------------------------- */
/* Four quadrants: solid black, solid white, checker, dither + centre cross.
 * The four patterns rotate through the quadrants on each pass. */
static void quadrant_page(void)
{
    static unsigned rot = 0U;
    static const uint8_t bayer4[4][4] =
    {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 },
    };
    static const int qx[4] = { 0, 100, 0, 100 };
    static const int qy[4] = { 0, 0, 100, 100 };

    canvas_clear(0);

    for (int k = 0; k < 4; k++)
    {
        int x0 = qx[k];
        int y0 = qy[k];
        int x1 = x0 + 99;
        int y1 = y0 + 99;

        switch ((k + (int)rot) & 3)
        {
            case 0:  /* solid black */
                canvas_fill(x0, y0, x1, y1, 1);
                break;
            case 1:  /* solid white */
                canvas_fill(x0, y0, x1, y1, 0);
                break;
            case 2:  /* checkerboard */
                for (int y = y0; y <= y1; y++)
                {
                    for (int x = x0; x <= x1; x++)
                    {
                        canvas_pixel(x, y, ((x >> 3) + (y >> 3)) & 1);
                    }
                }
                break;
            default: /* 25% dither */
                for (int y = y0; y <= y1; y++)
                {
                    for (int x = x0; x <= x1; x++)
                    {
                        canvas_pixel(x, y, (bayer4[y & 3][x & 3] < 4) ? 1 : 0);
                    }
                }
                break;
        }
    }

    canvas_line(0, 99, 199, 99, 1);          /* cross */
    canvas_line(99, 0, 99, 199, 1);

    epd_show();
    board_delay_ms(DWELL_MS);

    rot = (rot + 1U) & 3U;
}

/* --------------------------------------------------------------------- */
/* Horizontal dithered gradient (black -> white) using a 4x4 Bayer matrix. */
static void gradient_page(void)
{
    static const uint8_t bayer4[4][4] =
    {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 },
    };

    for (int y = 0; y < EPD_HEIGHT; y++)
    {
        for (int x = 0; x < EPD_WIDTH; x++)
        {
            int level = x * 16 / EPD_WIDTH;  /* 0..15 */
            canvas_pixel(x, y, (bayer4[y & 3][x & 3] < level) ? 1 : 0);
        }
    }

    canvas_rect(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, 1);
    epd_show();
    board_delay_ms(DWELL_MS);
}

/* --------------------------------------------------------------------- */
static void led_page(int on)
{
    canvas_clear(0);
    canvas_rect(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, 1);

    centered(50, "LED test", 3, 1);
    centered(100, on ? "LED1 ON" : "LED1 OFF", 2, 1);
    centered(130, on ? "LED2 ON" : "LED2 OFF", 2, 1);

    board_led_set(BOARD_LED_1, on != 0);
    board_led_set(BOARD_LED_2, on != 0);

    epd_show();
    board_delay_ms(DWELL_MS);
}

/* --------------------------------------------------------------------- */
int main(void)
{
    board_init();

    printf("\r\n==== xmc4500-relax-lite eink_154 (SSD1608 200x200) ====\r\n");
    printf("USIC2_CH0 HW SPI: SCK=P5.2 MOSI=P5.0 CS=P2.6 DC=P5.7 RES=P3.3 BUSY=P3.4\r\n");

    XMC_SCU_EnableTemperatureSensor();
    (void)XMC_SCU_StartTemperatureMeasurement();
    /* Discard the first, unstable conversion so page reads are meaningful. */
    while (XMC_SCU_IsTemperatureSensorReady() == false)
    {
    }
    (void)XMC_SCU_GetTemperatureMeasurement();
    (void)XMC_SCU_StartTemperatureMeasurement();

    /* Bring up the SPI before the banner so it can report the real clock. */
    epdif_init();
    printf("[epd] BUSY probe (pull-up) = %d\r\n", epdif_busy_pullup_read());

    for (;;)
    {
        printf("[epd] page: banner\r\n");
        banner_page();

        printf("[epd] page: info\r\n");
        info_page();

        printf("[epd] page: quadrant\r\n");
        quadrant_page();

        printf("[epd] page: gradient\r\n");
        gradient_page();

        printf("[epd] page: LED on\r\n");
        led_page(1);
        printf("[epd] page: LED off\r\n");
        led_page(0);
    }
}
