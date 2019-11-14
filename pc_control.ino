/* Very much not finished yet, this is a program that responds to commands
   sent to us on the serial port, and tells whatever's on the other end of the
   serial port when a buzz has occurred. */

#include "boz.h"

#ifdef BOZ_SERIAL

#include <avr/pgmspace.h>

#define reg int
typedef reg (*reg_read_fn)(struct reg_def *reg_def, int subscript);
typedef void (*reg_write_fn)(struct reg_def *reg_def, int subscript, reg value);

const byte REG_ORD = 1;
const byte REG_ARRAY = 2;
const byte REG_BITMASK = 3;

/* Bitmasks for control registers */
#define BOZ_BC_ENABLED 1
#define BOZ_BC_ALLOW_BUZZ_WHEN_CLOCK_STOPPED 2
#define BOZ_BC_AUTO_UNLOCK 4

#define BOZ_BA_STOP_CLOCK_ON_BUZZ 1
#define BOZ_BA_START_CLOCK_ON_BUZZ 2
#define BOZ_BA_LOCKOUT_ON_BUZZ 4
#define BOZ_BA_SET_LED_ON_BUZZ 8

/* Various states we can be in when reading the individual bytes that make up
   a command */
#define PCC_EXPECT_TAG 1
#define PCC_PRE_REG_NAME 2
#define PCC_REG_NAME 3
#define PCC_REG_SUBSCRIPT 4
#define PCC_PRE_ARG 5
#define PCC_ARG 6

/* Register definition: everything we might want to know about a particular
   register, such as how many elements in its array (if it's an array), what
   its current value is, and what function to call to read and write it. */
struct reg_def {
    char reg_name[5];
    int array_size;
    reg *value;
    reg_read_fn reg_read;
    reg_write_fn reg_write;
};

/* The set of logical "registers" the host can set and get. Most of the things
   the host can do are accomplished by setting and getting the values of these
   registers. */
struct reg_values {
    reg cpb, cpc, cpdr, cpdc, cpdl, cpl, cps;
    reg cpv[2];
    reg bl;
    reg bc[BOZ_NUM_BUZZERS];
    reg ba[BOZ_NUM_BUZZERS];
    reg bzid;
    reg ls;
};
struct reg_values reg_vals;

reg reg_read_std(struct reg_def *reg_def, int subscript) {
    if (subscript < 0 || subscript >= reg_def->array_size)
        return 0;
    return reg_def->value[subscript];
}

void reg_write_noop(struct reg_def *reg_def, int subscript, reg value) {
    return;
}

void reg_write_std(struct reg_def *reg_def, int subscript, reg value) {
    if (subscript < 0 || subscript >= reg_def->array_size)
        return;
    reg_def->value[subscript] = value;
}

void reg_write_ls(struct reg_def *reg_def, int subscript, reg value) {
    reg_write_std(reg_def, subscript, value);
    boz_leds_set(value);
}

/* Register definitions. These must be in alphabetical order of register
   name, because we use a binary search to find the one we want.
*/
struct reg_def reg_defs[] = {
    /* Registers for controlling buzzer behaviour */
    { "BC", BOZ_NUM_BUZZERS, &reg_vals.bc[0], reg_read_std, reg_write_std },
    { "BL", 1, &reg_vals.bl, reg_read_std, reg_write_std },
    { "BZID", 1, &reg_vals.bzid, reg_read_std, reg_write_std },

    /* Capabilities: read-only registers so the host can find out what
       features we support */
    { "CPB", 1, &reg_vals.cpb, reg_read_std, reg_write_noop },
    { "CPC", 1, &reg_vals.cpc, reg_read_std, reg_write_noop },
    { "CPDC", 1, &reg_vals.cpdc, reg_read_std, reg_write_noop },
    { "CPDL", 1, &reg_vals.cpdl, reg_read_std, reg_write_noop },
    { "CPDR", 1, &reg_vals.cpdr, reg_read_std, reg_write_noop },
    { "CPL", 1, &reg_vals.cpl, reg_read_std, reg_write_noop },
    { "CPS", 1, &reg_vals.cps, reg_read_std, reg_write_noop },
    { "CPV", 2, &reg_vals.cpv[0], reg_read_std, reg_write_noop },

    /* LEDs: registers which allow the host to control the LEDs */
    { "LS", 1, &reg_vals.ls, reg_read_std, reg_write_ls },
};
const int reg_def_count = sizeof(reg_defs) / sizeof(reg_defs[0]);

struct pcc_cmd {
    char started;
    char tag;
    char reg_name[5];
    char reg_name_p;
    int subscript;
    int value;
    int value_minus;
    int state;
};

struct pcc_cmd cmd_state;

struct reg_def *pcc_find_reg(char *name) {
    int low, high;

    low = 0;
    high = reg_def_count;

    while (low < high) {
        int index = (low + high) / 2;
        struct reg_def *def = &reg_defs[index];
        int result = strcmp(name, def->reg_name);
        if (result < 0) {
            high = index;
        }
        else if (result > 0) {
            low = index + 1;
        }
        else {
            return def;
        }
    }
    return NULL;
}

/* Read a character that's been given to us and modify the state of
   struct pcc_cmd accordingly. If we see something we're not expecting, we'll
   ignore the rest of the command and send an error to the host. */
void
pcc_cmd_feed(struct pcc_cmd *cmd, int c) {
    switch (cmd->state) {
        case PCC_EXPECT_TAG:
            cmd->tag = (char) c;
            cmd->state = PCC_PRE_REG_NAME;
            break;

        case PCC_PRE_REG_NAME:
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                cmd->state = PCC_REG_NAME;
                // and fall through
            }
            else if (c == ' ') {
                break;
            }
            else {
                goto invalid_command;
            }

        case PCC_REG_NAME:
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                if ((size_t) cmd->reg_name_p < sizeof(cmd->reg_name) - 1)
                    cmd->reg_name[(int) cmd->reg_name_p++] = (char) (c & 0xdf);
                else
                    goto invalid_command;
                break;
            }
            else if (c >= '0' && c <= '9') {
                cmd->state = PCC_REG_SUBSCRIPT;
                // fall through
            }
            else if (c == ' ') {
                cmd->state = PCC_PRE_ARG;
                break;
            }
            else {
                goto invalid_command;
            }

        case PCC_REG_SUBSCRIPT:
            if (c >= '0' && c <= '9') {
                cmd->subscript *= 10;
                cmd->subscript += c - '0';
            }
            else if (c == ' ') {
                cmd->state = PCC_PRE_ARG;
            }
            else {
                goto invalid_command;
            }
            break;

        case PCC_PRE_ARG:
            if ((c >= '0' && c <= '9') || c == '-') {
                cmd->state = PCC_ARG;
                // fall through
            }
            else if (c == ' ') {
                break;
            }
            else {
                goto invalid_command;
            }

        case PCC_ARG:
            if (c >= '0' && c <= '9') {
                cmd->value *= 10;
                if (cmd->value_minus)
                    cmd->value -= c - '0';
                else
                    cmd->value += c - '0';
            }
            else if (c == '-') {
                cmd->value_minus = 1;
            }
            else {
                goto invalid_command;
            }
            break;
    }

    return;

invalid_command:
    cmd->state = 0;
}

static void pcc_send_error(char e) {
    char msg[3];
    msg[0] = '?';
    msg[1] = e;
    msg[2] = '\n';

    boz_serial_enqueue_data_out(msg, 3);
}

static int str_put_int(char *dest, int value) {
    if (value == -32768) {
        dest[0] = '-';
        dest[1] = '3';
        dest[2] = '2';
        dest[3] = '7';
        dest[4] = '6';
        dest[5] = '8';
        return 6;
    }
    else {
        char *orig_dest = dest;
        char *num_start;
        int l, r;
        if (value < 0) {
            *(dest++) = '-';
            value = -value;
        }
        num_start = dest;
        do {
            *(dest++) = (char) ((value % 10) + '0');
            value /= 10;
        } while (value != 0);

        for (l = 0, r = (dest - num_start) - 1; l < r; ++l, --r) {
            char tmp = num_start[l];
            num_start[l] = num_start[r];
            num_start[r] = tmp;
        }
        return dest - orig_dest;
    }
}

static void pcc_send_rw_result(char tag, char *reg_name, int subscript, reg value) {
    char msg[30] = "$  ";
    int msgp = 3;

    msg[1] = tag;

    for (int i = 0; reg_name[i]; ++i) {
        msg[msgp++] = reg_name[i];
    }

    msgp += str_put_int(msg + msgp, subscript);
    msg[msgp++] = ' ';

    msgp += str_put_int(msg + msgp, value);
    msg[msgp++] = '\n';
    boz_serial_enqueue_data_out(msg, msgp);
}

static void pcc_send_write_result(char *reg_name, int subscript, reg value) {
    pcc_send_rw_result('W', reg_name, subscript, value);
}

static void pcc_send_read_result(char *reg_name, int subscript, reg value) {
    pcc_send_rw_result('R', reg_name, subscript, value);
}

void pcc_cmd_execute(struct pcc_cmd *cmd) {
    cmd->reg_name[(int) cmd->reg_name_p] = '\0';
    if (cmd->state == 0) {
        /* Couldn't make head or tail of what the host said */
        pcc_send_error('?');
    }
    else if (cmd->tag == 'R' || cmd->tag == 'W') {
        struct reg_def *reg_def = pcc_find_reg(cmd->reg_name);
        if (reg_def == NULL) {
            /* Unrecognised register name */
            pcc_send_error('R');
        }
        else {
            if (cmd->tag == 'R') {
                /* Read register value */
                pcc_send_read_result(cmd->reg_name, cmd->subscript, reg_def->reg_read(reg_def, cmd->subscript));
            }
            else {
                /* Write register value */
                reg_def->reg_write(reg_def, cmd->subscript, cmd->value);
                pcc_send_write_result(cmd->reg_name, cmd->subscript, cmd->value);
            }
        }
    }
    else {
        /* Unrecognised command tag */
        pcc_send_error('T');
    }
    cmd->state = 0;
}

void pcc_data_available(void *arg) {
    struct pcc_cmd *cmd = (struct pcc_cmd *) arg;

    while (Serial.available() > 0) {
        int c = Serial.read();

        switch (c) {
            case '$':
                //boz_leds_set(1);
                memset(cmd, 0, sizeof(*cmd));
                cmd->state = PCC_EXPECT_TAG;
                break;

            case '\n':
                //boz_leds_set(2);
                pcc_cmd_execute(cmd);
                break;

            default:
                //boz_leds_set(4);
                if (cmd->state > 0)
                    pcc_cmd_feed(cmd, c);
        }
    }
}

static void buzz_event(int which_buzzer) {
    char msg[30];
    int msgp;

    if (which_buzzer < 0 || which_buzzer >= BOZ_NUM_BUZZERS)
        return;

    reg ba = reg_vals.ba[which_buzzer];

    reg_vals.bzid = which_buzzer;
    if (ba & BOZ_BA_LOCKOUT_ON_BUZZ) {
        reg_vals.bl |= (1 << (BOZ_NUM_BUZZERS)) - 1;
    }
    if (ba & BOZ_BA_SET_LED_ON_BUZZ) {
        boz_leds_set(1 << which_buzzer);
    }

    /* Send buzz event message */
    msg[0] = '!';
    msg[1] = 'B';
    msg[2] = ' ';

    msgp = 3;    
    msgp += str_put_int(msg + msgp, which_buzzer);
    msg[msgp++] = ' ';

    /* Clock associated with buzzer (not supported yet) */
    msg[msgp++] = '-';
    msg[msgp++] = '1';

    /* Time (ms) showing on clock */
    msg[msgp++] = ' ';
    msg[msgp++] = '0';

    msg[msgp++] = '\n';
    boz_serial_enqueue_data_out(msg, msgp);
}

static void buzz_handler(void *cookie, int which_buzzer) {
    reg which_bc;

    if (which_buzzer < 0 || which_buzzer >= BOZ_NUM_BUZZERS)
        return;

    which_bc = reg_vals.bc[which_buzzer];
    if ((which_bc & BOZ_BC_ENABLED) && !(reg_vals.bl & (1 << which_buzzer))) {
        buzz_event(which_buzzer);
    }
}

void pcc_init(void *dummy) {
    boz_display_clear();

    memset(&reg_vals, 0, sizeof(reg_vals));
    reg_vals.cpb = BOZ_NUM_BUZZERS;
    reg_vals.cpc = BOZ_NUM_CLOCKS;
    reg_vals.cpdr = BOZ_DISPLAY_ROWS;
    reg_vals.cpdc = BOZ_DISPLAY_COLUMNS;
    reg_vals.cpdl = 1;
    reg_vals.cpl = BOZ_NUM_LEDS;
    reg_vals.cps = 1;
    reg_vals.cpv[0] = 0x2019;
    reg_vals.cpv[1] = 0x1111;
    reg_vals.bzid = -1;
    for (int i = 0; i < BOZ_NUM_BUZZERS; ++i) {
        /* Default initial value for BC: buzzer enabled (bit 0 set), allow buzz
           when clock stopped (bit 1 set), don't automatically unlocked buzzers
           after they're locked (bit 2 clear), lock out other buzzers when
           a buzzer is pressed (bit 3 set) */
        reg_vals.bc[i] = BOZ_BC_ENABLED | BOZ_BC_ALLOW_BUZZ_WHEN_CLOCK_STOPPED;

        /* Default initial value for BA (action on buzz):
           Lock out other buzzers on buzz, and set the appropriate LED on buzz.
        */
        reg_vals.ba[i] = BOZ_BA_LOCKOUT_ON_BUZZ | BOZ_BA_SET_LED_ON_BUZZ;
    }

    boz_set_event_cookie(&cmd_state);
    boz_set_event_handler_serial_data_available(pcc_data_available);

    boz_set_event_handler_buzz(buzz_handler);
}

#endif
