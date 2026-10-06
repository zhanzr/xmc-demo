/**
 * @file    fonts.h
 * @brief   Bitmap font descriptors (ST/Truetype-derived 5x8 font).
 *
 * Row-major: each glyph is `Height * ceil(Width/8)` bytes; each row is padded to
 * whole bytes, MSB = leftmost pixel.
 */
#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

typedef struct _tFont
{
    const uint8_t *table;
    uint16_t Width;
    uint16_t Height;
} sFONT;

extern const sFONT Font8;

#endif /* FONTS_H */
