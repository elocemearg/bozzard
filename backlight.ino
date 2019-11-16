#include "boz.h"

void
backlight_init(void *dummy) {
    /* Just toggle the backlight state, and exit. */
    boz_lcd_set_backlight_state(!boz_lcd_get_backlight_state());
    boz_app_exit(0);
}
