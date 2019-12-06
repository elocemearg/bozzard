#ifndef _BOZ_DISPLAY_H
#define _BOZ_DISPLAY_H

#include "boz_api.h"

#define BOZ_DISPLAY_ROWS 2
#define BOZ_DISPLAY_COLUMNS 16

/* Applications should not need to use boz_display_enqueue() directly -
 * instead, use one of the helper functions declared in boz_api.h. */
int
boz_display_enqueue(unsigned int cmd_word);

/* Set entry mode for the display. If increment is true, the cursor will move
 * one space to the right after each character is written. If false, it will
 * move one space to the left.
 * Note that the two lines of the display are not at consecutive locations
 * in the display's memory, so if you write a character at (0,15) the cursor
 * will not end up on (1,0).
 * If shift is set, something something display shift which I never use.
 *
 * By default, when the main setup routine initialises the display,
 * "increment" is set and "shift" is not.
 */
int
boz_display_set_entry_mode(int increment, int shift);

/* Something something shift again. */
int
boz_display_shift(int shift_control, int right);

/* You shouldn't need to call this function. Indeed, telling the display that
 * it's using an eight-bit interface will surprise it since only four of its
 * data pins are connected to anything.
 * The main setup function will initialise the display with the correct
 * parameters so there's no need for application code to call this. */
int
boz_display_function_set(int eight_bit_interface, int two_lines, int eleven_pixels_high);

#endif
