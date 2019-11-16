#include "boz.h"

const PROGMEM char s_crash_guru[] = "Guru meditation";

void
crash_init(void *ptr) {
    /* Display a message on the screen, switch on some LEDs, and then do
       nothing until the user switches the Bozzard off and on again */

    boz_display_clear();

    boz_display_write_string_P(s_crash_guru);

    if (ptr)
        boz_leds_set(*(int *) ptr);

    boz_sound_stop_all();
    for (int i = 0; i < 3; ++i) {
        boz_sound_note(NOTE_C2, 500);
        boz_sound_silence(500);
    }
}
