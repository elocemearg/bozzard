#ifndef _OPTIONS_H
#define _OPTIONS_H

/* THE OPTIONS APP
 *
 * Introduction
 *
 * The options app is designed to be called by other apps to show the user a
 * series of options on the display, one per page. When the options app is
 * started, the user is invited to scroll through the options by turning the
 * rotary knob, change an option by pressing the rotary knob and turning it to
 * set it to the appropriate value, and save that value by pressing the rotary
 * knob again. Finally, if the user scrolls to the end, the display asks them
 * to press "play" to accept or "reset" to discard.
 *
 * The names and types of options, as well as their initial values, are
 * provided by the calling app. When the options app returns, the calling app
 * can see which values the user chose for each option. The options app's
 * return value indicates whether the user accepted or discarded their changes.
 *
 * Input
 *
 * The options app takes as its argument a pointer to a 
 * struct option_menu_context. This struct defines what settings will be
 * presented to the user.
 *
 * Output
 *
 * When the options app exits, it returns a status of 0 if the user pressed the
 * "play" button to accept the changes, or a nonzero status if the user pressed
 * the "reset" button to discard the changes. In addition, the "results"
 * member of the struct option_menu_context contains the user's chosen values
 * of all the option settings.
 *
 * The struct option_menu_context
 *
 * The "pages" member of the options_menu_context struct contains an array
 * of struct option_page objects (the count of pages is given by "num_pages"),
 * which is expected to be IN PROGRAM MEMORY. Each "page" corresponds to one
 * thing the user can set. This may be a clock time, or a number, or one of a
 * list of choices, or a yes/no question.
 *
 * "page_disable_mask" specifies which pages, if any, are to be ignored and
 * skipped over in the scrolling order. It is useful if an app wants to hide
 * certain options in certain circumstances, without having to define a whole
 * set of similar arrays of struct option_pages in program memory. Pages
 * are numbered from 0, and if the bit representing the value (1 << page_number)
 * is set, that page will not be displayed. Only the first
 * (8 * sizeof(unsigned int)) pages may be disabled.
 *
 * "one_shot", if set to a nonzero value, indicates that once the user has
 * selected a value for the first page, the options app is to return
 * immediately. This is really only useful if there is only one page, and an
 * app wants to ask a quick question of the user.
 *
 * When the options app is called, the "results" member of the struct
 * option_menu_context must point to an array of num_pages longs. These define
 * the initial values of the settings on each page, and are overwritten by the
 * options app to indicate the values the user chose.
 *
 * struct option_page
 *
 * In a struct option_page, the "type" member specifies what kind of setting
 * this is. On each page, the "name" string will be displayed on the top line
 * of the display, and the value on the bottom line.
 *
 * "type" is OPTION_TYPE_YES_NO, OPTION_TYPE_CLOCK, OPTION_TYPE_NUMBER or
 * OPTION_TYPE_LIST.
 *
 * In the case of OPTION_TYPE_CLOCK, "type" should also have one or more of the
 * bits OPTION_TYPE_CLOCK_HR, OPTION_TYPE_CLOCK_MIN and OPTION_TYPE_CLOCK_SEC
 * set, to indicate which of the hour, minute and second fields should be
 * available to the user. Consider using one of the predefined constants
 * OPTION_TYPE_CLOCK_HR_MIN, OPTION_TYPE_CLOCK_MIN_SEC, etc.
 * 
 * The pointer "results" must point to an array of num_pages longs. It
 * specifies the initial values of the option settings when the options app
 * first starts, and when the options app returns, it contains the values the
 * user chose.
 *
 * For each page type, the corresponding value's meaning is as follows:
 *
 * Type                 Value
 * ----------------------------------------------------------------------------
 * OPTION_TYPE_YES_NO   1 for yes, 0 for no.
 * OPTION_TYPE_CLOCK_*  The clock time, in seconds.
 * OPTION_TYPE_NUMBER   The chosen number, which must be between min_value and
 *                      max_value.
 * OPTION_TYPE_LIST     The index of the selected option. For example, if
 *                      "choices" points to the array {"Apples", "Oranges",
 *                      "Pears"}, the value corresponding to the choice "Pears"
 *                      is 2.
 *
 * min_value and max_value specify the minimum and maximum value for option
 * page types OPTION_TYPE_CLOCK and OPTION_TYPE_NUMBER. For other types,
 * min_value and max_value are ignored.
 *
 * step_size specifies for OPTION_TYPE_NUMBER how much the value changes for
 * each step the rotary encoder's wheel is turned. It is ignored for other
 * option page types.
 *
 * If null_value is not NULL, it must point to a nul-terminated string which
 * appears as an option alongside the selectable numbers in an
 * OPTION_TYPE_NUMBER option page. It appears in sequence between the maximum
 * and minimum values. If it is selected, the corresponding value in "results"
 * is one step_size less than min_value.
 *
 * This is useful if you want to ask the user for a number, but also allow the
 * user to select a non-number option. For example, if you wanted to ask the
 * user how long to wait after a buzz before automatically unlocking the
 * buzzers: 1 second, 2 seconds, 3 seconds or not at all, you might set
 * min_value to 1, max_value to 3, step_size to 1 and null_value to "Disabled".
 * Turning the wheel would then cycle the value through "Disabled", 1, 2 and 3.
 *
 * "choices" points to a list of strings from which the user is invited to
 * select. It is used for the OPTION_TYPE_LIST page type, and is ignored for
 * other pages. If not set to NULL, it must point to an array of "num_choices"
 * strings in program memory.
 */

#define OPTION_TYPE_CLOCK 0
#define OPTION_TYPE_CLOCK_HR 4
#define OPTION_TYPE_CLOCK_MIN 2
#define OPTION_TYPE_CLOCK_SEC 1
#define OPTION_TYPE_CLOCK_HR_MIN (OPTION_TYPE_CLOCK_HR | OPTION_TYPE_CLOCK_MIN)
#define OPTION_TYPE_CLOCK_HR_MIN_SEC (OPTION_TYPE_CLOCK_HR | OPTION_TYPE_CLOCK_MIN | OPTION_TYPE_CLOCK_SEC)
#define OPTION_TYPE_CLOCK_MIN_SEC (OPTION_TYPE_CLOCK_MIN | OPTION_TYPE_CLOCK_SEC)

#define OPTION_TYPE_NUMBER 8
#define OPTION_TYPE_YES_NO 16
#define OPTION_TYPE_LIST 32

#define OPTION_MAIN_TYPE_MASK 0x78
#define OPTION_SUB_TYPE_MASK 0x07

struct option_page {
    /* The name of this setting, which will be displayed on the top line of the
     * display for this page, while the current value is displayed on the
     * bottom line. */
    const char *name;

    /* The type of this option page, which is OPTION_TYPE_CLOCK,
     * OPTION_TYPE_NUMBER, OPTION_TYPE_LIST, or OPTION_TYPE_YES_NO.
     *
     * In the case of OPTION_TYPE_CLOCK, it should be bitwise-ored with
     * one or more of OPTION_TYPE_CLOCK_HR, OPTION_TYPE_CLOCK_MIN and
     * OPTION_TYPE_CLOCK_SEC, to specify which fields of a clock time we should
     * invite the user to set. */
    char type;

    /* Minimum and maximum values of the number for OPTION_TYPE_NUMBER, or the
     * number of seconds for OPTION_TYPE_CLOCK */
    long min_value, max_value;

    /* Turning the wheel one click will advance the value by this (default 1) */
    int step_size;

    /* Name of a special value that appears one step before the minimum value
     * for OPTION_TYPE_NUMBER fields */
    const char *null_value;

    /* For lists - the name of each of a list of multiple-choice options */
    const char * const *choices;
    int num_choices;
};

/* struct option_menu_context, a pointer to which is the argument to the
 * "options" app. */
struct option_menu_context {
    /* Pointer to program memory, containing information on each option page. */
    const struct option_page *pages;

    /* The number of option pages in "pages". */
    int num_pages;

    /* If page_disable_mask is 0, all pages in "options" are enabled. If not,
     * then if the bit representing (1 << page) is set, then that page will be
     * skipped over and its value in "results" will be unchanged. */
    unsigned int page_disable_mask;

    /* Must point to an array of num_pages longs. This gives the options app
     * the initial values of all the option settings (see the documentation
     * above for more information). When the options app returns, it will
     * contain the values the user chose. */
    long *results;

    /* If true, the control on the page will already be selected, and when
     * OK is pressed, or we navigate through all the controls on the page,
     * the options app will return immediately. Only the first element of
     * options will be shown, so it's only useful to have one element
     * (and num_pages should be 1). */
    int one_shot;
};

#endif
