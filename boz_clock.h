#ifndef _BOZ_CLOCK_H
#define _BOZ_CLOCK_H

/* boz_clock
 *
 * A boz_clock (or "clock") measures and reports the passage of time. It's
 * best thought of as a stopwatch.
 * When it is created, it shows an initial value in milliseconds. When the
 * clock is started, its value increases or decreases, depending on which
 * direction the clock has been set to run in.
 * The clock may be stopped, restarted or reset to its initial value.
 * The clock may be set to deliver events to the application when certain
 * conditions are met.
 *
 * There are a limited number of clocks available to all currently-running
 * applications. If there are no more clocks, boz_clock_create() will return
 * NULL.
 */

struct boz_clock_s {
    /* id of this clock, for the main loop */
    int id;

    /* The initial value on the clock when initialised or reset */
    long initial_value_ms;

    /* Valid range for the clock's value. These only apply if min_enabled or
     * max_enabled (as appropriate) are set. If the clock's value is outside
     * this range and the relevant min_enabled or max_enabled is set, the
     * value returned by clock_value() will be capped within the range. */
    long min_ms;
    long max_ms;

    /* If running, this is the clock's value when millis() returned
     * last_value_ard_ms.
     * If not running, this is the clock's value. */
    long last_value_ms;

    /* If running, the return value of millis() when last_value_ms was the
     * clock's value. */
    unsigned long last_value_ard_millis;

    /* If alarm_enabled is set, then when the current value of the clock has
     * reached or passed alarm_ms ("passed" takes account of whether the clock
     * is running forwards or backwards), the main loop will disable the alarm
     * and call event_alarm(). */
    long alarm_ms;

    void *event_cookie;
    void (*event_expiry_min)(void *cookie, struct boz_clock_s *clock);
    void (*event_expiry_max)(void *cookie, struct boz_clock_s *clock);
    void (*event_alarm)(void *cookie, struct boz_clock_s *clock);

    /* Whether the clock is running. */
    unsigned int running : 1;

    /* FORWARD or BACKWARD. */
    unsigned int direction : 1;

    /* Whether the alarm callback is set */
    unsigned int alarm_enabled : 1;

    /* Whether the expiry callbacks (min and max) are set */
    unsigned int min_enabled : 1;
    unsigned int max_enabled : 1;
};

typedef struct boz_clock_s *boz_clock;

/* Set the clock running in its chosen direction. This has no effect if the
 * clock is already running. */
void
boz_clock_run(boz_clock clock);

/* Return 1 if the clock is currently running, and 0 otherwise. */
int
boz_clock_running(boz_clock clock);

/* Stop the clock on its current value. This has no effect if the clock is
 * already stopped. */
void
boz_clock_stop(boz_clock clock);

/* Set the clock's value to its initial value. This will not stop the clock.
 * If you want it to stop on its initial value, call boz_clock_stop() then
 * boz_clock_reset(). */
void
boz_clock_reset(boz_clock clock);

/* Set the clock's direction. It is permitted to change the clock's direction
 * while it is running, but note that if an alarm is set on this clock,
 * changing the clock's direction may make it look like that alarm time has
 * now "passed", resulting in the alarm's event handler being called. */
void
boz_clock_set_direction(boz_clock clock, int direction_forwards);

/* Return the current value on the clock, in milliseconds. */
long
boz_clock_value(boz_clock clock);

/* Set an arbitrary pointer which will be passed to the event callbacks as the
 * parameter "cookie". This is specific to this clock, and is independent of
 * the event cookie for general button-pressing events as set by
 * boz_set_event_cookie(). */
void
boz_clock_set_event_cookie(boz_clock clock, void *cookie);

/* boz_clock_set_alarm
 * Set the clock to call the callback function when the clock's value has
 * reached or passed alarm_value_ms. ("passed" takes account of which direction
 * the clock is running in.)
 * This overrides any previously-set alarm on this clock.
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
 * the handler is detached, and, if callback is not NULL, callback() is called,
 * passing the cookie (see boz_clock_set_event_cookie()) and the pointer to the
 * clock.
 */
void
boz_clock_set_expiry_max(boz_clock clock, long max_ms,
        void (*callback)(void *, boz_clock));

/* boz_clock_set_expiry_min
 * Set the minimum value for the clock, and optionally attach an event handler
 * to the event of the clock's value reaching this value.
 * When the clock's value reaches or falls below min_ms, the clock is stopped,
 * the handler is detached, and, if callback is not NULL, callback() is called,
 * passing the cookie (see boz_clock_set_event_cookie()) and the pointer to the
 * clock.
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
 * Returns 1 if the clock's direction is forwards, 0 if it's backwards.
 */
int
boz_clock_is_direction_forwards(boz_clock clock);

unsigned long
boz_clock_get_ms_until_alarm(boz_clock clock);

void
boz_clock_add(boz_clock clock, long ms_to_add);

#endif
