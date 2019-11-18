#ifndef _OPTIONS_H
#define _OPTIONS_H

#define OPTION_TYPE_CLOCK 0
#define OPTION_TYPE_CLOCK_HR 4
#define OPTION_TYPE_CLOCK_MIN 2
#define OPTION_TYPE_CLOCK_SEC 1
#define OPTION_TYPE_CLOCK_HR_MIN (OPTION_TYPE_CLOCK_HR | OPTION_TYPE_CLOCK_MIN)
#define OPTION_TYPE_CLOCK_HR_MIN_SEC (OPTION_TYPE_CLOCK_HR | OPTION_TYPE_CLOCK_MIN | OPTION_TYPE_CLOCK_SEC)
#define OPTION_TYPE_CLOCK_MIN_SEC (OPTION_TYPE_CLOCK_MIN | OPTION_TYPE_CLOCK_SEC)

#define OPTION_TYPE_NUMBER 8
#define OPTION_TYPE_DIGITS 12
#define OPTION_TYPE_YES_NO 16
#define OPTION_TYPE_LIST 32

#define OPTION_MAIN_TYPE_MASK 0x78
#define OPTION_SUB_TYPE_MASK 0x07

#define OPTION_CLOCK_HOUR 0
#define OPTION_CLOCK_MINUTE 1
#define OPTION_CLOCK_SECOND 2

struct option_page {
    const char *name;
    char type;

    /* The number for OPTION_TYPE_NUMBER, the number of seconds for
     * OPTION_TYPE_CLOCK */
    long min_value, max_value;

    /* Turning the wheel one click will advance the value by this (default 1) */
    int step_size;

    /* One less than the min for OPTION_TYPE_NUMBER fields */
    const char *null_value;

    /* For list fields */
    const char * const *choices;
    int num_choices;
};

struct option_menu_context {
    /* Pointer to program memory, containing information on each option page. */
    const struct option_page *options;

    /* The number of option pages in "options". */
    int num_options;

    /* If page_disable_mask is 0, all pages in "options" are enabled. If not,
     * then if the bit representing (1 << page) is set, then that page will be
     * skipped over and its value in "results" will be unchanged. */
    unsigned int page_disable_mask;

    /* Must point to an array of num_options longs */
    long *results;

    /* If true, the control on the page will already be selected, and when
     * OK is pressed, or we navigate through all the controls on the page,
     * the options app will return immediately. Only the first element of
     * options will be shown, so it's only useful to have one element
     * (and num_options should be 1). */
    int one_shot;
};

#endif
