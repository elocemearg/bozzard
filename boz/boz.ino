#include "boz_api.h"
#include "boz_util.h"
#include "boz_queue.h"
#include "boz_shiftreg.h"
#include "boz_lcd.h"
#include "boz_app.h"
#include "boz_pins.h"
#include "boz_notes.h"
#include "boz_crash.h"

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <EEPROM.h>

#include "boz_app_inits.h"


#define BOZ_VERSION_MAJOR   1L
#define BOZ_VERSION_MINOR   0L
#define BOZ_VERSION_RELEASE 0L

const long BOZ_VERSION = ((BOZ_VERSION_MAJOR << 24) |
            (BOZ_VERSION_MINOR << 16) |
            (BOZ_VERSION_RELEASE << 8));

/* If the start of EEPROM doesn't contain this when we turn the unit on, we'll
   write it there and fill the rest of EEPROM with 0xff to initialise it. */
struct boz_eeprom_header {
    char id[8];
    long version;
};
const PROGMEM struct boz_eeprom_header boz_eeprom_header = {
    "BOZZARD",
    BOZ_VERSION
};

#define SND_CMD_QUEUE_SIZE 16
#define DISP_CMD_QUEUE_SIZE 96
#define APP_CONTEXT_STACK_SIZE 4
#define NUM_CLOCKS BOZ_NUM_CLOCKS // must be less than the number of bits in an int
#define BOZ_DYN_ARENA_SIZE 512
//#define BOZ_SOUND_RAMP_LOG // better sound-ramp effect, but +2KB code size

/* A single sound command, which is put on the sound queue by an app, and is
   serviced by the main loop. */
struct snd_cmd {
    union {
        struct {
            unsigned short freq_start;
            unsigned short freq_end;
        };
        byte arp_notes[4];
    };

    /* How long this sound should last */
    unsigned short duration_ms;

    /* Number of times to play this sound */
    byte num_times;

    /* If is_arpeggio, we play the (up to) four notes mentioned in arp_notes[].
       Otherwise we play freq_start Hz, moving to freq_end Hz over duration_ms
       milliseconds. */
    unsigned int is_arpeggio : 1;
    
    /* One less than the number of notes in arp_notes[] */
    unsigned int arp_notes_max_index : 2;
};

/* A single display command, which is put on the display command queue by an
   app, and is serviced by the main loop. */
struct disp_cmd {
    unsigned short cmd;
};

struct snd_cmd_queue {
    struct snd_cmd q[SND_CMD_QUEUE_SIZE];
    struct queue_state qstate;
};

struct snd_cmd_state {
    struct snd_cmd cmd;
    unsigned long start_millis;      // millis() when sound started
    unsigned long next_step_millis;  // millis() time of next freq step
    unsigned short current_freq;
    unsigned short times_done;
    byte running;                    // true if command is in progress
    byte arp_index;                  // which note of an arpeggio we're on
};

struct disp_cmd_queue {
    struct disp_cmd q[DISP_CMD_QUEUE_SIZE];
    struct queue_state qstate;
};

struct disp_cmd_state {
    struct disp_cmd cmd;
    unsigned short state;
    unsigned long next_step_micros;  // micros() time of next freq step
    unsigned char running;
};

struct button_state {
    /* Pin for this button */
    byte pin;

    /* Function of this button, e.g. FUNC_BUZZER or FUNC_PLAY */
    byte button_function;

    /* If button_function is FUNC_BUZZER, which buzzer it is: 0 to 3 */
    byte buzzer_id;

    /* Number of microseconds we must have a continuous signal on a button's
       pin before we consider that button pressed. */
    unsigned int press_threshold_us;

    /* Number of microseconds we must have a continuous no-signal on a button's
       pin before we consider that button released, after which point a new
       signal will be interpreted as a new press. */
    unsigned int release_threshold_us;

    /* If is_pressed, pressed_since_micros is the value of micros() since when
       the button has been continuously pressed.
       If not is_pressed, released_since_micros is the value of micros()
       since when the button has been continuously not pressed. */
    unsigned long pressed_since_micros, released_since_micros;

    /* 1 if the button was pressed last time we looked, 0 if not. */
    unsigned int is_pressed : 1;
    
    /* If the button was pressed for long enough that we considered it a
       valid press, and we called the application's handler to tell it about
       the press event or we would have called it but there was no handler
       set, event_delivered is 1, otherwise it's 0. */
    unsigned int event_delivered : 1;

    /* The input pins have internal pull-up resistors, so are HIGH by
       default. A button connected between it and ground will pull it
       low when pressed. So HIGH means the button is not pressed, and
       LOW means the button is pressed. */
    unsigned int active_low : 1;
};

/* The sound and display commands queue. */
struct snd_cmd_queue snd_cmd_queue;
struct disp_cmd_queue disp_cmd_queue;

/* The sound and display command currently being executed. Sound and display
   commands often involve an element of waiting around (especially sound
   commands). snd_cmd_state and disp_cmd_state contain the command that was
   last taken off the queue and which is still executing. */
struct snd_cmd_state snd_cmd_state;
struct disp_cmd_state disp_cmd_state;

/* General-purpose clocks for use by applications. There are NUM_CLOCKS clocks,
   to be shared between all applications currently in memory. "clocks" is an
   array of boz_clocks whose memory has been dynamically allocated by
   boz_mm_main_alloc. boz_clock is a pointer type. */
boz_clock clocks[NUM_CLOCKS];
unsigned int master_clocks_enabled = 0; // bitmask

/* Press threshold: the number of microseconds we must sense a button held down
   for before we consider it a press event.
   Release threshold: the number of microseconds we must sense the button
   continuously released before we accept another press event.
*/
const unsigned int BUTTON_PRESS_THRESHOLD_US = 0;
const unsigned int BUTTON_RELEASE_THRESHOLD_US = 15000;
const unsigned int ROTARY_CLOCK_PRESS_THRESHOLD_US = 0;
const unsigned int ROTARY_CLOCK_RELEASE_THRESHOLD_US = 5000;

/* If the number of milliseconds between two clock pulses from the rotary
   encoder (with the data pin indicating the same direction) is less than
   this number of milliseconds, consider the second pulse to be accidental
   and disregard it. */
const byte ROTARY_TURN_MIN_GAP_MS = 15;

/* Next time we check all the buttons to see which one, if any, was pressed,
   this tells is which button we check first and which way we iterate through
   the list. This is varied to ensure fairness in the unlikely event that two
   buzzers are pressed simultaneously. */
char button_check_start = 0;
char button_check_direction = 1;

/* The state of each of the buttons (se struct button_state). Here we store
   the current state of the buzzer buttons, the play, yellow and reset buttons,
   the rotary encoder's pushbutton, and the rotary encoder's clock input. */
struct button_state buttons[] = {
    { PIN_BUZZER_0,  FUNC_BUZZER, 0, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 0, 0, 0, 0, 1 },
    { PIN_BUZZER_1,  FUNC_BUZZER, 1, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 1, 0, 0, 0, 1 },
    { PIN_BUZZER_2,  FUNC_BUZZER, 2, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 2, 0, 0, 0, 1 },
    { PIN_BUZZER_3,  FUNC_BUZZER, 3, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 3, 0, 0, 0, 1 },
    { PIN_QM_PLAY,   FUNC_PLAY,   0, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 0, 0, 0, 0, 1 },
    { PIN_QM_YELLOW, FUNC_YELLOW, 0, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 0, 0, 0, 0, 1 },
    { PIN_QM_RESET,  FUNC_RESET,  0, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 0, 0, 0, 0, 1 },
    { PIN_QM_RE_KEY, FUNC_RE_KEY, 0, BUTTON_PRESS_THRESHOLD_US, BUTTON_RELEASE_THRESHOLD_US, 0, 0, 0, 0, 1 },
    { PIN_QM_RE_CLOCK, FUNC_RE_CLOCK, 0, ROTARY_CLOCK_PRESS_THRESHOLD_US, ROTARY_CLOCK_RELEASE_THRESHOLD_US, 0, 0, 0, 0, 0 },
};
const int num_buttons = sizeof(buttons) / sizeof(buttons[0]);
#define BOZ_FIRST_BUZZER_BUTTON_INDEX 0
#define BOZ_LAST_BUZZER_BUTTON_INDEX 3

int re_data_value_last_clock = LOW;
unsigned long re_last_turn_high_ms = 0;
unsigned long re_last_turn_low_ms = 0;

/* A pointer to dynamic memory allocated by boz_mm_main_alloc, this contains
   the stack of app_context structs. There is one for each running app. When
   an app calls another app, the new app context gets pushed onto the stack,
   and when the called app exits, its context is taken off the stack again. */
struct app_context *app_context_stack;

/* pointer to current app context */
struct app_context *app_context = NULL;

/* dynamic memory pool for boz_mm_* functions */
char boz_dyn_arena[BOZ_DYN_ARENA_SIZE];

/* If an app calls another app, the app's init function, flags etc go in
   app_to_call_data, and app_call_defer points to it. We set app_call_defer
   back to NULL, and transfer control to the called app. */
const struct boz_app *app_call_defer = NULL;
struct boz_app app_to_call_data;
void *app_call_defer_init_cookie = NULL;
void (*app_call_return)(void *, int) = NULL;
void *app_call_return_cookie = NULL;

/* Set to 1 by boz_app_exit(). When the main loop regains control, we will
   take care of tearing down the just-exited app and passing its exit status
   to the calling app. */
byte app_exited = 0;
int app_exit_status = 0;

#ifdef BOZ_SERIAL
volatile byte serial_data_available = 0;
#endif

static void memzero(void *p, size_t n) {
    unsigned char *pc = (unsigned char *) p;
    while (n > 0) {
        *pc = 0;
        ++pc;
        --n;
    }
}

long
boz_get_version(void) {
    return BOZ_VERSION;
}

boz_clock 
boz_clock_create(long initial_value_ms, int direction_forwards) {
    int which_clock;
    boz_clock clock;

    for (which_clock = 0; which_clock < NUM_CLOCKS; ++which_clock) {
        if ((master_clocks_enabled & (1 << which_clock)) == 0) {
            break;
        }
    }
    if (which_clock >= NUM_CLOCKS) {
        // they're all being used
        return NULL;
    }

    clock = (boz_clock) boz_mm_main_alloc(sizeof(*clock));
    if (clock == NULL) {
        /* Not enough memory */
        return NULL;
    }
    clocks[which_clock] = clock;

    master_clocks_enabled |= (1 << which_clock);
    if (app_context)
        app_context->clocks_enabled |= (1 << which_clock);
    boz_clock_init(clock, which_clock, initial_value_ms, direction_forwards);
    return clock;
}

void
boz_clock_release(boz_clock clock) {
    unsigned int which_clock = clock->id;
    
    boz_clock_stop(clock);
    boz_clock_cancel_alarm(clock);
    boz_clock_cancel_expiry_min(clock);
    boz_clock_cancel_expiry_max(clock);
    if (app_context)
        app_context->clocks_enabled &= ~(1 << which_clock);
    master_clocks_enabled &= ~(1 << which_clock);
    boz_mm_main_free(clock);
    clocks[which_clock] = NULL;
}

int
boz_sound_enqueue(unsigned int freq_start, unsigned int freq_end,
        byte *arp_notes, int num_arp_notes, unsigned int duration_ms,
        byte num_times) {
    struct snd_cmd cmd;
    
    if (arp_notes != NULL && num_arp_notes > 0) {
        if (num_arp_notes > 4)
            num_arp_notes = 4;
        for (int i = 0; i < num_arp_notes; ++i) {
            cmd.arp_notes[i] = arp_notes[i];
        }
        cmd.is_arpeggio = 1;
        cmd.arp_notes_max_index = num_arp_notes - 1;
    }
    else {
        cmd.is_arpeggio = 0;
        cmd.freq_start = freq_start;
        cmd.freq_end = freq_end;
    }
    cmd.duration_ms = duration_ms;
    cmd.num_times = num_times;

    return queue_add(snd_cmd_queue.q,
            sizeof(snd_cmd_queue.q) / sizeof(snd_cmd_queue.q[0]),
            sizeof(snd_cmd_queue.q[0]), &snd_cmd_queue.qstate, &cmd);
}

void
boz_sound_stop(void) {
    /* Stop the currently-playing sound command */
    snd_cmd_state.running = 0;
    noTone(PIN_SPEAKER);
}

void
boz_sound_stop_all(void) {
    /* Stop the currently-playing sound command and throw away all queued
       sound commands */
    boz_sound_stop();
    queue_clear(&snd_cmd_queue.qstate);
}

#if BOZ_HW_REVISION == 0
void
boz_led_set(int which_led, int on) {
    /* which_led must be between 0 and 3 */
    if (which_led < 0 || which_led > 3)
        return;
    if (on) {
        boz_shift_reg_bit_set(which_led);
    }
    else {
        boz_shift_reg_bit_clear(which_led);
    }
}

void
boz_leds_set(int mask) {
    boz_shift_reg_set_bottom_nibble((byte) (mask & 0x0f));
}
#else

const PROGMEM byte leds_to_pins[] = { PIN_LED_R, PIN_LED_G, PIN_LED_Y, PIN_LED_B };

void
boz_led_set(int which_led, int on) {
    int pin = pgm_read_byte_near(&leds_to_pins[which_led & 3]);
    digitalWrite(pin, on ? HIGH : LOW);
}

void
boz_leds_set(int mask) {
    for (int i = 0; i < 4; ++i) {
        boz_led_set(i, mask & 1);
        mask >>= 1;
    }
}

#endif

int
boz_display_enqueue(unsigned int cmd_word) {
    struct disp_cmd cmd;
    cmd.cmd = cmd_word;
    return queue_add(disp_cmd_queue.q,
            sizeof(disp_cmd_queue.q) / sizeof(disp_cmd_queue.q[0]),
            sizeof(disp_cmd_queue.q[0]), &disp_cmd_queue.qstate, &cmd);
}

void
boz_set_event_cookie(void *cookie) {
    app_context->event_cookie = cookie;
}

void
boz_set_event_handler_buzz(void (*handler)(void *, int)) {
    app_context->event_buzz = handler;
}

void
boz_set_event_handler_qm_play(void (*handler)(void *)) {
    app_context->event_qm_play = handler;
}

void
boz_set_event_handler_qm_yellow(void (*handler)(void *)) {
    app_context->event_qm_yellow = handler;
}

void
boz_set_event_handler_qm_reset(void (*handler)(void *)) {
    app_context->event_qm_reset = handler;
}

void
boz_set_event_handler_qm_rotary(void (*handler)(void *, int)) {
    app_context->event_qm_rotary = handler;
}

void
boz_set_event_handler_qm_rotary_press(void (*handler)(void *)) {
    app_context->event_qm_rotary_press = handler;
}

void
boz_set_event_handler_sound_queue_not_full(void (*handler)(void *)) {
    app_context->event_sound_queue_not_full = handler;
}

#ifdef BOZ_SERIAL
void
boz_set_event_handler_serial_data_available(void (*handler)(void *)) {
    app_context->event_serial_data_available = handler;
}
#endif

void
boz_set_alarm(long ms_from_now, void (*handler)(void *), void *cookie) {
    if (ms_from_now >= 0) {
        app_context->alarm_time_millis = millis() + ms_from_now;
        app_context->alarm_handler = handler;
        app_context->alarm_handler_cookie = cookie;
    }
    else {
        /* Nope */
        app_context->alarm_handler = NULL;
    }
}

void
boz_cancel_alarm() {
    app_context->alarm_handler = NULL;
}

int
boz_app_call(int app_id, void *param, void (*return_callback)(void *, int), void *return_callback_cookie) {
    if (app_context != NULL && app_context - app_context_stack >= APP_CONTEXT_STACK_SIZE - 1) {
        // already at the maximum app nesting level
        return -1;
    }

    if (boz_app_lookup_id(app_id, &app_to_call_data) != 0) {
        // no such app
        return -1;
    }
    app_call_defer = &app_to_call_data;
    app_call_defer_init_cookie = param;
    app_call_return = return_callback;
    app_call_return_cookie = return_callback_cookie;
    return 0;
}

void
boz_app_exit(int exit_status) {
    app_exited = 1;
    app_exit_status = exit_status;
}

void
boz_crash(unsigned int pattern) {
    /* Not using automatic variables in this function because we want a
       predictable stack layout so we can find the return address of this
       function, which in turn tells us where we were called from. */
    static struct boz_crash_arg arg;

    /* Possibly the least portable part of this whole code.
       First, read the stack pointer, which is mapped at memory address
       0x5d. Add 1 to it, so we're pointing at the return address rather than
       at the byte immediately before it. */
    arg.addr = *(byte **) (0x5d) + 1;

    /* Return address is stored on the stack endian-swapped for some reason.
       Also it's half the actual address, which is an Atmel optimisation taking
       advantage of all instructions having an even number of bytes. So having
       extracted the return address, endian-swap it, double it, then take four
       off to take account of the call to this boz_crash function. arg.addr
       then points to the boz_crash call that called us. */
    arg.addr = (void *) ( 2 * ( (((unsigned int) ((byte *) arg.addr)[0]) << 8) | ((byte *) arg.addr)[1] ) - 4 );

    arg.pattern = pattern;

    /* Call the crash program. */
    app_context = NULL;
    boz_app_call(BOZ_APP_ID_CRASH, &arg, NULL, NULL);

    /* return to the main loop immediately, and stay there, don't return to
       the application that called us */
    while (1) {
        loop();
    }
}

int
boz_is_button_pressed(int button_func, int buzzer_id, unsigned long *pressed_since_micros_r) {
    for (int i = 0; i < num_buttons; ++i) {
        struct button_state *button = &buttons[i];
        if (button->button_function == button_func && (button_func != FUNC_BUZZER || buzzer_id == button->buzzer_id)) {
            if (button->is_pressed && button->event_delivered) {
                /* Button is pressed and event has already been delivered */
                if (pressed_since_micros_r) {
                    *pressed_since_micros_r = button->pressed_since_micros;
                }
                return 1;
            }
            break;
        }
    }
    return 0;
}

int
boz_get_battery_voltage(void) {
    /* analogRead() gives us a value between 0 and 1023, which represents a
       voltage between 0 and 5V. The voltage on this pin is in the middle of a
       potential divider between VIN and GND, so it's half the voltage at VIN.
       So if the analogue value on the pin is 0, the voltage is 0V, and if it's
       1023, it's 10V. We return the answer in millivolts. */
    int value = analogRead(A6);
    return (int) ((10000L * value) / 1023L);
}

unsigned int
boz_eeprom_get_region_size() {
    if (app_context) {
        /* Return the amount of EEPROM reserved for this application */
        return app_context->eeprom_length;
    }
    else {
        return 0;
    }
}

static int check_eeprom_pos(unsigned int app_offset, unsigned int length) {
    if (app_context == NULL)
        return -1;
    if (app_offset + length > app_context->eeprom_length)
        return -1;
    return 0;
}

int
boz_eeprom_write(unsigned int app_offset, const void *datav, unsigned int length) {
    int eeprom_pos;
    const byte *data = (const byte *) datav;

    if (check_eeprom_pos(app_offset, length))
        return -1;

    eeprom_pos = (int) (app_context->eeprom_start + app_offset);

    for (unsigned int i = 0; i < length; ++i) {
        EEPROM.update(eeprom_pos, *data);
        ++eeprom_pos;
        ++data;
    }

    return 0;
}

int
boz_eeprom_read(unsigned int app_offset, void *destv, unsigned int length) {
    int eeprom_pos;
    byte *dest = (byte *) destv;

    if (check_eeprom_pos(app_offset, length))
        return -1;

    eeprom_pos = (int) (app_context->eeprom_start + app_offset);
    for (unsigned int i = 0; i < length; ++i) {
        *dest = EEPROM.read(eeprom_pos);
        ++eeprom_pos;
        ++dest;
    }

    return 0;
}

int
boz_eeprom_global_reset() {
    int eeprom_length = EEPROM.length();
    for (int i = 0; i < eeprom_length; ++i) {
        /* Put our magic identifier header at the start of the EEPROM, and
           fill the rest of it with 0xff. */
        if (i < (int) sizeof(boz_eeprom_header)) {
            EEPROM.update(i, pgm_read_byte_near(((byte *) &boz_eeprom_header) + i));
        }
        else {
            EEPROM.update(i, 0xff);
        }
    }
    return 0;
}

void
app_context_tear_down(struct app_context *ac) {
    for (int i = 0; i < NUM_CLOCKS; ++i) {
        if (ac->clocks_enabled & (1 << i)) {
            boz_clock_release(clocks[i]);
        }
    }
    boz_sound_stop_all();
    queue_clear(&disp_cmd_queue.qstate);
}

void
app_context_init(struct app_context *ac) {
    memzero(ac, sizeof(*ac));
}

void
boz_env_reset() {
    memzero(&snd_cmd_queue, sizeof(snd_cmd_queue));
    memzero(&disp_cmd_queue, sizeof(disp_cmd_queue));
    memzero(&snd_cmd_state, sizeof(snd_cmd_state));
    memzero(&disp_cmd_state, sizeof(disp_cmd_state));
    memzero(&app_context, sizeof(app_context));
    master_clocks_enabled = 0;

    noTone(PIN_SPEAKER);

#if BOZ_HW_REVISION == 0
    boz_shift_reg_init();
#endif

    boz_lcd_clear();

    boz_lcd_reset_cgram_patterns();
}

static void snd_cmd_step(unsigned long now_ms) {
    const int step_ms = 20;
    
    /* If the duration is zero, then if the frequency is set, switch on the
       tone and run away. If the frequency is zero, switch off the tone and
       run away. */
    if (snd_cmd_state.cmd.duration_ms == 0) {
        if (snd_cmd_state.cmd.freq_start == 0) {
            noTone(PIN_SPEAKER);
        }
        else {
            noTone(PIN_SPEAKER);
            tone(PIN_SPEAKER, snd_cmd_state.cmd.freq_start);
        }
        snd_cmd_state.current_freq = snd_cmd_state.cmd.freq_start;
        snd_cmd_state.running = 0;
        return;
    }
    
    if ((snd_cmd_state.cmd.is_arpeggio && snd_cmd_state.arp_index > snd_cmd_state.cmd.arp_notes_max_index) ||
                time_passed(now_ms, snd_cmd_state.start_millis + snd_cmd_state.cmd.duration_ms)) {
        /* If the duration of this command has elapsed, increment times_done,
           reset the frequency to the initial frequency, and set
           start_millis to the current time. If times_done has now reached
           the required number, we're done, otherwise we play the sound
           command again. */
        snd_cmd_state.times_done++;
        snd_cmd_state.arp_index = 0;
        snd_cmd_state.current_freq = snd_cmd_state.cmd.freq_start;
        snd_cmd_state.start_millis = now_ms;
        if (snd_cmd_state.times_done >= snd_cmd_state.cmd.num_times) {
            noTone(PIN_SPEAKER);
            snd_cmd_state.running = 0;
        }
    }

    if (!snd_cmd_state.running)
        return;

    if (snd_cmd_state.cmd.is_arpeggio) {
        /* If this is an arpeggio command, play the part of the arpeggio
           indicated by arp_index, then increment arp_index. The note plays
           for the duration of the arpeggio divided by the number of notes
           in it. */
        unsigned int arp_split_duration = snd_cmd_state.cmd.duration_ms / (snd_cmd_state.cmd.arp_notes_max_index + 1);
        byte note = snd_cmd_state.cmd.arp_notes[snd_cmd_state.arp_index++];
        snd_cmd_state.current_freq = boz_note_to_freq(note);
        noTone(PIN_SPEAKER);
        if (snd_cmd_state.current_freq > 0)
            tone(PIN_SPEAKER, snd_cmd_state.current_freq, arp_split_duration);
        snd_cmd_state.next_step_millis = now_ms + arp_split_duration;
    }
    else if (snd_cmd_state.cmd.freq_start == snd_cmd_state.cmd.freq_end) {
        if (snd_cmd_state.cmd.freq_start == 0) {
            /* No tone, but we still have to wait for duration_ms before
               moving on to the next command in the queue */
            noTone(PIN_SPEAKER);
        }
        else {
            /* JUST PLAY THE F-ING NOTE */
            noTone(PIN_SPEAKER);
            tone(PIN_SPEAKER, snd_cmd_state.cmd.freq_start, snd_cmd_state.cmd.duration_ms);
        }
        snd_cmd_state.current_freq = snd_cmd_state.cmd.freq_start;

        /* Next step in duration_ms ms, at which time we'll notice that the
           duration has passed and stop the tone. */
        snd_cmd_state.next_step_millis = now_ms + snd_cmd_state.cmd.duration_ms;
    }
    else {
        /* If we get here, freq_start and freq_end are different, and
           duration_ms is positive. */
        long remaining_ms;
        long this_step_ms;
        unsigned int desired_freq;

        /* If you define BOZ_SOUND_RAMP_LOG and use logarithmic scaling, the
           use of the pow() function and floating point arithmetic bloats the
           code size by 2KB!
           Consider un-defining BOZ_SOUND_RAMP_LOG if there's a shortage of
           code space. */
#ifdef BOZ_SOUND_RAMP_LOG
        /* Scale the frequency logarithmically between the start and end
           frequency */
        /*  start_freq * (freq_end/freq_start) ^ ((now - start) / duration) */
        desired_freq = (int) (snd_cmd_state.cmd.freq_start *
                pow((float) snd_cmd_state.cmd.freq_end / snd_cmd_state.cmd.freq_start,
                    (float) (now_ms - snd_cmd_state.start_millis) / snd_cmd_state.cmd.duration_ms));
#else
        /* Scale the frequency linearly between the start and end frequency
           using the Arduino map() function */
        desired_freq = map(now_ms, snd_cmd_state.start_millis,
                snd_cmd_state.start_millis + snd_cmd_state.cmd.duration_ms,
                snd_cmd_state.cmd.freq_start, snd_cmd_state.cmd.freq_end);
#endif

        remaining_ms = time_elapsed(now_ms, snd_cmd_state.start_millis + snd_cmd_state.cmd.duration_ms);

        if (remaining_ms > step_ms)
            this_step_ms = step_ms;
        else
            this_step_ms = remaining_ms;

        // add 10 on to the step length, in case we get called late next time
        noTone(PIN_SPEAKER);
        tone(PIN_SPEAKER, desired_freq, this_step_ms + 10);
        snd_cmd_state.current_freq = desired_freq;
        snd_cmd_state.next_step_millis = now_ms + this_step_ms;
    }
}

static void disp_cmd_step(unsigned long now_us) {
    if (disp_cmd_state.state == 0) {
        /* Send this command byte to the display */
        boz_lcd_send(disp_cmd_state.cmd.cmd);

        /* If it was a clear-display or return-home command, we need to wait
           at least 1.52 milliseconds before sending another command. For any
           other command, we need to wait at least 37 microseconds.
         */
        disp_cmd_state.state = 1; // barrier wait
        
        if (disp_cmd_state.cmd.cmd == 1 || (disp_cmd_state.cmd.cmd & 0xfffe) == 0x02) {
            // wait 3ms
            disp_cmd_state.next_step_micros = now_us + 3000;
        }
        else {
            // wait 70us
            disp_cmd_state.next_step_micros = now_us + 70;
        }
    }
    else {
        // barrier wait finished
        disp_cmd_state.running = 0;
    }
}

static void deliver_button_event(struct button_state *button) {
    switch (button->button_function) {
        case FUNC_BUZZER:
            if (app_context->event_buzz) {
                app_context->event_buzz(app_context->event_cookie, button->buzzer_id);
            }
            break;

        case FUNC_PLAY:
            if (app_context->event_qm_play)
                app_context->event_qm_play(app_context->event_cookie);
            break;

        case FUNC_YELLOW:
            if (app_context->event_qm_yellow)
                app_context->event_qm_yellow(app_context->event_cookie);
            break;

        case FUNC_RESET:
            if (app_context->event_qm_reset)
                app_context->event_qm_reset(app_context->event_cookie);
            break;

        case FUNC_RE_KEY:
            if (app_context->event_qm_rotary_press)
                app_context->event_qm_rotary_press(app_context->event_cookie);
            break;
    }
    button->event_delivered = 1;
}

/* If *next_wake_set is zero, or if *next_wake is after t, then set
   *next_wake_set to 1 and set *next_wake to t. */
static void update_if_passed(byte *next_wake_set, unsigned long *next_wake, unsigned long t) {
    if (!*next_wake_set || time_passed(*next_wake, t)) {
        *next_wake_set = 1;
        *next_wake = t;
    }
}

volatile byte boz_wake = 0;

void
button_int_handler(void) {
    boz_wake = 1;
    sleep_disable();
    detachInterrupt(digitalPinToInterrupt(PIN_BUTTON_INT));
}

void
rotary_clock_int_handler(void) {
    boz_wake = 1;
    sleep_disable();
    detachInterrupt(digitalPinToInterrupt(PIN_QM_RE_CLOCK));
}

ISR(TIMER1_OVF_vect) {
    boz_wake = 1;

    /* Disable TIMER1 overflow interrupt */
    TIMSK1 &= ~(1 << TOIE1);
    sleep_disable();
}

byte read_turny_push_button(void) {
#if BOZ_HW_REVISION == 0
    return digitalRead(PIN_QM_RE_KEY);
#else
    byte value;
    /* Make the interrupt line high-impedance, so it doesn't interfere with
       the rotary-encoder's push button, which is connected to it */
    pinMode(PIN_BUTTON_INT, INPUT);
    value = digitalRead(PIN_QM_RE_KEY);

    /* Put the interrupt line back how we found it */
    pinMode(PIN_BUTTON_INT, OUTPUT);
    digitalWrite(PIN_BUTTON_INT, LOW);

    return value;
#endif
}

static byte read_button_value(int pin) {
    if (pin == PIN_QM_RE_KEY)
        return read_turny_push_button();
    else
        return digitalRead(pin);
}

const byte switch_pins[] = {
    PIN_BUZZER_0,
    PIN_BUZZER_1,
    PIN_BUZZER_2,
    PIN_BUZZER_3,
    PIN_QM_PLAY,
    PIN_QM_YELLOW,
    PIN_QM_RESET
};
#define num_switch_pins ((int) (sizeof(switch_pins) / sizeof(switch_pins[0])))

void setup() {
    /* Set our button inputs as inputs */
    for (int i = 0; i < num_switch_pins; ++i) {
        pinMode(switch_pins[i], INPUT_PULLUP);
    }
    pinMode(PIN_QM_RE_CLOCK, INPUT_PULLUP);
    pinMode(PIN_QM_RE_DATA, INPUT_PULLUP);
    pinMode(PIN_QM_RE_KEY, INPUT_PULLUP);

#if BOZ_HW_REVISION == 0
    pinMode(PIN_BUTTON_INT, INPUT_PULLUP);
#else
    /* Interrupt line is connected to the I/O pins through switches */
    pinMode(PIN_BUTTON_INT, OUTPUT);
    digitalWrite(PIN_BUTTON_INT, LOW);
#endif

    /* The pin feeding the signal to the speaker, and the builtin LED, are
       both outputs */
    pinMode(PIN_SPEAKER, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    /* Analogue pin on which we sense the battery voltage */
    pinMode(PIN_BATTERY_SENSOR, INPUT);

#if BOZ_HW_REVISION == 1
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_Y, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    boz_leds_set(0);
#endif

    boz_mm_init(boz_dyn_arena, sizeof(boz_dyn_arena));

#if BOZ_HW_REVISION == 0
    /* Initialise shift register - this will also set the appropriate pin
       modes on the pins we use to control the shift register */
    boz_shift_reg_init();
#endif

    /* Initialise the LCD */
    boz_lcd_init();

    /* Clear sound and display queue, etc */
    boz_env_reset();

#ifdef BOZ_SERIAL
    boz_serial_init();
#endif

    /* Set app_call_defer to the struct of the first application
       to run. This is the main menu app. I've commented out the logic to
       start with a different app if certain buttons are held down (the test
       app if it's the reset button, the music loop if it's the yellow
       button) */
    /*
    if (digitalRead(PIN_QM_RESET) == LOW) {
        app_call_defer_init = test_init;
    }
    else if (digitalRead(PIN_QM_YELLOW) == LOW) {
        app_call_defer_init = music_loop_init;
    }
    else {
        app_call_defer_init = main_menu_init;
    }
    */
    if (boz_app_lookup_id(BOZ_APP_ID_INIT, &app_to_call_data) != 0) {
        // No initial app?
        boz_leds_set(1);
    }
    else {
        app_call_defer = &app_to_call_data;
    }

    /* Allocate memory for any tables/arrays we need */
    app_context_stack = (struct app_context *) boz_mm_main_alloc(sizeof(struct app_context) * APP_CONTEXT_STACK_SIZE);
    for (int i = 0; i < NUM_CLOCKS; ++i) {
        clocks[i] = NULL;
    }

    /* Check the header at the start of EEPROM - if it isn't what we expect,
       or if the version number has changed, do a full EEPROM reset. */
    struct boz_eeprom_header seen_header, correct_header;
    int reset_eeprom = 0;
    for (int i = 0; i < (int) sizeof(seen_header); ++i) {
        ((byte *) &seen_header)[i] = EEPROM.read(i);
    }
    memcpy_P(&correct_header, &boz_eeprom_header, sizeof(correct_header));

    if (strcmp(correct_header.id, seen_header.id))
        reset_eeprom = 1;
    else if (correct_header.version != seen_header.version)
        reset_eeprom = 1;

    if (reset_eeprom) {
        /* EEPROM contains unrecognised header, so reinitialise the whole lot */
        boz_leds_set(15);
        boz_eeprom_global_reset();
        boz_leds_set(0);
    }
}

void loop() {
    unsigned long ms, us;
    unsigned long next_wake_ms, next_wake_us;
    byte next_wake_ms_set = 0, next_wake_us_set = 0;
    byte buttons_busy = 0;

    ms = millis();
    us = micros();

    /* Service the sound queue */
    do {
        if (snd_cmd_state.running && time_passed(ms, snd_cmd_state.next_step_millis)) {
            snd_cmd_step(ms);
        }

        /* If the sound command has finished, see if we can dequeue another one */
        if (!snd_cmd_state.running) {
            if (queue_serve(snd_cmd_queue.q,
                        sizeof(snd_cmd_queue.q) / sizeof(snd_cmd_queue.q[0]),
                        sizeof(snd_cmd_queue.q[0]), &snd_cmd_queue.qstate,
                        &snd_cmd_state.cmd) == 0) {
                /* If we did dequeue something, make a start on that new command */
                snd_cmd_state.running = 1;
                snd_cmd_state.start_millis = ms;
                snd_cmd_state.next_step_millis = ms;
                snd_cmd_state.current_freq = 0;
                snd_cmd_state.times_done = 0;
                snd_cmd_state.arp_index = 0;
                snd_cmd_step(ms);
            }
        }
    } while (snd_cmd_state.running && time_passed(ms, snd_cmd_state.next_step_millis));

    /* Service the display command queue */
    do {
        if (disp_cmd_state.running && time_passed(us, disp_cmd_state.next_step_micros)) {
            disp_cmd_step(us);
        }

        /* If the display command finished, dequeue the next one if it exists */
        if (!disp_cmd_state.running) {
            if (queue_serve(disp_cmd_queue.q,
                        sizeof(disp_cmd_queue.q) / sizeof(disp_cmd_queue.q[0]),
                        sizeof(disp_cmd_queue.q[0]), &disp_cmd_queue.qstate,
                        &disp_cmd_state.cmd) == 0) {
                disp_cmd_state.running = 1;
                disp_cmd_state.state = 0;
                disp_cmd_state.next_step_micros = us;
                disp_cmd_step(us);
            }
        }
    } while (disp_cmd_state.running && time_passed(us, disp_cmd_state.next_step_micros));

#ifdef BOZ_SERIAL
    /* If we have serial data to send and we can send it, then do so */
    boz_serial_service_send();

    /* If we have data available on the serial port, then tell the application
       if it's interested. If it's not interested, then throw away the data. */
    if (serial_data_available) {
        if (app_context && app_context->event_serial_data_available) {
            app_context->event_serial_data_available(app_context->event_cookie);
            if (Serial.available() == 0)
                serial_data_available = 0;
        }
        else {
            while (Serial.available() > 0)
                Serial.read();
            serial_data_available = 0;
        }
    }
#endif

    /* If the app has any clocks running, check them for any events */
    if (app_context && app_context->clocks_enabled) {
        for (byte clock_index = 0; clock_index < NUM_CLOCKS; ++clock_index) {
            if (app_context->clocks_enabled & (1 << clock_index)) {
                boz_clock clock = clocks[clock_index];
                long value = boz_clock_value(clock);

                if (clock->alarm_enabled) {
                    if (time_passed_aux(value, clock->alarm_ms, clock->direction)) {
                        boz_clock_cancel_alarm(clock);
                        if (clock->event_alarm) {
                            clock->event_alarm(clock->event_cookie, clock);
                        }
                    }
                }

                if (boz_clock_running(clock)) {
                    if (clock->min_enabled) {
                        if (value <= clock->min_ms) {
                            void (*handler)(void *, boz_clock) = clock->event_expiry_min;
                            boz_clock_stop(clock);
                            //boz_clock_cancel_expiry_min(clock);
                            if (handler) {
                                clock->event_expiry_min = NULL;
                                handler(clock->event_cookie, clock);
                            }
                        }
                    }

                    if (clock->max_enabled) {
                        if (value >= clock->max_ms) {
                            void (*handler)(void *, boz_clock) = clock->event_expiry_max;
                            //boz_clock_cancel_expiry_max(clock);
                            boz_clock_stop(clock);
                            if (handler) {
                                clock->event_expiry_max = NULL;
                                handler(clock->event_cookie, clock);
                            }
                        }
                    }
                }
            }
        }
    }

    /* Check if the app has set an alarm time which has now passed */
    if (app_context && app_context->alarm_handler &&
            time_passed(ms, app_context->alarm_time_millis)) {
        void (*handler)(void *) = app_context->alarm_handler;
        app_context->alarm_handler = NULL;
        handler(app_context->alarm_handler_cookie);
    }

    if (app_context) {
        /* Check if any buttons have changed state since we last checked.

           If we always checked the buttons in the same order, then if two
           buttons were pressed at the same time, or so close together that
           we couldn't tell which was first, then lower-numbered buzzers
           would always win.

           So to make it fair, each time round the main loop we vary which
           button we check first, and whether we iterate through the button
           list forwards or backwards.
         */

        int button_index = (int) button_check_start;
        do {
            struct button_state *button = &buttons[button_index];
            byte bval = read_button_value(button->pin);

            if (bval == (button->active_low ? HIGH : LOW)) {
                /* Button is not held down */
                if (button->is_pressed) {
                    /* If it was pressed, it no longer is, so record when we
                       saw the button get released */
                    button->is_pressed = 0;
                    button->released_since_micros = us;
                }
                if (button->event_delivered) {
                    if (button->release_threshold_us == 0 ||
                            time_elapsed(button->released_since_micros, us) >= button->release_threshold_us) {
                        /* Button has been released long enough that if we see
                           another press, we should consider it another event */
                        button->event_delivered = 0;
                    }
                }
            }
            else {
                /* Button is held down */
                if (!button->is_pressed) {
                    /* Button has changed state to "pressed". */
                    button->is_pressed = 1;
                    button->pressed_since_micros = us;
                    if (button->button_function == FUNC_RE_CLOCK) {
                        /* If this was the clock for the rotary encoder, read
                           the data pin now rather than when we're satisfied
                           this is a real signal. When the pin has been active
                           for the requisite duration we'll use this value
                           then. */
                        re_data_value_last_clock = digitalRead(PIN_QM_RE_DATA);
                    }
                }
                if (!button->event_delivered &&
                        (button->press_threshold_us == 0 ||
                         time_elapsed(button->pressed_since_micros, us) >= button->press_threshold_us)) {
                    /* Button has been continuously pressed long enough that we
                       can consider it an actual press. */
                    if (button->button_function == FUNC_RE_CLOCK) {
                        unsigned long *last_turn_ms;
                        /* We've seen a rising edge of the rotary encoder's
                           clock pin. Provided this is at least
                           ROTARY_TURN_MIN_GAP_MS after the last time we
                           decided the rotary encoder had been turned the same
                           way, we're now satisfied it's a real signal rather
                           than a bounce. re_data_value_last_clock
                           contains the value on the data pin at the point we
                           received the clock signal. */
                        if (re_data_value_last_clock == HIGH)
                            last_turn_ms = &re_last_turn_high_ms;
                        else
                            last_turn_ms = &re_last_turn_low_ms;

                        if (time_elapsed(*last_turn_ms, ms) >= ROTARY_TURN_MIN_GAP_MS) {
                            if (app_context->event_qm_rotary) {
                                app_context->event_qm_rotary(app_context->event_cookie, re_data_value_last_clock == LOW);
                            }
                            *last_turn_ms = ms;
                        }
                        button->event_delivered = 1;
                    }
                    else {
                        /* Call the application's event handler for this
                           button, if there is one. */
                        deliver_button_event(button);
                    }
                }
            }

            /* If the button is pressed, or if the button is not pressed but an
               event has just been delivered (and the required release wait
               time hasn't elapsed so we haven't cleared event_delivered), then
               the button is busy (i.e. we're waiting for something to happen
               shortly on this button) and we shouldn't sleep. */
            if (button->is_pressed || button->event_delivered) {
                buttons_busy = 1;
            }

            /* Move on to the next button in the array, and finish if we've
               reached the button we started on */
            button_index += button_check_direction;
            if (button_index < 0)
                button_index = num_buttons - 1;
            else if (button_index >= num_buttons)
                button_index = 0;
        } while (button_index != button_check_start);

        if (button_check_direction < 0) {
            /* If we went backwards this time, next time switch to forwards
               and start with the button after the one we started this time,
               subject to always starting on a buzzer pin (that is, not an
               operator button like play or reset). */
            button_check_direction = 1;
            button_check_start++;
            if (button_check_start > BOZ_LAST_BUZZER_BUTTON_INDEX)
                button_check_start = BOZ_FIRST_BUZZER_BUTTON_INDEX;
        }
        else {
            /* If we went forwards this time, go backwards next time, starting
               from the same place. */
            button_check_direction = -1;
        }
    }

    /* If the sound queue can receive input, see if the app is interested
       in hearing about this exciting news. */
    if (app_context && app_context->event_sound_queue_not_full) {
        if (!snd_cmd_queue.qstate.full) {
            void (*handler)(void *) = app_context->event_sound_queue_not_full;
            app_context->event_sound_queue_not_full = NULL;
            handler(app_context->event_cookie);
        }
    }

    /* Has the current app exited? */
    while (app_exited) {
        /* It has. First release any resources this app still has */
        app_context_tear_down(app_context);

        /* Free any dynamic storage this app has allocated */
        boz_mm_pop_context();

        /* You can't call another app in the same call as exiting */
        app_call_defer = NULL;
        app_exited = 0;

        if (app_context > app_context_stack) {
            /* Pass its return code to the previous app on the stack */
            app_context--;
            app_context->app_call_return_handler(app_context->app_call_return_cookie, app_exit_status);
        }
        else {
            /* First app isn't allowed to exit. Sorry. */
        }
    }

    /* Has this app, or the app that just exited, called another app? */
    if (app_call_defer != NULL) {
        void (*next_app_init)(void *) = app_call_defer->init;

        if (app_context == NULL) {
            /* This is the initial app */
            app_context = &app_context_stack[0];
        }
        else {
            app_context->app_call_return_handler = app_call_return;
            app_context->app_call_return_cookie = app_call_return_cookie;

            /* Call that set app_call_defer_init has already checked we have
               enough space on the app context stack */
            app_context++;
        }

        /* All apps are allowed to assume that when they're called, CGRAM will
           contain their default characters and the LEDs will be off. */
        boz_lcd_reset_cgram_patterns();
        boz_leds_set(0);

        app_context_init(app_context);

        /* Create a new allocated-chunks list in the memory manager for this
           new app context */
        boz_mm_push_context();

        /* Set forbid_sleep if necessary */
        if (app_call_defer->flags & BOZ_APP_NO_SLEEP) {
            app_context->forbid_sleep = 1;
        }

        /* Copy the details of this app's reserved EEPROM region, if any,
           from the struct boz_app into app_context */
        app_context->eeprom_start = app_call_defer->eeprom_start;
        app_context->eeprom_length = app_call_defer->eeprom_length;

        app_call_defer = NULL;

        next_app_init(app_call_defer_init_cookie);
    }


    /* Now the app has had all its event handlers called, work out when we
       next need to wake up.

       We can sleep (go into CPU idle mode) if all of the following
       conditions hold:
         * No buzzer or button is currently held down, or in a state where
           we're waiting a short debounce time for it to settle.
         * No display command is running.
         * There is nothing on the display queue.
         * A sound command is running, or (there is no sound command running
           and the sound command queue is empty).

       If all these conditions hold, we will sleep until any of the following
       things happen:
         * A clock alarm is triggered, or a clock reaches or passes its min or
           max value.
         * An app's alarm time is reached or passed.
         * A running sound command reaches its next_step_millis time.
         * Any buzzer or button is pressed, or the rotary knob is turned.
    */

    byte can_sleep = 1;

    /* In case the many event handlers we might have called above took a long
       time to run, update ms and us */
    ms = millis();
    us = micros();

    if (buttons_busy || app_context->forbid_sleep)
        can_sleep = 0;
    else if (disp_cmd_state.running || !queue_is_empty(&disp_cmd_queue.qstate))
        can_sleep = 0;
    else if (!snd_cmd_state.running && !queue_is_empty(&snd_cmd_queue.qstate))
        can_sleep = 0;

    if (can_sleep) {
        if (snd_cmd_state.running) {
            update_if_passed(&next_wake_ms_set, &next_wake_ms, snd_cmd_state.next_step_millis);
        }
        if (disp_cmd_state.running) {
            update_if_passed(&next_wake_us_set, &next_wake_us, disp_cmd_state.next_step_micros);
        }

        if (app_context && app_context->clocks_enabled) {
            /* Check each enabled, running clock */
            for (int clock_index = 0; clock_index < NUM_CLOCKS; ++clock_index) {
                boz_clock clock = clocks[clock_index];
                if ((app_context->clocks_enabled & (1 << clock_index)) && boz_clock_running(clock)) {

                    /* If the clock is heading for an alarm, a minimum or a
                       maximum limit, work out how long it'll take to get
                       there, and update next_wake_ms if necessary. */
                    long clock_event_millis;
                    if (clock->alarm_enabled) {
                        clock_event_millis = ms + boz_clock_get_ms_until_alarm(clock);
                        update_if_passed(&next_wake_ms_set, &next_wake_ms, clock_event_millis);
                    }

                    if (clock->min_enabled && !boz_clock_is_direction_forwards(clock)) {
                        clock_event_millis = ms + (boz_clock_value(clock) - clock->min_ms);
                        update_if_passed(&next_wake_ms_set, &next_wake_ms, clock_event_millis);
                    }

                    if (clock->max_enabled && boz_clock_is_direction_forwards(clock)) {
                        clock_event_millis = ms + (clock->max_ms - boz_clock_value(clock));
                        update_if_passed(&next_wake_ms_set, &next_wake_ms, clock_event_millis);
                    }
                }
            }
        }

        /* If this app has set a general alarm, take account of that */
        if (app_context && app_context->alarm_handler) {
            update_if_passed(&next_wake_ms_set, &next_wake_ms, app_context->alarm_time_millis);
        }
    }

    if (can_sleep) {
        /* Number of milliseconds to wait for the next timer-based event.
           If ms_to_wait is 0 and can_sleep is 1, it means there are no
           timer-based events pending and we should sleep until we get a
           button press. */
        unsigned long ms_to_wait = 0;

        if (next_wake_ms_set) {
            /* Set a timer to give us an interrupt when millis() reaches
               next_wake_ms. */

            /* Initialise TIMER1 with a 1/256 prescaler, so it counts with a
               frequency of 16MHz / 256 = 62.5kHz, or one count every
               1/62500 seconds.
             */
            TCCR1A = 0;
            TCCR1B = 0;
            TCCR1B |= (1 << CS12);

            if (time_passed(ms, next_wake_ms))
                ms_to_wait = 0;
            else
                ms_to_wait = time_elapsed(ms, next_wake_ms);

            if (ms_to_wait == 0) {
                can_sleep = 0;
            }
            else {
                /* Cap the wait time at 1 second. If we have to wait longer
                   than that, we'll wake up after a second, realise there's
                   nothing to do and go back to sleep again. */
                if (ms_to_wait > 1000)
                    ms_to_wait = 1000;
            }
        }

        if (can_sleep) {
            set_sleep_mode(SLEEP_MODE_IDLE);
            noInterrupts();
            boz_wake = 0;
            sleep_enable();

            if (ms_to_wait) {
                /* Initialise TIMER1 counter to 65536 minus the number of
                   counts we have to wait. When TIMER1 reaches 65536 (0) then
                   we'll get an interrupt. It's 62.5 counts per millisecond. */
                TCNT1 = (unsigned short) (65536L - ((ms_to_wait * 125L) / 2));

                /* Enable TIMER1 overflow interrupt */
                TIMSK1 |= (1 << TOIE1);
            }
            interrupts();

#if BOZ_HW_REVISION == 1
            /* The button input pins are connected to the interrupt pin through
               the switches. When going to sleep, we'll make the interrupt pin
               an input, and the I/O pins output low. */
            pinMode(PIN_BUTTON_INT, INPUT_PULLUP);
            for (int i = 0; i < num_switch_pins; ++i) {
                pinMode(switch_pins[i], OUTPUT);
                digitalWrite(switch_pins[i], LOW);
            }
#endif

            /* We want interrupts when any button is pressed (pin D2 is low) or
               the rotary knob is turned (pin D3 falls) */
            attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_INT), button_int_handler, LOW);
            attachInterrupt(digitalPinToInterrupt(PIN_QM_RE_CLOCK), rotary_clock_int_handler, RISING);

            /* If we get an interrupt between enabling interrupts and calling
               sleep_cpu(), the interrupt handler will have disabled sleep mode,
               so we don't now go to sleep and miss the interrupt. */

#if BOZ_HW_REVISION == 0
            digitalWrite(LED_BUILTIN, LOW);
#endif

            while (!boz_wake) {
                /* Only carry on with the main loop when boz_wake is 1,
                   otherwise we'll go round the main loop again on every
                   interrupt, including the one that updates the clock used
                   for millis(). boz_wake is only set when one of our
                   interrupt handlers is called. */
                sleep_cpu();
            }

#if BOZ_HW_REVISION == 0
            digitalWrite(LED_BUILTIN, HIGH);
#endif

            sleep_disable();

            /* No longer interested in button interrupts - we only want these
               when we're asleep */
            detachInterrupt(digitalPinToInterrupt(PIN_BUTTON_INT));
            detachInterrupt(digitalPinToInterrupt(PIN_QM_RE_CLOCK));

            /* Disable TIMER1 overflow interrupt */
            TIMSK1 &= ~(1 << TOIE1);

#if BOZ_HW_REVISION == 1
            /* Set the interrupt pin to output LOW and the I/O pins to
               INPUT_PULLUP, so that we can tell which button if any
               woke us up. */
            pinMode(PIN_BUTTON_INT, OUTPUT);
            digitalWrite(PIN_BUTTON_INT, LOW);
            for (int i = 0; i < num_switch_pins; ++i) {
                pinMode(switch_pins[i], INPUT_PULLUP);
            }
#endif
        }
    }
}

#ifdef BOZ_SERIAL
void serialEvent() {
    serial_data_available = 1;
}
#endif
