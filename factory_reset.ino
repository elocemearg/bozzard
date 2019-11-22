#include "boz_api.h"

const PROGMEM char s_fr_done[] = "Storage erased";
const PROGMEM char s_fr_ready_to_exit[] = "Press " BOZ_CHAR_PLAY_S " to exit";
const PROGMEM char s_fr_confirm[] = "Erase storage?";
const PROGMEM char s_fr_yes_no[] = " " BOZ_CHAR_PLAY_S " OK  " BOZ_CHAR_RESET_S " Cancel";

void fr_play(void *cookie) {
    boz_leds_set(15);
    boz_eeprom_global_reset();
    boz_leds_set(0);

    boz_display_clear();
    boz_display_write_string_P(s_fr_done);
    boz_display_set_cursor(1, 0);
    boz_display_write_string_P(s_fr_ready_to_exit);
    boz_set_event_handler_qm_play(fr_exit);
    boz_set_event_handler_qm_yellow(fr_exit);
}

void fr_exit(void *cookie) {
    boz_app_exit(0);
}

void factory_reset_init(void *dummy) {
    boz_set_event_handler_qm_play(fr_play);
    boz_set_event_handler_qm_reset(fr_exit);

    boz_display_clear();
    boz_display_write_string_P(s_fr_confirm);
    boz_display_set_cursor(1, 0);
    boz_display_write_string_P(s_fr_yes_no);
}
