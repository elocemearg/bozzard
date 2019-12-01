#include "boz_api.h"
#include "boz_crash.h"
#include "boz_sound.h"

const PROGMEM char s_crash_guru[] = "Guru Meditation";

void
crash_init(void *ptr) {
    struct boz_crash_arg *arg = (struct boz_crash_arg *) ptr;

    /* Clear the display and write "Guru Meditation" */
    boz_display_clear();
    boz_display_write_string_P(s_crash_guru);

    /* Set the LEDs to the lower four bits of arg->pattern, and write that
       number and the address of the crash to the display, which will look
       something like this:
     
       Guru Meditation
        2D30  0009
       
       First number is where we think boz_crash was called from, second number
       is arg->pattern. */
    if (ptr) {
        boz_leds_set(arg->pattern);
        boz_display_set_cursor(1, 0);
        boz_display_write_long((unsigned long) arg->addr, 4, BOZ_DISP_NUM_SPACES | BOZ_DISP_NUM_ZERO_PAD | BOZ_DISP_NUM_HEX);
        boz_display_write_long(arg->pattern, 4, BOZ_DISP_NUM_SPACES | BOZ_DISP_NUM_ZERO_PAD | BOZ_DISP_NUM_HEX);
        arg->light_state = 1;

        /* Flash the lights on and off according to the given pattern: half
           a second with the pattern, half a second all off. */
        boz_set_alarm(500, crash_alarm_handler, ptr);
    }

    /* Play an ominous-sounding tone */
    boz_sound_stop_all();
    for (int i = 0; i < 5; ++i) {
        boz_sound_note(NOTE_A3, 500);
        boz_sound_silence(500);
    }
}

void
crash_alarm_handler(void *ptr) {
    struct boz_crash_arg *arg = (struct boz_crash_arg *) ptr;
    if (arg->light_state) {
        arg->light_state = 0;
        boz_leds_set(0);
    }
    else {
        arg->light_state = 1;
        boz_leds_set(arg->pattern);
    }
    boz_set_alarm(500, crash_alarm_handler, ptr);
}
