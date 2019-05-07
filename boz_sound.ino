#include "boz.h"
#include "boz_sound.h"
#include "boz_notes.h"

int
boz_sound_silence(unsigned int duration_ms) {
    return boz_sound_enqueue(0, 0, NULL, 0, duration_ms, 1);
}

int
boz_sound_note(boz_note note, unsigned int duration_ms) {
    unsigned int freq = boz_note_to_freq(note);
    return boz_sound_enqueue(freq, freq, NULL, 0, duration_ms, 1);
}

int
boz_sound_varying(boz_note note_start, boz_note note_end, unsigned int duration_ms, byte num_times) {
    unsigned int freq_start = boz_note_to_freq(note_start);
    unsigned int freq_end = boz_note_to_freq(note_end);
    if (num_times < 1)
        num_times = 1;

    return boz_sound_enqueue(freq_start, freq_end, NULL, 0, duration_ms, num_times);
}

int
boz_sound_arpeggio(byte *notes, int num_notes, int duration_ms, byte num_times) {
    if (num_times < 1)
        num_times = 1;
    return boz_sound_enqueue(0, 0, notes, num_notes, duration_ms, num_times);
}
