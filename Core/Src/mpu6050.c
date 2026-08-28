#include "mpu6050.h"
#include "stm32l4xx_hal_i2c.h"
#include <stdint.h>

#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_PWR_MNGMT_1 0x6B
#define MPU6050_WHO_AM_I_VAL 0x68

MPU6050_StatusTypedef MPU6050_Init(MPU6050_HandleTypedef *dev,
                                   I2C_HandleTypeDef *hi2c) {
  dev->hi2c = hi2c;
  uint8_t who_am_i = 0;

  if (HAL_I2C_Mem_Read(dev->hi2c, MPU6050_I2C_ADDR, I2C_MEMADD_SIZE_8BIT,
                       MPU6050_REG_WHO_AM_I, &who_am_i, 1, 100) != HAL_OK) {
    return MPU6050_ERR_I2C;
  }

  if (who_am_i != MPU6050_WHO_AM_I_VAL) {
    return MPU6050_ERR_ID_MISMATCH;
  }

  uint8_t pwr_mngmt = 0x00;
  if (HAL_I2C_Mem_Write(dev->hi2c, MPU6050_I2C_ADDR, I2C_MEMADD_SIZE_8BIT,
                        MPU6050_REG_PWR_MNGMT_1, &pwr_mngmt, 1,
                        100) != HAL_OK) {
    return MPU6050_ERR_I2C;
  }

  return MPU6050_OK;
}
