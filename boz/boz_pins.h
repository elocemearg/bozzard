#ifndef _BOZ_PINS_H
#define _BOZ_PINS_H

#include "boz_hw.h"

/* Pin assignments */

#if BOZ_HW_REVISION == 0

/* Hardware revision 0 - the original prototype */
#define PIN_SPEAKER     A0
#define PIN_SR_DATA     A1
#define PIN_SR_LATCH    A2
#define PIN_SR_CLOCK    A3
#define PIN_DISP_E      A4
#define PIN_DISP_RS     A5
#define PIN_BATTERY_SENSOR A6

#define PIN_BUTTON_INT   2
#define PIN_QM_RE_CLOCK  3
#define PIN_BUZZER_0     4
#define PIN_BUZZER_1     5
#define PIN_BUZZER_2     6
#define PIN_BUZZER_3     7
#define PIN_QM_PLAY      8
#define PIN_QM_YELLOW    9
#define PIN_QM_RESET    10
#define PIN_QM_RE_DATA  11
#define PIN_QM_RE_KEY   12

#else

/* Hardware revision 1 */
#define PIN_LED_R       A0
#define PIN_LED_G       A1
#define PIN_LED_Y       A2
#define PIN_LED_B       A3
#define PIN_I2C_SDA     A4
#define PIN_I2C_SCL     A5
#define PIN_BATTERY_SENSOR A6

#define PIN_BUTTON_INT   2
#define PIN_QM_RE_CLOCK  3
#define PIN_BUZZER_0     4
#define PIN_BUZZER_1     5
#define PIN_BUZZER_2     6
#define PIN_BUZZER_3     7
#define PIN_QM_PLAY      8
#define PIN_QM_YELLOW    9
#define PIN_QM_RESET    10
#define PIN_QM_RE_DATA  11
#define PIN_QM_RE_KEY   12
#define PIN_SPEAKER     13
#endif

#endif
