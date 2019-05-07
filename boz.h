#ifndef _BOZ_H
#define _BOZ_H

#include "boz_display.h"
#include "boz_sound.h"
#include "boz_clock.h"

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

/* Applications should not need to use boz_sound_enqueue() directly - instead,
 * use one of the helper functions declared in boz_sound.h. */
int
boz_sound_enqueue(unsigned int freq_start, unsigned int freq_end, unsigned int duration_ms);

/* Applications should not need to use boz_display_enqueue() directly -
 * instead, use one of the helper functions declared in boz_display.h. */
int
boz_display_enqueue(unsigned int cmd_word);

/* boz_clock_create
 * Create a new clock with the given initial value, and return a pointer to
 * the clock. The clock is initially stopped and has no alarms or min/max
 * values set. When the clock is started, it will run forwards if
 * direction_forwards is nonzero, and backwards if direction_forwards is zero.
 * Clocks are statically allocated in memory, and there are a limited number.
 * If there are no more clocks, this function returns NULL.
 *
 * See boz_clock.h for the clock API.
 * */
struct boz_clock *
boz_clock_create(long initial_value_ms, int direction_forwards);

/* boz_clock_release
 * Release a clock previously created with boz_clock_create(), so it can be
 * handed out to a subsequent call to boz_clock_create(). If the clock is
 * running, it is stopped.
 * All an application's clocks are automatically stopped and released when the
 * application exits. */
void
boz_clock_release(struct boz_clock *clock);

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

/*****************************************************************************
 * EVENT HANDLERS
 * 
 * boz applications are entirely event-driven. After the app's init call
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
 * and, if pressed_since_millis_r is not NULL, *pressed_since_millis_r is set
 * to the value of millis() since when the button was continually pressed.
 * If the button is NOT currently considered pressed, we return 0 and the
 * value of *pressed_since_millis_r is undefined.
 */
int
boz_is_button_pressed(int button_func, int buzzer_id, unsigned long *pressed_since_millis_r);

/* boz_set_alarm
 * Tell the main loop to call a handler a certain number of milliseconds
 * from now.
 * ms_from_now: the number of milliseconds that should elapse before the
 *              handler is called.
 * handler:     the handler to call.
 * cookie:      the parameter to pass to the handler.
 *
 * Note that this event is independent of the "alarm" that can be set on a
 * boz_clock (see boz_clock.h). Furthermore, the "cookie" is independent of,
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

/* boz_app_call
 * When the calling event handler exits, the main loop will start up another
 * application and give it control. While the called app is running, the
 * calling app is suspended. Any clocks the calling app started will continue
 * to run, but any events which occur while the called app is running will not
 * be delivered to the calling app. If any clock events or alarms occur in the
 * calling app during this time, their handlers will only be called after the
 * called app exits and the calling app regains control.
 *
 * init:  The init function of the application to run.
 * param: The parameter to pass to the application's init function. Some
 *        applications ignore this parameter, and some, such as the options
 *        menu app, require it to point to a particular data structure.
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
boz_app_call(void (*init)(void *), void *param, void (*return_callback)(void *, int), void *return_callback_cookie);


/* boz_app_exit
 * Exit the application. Once the event handler which called boz_app_exit
 * returns, any running clocks the application had will be stopped and
 * released, and no further events will be delivered to the application.
 *
 * status: If this application was started with boz_app_call, the value of
 *         "status" will be passed to the calling app's return callback
 *         function.
 */
void
boz_app_exit(int status);


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
#endif
