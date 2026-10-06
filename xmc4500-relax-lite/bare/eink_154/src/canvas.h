/**
 * @file    canvas.h
 * @brief   Monochrome 200x200 drawing canvas for the 1.54" e-paper.
 *
 * 1 bit per pixel, MSB first, row-major, bit set = white. Text uses the 5x8
 * Font8 (row-major, padded rows).
 */
#ifndef CANVAS_H
#define CANVAS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void canvas_clear(int black);
void canvas_pixel(int x, int y, int black);
void canvas_fill(int x0, int y0, int x1, int y1, int black);
void canvas_rect(int x0, int y0, int x1, int y1, int black);
void canvas_line(int x0, int y0, int x1, int y1, int black);
void canvas_char(int x, int y, char c, int black);
void canvas_text(int x, int y, const char *s, int black);
void canvas_textf(int x, int y, int black, const char *fmt, ...);

/** @brief Integer-scaled variants (scale 1 = 5x8, 2 = 10x16, 3 = 15x24). */
void canvas_char_scaled(int x, int y, char c, int scale, int black);
void canvas_text_scaled(int x, int y, const char *s, int scale, int black);
int canvas_text_width_scaled(const char *s, int scale);

/** @brief Pixel width of @p s in the 5x8 font. */
int canvas_text_width(const char *s);

/** @brief The raw 5000-byte framebuffer. */
const uint8_t *canvas_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_H */
