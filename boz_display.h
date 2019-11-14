#ifndef _BOZ_DISPLAY_H
#define _BOZ_DISPLAY_H

#define BOZ_DISPLAY_ROWS 2
#define BOZ_DISPLAY_COLUMNS 16

/* The following functions add commands to the display commands queue. The
 * main loop takes commands off the queue and runs them, only executing the
 * next command when the previous command is known to have completed.
 *
 * This queue has a limited length, so if you enqueue too many commands at
 * once, the queue will fill up and any further attempts to enqueue commands
 * will fail until there's space. If this happens, the display_* function will
 * return -1. Normally these functions return 0 to indicate success - this
 * means the command was successfully put on the queue, not that it was
 * successfully executed! */

/* The display has two rows of 16 characters each. Co-ordinates are given
 * with the row number first. Rows and columns are numbered from zero, so the
 * top-left cell is (0,0) and the bottom-right cell is (1,15).
 */

/* Clear the LCD display and return to the cursor to (0,0) */
int
boz_display_clear();

/* Move the cursor to row 0, column 0. */
int
boz_display_home();

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

/* Set display properties.
 * display_on: switch display on if true, off if false.
 * cursor_on: show the location of the cursor on the display if true.
 * cursor_blink_on: blink the cursor if true.
 *
 * By default, when the main setup routine initialises the display, the display
 * is on, the cursor is off, and cursor blinking is off.
 */
int
boz_display_properties(int display_on, int cursor_on, int cursor_blink_on);

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

/* Move the display's "cursor" to point inside its character generator RAM,
 * for the purpose of defining your own characters. Character codes 0 through 7
 * are writable by the user in this way. The low three bits (0x07) indicate
 * which row of pixels you want to set, and the next highest three bits
 * (0x38) specify the character code you want to write to.
 * Once the cgram address has been set, you can write to that row of pixels in
 * using display_write_char() - only the five least significant bits of that
 * function's argument will be used, corresponding to the five pixels on the
 * chosen line.
 *
 * For example, to set character code 5 to a (crudely-drawn) smiley face, you
 * might do the following:
 * display_set_cgram_address((5 << 3) | 0);
 * display_write_char(0x0e);
 * display_write_char(0x11);
 * display_write_char(0x1b);
 * display_write_char(0x11);
 * display_write_char(0x1f);
 * display_write_char(0x11);
 * display_write_char(0x0e);
 *
 *   4 3 2 1 0
 * 0   # # # 
 * 1 #       #
 * 2 # #   # #
 * 3 #       #
 * 4 # # # # #
 * 5 #       #
 * 6   # # # 
 * 7
 *
 * Then, to write this smiley face to the top-left corner of the display:
 * display_set_cursor(0, 0);
 * display_write_char(5);
 * */
int
boz_display_set_cgram_address(int address);

/* Move the display's cursor to the given row and column. */
int
boz_display_set_cursor(int row, int column);

/* Write the given character at the display's cursor, and increment (or
 * decrement, depending on entry mode) the cursor position. */
int
boz_display_write_char(char c);

/* Write the characters in the nul-terminated string s to the display, not
 * including the terminating nul. This enqueues a command for every character
 * in the string. */
int
boz_display_write_string(const char *s);

/* The same as boz_display_write_string, but the string pointed to by str_pm
 * is expected to be in PROGMEM. */
int
boz_display_write_string_P(const char *str_pm);

/* Format the given long integer "l" as a string, and write it to the current
 * display position. If min_width is positive, it specifies the minimum number
 * of characters that will be written (max 20). If necessary, the number is
 * padded on the right with spaces (or zeroes, if the 0 flag is used) to
 * achieve this.
 * "flags" may contain a list of flag characters to modify how the number is
 * formatted.
 *     '+': Write a plus sign before a positive integer.
 *     '0': Left-pad with zeroes instead of spaces to make the number at least
 *          min_width chars.
 *     'A': Draw arrows (or brackets, or some sort of highlighter) on the left
 *          and right of the number.
 *     'a': Leave space for arrows on the left and right of the number
 *          but don't draw them.
 *          If arrows are drawn, then if we're padding with spaces (the
 *          default) the arrows are added before the padding. If we're padding
 *          with zeroes, the arrows are added after the padding. In any case,
 *          the min_width does not include the arrows.
 * If flags is empty or NULL, it is ignored.
 *
 * Returns the total number of characters actually written, or -1 on error.
 */
int
boz_display_write_long(long l, int min_width, const char *flags);

#endif
