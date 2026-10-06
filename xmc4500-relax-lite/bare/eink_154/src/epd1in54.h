/**
 * @file    epd1in54.h
 * @brief   1.54" 200x200 monochrome e-paper driver.
 *
 * Two controller variants share this driver:
 *   - EPD_VARIANT_V2: SSD1681 (Waveshare 1.54" V2) — full refresh ~2 s,
 *     partial ~0.3 s. Matches the likely LuatOS Eink-1.54 controller.
 *   - EPD_VARIANT_V1: SSD1608 (Waveshare 1.54" V1 / Arduino epd1in54) — the
 *     older panel.
 *
 * Framebuffer format: 1 bit per pixel, MSB first, row-major, bit set = white,
 * bit clear = black. Full screen = 200/8 * 200 = 5000 bytes.
 */
#ifndef EPD1IN54_H
#define EPD1IN54_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EPD_WIDTH  (200)
#define EPD_HEIGHT (200)
#define EPD_FB_SIZE ((EPD_WIDTH / 8) * EPD_HEIGHT)

/* Controller variant */
#define EPD_VARIANT_V1 (1) /* SSD1608 */
#define EPD_VARIANT_V2 (2) /* SSD1681 */

/* Shared commands */
#define DRIVER_OUTPUT_CONTROL                0x01
#define BOOSTER_SOFT_START_CONTROL           0x0C
#define DEEP_SLEEP_MODE                      0x10
#define DATA_ENTRY_MODE_SETTING              0x11
#define SW_RESET                             0x12
#define MASTER_ACTIVATION                    0x20
#define DISPLAY_UPDATE_CONTROL_2             0x22
#define WRITE_RAM                            0x24
#define WRITE_RAM_REDUNDANT                  0x26
#define WRITE_VCOM_REGISTER                  0x2C
#define WRITE_LUT_REGISTER                   0x32
#define SET_DUMMY_LINE_PERIOD                0x3A
#define SET_GATE_TIME                        0x3B
#define BORDER_WAVEFORM_CONTROL              0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION 0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION 0x45
#define SET_RAM_X_ADDRESS_COUNTER            0x4E
#define SET_RAM_Y_ADDRESS_COUNTER            0x4F
#define TERMINATE_FRAME_READ_WRITE           0xFF

/** @brief Reset and initialise the panel for the given EPD_VARIANT_*. 0 on success. */
int epd_init(int variant);

/** @brief Send a command byte (DC = 0). */
void epd_send_command(uint8_t command);

/** @brief Send a data byte (DC = 1). */
void epd_send_data(uint8_t data);

/** @brief Block until BUSY goes low (panel idle). */
void epd_wait_until_idle(void);

/** @brief Hardware reset pulse (variant specific). */
void epd_reset(void);

/** @brief Copy a full 200x200 framebuffer into panel RAM (no refresh). */
void epd_set_frame_memory_full(const uint8_t *image_buffer);

/** @brief Fill the whole panel RAM with @p color (0x00 black / 0xFF white). */
void epd_clear_frame_memory(uint8_t color);

/**
 * @brief Refresh the panel from the latest RAM contents.
 *
 * @return 1 if BUSY pulsed high (the controller accepted the update), else 0.
 */
int epd_display_frame(void);

/** @brief Override the display-update-control-2 (0x22) value used by refresh. */
void epd_set_update_value(uint8_t value);

/** @brief Enter deep sleep (wake with epd_init()). */
void epd_sleep(void);

#ifdef __cplusplus
}
#endif

#endif /* EPD1IN54_H */
