#include "boz_api.h"

#include <avr/pgmspace.h>

/* If the voltage at VIN is below this, we'll assume we're running from USB
   and the battery isn't switched on. The Arduino Nano can run with 7V-12V at
   VIN. The Bozzard control unit has a 9V supply. */
#define VIN_MIN_MV 6000

const PROGMEM char s_battery_power[] =   "Power:";
const PROGMEM char s_battery_battery[] = "battery";
const PROGMEM char s_battery_usb[] =     "USB    ";
const PROGMEM char s_battery_voltage[] = "Voltage:";
const PROGMEM char s_battery_unknown[] = " unknown";

void
battery_init(void *dummy) {
    /* Press any QM button to exit */
    boz_set_event_handler_qm_reset(battery_exit);
    boz_set_event_handler_qm_yellow(battery_exit);
    boz_set_event_handler_qm_play(battery_exit);

    battery_refresh(NULL);
}

void
battery_exit(void *cookie) {
    boz_app_exit(0);
}

void
battery_refresh(void *cookie) {
    boz_display_clear();

    /* Show "Power:" on the top line */
    boz_display_write_string_P(s_battery_power);

    /* Show "Voltage:" on the bottom line */
    boz_display_set_cursor(1, 0);
    boz_display_write_string_P(s_battery_voltage);

    /* Refresh the voltage display */
    battery_refresh_battery(cookie);
}

void battery_refresh_battery(void *cookie) {
    int millivolts = boz_get_battery_voltage();
    const char *power_source;

    boz_display_set_cursor(1, 8);
    if (millivolts < VIN_MIN_MV) {
        /* We're probably not running from a battery */
        boz_display_write_string_P(s_battery_unknown);
        power_source = s_battery_usb;
    }
    else {
        /* Voltage format: ##.##V */
        int centivolts = (millivolts + 5) / 10;
        boz_display_write_long(centivolts / 100, 2, NULL);
        boz_display_write_char('.');
        boz_display_write_long(centivolts % 100, 2, "0");
        boz_display_write_char('V');
        power_source = s_battery_battery;
    }
    boz_display_set_cursor(0, 7);
    boz_display_write_string_P(power_source);

    /* Call battery_refresh_battery every two seconds until this app exits */
    boz_set_alarm(2000L, battery_refresh_battery, NULL);
}
