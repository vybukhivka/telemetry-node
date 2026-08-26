#ifndef __BMP280_H
#define __BMP280_H

#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define BMP280_I2C_ADDR         (0x77 << 1)

typedef struct {
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;

  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;
} BMP280_CalibData;

typedef struct {
  I2C_HandleTypeDef *hi2c;
  BMP280_CalibData  calib;
  int32_t           t_fine;
} BMP280_HandleTypedef;

bool BMP280_Init(BMP280_HandleTypedef *dev, I2C_HandleTypeDef *hi2c);
bool BMP280_ReadRaw(BMP280_HandleTypedef *dev, int32_t *adc_T, int32_t *adc_P);
int32_t BMP280_Compensate_T(BMP280_HandleTypedef *dev, int32_t adc_T);
uint32_t BMP280_Compensate_P(BMP280_HandleTypedef *dev, int32_t adc_P);

#endif /* __BMP280_H */
