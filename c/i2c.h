#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#define I2C_BUS_PATH "/dev/i2c-1"

/* Bus öffnen / schliessen */
int  i2c_open_bus(void);
void i2c_close_bus(void);

/* I2C-Device auswählen */
int i2c_select_device(uint8_t dev);

/* Daten schreiben – Rückgabe 0 = ok, -1 = Fehler (errno gesetzt).
 * Schreibfehler werden NICHT selbst ausgegeben, damit ein fehlender
 * Chip nicht jede Sekunde das Log flutet. */
int i2c_write_bytes(uint8_t dev, const uint8_t *data, int len);
int i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t value);

#endif
