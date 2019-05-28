#include "boz.h"
#include "options.h"
#include "boz_app_inits.h"

/* Rules variables:
   Time limit (0 = no limit)
   Count up/count down (ironically, Countdown Conundrum counts up)
   Buzz stops clock (Yes/No)
   Lockout for N seconds or until QM unlocks
   Warn remaining N seconds
   Re-buzzing allowed (Yes/No)
   1v2v3v4 or 1v234 or 12v34 or 123v4
   Show buzz time (Yes/No) (available only if not 1v2v3v4)
*/

const PROGMEM char s_con_time_limit[] = "Time limit";
const PROGMEM char s_con_warn_remaining[] = "Warn remaining";
const PROGMEM char s_which_buzzers[] = "Which buzzers?";
const PROGMEM char s_con_allow_rebuzz[] = "Allow re-buzz";
const PROGMEM char s_con_buzz_length_ms[] = "Buzz length (ms)";
const PROGMEM char s_con_ready[] = BOZ_CHAR_PLAY_S " to start";
const PROGMEM char s_con_time[] = "TIME UP  " BOZ_CHAR_RESET_S " reset";
const PROGMEM char s_con_left_arrow[] = "\x01\x03\x03\x03\x03";
const PROGMEM char s_con_right_arrow[] = "\x03\x03\x03\x03\x02";
const PROGMEM char s_con_space5[] = "     ";

const PROGMEM char s_which_buzzers_1_234[] = "1 v 2,3,4";
const PROGMEM char s_which_buzzers_12_34[] = "1&2 v 3&4";
const PROGMEM char s_which_buzzers_123_4[] = "1,2,3 v 4";
const PROGMEM char * const which_buzzers_list[] = {
    s_which_buzzers_1_234,
    s_which_buzzers_12_34,
    s_which_buzzers_123_4
};

const PROGMEM struct option_page conundrum_options[] = {
    {
        s_con_time_limit,
        OPTION_TYPE_CLOCK_MIN_SEC,
        0,
        9L * 60L + 59L,
        1,
        NULL,
        NULL,
        0
    },
    {
        s_con_warn_remaining,
        OPTION_TYPE_CLOCK_SEC,
        0,
        60,
        1,
        NULL,
        NULL,
        0
    },
    {
        s_which_buzzers,
        OPTION_TYPE_LIST,
        0,
        0,
        1,
        NULL,
        which_buzzers_list,
        3
    },
    {
        s_con_buzz_length_ms,
        OPTION_TYPE_NUMBER,
        0,
        3000,
        100,
        NULL,
        NULL,
        0
    },
    {
        s_con_allow_rebuzz,
        OPTION_TYPE_YES_NO,
        0,
        0,
        1,
        NULL,
        NULL,
        0
    },
};

long con_options_return[5];
#define CON_OPTIONS_INDEX_TIME_LIMIT 0
#define CON_OPTIONS_INDEX_WARN_TIME 1
#define CON_OPTIONS_INDEX_WHICH_BUZZERS 2
#define CON_OPTIONS_INDEX_BUZZ_LENGTH 3
#define CON_OPTIONS_INDEX_ALLOW_REBUZZ 4

struct option_menu_context con_options_context = {
    conundrum_options,
    5,
    con_options_return
};


struct conundrum_state {
    struct boz_clock *clock;

    /* Whether each player has buzzed yet */
    byte buzzed[2];

    /* The clock time when each player buzzed */
    long buzz_times[2];

    /* If a player buzzes too late (that is, when the clock is stopped after a
       buzz or the expiry of the time), how late was it?
       This is -1 if there is no late buzz since the clock last stopped.
       Otherwise it's the number of milliseconds late the buzz was (which
       can be zero!) */
    long late_buzz_ms[2];

    char current_buzzer; // whose light is on?
    byte time_expired; // has the clock run out?
    byte clock_has_started; // have we started the clock since the last reset?

    // we last gave a warning for this many seconds left
    int most_recent_warning_remain_sec;

    /* Pressing the yellow button generates a random number 101-999 and
       shows it in the top-left corner */
    int generated_target;

    unsigned long last_buzz_at_millis; // millis() when last buzz occurred
    unsigned long time_expired_at_millis; // millis() time when time ran out
};

static struct conundrum_state con_state;
static struct conundrum_state *statep;

int conundrum_time_limit_sec = 30;
int warn_remaining_sec = 5;
int buzz_length_tenths = 8;
char allow_rebuzz = 0;
char prng_seeded = 0;

/* Buzzer number (zero-based) of the leftmost buzzer who is player 2. All
   buzzers less than this will be player 1, all buzzers greater than or equal
   to it will be player 2. */
char first_c2_buzzer = 1;

#define NUM_BUZZERS 4
#define BUZZER_TO_PLAYER(BUZZER) (((BUZZER) < first_c2_buzzer) ? 0 : 1)
#define IS_LEFT_PLAYER(BUZZER) ((BUZZER) < first_c2_buzzer)
#define IS_RIGHT_PLAYER(BUZZER) ((BUZZER) >= first_c2_buzzer)
#define IS_PLAYER_N(BUZZER, PLAYER) ((BUZZER) >= 0 && ((!!(PLAYER)) == ((BUZZER) >= first_c2_buzzer)))

static void update_clock_value_tenths(long ms) {
    long sec = ms / 1000;
    long tenths = (ms % 1000) / 100;
    boz_display_set_cursor(0, 5);
    boz_display_write_long(sec, 2, NULL);
    boz_display_write_char('.');
    boz_display_write_long(tenths, 1, NULL);
    boz_display_write_string("s  ");
}

static void update_clock_value_ms(long ms) {
    long sec = ms / 1000;
    ms %= 1000;

    boz_display_set_cursor(0, 5);
    boz_display_write_long(sec, 2, NULL);
    boz_display_write_char('.');
    boz_display_write_long(ms, 3, "0");
    boz_display_write_char('s');
}

static void update_arrows(int left, int right) {
    boz_display_set_cursor(1, 0);
    if (left) {
        boz_display_write_string_P(s_con_left_arrow);
    }
    else {
        boz_display_write_string_P(s_con_space5);
    }
    boz_display_set_cursor(1, 11);
    if (right) {
        boz_display_write_string_P(s_con_right_arrow);
    }
    else {
        boz_display_write_string_P(s_con_space5);
    }
}

static void update_past_buzz_time(int is_right, long ms) {
    long sec = ms / 1000;
    if (is_right)
        boz_display_set_cursor(1, 9);
    else
        boz_display_set_cursor(1, 0);

    boz_display_write_long(sec, is_right ? 2 : 1, "");
    boz_display_write_char('.');
    boz_display_write_long(ms % 1000, 3, "0");
    boz_display_write_char('s');
}

static void update_late_buzz(int is_right, long ms_late) {
    long sec = ms_late / 1000;
    if (is_right)
        boz_display_set_cursor(1, 9);
    else
        boz_display_set_cursor(1, 0);
    
    if (ms_late < 1000) {
        boz_display_write_long(ms_late, is_right ? 5 : 0, "+");
        boz_display_write_string("ms");
    }
    else {
        boz_display_write_long(sec, is_right ? 3 : 0, "+");
        boz_display_write_char('.');
        boz_display_write_long((ms_late % 1000) / 10, 2, "0");
        boz_display_write_char('s');
    }
    if (!is_right)
        boz_display_write_char(' ');
}

static void redraw_display(struct conundrum_state *state) {
    long ms = boz_clock_value(state->clock);
    boz_display_clear();

    if (state->generated_target) {
        boz_display_write_long(state->generated_target, 3, NULL);
    }

    if (boz_clock_running(state->clock)) {
        update_clock_value_tenths(ms);

        // all lights on while clock is running
        boz_leds_set(0xf);
    }
    else if (!state->clock_has_started) {
        // clock not started yet
        update_clock_value_tenths(ms);

        boz_display_set_cursor(1, 3);
        boz_display_write_string_P(s_con_ready);

        // all lights off before clock is started
        boz_leds_set(0);
    }
    else if (state->time_expired) {
        // time expired
        boz_display_set_cursor(0, 0);
        boz_display_write_string_P(s_con_time);

        // all lights off after clock has finished
        boz_leds_set(0);
    }
    else {
        update_clock_value_ms(ms);
        // lights off if clock has stopped
        boz_leds_set(0);
        boz_display_set_cursor(1, 8);
    }

    if (state->current_buzzer >= 0) {
        // someone has buzzed - point at them
        if (IS_LEFT_PLAYER(state->current_buzzer)) {
            update_arrows(1, 0);
        }
        else {
            update_arrows(0, 1);
        }

        // enable only that player's light
        boz_leds_set(1 << state->current_buzzer);
    }
    
    for (int p = 0; p < 2; ++p) {
        if (state->buzzed[p] && !IS_PLAYER_N(state->current_buzzer, p)) {
            update_past_buzz_time(p, state->buzz_times[p]);
        }
    }
}

void
con_reset_state(struct conundrum_state *state) {
    boz_clock_stop(state->clock);
    boz_clock_reset(state->clock);
    boz_clock_cancel_alarm(state->clock);
    boz_clock_set_expiry_max(state->clock, conundrum_time_limit_sec * 1000L, con_time_expired_handler);
    for (int i = 0; i < 2; ++i) {
        state->buzzed[i] = 0;
        state->buzz_times[i] = 0;
        state->late_buzz_ms[i] = -1;
    }
    state->current_buzzer = -1;
    state->time_expired = 0;
    state->clock_has_started = 0;
    state->most_recent_warning_remain_sec = 0;
    state->generated_target = 0;
    redraw_display(state);
}

/* Reset button event handler */
void
con_reset(void *cookie) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;
    if (state->clock_has_started) {
        con_reset_state(state);
    }
    else {
        /* Reset on the "ready" screen means exit */
        boz_app_exit(0);
    }
}

void
con_alarm_handler(void *cookie, struct boz_clock *clock) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;
    long ms = boz_clock_value(clock);
    long ms_left = conundrum_time_limit_sec * 1000L - ms;

    update_clock_value_tenths(ms);

    if (warn_remaining_sec > 0 && ms_left <= warn_remaining_sec * 1000L) {
        int issued_warning = 0;
        int sec_left = (int) ((ms_left + 999L) / 1000L); // no of secs or part secs left
        if (sec_left > 0 && (state->most_recent_warning_remain_sec == 0 ||
                state->most_recent_warning_remain_sec > sec_left)) {
            //byte warning_sound[] = { NOTE_E3, 0, NOTE_E3, 0 };
            /* It's <= N seconds left and we haven't given an N-second
               warning. Do so now. */

            // Switch the lights off until the next alarm
            boz_leds_set(0);

            // Sound a... drumbeat?
            boz_sound_note(NOTE_G3, 10);
            //boz_sound_arpeggio(warning_sound, 4, 50, 1);

            state->most_recent_warning_remain_sec = sec_left;
            issued_warning = 1;
        }
        if (!issued_warning) {
            /* If we didn't just switch all the LEDs off for a warning,
               switch them back on */
            boz_leds_set(0xf);
        }
    }

    /* Set another alarm for the next .2 second boundary */
    boz_clock_set_alarm(clock, ((ms + 200) / 200) * 200, con_alarm_handler);
}

void
con_time_expired_handler(void *cookie, struct boz_clock *clock) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;
    byte time_up_arp[] = { NOTE_C2, 0, NOTE_C2, 0 };

    boz_clock_stop(clock);
    boz_clock_cancel_alarm(state->clock);
    state->time_expired = 1;
    state->time_expired_at_millis = millis();

    boz_sound_stop_all();
    boz_sound_arpeggio(time_up_arp, 3, 500, 1);
    //boz_sound_note(NOTE_C5, 500);

    redraw_display(state);
}

static void accept_buzz(struct conundrum_state *state, int which_buzzer) {
    int which_player = BUZZER_TO_PLAYER(which_buzzer);

    /* If we get here, the clock is running. Stop the clock, record the buzz
       time, then update the display and LEDs with the current state. */
    boz_clock_stop(state->clock);
    state->current_buzzer = which_buzzer;
    state->last_buzz_at_millis = millis();

    state->buzz_times[which_player] = boz_clock_value(state->clock);
    state->buzzed[which_player] = 1;

    /* Whatever we're bleating out into the world, stop and make a buzzer noise */
    boz_sound_stop_all();

    if (buzz_length_tenths > 0) {
        if (which_player == 0) {
            //boz_sound_varying( NOTE_C8, NOTE_C4, buzz_length_tenths * 100, 1);
            byte arp[] = { NOTE_D6, NOTE_D7, NOTE_D4, NOTE_D5 };
            boz_sound_arpeggio(arp, 4, 100, buzz_length_tenths);
        }
        else {
            //boz_sound_varying( NOTE_C4, NOTE_C8, buzz_length_tenths * 100, 1);
            byte arp[] = { NOTE_A7, NOTE_A8, NOTE_A5, NOTE_A6 };
            boz_sound_arpeggio(arp, 4, 100, buzz_length_tenths);
        }
    }

    redraw_display(state);
}

static int is_entitled_to_buzz(struct conundrum_state *state, int which_player) {
    if (which_player < 0 || which_player > 1)
        return 0;
    else
        return allow_rebuzz || !state->buzzed[which_player];
}

void
con_buzz_handler(void *cookie, int which_buzzer) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;
    int which_player;

    if (which_buzzer < 0 || which_buzzer >= NUM_BUZZERS) {
        // passer-by made a convincing buzzer noise
        return;
    }

    which_player = BUZZER_TO_PLAYER(which_buzzer);

    if (!is_entitled_to_buzz(state, which_player)) {
        // you already had a go and you cocked it up
        return;
    }

    if (state->current_buzzer >= 0 && which_player == BUZZER_TO_PLAYER(state->current_buzzer)) {
        // yes, we heard you, now leave the button alone and answer the QM
        return;
    }

    if (!boz_clock_running(state->clock) || state->current_buzzer >= 0) {
        /* Too late or too early - either way, hop it, but I might
           record the time you did this, just to laugh at you */
        if (state->clock_has_started && (state->current_buzzer >= 0 || state->time_expired)) {
            if (state->late_buzz_ms[which_player] < 0) {
                unsigned long millis_end;
                if (state->current_buzzer >= 0)
                    millis_end = state->last_buzz_at_millis;
                else
                    millis_end = state->time_expired_at_millis;

                state->late_buzz_ms[which_player] = time_elapsed(millis_end, millis());
                update_late_buzz(which_player != 0, state->late_buzz_ms[which_player]);
            }
        }
        return;
    }

    accept_buzz(state, which_buzzer);
}


void
con_play(void *cookie) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;
    /* QM has pressed the Play button.
       If the clock has expired then do nothing.
       If the clock is running then pause it.
       If the clock isn't running then start it. */

    if (state->time_expired) {
        return;
    }

    if (boz_clock_running(state->clock)) {
        /* Stop the clock, redraw the display */
        boz_clock_stop(state->clock);
        boz_clock_cancel_alarm(state->clock);
        redraw_display(state);
    }
    else {
        /* QM pressed play to start or resume the clock */
        long current_clock_value = boz_clock_value(state->clock);

        /* There is no current buzzing player */
        state->current_buzzer = -1;

        /* Remove "late buzz" notifications */
        state->late_buzz_ms[0] = state->late_buzz_ms[1] = -1;

        /* Resume the clock */
        boz_clock_run(state->clock);

        /* Is anyone who is still entitled to buzz currently holding their
           buzzer down? If so, and if the clock has previously been started
           (i.e. someone isn't trying to buzz on 0.000s) stop the clock before
           it does anything else and call the buzz handler. If more than one
           buzzer is held down, the one which was held down first buzzes.
         */
        unsigned long earliest_press_micros;
        int earliest_buzzer = -1;

        if (state->clock_has_started) {
            for (int buzzer = 0; buzzer < NUM_BUZZERS; ++buzzer) {
                unsigned long pressed_since_micros;
                if (is_entitled_to_buzz(state, BUZZER_TO_PLAYER(buzzer)) && boz_is_button_pressed(FUNC_BUZZER, buzzer, &pressed_since_micros)) {
                    if (earliest_buzzer == -1 || time_passed(earliest_press_micros, pressed_since_micros)) {
                        earliest_buzzer = buzzer;
                        earliest_press_micros = pressed_since_micros;
                    }
                }
            }
        }

        /* The clock has started */
        state->clock_has_started = 1;

        if (earliest_buzzer >= 0) {
            /* Someone held down the buzzer in anticipation of the clock
               restarting. The clock restarted and they've now buzzed. */
            accept_buzz(state, earliest_buzzer);
        }
        else {
            /* The usual case: the clock has restarted and no buttons were
               held down. Redraw the display and set an alarm to wake us up
               when we next need to do an update. */

            long alarm_ms;

            redraw_display(state);

            /* Set an alarm for the next .2 second boundary, which is when we'll
               next update the display */
            alarm_ms = ((current_clock_value + 200) / 200) * 200;
            boz_clock_set_alarm(state->clock, alarm_ms, con_alarm_handler);
        }
    }
}

void
con_options_return_callback(void *cookie, int rc) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;
    if (rc == 0) {
        conundrum_time_limit_sec = (int) con_options_return[CON_OPTIONS_INDEX_TIME_LIMIT];
        if (conundrum_time_limit_sec <= 0)
            conundrum_time_limit_sec = 1;

        if (!state->time_expired)
            boz_clock_set_expiry_max(state->clock, conundrum_time_limit_sec * 1000L, con_time_expired_handler);

        warn_remaining_sec = (int) con_options_return[CON_OPTIONS_INDEX_WARN_TIME];
        first_c2_buzzer = (char) (con_options_return[CON_OPTIONS_INDEX_WHICH_BUZZERS] + 1);

        buzz_length_tenths = (int) (con_options_return[CON_OPTIONS_INDEX_BUZZ_LENGTH] / 100);
        allow_rebuzz = (char) con_options_return[CON_OPTIONS_INDEX_ALLOW_REBUZZ];
    }
    redraw_display(state);
}

void
con_rotary_press(void *cookie) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;

    if (!boz_clock_running(state->clock)) {
        con_options_return[CON_OPTIONS_INDEX_TIME_LIMIT] = conundrum_time_limit_sec;
        con_options_return[CON_OPTIONS_INDEX_WARN_TIME] = warn_remaining_sec;
        con_options_return[CON_OPTIONS_INDEX_WHICH_BUZZERS] = first_c2_buzzer - 1;
        con_options_return[CON_OPTIONS_INDEX_BUZZ_LENGTH] = buzz_length_tenths * 100;
        con_options_return[CON_OPTIONS_INDEX_ALLOW_REBUZZ] = allow_rebuzz;

        if (boz_app_call(option_menu_init, &con_options_context, con_options_return_callback, state)) {
            // lolfail
        }
    }
}

void
con_generate_target(void *cookie) {
    struct conundrum_state *state = (struct conundrum_state *) cookie;

    if (!prng_seeded) {
        /* Current time in microseconds plus 1009 times the analog value on an
           unconnected pin gives us a (somewhat) random initial seed. */
        pinMode(A6, INPUT);
        randomSeed(micros() + 1009L * (long) analogRead(A6));
        prng_seeded = 1;
    }

    /* Generate a pseudorandom target between 101 and 999 inclusive. */
    state->generated_target = (int) random(101, 1000);

    boz_display_set_cursor(0, 0);
    boz_display_write_long(state->generated_target, 3, NULL);
}

void
conundrum_init(void *dummy) {
    statep = &con_state;

    statep->clock = boz_clock_create(0, 1);
    con_reset_state(statep);

    boz_set_event_handler_buzz(con_buzz_handler);
    boz_set_event_handler_qm_play(con_play);
    boz_set_event_handler_qm_reset(con_reset);
    boz_set_event_handler_qm_yellow(con_generate_target);
    boz_set_event_handler_qm_rotary_press(con_rotary_press);
    boz_set_event_cookie(statep);

    boz_clock_set_event_cookie(statep->clock, statep);
}
