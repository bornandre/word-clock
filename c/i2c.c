#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "i2c.h"

static int i2cfd = -1;

int i2c_open_bus(void) {
    if (i2cfd >= 0) return 0; /* schon offen */

    i2cfd = open(I2C_BUS_PATH, O_RDWR);
    if (i2cfd < 0) {
        perror("open " I2C_BUS_PATH);
        return -1;
    }
    return 0;
}

void i2c_close_bus(void) {
    if (i2cfd >= 0) {
        close(i2cfd);
        i2cfd = -1;
    }
}

int i2c_select_device(uint8_t dev) {
    if (i2cfd < 0) {
        errno = EBADF;
        return -1;
    }
    if (ioctl(i2cfd, I2C_SLAVE, dev) < 0) {
        return -1;
    }
    return 0;
}

int i2c_write_bytes(uint8_t dev, const uint8_t *data, int len) {
    if (i2c_select_device(dev) < 0) return -1;

    ssize_t res = write(i2cfd, data, (size_t)len);
    if (res != (ssize_t)len) {
        if (res >= 0) errno = EIO;
        return -1;
    }
    return 0;
}

int i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    return i2c_write_bytes(dev, buf, 2);
}
