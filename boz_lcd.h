#ifndef _BOZ_LCD_H
#define _BOZ_LCD_H

void
boz_lcd_send(unsigned int cmd);

void
boz_lcd_clear();

void
boz_lcd_init();

void
boz_lcd_set_cgram_address(byte address);

#endif
