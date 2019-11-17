#include "boz_api.h"

#include <avr/pgmspace.h>

const PROGMEM char s_sysinfo_boz_v[] = "Bozzard v";
const PROGMEM char s_sysinfo_copyright[] = "(c) Graeme Cole";
const PROGMEM char s_sysinfo_compiled_on[] = "Compiled on";
const PROGMEM char s_sysinfo_compilation_date[] = __DATE__;
byte sysinfo_current_page = 0;

void
sysinfo_init(void *dummy) {
    /* Press any QM button to exit */
    boz_set_event_handler_qm_reset(sysinfo_exit);
    boz_set_event_handler_qm_yellow(sysinfo_exit);
    boz_set_event_handler_qm_play(sysinfo_exit);

    /* Turn the knob to switch pages */
    boz_set_event_handler_qm_rotary(sysinfo_turn_page);

    sysinfo_refresh(0);
}

void
sysinfo_exit(void *cookie) {
    boz_app_exit(0);
}

void
sysinfo_turn_page(void *cookie, int clockwise) {
    if (sysinfo_current_page != clockwise) {
        sysinfo_refresh((byte) clockwise);
    }
}

void
sysinfo_refresh(byte page) {
    sysinfo_current_page = page;

    boz_display_clear();
    if (page == 0) {
        long version = boz_get_version();

        /* Write Bozzard followed by the version number */
        boz_display_write_string_P(s_sysinfo_boz_v);
        for (int i = 0; i < 3; ++i) {
            boz_display_write_long((version >> (24 - i * 8)) & 0xff, 0, NULL);
            if (i < 2)
                boz_display_write_char('.');
        }

        /* Show the copyright information */
        boz_display_set_cursor(1, 0);
        boz_display_write_string_P(s_sysinfo_copyright);
    }
    else {
        boz_display_write_string_P(s_sysinfo_compiled_on);
        boz_display_set_cursor(1, 0);
        boz_display_write_string_P(s_sysinfo_compilation_date);
    }
}
