#include "boz_lcd.h"
#include "boz_shiftreg.h"

void
boz_lcd_send(unsigned int cmd) {
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
