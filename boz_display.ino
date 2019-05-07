#include "boz.h"
#include <avr/pgmspace.h>

int
boz_display_clear() {
    return boz_display_enqueue(0x01);
}

int
boz_display_home() {
    return boz_display_enqueue(0x02);
}

int
boz_display_set_entry_mode(int increment, int shift) {
    unsigned int n = 0x04;
    if (increment)
        n |= 2;
    if (shift)
        n |= 1;
    return boz_display_enqueue(n);
}

int
boz_display_properties(int display_on, int cursor_on, int cursor_blink_on) {
    unsigned int n = 0x08;
    if (display_on)
        n |= 4;
    if (cursor_on)
        n |= 2;
    if (cursor_blink_on)
        n |= 1;
    return boz_display_enqueue(n);
}

int
boz_display_shift(int shift_control, int right) {
    unsigned int n = 0x10;
    if (shift_control)
        n |= 8;
    if (right)
        n |= 4;
    return boz_display_enqueue(n);
}

int
boz_display_function_set(int eight_bit_interface, int two_lines, int eleven_pixels_high) {
    unsigned int n = 0x20;
    if (eight_bit_interface)
        n |= 0x10;
    if (two_lines)
        n |= 0x08;
    if (eleven_pixels_high)
        n |= 0x04;
    return boz_display_enqueue(n);
}

int
boz_display_set_cgram_address(int address) {
    return boz_display_enqueue(0x40 | (address & 0x3f));
}

int
boz_display_set_cursor(int row, int column) {
    unsigned int n = 0x80;
    if (row != 0) {
        n |= 0x40;
    }
    n |= (column & 0x3f);
    return boz_display_enqueue(n);
}

int
boz_display_write_char(char c) {
    return boz_display_enqueue(0x100 | (unsigned char) c);
}

int
boz_display_write_string(const char *s) {
    int ret;
    while (*s) {
        ret = boz_display_write_char(*s);
        if (ret)
            return ret;
        ++s;
    }
    return 0;
}

int
boz_display_write_string_P(const char *str_pm) {
    char str[17];
    strncpy_P(str, str_pm, sizeof(str));
    str[16] = '\0';
    return boz_display_write_string(str);
}


#define BOZ_DISPLAY_OPEN_ARROW_CHAR '['
#define BOZ_DISPLAY_CLOSE_ARROW_CHAR ']'

int
boz_display_write_long(long n, int min_width, const char *flags) {
    char str[20]; // space for arrow/bracket, sign, 16 characters, arrow, nul
    unsigned int str_p = 0;
    int force_sign = 0;
    unsigned int num_start = 0;
    char pad_char = ' ';
    char sign = '\0';
    char arrow_char_left = 0;
    char arrow_char_right = 0;

    if (flags) {
        while (*flags) {
            if (*flags == '+')
                force_sign = 1;
            else if (*flags == '0')
                pad_char = '0';
            else if (*flags == 'A') {
                arrow_char_left = BOZ_DISPLAY_OPEN_ARROW_CHAR;
                arrow_char_right = BOZ_DISPLAY_CLOSE_ARROW_CHAR;
            }
            else if (*flags == 'a') {
                arrow_char_left = ' ';
                arrow_char_right = ' ';
            }

            ++flags;
        }
    }

    if (min_width > 16)
        min_width = 16;
    else if (min_width < 0)
        min_width = 0;

    if (n < 0)
        sign = '-';
    else if (n > 0 && force_sign)
        sign = '+';

    /* Build the number backwards, because that's easier, then reverse it
       afterwards. */
    if (arrow_char_right) {
        str[str_p++] = arrow_char_right;
    }

    num_start = str_p;

    do {
        int digit = n % 10;
        if (digit < 0)
            digit = -digit;
        str[str_p++] = '0' + digit;
        n /= 10;
    } while (n != 0);

    if (sign)
        str[str_p++] = sign;

    if (arrow_char_left && pad_char == ' ')
        str[str_p++] = arrow_char_left;

    /* Pad with spaces or zeroes */
    while (str_p - num_start < (unsigned int) min_width && str_p < sizeof(str) - 1)
        str[str_p++] = pad_char;

    if (arrow_char_left && pad_char != ' ')
        str[str_p++] = arrow_char_left;

    /* Reverse everything from the start up to but excluding str_p */
    int l, r;
    for (l = 0, r = str_p - 1; l < r; ++l, --r) {
        char tmp = str[l];
        str[l] = str[r];
        str[r] = tmp;
    }

    str[str_p] = '\0';

    r = boz_display_write_string(str);
    if (r < 0)
        return r;
    else
        return str_p;
}
