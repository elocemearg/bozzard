#include "boz_notes.h"
#include <avr/pgmspace.h>

/* The indexes into this array correspond with the piano key number. We
   can't play notes less than 31Hz, so we start at key 4, C1 (33Hz). */
const PROGMEM unsigned int notes_to_freqs[] = {
    0, // 0
    0, // 1
    0, // 2
    0, // 3
    33,
    35,
    37,
    39,
    41,
    44,
    46, // 10
    49,
    52,
    55,
    58,
    62,
    65,
    69,
    73,
    78,
    82, // 20
    87,
    92,
    98,
    104,
    110,
    117,
    123,
    131,
    139,
    147, // 30
    156,
    165,
    175,
    185,
    196,
    208,
    220,
    233,
    247,
    262, // 40
    277,
    294,
    311,
    330,
    349,
    370,
    392,
    415,
    440,
    466, // 50
    494,
    523,
    554,
    587,
    622,
    659,
    698,
    740,
    784,
    831, // 60
    880,
    932,
    988,
    1047,
    1109,
    1175,
    1245,
    1319,
    1397,
    1480, // 70
    1568,
    1661,
    1760,
    1865,
    1976,
    2093,
    2217,
    2349,
    2489,
    2637, // 80
    2794,
    2960,
    3136,
    3322,
    3520,
    3729,
    3951,
    4186,
    4435,
    4699, // 90
    4978,
    5274,
    5588,
    5920,
    6272,
    6645,
    7040,
    7459,
    7902,
};

unsigned int
boz_note_to_freq(byte note) {
    return (unsigned int) pgm_read_word_near(notes_to_freqs + note);
}
