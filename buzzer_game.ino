#include "boz_api.h"
#include "options.h"

#include <avr/pgmspace.h>

/* General-purpose program for games where you have lockout buzzers and a
   clock. Exact behaviour is controlled by the values in
   struct game_rules *rules, which must be set before buzzer_game_general_init()
   is called. */


const PROGMEM char s_bg_time_limit[] = "Time limit";
const PROGMEM char s_bg_clock_counts_up[] = "Clock counts up";
const PROGMEM char s_bg_warn_remaining[] = "Warn remaining";
const PROGMEM char s_bg_buzz_stops_clock[] = "Buzz stops clock";
const PROGMEM char s_bg_which_buzzers[] = "Which buzzers?";
const PROGMEM char s_bg_allow_rebuzz[] = "Allow re-buzz";
const PROGMEM char s_bg_lockout_time[] = "Unlock after(ms)";
const PROGMEM char s_bg_lockout_null[] = "Yellow button";
const PROGMEM char s_bg_buzz_length_ms[] = "Buzz length (ms)";
const PROGMEM char s_bg_buzz_noise[] = "Buzzer noise";
const PROGMEM char s_bg_buzz_noise_default[] = "Square Bell";
const PROGMEM char s_bg_buzz_noise_fanfare[] = "Fanfare";
const PROGMEM char s_bg_buzz_noise_rising[] = "Phantom's End";
const PROGMEM char s_bg_buzz_noise_1up[] = "1UP";
const PROGMEM char s_bg_buzz_noise_all[] = "All different";
const PROGMEM char s_bg_time_up_noise[] = "Time up noise";
const PROGMEM char s_bg_time_up_noise_brrp_brrp[] = "Brrp brrp";
const PROGMEM char s_bg_time_up_noise_beep_4[] = "Beep x4";

const PROGMEM char s_bg_ready[] = BOZ_CHAR_PLAY_S " to start";
const PROGMEM char s_bg_time[] = "TIME UP  " BOZ_CHAR_RESET_S " reset";
const PROGMEM char s_bg_left_arrow[] = "\x01\x03\x03\x03\x03";
const PROGMEM char s_bg_right_arrow[] = "\x03\x03\x03\x03\x02";
const PROGMEM char s_bg_space5[] = "     ";
const PROGMEM char s_bg_up[] = "UP ";

const PROGMEM char s_bg_which_buzzers_1_234[] = "1 v 2,3,4";
const PROGMEM char s_bg_which_buzzers_12_34[] = "1&2 v 3&4";
const PROGMEM char s_bg_which_buzzers_123_4[] = "1,2,3 v 4";
const PROGMEM char s_bg_which_buzzers_1234[] =  "1v2v3v4";
const PROGMEM char * const s_bg_which_buzzers_list[] = {
    s_bg_which_buzzers_1_234,
    s_bg_which_buzzers_12_34,
    s_bg_which_buzzers_123_4,
    s_bg_which_buzzers_1234,
};
const PROGMEM char * const s_bg_buzz_noise_names[] = {
    s_bg_buzz_noise_default,
    s_bg_buzz_noise_fanfare,
    s_bg_buzz_noise_rising,
    s_bg_buzz_noise_1up,
    s_bg_buzz_noise_all
};
const PROGMEM char * const s_bg_time_up_noise_names[] = {
    s_bg_time_up_noise_brrp_brrp,
    s_bg_time_up_noise_beep_4,
};

#define BG_OPTIONS_DISABLE_BIT(X) (1 << (X))

#define BG_OPTIONS_INDEX_TIME_LIMIT       0
#define BG_OPTIONS_INDEX_CLOCK_COUNTS_UP  1
#define BG_OPTIONS_INDEX_WARN_TIME        2
#define BG_OPTIONS_INDEX_BUZZ_STOPS_CLOCK 3
#define BG_OPTIONS_INDEX_LOCKOUT_TIME     4
#define BG_OPTIONS_INDEX_ALLOW_REBUZZ     5
#define BG_OPTIONS_INDEX_BUZZ_LENGTH      6
#define BG_OPTIONS_INDEX_BUZZ_NOISE       7
#define BG_OPTIONS_INDEX_TIME_UP_NOISE    8
#define BG_OPTIONS_INDEX_WHICH_BUZZERS    9

#define BG_OPTIONS_LENGTH                10

const PROGMEM struct option_page buzzer_game_options[] = {
    {
        s_bg_time_limit,
        OPTION_TYPE_CLOCK_MIN_SEC,
        0,
        59L * 60L + 59L,
        1,
        NULL,
        NULL,
        0
    },
    {
        s_bg_clock_counts_up,
        OPTION_TYPE_YES_NO,
        0,
        0,
        1,
        NULL,
        NULL,
        0
    },
    {
        s_bg_warn_remaining,
        OPTION_TYPE_CLOCK_SEC,
        0,
        60,
        1,
        NULL,
        NULL,
        0
    },
    {
        s_bg_buzz_stops_clock,
        OPTION_TYPE_YES_NO,
        0,
        0,
        1,
        NULL,
        NULL,
        0
    },
    {
        s_bg_lockout_time,
        OPTION_TYPE_NUMBER,
        250,
        10000L,
        250,
        s_bg_lockout_null,
        NULL,
        0
    },
    {
        s_bg_allow_rebuzz,
        OPTION_TYPE_YES_NO,
        0,
        0,
        1,
        NULL,
        NULL,
        0
    },
    {
        s_bg_buzz_length_ms,
        OPTION_TYPE_NUMBER,
        0,
        3000,
        100,
        NULL,
        NULL,
        0
    },
    {
        s_bg_buzz_noise,
        OPTION_TYPE_LIST,
        0,
        0,
        1,
        NULL,
        s_bg_buzz_noise_names,
        sizeof(s_bg_buzz_noise_names) / sizeof(s_bg_buzz_noise_names[0])
    },
    {
        s_bg_time_up_noise,
        OPTION_TYPE_LIST,
        0,
        0,
        1,
        NULL,
        s_bg_time_up_noise_names,
        sizeof(s_bg_time_up_noise_names) / sizeof(s_bg_time_up_noise_names[0])
    },
    {
        s_bg_which_buzzers,
        OPTION_TYPE_LIST,
        0,
        0,
        1,
        NULL,
        s_bg_which_buzzers_list,
        sizeof(s_bg_which_buzzers_list) / sizeof(s_bg_which_buzzers_list[0])
    },
};

struct game_rules {
    /* time_limit_sec
       The time limit for the game. This is in seconds. If the time limit is 0,
       then the clock has no limit and always counts up, regardless of what
       clock_counts_up says.
    */
    int time_limit_sec;

    /* lockout_time_ms
       When a buzz is accepted, and if buzz_stops_clock is false, keep that
       buzzer's LED on and prevent anyone else from buzzing for this many
       milliseconds. If lockout_time_ms is zero, the buzzers will never
       automatically unlock - the QM must unlock them with the yellow button.

       If buzz_stops_clock is true, lockout_time_ms has no effect - the LED
       stays on and the other buzzers are locked out until the QM restarts the
       clock.

       Manually stopping and starting the clock with the QM controls always
       unlocks the buzzers. */
    unsigned int lockout_time_ms;

    /* warn_remaining_sec
       If nonzero, a warning pip will be given every second, on the second,
       from when this many seconds are remaining.
    */
    int warn_remaining_sec;

    /* buzz_length_tenths
       Length of the buzzer noise, in tenths of a second.
    */
    int buzz_length_tenths;

    /* clock_counts_up
       If true, the clock starts at zero and counts up until it reaches
       time_limit_sec (if set), at which point the game ends.
       If false, the clock starts at time_limit_sec seconds and counts down
       until it reads zero, at which point the game ends.
       If time_limit_sec is zero, clock_counts_up has no effect - the clock
       will count up regardless.
    */
    unsigned int clock_counts_up : 1;

    /* buzz_stops_clock
       If true, an accepted buzz will stop the clock. The clock must be
       manually restarted by the QM pressing the green (play) button.
       If false, a buzz will not stop the clock. The clock may still be
       manually stopped and restarted by the QM.
    */
    unsigned int buzz_stops_clock : 1;

    /* allow_rebuzz
       If true, there is no restriction on how many times each buzzer, or side,
       may buzz.
       If false, each side (see below) is only allowed one buzz for the
       duration of the game.
    */
    unsigned int allow_rebuzz : 1;

    /* show_buzz_time
       If true, when a player buzzes, the time showing on the clock when they
       buzzed shows on the bottom line of the display, either on the left or
       right depending on which side (see below) buzzed.
       If false, the bottom line shows 1UP, 2UP, 3UP or 4UP, depending on who
       last buzzed.
    */
    unsigned int show_buzz_time : 1;

    /* yellow_generates_target
       If true, the yellow button generates a pseudorandom number between 101
       and 999 inclusive and displays it in the top-left corner. This is pretty
       much only useful for Countdown.
       If false, the yellow button unlocks the buzzers. This is useful if
       lockout_time_ms is large (or zero), if you want to unlock the buzzers
       early (or at all).
    */
    unsigned int yellow_generates_target : 1;

    /* two_sides
       If true, all buzzers whose (zero-based) index is less than
       first_c2_buzzer are the "left" team or side, and all buzzers whose
       index is equal to or greater than first_c2_buzzer are on the "right"
       team or side.
       If false, there are no teams or sides, each of the four buzzers is its
       own player. */
    unsigned int two_sides : 1;

    /* If two_sides is true, first_c2_buzzer is the buzzer index (zero-based)
       of the leftmost buzzer who is on the right-hand side. All buzzers less
       than this will be left side, all buzzers greater than or equal to it
       will be right side. */
    char first_c2_buzzer;

    /* One of the buzzer noise IDs supported by make_buzzer_noise. */
    char buzzer_noise;

    /* One of the time-up noise IDs supported by make_time_up_noise. */
    char time_up_noise;

    /* A bitmask telling us which pages of the option menu NOT to show. */
    unsigned int opt_page_disable_mask;
};

/* Rules for the current game (pointer to dynamically allocated memory) */
static struct game_rules *rules;

const PROGMEM struct game_rules conundrum_rules = {
    30,   // time_limit_sec
    0,    // lockout_time_ms
    5,    // warn_remaining_sec
    8,    // buzz_length_tenths
    1,    // clock_counts_up
    1,    // buzz_stops_clock
    0,    // allow_rebuzz
    1,    // show_buzz_time
    1,    // yellow_generates_target
    1,    // two_sides
    2,    // first_c2_buzzer (2): red and green are C1, yellow and blue are C2
    0,    // buzzer_noise
    0,    // time_up_noise
    // opt_page_disable_mask
    BG_OPTIONS_DISABLE_BIT(BG_OPTIONS_INDEX_CLOCK_COUNTS_UP) |
        BG_OPTIONS_DISABLE_BIT(BG_OPTIONS_INDEX_BUZZ_STOPS_CLOCK) |
        BG_OPTIONS_DISABLE_BIT(BG_OPTIONS_INDEX_LOCKOUT_TIME)
};

const PROGMEM struct game_rules basic_buzzer_rules = {
    0,    // time_limit_sec
    3000, // lockout_time_ms
    0,    // warn_remaining_sec
    8,    // buzz_length_tenths
    0,    // clock_counts_up
    0,    // buzz_stops_clock
    1,    // allow_rebuzz
    0,    // show_buzz_time
    0,    // yellow_generates_target
    0,    // two_sides
    1,    // first_c2_buzzer
    0,    // buzzer_noise
    0,    // time_up_noise
    0,    // opt_page_disable_mask
};

#define BUZZER_NOISE_DEFAULT 0
#define BUZZER_NOISE_FANFARE 1
#define BUZZER_NOISE_RISING 2
#define BUZZER_NOISE_1UP 3
#define BUZZER_NOISE_ALL 4

#define TIME_UP_NOISE_BRRP_BRRP 0
#define TIME_UP_NOISE_BEEP_4    1

#define NUM_BUZZERS 4

struct buzzer_game_state {
    boz_clock clock;

    /* Whether each side (or player) has buzzed yet. If two_sides is not set,
       then we use all four elements, otherwise we only use the first two. */
    byte buzzed[NUM_BUZZERS];

    /* The clock time when each side (or player) buzzed */
    long buzz_times[NUM_BUZZERS];

    /* If a player buzzes too late (that is, when the clock is stopped after a
       buzz or the expiry of the time), how late was it?
       This is -1 if there is no late buzz since the clock last stopped.
       Otherwise it's the number of milliseconds late the buzz was (which
       can be zero!) */
    long late_buzz_ms[NUM_BUZZERS];

    char current_buzzer; // whose light is on?
    char last_buzzer; // who buzzed last?
    byte time_expired; // has the clock run out?
    byte clock_has_started; // have we started the clock since the last reset?

    // we last gave a warning for this many seconds left
    int most_recent_warning_remain_sec;

    /* Pressing the yellow button generates a random number 101-999 and
       shows it in the top-left corner */
    int generated_target;

    unsigned long last_buzz_at_millis; // millis() when last buzz occurred
    unsigned long time_expired_at_millis; // millis() time when time ran out

    /* If we call the option menu app, we allocate a struct option_menu_context
       and point this to it. The option menu app puts the selected option
       values in here and returns. Our return callback function frees it. */
    struct option_menu_context *bg_options_context;
};

// Pointer to dynamically allocated memory
static struct buzzer_game_state *bg_state;

#define BUZZER_TO_SIDE(BUZZER) (!rules->two_sides ? (BUZZER) : (((BUZZER) < rules->first_c2_buzzer) ? 0 : 1))
#define IS_LEFT_SIDE(BUZZER) ((BUZZER) < rules->first_c2_buzzer)
#define IS_RIGHT_SIDE(BUZZER) ((BUZZER) >= rules->first_c2_buzzer)
#define IS_SIDE_N(BUZZER, SIDE) ((BUZZER) >= 0 && ((!!(SIDE)) == ((BUZZER) >= rules->first_c2_buzzer)))

char prng_seeded = 0;

static void update_clock_value(long ms, int decimal_places) {
    int divisor = 1;
    for (int i = decimal_places; i < 3; ++i)
        divisor *= 10;

    long minutes = ms / 60000L;
    long seconds = (ms / 1000L) % 60;
    long frac = (ms % 1000) / divisor;
    boz_display_set_cursor(0, 4);
    if (minutes > 0 || rules->time_limit_sec == 0 || rules->time_limit_sec > 60) {
        boz_display_write_long(minutes, 2, "");
        boz_display_write_char(':');
        boz_display_write_long(seconds, 2, "0");
    }
    else {
        boz_display_write_long(seconds, 3, NULL);
    }
    boz_display_write_char('.');
    boz_display_write_long(frac, decimal_places, "0");
    for (int i = decimal_places; i < 3; ++i) {
        boz_display_write_char(' ');
    }
}

static void update_clock_value_tenths(long ms) {
    update_clock_value(ms, 1);
}

static void update_clock_value_ms(long ms) {
    update_clock_value(ms, 3);
}

static void bg_set_leds(struct buzzer_game_state *state) {
    byte leds;
    if (!state->clock_has_started || state->time_expired)
        leds = 0;
    else if (state->current_buzzer < 0) {
        if (!boz_clock_running(state->clock))
            leds = 0;
        else
            leds = 0xf;
    }
    else {
        leds = 1 << state->current_buzzer;
    }

    boz_leds_set(leds);
}

static void update_arrows(int left, int right) {
    boz_display_set_cursor(1, 0);
    if (left) {
        boz_display_write_string_P(s_bg_left_arrow);
    }
    else {
        boz_display_write_string_P(s_bg_space5);
    }
    boz_display_set_cursor(1, 11);
    if (right) {
        boz_display_write_string_P(s_bg_right_arrow);
    }
    else {
        boz_display_write_string_P(s_bg_space5);
    }
}

static void update_buzz_indicator(int which_buzzer) {
    boz_display_set_cursor(1, 0);
    for (int b = 0; b < 4; ++b) {
        if (b == which_buzzer) {
            boz_display_write_long(b + 1, 1, "");
            boz_display_write_string_P(s_bg_up);
        }
        else {
            boz_display_write_string_P(s_bg_space5 + 1);
        }
    }
}

static void update_past_buzz_time(int is_right, long ms) {
    if (is_right)
        boz_display_set_cursor(1, 9);
    else
        boz_display_set_cursor(1, 0);

    if (ms >= 60000L) {
        /* If the buzz is one minute or more, use the format MM:SS.T */
        boz_display_write_long(ms / 60000L, is_right ? 2 : 1, "");
        boz_display_write_char(':');
        boz_display_write_long((ms / 1000L) % 60, 2, "0");
        boz_display_write_char('.');
        boz_display_write_long((ms / 100) % 10, 1, "0");
    }
    else {
        /* If the buzz is on less than a minute, use the format SS.TTTs */
        boz_display_write_long(ms / 1000, is_right ? 2 : 1, "");
        boz_display_write_char('.');
        boz_display_write_long(ms % 1000, 3, "0");
        boz_display_write_char('s');
    }
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

static void redraw_display(struct buzzer_game_state *state) {
    long ms = boz_clock_value(state->clock);
    boz_display_clear();

    if (state->generated_target) {
        boz_display_write_long(state->generated_target, 3, NULL);
    }

    if (boz_clock_running(state->clock)) {
        update_clock_value_tenths(ms);
    }
    else if (!state->clock_has_started) {
        // clock not started yet
        update_clock_value_tenths(ms);

        boz_display_set_cursor(1, 3);
        boz_display_write_string_P(s_bg_ready);
    }
    else if (state->time_expired) {
        // time expired
        boz_display_set_cursor(0, 0);
        boz_display_write_string_P(s_bg_time);
    }
    else {
        update_clock_value_ms(ms);
        boz_display_set_cursor(1, 8);
    }

    if (state->current_buzzer >= 0) {
        // someone has buzzed - point at them
        if (!rules->two_sides) {
            update_buzz_indicator(state->current_buzzer);
        }
        else {
            if (IS_LEFT_SIDE(state->current_buzzer)) {
                update_arrows(1, 0);
            }
            else {
                update_arrows(0, 1);
            }
        }
    }
    else if (!rules->two_sides && !rules->show_buzz_time && state->clock_has_started) {
        update_buzz_indicator(state->last_buzzer);
    }

    bg_set_leds(state);
 
    if (rules->two_sides && rules->show_buzz_time) {
        for (int p = 0; p < 2; ++p) {
            if (state->buzzed[p] && !IS_SIDE_N(state->current_buzzer, p)) {
                update_past_buzz_time(p, state->buzz_times[p]);
            }
        }
    }
}

void
bg_reset_state(struct buzzer_game_state *state) {
    boz_clock_stop(state->clock);
    set_up_clock(state);
    boz_cancel_alarm();
    boz_clock_reset(state->clock);
    boz_clock_cancel_alarm(state->clock);

    for (int i = 0; i < NUM_BUZZERS; ++i) {
        state->buzzed[i] = 0;
        state->buzz_times[i] = 0;
        state->late_buzz_ms[i] = -1;
    }
    state->current_buzzer = -1;
    state->last_buzzer = -1;
    state->time_expired = 0;
    state->clock_has_started = 0;
    state->most_recent_warning_remain_sec = 0;
    state->generated_target = 0;
    redraw_display(state);
}

/* Reset button event handler */
void
bg_reset(void *cookie) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;
    if (state->clock_has_started) {
        bg_reset_state(state);
    }
    else {
        /* Reset on the "ready" screen means exit */
        boz_app_exit(0);
    }
}

void
bg_alarm_handler(void *cookie, boz_clock clock) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;
    long ms = boz_clock_value(clock);
    long ms_left;
    
    if (boz_clock_is_direction_forwards(clock))
        ms_left = rules->time_limit_sec * 1000L - ms;
    else
        ms_left = ms;

    update_clock_value_tenths(ms);

    if (rules->warn_remaining_sec > 0 && ms_left <= rules->warn_remaining_sec * 1000L) {
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
            boz_sound_note(NOTE_G3, 5);
            //boz_sound_arpeggio(warning_sound, 4, 50, 1);

            state->most_recent_warning_remain_sec = sec_left;
            issued_warning = 1;
        }
        if (!issued_warning) {
            /* If we didn't just switch all the LEDs off for a warning,
               switch them back on */
            bg_set_leds(state);
        }
    }

    /* Set another alarm for the next .2 second boundary */
    if (boz_clock_is_direction_forwards(clock))
        boz_clock_set_alarm(clock, ((ms + 200) / 200) * 200, bg_alarm_handler);
    else
        boz_clock_set_alarm(clock, ((ms - 1) / 200) * 200, bg_alarm_handler);
}

static void make_time_up_noise(char noise) {
    byte brrp_brrp_arp[] = { NOTE_C2, 0 };
    byte beep_4_arp[] = { NOTE_G5, 0 };

    switch (noise) {
        default:
        case TIME_UP_NOISE_BRRP_BRRP:
            boz_sound_arpeggio(brrp_brrp_arp, 2, 333, 2);
            break;

        case TIME_UP_NOISE_BEEP_4:
            boz_sound_arpeggio(beep_4_arp, 2, 333, 4);
            break;
    }
}


static void make_buzzer_noise(int noise, int buzzer) {
    /* If noise is BUZZER_NOISE_ALL, then give each buzzer a different noise */
    if (noise == BUZZER_NOISE_ALL)
        noise = (buzzer & 3);

    switch (noise) {
        default:
        case BUZZER_NOISE_DEFAULT:
            boz_sound_square_bell(buzzer, rules->buzz_length_tenths);
            break;

        case BUZZER_NOISE_FANFARE: {
            /* This is about one and a half times the length specified in
               rules->buzz_length_tenths, but nobody will notice */
            int beat_ms = rules->buzz_length_tenths * 15;
            byte notes[] = { NOTE_C5, NOTE_F5, NOTE_A5, NOTE_C6, 0, NOTE_A5, NOTE_C6 };
            byte beats[] = { 1, 1, 1, 1, 1, 1, 4 };

            for (int i = 0; i < (int) (sizeof(notes) / sizeof(notes[0])); ++i) {
                boz_sound_note((boz_note) (notes[i] == 0 ? 0 : (notes[i] + 3 * buzzer)), beat_ms * beats[i]);
            }
        }
        break;

        case BUZZER_NOISE_RISING: {
            boz_sound_varying(NOTE_C4, NOTE_C8, 100 * rules->buzz_length_tenths, 1);
        }
        break;

        case BUZZER_NOISE_1UP: {
            byte arp[] = { NOTE_F6, NOTE_F4, NOTE_F6 };
            for (int i = 0; i < 3; ++i) {
                boz_sound_arpeggio(arp, 3, rules->buzz_length_tenths * 100 / 3, 1);
                for (int j = 0; j < 3; ++j) {
                    arp[j] += 2;
                }
            }
        }
        break;
    }
}

void
bg_time_expired_handler(void *cookie, boz_clock clock) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;

    boz_clock_stop(clock);
    boz_clock_cancel_alarm(state->clock);
    state->time_expired = 1;
    state->time_expired_at_millis = millis();

    boz_sound_stop_all();
    make_time_up_noise(rules->time_up_noise);

    redraw_display(state);
}

static void bg_unlock_buzzers(void *statev) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) statev;
    state->current_buzzer = -1;
    redraw_display(state);
}

static void accept_buzz(struct buzzer_game_state *state, int which_buzzer) {
    /* If we get here, the clock is running. Stop the clock if the rules
       require it, record the buzz time, then update the display and LEDs
       with the current state. */
    if (rules->buzz_stops_clock) {
        boz_clock_stop(state->clock);
    }
    else {
        if (rules->lockout_time_ms > 0) {
            boz_set_alarm((long) rules->lockout_time_ms, bg_unlock_buzzers, state);
        }
    }
    state->current_buzzer = which_buzzer;
    state->last_buzzer = which_buzzer;
    state->last_buzz_at_millis = millis();

    if (rules->two_sides) {
        int which_side = BUZZER_TO_SIDE(which_buzzer);
        state->buzz_times[which_side] = boz_clock_value(state->clock);
        state->buzzed[which_side] = 1;
    }
    else {
        state->buzzed[which_buzzer] = 1;
    }

    /* Whatever we're bleating out into the world, stop and make a buzzer noise */
    boz_sound_stop_all();

    if (rules->buzz_length_tenths > 0) {
        make_buzzer_noise(rules->buzzer_noise, which_buzzer);
    }

    redraw_display(state);
}

static int is_entitled_to_buzz(struct buzzer_game_state *state, int which_buzzer) {
    if (!rules->two_sides) {
        if (which_buzzer < 0 || which_buzzer >= NUM_BUZZERS)
            return 0;
        else
            return rules->allow_rebuzz || !state->buzzed[which_buzzer];
    }
    else {
        int which_side = BUZZER_TO_SIDE(which_buzzer);
        if (which_side < 0 || which_side > 1)
            return 0;
        else
            return rules->allow_rebuzz || !state->buzzed[which_side];
    }
}

void
bg_buzz_handler(void *cookie, int which_buzzer) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;
    int which_side;

    if (which_buzzer < 0 || which_buzzer >= NUM_BUZZERS) {
        // passer-by made a convincing buzzer noise
        return;
    }

    if (!is_entitled_to_buzz(state, which_buzzer)) {
        // you already had a go and you cocked it up
        return;
    }

    which_side = BUZZER_TO_SIDE(which_buzzer);

    if (state->current_buzzer >= 0 && which_side == BUZZER_TO_SIDE(state->current_buzzer)) {
        // yes, we heard you, now leave the button alone and answer the QM
        return;
    }

    if (!boz_clock_running(state->clock) || state->current_buzzer >= 0) {
        /* Too late or too early - either way, hop it, but I might
           record the time you did this, just to laugh at you */
        if (state->clock_has_started && (state->current_buzzer >= 0 || state->time_expired)) {
            if (state->late_buzz_ms[which_side] < 0) {
                unsigned long millis_end;
                if (state->current_buzzer >= 0)
                    millis_end = state->last_buzz_at_millis;
                else
                    millis_end = state->time_expired_at_millis;

                state->late_buzz_ms[which_side] = time_elapsed(millis_end, millis());
                if (rules->two_sides && rules->show_buzz_time) {
                    update_late_buzz(which_side != 0, state->late_buzz_ms[which_side]);
                }
            }
        }
        return;
    }

    accept_buzz(state, which_buzzer);
}


void
bg_play(void *cookie) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;
    /* QM has pressed the Play button.
       If the clock has expired, then do nothing.
       If the clock is running then pause it.
       If the clock isn't running then start it. */

    if (state->time_expired) {
        return;
    }

    if (boz_clock_running(state->clock)) {
        /* Stop the clock, redraw the display */
        boz_clock_stop(state->clock);
        boz_clock_cancel_alarm(state->clock);

        /* Any pending buzzer-unlock alarm is no longer required */
        boz_cancel_alarm();

        redraw_display(state);
    }
    else {
        /* Any pending buzzer-unlock alarm is no longer required */
        boz_cancel_alarm();

        /* QM pressed play to start or resume the clock */
        long current_clock_value = boz_clock_value(state->clock);

        /* There is no current buzzing player, unlock the other buzzers */
        state->current_buzzer = -1;

        /* Remove "late buzz" notifications */
        for (int i = 0; i < NUM_BUZZERS; ++i)
            state->late_buzz_ms[i] = -1;

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
                if (is_entitled_to_buzz(state, buzzer) && boz_is_button_pressed(FUNC_BUZZER, buzzer, &pressed_since_micros)) {
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

        if (earliest_buzzer < 0 || boz_clock_running(state->clock)) {
            /* The usual case: the clock has restarted and no buttons were
               held down, or a button was held down but the clock is allowed
               to run while a buzzer is lit. Redraw the display and set an
               alarm to wake us up when we next need to do an update. */

            long alarm_ms;

            redraw_display(state);

            /* Set an alarm for the next .2 second boundary, which is when we'll
               next update the display */
            if (boz_clock_is_direction_forwards(state->clock))
                alarm_ms = ((current_clock_value + 200) / 200) * 200;
            else
                alarm_ms = ((current_clock_value - 1) / 200) * 200;
            boz_clock_set_alarm(state->clock, alarm_ms, bg_alarm_handler);
        }
    }
}

void set_up_clock(struct buzzer_game_state *state) {
    boz_clock_stop(state->clock);
    boz_clock_cancel_expiry_max(state->clock);
    boz_clock_cancel_expiry_min(state->clock);
    if (rules->time_limit_sec) {
        boz_clock_set_direction(state->clock, rules->clock_counts_up);
        if (rules->clock_counts_up) {
            boz_clock_set_initial_value(state->clock, 0);
            boz_clock_set_expiry_max(state->clock, rules->time_limit_sec * 1000L, bg_time_expired_handler);
        }
        else {
            boz_clock_set_initial_value(state->clock, rules->time_limit_sec * 1000L);
            boz_clock_set_expiry_min(state->clock, 0, bg_time_expired_handler);
        }
    }
    else {
        /* If there is no time limit, the clock always counts up */
        boz_clock_set_initial_value(state->clock, 0);
        boz_clock_set_direction(state->clock, 1);
    }
    boz_clock_reset(state->clock);
}

void
bg_options_return_callback(void *cookie, int rc) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;
    struct option_menu_context *omc = state->bg_options_context;
    if (rc == 0) {
        if (omc == NULL) {
            /* bg_options_context was never allocated before we called the
               options menu app? */
            redraw_display(state);
            return;
        }
        long *bg_options_return = omc->results;

        rules->time_limit_sec = (int) bg_options_return[BG_OPTIONS_INDEX_TIME_LIMIT];
        rules->clock_counts_up = (unsigned int) bg_options_return[BG_OPTIONS_INDEX_CLOCK_COUNTS_UP];

        if (state->time_expired || !state->clock_has_started)
            set_up_clock(state);

        rules->warn_remaining_sec = (int) bg_options_return[BG_OPTIONS_INDEX_WARN_TIME];
        rules->buzz_stops_clock = (unsigned int) bg_options_return[BG_OPTIONS_INDEX_BUZZ_STOPS_CLOCK];
        rules->buzz_length_tenths = (int) (bg_options_return[BG_OPTIONS_INDEX_BUZZ_LENGTH] / 100);
        rules->buzzer_noise = (char) bg_options_return[BG_OPTIONS_INDEX_BUZZ_NOISE];
        rules->time_up_noise = (char) bg_options_return[BG_OPTIONS_INDEX_TIME_UP_NOISE];
        rules->allow_rebuzz = (unsigned int) bg_options_return[BG_OPTIONS_INDEX_ALLOW_REBUZZ];

        rules->lockout_time_ms = (int) (bg_options_return[BG_OPTIONS_INDEX_LOCKOUT_TIME]);

        rules->first_c2_buzzer = (char) (bg_options_return[BG_OPTIONS_INDEX_WHICH_BUZZERS] + 1);
        rules->two_sides = (rules->first_c2_buzzer < 4);
    }

    /* Free the array of options context structure we created for the options
       menu app. */
    if (omc != NULL) {
        boz_mm_free(omc->results);
        boz_mm_free(omc);
        state->bg_options_context = NULL;
    }

    redraw_display(state);
}

void
bg_rotary_press(void *cookie) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;
    long *bg_options_return;
    struct option_menu_context *omc;

    if (boz_clock_running(state->clock)) {
        return;
    }

    omc = (struct option_menu_context *) boz_mm_alloc(sizeof(struct option_menu_context));
    if (omc == NULL)
        goto fail;
    state->bg_options_context = omc;

    omc->results = (long *) boz_mm_alloc(sizeof(long) * BG_OPTIONS_LENGTH);
    if (omc->results == NULL)
        goto fail;

    omc->pages = buzzer_game_options;
    omc->one_shot = 0;

    bg_options_return = omc->results;

    bg_options_return[BG_OPTIONS_INDEX_TIME_LIMIT] = rules->time_limit_sec;
    bg_options_return[BG_OPTIONS_INDEX_CLOCK_COUNTS_UP] = rules->clock_counts_up;
    bg_options_return[BG_OPTIONS_INDEX_WARN_TIME] = rules->warn_remaining_sec;
    bg_options_return[BG_OPTIONS_INDEX_BUZZ_STOPS_CLOCK] = rules->buzz_stops_clock;
    bg_options_return[BG_OPTIONS_INDEX_BUZZ_LENGTH] = rules->buzz_length_tenths * 100;
    bg_options_return[BG_OPTIONS_INDEX_BUZZ_NOISE] = rules->buzzer_noise;
    bg_options_return[BG_OPTIONS_INDEX_TIME_UP_NOISE] = rules->time_up_noise;
    bg_options_return[BG_OPTIONS_INDEX_ALLOW_REBUZZ] = rules->allow_rebuzz;
    bg_options_return[BG_OPTIONS_INDEX_LOCKOUT_TIME] = (long) rules->lockout_time_ms;
    if (!rules->two_sides)
        bg_options_return[BG_OPTIONS_INDEX_WHICH_BUZZERS] = 3;
    else
        bg_options_return[BG_OPTIONS_INDEX_WHICH_BUZZERS] = rules->first_c2_buzzer - 1;

    omc->page_disable_mask = rules->opt_page_disable_mask;

    omc->num_pages = BG_OPTIONS_LENGTH;

    if (boz_app_call(BOZ_APP_ID_OPTION_MENU, omc, bg_options_return_callback, state)) {
        goto fail;
    }
    return;

fail:
    if (state->bg_options_context != NULL) {
        boz_mm_free(state->bg_options_context->results);
        boz_mm_free(state->bg_options_context);
        state->bg_options_context = NULL;
    }
}

void
bg_generate_target(void *cookie) {
    struct buzzer_game_state *state = (struct buzzer_game_state *) cookie;

    if (!prng_seeded) {
        /* Current time in microseconds plus 1009 times the analog value on an
           unconnected pin gives us a (somewhat) random initial seed. */
        pinMode(A7, INPUT);
        randomSeed(micros() + 1009L * (long) analogRead(A7));
        prng_seeded = 1;
    }

    /* Generate a pseudorandom target between 101 and 999 inclusive. */
    state->generated_target = (int) random(101, 1000);

    boz_display_set_cursor(0, 0);
    boz_display_write_long(state->generated_target, 3, NULL);
}

void
buzzer_game_general_init(const struct game_rules *rules_progmem) {
    rules = (struct game_rules *) boz_mm_alloc(sizeof(*rules));
    bg_state = (struct buzzer_game_state *) boz_mm_alloc(sizeof(*bg_state));

    memcpy_P(rules, rules_progmem, sizeof(*rules));

    bg_state->clock = boz_clock_create(0, 1);

    bg_reset_state(bg_state);

    boz_set_event_handler_buzz(bg_buzz_handler);
    boz_set_event_handler_qm_play(bg_play);
    boz_set_event_handler_qm_reset(bg_reset);
    if (rules->yellow_generates_target)
        boz_set_event_handler_qm_yellow(bg_generate_target);
    else
        boz_set_event_handler_qm_yellow(bg_unlock_buzzers);
    boz_set_event_handler_qm_rotary_press(bg_rotary_press);
    boz_set_event_cookie(bg_state);

    boz_clock_set_event_cookie(bg_state->clock, bg_state);
}

void
conundrum_init(void *dummy) {
    buzzer_game_general_init(&conundrum_rules);
}

void
buzzer_game_init(void *dummy) {
    buzzer_game_general_init(&basic_buzzer_rules);
}
