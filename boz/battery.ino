#include "boz_api.h"

#include <avr/pgmspace.h>

/* If the voltage at VIN is below this, we'll assume we're running from USB
   and the battery isn't switched on. The Arduino Nano can run with 7V-12V at
   VIN. The Bozzard control unit has a 9V supply. */
#define VIN_MIN_MV 6000

const PROGMEM char s_battery_power[] =   "Power:";
const PROGMEM char s_battery_battery[] = "battery";
const PROGMEM char s_battery_usb[] =     "USB    ";
const PROGMEM char s_battery_v_space[] = "V  ";
const PROGMEM char s_battery_small_pic_4_spaces[] = "\x08    ";

struct battery_app_state {
    char battery_picture;
};

#define BATTERY_PICTURE_NONE 0
#define BATTERY_PICTURE_SMALL 1
#define BATTERY_PICTURE_BIG 2

/* Define BATTERY_PICTURE to be one of the following, in ascending order of
 * code size.
 *
 *   BATTERY_PICTURE_NONE: no picture of a battery, just a percentage.
 *   BATTERY_PICTURE_SMALL: a small picture of a battery filling one char cell.
 *   BATTERY_PICTURE_BIG: a long picture of a battery filling five char cells.
 */
#define BATTERY_PICTURE BATTERY_PICTURE_BIG


#if BATTERY_PICTURE == BATTERY_PICTURE_BIG
enum battery_char_codes {
    BATTERY_LEFT_EMPTY,
    BATTERY_MIDDLE_EMPTY,
    BATTERY_RIGHT_EMPTY,
    BATTERY_LEFT_FULL,
    BATTERY_MIDDLE_FULL,
    BATTERY_RIGHT_FULL
};

/* All these characters are vertically symmetric (symmetric about a horizontal
   line) so we only need to encode rows 0, 1, 2 and 3. */
const PROGMEM byte battery_cgram_patterns[] = {
    /* Left end of battery, empty */
    0x1f, 0x10, 0x10, 0x10,

    /* Middle part of battery, empty */
    0x1f, 0, 0, 0,

    /* Right end of battery, empty */
    0x1e, 0x02, 0x03, 0x01,

    /* Left end of battery, full */
    0x1f, 0x10, 0x17, 0x17,
   
    /* Middle part of battery, full */
    0x1f, 0x00, 0x1f, 0x1f,

    /* Right end of battery, full */
    0x1e, 0x02, 0x1b, 0x19,

    /* No more patterns */
    0xff
};
#else

/* This is an empty battery, but we'll edit the pattern to make as many lines
   filled in as we need. */
#define BATTERY_ROW_SMALL_TOP0 0x0a
#define BATTERY_ROW_SMALL_TOP1 0x1f
#define BATTERY_ROW_SMALL_MIDDLE 0x11
#define BATTERY_ROW_SMALL_BOTTOM 0x1f
#define BATTERY_ROW_SMALL_MIDDLE_FULL 0x1f
#endif

void
battery_init(void *dummy) {
    boz_display_clear();
#if BATTERY_PICTURE == BATTERY_PICTURE_BIG
    battery_set_cgram_pattern((void *) battery_cgram_patterns);
#else
    battery_start(NULL);
#endif
}

#if BATTERY_PICTURE == BATTERY_PICTURE_BIG
void
battery_set_cgram_pattern(void *ptr) {
    if (pgm_read_byte_near(ptr) == 0xff) {
        /* Finished messing about with CGRAM, start the application proper */
        battery_start(NULL);
    }
    else {
        /* Set this CGRAM char, then wait for that to be done before moving
           on to the next character, so as not to overflow the display command
           queue. These commands should only take a millisecond to run, but
           we want to give the main loop a chance to actually run them. */
        int code = ((byte *) ptr - battery_cgram_patterns) / 4;
        boz_display_set_cgram_address(code << 3);
        for (byte i = 0; i < 8; ++i) {
            byte row = (i >= 4 ? (7 - i) : i);
            boz_display_write_char(pgm_read_byte_near((byte *) ptr + row));
        }
        boz_set_alarm(10, battery_set_cgram_pattern, (byte *) ptr + 4);
    }
}
#endif

void
battery_start(void *dummy) {
    /* Press green or red to exit, or press yellow to change display mode */
    boz_set_event_handler_qm_reset(battery_exit);
#if BATTERY_PICTURE != BATTERY_PICTURE_NONE
    boz_set_event_handler_qm_yellow(battery_poke);
#endif
    boz_set_event_handler_qm_play(battery_exit);

#if BATTERY_PICTURE == BATTERY_PICTURE_NONE
    battery_refresh(NULL);
#else
    struct battery_app_state *state = (struct battery_app_state *) boz_mm_alloc(sizeof(*state));
    if (state == NULL) {
        battery_exit(NULL);
    }
    else {
        state->battery_picture = 1;
        boz_set_event_cookie(state);
        battery_refresh(state);
    }
#endif
}

void
battery_exit(void *cookie) {
    boz_app_exit(0);
}

void
battery_refresh(struct battery_app_state *state) {
    boz_display_clear();

    /* Show "Power:" on the top line */
    boz_display_write_string_P(s_battery_power);

    /* Refresh the voltage display */
    battery_refresh_battery(state);
}

/* Our minimum voltage (0%) is 7 volts because anything less than that is
   outside the spec for the Arduino.
   Our maximum voltage (100%) we'll call 9.3 volts, because a brand-new 9V
   battery usually supplies about that.
   The voltages corresponding to 20%, 40%, 60%, 80% are guessed from a Duracell
   datasheet. */

const PROGMEM int voltage_per_20_percent[] = {
    //   0%  20%  40%  60%  80%  100%
        700, 730, 755, 770, 825, 930
};

static byte voltage_to_percent(int centivolts) {
    int left_threshold, right_threshold;
    byte left_threshold_percent, right_threshold_percent;
    byte quintile;
    
    left_threshold = 0;
    for (quintile = 0; quintile < 6; ++quintile) {
        right_threshold = pgm_read_word_near(&voltage_per_20_percent[(int) quintile]); 
        if (centivolts < right_threshold) {
            break;
        }
        left_threshold = right_threshold;
    }

    /* percentage is between the one represented by left_threshold and the one
       represented by right_threshold - from here we'll linearly interpolate */

    if (quintile == 0)
        return 0;
    
    if (quintile >= 6)
        return 100;
    
    right_threshold_percent = (byte) (quintile * 20);
    left_threshold_percent = right_threshold_percent - 20;
    return (byte) map(centivolts, left_threshold, right_threshold, left_threshold_percent, right_threshold_percent);
}

#if BATTERY_PICTURE != BATTERY_PICTURE_NONE
void battery_poke(void *cookie) {
    /* Change between displaying a picture of a battery and a precise but
       not terribly accurate percentage */
    struct battery_app_state *state = (struct battery_app_state *) cookie;
    state->battery_picture = !state->battery_picture;
    battery_refresh_battery(state);
}
#endif

void battery_refresh_battery(void *cookie) {
    struct battery_app_state *state = (struct battery_app_state *) cookie;
    int millivolts = boz_get_battery_voltage();
    const char *power_source;

    boz_display_set_cursor(1, 0);
    if (millivolts < VIN_MIN_MV) {
        /* We're probably not running from a battery */
        for (int i = 0; i < 16; ++i) {
            boz_display_write_char(' ');
        }
        power_source = s_battery_usb;
    }
    else {
        /* Voltage format: ##.##V */
        int centivolts = (millivolts + 5) / 10;
        byte percent;

        boz_display_write_long(centivolts / 100, 2, 0);
        boz_display_write_char('.');
        boz_display_write_long(centivolts % 100, 2, BOZ_DISP_NUM_ZERO_PAD);
        boz_display_write_string_P(s_battery_v_space);

        percent = voltage_to_percent(centivolts);

#if BATTERY_PICTURE != BATTERY_PICTURE_NONE
        if (state->battery_picture) {
#if BATTERY_PICTURE == BATTERY_PICTURE_BIG
            /* Draw a battery with the appropriate fullness, to the nearest
               20% */
            for (byte battery_section = 10; battery_section <= 90; battery_section += 20) {
                char code;
                if (percent < battery_section)
                    code = BATTERY_LEFT_EMPTY;
                else
                    code = BATTERY_LEFT_FULL;

                if (battery_section > 10) {
                    code++;
                    if (battery_section == 90) {
                        code++;
                    }
                }
                boz_display_write_char(code);
            }
#else
            /* Draw a battery in one character cell, using CGRAM code 0 or 8,
               and four spaces to remove any percentage that might have been
               there before */
            boz_display_write_string_P(s_battery_small_pic_4_spaces);

            /* Define our battery picture. It consists of eight rows. The top
               two and bottom one are constant, and the middle five rows depend
               on the battery level. */
            boz_display_set_cgram_address(0);
            boz_display_write_char(BATTERY_ROW_SMALL_TOP0);
            boz_display_write_char(BATTERY_ROW_SMALL_TOP1);
            for (char battery_section = 90; battery_section >= 10; battery_section -= 20) {
                char row;
                if (percent < battery_section)
                    row = BATTERY_ROW_SMALL_MIDDLE;
                else
                    row = BATTERY_ROW_SMALL_MIDDLE_FULL;
                boz_display_write_char(row);
            }
            boz_display_write_char(BATTERY_ROW_SMALL_BOTTOM);
#endif
        }
        else
#endif
        {
            boz_display_write_long(percent, 4, 0);
            boz_display_write_char('%');
        }

        power_source = s_battery_battery;
    }

    boz_display_set_cursor(0, 7);
    boz_display_write_string_P(power_source);

    /* Call battery_refresh_battery every two seconds until this app exits */
    boz_set_alarm(2000L, battery_refresh_battery, state);
}
