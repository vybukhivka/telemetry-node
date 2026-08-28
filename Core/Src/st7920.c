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
ST7920_ConvertHALStatus(HAL_StatusTypeDef hal_status);
static ST7920_StatusTypeDef ST7920_Write(uint8_t isData, uint8_t payload);

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

static ST7920_StatusTypeDef ST7920_Write(uint8_t isData, uint8_t payload) {
  uint8_t packet[3];
  packet[0] = isData ? 0xFA : 0xF8;
  packet[1] = payload & 0xF0;
  packet[2] = (payload << 4) & 0xF0;

  HAL_GPIO_WritePin(ST7920_CS_PORT, ST7920_CS_PIN, GPIO_PIN_SET);
  ST7920_DelayUs(2);

  HAL_StatusTypeDef hal_status =
      HAL_SPI_Transmit(&hspi2, packet, 3, ST7920_TIMEOUT_MS);
  ST7920_DelayUs(2);

  HAL_GPIO_WritePin(ST7920_CS_PORT, ST7920_CS_PIN, GPIO_PIN_RESET);
  ST7920_DelayUs(5);

  return ST7920_ConvertHALStatus(hal_status);
}

ST7920_StatusTypeDef ST7920_SendCmd(uint8_t cmd) {
  return ST7920_Write(0, cmd);
}

ST7920_StatusTypeDef ST7920_SendData(uint8_t cmd) {
  return ST7920_Write(1, cmd);
}

ST7920_StatusTypeDef ST7920_Init(void) {
  ST7920_StatusTypeDef status;

  HAL_Delay(50); // Power-on

  status = ST7920_SendCmd(0x30); // Basic instruction set
  if (status != ST7920_OK)
    return status;
  HAL_Delay(1);

  status = ST7920_SendCmd(0x0C); // Display ON, Cursor OFF
  if (status != ST7920_OK)
    return status;
  HAL_Delay(1);

  status = ST7920_SendCmd(0x01); // Clear display
  if (status != ST7920_OK)
    return status;
  HAL_Delay(1);

  status = ST7920_SendCmd(0x06); // Basic instruction set
  if (status != ST7920_OK)
    return status;
  HAL_Delay(1);

  return ST7920_OK;
}

ST7920_StatusTypeDef ST7920_SetCursor(uint8_t row, uint8_t col) {
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
  return ST7920_SendCmd(addr);
}

ST7920_StatusTypeDef ST7920_SendString(uint8_t row, uint8_t col, char *str) {
  ST7920_StatusTypeDef status;
  status = ST7920_SetCursor(row, col);
  if (status != ST7920_OK)
    return status;

  while (*str) {
    status = ST7920_SendData((uint8_t)(*str++));
    if (status != ST7920_OK)
      return status;
  }
  return ST7920_OK;
}

ST7920_StatusTypeDef ST7920_Clear(void) {
  ST7920_StatusTypeDef status = ST7920_SendCmd(0x01);
  HAL_Delay(10);
  return status;
}
