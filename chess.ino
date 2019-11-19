#include "boz_api.h"
#include "options.h"
#include <avr/pgmspace.h>

/* No increment. */
#define CHESS_INC_MODE_NONE 0

/* At the start of a player's move, increment_ms is added on to their clock. */
#define CHESS_INC_MODE_INC 1

/* At the start of a player's move, the delay clock is started, counting down
   from increment_ms to 0. When the delay clock expires, the player's main
   clock starts. If the player finishes their move before the delay clock runs
   out, the player's main clock is unchanged. */
#define CHESS_INC_MODE_SIMPLE_DELAY 2

/* At the start of a player's move, the delay clock is started, counting up
   from 0 to increment_ms. At the end of the player's move, the length of time
   the player took to move, or increment_ms (whichever is smaller) is added on
   to the player's clock.
   This is functionally equivalent to CHESS_INC_MODE_SIMPLE_DELAY. */
#define CHESS_INC_MODE_BRONSTEIN_DELAY 3

struct chess_clock_rules {
    long initial_time_ms;
    unsigned int increment_ms;
    unsigned int increment_mode : 2;
    unsigned int allow_negative : 1;
};

const PROGMEM char s_time_control[] = "Time control? " BOZ_CHAR_WHEEL_AC_S BOZ_CHAR_WHEEL_C_S;

/* Some popular chess time control formats gleaned from bare minimum research */
const PROGMEM char s_cr_lightning[] = "Lightning 2m";
const PROGMEM char s_cr_blitz[] = "Blitz 5m";
const PROGMEM char s_cr_scrabble[] = "Scrabble 25m";
const PROGMEM char s_cr_3m2s[] = "3m +2s inc";
const PROGMEM char s_cr_5m10s[] = "5m +10s inc";
const PROGMEM char s_cr_15m30s[] = "15m +30s inc";
const PROGMEM char s_cr_30m[] = "30m";
const PROGMEM char s_cr_30m30s[] = "30m +30s inc";
const PROGMEM char s_cr_90m[] = "90m";
const PROGMEM char s_cr_90m30s[] = "90m +30s inc";
const PROGMEM char s_cr_120m[] = "120m";
const PROGMEM char s_cr_120m30s[] = "120m +30s inc";
const PROGMEM char s_cr_bron_blitz[] = "5m +3s Bron";
const PROGMEM char s_cr_bron_rapid[] = "25m +10s Bron";
const PROGMEM char s_cr_bron_classic[] = "115m +5s Bron";
const PROGMEM char s_cr_delay_blitz[] = "5m +3s delay";
const PROGMEM char s_cr_delay_rapid[] = "25m +10s delay";
const PROGMEM char s_cr_delay_classic[] = "115m +5s delay";


const PROGMEM char * const rules_names[] = {
    s_cr_lightning,
    s_cr_blitz,
    s_cr_scrabble,
    s_cr_3m2s,
    s_cr_5m10s,
    s_cr_15m30s,
    s_cr_30m,
    s_cr_30m30s,
    s_cr_90m,
    s_cr_90m30s,
    s_cr_120m,
    s_cr_120m30s,
    s_cr_bron_blitz,
    s_cr_bron_rapid,
    s_cr_bron_classic,
    s_cr_delay_blitz,
    s_cr_delay_rapid,
    s_cr_delay_classic,
};

const PROGMEM struct chess_clock_rules rules_list[] = {
    /* initial time (ms), increment (ms), increment mode, allow negative */
    { 2L *   60000L, 0, 0, 0 },
    { 5L *   60000L, 0, 0, 0 },
    { 25L *  60000L, 0, 0, 1 },
    { 3L *   60000L, 2000, CHESS_INC_MODE_INC, 0 },
    { 5L *   60000L, 10000, CHESS_INC_MODE_INC, 0 },
    { 15L *  60000L, 30000, CHESS_INC_MODE_INC, 0 },
    { 30L *  60000L, 0, 0, 0 },
    { 30L *  60000L, 30000, CHESS_INC_MODE_INC, 0 },
    { 90L *  60000L, 0, 0, 0 },
    { 90L *  60000L, 30000, CHESS_INC_MODE_INC, 0 },
    { 120L * 60000L, 0, 0, 0 },
    { 120L * 60000L, 30000, CHESS_INC_MODE_INC, 0 },
    { 5L *   60000L, 3000, CHESS_INC_MODE_BRONSTEIN_DELAY, 0 },
    { 25L *  60000L, 10000, CHESS_INC_MODE_BRONSTEIN_DELAY, 0 },
    { 115L * 60000L, 5000, CHESS_INC_MODE_BRONSTEIN_DELAY, 0 },
    { 5L *   60000L, 3000, CHESS_INC_MODE_SIMPLE_DELAY, 0 },
    { 25L *  60000L, 10000, CHESS_INC_MODE_SIMPLE_DELAY, 0 },
    { 115L * 60000L, 5000, CHESS_INC_MODE_SIMPLE_DELAY, 0 },
};
#define RULES_LIST_LENGTH (sizeof(rules_list) / sizeof(rules_list[0]))

const PROGMEM struct option_page preset_menu[] = {
    {
        s_time_control,
        OPTION_TYPE_LIST,
        0,
        0,
        1,
        NULL,
        rules_names,
        RULES_LIST_LENGTH
    }
};
//long preset_picked = 0;

/*struct option_menu_context preset_menu_context = {
    preset_menu,
    1,
    &preset_picked,
    1
};*/

const PROGMEM char s_inc_none[] = "None";
const PROGMEM char s_inc_standard[] = "Increment";
const PROGMEM char s_inc_delay[] = "Simple delay";
const PROGMEM char s_inc_bronstein[] = "Bronstein";
const PROGMEM char * const s_increment_mode_list[] = {
    s_inc_none,
    s_inc_standard,
    s_inc_delay,
    s_inc_bronstein
};
const PROGMEM char s_initial_clock_time[] = "Initial time";
//const PROGMEM char s_right_clock[] = "Right clock time";
const PROGMEM char s_increment_mode[] = "Increment mode";
const PROGMEM char s_increment_time[] = "Increment time";
const PROGMEM char s_allow_negative[] = "Allow negative";

const PROGMEM struct option_page clock_settings_menu[] = {
    {
        s_initial_clock_time,
        OPTION_TYPE_CLOCK_HR_MIN_SEC,
        0,
        9L * 3600L + 59L * 60L + 59L,
        1,
        NULL,
        NULL,
        0,
    },
/*    {
        s_right_clock,
        OPTION_TYPE_CLOCK_HR_MIN_SEC,
        0,
        9L * 3600L + 59L * 60L + 59L,
        1,
        NULL,
        NULL,
        0,
    },*/
    {
        s_increment_mode,
        OPTION_TYPE_LIST,
        0,
        0,
        0,
        NULL,
        s_increment_mode_list,
        4,
    },
    {
        s_increment_time,
        OPTION_TYPE_CLOCK_SEC,
        0,
        60,
        1,
        NULL,
        NULL,
        0,
    },
    {
        s_allow_negative,
        OPTION_TYPE_YES_NO,
        0,
        0,
        0,
        NULL,
        NULL,
        0
    }
};

//long clock_settings_results[4];
#define CLOCK_SETTINGS_INITIAL_TIME 0
#define CLOCK_SETTINGS_INCREMENT_MODE 1
#define CLOCK_SETTINGS_INCREMENT_TIME 2
#define CLOCK_SETTINGS_ALLOW_NEGATIVE 3
#define CLOCK_SETTINGS_LENGTH 4

/*struct option_menu_context clock_settings_menu_context = {
    clock_settings_menu,
    sizeof(clock_settings_menu) / sizeof(clock_settings_menu[0]),
    clock_settings_results,
    0
};*/


struct chess_clock_format {
    long abs_value_min;
    unsigned int leading_spaces : 4;
    unsigned int trailing_spaces : 4;
    unsigned int show_sign : 1;
    unsigned int show_hours : 1;
    unsigned int show_hour_colon : 1;
    unsigned int show_minutes : 1;
    unsigned int minutes_zero_pad : 1;
    unsigned int minutes_field_width : 2;
    unsigned int show_minutes_colon : 1;
    unsigned int show_seconds : 1;
    unsigned int seconds_zero_pad : 1;
    unsigned int seconds_field_width : 2;
    unsigned int show_decimal_point : 1;
    unsigned int show_tenths : 1;
};

const PROGMEM struct chess_clock_format left_clock_formats[] = {
    /* value   ls ts  -  h  :  m  0 fw  :  s  0 fw dp /10  */

    /* an hour or more: show as "-H:MM:SS" */
    { 3600000L, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0 },

    /* ten minutes to one hour: show as "-MM:SS  " */
    { 600000L,  0, 2, 1, 0, 0, 1, 0, 2, 1, 1, 1, 2, 0, 0 },

    /* 9m59s to 1m00s: show as " -M:SS  " */
    { 60000L,   1, 2, 1, 0, 0, 1, 0, 1, 1, 1, 1, 2, 0, 0 },

    /* 0m59 to 0m10: show as "-SS.T   " */
    { 10000L,   0, 3, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 1 },

    /* 0m10 to 0: show as " -S.T   " */
    { 0L,       1, 3, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1 },
};

const PROGMEM struct chess_clock_format right_clock_formats[] = {
    /* value   ls ts  -  h  :  m  0 fw  :  s  0 fw dp /10  */

    /* an hour or more: show as "-H:MM:SS" */
    { 3600000L, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0 },

    /* ten minutes to one hour: show as " -MM:SS " */
    { 600000L,  1, 1, 1, 0, 0, 1, 0, 2, 1, 1, 1, 2, 0, 0 },

    /* 9m59s to 1m00s: show as "  -M:SS " */
    { 60000L,   2, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 2, 0, 0 },

    /* 0m59 to 0m10: show as   "  -SS.T " */
    { 10000L,   2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 1 },

    /* 0m10 to 0: show as      "   -S.T " */
    { 0L,       3, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1 },
};

const PROGMEM struct chess_clock_format delay_clock_formats[] = {
    /* value   ls ts  -  h  :  m  0 fw  :  s  0 fw dp /10  */

    /* An hour or more: show as H:MM */
    { 3600000L, 0, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0 },

    /* 10 minutes to one hour: show as MM */
    { 600000L,  2, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0 },

    /* 1m00 to 9m59: show as M:SS */
    { 60000L,   0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 0, 0 },

    /* 0 to 0m59: show as SS.T */
    { 0L,       0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 1, 1 },
};

#define DELAY_ROW 1
#define DELAY_LEFT_COL 2
#define DELAY_RIGHT_COL 10
#define FLAG_ROW 1
#define FLAG_LEFT_COL 1
#define FLAG_RIGHT_COL 14
#define TURN_ROW 1
#define TURN_LEFT_COL 0
#define TURN_RIGHT_COL 15
#define FLAG_CHAR 0
#define TURN_CHAR 7

/* Crude drawing of a flag */
const PROGMEM byte flag_char_pattern[] = {
    0x1c, 0x17, 0x15, 0x1b, 0x17, 0x10, 0x10, 0x10
//    0x1c, 0x17, 0x11, 0x1d, 0x17, 0x10, 0x10, 0x10
};

/* Crude drawing of an hourglass */
const PROGMEM byte turn_char_pattern[] = {
    0x1f, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x1f, 0x00
};


struct chess_state {
    boz_clock clocks[2];
    int num_moves[2];
    byte delay_expired[2];
    boz_clock delay_clock;

    /* flags[x] if player x's flag has fallen */
    byte flags[2];

    /* -1 if clocks are stopped, 0 if it's left player, 1 if right player */
    char whose_turn;

    /* If the clocks were paused, whose turn was it before that happened */
    char whose_turn_before_stopped;

    char clocks_have_started;

    struct chess_clock_rules rules;

    struct option_menu_context *clock_settings_menu_context;
    struct option_menu_context *preset_menu_context;
};

struct chess_state *chess_state;

void
chess_init_callback(void *cookie, int rc) {
    int preset_picked;
    struct option_menu_context *omc = chess_state->preset_menu_context;

    if (rc != 0) {
        boz_app_exit(1);
        return;
    }

    if (omc == NULL) {
        preset_picked = 0;
    }
    else {
        preset_picked = (int) omc->results[0];
        boz_mm_free(omc->results);
        boz_mm_free(omc);
        chess_state->preset_menu_context = NULL;
    }

    /* Load the selected time control, whose index is now in preset_picked */
    memcpy_P(&chess_state->rules, &rules_list[preset_picked], sizeof(chess_state->rules));

    /* Program display CGRAM with our whose-turn-it-is and flag characters */
    boz_display_set_cgram_address(TURN_CHAR << 3);
    for (int i = 0; i < 8; ++i) {
        boz_display_write_char(pgm_read_byte_near(turn_char_pattern + i));
    }
    boz_display_set_cgram_address(FLAG_CHAR << 3);
    for (int i = 0; i < 8; ++i) {
        boz_display_write_char(pgm_read_byte_near(flag_char_pattern + i));
    }

    chess_game_reset(chess_state);

    boz_set_event_cookie(chess_state);
    boz_set_event_handler_qm_play(chess_play);
    boz_set_event_handler_qm_reset(chess_reset);
    boz_set_event_handler_buzz(chess_buzzer);
    boz_set_event_handler_qm_rotary_press(chess_rotary_press);
}

static void start_preset_menu() {
    struct option_menu_context *omc = (struct option_menu_context *) boz_mm_alloc(sizeof(struct option_menu_context));
    chess_state->preset_menu_context = omc;
    omc->pages = preset_menu;
    omc->num_pages = 1;
    omc->results = (long *) boz_mm_alloc(sizeof(long));
    omc->results[0] = 0;
    omc->one_shot = 1;
    omc->page_disable_mask = 0;
    boz_app_call(BOZ_APP_ID_OPTION_MENU, omc, chess_init_callback, NULL);
}

void
chess_init(void *dummy) {
    /* Before we do anything, call out to the options app, and get the user
       to pick a preset time control. */
    chess_state = (struct chess_state *) boz_mm_alloc(sizeof(*chess_state));
    memset(chess_state, 0, sizeof(*chess_state));
    chess_state->clocks[0] = NULL;
    chess_state->clocks[1] = NULL;
    chess_state->delay_clock = NULL;

    start_preset_menu();
}

static void print_clock_value(long value_ms, const struct chess_clock_format *format_array) {
    struct chess_clock_format format;
    long abs_value_ms = value_ms;
    if (abs_value_ms < 0)
        abs_value_ms = -abs_value_ms;

    /* Find the first element of format_array suitable for this value. The
       last element in the array must have an abs_value_min of zero! */
    do {
        memcpy_P(&format, format_array, sizeof(format));
        ++format_array;
    } while (abs_value_ms < format.abs_value_min);

    for (int i = 0; i < format.leading_spaces; ++i) {
        boz_display_write_char(' ');
    }
    if (format.show_sign) {
        boz_display_write_char((value_ms < 0) ? '-' : ' ');
    }

    value_ms = abs_value_ms;

    if (format.show_hours) {
        boz_display_write_long(value_ms / 3600000L, 1, NULL);
    }
    if (format.show_hour_colon) {
        boz_display_write_char(':');
    }
    if (format.show_minutes) {
        boz_display_write_long((value_ms / 60000L) % 60, format.minutes_field_width, format.minutes_zero_pad ? "0" : NULL);
    }
    if (format.show_minutes_colon) {
        boz_display_write_char(':');
    }
    if (format.show_seconds) {
        boz_display_write_long((value_ms / 1000L) % 60, format.seconds_field_width, format.seconds_zero_pad ? "0" : NULL);
    }
    if (format.show_decimal_point) {
        boz_display_write_char('.');
    }
    if (format.show_tenths) {
        boz_display_write_long((value_ms / 100L) % 10, 1, "0");
    }

    for (int i = 0; i < format.trailing_spaces; ++i) {
        boz_display_write_char(' ');
    }
}

static void redraw_clock(struct chess_state *state, int which_clock) {
    long value_ms = boz_clock_value(state->clocks[which_clock]);
    const struct chess_clock_format *formatp;
    long abs_value_ms = value_ms;

    if (abs_value_ms < 0)
        abs_value_ms = -abs_value_ms;

    if (which_clock)
        formatp = right_clock_formats;
    else
        formatp = left_clock_formats;

    boz_display_set_cursor(0, which_clock ? 8 : 0);
    print_clock_value(value_ms, formatp);

    attach_clock_update_alarm(state->clocks[which_clock], which_clock);
}

static void redraw_delay_clock(void *cookie, boz_clock clock) {
    struct chess_state *state = (struct chess_state *) cookie;
    long value_ms = boz_clock_value(clock);
    char turn = state->whose_turn;
    if (turn < 0)
        turn = state->whose_turn_before_stopped;

    boz_display_set_cursor(DELAY_ROW, turn == 0 ? DELAY_LEFT_COL : DELAY_RIGHT_COL);
    print_clock_value(value_ms, delay_clock_formats);

    attach_delay_update_alarm(clock);
}

static void clock_update_alarm(void *cookie, boz_clock clock) {
    struct chess_state *state = (struct chess_state *) cookie;
    int which_player;

    if (clock == state->clocks[0]) {
        which_player = 0;
    }
    else {
        which_player = 1;
    }
    redraw_clock(state, which_player);

    if (boz_clock_value(clock) <= 0 && state->flags[which_player] == 0) {
        flag_fall(state, which_player);
    }
}

static void attach_clock_update_alarm(boz_clock clock, int which_player) {
    static const int step_ms = 200;
    long value_ms = boz_clock_value(clock);
    long alarm_ms;
    int mod;

    if (value_ms > 0) {
        mod = value_ms % step_ms;
        if (mod == 0)
            mod = step_ms;
    }
    else {
        mod = -value_ms % step_ms;
        mod = step_ms - mod;
    }

    /* Set the alarm for when the clock's value is e.g. 123.010 seconds, not
       123.000, so that if it takes more than 1ms to wake up and be called, we
       don't end up displaying 122 seconds. If the alarm time is exactly on
       a multiple of 200ms, it can lead to the seconds appearing to change
       irregularly, e.g.:
       2:03 (wait 1000ms) 2:02 (wait 800ms) 2:01 (wait 1200ms) 2:00
    */
    alarm_ms = value_ms - mod + 10;

    /* If the alarm time is greater than (i.e. earlier than) the clock value
       (this can happen if at 123.005 seconds we set the alarm, which will end
       up being at 123.010 seconds) then we really want the alarm at the next
       200ms + 10 boundary, so 122.810 seconds. */
    if (alarm_ms >= value_ms)
        alarm_ms -= step_ms;

    boz_clock_set_alarm(clock, alarm_ms, clock_update_alarm);
}

static void attach_delay_update_alarm(boz_clock clock) {
    static const int step_ms = 200;
    long value_ms = boz_clock_value(clock);
    long alarm_ms;
    int mod = value_ms % step_ms;

    if (boz_clock_is_direction_forwards(clock)) {
        mod = step_ms - mod;
        alarm_ms = value_ms + mod;
    }
    else {
        if (mod == 0)
            mod = step_ms;
        alarm_ms = value_ms - mod + 10;
        if (alarm_ms >= value_ms)
            alarm_ms -= step_ms;
    }

    if (!(alarm_ms == 0 && value_ms == 0))
        boz_clock_set_alarm(clock, alarm_ms, redraw_delay_clock);
}

static void flag_fall(struct chess_state *state, int which_player) {
    if (which_player == 0) {
        state->flags[0] = 1;
        boz_sound_note(NOTE_C4, 800);
        boz_display_set_cursor(FLAG_ROW, FLAG_LEFT_COL);
    }
    else {
        state->flags[1] = 1;
        boz_sound_note(NOTE_G4, 800);
        boz_display_set_cursor(FLAG_ROW, FLAG_RIGHT_COL);
    }
    boz_display_write_char(FLAG_CHAR);
}

static void chess_clock_expired(void *cookie, boz_clock clock) {
    struct chess_state *state = (struct chess_state *) cookie;
    for (int p = 0; p < 2; ++p) {
        if (clock == state->clocks[p] && !state->flags[p]) {
            flag_fall(state, p);
        }
    }

    chess_redraw(state);
}

void chess_redraw(struct chess_state *state) {
    char turn;

    boz_display_clear();

    for (int i = 0; i < 2; ++i)
        redraw_clock(state, i);

    turn = state->whose_turn;
    if (turn < 0)
        turn = state->whose_turn_before_stopped;

    if (state->delay_clock && turn >= 0) {
        if (!state->delay_expired[(int) turn]) {
            redraw_delay_clock(state, state->delay_clock);
        }
    }

    if (state->flags[0]) {
        boz_display_set_cursor(FLAG_ROW, FLAG_LEFT_COL);
        boz_display_write_char(FLAG_CHAR);
    }
    if (state->flags[1]) {
        boz_display_set_cursor(FLAG_ROW, FLAG_RIGHT_COL);
        boz_display_write_char(FLAG_CHAR);
    }

    if (turn < 0) {
        boz_leds_set(0);
    }
    else if (turn == 0) {
        boz_leds_set(1);
        boz_display_set_cursor(TURN_ROW, TURN_LEFT_COL);
        boz_display_write_char(TURN_CHAR);
    }
    else {
        boz_leds_set(8);
        boz_display_set_cursor(TURN_ROW, TURN_RIGHT_COL);
        boz_display_write_char(TURN_CHAR);
    }

    if (state->whose_turn < 0 && turn >= 0) {
        /* Game paused:  show play character */
        boz_display_set_cursor(1, 8);
        boz_display_write_char(BOZ_CHAR_PLAY);
    }
}

void
chess_game_reset(struct chess_state *state) {
    for (int i = 0; i < 2; ++i) {
        /* Get some new clocks */
        if (state->clocks[i] != NULL)
            boz_clock_release(state->clocks[i]);
        state->clocks[i] = boz_clock_create(state->rules.initial_time_ms, 0);

        boz_clock_set_event_cookie(state->clocks[i], state);

        /* If the clock is not allowed to go negative, set 0 as its minimum
           value which makes the flag fall. */
        if (!state->rules.allow_negative)
            boz_clock_set_expiry_min(state->clocks[i], 0, chess_clock_expired);

        /* Alarm which updates the display a few times a second */
        attach_clock_update_alarm(state->clocks[i], i);
        state->flags[i] = 0;
        state->delay_expired[i] = 0;
        state->num_moves[i] = 0;
    }
    state->whose_turn = -1;
    state->whose_turn_before_stopped = -1;
    state->clocks_have_started = 0;

    if (state->delay_clock != NULL)
        boz_clock_release(state->delay_clock);

    state->delay_clock = NULL;

    switch (state->rules.increment_mode) {
        case CHESS_INC_MODE_SIMPLE_DELAY:
            /* Delay clock counts down to when we start the player's main
               clock */
            state->delay_clock = boz_clock_create(state->rules.increment_ms, 0);
            boz_clock_set_event_cookie(state->delay_clock, state);
            attach_delay_update_alarm(state->delay_clock);
            boz_clock_set_expiry_min(state->delay_clock, 0, chess_delay_expired);
            break;
        case CHESS_INC_MODE_BRONSTEIN_DELAY:
            /* delay clock counts the time the player took for their move,
               then after their move we add that value, or the increment,
               onto the player's main time, whichever is smaller */
            state->delay_clock = boz_clock_create(0, 1);
            boz_clock_set_event_cookie(state->delay_clock, state);
            boz_clock_set_expiry_max(state->delay_clock, state->rules.increment_ms, chess_delay_expired);
            break;
    }

    chess_redraw(state);
}

static void chess_delay_expired(void *cookie, boz_clock clock) {
    struct chess_state *state = (struct chess_state *) cookie;
    int player = state->whose_turn;
    if (player >= 0) {
        boz_clock_run(state->clocks[player]);
        state->delay_expired[player] = 1;
    }
    chess_redraw(state);
}

static void start_player_move(struct chess_state *state, int player) {
    switch (state->rules.increment_mode) {
        case CHESS_INC_MODE_INC:
            boz_clock_add(state->clocks[player], state->rules.increment_ms);
            attach_clock_update_alarm(state->clocks[player], player);
            state->delay_expired[player] = 1;
            break;

        case CHESS_INC_MODE_SIMPLE_DELAY:
        case CHESS_INC_MODE_BRONSTEIN_DELAY:
            boz_clock_stop(state->delay_clock);
            boz_clock_reset(state->delay_clock);
            if (state->rules.increment_mode == CHESS_INC_MODE_SIMPLE_DELAY)
                boz_clock_set_expiry_min(state->delay_clock, 0, chess_delay_expired);
            else
                boz_clock_set_expiry_max(state->delay_clock, state->rules.increment_ms, chess_delay_expired);
            state->delay_expired[player] = 0;
            break;

        default:
            state->delay_expired[player] = 1;
    }
    state->whose_turn = player;
    state->num_moves[player]++;
    start_player_clock(state, player);
}

static void start_player_clock(struct chess_state *state, int player) {
    if (state->delay_expired[player] || state->rules.increment_mode == CHESS_INC_MODE_BRONSTEIN_DELAY) {
        boz_clock_run(state->clocks[player]);
    }

    if (!state->delay_expired[player]) {
        boz_clock_run(state->delay_clock);
    }
    state->clocks_have_started = 1;
}

static void stop_player_clock(struct chess_state *state, int player) {
    boz_clock_stop(state->clocks[player]);
    if (state->delay_clock)
        boz_clock_stop(state->delay_clock);
}

static void end_player_move(struct chess_state *state, int player) {
    /* Only end a move if there's a move to end */
    if (state->num_moves[player] > 0) {
        long ms_to_add = 0;
        switch (state->rules.increment_mode) {
            case CHESS_INC_MODE_BRONSTEIN_DELAY:
                ms_to_add = boz_clock_value(state->delay_clock);
                if (ms_to_add > state->rules.increment_ms) {
                    ms_to_add = state->rules.increment_ms;
                }
                break;
        }
        if (ms_to_add > 0) {
            boz_clock_add(state->clocks[player], ms_to_add);
            attach_clock_update_alarm(state->clocks[player], player);
        }
        stop_player_clock(state, player);
    }
}

void
chess_play(void *cookie) {
    struct chess_state *state = (struct chess_state *) cookie;

    if (state->whose_turn >= 0) {
        state->whose_turn_before_stopped = state->whose_turn;
        state->whose_turn = -1;
        stop_player_clock(state, state->whose_turn_before_stopped);
    }
    else if (state->whose_turn_before_stopped >= 0) {
        state->whose_turn = state->whose_turn_before_stopped;
        start_player_clock(state, state->whose_turn);
    }
    chess_redraw(state);
}

void
chess_reset(void *cookie) {
    struct chess_state *state = (struct chess_state *) cookie;
    if (!state->clocks_have_started) {
        /* Go back into the preset menu, and then to chess_init_callback */
        start_preset_menu();
    }
    else {
        /* Reset the clock to its initial state */
        chess_game_reset(state);
    }
}

void
chess_buzzer(void *cookie, int which_buzzer) {
    struct chess_state *state = (struct chess_state *) cookie;
    int which_player = (which_buzzer == 0) ? 0 : 1;

    if (state->whose_turn >= 0 && state->whose_turn != which_player)
        return;
    if (state->whose_turn < 0 && state->whose_turn_before_stopped >= 0 && state->whose_turn_before_stopped != which_player)
        return;

    end_player_move(state, which_player);
    start_player_move(state, !which_player);
    chess_redraw(state);
}

void
chess_rotary_press(void *cookie) {
    struct chess_state *state = (struct chess_state *) cookie;
    if (state->whose_turn < 0 && !state->clocks_have_started) {
        /* Clocks are stopped and nothing has started yet - open clock settings
           menu */
        long *clock_settings_results;
        struct option_menu_context *omc = (struct option_menu_context *) boz_mm_alloc(sizeof(struct option_menu_context));

        state->clock_settings_menu_context = omc;
        omc->pages = clock_settings_menu;
        omc->num_pages = CLOCK_SETTINGS_LENGTH;
        omc->results = (long *) boz_mm_alloc(sizeof(long) * CLOCK_SETTINGS_LENGTH);
        omc->one_shot = 0;
        omc->page_disable_mask = 0;
        clock_settings_results = omc->results;

        clock_settings_results[CLOCK_SETTINGS_INITIAL_TIME] = state->rules.initial_time_ms / 1000;
        clock_settings_results[CLOCK_SETTINGS_INCREMENT_MODE] = state->rules.increment_mode;
        clock_settings_results[CLOCK_SETTINGS_INCREMENT_TIME] = state->rules.increment_ms / 1000;
        clock_settings_results[CLOCK_SETTINGS_ALLOW_NEGATIVE] = state->rules.allow_negative;
        boz_app_call(BOZ_APP_ID_OPTION_MENU, omc, chess_settings_callback, state);
    }
}

void
chess_settings_callback(void *cookie, int rc) {
    struct chess_state *state = (struct chess_state *) cookie;
    struct option_menu_context *omc = state->clock_settings_menu_context;

    if (rc == 0 && state->clock_settings_menu_context != NULL) {
        long *clock_settings_results = omc->results;

        state->rules.initial_time_ms = clock_settings_results[CLOCK_SETTINGS_INITIAL_TIME] * 1000L;
        state->rules.increment_mode = clock_settings_results[CLOCK_SETTINGS_INCREMENT_MODE];
        state->rules.increment_ms = clock_settings_results[CLOCK_SETTINGS_INCREMENT_TIME] * 1000;
        state->rules.allow_negative = (char) clock_settings_results[CLOCK_SETTINGS_ALLOW_NEGATIVE];

        chess_game_reset(state);
    }
    else {
        chess_redraw(state);
    }

    if (omc != NULL) {
        boz_mm_free(omc->results);
        boz_mm_free(omc);
        state->clock_settings_menu_context = NULL;
    }
}
