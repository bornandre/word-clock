#ifndef SAA1064_H
#define SAA1064_H

#include <stdint.h>

/* Subadressen */
#define SAA1064_REG_CONTROL 0x00
#define SAA1064_REG_DIG1    0x01
#define SAA1064_REG_DIG2    0x02
#define SAA1064_REG_DIG3    0x03
#define SAA1064_REG_DIG4    0x04

/* Bits im Control-Register */
#define SAA1064_CTRL_DYNAMIC  0x01  /* Multiplex-Betrieb (4 Digits)   */
#define SAA1064_CTRL_DIG13_ON 0x02  /* Digits 1+3 eingeschaltet        */
#define SAA1064_CTRL_DIG24_ON 0x04  /* Digits 2+4 eingeschaltet        */
#define SAA1064_CTRL_TEST     0x08  /* Segmenttest: alle LEDs an       */
#define SAA1064_CTRL_3MA      0x10  /* +3 mA Segmentstrom              */
#define SAA1064_CTRL_6MA      0x20  /* +6 mA                           */
#define SAA1064_CTRL_12MA     0x40  /* +12 mA  (Summe max. 21 mA)      */

/* Standard = wie bisher 0x17: Multiplex, alle Digits an, 3 mA.
 * Heller z.B. mit: make CFLAGS_EXTRA='-DSAA1064_CTRL_DEFAULT=0x47'  (12 mA) */
#ifndef SAA1064_CTRL_DEFAULT
#define SAA1064_CTRL_DEFAULT (SAA1064_CTRL_DYNAMIC | SAA1064_CTRL_DIG13_ON | \
                              SAA1064_CTRL_DIG24_ON | SAA1064_CTRL_3MA)
#endif

void saa1064_set_control(uint8_t ctrl);

/* Rückgabe 0 = ok, -1 = Chip nicht erreichbar */
int saa1064_init(uint8_t dev);
int saa1064_clear(uint8_t dev);
int saa1064_write_digits(uint8_t dev,
                         uint8_t r1, uint8_t r2,
                         uint8_t r3, uint8_t r4);

#endif
