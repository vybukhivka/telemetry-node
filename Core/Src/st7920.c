#include "st7920.h"
#include "spi.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_spi.h"
#include <stdint.h>

extern SPI_HandleTypeDef hspi2;

#define ST7920_TIMEOUT_MS 100

static void ST7920_DelayUs(uint32_t us) {
  uint32_t count = us * (SystemCoreClock / 1000000U) / 5U;
  while (count--) {
    __NOP();
  }
}

static ST7920_StatusTypeDef
ST7920_ConvertHALStatus(HAL_StatusTypeDef hal_status) {
  switch (hal_status) {
  case HAL_OK:
    return ST7920_OK;
  case HAL_TIMEOUT:
    return ST7920_TIMEOUT;
  default:
    return ST7920_ERROR;
  }
}

static ST7920_StatusTypeDef ST7920_Write(ST7920_HandleTypedef *dev,
                                         uint8_t isData, uint8_t payload) {
  uint8_t packet[3];
  packet[0] = isData ? 0xFA : 0xF8;
  packet[1] = payload & 0xF0;
  packet[2] = (payload << 4) & 0xF0;

  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
  ST7920_DelayUs(2);

  HAL_StatusTypeDef hal_status =
      HAL_SPI_Transmit(dev->hspi, packet, 3, ST7920_TIMEOUT_MS);
  ST7920_DelayUs(2);

  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
  ST7920_DelayUs(5);

  return ST7920_ConvertHALStatus(hal_status);
}

ST7920_StatusTypeDef ST7920_SendCmd(ST7920_HandleTypedef *dev, uint8_t cmd) {
  return ST7920_Write(dev, 0, cmd);
}

ST7920_StatusTypeDef ST7920_SendData(ST7920_HandleTypedef *dev, uint8_t data) {
  return ST7920_Write(dev, 1, data);
}

ST7920_StatusTypeDef ST7920_Init(ST7920_HandleTypedef *dev,
                                 SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port,
                                 uint16_t cs_pin) {
  dev->hspi = hspi;
  dev->cs_port = cs_port;
  dev->cs_pin = cs_pin;

  HAL_Delay(50); // Power-on wait

  if (ST7920_SendCmd(dev, 0x30) != ST7920_OK)
    return ST7920_ERROR; // Basic instruction set
  HAL_Delay(1);

  if (ST7920_SendCmd(dev, 0x0C) != ST7920_OK)
    return ST7920_ERROR; // Display ON, Cursor OFF
  HAL_Delay(1);

  if (ST7920_SendCmd(dev, 0x01) != ST7920_OK)
    return ST7920_ERROR; // Clear display
  HAL_Delay(10);         // ST7920 Clear requires >1.6ms execution time

  if (ST7920_SendCmd(dev, 0x06) != ST7920_OK)
    return ST7920_ERROR; // Entry mode set
  HAL_Delay(1);

  return ST7920_OK;
}

ST7920_StatusTypeDef ST7920_SetCursor(ST7920_HandleTypedef *dev, uint8_t row,
                                      uint8_t col) {
  uint8_t addr;
  switch (row) {
  case 0:
    addr = 0x80 + col;
    break;
  case 1:
    addr = 0x90 + col;
    break;
  case 2:
    addr = 0x88 + col;
    break;
  case 3:
    addr = 0x98 + col;
    break;
  default:
    return ST7920_ERROR;
  }
  return ST7920_SendCmd(dev, addr);
}

ST7920_StatusTypeDef ST7920_SendString(ST7920_HandleTypedef *dev, uint8_t row,
                                       uint8_t col, const char *str) {
  ST7920_StatusTypeDef status = ST7920_SetCursor(dev, row, col);
  if (status != ST7920_OK)
    return status;

  while (*str) {
    status = ST7920_SendData(dev, (uint8_t)(*str++));
    if (status != ST7920_OK)
      return status;
  }
  return ST7920_OK;
}

ST7920_StatusTypeDef ST7920_Clear(ST7920_HandleTypedef *dev) {
  ST7920_StatusTypeDef status = ST7920_SendCmd(dev, 0x01);
  HAL_Delay(10);
  return status;
}
