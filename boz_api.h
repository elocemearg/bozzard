#ifndef _BOZ_API_H
#define _BOZ_API_H

#include "boz_hw.h"
#include "boz_display.h"
#include "boz_sound.h"
#include "boz_clock.h"
#include "boz_serial.h"
#include "boz_mm.h"
#include "boz_app_inits.h"

#define FUNC_BUZZER 0
#define FUNC_PLAY 1
#define FUNC_YELLOW 2
#define FUNC_RESET 3
#define FUNC_RE_KEY 4
#define FUNC_RE_CLOCK 5

#define BOZ_CHAR_BACK 1
#define BOZ_CHAR_PLAY 2
#define BOZ_CHAR_HBAR 3
#define BOZ_CHAR_RESET 4
#define BOZ_CHAR_WHEEL_AC 5
#define BOZ_CHAR_WHEEL_C 6

#define BOZ_CHAR_BACK_S "\x01"
#define BOZ_CHAR_PLAY_S "\x02"
#define BOZ_CHAR_HBAR_S "\x03"
#define BOZ_CHAR_RESET_S "\x04"
#define BOZ_CHAR_WHEEL_AC_S "\x05"
#define BOZ_CHAR_WHEEL_C_S "\x06"

#define BOZ_NUM_BUZZERS 4
#define BOZ_NUM_CLOCKS 4
#define BOZ_NUM_LEDS 4

#define BOZ_DISP_NUM_FORCE_SIGN 1
#define BOZ_DISP_NUM_ZERO_PAD 2
#define BOZ_DISP_NUM_ARROWS 4
#define BOZ_DISP_NUM_SPACES 8
#define BOZ_DISP_NUM_HEX 16

/******************************************************************************
 *
 * To create a new app:
 *
 * 1. Create a new .ino file for your app, which is where all your
 *    application's code goes, and #include "boz_api.h".
 *    This app must have (at least) an init function.
 *    This init function returns void and takes a single void * argument. If
 *    your app is called from the main menu, this argument will be NULL.
 *    See sysinfo.ino (which displays version information) for an example of
 *    a simple app.
 *
 * 2. Add a symbol for your app in the BOZ_APP_ID enum in boz_app_inits.h.
 *    For example, if your app is called FOOBAR, you might add the symbol
 *    BOZ_APP_ID_FOOBAR.
 *
 * 3. Add a declaration for your apps init function in the same file,
 *    boz_app_inits.h. It should look like all the others, e.g.:
 *        extern void foobar_init(void *);
 *
 * 4. Add an entry for your app into the struct boz_app app_list[] array in
 *    boz_app_list.ino. The four fields in the entry are your app's ID, your
 *    app's name (up to 16 characters), your app's init function, and any
 *    flags for this app entry. If you want your app to appear in the main menu
 *    the flags value should be BOZ_APP_MAIN. For example, the entry for the
 *    FOOBAR application might be this:
 *        { BOZ_APP_ID_FOOBAR, "Foobar", foobar_init, BOZ_APP_MAIN }
 *
 * 5. When you build the Bozzard software, your app should now be included.
 *
 *****************************************************************************/

/******************************************************************************
 * LEDs
 *
 * Bozzard API functions to set the state of the four LEDs.
 *****************************************************************************/

/* boz_led_set
 * Switch one of the LEDs on or off, as follows:
 * which_led    LED colour
 *         0           red
 *         1         green
 *         2        yellow
 *         3          blue
 * If which_led is any other number, the call has no effect.
 * The LED is switched on if "on" is nonzero, otherwise it is switched off.
 */
void
boz_led_set(int which_led, int on);

/* boz_leds_set
 * Switch the four LEDs on or off, as specified by the four lowest bits
 * of "mask", which is a bitwise-OR of zero or more of the following flags.
 * If the bit is set the LED is switched on, otherwise it is switched off.
 * bitmask      LED colour
 *       1             red
 *       2           green
 *       4          yellow
 *       8            blue
 */
void
boz_leds_set(int mask);


/******************************************************************************
 * APP CALLING AND EXITING
 *
 * API functions to call other apps, and to exit from your app and return
 * control to the app that called yours.
 *****************************************************************************/

/* boz_app_call
 * When the calling event handler returns, the main loop will start up another
 * application and give it control. While the called app is running, the
 * calling app is suspended. Any clocks the calling app started will continue
 * to run, but any events which occur while the called app is running will not
 * be delivered to the calling app. If any clock events or alarms occur in the
 * calling app during this time, their handlers will only be called after the
 * called app exits and the calling app regains control.
 *
 * app_id: the ID of the app to run, as listed in boz_app_inits.h.
 * param: The parameter to pass to the application's init function. Some
 *        applications ignore this parameter, and some, such as the options
 *        menu app, require it to point to a particular data structure. This
 *        depends on the specific app.
 * return_callback: The function to be called when the called app exits. This
 *                  callback function takes two arguments: a cookie
 *                  (which will be return_callback_cookie) and a status (which
 *                  will be the integer exit status of the application as it
 *                  passed to boz_app_exit().
 * return_callback_cookie: The first parameter to pass to return_callback().
 *                         This has meaning only to the return_callback
 *                         function.
 */
int
boz_app_call(int app_id, void *param, void (*return_callback)(void *, int), void *return_callback_cookie);

/* boz_app_exit
 * Exit the application.
 * Once the event handler which called boz_app_exit returns:
 *     * any running clocks the application had will be stopped and released;
 *     * any chunks of dynamically-allocated memory obtained by a call by the
 *       application to boz_mm_alloc() will be freed;
 *     * no further events will be delivered to the application.
 *
 * status: If this application was started with boz_app_call, the value of
 *         "status" will be passed to the calling app's return callback
 *         function.
 */
void
boz_app_exit(int status);


/******************************************************************************
 * DYNAMIC MEMORY ALLOCATION
 *
 * Unsophisticated equivalent implementations of malloc() and free() which
 * work on a very small (512-byte) pool of memory, for use by applications.
 *****************************************************************************/

/* boz_mm_alloc
 * Allocate "size" bytes of memory from the pool and return a pointer to it.
 * If "size" is zero, or if there is no contiguous chunk of memory in the pool
 * large enough to satisfy the request, NULL is returned.
 *
 * If a valid pointer is returned, the application owns this memory until it
 * is freed by boz_mm_free() or until the application exits.
 *
 * When an application exits after a call to boz_app_exit(), all memory it has
 * allocated with boz_mm_alloc() is freed automatically.
 * */
void *boz_mm_alloc(boz_mm_size size);

/* boz_mm_free
 * Free a chunk of memory previously returned by boz_mm_alloc(), and return
 * it to the pool.
 *
 * If the supplied pointer is NULL, nothing happens.
 *
 * Otherwise, if the supplied pointer has already been freed, or was not
 * returned by a previous call to boz_mm_alloc(), behaviour is undefined.
 */
void boz_mm_free(void *p);


/******************************************************************************
 * EVENT HANDLERS
 * 
 * Bozzard applications are entirely event-driven. After the app's init call
 * returns, the app will not be given control again except in response to
 * events. For this reason, the app's init method will almost certainly
 * attach one or more event handlers, which will receive control whenever the
 * associated event occurs.
 *
 * When an event handler has control, the main loop doesn't, which means the
 * main loop can't respond to any other events. An event handler will not be
 * interrupted by another event handler - one event handler must return before
 * the next will be called. For this reason, event handlers MUST NOT BLOCK,
 * or events could be missed. Do not call delay() from an event handler. The
 * boz_display_* and boz_sound_* functions are safe to call from an event
 * handler, because these functions put commands on a queue to be dealt with
 * asynchronously.
 *
 * If an application needs to wait for a period of time before doing something,
 * consider setting an alarm event instead (see boz_set_alarm()).
 *
 * Event handlers corresponding to button presses, including the buzzers and
 * the QM controls (play, reset, yellow, and the rotary knob), are "sticky" -
 * they will be called once for each time the event happens, and the handler
 * does not need to re-attach itself.
 *
 * An event handler can be detached by setting its handler function to NULL.
 * The application will then not receive any more calls for that event unless
 * and until the event handler is re-attached.
 *
 * Other event handlers - those not associated with button presses - are
 * usually one-shot only. This includes the event handler for telling the
 * application that the sound queue is no longer full. These event handlers
 * will be detached by the main loop immediately before being called. If the
 * application wants to be called again the next time the event happens, the
 * event handler must manually re-attach itself by calling the appropriate
 * boz_set_event_handler_...() function.
 *
 * If an application calls another application, each running application has a
 * separate list of event handlers. So you don't need to worry about messing up
 * any event handler settings for a calling application - when your app exits
 * and returns control to the calling app, the calling app will see all the
 * event handler settings exactly the same as they were before your app
 * started.
 *****************************************************************************/

/* boz_set_event_cookie
 * Set the cookie which will be passed to calls to event handlers for
 * buzzer events and QM button events. This can be any pointer - it has
 * meaning only to the application.
 */
void
boz_set_event_cookie(void *cookie);

/* boz_set_event_handler_buzz
 * Set the event handler which will be called when one of the buzzers is
 * pressed. The first argument of this handler is a void * ("cookie"), which
 * was set with boz_set_event_cookie(). The second argument indicates to the
 * handler which buzzer was pressed - it is an integer 0, 1, 2 or 3. The
 * sockets for these buzzers are numbered 1, 2, 3 and 4 respectively on the
 * control unit.
 * If handler is NULL, the event handler is detached and the application will
 * not receive calls for this event.
 */
void
boz_set_event_handler_buzz(void (*handler)(void *cookie, int which_buzzer));

/* boz_set_event_handler_qm_play
 * Set the event handler to be called when the QM's "play" button is pressed.
 * If "handler" is NULL, the event handler is detached. The handler takes one
 * argument - a pointer which was set with boz_set_event_cookie().
 * */
void
boz_set_event_handler_qm_play(void (*handler)(void *));

/* boz_set_event_handler_qm_yellow
 * Set the event handler to be called when the QM's yellow button is pressed.
 * If "handler" is NULL, the event handler is detached. The handler takes one
 * argument - a pointer which was set with boz_set_event_cookie(). */
void
boz_set_event_handler_qm_yellow(void (*handler)(void *));

/* boz_set_event_handler_qm_reset
 * Set the event handler to be called when the QM's "reset" button is pressed.
 * If "handler" is NULL, the event handler is detached. The handler takes one
 * argument - a pointer which was set with boz_set_event_cookie(). */
void
boz_set_event_handler_qm_reset(void (*handler)(void *));

/* boz_set_event_handler_qm_rotary
 * Set the event handler to be called when the QM's rotary knob is turned.
 * This event handler will be called for each "step" of the knob.
 * If "handler" is NULL, the event handler is detached.
 * The handler takes two arguments - the first is a "cookie" which was set
 * with boz_set_event_cookie(), and the second is an integer: 0 if the
 * knob was turned anticlockwise, and 1 if it was turned clockwise. */
void
boz_set_event_handler_qm_rotary(void (*handler)(void *, int clockwise));

/* boz_set_event_handler_qm_rotary_press
 * Set the event handler to be called when the QM's rotary knob is pressed.
 * If "handler" is NULL, the event handler is detached.
 * The handler takes one argument - a pointer which was set with
 * boz_set_event_cookie().
 */
void
boz_set_event_handler_qm_rotary_press(void (*handler)(void *));


/* boz_set_event_handler_sound_queue_not_full
 * Set the event handler to be called whenever the sound queue can accept
 * input. The boz_sound_*() functions all put commands on this queue, and if
 * the queue is full those functions will fail.
 * If "handler" is NULL, the event handler is detached.
 *
 * This event handler also automatically detaches itself immediately before
 * the handler is called. If the application wishes to continue to be notified
 * of this event after the event handler has returned, the event handler must
 * manually re-attach itself.
 *
 * The cookie passed to the handler is, as with the other event handlers, the
 * pointer given to the last call to boz_set_event_cookie().
 */
void
boz_set_event_handler_sound_queue_not_full(void (*handler)(void *));

/* boz_set_event_handler_serial_data_available
 * Only available if BOZ_SERIAL is defined.
 * Set the event handler to be called when there is data to read on the
 * serial port.
 * This event handler does not detach itself automatically.
 * The cookie passed to the handler is the pointer given to the last call
 * to boz_set_event_cookie(). */
void
boz_set_event_handler_serial_data_available(void (*handler)(void *));


/******************************************************************************
 * LCD (DISPLAY) CONTROL
 *
 * Functions for configuring and writing to the LCD, which has two rows by
 * 16 columns of text.
 *
 * The following functions add commands to the display commands queue.
 * Bozzard's main loop takes commands off the queue and runs them, only
 * executing the next command when the previous command is known to have
 * completed.
 *
 * This queue has a limited length, so if you enqueue too many commands at
 * once, the queue will fill up and any further attempts to enqueue commands
 * will fail until there's space. If this happens, the display_* function will
 * return -1. Normally these functions return 0 to indicate success - this
 * means the command was successfully put on the queue, not that it was
 * successfully executed!
 *
 * The display has two rows of 16 characters each. Co-ordinates are given
 * with the row number first. Rows and columns are numbered from zero, so the
 * top-left cell is (0,0) and the bottom-right cell is (1,15).
 *****************************************************************************/

/* boz_display_clear
 * Clear the LCD display and return to the cursor to the top-left cell (0,0). */
int
boz_display_clear();

/* boz_display_home
 * Move the cursor to the top-left cell (0,0). */
int
boz_display_home();

/* boz_display_properties
 * Set display properties.
 * display_on: switch display on if true, off if false.
 * cursor_on: show the location of the cursor on the display if true.
 * cursor_blink_on: blink the cursor if true.
 *
 * By default, when the main setup routine initialises the display, the display
 * is on, the cursor is off, and cursor blinking is off.
 */
int
boz_display_properties(int display_on, int cursor_on, int cursor_blink_on);

/* boz_display_set_cgram_address
 *
 * Move the display's "cursor" to point inside its character generator RAM,
 * for the purpose of defining your own characters. Character codes 0 through 7
 * are writable by the user in this way. The low three bits (0x07) indicate
 * which row of pixels you want to set, and the next highest three bits
 * (0x38) specify the character code you want to write to.
 * Once the cgram address has been set, you can write to that row of pixels in
 * using display_write_char() - only the five least significant bits of that
 * function's argument will be used, corresponding to the five pixels on the
 * chosen line.
 *
 * For example, to set character code 5 to a (crudely-drawn) smiley face, you
 * might do the following:
 * display_set_cgram_address((5 << 3) | 0);
 * display_write_char(0x0e);
 * display_write_char(0x11);
 * display_write_char(0x1b);
 * display_write_char(0x11);
 * display_write_char(0x1f);
 * display_write_char(0x11);
 * display_write_char(0x0e);
 *
 *   4 3 2 1 0
 * 0   # # # 
 * 1 #       #
 * 2 # #   # #
 * 3 #       #
 * 4 # # # # #
 * 5 #       #
 * 6   # # # 
 * 7
 *
 * Then, to write this smiley face to the top-left corner of the display:
 * display_set_cursor(0, 0);
 * display_write_char(5);
 * */
int
boz_display_set_cgram_address(int address);

/* boz_display_set_cursor
 * Move the display's cursor to the given row and column. The top row is row 0,
 * and the left-hand column is column 0. */
int
boz_display_set_cursor(int row, int column);

/* boz_display_write_char
 * Write the given character at the display's cursor, and increment the cursor
 * position. */
int
boz_display_write_char(char c);

/* boz_display_write_string
 * Write the characters in the nul-terminated string s to the display, not
 * including the terminating nul. This enqueues a command for every character
 * in the string. */
int
boz_display_write_string(const char *s);

/* boz_display_write_string_P
 * The same as boz_display_write_string, but the string pointed to by str_pm
 * is expected to be in PROGMEM. */
int
boz_display_write_string_P(const char *str_pm);

/* Format the given long integer "num" as a string, and write it to the current
 * display position. If min_width is positive, it specifies the minimum number
 * of characters that will be written (max 20). If necessary, the number is
 * padded on the right with spaces (or zeroes, if the BOZ_DISP_NUM_ZERO_PAD
 * flag is used) to achieve this.
 *
 * "flags" is a set of any of the following flags, bitwise-ORed together. They
 * modify how the number is formatted.
 *
 * BOZ_DISP_NUM_FORCE_SIGN
 *     Write a plus sign before a positive integer.
 * BOZ_DISP_NUM_ZERO_PAD
 *     Left-pad with zeroes instead of spaces to make the number at least
 *     min_width chars.
 * BOZ_DISP_NUM_ARROWS
 *     Draw arrows (or brackets, or some sort of highlighter) on the left
 *     and right of the number.
 * BOZ_DISP_NUM_SPACES
 *     Leave space for arrows on the left and right of the number but don't
 *     draw them.
 *     If arrows are drawn, then if we're padding with spaces (the
 *     default) the arrows are added before the padding. If we're padding
 *     with zeroes, the arrows are added after the padding. In any case,
 *     the min_width does not include the arrows.
 * BOZ_DISP_NUM_HEX
 *     'X': Format the number in hexadecimal, not base 10.
 *
 * Returns the total number of characters actually written, or -1 on error.
 */
int
boz_display_write_long(long num, int min_width, int flags);

/* boz_lcd_get_backlight_state
 * Query the state of the LCD's backlight.
 *
 * This does not enqueue any command on the display queue, but returns
 * immediately.
 *
 * Return 1 if the backlight is currently on, or 0 if it is off.
 */
int
boz_lcd_get_backlight_state(void);

/* boz_lcd_set_backlight_state
 * Set the state of the LCD's backlight.
 *
 * This does not enqueue a command - it takes effect immediately.
 *
 * If "state" is nonzero, the backlight is switched on. If "state" is zero,
 * the backlight is switched off.
 */
void
boz_lcd_set_backlight_state(int state);


/******************************************************************************
 * SOUND
 *
 * The Bozzard has a passive speaker connected to a digital output pin on the
 * Arduino. This can produce a surprisingly varied selection of
 * square-wave-based cacophonies.
 *
 * The following functions enqueue sound commands to the sound command queue,
 * which has space for a limited number of commands. As commands are completed,
 * they come off the queue.
 *
 * If you try to enqueue too many sound commands at once, the queue will fill
 * up and won't accept any more commands. If this happens, the boz_sound_*
 * call will return -1, in the same way as the display calls above.
 *
 * Some of these calls take a parameter of type boz_note. The valid values
 * for this parameter are listed in boz_notes.h. For example, middle C is
 * the constant NOTE_C4.
 *****************************************************************************/

/* boz_sound_silence
 * Enqueue a command to play nothing for the given duration. */
int
boz_sound_silence(unsigned int duration_ms);

/* boz_sound_note
 * Enqueue a command to play the named note for the given duration. */
int
boz_sound_note(boz_note note, unsigned int duration_ms);

/* boz_sound_varying
 * Enqueue a command to start playing note_start, and then change the frequency
 * to note_end over duration_ms milliseconds, num_times times. */
int
boz_sound_varying(boz_note note_start, boz_note note_end, unsigned int duration_ms, byte num_times);

/* boz_sound_arpeggio
 * Enqueue a command to play an arpeggio num_times times. The notes of the
 * arpeggio are indexes into the note table, and notes must point to a buffer
 * of num_notes notes. num_notes must be between 1 and 4 inclusive.
 * One play of the arpeggio will take duration_ms milliseconds. */
int
boz_sound_arpeggio(byte *notes, int num_notes, int duration_ms, byte num_times);

/* boz_sound_stop
 * Interrupt the currently-playing sound command. The main loop will stop
 * playing whatever it's playing and move on to the next sound command in
 * the queue. */
void
boz_sound_stop(void);

/* boz_sound_stop_all
 * Like sound_stop(), but also removes all pending sound commands from the
 * queue. */
void
boz_sound_stop_all(void);

/* boz_sound_square_bell
 * Enqueue the necessary command(s) to make a standard buzzer noise that sounds
 * almost, but not quite, entirely unlike a bell. which_buzzer must be between
 * 0 and 3 inclusive. This allows each buzzer noise to have a slightly
 * different pitch. */
void
boz_sound_square_bell(int which_buzzer);



/******************************************************************************
 * CLOCKS
 * 
 * A boz_clock (or "clock") measures and reports the passage of time. It is
 * best thought of as a stopwatch.
 *
 * When it is created, it shows an initial value in milliseconds. When the
 * clock is started, its value increases or decreases, depending on which
 * direction the clock has been set to run in.
 *
 * The clock may be stopped, restarted or reset to its initial value.
 * The clock may be set to deliver events to the application when certain
 * conditions are met.
 *
 * There are a limited number of clocks available to all currently-running
 * applications. If there are no more clocks, boz_clock_create() will return
 * NULL.
 *****************************************************************************/

/* boz_clock_create
 * Create a new clock with the given initial value, and return it.
 * The clock is initially stopped and has no alarms or min/max values set.
 * When the clock is started, it will run forwards if direction_forwards is
 * nonzero, or backwards if direction_forwards is zero.
 * If the maximum number of clocks would be exceeded by this call, this
 * function returns NULL.
 * */
boz_clock
boz_clock_create(long initial_value_ms, int direction_forwards);

/* boz_clock_release
 * Release a clock previously created with boz_clock_create(), so it can be
 * handed out to a subsequent call to boz_clock_create(). If the clock is
 * running, it is stopped.
 * All an application's clocks are automatically stopped and released when the
 * application exits. */
void
boz_clock_release(boz_clock clock);

/* boz_clock_run
 * Set the clock running in its chosen direction. This has no effect if the
 * clock is already running. */
void
boz_clock_run(boz_clock clock);

/* boz_clock_running
 * Check whether a clock is currently running.
 * Return 1 if the clock is currently running, and 0 otherwise. */
int
boz_clock_running(boz_clock clock);

/* boz_clock_stop
 * Stop the clock on its current value. This has no effect if the clock is
 * already stopped. */
void
boz_clock_stop(boz_clock clock);

/* boz_clock_reset
 * Set the clock's value to its initial value. If the clock is currently
 * running, this call will NOT stop the clock.
 * If you want it to stop on its initial value, call boz_clock_stop() then
 * boz_clock_reset(). */
void
boz_clock_reset(boz_clock clock);

/* boz_clock_set_direction
 * Set the clock's direction. It is permitted to change the clock's direction
 * while it is running, but note that if an alarm is set on this clock,
 * changing the clock's direction may make it look like that alarm time has
 * now "passed", resulting in the alarm's event handler being called. */
void
boz_clock_set_direction(boz_clock clock, int direction_forwards);

/* boz_clock_value
 * Return the current value on the clock, in milliseconds. */
long
boz_clock_value(boz_clock clock);

/* boz_clock_event_cookie
 * Set an arbitrary pointer ("cookie") which will be passed to this clock's
 * event callbacks. This is specific to this clock, and is independent of
 * the event cookie for general button-pressing events as set by
 * boz_set_event_cookie(). */
void
boz_clock_set_event_cookie(boz_clock clock, void *cookie);

/* boz_clock_set_alarm
 * Configure the clock to call the supplied callback function when the clock's
 * value has reached or passed alarm_value_ms. ("passed" takes account of which
 * direction the clock is running in.)
 *
 * This overrides any previously-set alarm on this clock.
 *
 * When the clock's value has reached or passed alarm_value_ms, the alarm is
 * automatically detached and callback() is called, with the cookie (set by
 * boz_clock_set_event_cookie()) as its first argument and a pointer to the
 * clock as its second.
 *
 * If the application wants a repeated alarm, the event handler callback must
 * manually re-attach itself by calling boz_clock_set_alarm with a new
 * alarm value.
 */
void
boz_clock_set_alarm(boz_clock clock, long alarm_value_ms,
        void (*callback)(void *, boz_clock));

/* boz_clock_cancel_alarm
 * Cancel any alarm previously set with boz_clock_set_alarm(). This function
 * has no effect if no such alarm was set. */
void
boz_clock_cancel_alarm(boz_clock clock);

/* boz_clock_set_expiry_max
 * Set the maximum value for the clock, and optionally attach an event handler
 * to the event of the clock's value reaching this value.
 * When the clock's value reaches or exceeds max_ms, the clock is stopped,
 * the handler (if any) is detached, and, if callback is not NULL, callback()
 * is called, passing the cookie (see boz_clock_set_event_cookie()) and the
 * pointer to the clock.
 */
void
boz_clock_set_expiry_max(boz_clock clock, long max_ms,
        void (*callback)(void *, boz_clock));

/* boz_clock_set_expiry_min
 * Set the minimum value for the clock, and optionally attach an event handler
 * to the event of the clock's value reaching this value.
 * When the clock's value reaches or falls below min_ms, the clock is stopped,
 * the handler (if any) is detached, and, if callback is not NULL, callback()
 * is called, passing the cookie (see boz_clock_set_event_cookie()) and the
 * pointer to the clock.
 */
void
boz_clock_set_expiry_min(boz_clock clock, long min_ms,
        void (*callback)(void *, boz_clock));

/* boz_clock_cancel_expiry_min
 * Cancel any minimum clock value and its associated handler. */
void
boz_clock_cancel_expiry_min(boz_clock clock);

/* boz_clock_cancel_expiry_max
 * Cancel any maximum clock value and its associated handler. */
void
boz_clock_cancel_expiry_max(boz_clock clock);

/* boz_clock_set_initial_value
 * Change the clock's initial value to value_ms milliseconds. This does not
 * set the clock's current value - it sets what initial value the clock will
 * have the next time it is reset with boz_clock_reset(). */
void
boz_clock_set_initial_value(boz_clock clock, long value_ms);

/* boz_clock_is_direction_forwards
 * Determine which direction the clock is running. If the clock is currently
 * stopped, we return the direction the clock would be running if it were
 * running.
 * Returns 1 if the clock's direction is forwards, 0 if it's backwards.
 */
int
boz_clock_is_direction_forwards(boz_clock clock);

/* boz_clock_get_ms_until_alarm
 * Check how many milliseconds the clock needs to run to reach the alarm time
 * set with boz_clock_set_alarm.
 *
 * If there is no alarm set on this clock, (unsigned long) -1 is returned.
 *
 * If the alarm time is already reached or passed, 0 is returned.
 *
 * Otherwise, the number of milliseconds this clock is away from the alarm
 * time is returned.
 */
unsigned long
boz_clock_get_ms_until_alarm(boz_clock clock);

/* boz_clock_add
 * Add a given amount of time to the clock's value.
 */
void
boz_clock_add(boz_clock clock, long ms_to_add);


/******************************************************************************
 * ALARMS
 *
 * API functions for managing alarms, which tell the Bozzard to call a function
 * in a given number of milliseconds from now.
 *****************************************************************************/

/* boz_set_alarm
 * Tell the main loop to call a handler a certain number of milliseconds
 * from now.
 * ms_from_now: the number of milliseconds that should elapse before the
 *              handler is called.
 * handler:     the handler to call.
 * cookie:      the parameter to pass to the handler.
 *
 * Note that this event is independent of the "alarm" that can be set on a
 * boz_clock (see above). Furthermore, the "cookie" is independent of,
 * and nothing to do with, the cookie set with boz_set_event_cookie(). 
 * The cookie and handler passed to this function are used only for this
 * alarm.
 *
 * This event is not sticky - it will detach itself immediately before the
 * event handler is called. If the application wants a repeated alarm,
 * the event handler should manually re-attach itself by calling
 * boz_set_alarm().
 */
void
boz_set_alarm(long ms_from_now, void (*handler)(void *), void *cookie);

/* boz_cancel_alarm
 * Cancel a pending alarm previously set by boz_set_alarm(). If no alarm was
 * set, this function has no effect. */
void
boz_cancel_alarm();


/******************************************************************************
 * NON-VOLATILE STORAGE (EEPROM)
 *
 * Each app has a region of EEPROM reserved for it. The app list contains,
 * for each application, the offset within EEPROM where that app's region
 * starts, and how large that region is in bytes.
 *
 * On the Arduino Nano, valid EEPROM locations are 0x0 to 0x3ff inclusive,
 * which is 1024 bytes.
 * 
 * An app may call boz_eeprom_write() and boz_eeprom_read() to write to and
 * read from EEPROM. These calls will write to and read from the current app's
 * own EEPROM region only.
 * 
 * At any point, an app may assume that its own EEPROM region contains EITHER:
 *     (a) data the app has previously written to that region, or
 *     (b) all 0xff.
 * 
 *****************************************************************************/

/* boz_eeprom_get_region_size
 * Return the size of the currently-running app's reserved EEPROM region,
 * in bytes. The return value may be zero, which means the app doesn't have an
 * EEPROM region.
 */
unsigned int boz_eeprom_get_region_size();

/* boz_eeprom_write
 * Write the "length" bytes pointed to by "data" to the EEPROM, starting
 * "eeprom_region_offset" bytes from the start of the app's own EEPROM region.
 *
 *     eeprom_region_offset: the offset within the app's EEPROM region.
 *     data: a pointer to the data to write.
 *     length: the number of bytes to write.
 *
 * returns: 0 on success, <0 on failure.
 *
 * This function fails, and writes nothing, if eeprom_region_offset is greater
 * than the size of the app's own EEPROM region, or if eeprom_region_offset +
 * size is greater than the size of the app's own EEPROM region. In other
 * words, an app is only allowed to write to its own EEPROM region and not
 * anywhere else.
 *
 * This function uses EEPROM.update, which only actually writes a byte to an
 * EEPROM location if the current value of that byte is different.
 *
 * It takes about 3.3ms to write a single EEPROM location (if its value needs
 * to change) so this call may block for a significant amount of time. This may
 * cause events to be missed.
 */
int boz_eeprom_write(unsigned int eeprom_region_offset, const void *data, unsigned int length);

/* boz_eeprom_read
 * Read "length" bytes from the app's own EEPROM region, starting
 * "eeprom_region_offset" bytes from the start of that region, and put the
 * data in the buffer pointed to by "dest".
 *
 *     eeprom_region_offset: the offset within the app's EEPROM region.
 *     dest: where to copy the data to.
 *     length: the number of bytes to read.
 *
 * returns: 0 on success, <0 on failure.
 *
 * This function fails if eeprom_region_offset is greater than the app's own
 * EEPROM region size, or if eeprom_region_offset + length is greater than the
 * app's own EEPROM region size. In other words, the app is only allowed to
 * read from its own EEPROM region.  In this case the contents of "data" are
 * undefined.
 */
int boz_eeprom_read(unsigned int eeprom_region_offset, void *dest, unsigned int length);

/* boz_eeprom_global_reset
   Write over the entire EEPROM, filling it with the byte '\xFF', except for
   a region at the beginning (currently 12 bytes) where a special header is
   written.
   Note this resets the entire EEPROM, not just the region belonging to the
   currently-running application.
   Return 0 on success and <0 on failure. */
int boz_eeprom_global_reset();



/******************************************************************************
 * MISCELLANEOUS
 *****************************************************************************/

/* boz_get_version
 * Return the version number of the Bozzard API implementation.
 * A 4-byte long is returned. The most significant byte is the major version
 * number, the next most significant byte is the minor version number, and
 * the next most significant byte is the release number. The least significant
 * byte is not currently used.
 */
long
boz_get_version(void);

/* boz_is_button_pressed
 * Ask whether a button is considered "pressed" by the main loop. A button is
 * pressed if it has been active for more than a certain length of time
 * (typically a few milliseconds), or has been inactive for LESS than a
 * certain length of time (again, typically a few milliseconds). This
 * qualification helps filter out bounces.
 * button_func and buzzer_id identify which button is being asked about.
 * 
 * button_func:
 *     FUNC_BUZZER: one of the buzzers. buzzer_id identifies which one. This
 *                  must be an integer between 0 and 3 inclusive.
 *     FUNC_PLAY:   the QM's play button.
 *     FUNC_YELLOW: the QM's yellow button.
 *     FUNC_RESET:  the QM's reset button.
 *     FUNC_RE_KEY: the QM's rotary knob pushbutton.
 * For all button_func values other than FUNC_BUZZER, the buzzer_id value is
 * ignored.
 *
 * If the button is currently considered pressed, this function returns 1
 * and, if pressed_since_micros_r is not NULL, *pressed_since_micros_r is set
 * to the value of micros() since when the button was continually pressed.
 * If the button is NOT currently considered pressed, we return 0 and the
 * value of *pressed_since_micros_r is undefined.
 */

int
boz_is_button_pressed(int button_func, int buzzer_id, unsigned long *pressed_since_micros_r);

/* boz_get_battery_voltage
 * Return the voltage currently provided by the battery, in millivolts.
 * This is accomplished by a potential divider between the VIN pin, the A6 pin
 * (analogue input) and ground. The voltage on the A6 pin is half the voltage
 * at VIN. We read the analogue value at A6, double it and reference it to 5V
 * to get the voltage of VIN.
 * Note that while we return the value in millivolts, that doesn't mean it's
 * accurate to the nearest millivolt! The analogRead() function returns a
 * number between 0 and 1023, so the resolution with a 5V reference is about
 * 5mV. We double this to take account of the fact that the voltage at A6 is
 * half that at VIN, so the return value should be considered to be roughly
 * accurate to about 10mV.
 */
int
boz_get_battery_voltage(void);

/* boz_crash
 * Run the crash application, which displays a message on the screen and
 * sets the LEDs to the bottom four bits of the given pattern. The pattern
 * value will be displayed on the screen in hex, along with the address of the
 * boz_crash caller.
 * This function never returns, and there is no way to continue. The user has
 * to reboot the Bozzard. */
void
boz_crash(unsigned int pattern);

#endif
