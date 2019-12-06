#ifndef _CRASH_H
#define _CRASH_H

struct boz_crash_arg {
    void *addr;
    unsigned int pattern;
    byte light_state;
};

#endif
