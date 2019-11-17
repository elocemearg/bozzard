#ifndef _BOZ_APP_INITS_H
#define _BOZ_APP_INITS_H

extern void main_menu_init(void *);
extern void conundrum_init(void *);
extern void test_init(void *);
extern void buzzer_round_init(void *);
extern void chess_init(void *);
extern void option_menu_init(void *);
extern void music_loop_init(void *);
extern void sysinfo_init(void *);
extern void crash_init(void *);
extern void backlight_init(void *);
#ifdef BOZ_SERIAL
extern void pcc_init(void *);
#endif

enum BOZ_APP_ID {
    BOZ_APP_ID_MAIN_MENU = 0,
    BOZ_APP_ID_PC_CONTROL,
    BOZ_APP_ID_CONUNDRUM,
    BOZ_APP_ID_BUZZER_ROUND,
    BOZ_APP_ID_CHESS_CLOCKS,
    BOZ_APP_ID_BACKLIGHT,
    BOZ_APP_ID_SYSINFO,
    BOZ_APP_ID_OPTION_MENU,
    BOZ_APP_ID_CRASH
};
#define BOZ_APP_ID_INIT 0

#endif
