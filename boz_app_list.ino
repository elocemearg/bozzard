#include "boz_hw.h"
#include "boz_app.h"

#include <avr/pgmspace.h>
#include "boz_app_list.h"
#include "boz_app_inits.h"

#define APP_LIST_LENGTH (sizeof(app_list) / sizeof(app_list[0]))

const PROGMEM struct boz_app app_list[] = {
    { BOZ_APP_ID_MAIN_MENU, "Menu", main_menu_init, 0 },
#ifdef BOZ_SERIAL
    { BOZ_APP_ID_PC_CONTROL, "PC control", pcc_init, BOZ_APP_MAIN | BOZ_APP_NO_SLEEP },
#else
    { BOZ_APP_ID_CONUNDRUM, "Conundrum", conundrum_init, BOZ_APP_MAIN },
    { BOZ_APP_ID_BUZZER_ROUND, "Buzzer round", buzzer_round_init, BOZ_APP_MAIN },
    { BOZ_APP_ID_CHESS_CLOCKS, "Chess clocks", chess_init, BOZ_APP_MAIN },
#endif

    /* Not enough room for the null terminator, so don't have one here.
       The main menu app will still only read the first 16 characters. */
    { BOZ_APP_ID_BACKLIGHT, { 'B', 'a', 'c', 'k', 'l', 'i', 'g', 'h', 't', ' ', 'o', 'n', '/', 'o', 'f', 'f' }, backlight_init, BOZ_APP_MAIN },

    { BOZ_APP_ID_BATTERY, "Battery info", battery_init, BOZ_APP_MAIN },
    { BOZ_APP_ID_SYSINFO, "About Bozzard", sysinfo_init, BOZ_APP_MAIN },

    { BOZ_APP_ID_OPTION_MENU, "Option menu", option_menu_init, 0 },
    { BOZ_APP_ID_CRASH, "Crash", crash_init, 0 },
};

#if 0
static int strings_equal(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) {
            return 0;
        }
        ++s1;
        ++s2;
    }
    return *s1 == *s2;
}

int
boz_app_entry_fetch_by_name(struct boz_app *dest, const char *name) {
    struct boz_app app;
    for (int i = 0; i < (int) APP_LIST_LENGTH; ++i) {
        memcpy_P(&app, &app_list[i], sizeof(app));
        if (strings_equal(app.name, name)) {
            if (dest != NULL)
                *dest = app;
            return 1;
        }
    }
    return 0;
}
#endif

int
boz_app_lookup_id(int id, struct boz_app *dest) {
    for (int i = 0; i < (int) APP_LIST_LENGTH; ++i) {
        if ((int) pgm_read_word_near(&app_list[i].id) == id) {
            memcpy_P(dest, &app_list[i], sizeof(struct boz_app));
            return 0;
        }
    }
    return -1;
}

const struct boz_app *
boz_app_list_get(void) {
    return app_list;
}

int
boz_app_list_get_length(void) {
    return APP_LIST_LENGTH;
}
