#ifndef _BOZ_HW
#define _BOZ_HW

/* Define BOZ_HW_REVISION as 0 if this is the software for the original Bozzard
 * hardware with the directly-connected display (no I2C), the buttons
 * connecting the I/O pins to ground rather than connecting them to the
 * interrupt pin D2, and the speaker on A0 rather than D13.
 *
 * BOZ_HW_REVISION == 1 is the second hardware revision, where we communicate
 * with the display using I2C, there's no shift register, and the buttons
 * connect the I/O pins with the interrupt pin D2. */

#define BOZ_HW_REVISION 1

/* Define BOZ_SERIAL to include the PC Control app, and all the various
 * serial port-related code. */
//#define BOZ_SERIAL

#endif
