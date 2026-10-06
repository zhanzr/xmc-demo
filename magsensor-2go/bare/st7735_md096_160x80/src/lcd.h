/**
 * @file    lcd.h
 * @brief   ST7735S 0.96" 160x80 LCD driver (magsensor-2go port).
 *
 * Panel geometry: ST7735S GRAM 132x162, visible 160x80 in landscape (MADCTL
 * 0x60 = row/col exchange + BGR). The 0.96" glass seats 80 x 160 of that GRAM
 * at (26, 1); through the exchange the drawing window lands at COL_Pre / ROW_Pre
 * below. If the image is shifted or split, re-tune MADCTL/COL_Pre/ROW_Pre.
 *
 * Pins: SCL = P0.8, SDA(MOSI) = P0.7, CS = P0.9, DC = P0.5, RES = P0.6, BL =
 * P2.0 (backlight), MISO = P2.6 unconnected.
 */

#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>

#include "lcd/lcd_fonts.h"

/* ---- Panel geometry (landscape 160x80) ---- */
#define LCD_Width  160
#define LCD_Height 80
#define COL        160
#define ROW        80
#define COL_Pre    0
#define ROW_Pre    24
#define Delay_Time 500

/* ---- ST7735 registers used by the init sequence ---- */
#define ST7735_MADCTL         (0x60U)  /* MV | BGR: landscape 160x80 */
#define ST7735_INVERT_COLORS  (0U)     /* set to 1 to send INVON (0x21) */

/* ---- 4-wire SPI control pins ---- */
#define LCD_SCL_PORT (0U)
#define LCD_SCL_PIN  (8U)
#define LCD_SDA_PORT (0U)
#define LCD_SDA_PIN  (7U)
#define LCD_RS_PORT  (0U)   /* DC */
#define LCD_RS_PIN   (5U)
#define LCD_RST_PORT (0U)
#define LCD_RST_PIN  (6U)
#define LCD_CS_PORT  (0U)
#define LCD_CS_PIN   (9U)
#define LCD_BL_PORT  (2U)   /* backlight */
#define LCD_BL_PIN   (0U)

/* ---- pin accessors (bit-banged SPI) ---- */
#define LCD_SPI_SCL_SET  lcd_io_set(LCD_SCL_PORT, LCD_SCL_PIN)
#define LCD_SPI_SCL_CLR  lcd_io_clr(LCD_SCL_PORT, LCD_SCL_PIN)
#define LCD_SPI_SDA_SET  lcd_io_set(LCD_SDA_PORT, LCD_SDA_PIN)
#define LCD_SPI_SDA_CLR  lcd_io_clr(LCD_SDA_PORT, LCD_SDA_PIN)
#define LCD_RS_SET       lcd_io_set(LCD_RS_PORT, LCD_RS_PIN)
#define LCD_RS_CLR       lcd_io_clr(LCD_RS_PORT, LCD_RS_PIN)
#define LCD_RST_SET      lcd_io_set(LCD_RST_PORT, LCD_RST_PIN)
#define LCD_RST_CLR      lcd_io_clr(LCD_RST_PORT, LCD_RST_PIN)
#define LCD_CS_SET       lcd_io_set(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_CS_CLR       lcd_io_clr(LCD_CS_PORT, LCD_CS_PIN)

/* ---- 24-bit colors (RGB888, converted to RGB565 by LCD_SetColor) ---- */
#define LCD_WHITE   0xFFFFFF
#define LCD_BLACK   0x000000
#define LCD_BLUE    0x0000FF
#define LCD_GREEN   0x00FF00
#define LCD_RED     0xFF0000
#define LCD_CYAN    0x00FFFF
#define LCD_MAGENTA 0xFF00FF
#define LCD_YELLOW  0xFFFF00

/* ---- raw RGB565 colors (vendor screens use these directly) ---- */
#define WHITE   0xFFFF
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define MAGENTA 0xF81F
#define GREEN   0x07E0
#define CYAN    0x7FFF
#define YELLOW  0xFFE0

#define ABS(X) ((X) > 0 ? (X) : -(X))

/* ---- API: init / window / colors ---- */
void LCD_GPIOInit(void);
void LCD_RESET(void);
void LCD_IC_Init(void);
void LCD_Init(void);
void LCD_Reinit(void);
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_SetColor(uint32_t rbg888);
void LCD_SetBackColor(uint32_t rbg888);
void LCD_Clear(void);
void LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/* ---- API: vendor screen-window helper (raw, absolute coords) ---- */
void BlockWrite(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend);

/* ---- API: vendor demo screens ---- */
void DispColor(uint32_t color);
void DispFrame(void);
void DispGrayHor16(void);
void DispBand(void);
void StopDelay(uint16_t ms);

/* ---- API: ASCII text ---- */
void LCD_SetAsciiFont(pFONT *font);
void LCD_ShowTransparent(uint8_t mode);
void LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c);
void LCD_DisplayString(uint16_t x, uint16_t y, char *p);

/* ---- API: 2D drawing ---- */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color);
void LCD_DrawLine_V(uint16_t x, uint16_t y, uint16_t height);
void LCD_DrawLine_H(uint16_t x, uint16_t y, uint16_t width);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r);

/* ---- API: raw buffer blit (RGB565 words) ---- */
void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint16_t *data);

#endif /* __LCD_H */