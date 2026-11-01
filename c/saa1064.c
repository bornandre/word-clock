#include "saa1064.h"
#include "i2c.h"

static uint8_t control = SAA1064_CTRL_DEFAULT;

void saa1064_set_control(uint8_t ctrl) {
    control = ctrl;
}

/*
 * Schreibt Control-Byte und alle 4 Digits in EINER I2C-Übertragung
 * (der SAA1064 erhöht die Subadresse automatisch).
 * Das Control-Byte wird jedes Mal mitgeschickt: Falls ein Chip durch
 * einen Spannungseinbruch zurückgesetzt wurde (Control = 0 -> dunkel),
 * ist er beim nächsten Update wieder korrekt konfiguriert.
 */
int saa1064_write_digits(uint8_t dev,
                         uint8_t r1, uint8_t r2,
                         uint8_t r3, uint8_t r4) {
    uint8_t buf[6] = { SAA1064_REG_CONTROL, control, r1, r2, r3, r4 };
    return i2c_write_bytes(dev, buf, (int)sizeof buf);
}

int saa1064_clear(uint8_t dev) {
    return saa1064_write_digits(dev, 0, 0, 0, 0);
}

int saa1064_init(uint8_t dev) {
    return saa1064_clear(dev);
}
