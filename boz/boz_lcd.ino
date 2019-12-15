#include "boz_lcd.h"
#include "boz_shiftreg.h"

#define BOZ_NUM_CHAR_PATTERNS 8
const PROGMEM byte boz_char_patterns[][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0: blank
    { 0x01, 0x03, 0x07, 0x0f, 0x07, 0x03, 0x01, 0x00 }, // 1: back triangle
    { 0x10, 0x18, 0x1c, 0x1e, 0x1c, 0x18, 0x10, 0x00 }, // 2: play triangle
    { 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00 }, // 3: thick horiz bar
    { 0x04, 0x08, 0x1e, 0x09, 0x05, 0x01, 0x01, 0x0e }, // 4: reset symbol
    { 0x03, 0x14, 0x18, 0x1c, 0x00, 0x00, 0x00, 0x00 }, // 5: a-clockwise wheel
    { 0x18, 0x05, 0x03, 0x07, 0x00, 0x00, 0x00, 0x00 }, // 6: clockwise wheel
    { 0x00, 0x0e, 0x19, 0x17, 0x17, 0x19, 0x0e, 0x00 }, // 7: copyright symbol
};

#if BOZ_HW_REVISION == 0

/* Code for the original Bozzer prototype. You probably want to ignore all
   of this and start after #else.
*/

void
boz_lcd_send(unsigned int cmd) {
    if (cmd == BOZ_LCD_RESET_CGRAM) {
        boz_lcd_reset_cgram_patterns();
        return;
    }

    if (cmd & 0x100) {
        digitalWrite(PIN_DISP_RS, HIGH);
    }
    else {
        digitalWrite(PIN_DISP_RS, LOW);
    }
    
    /* The top nibble of the shift register is what's connected to the
       display. Make sure we only work on that top nibble so as to leave
       everything else how it is. */
    boz_shift_reg_set_top_nibble((cmd & 0xf0) >> 4);
    digitalWrite(PIN_DISP_E, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_DISP_E, LOW);
    delayMicroseconds(2);

    boz_shift_reg_set_top_nibble(cmd & 0x0f);
    digitalWrite(PIN_DISP_E, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_DISP_E, LOW);
    delayMicroseconds(2);
}

void
boz_lcd_clear() {
    boz_lcd_send(0x01);
    delayMicroseconds(10000);
}

void
boz_lcd_init() {
    pinMode(PIN_DISP_E, OUTPUT);
    pinMode(PIN_DISP_RS, OUTPUT);

    /* Wait 50 ms */
    delay(50);

    /* Send initial function set word, which is half a word */
    digitalWrite(PIN_DISP_RS, LOW);

    boz_shift_reg_set_top_nibble(3);
    digitalWrite(PIN_DISP_E, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_DISP_E, LOW);

    delay(10); // "wait for more than 4ms"

    boz_shift_reg_set_top_nibble(3);
    digitalWrite(PIN_DISP_E, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_DISP_E, LOW);

    delayMicroseconds(200); // "wait for more than 100us"

    boz_shift_reg_set_top_nibble(3);
    digitalWrite(PIN_DISP_E, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_DISP_E, LOW);

    delayMicroseconds(200);

    // now change to 4-bit interface
    boz_shift_reg_set_top_nibble(2);
    digitalWrite(PIN_DISP_E, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_DISP_E, LOW);

    delayMicroseconds(200);

    /* Send function set command, twice, with a delay between */
    boz_lcd_send(0x28);
    delayMicroseconds(150);
    boz_lcd_send(0x28);
    delayMicroseconds(150);

    /* Initialise display properties */
    boz_lcd_send(0x0c);
    delayMicroseconds(150);

    /* Clear display */
    boz_lcd_clear();
    
    /* Entry mode set */
    boz_lcd_send(0x06);
    delayMicroseconds(150);
}

void
boz_lcd_set_cgram_address(byte address) {
    boz_lcd_send(0x40 | (address & 0x3f));
}

#else

/* Code for the display attached to the modern Bozzard, which uses I2C */

#include <Wire.h>

byte backlight_on = 1;

/* The I2C helper on the back of the display board receives a data byte,
   like this:
   I7 I6 I5 I4 I3 I2 I1 I0

   Each bit controls one of the pins on the display interface:
   I7: D7
   I6: D6
   I5: D5
   I4: D4
   I3: backlight
   I2: E
   I1: R/W
   I0: RS
*/

#define PAYLOAD_RS (1 << 0)
#define PAYLOAD_RW (1 << 1)
#define PAYLOAD_E (1 << 2)
#define PAYLOAD_BACKLIGHT (1 << 3)
#define PAYLOAD_NIBBLE_SHIFT 4

/* The I2C display component I have uses 0x27 as its I2C address. Change this
   if you're using a display which uses a different address. */
#define DISPLAY_I2C_ADDRESS 0x27

void
boz_lcd_send(unsigned int cmd) {
    byte rs = 0;

    /* If cmd == BOZ_LCD_RESET_CGRAM, then instead of sending one command,
       we send a whole load of commands to reset the CGRAM characters to their
       defaults. */
    if (cmd == BOZ_LCD_RESET_CGRAM) {
        boz_lcd_reset_cgram_patterns();
        return;
    }

    /* If cmd & 0x100 then RS is set, otherwise it's clear. */
    if (cmd & 0x100) {
        rs = 1;
    }

    boz_lcd_send_nibble((cmd & 0xf0) >> 4, rs);
    boz_lcd_send_nibble(cmd & 0x0f, rs);
}

static void send_i2c(byte payload) {
    Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
    Wire.write(payload);
    Wire.endTransmission();
}

void
boz_lcd_send_nibble(unsigned int data, byte rs) {
    byte payload = data << PAYLOAD_NIBBLE_SHIFT;

    if (rs)
        payload |= PAYLOAD_RS;

    if (backlight_on)
        payload |= PAYLOAD_BACKLIGHT;

    /* Upsy downsy */
    delayMicroseconds(2);
    payload |= PAYLOAD_E;
    send_i2c(payload);

    delayMicroseconds(2);
    payload &= ~PAYLOAD_E;
    send_i2c(payload);
}

byte
boz_lcd_get_backlight_state(void) {
    return backlight_on;
}

void
boz_lcd_set_backlight_state(byte value) {
    backlight_on = value;
    send_i2c(backlight_on ? PAYLOAD_BACKLIGHT : 0);
}

void
boz_lcd_clear() {
    boz_lcd_send(0x01);
    delayMicroseconds(10000);
}

void
boz_lcd_init() {
    Wire.begin();

    delay(100);

    /* Start convoluted initialisation process */
    boz_lcd_send_nibble(0x03, 0);
    delay(10);
    boz_lcd_send_nibble(0x03, 0);
    delayMicroseconds(200);
    boz_lcd_send_nibble(0x03, 0);
    delayMicroseconds(200);
    boz_lcd_send_nibble(0x02, 0);
    delayMicroseconds(200);

    /* Send function set command, twice, with a delay between */
    boz_lcd_send(0x28);
    delayMicroseconds(150);
    boz_lcd_send(0x28);
    delayMicroseconds(150);

    /* Initialise display properties */
    boz_lcd_send(0x0c);
    delayMicroseconds(150);

    /* Clear display */
    boz_lcd_clear();
    
    /* Entry mode set */
    boz_lcd_send(0x06);
    delayMicroseconds(150);

}

void
boz_lcd_set_cgram_address(byte address) {
    boz_lcd_send(0x40 | (address & 0x3f));
}

#endif

void
boz_lcd_reset_cgram_patterns(void) {
    /* Set up the default character patterns for the eight user-definable
       character codes. This doesn't use the display command queue, it talks
       to the display directly with the necessary delays. This is because there
       are a lot of characters to define, each with eight rows, so we'd be in
       danger of filling up the queue if we try to enqueue all of this at
       once. */
    for (int char_index = 0; char_index < BOZ_NUM_CHAR_PATTERNS; ++char_index) {
        boz_lcd_set_cgram_address(char_index << 3);
        delayMicroseconds(150);
        for (int i = 0; i < 8; ++i) {
            boz_lcd_send(0x100 | pgm_read_byte_near(&boz_char_patterns[char_index][i]));
            delayMicroseconds(150);
        }
    }
}

