#ifndef _BOZ_SOUND_H
#define _BOZ_SOUND_H

#include "boz_notes.h"

/* These operations add sound commands to the sound commands queue, which has
 * space for 32 commands. As commands are completed, they come off the queue.
 * If you try to enqueue too many sound commands at once, the queue will fill
 * up and won't accept any more commands. If this happens, the boz_sound_*
 * call will return -1. */

/* Enqueue a command to play nothing for the given duration. */
int
boz_sound_silence(unsigned int duration_ms);

/* Enqueue a command to play the named note for the given duration. */
int
boz_sound_note(boz_note note, unsigned int duration_ms);

/* Enqueue a command to start playing note_start, and then change the frequency
 * to note_end over duration_ms milliseconds, num_times times. */
int
boz_sound_varying(boz_note note_start, boz_note note_end, unsigned int duration_ms, byte num_times);

/* Enqueue a command to play an arpeggio num_times times. The notes of the
 * arpeggio are indexes into the note table, and notes must point to a buffer
 * of num_notes notes. num_notes must be between 1 and 4 inclusive.
 * One play of the arpeggio will take duration_ms milliseconds. */
int
boz_sound_arpeggio(byte *notes, int num_notes, int duration_ms, byte num_times);

/* Interrupt the currently-playing sound command. The main loop will stop
 * playing whatever it's playing and move on to the next sound command in
 * the queue. */
void
boz_sound_stop(void);

/* Like sound_stop(), but also removes all sound commands from the queue. */
void
boz_sound_stop_all(void);

#endif
