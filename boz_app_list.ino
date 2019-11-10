#include "boz_app.h"

#include <avr/pgmspace.h>
#include "boz_app_list.h"
#include "boz_app_inits.h"

#define APP_LIST_LENGTH (sizeof(app_list) / sizeof(app_list[0]))

const PROGMEM struct boz_app app_list[] = {
    { "Menu", main_menu_init, 0 },
    { "Conundrum", conundrum_init, BOZ_APP_MAIN },
    { "Buzzer round", buzzer_round_init, BOZ_APP_MAIN },
    { "Chess clocks", chess_init, BOZ_APP_MAIN },
    { "System info", sysinfo_init, BOZ_APP_MAIN },
    { "Option menu", option_menu_init, 0 },
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

const struct boz_app *
boz_app_list_get(void) {
    return app_list;
}

int
boz_app_list_get_length(void) {
    return APP_LIST_LENGTH;
}
