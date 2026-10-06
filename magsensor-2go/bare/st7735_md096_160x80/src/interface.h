/**
 * @file    interface.h
 * @brief   Low-level ST7735 bus primitives for the magsensor-2go XMC1100.
 *
 * Two busses: "soft" bit-banged GPIO (the proven path) and the USIC0_CH1 SPI
 * master (mode 3, 8 MHz). MISO stays unconnected.
 */

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <stdint.h>

typedef enum
{
    LCD_BUS_SOFT = 0,   /* bit-banged GPIO */
    LCD_BUS_HW          /* USIC0_CH1 SPI master, mode 3 */
} lcd_bus_t;

void WriteComm(uint16_t data);
void WriteData(uint16_t data);
void SendData(uint32_t color);
void LCD_WriteDataFast(uint8_t data);   /* raw byte, caller manages CS/DC */
void LCD_BeginData(void);                /* DC high, CS low, for raster bursts */
void LCD_EndData(void);                  /* CS high after a burst */

void lcd_bus_select(lcd_bus_t bus);   /* switch bus, reconfigures SCL/SDA pins */

/* Configured HW-SPI shift clock in Hz (0 / N/A for the soft bus). */
uint32_t lcd_hw_spi_hz(void);

/* Shared XMC1 GPIO helpers (port 0 = PORT0, port 2 = PORT2). */
void lcd_io_init_out(uint8_t port, uint8_t pin); /* push-pull output, cleared */
void lcd_io_bind_af(uint8_t port, uint8_t pin, uint32_t af_lane);
void lcd_io_set(uint8_t port, uint8_t pin);
void lcd_io_clr(uint8_t port, uint8_t pin);

#endif /* __INTERFACE_H */