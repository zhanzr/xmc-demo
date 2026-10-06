/**
 * @file    main.c
 * @brief   magsensor-2go st7735_md096_160x80 bring-up (soft + USIC0_CH1 SPI).
 *
 * Wiring: SCL=P0.8 MOSI=P0.7 CS=P0.9 DC=P0.5 RES=P0.6 BL=P2.0
 *         MISO=P2.6 (unused).
 *
 * Pattern set (kept compact so the image fits the ~60 KB app region): boot
 * banner -> info page -> four-colour quadrant check -> HSV gradient with FPS
 * readout -> LED test. Each phase runs twice, once per bus: the bit-banged
 * GPIO path, then the USIC0_CH1 hardware SPI (mode 3). Backlight is
 * CCU40 slice-0 PWM, 100 % duty for bring-up (duty to be tuned later).
 */

#include <stdio.h>

#include "board.h"
#include "console.h"
#include "system_xmc1100.h"
#include "lcd.h"
#include "interface.h"
#include "backlight.h"

#include "XMC1100.h"

#define SCREEN_W   LCD_Width     /* 160 */
#define SCREEN_H   LCD_Height    /* 80  */
#define FPS_BAND   14            /* bottom rows reserved for the FPS text   */
#define ANIM_H     (SCREEN_H - FPS_BAND)
#define BACK_COLOR LCD_BLACK
#define LED_HALF   1500          /* LED test dwell per state (ms)           */

/* 160 px / 6 px per glyph = 26 chars per line with the 6x12 font. */
#define INFO_DY    12

/* --------------------------------------------------------------------- */
/* FPS counter.                                                          */
static volatile uint32_t g_frames;
static uint32_t         g_last_frames;
static uint32_t         g_fps_last_tick;
static uint32_t         g_fps_color = LCD_WHITE;

static void fps_frame(void)
{
    g_frames++;
}

static void fps_update(void)
{
    uint32_t now = board_millis();
    if (now - g_fps_last_tick >= 1000)
    {
        uint32_t fps = g_frames - g_last_frames;
        g_last_frames = g_frames;
        g_fps_last_tick = now;

        char buf[8];
        buf[0] = 'F'; buf[1] = 'P'; buf[2] = 'S'; buf[3] = ':';
        buf[4] = (char)('0' + (fps / 100) % 10);
        buf[5] = (char)('0' + (fps / 10) % 10);
        buf[6] = (char)('0' + fps % 10);
        buf[7] = '\0';
        LCD_SetColor(g_fps_color);
        LCD_ShowTransparent(1);              /* no opaque box */
        LCD_DisplayString(2, ANIM_H + 2, buf);
        LCD_ShowTransparent(0);
    }
}

static void paint_fps_band(void)
{
    LCD_SetColor(BACK_COLOR);
    LCD_SetBackColor(BACK_COLOR);
    LCD_FillRect(0, ANIM_H, SCREEN_W, FPS_BAND);
}

static void delay_with_fps(uint32_t ms)
{
    uint32_t start = board_millis();
    do
    {
        fps_update();
        board_delay_ms(50);
    } while (board_millis() - start < ms);
}

/* --------------------------------------------------------------------- */
/* Animated gradient: hue sweeps left-to-right across the width.         */
static uint32_t hsv_to_rgb(int h, int s, int v)
{
    int region = (h / 600) % 6;
    int fpart  = h % 600;
    int p = v * (255 - s) / 255;
    int q = v * (255 - (s * fpart) / 600) / 255;
    int t = v * (255 - (s * (600 - fpart)) / 600) / 255;
    int r, g, b;
    switch (region)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default:r = v; g = p; b = q; break;
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void draw_gradient(int hue_a, int hue_b)
{
    for (int x = 0; x < SCREEN_W; x++)
    {
        int frac = x * 1000 / SCREEN_W;
        int hue  = hue_a + (hue_b - hue_a) * frac / 1000;
        LCD_SetColor(hsv_to_rgb(hue, 255, 255));
        LCD_DrawLine_V((uint16_t)x, 0, ANIM_H);
    }
    fps_frame();
}

static void gradient_demo(uint32_t ms)
{
    LCD_SetBackColor(BACK_COLOR);
    paint_fps_band();

    uint32_t start = board_millis();
    uint32_t t = 0;
    do
    {
        int hue_a = (int)(t * 3600 / ms);
        int hue_b = hue_a + 1800;
        if (hue_b >= 3600) { hue_b -= 3600; }
        draw_gradient(hue_a, hue_b);
        fps_update();
        t = board_millis() - start;
    } while (t < ms);
}

/* --------------------------------------------------------------------- */
/* LED test (P1.0, active high).                                         */
static void led_test(void)
{
    printf("[LCD] LED ON\r\n");
    board_led_set(BOARD_LED_1, true);
    delay_with_fps(LED_HALF);
    printf("[LCD] LED OFF\r\n");
    board_led_set(BOARD_LED_1, false);
    delay_with_fps(LED_HALF);
}

/* --------------------------------------------------------------------- */
/* Four-colour quadrants: orientation + window-offset sanity check.      */
static void quadrant_check(void)
{
    LCD_SetColor(LCD_RED);
    LCD_FillRect(0, 0, SCREEN_W / 2, ANIM_H / 2);
    LCD_SetColor(LCD_GREEN);
    LCD_FillRect(SCREEN_W / 2, 0, SCREEN_W / 2, ANIM_H / 2);
    LCD_SetColor(LCD_BLUE);
    LCD_FillRect(0, ANIM_H / 2, SCREEN_W / 2, ANIM_H / 2);
    LCD_SetColor(LCD_WHITE);
    LCD_FillRect(SCREEN_W / 2, ANIM_H / 2, SCREEN_W / 2, ANIM_H / 2);
    /* centre cross in the quadrants' colour, to pin down any shift/split. */
    LCD_SetColor(LCD_BLACK);
    LCD_DrawLine_H(0, (uint16_t)(ANIM_H / 2), SCREEN_W);
    LCD_DrawLine_V((uint16_t)(SCREEN_W / 2), 0, ANIM_H);
}

/* --------------------------------------------------------------------- */
/* Info page: compiler, build date, frequency + bus, the IO map, the
   backlight duty and the chip ID.                                       */
static void info_demo(uint32_t ms, const char *drv, uint32_t spi_hz)
{
    char buf[28];
    char comp[24];
    uint16_t duty = Backlight_GetDuty();

#if defined(__ARMCC_VERSION)
    snprintf(comp, sizeof comp, "AC6 %lu", (unsigned long)__ARMCC_VERSION);
#elif defined(__clang__)
    snprintf(comp, sizeof comp, "Clang %d.%d", __clang_major__, __clang_minor__);
#elif defined(__GNUC__)
    snprintf(comp, sizeof comp, "GCC %d.%d.%d",
             __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
    snprintf(comp, sizeof comp, "unknown");
#endif

    printf("[LCD] info: comp=%s build=%s %s %s\r\n", comp, __DATE__, __TIME__, drv);

    LCD_SetColor(LCD_WHITE);
    LCD_SetBackColor(LCD_BLACK);
    LCD_Clear();

    const int ix = 2;
    int y = 4;
    snprintf(buf, sizeof buf, "%s", comp);
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "Build %s", __DATE__);
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    if (spi_hz != 0U)
    {
        unsigned long mhz = (unsigned long)((spi_hz + 500000UL) / 1000000UL);
        snprintf(buf, sizeof buf, "HW SPI %lu MHz", mhz);
    }
    else
    {
        snprintf(buf, sizeof buf, "SW");
    }
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "SCL=P0.8 MOSI=P0.7");
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "BL=P2.0 %u%% ID 0x%06lX",
             (unsigned)duty, (unsigned long)SCU_GENERAL->IDCHIP & 0xFFFFFFUL);
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "CS=P0.9 DC=P0.5 RES=P0.6");
    LCD_DisplayString(ix, (uint16_t)y, buf);

    uint32_t start = board_millis();
    do
    {
        board_delay_ms(50);
    } while (board_millis() - start < ms);
}

/* --------------------------------------------------------------------- */
/* Banner page (6x12 font).                                              */
static void banner_page(const char *l1, const char *l2, const char *drv,
                        uint32_t fg, uint32_t bg, uint32_t ms)
{
    char line[32];

    g_fps_color = fg;

    snprintf(line, sizeof line, "%s  BL %u%%",
             drv, (unsigned)Backlight_GetDuty());

    LCD_SetColor(fg);
    LCD_SetBackColor(bg);
    LCD_Clear();
    LCD_DisplayString(2, 16, (char *)l1);
    LCD_DisplayString(2, 40, (char *)l2);
    LCD_DisplayString(2, 58, line);

    board_delay_ms(ms);

    g_fps_color = LCD_WHITE;
}

/* --------------------------------------------------------------------- */
/* Full test-pattern set for one bus.                                    */
static void run_patterns(const char *drv, uint32_t spi_hz)
{
    printf("[LCD] phase: info (%s)\r\n", drv);
    info_demo(6000, drv, spi_hz);

    printf("[LCD] phase: quadrant check\r\n");
    quadrant_check();
    board_delay_ms(4000);

    printf("[LCD] phase: gradient (%s)\r\n", drv);
    gradient_demo(6000);

    printf("[LCD] phase: LED test\r\n");
    led_test();
}

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
int main(void)
{
    board_init();

    printf("\r\n==== magsensor-2go (XMC1100-Q024x0064) st7735_md096_160x80 "
           "====\r\n");
    printf("ST7735S 0.96\" 160x80 4-wire SPI: "
           "SCL=P0.8 MOSI=P0.7 CS=P0.9 DC=P0.5 RES=P0.6 BL=P2.0\r\n");
    printf("MADCTL=0x%02X COL_Pre=%u ROW_Pre=%u (tune in lcd.h if shifted)\r\n",
           (unsigned)ST7735_MADCTL, (unsigned)COL_Pre, (unsigned)ROW_Pre);

    Backlight_Init();   /* CCU40 PWM, 100 % duty for bring-up */
    LCD_Init();

    for (;;)
    {
        printf("[LCD] bus: SOFT bit-banged GPIO\r\n");
        lcd_bus_select(LCD_BUS_SOFT);
        Backlight_SetDuty(10U);
        banner_page("magsensor-2go", "ST7735 0.96 160x80",
                    "SW", LCD_YELLOW, LCD_BLUE, 3000);
        run_patterns("soft gpio", 0U);

        printf("[LCD] bus: USIC0_CH1 SPI, mode 3\r\n");
        lcd_bus_select(LCD_BUS_HW);
        Backlight_SetDuty(11U);
        banner_page("magsensor-2go", "ST7735 0.96 160x80",
                    "HW  USIC0_CH1 SPI", LCD_YELLOW, LCD_BLUE, 3000);
        run_patterns("usic0-ch1", lcd_hw_spi_hz());
    }

    return 0;
}