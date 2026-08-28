#include "mpu6050.h"
#include "stm32l4xx_hal_i2c.h"
#include <stdint.h>

#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I_VAL 0x68

MPU6050_StatusTypedef MPU6050_Init(MPU6050_HandleTypedef *dev,
                                   I2C_HandleTypeDef *hi2c) {
  dev->hi2c = hi2c;
  uint8_t who_am_i = 0;

  if (HAL_I2C_Mem_Read(dev->hi2c, MPU6050_I2C_ADDR, MPU6050_REG_WHO_AM_I,
                       I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, 100) != HAL_OK) {
    return MPU6050_ERR_I2C;
  }

  if (who_am_i != MPU6050_WHO_AM_I_VAL) {
    return MPU6050_ERR_ID_MISMATCH;
  }

  uint8_t pwr_mgmt =
      0x01; // wake up sensor and set clock source to X-axis Gyro PPL
  if (HAL_I2C_Mem_Write(dev->hi2c, MPU6050_I2C_ADDR, MPU6050_REG_PWR_MGMT_1,
                        I2C_MEMADD_SIZE_8BIT, &pwr_mgmt, 1, 100) != HAL_OK) {
    return MPU6050_ERR_I2C;
  }

  return MPU6050_OK;
}

MPU6050_StatusTypedef MPU6050_Calibrate(MPU6050_HandleTypedef *dev,
                                        uint16_t num_samples) {
  if (num_samples == 0)
    return MPU6050_ERR_I2C;

  dev->Accel_X_Offset = 0;
  dev->Accel_Y_Offset = 0;
  dev->Accel_Z_Offset = 0;
  dev->Gyro_X_Offset = 0;
  dev->Gyro_Y_Offset = 0;
  dev->Gyro_Z_Offset = 0;

  int32_t ax_sum = 0, ay_sum = 0, az_sum = 0;
  int32_t gx_sum = 0, gy_sum = 0, gz_sum = 0;

  for (uint16_t i = 0; i < num_samples; i++) {
    if (MPU6050_ReadAll(dev) != MPU6050_OK) {
      return MPU6050_ERR_I2C;
    }

    ax_sum += dev->Accel_X_RAW;
    ay_sum += dev->Accel_Y_RAW;
    az_sum += dev->Accel_Z_RAW;

    gx_sum += dev->Gyro_X_RAW;
    gy_sum += dev->Gyro_Y_RAW;
    gz_sum += dev->Gyro_Z_RAW;

    HAL_Delay(3); // wait for new sample, based on output rate
  }

  // Average the accumulated values
  dev->Accel_X_Offset = ax_sum / num_samples;
  dev->Accel_Y_Offset = ay_sum / num_samples;
  // Subtract 1g (16384 LSB at +/-2g range) from Z axis gravity
  dev->Accel_Z_Offset = (az_sum / num_samples) - 16384;

  dev->Gyro_X_Offset = gx_sum / num_samples;
  dev->Gyro_Y_Offset = gy_sum / num_samples;
  dev->Gyro_Z_Offset = gz_sum / num_samples;

  return MPU6050_OK;
}
