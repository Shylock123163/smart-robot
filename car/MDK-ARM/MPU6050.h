#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"
#include "i2c.h"

/* MPU6050 I2C地址（AD0接GND = 0x68，左移1位变0xD0）*/
#define MPU6050_ADDR        0xD0

/* 寄存器地址 */
#define MPU6050_PWR_MGMT_1  0x6B
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_GYRO_ZOUT_H 0x47
#define MPU6050_WHO_AM_I    0x75

/* 陀螺仪量程：±500°/s，灵敏度65.5 LSB/(°/s) */
#define GYRO_SENSITIVITY    65.5f

/* 对外接口 */
uint8_t MPU6050_Init(void);
float   MPU6050_GetGyroZ(void);
void    MPU6050_ResetYaw(void);
float   MPU6050_GetYaw(void);

#endif