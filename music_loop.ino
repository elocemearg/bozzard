#include "boz.h"
#include "boz_notes.h"

#include <avr/pgmspace.h>

/* A "melodeme" is one component of a melody, in the same way that a lexeme is
   one component of a language or a grapheme is one component of writing.
   It can be a note or a rest.
   Somehow this word has not caught on.
*/

struct melodeme {
    /* One of the notes defined in boz_note.h */
    byte note;

    /* Duration reciprocal: the note is 1 / dur_recip of a four-beat bar.
       So 1 is a whole note, 2 is a minim, 4 is a crotchet, etc.
       The special value 0 is a 1/16 note (semiquaver)
    */
    unsigned int dur_recip : 4;
    
    /* If dur_dot is 1, it indicates that this note is "dotted" - that is, it
       has 1.5 times its normal duration. So if dur_recip is 4 and dur_dot is
       1, then the note lasts for 1.5 beats.
     */
    unsigned int dur_dot : 1;

    /* Continuously glide up or down to the next note.
       I've deliberately made this have no effect because it sounds like
       someone stirring a bucket of marbles. */
    unsigned int portamento : 1;
};

const PROGMEM struct melodeme spanish_flea[] = {
    { NOTE_C4, 8 },
    { NOTE_REST, 8 },
    { NOTE_G4, 8 },
    { NOTE_G4, 8 },
    { NOTE_G3, 8 },
    { NOTE_REST, 8 },
    { NOTE_G4, 8 },
    { NOTE_REST, 8 },

    { NOTE_C4, 8 },
    { NOTE_REST, 8 },
    { NOTE_G4, 8 },
    { NOTE_G3, 8 },
    { NOTE_REST, 8 },
    { NOTE_G3, 8 },
    { NOTE_G4, 8 },
    { NOTE_REST, 8 },
    
    { NOTE_C4, 8 },
    { NOTE_REST, 8 },
    { NOTE_G4, 8 },
    { NOTE_G4, 8 },
    { NOTE_G3, 8 },
    { NOTE_REST, 8 },
    { NOTE_G4, 8 },
    { NOTE_REST, 8 },

    { NOTE_C4, 8 },
    { NOTE_REST, 2 },
    { NOTE_E4, 8 },
    { NOTE_F4, 8 },
    { NOTE_Fs4, 8 },

    { NOTE_G4, 8 },
    { NOTE_A2, 8 }, //{ NOTE_REST, 8 },
    { NOTE_E5, 8 },
    { NOTE_F3, 8 }, // { NOTE_REST, 8 },
    { NOTE_E5, 8 },
    { NOTE_D5, 4 },
    { NOTE_Cs5, 4, 1 },

    { NOTE_REST, 4, 1 },
    { NOTE_A4, 8, 0, 1 },
    { NOTE_Gs4, 8 },
    { NOTE_G4, 8 },

    { NOTE_Fs4, 8 },
    { NOTE_REST, 8 },
    { NOTE_D5, 8 },
    { NOTE_C4, 8 },
    { NOTE_D5, 8 },
    { NOTE_C5, 4 },
    { NOTE_B4, 4, 1 },

    { NOTE_REST, 4, 1 },
    { NOTE_G4, 8, 0, 1 },
    { NOTE_Fs4, 8 },
    { NOTE_F4, 8 },
    
    { NOTE_E4, 8 },
    { NOTE_G4, 8 },
    { NOTE_C5, 8 },
    { NOTE_A4, 4 },
    { NOTE_C5, 8 }, // 32
    { NOTE_D5, 4, 0, 1 },
    
    { NOTE_G4, 8, 0, 1 },
    { NOTE_As4, 8 },
    { NOTE_Ds5, 8 },
    { NOTE_C5, 4 },
    { NOTE_Ds5, 8 },
    { NOTE_F5, 4 },
    
    { NOTE_G5, 1 },

    { NOTE_REST, 4 },
    { NOTE_G5, 8 },
    { NOTE_G5, 8 },
    { NOTE_A5, 8 },
    { NOTE_G5, 8 },
    { NOTE_Ds5, 0, 0, 1 }, // semiquaver
    { NOTE_D5, 8, 1 },

    { NOTE_C5, 8 },
    { NOTE_REST, 8 },
    { NOTE_E5, 8 },
    { NOTE_E4, 8 },
    { NOTE_E5, 8 },
    { NOTE_D5, 4 },
    { NOTE_Cs5, 4, 1 },

    { NOTE_REST, 4, 1 },
    { NOTE_A4, 8, 0, 1 },
    { NOTE_Gs4, 8 },
    { NOTE_G4, 8 },

    { NOTE_Fs4, 8 },
    { NOTE_REST, 8 },
    { NOTE_D5, 8 },
    { NOTE_C4, 8 },
    { NOTE_D5, 8 },
    { NOTE_C5, 4 },
    { NOTE_B4, 4, 1 },
    
    { NOTE_REST, 4, 1 },
    { NOTE_G4, 8, 0, 1 },
    { NOTE_Fs4, 8 },
    { NOTE_F4, 8 },
    
    { NOTE_E4, 8 },
    { NOTE_G4, 8 },
    { NOTE_C5, 8 },
    { NOTE_A4, 4 },
    { NOTE_C5, 8 },
    { NOTE_D5, 4, 0, 1 },

    { NOTE_G4, 8, 0, 1 },
    { NOTE_As4, 8 },
    { NOTE_Ds5, 8 },
    { NOTE_C5, 4 },
    { NOTE_Ds5, 8 },
    { NOTE_F5, 4 },

    { NOTE_G5, 1 },

    { NOTE_REST, 4 },
    { NOTE_G5, 8 },
    { NOTE_G5, 8 },
    { NOTE_A5, 8 },
    { NOTE_G5, 8 },
    { NOTE_Ds5, 0, 0, 1 },
    { NOTE_D5, 8, 1 },

/*    { NOTE_C5, 8 },
    { NOTE_REST, 8 },
    { NOTE_REST, 4 },
    { NOTE_REST, 2 },

    { NOTE_REST, 1 },

    { NOTE_REST, 1 },

    { NOTE_REST, 4 },
    { NOTE_C5, 4 },
    { NOTE_D5, 4 },
    { NOTE_E5, 4 },

    { NOTE_F5, 8 },
    { NOTE_REST, 2 },
    { NOTE_F5, 8, 0, 1 },
    { NOTE_G5, 8 },
    { NOTE_F5, 8 },
    
    { NOTE_A5, 8 },
    { NOTE_G5, 4 },
    { NOTE_F5, 4 },
    { NOTE_Ds5, 8 },
    { NOTE_D5, 8 },
    { NOTE_C5, 8 },

    { NOTE_As4, 4 },
    { NOTE_As4, 4 },
    { NOTE_C5, 8 },
    { NOTE_Cs5, 4 },
    { NOTE_D5, 8 },

    { NOTE_D5, 2 },
    { NOTE_REST, 8 },
    { NOTE_As4, 8 },
    { NOTE_A4, 8 },
    { NOTE_Gs4, 8 },
    
    { NOTE_G4, 8 },
    { NOTE_Ds5, 8 },
    { NOTE_Ds5, 4 },
    { NOTE_Ds5, 8, 0, 1 },
    { NOTE_F5, 8 },
    { NOTE_Ds5, 8 },
    
    { NOTE_G5, 8 },
    { NOTE_F5, 4 },
    { NOTE_Ds5, 4 },
    { NOTE_Cs5, 8 },
    { NOTE_C5, 8 },
    { NOTE_As4, 8 },
    */
};

/* Set up the ml_melody_* variables for Spanish Flea */
const struct melodeme *ml_melody = spanish_flea;
int ml_melody_length = sizeof(spanish_flea) / sizeof(struct melodeme);
int ml_melody_bpm = 160;
int ml_melody_pos = 0;
int ml_melody_beats_per_bar = 4;

int ml_beat = 0;

const PROGMEM byte treble_clef_top[] = {
    0x02, 0x05, 0x05, 0x05, 0x05, 0x06, 0x0c, 0x14
};
const PROGMEM byte treble_clef_bottom[] = {
    0x16, 0x1d, 0x15, 0x15, 0x0e, 0x04, 0x14, 0x08
};
const PROGMEM byte stave_top[] = {
    0x00, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f
};
const PROGMEM byte stave_bottom[] = {
    0x00, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00
};
const PROGMEM byte stave_start_top[] = {
    0x00, 0x1f, 0x10, 0x10, 0x1f, 0x10, 0x10, 0x1f
};
const PROGMEM byte stave_start_bottom[] = {
    0x10, 0x1f, 0x10, 0x10, 0x1f, 0x00, 0x00, 0x00
};

const PROGMEM byte stave_filled_top[] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
const PROGMEM byte stave_filled_bottom[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00
};

void
ml_set_cgram_char(int address, const byte *data) {
    boz_display_set_cgram_address(address << 3);
    for (int i = 0; i < 8; ++i) {
        boz_display_write_char(pgm_read_byte_near(data + i));
    }
}

void
music_loop_init(void *dummy) {
    boz_display_clear();
    boz_set_event_handler_qm_reset(music_loop_reset);
    boz_set_event_handler_qm_yellow(music_loop_yellow);
    boz_set_event_handler_qm_play(music_loop_play);

    ml_set_cgram_char(0, treble_clef_top);
    ml_set_cgram_char(1, treble_clef_bottom);
    ml_set_cgram_char(2, stave_top);
    ml_set_cgram_char(3, stave_bottom);
    ml_set_cgram_char(4, stave_start_top);
    ml_set_cgram_char(5, stave_start_bottom);
    ml_set_cgram_char(6, stave_filled_top);
    ml_set_cgram_char(7, stave_filled_bottom);

    boz_display_set_cursor(0, 0);
    boz_display_write_char(0);
    boz_display_write_long(ml_melody_beats_per_bar, 2, NULL);
    boz_display_write_char(' ');
    boz_display_write_char(4);
    for (int i = 5; i < 16; ++i)
        boz_display_write_char(2);

    boz_display_set_cursor(1, 0);
    boz_display_write_char(1);
    boz_display_write_long(4, 2, NULL);
    boz_display_write_char(' ');
    boz_display_write_char(5);
    for (int i = 5; i < 16; ++i)
        boz_display_write_char(3);

    ml_melody_pos = 0;

    music_loop_yellow(NULL);
}

void
music_loop_sound_queue_ready(void *cookie) {
    int queue_ret;

    do {
        struct melodeme m, next_m;
        unsigned int duration_ms;
        int dur_recip;
        int porta = 0;
        memcpy_P(&m, &ml_melody[ml_melody_pos], sizeof(m));

        dur_recip = m.dur_recip;
        if (dur_recip == 0)
            dur_recip = 16;

        // beats = 4 / dur_recip
        // minutes = beats / bpm
        // milliseconds = minutes * 60,000
        // milliseconds = beats * 60000 / bpm
        // milliseconds = (4 / dur_recip) * (60000 / bpm)
        //              = 240000 / (dur_recip * bpm)

        duration_ms = (unsigned int) (240000UL / (((long) dur_recip) * ml_melody_bpm));
        if (m.dur_dot)
            duration_ms = (duration_ms * 3) / 2;

        if (m.portamento) {
            int next_index = ml_melody_pos + 1;
            if (next_index >= ml_melody_length)
                next_index = 0;
            memcpy_P(&next_m, &ml_melody[next_index], sizeof(next_m));
            if (next_m.note != NOTE_REST) {
                porta = 1;
            }
        }

        // Portamento on this device sounds utterly awful
        porta = 0;

        if (porta) {
            queue_ret = boz_sound_varying((boz_note) m.note, (boz_note) next_m.note, duration_ms, 1);
        }
        else if (m.note == NOTE_REST) {
            queue_ret = boz_sound_silence(duration_ms);
        }
        else {
            queue_ret = boz_sound_note((boz_note) m.note, duration_ms);
        }

        if (queue_ret == 0) {
            /* Sound was successfully enqueued */
            ml_melody_pos++;
            if (ml_melody_pos >= ml_melody_length)
                ml_melody_pos = 0;

        }
    } while (queue_ret == 0);

    /* We filled up the queue. Ask the main loop to tell us when the queue
       is unfull again. */
    boz_set_event_handler_sound_queue_not_full(music_loop_sound_queue_ready);
}

void
music_loop_reset(void *cookie) {
    boz_sound_stop_all();
    boz_set_event_handler_sound_queue_not_full(NULL);
    boz_app_exit(0);
}

void
ml_beat_handler(void *cookie) {
    /* Display a load of rubbish vaguely in time with the beat */
    int cells_filled;
    ml_beat++;
    if (ml_beat >= ml_melody_beats_per_bar)
        ml_beat = 0;
    if (ml_melody_beats_per_bar > 1) {
        cells_filled = 12 * ml_beat / (ml_melody_beats_per_bar - 1);
        for (int row = 0; row < 2; ++row) {
            boz_display_set_cursor(row, 4);
            for (int cell = 0; cell < 12; ++cell) {
                if (cell < cells_filled)
                    boz_display_write_char(row ? 7 : 6);
                else if (cell == 0)
                    boz_display_write_char(row ? 5 : 4);
                else
                    boz_display_write_char(row ? 3 : 2);
            }
        }
    }
    boz_set_alarm(60000L / ml_melody_bpm, ml_beat_handler, NULL);
}

void
music_loop_yellow(void *cookie) {
    boz_sound_stop_all();
    ml_melody_pos = 0;
    ml_beat = 0;
    boz_set_event_handler_sound_queue_not_full(music_loop_sound_queue_ready);
    
    /* First alarm is 100ms early, to give time for the display to change */
    long alarm_ms = 60000L / ml_melody_bpm - 100;
    if (alarm_ms < 0)
        alarm_ms = 0;
    boz_set_alarm(alarm_ms, ml_beat_handler, NULL);
}

void
music_loop_play(void *cookie) {
    boz_sound_stop_all();
    ml_melody_pos = 0;
    ml_beat = 0;
    boz_set_event_handler_sound_queue_not_full(NULL);
    boz_cancel_alarm();
}
