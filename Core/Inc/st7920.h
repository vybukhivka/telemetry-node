#ifndef ST7920_H
#define ST7920_H

#include <stdint.h>
#include "main.h"

#define ST7920_CS_PORT GPIOB
#define ST7920_CS_PIN GPIO_PIN_12

typedef enum {
	ST7920_OK = 0x00,
	ST7920_ERROR = 0x01,
	ST7920_TIMEOUT = 0x02,
} ST7920_StatusTypeDef;

ST7920_StatusTypeDef ST7920_Init(void);
ST7920_StatusTypeDef ST7920_SendCmd(uint8_t cmd);
ST7920_StatusTypeDef ST7920_SendData(uint8_t data);
ST7920_StatusTypeDef ST7920_SetCursor(uint8_t row, uint8_t col);
ST7920_StatusTypeDef ST7920_SendString(uint8_t row, uint8_t col, char *str);
ST7920_StatusTypeDef ST7920_Clear(void);

#endif /* ST9720_H */
