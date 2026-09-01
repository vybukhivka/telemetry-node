#ifndef ST7920_H
#define ST7920_H

#include <stdint.h>
#include "main.h"
#include "stm32l4xx_hal_spi.h"

#define ST7920_CS_PORT GPIOB
#define ST7920_CS_PIN GPIO_PIN_12

typedef enum {
	ST7920_OK = 0x00,
	ST7920_ERROR = 0x01,
	ST7920_TIMEOUT = 0x02,
} ST7920_StatusTypeDef;

typedef struct {
	SPI_HandleTypeDef *hspi;
	GPIO_TypeDef *cs_port;
	uint16_t cs_pin;
} ST7920_HandleTypedef;

ST7920_StatusTypeDef ST7920_Init(ST7920_HandleTypedef *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
ST7920_StatusTypeDef ST7920_SendCmd(ST7920_HandleTypedef *dev, uint8_t cmd);
ST7920_StatusTypeDef ST7920_SendData(ST7920_HandleTypedef *dev, uint8_t data);
ST7920_StatusTypeDef ST7920_SetCursor(ST7920_HandleTypedef *dev, uint8_t row, uint8_t col);
ST7920_StatusTypeDef ST7920_SendString(ST7920_HandleTypedef *dev, uint8_t row, uint8_t col, const char *str);
ST7920_StatusTypeDef ST7920_Clear(ST7920_HandleTypedef *dev);

#endif /* ST9720_H */
