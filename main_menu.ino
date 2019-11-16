#include "boz.h"

#include "boz_app_list.h"
#include "boz_app.h"

const static int buzz_lock_timeout_ms = 3000;
const static char buzz_length_tenths_sec = 8;

struct main_menu_state {
    const struct boz_app *mm_apps;
    int mm_apps_count;
    int mm_menu_apps_count;
    int mm_app_cursor;
    int mm_first_runnable;
    int mm_last_runnable;

    int last_buzzer_id;
    int buzzers_locked;
};

static struct main_menu_state *menu_state;

const PROGMEM char s_mm_help[] = "   select  " BOZ_CHAR_PLAY_S " run";
const PROGMEM char s_mm_no_loadable_apps[] = "No runnable apps!";

void
mm_load_app(struct boz_app *dest, int index) {
    memcpy_P(dest, menu_state->mm_apps + index, sizeof(*dest));
}

void
mm_redraw_display(int include_help) {
    struct boz_app current_app;
    int i;

    if (include_help) {
        int help_start;

        boz_display_clear();

        if (menu_state->mm_first_runnable == menu_state->mm_last_runnable) {
            help_start = 9;
        }
        else {
            help_start = 2;
        }

        boz_display_set_cursor(1, help_start);
        boz_display_write_string_P(s_mm_help + help_start);
    }
    boz_display_set_cursor(0, 0);

    mm_load_app(&current_app, menu_state->mm_app_cursor);
    for (i = 0; i < 16 && current_app.name[i]; ++i) {
        boz_display_write_char(current_app.name[i]);
    }
    for (; i < 16; ++i) {
        boz_display_write_char(' ');
    }

    /* If this is not the first app in the list, write an anticlockwise
       wheel symbol next to "select". If it's not the last, write a
       clockwise one as well.
    */
    boz_display_set_cursor(1, 0);
    if (menu_state->mm_app_cursor == menu_state->mm_first_runnable) {
        boz_display_write_char(' ');
    }
    else {
        boz_display_write_char(BOZ_CHAR_WHEEL_AC);
    }
    if (menu_state->mm_app_cursor == menu_state->mm_last_runnable) {
        boz_display_write_char(' ');
    }
    else {
        boz_display_write_char(BOZ_CHAR_WHEEL_C);
    }
}

void
main_menu_init(void *dummy) {
    struct boz_app app;

    menu_state = (struct main_menu_state *) boz_mm_alloc(sizeof(struct main_menu_state));
    menu_state->mm_apps = boz_app_list_get();
    menu_state->mm_apps_count = boz_app_list_get_length();
    menu_state->mm_app_cursor = 0;
    menu_state->mm_menu_apps_count = 0;
    menu_state->mm_first_runnable = -1;
    menu_state->mm_last_runnable = -1;
    menu_state->mm_menu_apps_count = 0;
    menu_state->last_buzzer_id = -1;
    menu_state->buzzers_locked = 0;

    for (int i = 0; i < menu_state->mm_apps_count; ++i) {
        mm_load_app(&app, i);
        if (app.flags & BOZ_APP_MAIN) {
            /* This app appears in the main menu */
            if (menu_state->mm_first_runnable < 0)
                menu_state->mm_first_runnable = i;
            menu_state->mm_last_runnable = i;
            menu_state->mm_menu_apps_count++;
        }
    }

    if (menu_state->mm_menu_apps_count <= 0) {
        /* Display "no apps" message and go into infinite sleep */
        boz_display_clear();
        boz_display_write_string_P(s_mm_no_loadable_apps);
    }
    else {
        boz_set_event_cookie(NULL);
        boz_set_event_handler_qm_rotary(mm_rotary_handler);
        boz_set_event_handler_qm_rotary_press(mm_play_handler);
        boz_set_event_handler_qm_play(mm_play_handler);

        menu_state->mm_app_cursor = menu_state->mm_first_runnable;
        mm_redraw_display(1);
    }

    boz_set_event_handler_qm_yellow(mm_unlock_buzzers);
    boz_set_event_handler_buzz(mm_buzz_handler);
}

void
mm_rotary_handler(void *cookie, int clockwise) {
    struct boz_app app;
    int direction = (clockwise ? 1 : -1);
    int cursor = menu_state->mm_app_cursor;

    /* Find the next (or previous) app in the list with BOZ_APP_MAIN set, and
       if we hit either end of the list, give up */
    do {
        cursor += direction;
        if (cursor >= 0 && cursor < menu_state->mm_apps_count) {
            mm_load_app(&app, cursor);
        }
    } while (cursor >= 0 && cursor < menu_state->mm_apps_count && (app.flags & BOZ_APP_MAIN) == 0);

    if (cursor >= 0 && cursor < menu_state->mm_apps_count) {
        menu_state->mm_app_cursor = cursor;
        mm_redraw_display(0);
    }
}

void
mm_play_handler(void *cookie) {
    struct boz_app app;
    mm_load_app(&app, menu_state->mm_app_cursor);
    mm_unlock_buzzers(cookie);
    boz_app_call(app.id, NULL, mm_return_callback, NULL);
}

void
mm_return_callback(void *cookie, int rc) {
    mm_redraw_display(1);
    mm_unlock_buzzers(NULL);
}

void
mm_unlock_buzzers(void *cookie) {
    boz_cancel_alarm();
    boz_leds_set(0);
    menu_state->buzzers_locked = 0;
}

void
mm_buzz_handler(void *cookie, int which_buzzer) {
    /* Very simple buzz handling for the main menu, so the user can play with
       the buzzers even before selecting an app. There are no clocks or
       options, just simple fixed rules:
       * Pressing a buzzer makes a noise, illuminates the appropriate LED,
         and locks out all other buzzers for 3 seconds.
       * After 3 seconds, or if the yellow button is pressed, buzzers are
         enabled again and the LEDs go out.
    */

    if (!menu_state->buzzers_locked && which_buzzer >= 0 && which_buzzer <= BOZ_NUM_BUZZERS) {
        boz_sound_square_bell(which_buzzer, buzz_length_tenths_sec);
        boz_leds_set(1 << which_buzzer);
        menu_state->last_buzzer_id = which_buzzer;
        menu_state->buzzers_locked = 1;
        boz_set_alarm(buzz_lock_timeout_ms, mm_unlock_buzzers, NULL);
    }
}
