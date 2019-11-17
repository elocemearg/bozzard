#ifndef _BOZ_SOUND_H
#define _BOZ_SOUND_H

#include "boz_api.h"
#include "boz_notes.h"

/* Applications should not need to use boz_sound_enqueue() directly - instead,
 * use one of the helper functions declared in boz_api.h. */
int
boz_sound_enqueue(unsigned int freq_start, unsigned int freq_end, unsigned int duration_ms);

#endif
