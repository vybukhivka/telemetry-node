#ifndef __MPU6050_H
#define __MPU6050_H

#include "stm32l4xx_hal.h"
#include <stdint.h>
#define MPU6050_I2C_ADDR (0x68 << 1)

typedef enum {
	MPU6050_OK = 0,
	MPU6050_ERR_I2C = 1,
	MPU6050_ERR_ID_MISMATCH = 2,
} MPU6050_StatusTypedef;

typedef struct {
	I2C_HandleTypeDef *hi2c;

	int16_t Accel_X_RAW;
	int16_t Accel_Y_RAW;
	int16_t Accel_Z_RAW;
	int16_t Gyro_X_RAW;
	int16_t Gyro_Y_RAW;
	int16_t Gyro_Z_RAW;

	int32_t Accel_X_Offset;
	int32_t Accel_Y_Offset;
	int32_t Accel_Z_Offset;
	int32_t Gyro_X_Offset;
	int32_t Gyro_Y_Offset;
	int32_t Gyro_Z_Offset;

	float Ax, Ay, Az;
	float Gx, Gy, Gz;
} MPU6050_HandleTypedef;

MPU6050_StatusTypedef MPU6050_Init(MPU6050_HandleTypedef *dev, I2C_HandleTypeDef *hi2c);
MPU6050_StatusTypedef MPU6050_Calibrate(MPU6050_HandleTypedef *dev, uint16_t num_samples);
MPU6050_StatusTypedef MPU6050_ReadAll(MPU6050_HandleTypedef *dev);

#endif /* __MPU6050_H */
