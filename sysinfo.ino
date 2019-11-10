#include "boz.h"

#include <avr/pgmspace.h>

/* If the voltage at VIN is below this, we'll assume we're running from USB
   and the battery isn't switched on. The Arduino Nano can run with 7V-12V at
   VIN. The Bozzard control unit has a 9V supply. */
#define VIN_MIN_MV 6000

const PROGMEM char s_sysinfo_boz[] = "BOZZARD";
const PROGMEM char s_sysinfo_battery[] = "Battery:";
const PROGMEM char s_sysinfo_off[] = " off    ";
const PROGMEM char s_compilation_date[] = __DATE__;

void
sysinfo_init(void *dummy) {
    /* Press any QM button to exit */
    boz_set_event_handler_qm_reset(sysinfo_exit);
    boz_set_event_handler_qm_yellow(sysinfo_exit);
    boz_set_event_handler_qm_play(sysinfo_exit);

    sysinfo_refresh(NULL);
}

void
sysinfo_exit(void *cookie) {
    boz_app_exit(0);
}

void
sysinfo_refresh(void *cookie) {
    byte date_offsets[] = { 4, 0, 7 };
    byte date_lengths[] = { 2, 3, 4 };

    boz_display_clear();

    /* Write BOZZARD followed by the date the software was compiled */
    boz_display_write_string_P(s_sysinfo_boz);
    for (int i = 0; i < 3; ++i) {
        for (int p = date_offsets[i]; p < date_offsets[i] + date_lengths[i]; ++p) {
            boz_display_write_char(pgm_read_byte_near(s_compilation_date + p));
        }
    }

    /* Show the label "Battery:" then print the voltage */
    boz_display_set_cursor(1, 0);
    boz_display_write_string_P(s_sysinfo_battery);

    sysinfo_refresh_battery(cookie);
}

void sysinfo_refresh_battery(void *cookie) {
    int millivolts = boz_get_battery_voltage();

    boz_display_set_cursor(1, 8);
    if (millivolts < VIN_MIN_MV) {
        /* We're probably not running from a battery */
        boz_display_write_string_P(s_sysinfo_off);
    }
    else {
        /* Voltage format: ##.##V */
        int centivolts = (millivolts + 5) / 10;
        boz_display_write_long(centivolts / 100, 2, NULL);
        boz_display_write_char('.');
        boz_display_write_long(centivolts % 100, 2, "0");
        boz_display_write_char('V');
    }

    /* Call sysinfo_refresh_battery once per second until this app exits */
    boz_set_alarm(1000L, sysinfo_refresh_battery, NULL);
}
