/**
 * @file    canvas.c
 * @brief   Monochrome 200x200 drawing canvas for the 1.54" e-paper.
 */

#include "canvas.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "epd1in54.h"
#include "fonts.h"

#define CANVAS_W (EPD_WIDTH)
#define CANVAS_HEIGHT (EPD_HEIGHT)

static uint8_t s_fb[(EPD_WIDTH / 8) * EPD_HEIGHT];

void canvas_clear(int black)
{
    memset(s_fb, (black != 0) ? 0x00 : 0xFF, sizeof(s_fb));
}

void canvas_pixel(int x, int y, int black)
{
    uint32_t idx;
    uint8_t mask;

    if (x < 0 || y < 0 || x >= CANVAS_W || y >= CANVAS_HEIGHT)
    {
        return;
    }

    idx = (uint32_t)y * (CANVAS_W / 8) + ((uint32_t)x >> 3);
    mask = (uint8_t)(0x80U >> ((uint32_t)x & 7U));

    if (black != 0)
    {
        s_fb[idx] &= (uint8_t)~mask;
    }
    else
    {
        s_fb[idx] |= mask;
    }
}

void canvas_fill(int x0, int y0, int x1, int y1, int black)
{
    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++)
        {
            canvas_pixel(x, y, black);
        }
    }
}

void canvas_rect(int x0, int y0, int x1, int y1, int black)
{
    for (int x = x0; x <= x1; x++)
    {
        canvas_pixel(x, y0, black);
        canvas_pixel(x, y1, black);
    }
    for (int y = y0; y <= y1; y++)
    {
        canvas_pixel(x0, y, black);
        canvas_pixel(x1, y, black);
    }
}

void canvas_line(int x0, int y0, int x1, int y1, int black)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    for (;;)
    {
        canvas_pixel(x0, y0, black);
        if (x0 == x1 && y0 == y1)
        {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void canvas_char(int x, int y, char c, int black)
{
    const sFONT *f = &Font8;
    const uint8_t *glyph;
    int bpr = ((int)f->Width + 7) / 8;   /* bytes per row (padded) */
    int bpc = (int)f->Height * bpr;      /* bytes per glyph */

    if (c < ' ' || c > '~')
    {
        c = '?';
    }

    glyph = &f->table[(unsigned)(c - ' ') * (unsigned)bpc];

    for (int j = 0; j < (int)f->Height; j++)
    {
        for (int i = 0; i < (int)f->Width; i++)
        {
            uint8_t b = glyph[j * bpr + (i >> 3)];
            if ((b & (uint8_t)(0x80U >> (i & 7))) != 0U)
            {
                canvas_pixel(x + i, y + j, black);
            }
        }
    }
}

void canvas_text(int x, int y, const char *s, int black)
{
    while (*s != '\0')
    {
        canvas_char(x, y, *s, black);
        x += (int)Font8.Width;
        s++;
    }
}

void canvas_textf(int x, int y, int black, const char *fmt, ...)
{
    char buf[64];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    canvas_text(x, y, buf, black);
}

int canvas_text_width(const char *s)
{
    return (int)strlen(s) * (int)Font8.Width;
}

void canvas_char_scaled(int x, int y, char c, int scale, int black)
{
    const sFONT *f = &Font8;
    const uint8_t *glyph;
    int bpr = ((int)f->Width + 7) / 8;
    int bpc = (int)f->Height * bpr;

    if (scale < 1)
    {
        scale = 1;
    }
    if (c < ' ' || c > '~')
    {
        c = '?';
    }

    glyph = &f->table[(unsigned)(c - ' ') * (unsigned)bpc];

    for (int j = 0; j < (int)f->Height; j++)
    {
        for (int i = 0; i < (int)f->Width; i++)
        {
            uint8_t b = glyph[j * bpr + (i >> 3)];
            if ((b & (uint8_t)(0x80U >> (i & 7))) != 0U)
            {
                canvas_fill(x + i * scale, y + j * scale,
                            x + i * scale + scale - 1, y + j * scale + scale - 1, black);
            }
        }
    }
}

void canvas_text_scaled(int x, int y, const char *s, int scale, int black)
{
    if (scale < 1)
    {
        scale = 1;
    }
    while (*s != '\0')
    {
        canvas_char_scaled(x, y, *s, scale, black);
        x += (int)Font8.Width * scale;
        s++;
    }
}

int canvas_text_width_scaled(const char *s, int scale)
{
    if (scale < 1)
    {
        scale = 1;
    }
    return (int)strlen(s) * (int)Font8.Width * scale;
}

const uint8_t *canvas_buffer(void)
{
    return s_fb;
}
