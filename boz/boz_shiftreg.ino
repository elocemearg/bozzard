#include "boz_shiftreg.h"
#include "boz_pins.h"

#ifdef BOZ_ORIGINAL

/* 8-bit shift register connected to pins PIN_SR_DATA, PIN_SR_CLOCK and
   PIN_SR_LATCH. */

byte shift_reg_value = 0;

void
boz_shift_reg_write(void) {
    shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, MSBFIRST, shift_reg_value);
    digitalWrite(PIN_SR_LATCH, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_SR_LATCH, LOW);
}

void
boz_shift_reg_init(void) {
    pinMode(PIN_SR_LATCH, OUTPUT);
    pinMode(PIN_SR_CLOCK, OUTPUT);
    pinMode(PIN_SR_DATA, OUTPUT);
    digitalWrite(PIN_SR_LATCH, LOW);
    digitalWrite(PIN_SR_CLOCK, LOW);
    digitalWrite(PIN_SR_DATA, LOW);
    shift_reg_value = 0;
    boz_shift_reg_write();
}

void
boz_shift_reg_set(byte value) {
    shift_reg_value = value;
    boz_shift_reg_write();
}

void
boz_shift_reg_set_top_nibble(byte value) {
    shift_reg_value = (value << 4) | (shift_reg_value & 0x0f);
    boz_shift_reg_write();
}

void
boz_shift_reg_set_bottom_nibble(byte value) {
    shift_reg_value = (shift_reg_value & 0xf0) | (value & 0x0f);
    boz_shift_reg_write();
}

void
boz_shift_reg_bit_set(byte which_bit) {
    shift_reg_value |= (1 << which_bit);
    boz_shift_reg_write();
}

void
boz_shift_reg_bit_clear(byte which_bit) {
    shift_reg_value &= ~(1 << which_bit);
    boz_shift_reg_write();
}

#else

/* Shift register doesn't exist */
void
boz_shift_reg_init(void) {
}

#endif
