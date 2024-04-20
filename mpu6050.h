/**
 ******************************************************************************
 * @file    mpu6050.h
 * @author  Rafael Martins
 ******************************************************************************
 */

#ifndef MPU6050_H_
#define MPU6050_H_
#include <stdint.h>

int mpu6050_init();
void mpu6050_finish();
int mpu6050_get_accel(float *x, float *y, float *z);
int mpu6050_get_gyro(float *x, float *y, float *z);

#endif /* MPU6050_H_ */