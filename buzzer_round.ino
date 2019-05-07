#include "boz.h"
#include "options.h"

/* "Buzzer round" app.
   When a buzzer is pressed, it locks out the other buzzers for N seconds or
   until the PLAY button is pressed.
   There is optionally a countdown clock which locks all the buzzers when it
   expires. The clock does not stop when a buzzer is pressed.
*/

const PROGMEM char s_br_play_to_start[] = BOZ_CHAR_PLAY_S " to start";
const PROGMEM char s_br_reset_to_reset[] = BOZ_CHAR_RESET_S " to reset";
const PROGMEM char s_br_play_and_reset[] = BOZ_CHAR_PLAY_S " start  " BOZ_CHAR_RESET_S " reset";

const PROGMEM char s_br_round_time[] = "Round time";
const PROGMEM char s_br_lockout_time[] = "Lockout time (s)";
const PROGMEM char s_br_lockout_null[] = "Yellow button";
const PROGMEM char s_br_buzz_dur_ms[] = "Buzz length (ms)";

const PROGMEM char s_br_ready[] = "READY    ";
const PROGMEM char s_br_time[]  = " TIME    ";

const PROGMEM struct option_page br_options [] = {
    {
        s_br_round_time,
        OPTION_TYPE_CLOCK_MIN_SEC,
        0,
        60*60-1,
        1,
        NULL,
        NULL,
        0,
    },
    {
        s_br_lockout_time,
        OPTION_TYPE_NUMBER,
        1,
        59,
        1,
        s_br_lockout_null,
        NULL,
        0,
    },
    {
        s_br_buzz_dur_ms,
        OPTION_TYPE_NUMBER,
        0,
        3000,
        100,
        NULL,
        NULL,
        0
    },
};

unsigned long time_limit_ms = 0;
long lockout_ms = 2000;
int buzz_duration_ms = 800;
const int clock_update_boundary = 1000;

struct option_menu_context br_omc;
long br_omc_results[3];

struct br_state {
    struct boz_clock *clock;
    char current_buzzer;
    char last_buzzer;
    char time_started;
    char time_expired;
};

struct br_state static_state;


void
br_state_init(struct br_state *state) {
    if (time_limit_ms == 0)
        state->clock = NULL;
    else
        state->clock = boz_clock_create(time_limit_ms, 0);

    br_state_reset(state);
}

void
br_init(void *dummy) {
    boz_display_clear();
    br_state_init(&static_state);

    boz_set_event_cookie(&static_state);
    boz_set_event_handler_buzz(br_buzz);
    boz_set_event_handler_qm_play(br_play);
    boz_set_event_handler_qm_yellow(br_yellow);
    boz_set_event_handler_qm_reset(br_reset);
    boz_set_event_handler_qm_rotary_press(br_rotary_press);
}

void
br_state_reset(struct br_state *state) {
    if (state->clock) {
        boz_clock_stop(state->clock);
        boz_clock_reset(state->clock);
        boz_clock_set_event_cookie(state->clock, state);
        boz_clock_set_expiry_min(state->clock, 0, br_time_expired);
    }
    state->current_buzzer = -1;
    state->last_buzzer = -1;
    state->time_expired = 0;
    state->time_started = 0;
    boz_cancel_alarm();
    redraw_buzzer_indicator(state);
    redraw_clock(state);
}

int
br_state_is_reset(struct br_state *state) {
    if (state->current_buzzer == -1 && state->last_buzzer == -1 &&
            !state->time_started &&
            (state->clock == NULL || !boz_clock_running(state->clock))) {
        return 1;
    }
    else {
        return 0;
    }
}

void
br_reset(void *cookie) {
    struct br_state *state = (struct br_state *) cookie;
    if (br_state_is_reset(state)) {
        boz_app_exit(0);
    }
    else {
        if (state->clock == NULL || !boz_clock_running(state->clock))
            br_state_reset(state);
    }
}

void
br_play(void *cookie) {
    struct br_state *state = (struct br_state *) cookie;
    if (!state->time_expired) {
        if (state->clock != NULL) {
            if (!boz_clock_running(state->clock)) {
                /* Start the clock */
                long value = boz_clock_value(state->clock);
                boz_clock_run(state->clock);
                state->time_started = 1;

                /* Set an alarm for N ms of clock time from now, to update the
                   clock on the display */
                if (value > clock_update_boundary)
                    boz_clock_set_alarm(state->clock, ((value - 1) / clock_update_boundary) * clock_update_boundary, br_clock_update);

                /* Unlock the buzzers if they're locked */
                state->current_buzzer = -1;

                redraw_clock(state);
                redraw_buzzer_indicator(state);
            }
            else {
                /* Pause the clock */
                boz_clock_stop(state->clock);
                redraw_clock(state);
                redraw_buzzer_indicator(state);
                boz_cancel_alarm();
            }
        }
    }
}

void
br_yellow(void *cookie) {
    struct br_state *state = (struct br_state *) cookie;

    /* Unlock the buzzers */
    if (!state->time_expired) {
        state->current_buzzer = -1;
        boz_cancel_alarm();
        redraw_buzzer_indicator(state);
    }
}

void
br_rotary_press(void *cookie) {
    struct br_state *state = (struct br_state *) cookie;
    if (state->clock == NULL || !boz_clock_running(state->clock)) {
        br_omc.options = br_options;
        br_omc.num_options = sizeof(br_options) / sizeof(br_options[0]);

        br_omc_results[0] = time_limit_ms / 1000;
        if (lockout_ms < 0)
            br_omc_results[1] = lockout_ms;
        else
            br_omc_results[1] = lockout_ms / 1000;
        br_omc_results[2] = buzz_duration_ms;

        br_omc.results = br_omc_results;

        boz_app_call(option_menu_init, &br_omc, br_opt_set_return, cookie);
    }
}

void
br_opt_set_return(void *cookie, int rc) {
    struct br_state *state = (struct br_state *) cookie;
    if (rc == 0) {
        time_limit_ms = br_omc_results[0] * 1000;

        if (time_limit_ms == 0) {
            if (state->clock != NULL) {
                boz_clock_release(state->clock);
                state->clock = NULL;
            }
        }
        else {
            if (state->clock == NULL) {
                state->time_started = 0;
                state->clock = boz_clock_create(time_limit_ms, 0);
            }

            boz_clock_set_initial_value(state->clock, time_limit_ms);
            if (!state->time_started) {
                br_state_reset(state);
            }
        }
        if (br_omc_results[1] <= 0)
            lockout_ms = -1;
        else
            lockout_ms = br_omc_results[1] * 1000;

        buzz_duration_ms = (int) br_omc_results[2];
    }
    boz_display_clear();
    redraw_clock(&static_state);
    redraw_buzzer_indicator(&static_state);
}

void
boz_lockout_expire(void *cookie) {
    struct br_state *state = (struct br_state *) cookie;
    state->current_buzzer = -1;
    redraw_buzzer_indicator(state);
}

void
br_buzz(void *cookie, int which_buzzer) {
    struct br_state *state = (struct br_state *) cookie;

    if (which_buzzer < 0 || which_buzzer > 3)
        return;

    if (state->current_buzzer < 0 && (state->clock == NULL || boz_clock_running(state->clock))) {
        if (buzz_duration_ms > 0) {
            byte arp[] = { NOTE_D6, NOTE_D7, NOTE_D4, NOTE_D5 };
            for (int i = 0; i < 4; ++i) {
                arp[i] += 3 * which_buzzer;
            }
            boz_sound_arpeggio(arp, 4, 100, buzz_duration_ms / 100);
        }
        state->current_buzzer = (char) which_buzzer;
        state->last_buzzer = state->current_buzzer;
        redraw_buzzer_indicator(state);

        if (state->clock)
            redraw_clock(state);

        if (lockout_ms > 0)
            boz_set_alarm(lockout_ms, boz_lockout_expire, state);
        else if (lockout_ms == 0)
            boz_lockout_expire(state);
    }
}


void
br_clock_update(void *cookie, struct boz_clock *clock) {
    struct br_state *state = (struct br_state *) cookie;
    long value_ms = boz_clock_value(clock);
    redraw_clock(state);
    if (value_ms > clock_update_boundary)
        boz_clock_set_alarm(state->clock, ((value_ms - 1) / clock_update_boundary) * clock_update_boundary, br_clock_update);
}

void
br_time_expired(void *cookie, struct boz_clock *clock) {
    struct br_state *state = (struct br_state *) cookie;
    byte time_up_arp[] = { NOTE_C3, 0, NOTE_C3, 0 };

    state->time_expired = 1;

    boz_clock_cancel_alarm(clock);
    boz_clock_stop(clock);
    redraw_clock(state);
    redraw_buzzer_indicator(state);
    boz_sound_arpeggio(time_up_arp, 3, 500, 1);
}

void
redraw_buzzer_indicator(struct br_state *state) {
    if (state->current_buzzer >= 0) {
        byte start_cols[] = { 0, 4, 9, 13 };
        boz_leds_set(1 << state->current_buzzer);
        for (int i = 0; i < 4; ++i) {
            boz_display_set_cursor(0, start_cols[i]);
            if (i == state->current_buzzer) {
                boz_display_write_char('1' + state->current_buzzer);
                boz_display_write_string("UP");
            }
            else {
                boz_display_write_string("   ");
            }
        }
    }
    else {
        boz_display_set_cursor(0, 0);
        for (int i = 0; i < 16; ++i)
            boz_display_write_char(' ');
        if (state->clock != NULL && !boz_clock_running(state->clock)) {
            /* Clock is stopped - display instructions on how to
               (re)start it */
            boz_display_set_cursor(0, 0);
            if (state->time_started && !state->time_expired) {
                boz_display_write_string_P(s_br_play_and_reset);
            }
            else {
                boz_display_set_cursor(0, 3);
                if (state->time_expired) {
                    boz_display_write_string_P(s_br_reset_to_reset);
                }
                else {
                    boz_display_write_string_P(s_br_play_to_start);
                }
            }
            boz_leds_set(0);
        }
        else {
            boz_leds_set(0xf);
        }
    }

    /* In the bottom left corner, display who last buzzed */
    boz_display_set_cursor(1, 0);
    if (state->last_buzzer >= 0) {
        boz_display_write_char('1' + state->last_buzzer);
    }
    else {
        boz_display_write_char(' ');
    }
}

void
redraw_clock(struct br_state *state) {
    boz_display_set_cursor(1, 5);
    if (state->clock == NULL) {
        boz_display_write_string_P(s_br_ready);
    }
    else if (state->time_expired) {
        boz_display_write_string_P(s_br_time);
    }
    else {
        long mins, secs, ms;
        long value = boz_clock_value(state->clock);
        int round_up = (!state->time_started || boz_clock_running(state->clock));

        if (round_up) {
            /* Round up to the nearest second, so e.g. 0.01 seconds displays
               as 1 second, but 0.00 displays as 0 */
            value += 999;
            ms = 0;
        }
        else {
            ms = value % 1000;
        }
        mins = value / 60000;
        secs = (value / 1000) % 60;
        boz_display_write_long(mins, 2, "");
        boz_display_write_char(':');
        boz_display_write_long(secs, 2, "0");

        if (round_up) {
            boz_display_write_string("    ");
        }
        else {
            boz_display_write_char('.');
            boz_display_write_long(ms, 3, "0");
        }
    }
}
