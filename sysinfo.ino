#include "boz_api.h"

#include <avr/pgmspace.h>

const PROGMEM char s_sysinfo_boz_v[] = "Bozzard v";
const PROGMEM char s_sysinfo_copyright[] = BOZ_CHAR_COPYRIGHT_S " 2019 G.Cole";

void
sysinfo_init(void *dummy) {
    /* Press red reset button to exit */
    boz_set_event_handler_qm_reset(sysinfo_exit);

    /* If buzzer 0 is being held down, crash - this tests the crash routine */
    if (boz_is_button_pressed(FUNC_BUZZER, 0, NULL))
        boz_crash(0xdead);

    sysinfo_refresh(0);
}

void
sysinfo_exit(void *cookie) {
    boz_app_exit(0);
}

void
sysinfo_refresh(byte page) {
    boz_display_clear();

    long version = boz_get_version();

    /* Write Bozzard followed by the version number */
    boz_display_write_string_P(s_sysinfo_boz_v);
    for (byte i = 0; i < 3; ++i) {
        boz_display_write_long((version >> 24) & 0xff, 0, 0);
        if (i < 2)
            boz_display_write_char('.');
        version <<= 8;
    }

    /* Show the copyright information */
    boz_display_set_cursor(1, 0);
    boz_display_write_string_P(s_sysinfo_copyright);
}
