#include "aht20.h"

static uint8_t AHT20_CalculateCRC8(const uint8_t *data, size_t len);
static AHT20_StatusTypedef AHT20_TriggerMeasurement(AHT20_HandleTypedef *dev);

static uint8_t AHT20_CalculateCRC8(const uint8_t *data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}

AHT20_StatusTypedef AHT20_Init(AHT20_HandleTypedef *dev,
                               I2C_HandleTypeDef *hi2c) {
  dev->hi2c = hi2c;
  dev->temperature = 0.0f;
  dev->humidity = 0.0f;

  if (HAL_I2C_IsDeviceReady(dev->hi2c, AHT20_I2C_ADDR, 1, 100) != HAL_OK) {
    return AHT20_ERR_I2C;
  }

  uint8_t status = 0;
  uint8_t cmd_status = 0x71;

  HAL_Delay(40);

  if (HAL_I2C_Master_Transmit(dev->hi2c, AHT20_I2C_ADDR, &cmd_status, 1, 100) !=
      HAL_OK) {
    return AHT20_ERR_I2C;
  }

  HAL_Delay(10);

  if (HAL_I2C_Master_Receive(dev->hi2c, AHT20_I2C_ADDR, &status, 1, 100) !=
      HAL_OK) {
    return AHT20_ERR_I2C;
  }

  // Check calibration bit (Bit 3)
  if ((status & 0x08) == 0) {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    HAL_Delay(10);

    if (HAL_I2C_Master_Transmit(dev->hi2c, AHT20_I2C_ADDR, init_cmd, 3, 100) !=
        HAL_OK) {
      return AHT20_ERR_I2C;
    }
    HAL_Delay(10);
  }

  return AHT20_OK;
}

static AHT20_StatusTypedef AHT20_TriggerMeasurement(AHT20_HandleTypedef *dev) {
  uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
  uint8_t status = 0;
  uint8_t retries = 10;

  if (HAL_I2C_Master_Transmit(dev->hi2c, AHT20_I2C_ADDR, measure_cmd, 3, 100) !=
      HAL_OK) {
    return AHT20_ERR_I2C;
  }

  HAL_Delay(80);

  // Poll busy bit (Bit 7)
  do {
    if (HAL_I2C_Master_Receive(dev->hi2c, AHT20_I2C_ADDR, &status, 1, 100) ==
        HAL_OK) {
      if ((status & 0x80) == 0) {
        return AHT20_OK;
      }
    }
    retries--;
    HAL_Delay(10);
  } while (retries > 0);

  return AHT20_ERR_TIMEOUT;
}

AHT20_StatusTypedef AHT20_ReadData(AHT20_HandleTypedef *dev) {
  AHT20_StatusTypedef status = AHT20_TriggerMeasurement(dev);
  if (status != AHT20_OK) {
    return status;
  }

  uint8_t buf[7];
  if (HAL_I2C_Master_Receive(dev->hi2c, AHT20_I2C_ADDR, buf, 7, 100) !=
      HAL_OK) {
    return AHT20_ERR_I2C;
  }

  if (AHT20_CalculateCRC8(buf, 6) != buf[6]) {
    return AHT20_ERR_CRC;
  }

  uint32_t raw_humidity = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) |
                          ((uint32_t)buf[3] >> 4);
  uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) |
                      ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];

  dev->humidity = ((float)raw_humidity / 1048576.0f) * 100.0f;
  dev->temperature = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;

  return AHT20_OK;
}
