#ifndef __AHT20_H
#define __AHT20_H

#include "stm32l4xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AHT20_I2C_ADDR (0x38 << 1)

typedef enum {
  AHT20_OK = 0,
  AHT20_ERR_INIT_FAIL = 1,
  AHT20_ERR_TIMEOUT = 2,
  AHT20_ERR_I2C = 3,
  AHT20_ERR_CRC = 4
} AHT20_StatusTypedef;

typedef struct {
  I2C_HandleTypeDef *hi2c;
  float temperature;
  float humidity;
} AHT20_HandleTypedef;

AHT20_StatusTypedef AHT20_Init(AHT20_HandleTypedef *dev, I2C_HandleTypeDef *hi2c);
AHT20_StatusTypedef AHT20_ReadData(AHT20_HandleTypedef *dev);

#endif /* __AHT20_H */
