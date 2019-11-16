#include "boz_clock.h"
#include "boz_util.h"

const unsigned int FORWARDS = 1;
const unsigned int BACKWARDS = 0;

void
boz_clock_init(boz_clock clock, int id, long initial_value_ms, int direction_forwards) {
    memset(clock, 0, sizeof(*clock));
    clock->initial_value_ms = initial_value_ms;
    clock->id = id;

    clock->last_value_ms = initial_value_ms;
    clock->direction = direction_forwards ? FORWARDS : BACKWARDS;

    clock->event_cookie = NULL;
    clock->event_expiry_min = NULL;
    clock->event_expiry_max = NULL;
    clock->event_alarm = NULL;
}

void
boz_clock_run(boz_clock clock) {
    if (!clock->running) {
        clock->running = 1;
        clock->last_value_ard_millis = millis();
    }
}

int
boz_clock_running(boz_clock clock) {
    return clock->running;
}

int
boz_clock_is_direction_forwards(boz_clock clock) {
    return clock->direction == FORWARDS;
}

void
boz_clock_stop(boz_clock clock) {
    if (clock->running) {
        unsigned long ms = millis();
        clock->last_value_ms = boz_clock_value_at(clock, ms);
        clock->running = 0;
        clock->last_value_ard_millis = ms;
    }
}

void
boz_clock_reset(boz_clock clock) {
    clock->last_value_ms = clock->initial_value_ms;
    clock->last_value_ard_millis = millis();
}

void
boz_clock_set_direction(boz_clock clock, int direction_forwards) {
    /* Update last_value_ms to the current value */
    unsigned long now_ms = millis();
    clock->last_value_ms = boz_clock_value_at(clock, now_ms);
    clock->last_value_ard_millis = now_ms;

    /* Change direction */
    clock->direction = direction_forwards ? FORWARDS : BACKWARDS;
}

long
boz_clock_value_at(boz_clock clock, unsigned long now_ms_ard) {
    if (!clock->running) {
        return clock->last_value_ms;
    }
    else {
        long value_ms = clock->last_value_ms;
        if (clock->direction == FORWARDS) {
            value_ms += time_elapsed(clock->last_value_ard_millis, now_ms_ard);
        }
        else {
            value_ms -= time_elapsed(clock->last_value_ard_millis, now_ms_ard);
        }

        if (clock->min_enabled && value_ms < clock->min_ms)
            return clock->min_ms;
        else if (clock->max_enabled && value_ms > clock->max_ms)
            return clock->max_ms;
        else
            return value_ms;
    }
}

long
boz_clock_value(boz_clock clock) {
    return boz_clock_value_at(clock, millis());
}

void
boz_clock_set_event_cookie(boz_clock clock, void *cookie) {
    clock->event_cookie = cookie;
}

void
boz_clock_set_alarm(boz_clock clock, long alarm_value_ms,
        void (*callback)(void *, boz_clock)) {
    clock->alarm_ms = alarm_value_ms;
    clock->alarm_enabled = 1;
    clock->event_alarm = callback;
}

void
boz_clock_cancel_alarm(boz_clock clock) {
    clock->alarm_enabled = 0;
}

void
boz_clock_set_expiry_max(boz_clock clock, long max_ms,
        void (*callback)(void *, boz_clock)) {
    clock->max_ms = max_ms;
    clock->max_enabled = 1;
    clock->event_expiry_max = callback;
}

void
boz_clock_set_expiry_min(boz_clock clock, long min_ms,
        void (*callback)(void *, boz_clock)) {
    clock->min_ms = min_ms;
    clock->min_enabled = 1;
    clock->event_expiry_min = callback;
}

void
boz_clock_cancel_expiry_min(boz_clock clock) {
    clock->min_enabled = 0;
}

void
boz_clock_cancel_expiry_max(boz_clock clock) {
    clock->max_enabled = 0;
}

void
boz_clock_set_initial_value(boz_clock clock, long value) {
    clock->initial_value_ms = value;
}

unsigned long
boz_clock_get_ms_until_alarm(boz_clock clock) {
    if (clock->alarm_enabled) {
        long value = boz_clock_value(clock);
        if (clock->direction == FORWARDS) {
            if (value >= clock->alarm_ms) {
                return 0;
            }
            else {
                return clock->alarm_ms - value;
            }
        }
        else {
            if (value <= clock->alarm_ms) {
                return 0;
            }
            else {
                return value - clock->alarm_ms;
            }
        }
    }
    else {
        return (unsigned long) -1;
    }
}

void
boz_clock_add(boz_clock clock, long ms_to_add) {
    long mil = millis();
    clock->last_value_ms = boz_clock_value_at(clock, mil) + ms_to_add;
    clock->last_value_ard_millis = mil;
}
