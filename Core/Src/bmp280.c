#include "bmp280.h"

#define BMP280_REG_ID 0xD0
#define BMP280_REG_RESET 0xE0
#define BMP280_REG_STATUS 0xF3
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_DATA 0xF7
#define BMP280_REG_CALIB 0x88

#define BMP280_CHIP_ID 0x58

static BMP280_StatusTypedef BMP280_ReadCalibData(BMP280_HandleTypedef *dev) {
  uint8_t calib_buf[24];
  if (HAL_I2C_Mem_Read(dev->hi2c, BMP280_I2C_ADDR, BMP280_REG_CALIB,
                       I2C_MEMADD_SIZE_8BIT, calib_buf, 24, 100) != HAL_OK) {
    return BMP280_ERR_I2C;
  }

  dev->calib.dig_T1 = (uint16_t)(calib_buf[1] << 8 | calib_buf[0]);
  dev->calib.dig_T2 = (int16_t)(calib_buf[3] << 8 | calib_buf[2]);
  dev->calib.dig_T3 = (int16_t)(calib_buf[5] << 8 | calib_buf[4]);
  dev->calib.dig_P1 = (uint16_t)(calib_buf[7] << 8 | calib_buf[6]);
  dev->calib.dig_P2 = (int16_t)(calib_buf[9] << 8 | calib_buf[8]);
  dev->calib.dig_P3 = (int16_t)(calib_buf[11] << 8 | calib_buf[10]);
  dev->calib.dig_P4 = (int16_t)(calib_buf[13] << 8 | calib_buf[12]);
  dev->calib.dig_P5 = (int16_t)(calib_buf[15] << 8 | calib_buf[14]);
  dev->calib.dig_P6 = (int16_t)(calib_buf[17] << 8 | calib_buf[16]);
  dev->calib.dig_P7 = (int16_t)(calib_buf[19] << 8 | calib_buf[18]);
  dev->calib.dig_P8 = (int16_t)(calib_buf[21] << 8 | calib_buf[20]);
  dev->calib.dig_P9 = (int16_t)(calib_buf[23] << 8 | calib_buf[22]);

  return BMP280_OK;
}

BMP280_StatusTypedef BMP280_Init(BMP280_HandleTypedef *dev,
                                 I2C_HandleTypeDef *hi2c) {
  dev->hi2c = hi2c;
  dev->t_fine = 0;

  uint8_t chip_id = 0;
  if (HAL_I2C_Mem_Read(dev->hi2c, BMP280_I2C_ADDR, BMP280_REG_ID,
                       I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 100) != HAL_OK) {
    return BMP280_ERR_I2C;
  }

  if (chip_id != BMP280_CHIP_ID) {
    return BMP280_ERR_ID_MISMATCH;
  }

  BMP280_StatusTypedef calib_status = BMP280_ReadCalibData(dev);
  if (calib_status != BMP280_OK) {
    return calib_status;
  }

  // ctrl_meas: osrs_t x2, osrs_p x16, normal mode -> 0x57
  uint8_t ctrl_meas = 0x57;
  if (HAL_I2C_Mem_Write(dev->hi2c, BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS,
                        I2C_MEMADD_SIZE_8BIT, &ctrl_meas, 1, 100) != HAL_OK) {
    return BMP280_ERR_I2C;
  }

  // config: t_sb 0.5ms, filter 16 -> 0x10
  uint8_t config = 0x10;
  if (HAL_I2C_Mem_Write(dev->hi2c, BMP280_I2C_ADDR, BMP280_REG_CONFIG,
                        I2C_MEMADD_SIZE_8BIT, &config, 1, 100) != HAL_OK) {
    return BMP280_ERR_I2C;
  }

  return BMP280_OK;
}

BMP280_StatusTypedef BMP280_ReadRaw(BMP280_HandleTypedef *dev, int32_t *adc_T,
                                    int32_t *adc_P) {
  uint8_t data[6];
  if (HAL_I2C_Mem_Read(dev->hi2c, BMP280_I2C_ADDR, BMP280_REG_DATA,
                       I2C_MEMADD_SIZE_8BIT, data, 6, 100) != HAL_OK) {
    return BMP280_ERR_I2C;
  }

  *adc_P = (int32_t)(((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) |
                     ((uint32_t)data[2] >> 4));
  *adc_T = (int32_t)(((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) |
                     ((uint32_t)data[5] >> 4));

  return BMP280_OK;
}

int32_t BMP280_Compensate_T(BMP280_HandleTypedef *dev, int32_t adc_T) {
  int32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((int32_t)dev->calib.dig_T1 << 1))) *
          ((int32_t)dev->calib.dig_T2)) >>
         11;
  var2 = (((((adc_T >> 4) - ((int32_t)dev->calib.dig_T1)) *
            ((adc_T >> 4) - ((int32_t)dev->calib.dig_T1))) >>
           12) *
          ((int32_t)dev->calib.dig_T3)) >>
         14;
  dev->t_fine = var1 + var2;
  T = (dev->t_fine * 5 + 128) >> 8;
  return T;
}

uint32_t BMP280_Compensate_P(BMP280_HandleTypedef *dev, int32_t adc_P) {
  int64_t var1, var2, p;
  var1 = ((int64_t)dev->t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)dev->calib.dig_P6;
  var2 = var2 + ((var1 * (int64_t)dev->calib.dig_P5) << 17);
  var2 = var2 + (((int64_t)dev->calib.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)dev->calib.dig_P3) >> 8) +
         ((var1 * (int64_t)dev->calib.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib.dig_P1) >> 33;

  if (var1 == 0) {
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)dev->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)dev->calib.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib.dig_P7) << 4);
  return (uint32_t)p;
}
