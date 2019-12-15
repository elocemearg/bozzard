#ifndef _BOZ_LCD_H
#define _BOZ_LCD_H

#define BOZ_LCD_RESET_CGRAM 0x200

void
boz_lcd_send(unsigned int cmd);

void
boz_lcd_clear();

void
boz_lcd_init();

void
boz_lcd_set_cgram_address(byte address);

#if BOZ_HW_REVISION >= 1
byte
boz_lcd_get_backlight_state(void);

void
boz_lcd_set_backlight_state(byte);
#endif

void
boz_lcd_reset_cgram_patterns(void);

#endif
