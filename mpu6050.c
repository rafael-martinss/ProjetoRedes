#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "mpu6050.h"

// Device BUS
#define I2C_BUS "/dev/i2c-1"

// Device address
#define MPU6050_ADDR 0x68

// Device Registers
#define PWR_MGMT_1   0x6B
#define SMPLRT_DIV   0x19
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define INT_ENABLE   0x38
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H  0x43
#define GYRO_YOUT_H  0x45
#define GYRO_ZOUT_H  0x47

// File descriptor for I2C data
static int m_i2c_fd = 0;

// Read from: https://www.electronicwings.com/raspberry-pi/mpu6050-accelerometergyroscope-interfacing-with-raspberry-pi

static int i2c_initilize() {

    if ((m_i2c_fd = open(I2C_BUS, O_RDWR)) < 0) {
        perror("Failed to open the bus.");
        return 1;
    }
    return 0;
}

static int mpu6050_config_register(uint8_t reg, uint8_t value) {
    if (write(m_i2c_fd, &value, 1) != 1) {
        printf("Failed to write data to register %d\n", (int)reg);
        return 1;
    }
    return 0;
}

static int mpu6050_get_data_from(uint8_t reg, uint16_t *value) {
    uint8_t buf[2] = {0};

    buf[0] = reg;
    if (write(m_i2c_fd, buf, 1) != 1) {
        printf("Failed to select the data register %d\n", (int)reg);
        return 1;
    }
    if (read(m_i2c_fd, buf, 2) != 2) {
        printf("Failed to read the data from the register %d\n", (int)reg);
        return 1;
    }
    *value = (buf[0] << 8) | buf[1];
    return 0;
}

int mpu6050_init() {
    int err;

    // Initialize the BUS
    err = i2c_initilize();
    if (err)
        return err;
    
    // Connect to device
    if (ioctl(m_i2c_fd, I2C_SLAVE, MPU6050_ADDR) < 0) {
        perror("Failed to connect to the sensor.");
        return 1;
    }

    err |= mpu6050_config_register(SMPLRT_DIV, 0x07);	/* Write to sample rate register */
	err |= mpu6050_config_register(PWR_MGMT_1, 0x01);	/* Write to power management register */
	err |= mpu6050_config_register(CONFIG, 0);		/* Write to Configuration register */
	err |= mpu6050_config_register(GYRO_CONFIG, 24);	/* Write to Gyro Configuration register */
	err |= mpu6050_config_register(INT_ENABLE, 0x01);	/*Write to interrupt enable register */

    return err;
}

void mpu6050_finish() {
    close(m_i2c_fd);
}

int mpu6050_get_accel(float *x, float *y, float *z) {
    int err = 0;
    uint16_t acc_x, acc_y, acc_z = 0;
    err = mpu6050_get_data_from(ACCEL_XOUT_H, &acc_x);
    err |= mpu6050_get_data_from(ACCEL_YOUT_H, &acc_y);
    err |= mpu6050_get_data_from(ACCEL_ZOUT_H, &acc_z);

    if(!err) {
        *x = acc_x/16384.0;
        *y = acc_y/16384.0;
        *z = acc_z/16384.0;
    }
		
    return err;
}

int mpu6050_get_gyro(float *x, float *y, float *z) {
    int err = 0;
    uint16_t gyro_x, gyro_y, gyro_z = 0;
    err  = mpu6050_get_data_from(GYRO_XOUT_H, &gyro_x);
    err |= mpu6050_get_data_from(GYRO_YOUT_H, &gyro_y);
    err |= mpu6050_get_data_from(GYRO_ZOUT_H, &gyro_z);

    if(!err){
        *x = gyro_x/131;
		*y = gyro_y/131;
		*z = gyro_z/131;
    }
    return err;
}
