#include "boz.h"

static struct boz_clock *clock;

const long clock_initial_ms = 0;
const long clock_expiry_ms = 30000L;

int rotary_value = 0;

void
test_update_display_clock(long value, int full) {
    long minutes, seconds, tenths, ms;
    boz_display_set_cursor(1, 5);

    minutes = value / 60000L;
    seconds = (value % 60000) / 1000;
    ms = value % 1000;
    tenths = ms / 100;

    boz_display_write_long(minutes, 2, "0");
    boz_display_write_char(':');
    boz_display_write_long(seconds, 2, "0");
    boz_display_write_char('.');
    if (full) {
        boz_display_write_long(ms, 3, "0");
    }
    else {
        boz_display_write_long(tenths, 1, NULL);
        boz_display_write_char(' ');
        boz_display_write_char(' ');
    }
}

void
test_clock_alarm(void *cookie, struct boz_clock *clock) {
    long value = boz_clock_value(clock);

    test_update_display_clock(value, 0);

    boz_clock_set_alarm(clock, ((value + 200) / 200) * 200, test_clock_alarm);
}

void
test_play(void *cookie) {
    if (boz_clock_running(clock)) {
        boz_clock_stop(clock);
        test_update_display_clock(boz_clock_value(clock), 1);
    }
    else {
        boz_clock_run(clock);
    }
}

void
test_reset(void *cookie) {
    boz_clock_stop(clock);
    boz_clock_reset(clock);
    boz_clock_set_alarm(clock, 200, test_clock_alarm);
    test_update_display_clock(boz_clock_value(clock), 0);
}

void
test_buzz(void *cookie, int which_buzz) {
    boz_clock_stop(clock);
    boz_display_set_cursor(0, 15);
    boz_display_write_long((long) which_buzz + 1, 1, NULL);
    test_update_display_clock(boz_clock_value(clock), 1);
}


void
refresh_rotary_value() {
    boz_display_set_cursor(0, 0);
    boz_display_write_long(rotary_value, 6, NULL);
}

void
test_rotary(void *cookie, int clockwise) {
    if (clockwise)
        rotary_value++;
    else
        rotary_value--;
    refresh_rotary_value();
}

void
test_rotary_press(void *cookie) {
    rotary_value = 0;
    refresh_rotary_value();
}

void test_init(void *cookie) {
    boz_display_clear();

    boz_display_set_cursor(0, 0);
    boz_display_write_long(rotary_value, 6, "+");

    clock = boz_clock_create(clock_initial_ms, 1);

    boz_clock_set_event_cookie(clock, NULL);
    boz_clock_set_alarm(clock, 200, test_clock_alarm);
    boz_clock_set_expiry_max(clock, clock_expiry_ms, test_clock_expired);

    boz_set_event_handler_qm_play(test_play);
    boz_set_event_handler_qm_reset(test_reset);
    boz_set_event_handler_qm_rotary(test_rotary);
    boz_set_event_handler_qm_rotary_press(test_rotary_press);
    boz_set_event_handler_buzz(test_buzz);

    boz_clock_run(clock);
}

void test_clock_expired(void *cookie, struct boz_clock *clock) {
    boz_clock_stop(clock);
    test_update_display_clock(boz_clock_value(clock), 1);

    /*boz_sound_note(440, 1000);

    boz_sound_silence(500);
    boz_sound_varying(440, 880, 1000, 1);

    boz_sound_silence(500);
    boz_sound_varying(880, 440, 1000, 1);

    boz_sound_silence(500);
    boz_sound_varying(440, 1760, 200, 10);

    boz_sound_silence(500);*/

    /*for (int freq = 440; freq <= 1760; freq = (freq * 6) / 5) {
        boz_sound_varying(freq * 4, freq, 200, 1);
    }*/

    // we stan a piezoelectric queen
    boz_sound_varying(NOTE_A6, NOTE_A4, 200, 10);

    // omgwtfarpeggio
    byte arp[] = { NOTE_C4, NOTE_G4, NOTE_E4, NOTE_C5 };
    for (int i = 0; i < 4; ++i) {
        boz_sound_arpeggio(arp, 4, 800, 2);
        for (int j = 0; j < 4; ++j) {
            arp[j] += 5;
        }
    }
}
