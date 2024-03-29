#ifndef _APP_H
#define _APP_H

#include "boz_hw.h"

/* bits for boz_app.flags */

/* List this app in the main menu */
#define BOZ_APP_MAIN 1

/* Do not put the MCU to sleep while this app is running */
#define BOZ_APP_NO_SLEEP 2

struct boz_app {
    int id;
    char name[16];
    void (*init)(void *);
    unsigned int flags;
    unsigned int eeprom_start;
    unsigned int eeprom_length;
};

struct app_context {
    /* If bit N is set, then this app is using clock N, and we know to release
     * that clock if the app exits without releasing it. */
    unsigned short clocks_enabled;

    /* If alarm_handler is not NULL, then when millis() is equal or past
     * alarm_time_millis, the main loop will call alarm_handler(alarm_cookie),
     * having set alarm_handler to NULL first. */
    void *alarm_handler_cookie;
    unsigned long alarm_time_millis;
    void (*alarm_handler)(void *cookie);

    /* If true, the MCU will not be put to sleep while this app is running. */
    char forbid_sleep;

    /* Position of this app's reserved region in the EEPROM, and its length
     * in bytes. If the length is zero, this app doesn't have a region of
     * EEPROM. */
    unsigned int eeprom_start, eeprom_length;

    /* Opaque pointer passed to all the event_* handlers that handle
     * button presses. */
    void *event_cookie;

    /* Event handlers to do with button presses are "sticky" - that is, once
     * the event handler has returned, the app will continue to receive
     * events until they are explicitly cancelled by setting the handler to
     * NULL. */

    /* Contestant presses buzzer */
    void (*event_buzz)(void *cookie, int which_buzzer);

    /* QM presses PLAY (>) button */
    void (*event_qm_play)(void *cookie);

    /* QM presses YELLOW button */
    void (*event_qm_yellow)(void *cookie);

    /* QM presses RESET button */
    void (*event_qm_reset)(void *cookie);

    /* QM turns rotary encoder one step clockwise or anticlockwise */
    void (*event_qm_rotary)(void *cookie, int clockwise);

    /* QM presses rotary encoder's pushbutton */
    void (*event_qm_rotary_press)(void *cookie);

    /* Sound queue has space in it. This event handler will detach itself
     * immediately before being called. If the application wants to continue
     * to receive events about the sound queue not being full, it must
     * re-attach itself in the event handler. */
    void (*event_sound_queue_not_full)(void *cookie);

#ifdef BOZ_SERIAL
    /* Data is available to read on the serial port */
    void (*event_serial_data_available)(void *cookie);
#endif

    /* If this app has called another one, we'll call this handler when the
     * called app returns. */
    void *app_call_return_cookie;
    void (*app_call_return_handler)(void *, int);
};

#endif
