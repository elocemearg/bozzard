#include "boz_api.h"
#include "options.h"
#include <avr/pgmspace.h>

struct options_state {
    struct option_page current_page;
    int page_number;
    struct option_menu_context *menu;

    /* Zero if turning the rotary knob will move from page to page.
       If it's 1 or more, it refers to one of the things we can select on the
       page, and turning the knob will change that thing.
    */
    int control_depth;
};

static struct options_state *options_state;


/* Instructions for printing a clock value on the bottom row.
   Each disp_print_inst prints either a literal or some part of the value.
   Each possible type of clock format (H:M:S, H:M, H, M:S, M, S) has a list of
   these instructions.
   */
struct disp_print_inst {
    // if literal != 0, print that character num_chars times.
    char literal;

    /* Number of times to print the literal, or number of chars a number
       should take up (not including any brackets around the number to indicate
       it is selected). */
    byte num_chars;

    /* If this is a control, the corresponding control depth (1 = first
       control, 2 = second control, etc). For clock and number pages, if a
       control matches the control depth it will be printed with brackets
       around it, otherwise it will be printed with spaces around it. */
    char control_depth;

    // the value for a clock control is a number of seconds
    // divide value by this before printing it
    int value_div;

    // if nonzero, take the value mod this after dividing but before printing
    int value_mod;
};

struct clock_print_inst {
    const struct disp_print_inst *insts;
    byte num_insts;
};

/*
   Bottom line of display:
              0123456789012345
   Clock:     (99)h(00)m(00)s
              (99)h(00)m
              (99)h
                   (90)m(00)s
                   (90)m
                        (90)s
*/

const PROGMEM struct disp_print_inst om_clock_hms_inst[] = {
    {   0, 3, 1, 3600,  0 },
    { 'h', 1, 0,    0,  0 },
    {   0, 2, 2,   60, 60 },
    { 'm', 1, 0,    0,  0 },
    {   0, 2, 3,    1, 60 },
    { 's', 1, 0,    0,  0 },
};

const PROGMEM struct disp_print_inst om_clock_s_inst[] = {
    {   0,12, 1,    1,  0 },
    { 's', 1, 0,    0,  0 }
};

const PROGMEM struct disp_print_inst om_clock_ms_inst[] = {
    {   0, 8, 1,   60,  0 },
    { 'm', 1, 0,    0,  0 },
    {   0, 2, 2,    1, 60 },
    { 's', 1, 0,    0,  0 },
};

const PROGMEM struct disp_print_inst om_num_inst =
    {   0, 10, 1,   1,  0 }
;

const PROGMEM struct clock_print_inst clock_print_insts[] = {
    // 0: no fields - not valid.
    { NULL, 0 },

    // 1: seconds only.
    { om_clock_s_inst, 2 },

    // 2: minutes only.
    { om_clock_ms_inst, 2 },

    // 3: minutes and seconds.
    { om_clock_ms_inst, 4 },

    // 4: hours only.
    { om_clock_hms_inst, 2 },

    // 5: hours and seconds - not valid.
    { NULL, 0 },

    // 6: hours and minutes.
    { om_clock_hms_inst, 4 },

    // 7: hours, minutes and seconds.
    { om_clock_hms_inst, 6 },
};

const PROGMEM char s_accept[]  = "  " BOZ_CHAR_PLAY_S  " to accept";
const PROGMEM char s_discard[] = "  " BOZ_CHAR_RESET_S " to discard";
const PROGMEM char s_yes[] = "      Yes";
const PROGMEM char s_no[]  = "      No ";

/* cookie must be a pointer to a struct option_menu_context, which describes
   the option pages to show. */
void
option_menu_init(void *cookie) {
    struct option_menu_context *context = (struct option_menu_context *) cookie;
    int first_page = 0;

    options_state = (struct options_state *) boz_mm_alloc(sizeof(*options_state));
    memset(options_state, 0, sizeof(*options_state));

    while (context->page_disable_mask & (1 << first_page)) {
        ++first_page;
    }
    if (first_page >= context->num_pages) {
        /* No enabled pages? */
        boz_app_exit(1);
    }

    om_load_page(context, first_page);

    boz_set_event_handler_qm_play(om_play);
    boz_set_event_handler_qm_yellow(om_yellow);
    boz_set_event_handler_qm_reset(om_reset);
    boz_set_event_handler_qm_rotary(om_rotary_turn);
    boz_set_event_handler_qm_rotary_press(om_rotary_press);
    boz_set_event_cookie(context);
    options_state->menu = context;

    if (context->one_shot) {
        options_state->control_depth = 1;
    }

    om_redraw_display(context);
}

void
om_play(void *cookie) {
    struct option_menu_context *context = (struct option_menu_context *) cookie;
    if (options_state->control_depth == 0 || options_state->menu->one_shot) {
        /* Save and exit */
        boz_app_exit(0);
    }
    else {
        options_state->control_depth = 0;
        om_redraw_display(context);
    }
}

void
om_reset(void *cookie) {
    struct option_menu_context *context = (struct option_menu_context *) cookie;
    if (options_state->control_depth == 0 || options_state->menu->one_shot) {
        /* Discard and exit */
        boz_app_exit(1);
    }
    else {
        options_state->control_depth = 0;
        om_redraw_display(context);
    }
}

void
om_yellow(void *cookie) {
    struct option_menu_context *context = (struct option_menu_context *) cookie;
    int changed = 0;

    if (options_state->control_depth == 0 || options_state->menu->one_shot) {
        /* Discard and exit */
        boz_app_exit(1);
        return;
    }
    else {
        options_state->control_depth--;
        changed = 1;
    }
    if (changed)
        om_redraw_display(context);
}

void
om_rotary_turn(void *cookie, int clockwise) {
    struct option_menu_context *context = (struct option_menu_context *) cookie;
    int direction = (clockwise ? 1 : -1);

    if (options_state->control_depth == 0) {
        int next_page;

        /* Find next enabled page in the direction we've been given. */
        for (next_page = options_state->page_number + direction;
                next_page >= 0 && next_page <= context->num_pages &&
                    (context->page_disable_mask & (1 << next_page));
                    next_page += direction);

        /* We allow page_number to equal context->num_pages - this means
           we display a "Play to save, Reset to discard" banner. Otherwise,
           if the next page we've chosen is not a valid page, do nothing. */
        if (next_page >= 0 && next_page <= context->num_pages) {
            om_load_page(context, next_page);
            om_redraw_display(context);
        }
    }
    else {
        om_adjust_control(context, direction);
    }
}

void
om_rotary_press(void *cookie) {
    struct option_menu_context *context = (struct option_menu_context *) cookie;
    om_next_control(context);
}

void
om_next_control(struct option_menu_context *context) {
    if (options_state->control_depth >= om_num_controls_on_page(context)) {
        /* If there are no more controls on this page, then if we're a one-shot
           menu, exit successfully. Otherwise, select nothing. */
        if (options_state->menu->one_shot) {
            boz_app_exit(0);
            return;
        }
        else {
            options_state->control_depth = 0;
        }
    }
    else {
        /* Special case: a rotary press on a yes/no page shouldn't select
           the control, it should adjust it. */
        if (options_state->current_page.type == OPTION_TYPE_YES_NO) {
            options_state->control_depth = 1;
            om_adjust_control(context, 1);
            options_state->control_depth = 0;
        }
        else {
            options_state->control_depth++;
        }
    }
    om_redraw_display(context);
}

int
om_num_controls_on_page(struct option_menu_context *context) {
    int num_controls = 0;

    if (options_state->page_number < 0 || options_state->page_number >= context->num_pages)
        return 0;

    switch (options_state->current_page.type & OPTION_MAIN_TYPE_MASK) {
        case OPTION_TYPE_CLOCK:
            for (int mask = 1; mask <= 4; mask <<= 1)
                if (options_state->current_page.type & mask)
                    num_controls++;
            break;

        default:
            num_controls = 1;
    }

    return num_controls;
}

void
om_load_page(struct option_menu_context *context, int new_page_number) {
    if (new_page_number >= 0 && new_page_number < context->num_pages) {
        memcpy_P(&options_state->current_page, &context->pages[new_page_number], sizeof(options_state->current_page));
    }
    options_state->page_number = new_page_number;
}

void
om_adjust_control(struct option_menu_context *context, int direction) {
    long hr = 0, mi = 0, sec = 0;
    long old_value, new_value;
    struct option_page *current_page = &options_state->current_page;

    if (options_state->page_number >= context->num_pages)
        return;

    old_value = context->results[options_state->page_number];

    switch (current_page->type & OPTION_MAIN_TYPE_MASK) {
        case OPTION_TYPE_CLOCK:
            if (current_page->type & OPTION_TYPE_CLOCK_HR) {
                hr = old_value / 3600;
            }
            if (current_page->type & OPTION_TYPE_CLOCK_MIN) {
                mi = old_value / 60;
                if (current_page->type & OPTION_TYPE_CLOCK_HR)
                    mi %= 60;
            }
            if (current_page->type & OPTION_TYPE_CLOCK_SEC) {
                sec = old_value;
                if (current_page->type & (OPTION_TYPE_CLOCK_HR | OPTION_TYPE_CLOCK_MIN))
                    sec %= 60;
            }

            if (options_state->control_depth == 1) {
                /* Adjust most significant field. This doesn't wrap, it just
                   stops if you try to go too far. */
                if (current_page->type & OPTION_TYPE_CLOCK_HR) {
                    hr += direction;
                }
                else if (current_page->type & OPTION_TYPE_CLOCK_MIN) {
                    mi += direction;
                }
                else if (current_page->type & OPTION_TYPE_CLOCK_SEC) {
                    sec += direction;
                }

            }
            else if (options_state->control_depth == 2 && (current_page->type == OPTION_TYPE_CLOCK_HR_MIN || current_page->type == OPTION_TYPE_CLOCK_HR_MIN_SEC)) {
                mi += direction;
                if (mi < 0)
                    mi = 59;
                else if (mi > 59)
                    mi = 0;
            }
            else if ((options_state->control_depth == 2 && current_page->type == OPTION_TYPE_CLOCK_MIN_SEC) || (options_state->control_depth == 3 && current_page->type == OPTION_TYPE_CLOCK_HR_MIN_SEC)) {
                sec += direction;
                if (sec < 0)
                    sec = 59;
                else if (sec > 59)
                    sec = 0;
            }

            new_value = hr * 3600L + mi * 60L + sec;
            if (new_value < 0 || new_value < current_page->min_value ||
                    new_value > current_page->max_value) {
                /* Leave the value alone */
            }
            else {
                context->results[options_state->page_number] = new_value;
                om_redraw_option_value(context);
            }
            break;

        case OPTION_TYPE_NUMBER:
            if (old_value < current_page->min_value && direction < 0) {
                /* Current value is NULL, next step is max value */
                new_value = current_page->max_value;
            }
            else {
                new_value = old_value + direction * current_page->step_size;
                if (new_value > current_page->max_value) {
                    if (current_page->null_value) {
                        /* Position ourselves one step below the min value,
                           which is the NULL value for this option */
                        new_value = current_page->min_value - current_page->step_size;
                    }
                    else {
                        /* Wrap round to the min value */
                        new_value = current_page->min_value;
                    }
                }
                else if (new_value < current_page->min_value && current_page->null_value == NULL) {
                    /* Wrap around to the max value unless there's a null value */
                    new_value = current_page->max_value;
                }
            }
            context->results[options_state->page_number] = new_value;
            om_redraw_option_value(context);
            break;

        case OPTION_TYPE_YES_NO:
            new_value = !old_value;
            context->results[options_state->page_number] = new_value;
            om_redraw_option_value(context);
            break;

        case OPTION_TYPE_LIST:
            new_value = old_value + direction;
            if (new_value < 0)
                new_value = current_page->num_choices - 1;
            else if (new_value >= current_page->num_choices)
                new_value = 0;
            context->results[options_state->page_number] = new_value;
            om_redraw_option_value(context);
            break;
    }
}

void
om_redraw_display(struct option_menu_context *context) {
    boz_display_clear();

    if (options_state->page_number >= context->num_pages) {
        boz_display_write_string_P(s_accept);
        boz_display_set_cursor(1, 0);
        boz_display_write_string_P(s_discard);
    }
    else {
        boz_display_write_string_P(options_state->current_page.name);
        om_redraw_option_value(context);
    }
}

int
execute_disp_inst(struct disp_print_inst *inst, long value) {
    int i;
    int chars_written = 0;
    if (inst->literal) {
        for (i = 0; i < inst->num_chars; ++i) {
            boz_display_write_char(inst->literal);
            ++chars_written;
        }
        return chars_written;
    }
    else {
        long n = value / inst->value_div;
        int flags = 0;

        if (inst->value_mod)
            n = n % inst->value_mod;

        if (inst->control_depth == options_state->control_depth) {
            /* Draw arrows pointing to this field */
            flags |= BOZ_DISP_NUM_ARROWS;
        }
        else {
            /* Leave space for arrows pointing to this field */
            flags |= BOZ_DISP_NUM_SPACES;
        }
        if (inst->control_depth > 1)
            flags |= BOZ_DISP_NUM_ZERO_PAD;
        i = boz_display_write_long(n, inst->num_chars, flags);
        if (i < 0)
            return i;
        else
            return chars_written + i;
    }
}

void
om_display_write_centre(const char *str, int max_len) {
    int len, i;
    for (len = 0; str[len]; ++len);
    for (i = 0; i < (max_len - len) / 2; ++i)
        boz_display_write_char(' ');
    boz_display_write_string(str);
    for (i = 0; i < (1 + max_len - len) / 2; ++i)
        boz_display_write_char(' ');
}

static void bracket_bottom_row() {
    boz_display_set_cursor(1, 0);
    boz_display_write_char('[');
    boz_display_set_cursor(1, 15);
    boz_display_write_char(']');
}

void
om_redraw_option_value(struct option_menu_context *context) {
    char str[17];
    const char *str_p;
    byte col = 0;
    long value = context->results[options_state->page_number];
    struct clock_print_inst clock_insts;
    struct disp_print_inst inst;
    struct option_page *current_page = &options_state->current_page;
    boz_display_set_cursor(1, 0);

    switch (current_page->type & OPTION_MAIN_TYPE_MASK) {
        case OPTION_TYPE_CLOCK:
            memcpy_P(&clock_insts, clock_print_insts + (current_page->type & OPTION_SUB_TYPE_MASK), sizeof(clock_insts));
            for (int inst_index = 0; inst_index < clock_insts.num_insts; ++inst_index) {
                memcpy_P(&inst, clock_insts.insts + inst_index, sizeof(inst));
                col += execute_disp_inst(&inst, value);
            }
            break;

        case OPTION_TYPE_NUMBER:
            if (value < current_page->min_value) {
                strncpy_P(str, current_page->null_value, sizeof(str));
                str[15] = '\0';
                boz_display_set_cursor(1, 1);
                om_display_write_centre(str, 15);
                if (options_state->control_depth > 0) {
                    bracket_bottom_row();
                }
                col = 16;
            }
            else {
                memcpy_P(&inst, &om_num_inst, sizeof(inst));
                col += execute_disp_inst(&inst, value);
            }
            break;

        case OPTION_TYPE_YES_NO:
            if (value)
                boz_display_write_string_P(s_yes);
            else
                boz_display_write_string_P(s_no);
            col = 9;
            break;

        case OPTION_TYPE_LIST:
            str_p = (const char *) pgm_read_ptr_near(&current_page->choices[value]);
            strncpy_P(str, str_p, sizeof(str));
            str[15] = '\0';
            boz_display_set_cursor(1, 1);
            om_display_write_centre(str, 15);
            if (options_state->control_depth > 0) {
                /* Put brackets round it if it is selected, that is, if
                   turning the knob will change the value */
                bracket_bottom_row();
            }
            col = 16;
            break;
    }

    while (col < 16) {
        boz_display_write_char(' ');
        ++col;
    }
}
