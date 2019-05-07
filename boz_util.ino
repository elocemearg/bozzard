#include "boz_util.h"

int
time_passed_aux(unsigned long now, unsigned long when, int dir_fwd) {
    if (!dir_fwd) {
        return time_passed_aux(when, now, 1);
    }
    if (now >= when) {
        /* |-------W---------------------------N--|
         * NOW has not yet reached WHEN
         *
         * |-------W------N-----------------------|
         * NOW has passed WHEN
         */
        return (now - when) < (1UL << (sizeof(unsigned long) * 8 - 1));
    }
    else {
        /* |-------N------W-----------------------|
         * NOW has not yet reached WHEN
         *
         * |-------N---------------------------W--|
         * NOW has passed WHEN
         */
        return (when - now) > (1UL << (sizeof(unsigned long) * 8 - 1));
    }
}

/* If now is after when, return 1, else return 0.
   If now is more than 2^31 milliseconds after when, you'll get the wrong
   answer. */
int
time_passed(unsigned long now, unsigned long when) {
    return time_passed_aux(now, when, 1);
}

/* Given that "before" is no later than "after", return the number of time
   units separating the two points in time. */
unsigned long
time_elapsed(unsigned long before, unsigned long after) {
    if (after >= before) {
        return after - before;
    }
    else {
        /* |------------------------------------------|
           0   A                                   B  ~0UL
        */
        return (~(0UL) - before) + after + 1;
    }
}
