#ifndef _BOZ_SHIFTREG_H
#define _BOZ_SHIFTREG_H

void
boz_shift_reg_set_top_nibble(byte value);

void
boz_shift_reg_set_bottom_nibble(byte value);

void
boz_shift_reg_set(byte value);

void
boz_shift_reg_bit_set(byte which_bit);

void
boz_shift_reg_bit_clear(byte which_bit);

void
boz_shift_reg_init(void);

#endif
