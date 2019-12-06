#ifndef _BOZ_UTIL_H
#define _BOZ_UTIL_H

int
time_passed(unsigned long now, unsigned long when);

int
time_passed_aux(unsigned long now, unsigned long when, int dir_fwd);

unsigned long
time_elapsed(unsigned long before, unsigned long after);

#endif
