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

#ifndef BOZ_ORIGINAL
int
boz_lcd_get_backlight_state(void);

void
boz_lcd_set_backlight_state(int);
#endif

#endif
