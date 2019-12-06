#ifndef _BOZ_CLOCK_H
#define _BOZ_CLOCK_H

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

#endif
