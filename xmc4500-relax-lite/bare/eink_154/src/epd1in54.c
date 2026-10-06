/**
 * @file    epd1in54.c
 * @brief   1.54" 200x200 monochrome e-paper driver (SSD1681 / SSD1608).
 *
 * C port of the Waveshare `epd1in54` (V1 / SSD1608) and `epd1in54_V2` (V2 /
 * SSD1681) Arduino libraries. The full-frame upload reads the framebuffer
 * directly (ARM has a flat address space, so there is no `pgm_read_byte`).
 */

#include "epd1in54.h"

#include <stdio.h>

#include "epdif.h"

static int s_variant = EPD_VARIANT_V1;
static uint8_t s_update_value = 0xC4;

#define EPD_WAIT_TIMEOUT_MS (1000UL)

/* ---- V1 (SSD1608) 30-byte look-up tables ---- */
static const uint8_t lut_full_update[] =
{
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

/* ---- V2 (SSD1681) 159-byte waveforms (full refresh + partial fast) ---- */
static const uint8_t wf_full_1in54[159] =
{
    0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x48, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x48, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x02,
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00,
    0x22, 0x17, 0x41, 0x00, 0x32, 0x20
};

static void epd_set_lut_v2(const uint8_t *lut)
{
    epd_send_command(WRITE_LUT_REGISTER);
    for (unsigned i = 0; i < 153; i++)
    {
        epd_send_data(lut[i]);
    }
    epd_wait_until_idle();

    epd_send_command(0x3F);
    epd_send_data(lut[153]);
    epd_send_command(0x03);
    epd_send_data(lut[154]);
    epd_send_command(0x04);
    epd_send_data(lut[155]);
    epd_send_data(lut[156]);
    epd_send_data(lut[157]);
    epd_send_command(WRITE_VCOM_REGISTER);
    epd_send_data(lut[158]);
}

static void epd_set_full_window(void)
{
    epd_send_command(SET_RAM_X_ADDRESS_START_END_POSITION);
    epd_send_data(0x00);
    epd_send_data((uint8_t)((EPD_WIDTH - 1) >> 3));
    epd_send_command(SET_RAM_Y_ADDRESS_START_END_POSITION);
    epd_send_data(0x00);
    epd_send_data(0x00);
    epd_send_data((uint8_t)((EPD_HEIGHT - 1) & 0xFF));
    epd_send_data((uint8_t)(((EPD_HEIGHT - 1) >> 8) & 0xFF));

    epd_send_command(SET_RAM_X_ADDRESS_COUNTER);
    epd_send_data(0x00);
    epd_send_command(SET_RAM_Y_ADDRESS_COUNTER);
    epd_send_data(0x00);
    epd_send_data(0x00);
    epd_wait_until_idle();
}

int epd_init(int variant)
{
    s_variant = variant;
    s_update_value = (variant == EPD_VARIANT_V2) ? 0xC7U : 0xC4U;

    epdif_init();
    epd_reset();

    if (variant == EPD_VARIANT_V2)
    {
        epd_wait_until_idle();
        epd_send_command(SW_RESET);
        epd_wait_until_idle();

        epd_send_command(DRIVER_OUTPUT_CONTROL);
        epd_send_data(0xC7);
        epd_send_data(0x00);
        epd_send_data(0x00);

        epd_send_command(DATA_ENTRY_MODE_SETTING);
        epd_send_data(0x03); /* X decrement?? -> increment; Y increment */

        epd_send_command(SET_RAM_X_ADDRESS_START_END_POSITION);
        epd_send_data(0x00);
        epd_send_data(0x18); /* (199+1)/8 = 25 */

        epd_send_command(SET_RAM_Y_ADDRESS_START_END_POSITION);
        epd_send_data(0x00);
        epd_send_data(0x00);
        epd_send_data(0xC7); /* 199 */
        epd_send_data(0x00);

        epd_send_command(BORDER_WAVEFORM_CONTROL);
        epd_send_data(0x01);

        epd_send_command(0x18); /* temperature sensor control */
        epd_send_data(0x80);

        epd_send_command(DISPLAY_UPDATE_CONTROL_2);
        epd_send_data(0xB1); /* load temperature and waveform setting */
        epd_send_command(MASTER_ACTIVATION);

        epd_send_command(SET_RAM_X_ADDRESS_COUNTER);
        epd_send_data(0x00);
        epd_send_command(SET_RAM_Y_ADDRESS_COUNTER);
        epd_send_data(0xC7);
        epd_send_data(0x00);
        epd_wait_until_idle();

        epd_set_lut_v2(wf_full_1in54);
    }
    else
    {
        /* SSD1608 (V1) init. */
        epd_send_command(DRIVER_OUTPUT_CONTROL);
        epd_send_data((EPD_HEIGHT - 1) & 0xFF);
        epd_send_data((uint8_t)(((EPD_HEIGHT - 1) >> 8) & 0xFF));
        epd_send_data(0x00);

        epd_send_command(BOOSTER_SOFT_START_CONTROL);
        epd_send_data(0xD7);
        epd_send_data(0xD6);
        epd_send_data(0x9D);

        epd_send_command(WRITE_VCOM_REGISTER);
        epd_send_data(0xA8);

        epd_send_command(SET_DUMMY_LINE_PERIOD);
        epd_send_data(0x1A);
        epd_send_command(SET_GATE_TIME);
        epd_send_data(0x08);

        epd_send_command(DATA_ENTRY_MODE_SETTING);
        epd_send_data(0x03);

        epd_send_command(WRITE_LUT_REGISTER);
        for (unsigned i = 0; i < 30; i++)
        {
            epd_send_data(lut_full_update[i]);
        }
    }

    return 0;
}

void epd_send_command(uint8_t command)
{
    epdif_write_pin(EPDIF_PIN_DC, 0);
    epdif_spi_transfer(command);
}

void epd_send_data(uint8_t data)
{
    epdif_write_pin(EPDIF_PIN_DC, 1);
    epdif_spi_transfer(data);
}

void epd_wait_until_idle(void)
{
    uint32_t waited = 0U;

    /* BUSY high = busy, low = idle. Bounded so a mis-sequenced panel cannot
     * deadlock the firmware during bring-up. */
    while (epdif_read_pin(EPDIF_PIN_BUSY) != 0)
    {
        if (waited >= EPD_WAIT_TIMEOUT_MS)
        {
            printf("[epd] BUSY timeout (%lu ms), continuing\r\n", (unsigned long)waited);
            return;
        }
        epdif_delay_ms(100);
        waited += 100U;
    }
    if (s_variant == EPD_VARIANT_V2)
    {
        epdif_delay_ms(200);
    }
}

void epd_reset(void)
{
    if (s_variant == EPD_VARIANT_V2)
    {
        epdif_write_pin(EPDIF_PIN_RST, 1);
        epdif_delay_ms(20);
        epdif_write_pin(EPDIF_PIN_RST, 0);
        epdif_delay_ms(5);
        epdif_write_pin(EPDIF_PIN_RST, 1);
        epdif_delay_ms(20);
    }
    else
    {
        epdif_write_pin(EPDIF_PIN_RST, 0);
        epdif_delay_ms(200);
        epdif_write_pin(EPDIF_PIN_RST, 1);
        epdif_delay_ms(200);
    }
}

void epd_set_frame_memory_full(const uint8_t *image_buffer)
{
    epd_set_full_window();

    epd_send_command(WRITE_RAM);
    epdif_write_pin(EPDIF_PIN_DC, 1);
    epdif_spi_write_burst(image_buffer, EPD_FB_SIZE);
}

void epd_clear_frame_memory(uint8_t color)
{
    epd_set_full_window();

    epd_send_command(WRITE_RAM);
    epdif_write_pin(EPDIF_PIN_DC, 1);
    epdif_spi_write_burst_fill(color, EPD_FB_SIZE);

    if (s_variant == EPD_VARIANT_V2)
    {
        /* Also clear the previous-image RAM so a full refresh starts clean. */
        epd_send_command(WRITE_RAM_REDUNDANT);
        epdif_write_pin(EPDIF_PIN_DC, 1);
        epdif_spi_write_burst_fill(color, EPD_FB_SIZE);
    }
}

int epd_display_frame(void)
{
    int busy_seen = 0;
    uint32_t waited = 0U;

    epd_send_command(DISPLAY_UPDATE_CONTROL_2);
    epd_send_data(s_update_value);
    epd_send_command(MASTER_ACTIVATION);
    if (s_variant == EPD_VARIANT_V1)
    {
        epd_send_command(TERMINATE_FRAME_READ_WRITE);
    }

    /* The controller raises BUSY shortly after accepting the update. Poll for it
     * (this is how a present/working panel is told apart from a silent one). */
    while (waited < 400U)
    {
        if (epdif_read_pin(EPDIF_PIN_BUSY) != 0)
        {
            busy_seen = 1;
            break;
        }
        epdif_delay_ms(5);
        waited += 5U;
    }

    epd_wait_until_idle();

    return busy_seen;
}

void epd_set_update_value(uint8_t value)
{
    s_update_value = value;
}

void epd_sleep(void)
{
    epd_send_command(DEEP_SLEEP_MODE);
    if (s_variant == EPD_VARIANT_V2)
    {
        epd_send_data(0x01);
        epdif_delay_ms(200);
    }
    epd_wait_until_idle();

    epdif_write_pin(EPDIF_PIN_RST, 0);
}
