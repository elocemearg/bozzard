#include "boz.h"

#include "boz_app_list.h"
#include "boz_app.h"

const struct boz_app *mm_apps;
int mm_apps_count;
int mm_menu_apps_count;
int mm_app_cursor = 0;
int mm_first_runnable = -1;
int mm_last_runnable = -1;

const PROGMEM char s_mm_help[] = "   select  " BOZ_CHAR_PLAY_S " run";
const PROGMEM char s_mm_no_loadable_apps[] = "No runnable apps!";

void
mm_load_app(struct boz_app *dest, int index) {
    memcpy_P(dest, mm_apps + index, sizeof(*dest));
}

void
mm_redraw_display(int include_help) {
    struct boz_app current_app;
    int i;

    if (include_help) {
        int help_start;

        boz_display_clear();

        if (mm_first_runnable == mm_last_runnable) {
            help_start = 9;
        }
        else {
            help_start = 2;
        }

        boz_display_set_cursor(1, help_start);
        boz_display_write_string_P(s_mm_help + help_start);
    }
    boz_display_set_cursor(0, 0);

    mm_load_app(&current_app, mm_app_cursor);
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
    if (mm_app_cursor == mm_first_runnable) {
        boz_display_write_char(' ');
    }
    else {
        boz_display_write_char(BOZ_CHAR_WHEEL_AC);
    }
    if (mm_app_cursor == mm_last_runnable) {
        boz_display_write_char(' ');
    }
    else {
        boz_display_write_char(BOZ_CHAR_WHEEL_C);
    }
}

void
main_menu_init(void *dummy) {
    struct boz_app app;

    mm_apps = boz_app_list_get();
    mm_apps_count = boz_app_list_get_length();
    mm_app_cursor = 0;
    mm_menu_apps_count = 0;
    mm_first_runnable = -1;
    mm_last_runnable = -1;
    for (int i = 0; i < mm_apps_count; ++i) {
        mm_load_app(&app, i);
        if (app.flags & BOZ_APP_MAIN) {
            /* This app appears in the main menu */
            if (mm_first_runnable < 0)
                mm_first_runnable = i;
            mm_last_runnable = i;
            mm_menu_apps_count++;
        }
    }

    if (mm_menu_apps_count <= 0) {
        /* Display "no apps" message and go into infinite sleep */
        boz_display_clear();
        boz_display_write_string_P(s_mm_no_loadable_apps);
    }
    else {
        boz_set_event_cookie(NULL);
        boz_set_event_handler_qm_rotary(mm_rotary_handler);
        boz_set_event_handler_qm_rotary_press(mm_play_handler);
        boz_set_event_handler_qm_play(mm_play_handler);

        mm_app_cursor = mm_first_runnable;
        mm_redraw_display(1);
    }
}

void
mm_rotary_handler(void *cookie, int clockwise) {
    struct boz_app app;
    int direction = (clockwise ? 1 : -1);
    int cursor = mm_app_cursor;

    /* Find the next (or previous) app in the list with BOZ_APP_MAIN set, and
       if we hit either end of the list, give up */
    do {
        cursor += direction;
        if (cursor >= 0 && cursor < mm_apps_count) {
            mm_load_app(&app, cursor);
        }
    } while (cursor >= 0 && cursor < mm_apps_count && (app.flags & BOZ_APP_MAIN) == 0);

    if (cursor >= 0 && cursor < mm_apps_count) {
        mm_app_cursor = cursor;
        mm_redraw_display(0);
    }
}

void
mm_play_handler(void *cookie) {
    struct boz_app app;
    mm_load_app(&app, mm_app_cursor);
    boz_app_call(app.id, NULL, mm_return_callback, NULL);
}

void
mm_return_callback(void *cookie, int rc) {
    mm_redraw_display(1);
}
