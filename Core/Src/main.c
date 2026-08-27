/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "bmp280.h"
#include "gpio.h"
#include "i2c.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_i2c.h"
#include "usart.h"
#include <stdint.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2C_AHT20_ADDRESS (0x38 << 1)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
BMP280_HandleTypedef bmp280;

bool AHT20_Calibration(I2C_HandleTypeDef *hi2c) {
  uint8_t status = 0;
  uint8_t cmd_status = 0x71;

  HAL_Delay(40);

  if (HAL_I2C_Master_Transmit(hi2c, I2C_AHT20_ADDRESS, &cmd_status, 1, 100) !=
      HAL_OK) {
    printf("Master Transmit status byte error!\r\n");
    return 1;
  }

  HAL_Delay(10);

  if (HAL_I2C_Master_Receive(hi2c, I2C_AHT20_ADDRESS, &status, 1, 100) !=
      HAL_OK) {
    printf("Master Receive status byte error!\r\n");
    return 1;
  }

  if ((status & 0x08) == 0) {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};

    HAL_Delay(10);

    if (HAL_I2C_Master_Transmit(hi2c, I2C_AHT20_ADDRESS, init_cmd, 3, 100) !=
        HAL_OK) {
      printf("Master Transmit calibration enable bit error!\r\n");
      return 1;
    }

    HAL_Delay(10);
  }

  printf("AHT20 Calibration check is OK!\r\n");
  return 0;
}

int AHT20_TriggerMeasurement(I2C_HandleTypeDef *hi2c) {
  uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};

  if (HAL_I2C_Master_Transmit(hi2c, I2C_AHT20_ADDRESS, measure_cmd, 3, 100) !=
      HAL_OK) {
    printf("AHT20 Transmit trigger measurement failed!\r\n");
    return 1;
  }

  HAL_Delay(80);
  printf("AHT20 Measurement triggered!\r\n");
  return 0;
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  if (BMP280_Init(&bmp280, &hi2c1)) {
    printf("BMP280 Initialize!\r\n");
  } else {
    printf("BMP280 Initialize failed!\r\n");
  }

  if (HAL_I2C_IsDeviceReady(&hi2c1, (I2C_AHT20_ADDRESS), 1,
                            I2C_FIRST_AND_LAST_FRAME) == HAL_OK) {
    printf("AHT20 Responded!\r\n");
    AHT20_Calibration(&hi2c1);
    AHT20_TriggerMeasurement(&hi2c1);
  }
  HAL_Delay(50);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    int32_t adc_T, adc_P;

    if (BMP280_ReadRaw(&bmp280, &adc_T, &adc_P)) {
      int32_t temp_raw = BMP280_Compensate_T(&bmp280, adc_T);
      uint32_t press_raw = BMP280_Compensate_P(&bmp280, adc_P);

      int32_t temp_int = temp_raw / 100;
      int32_t temp_frac = temp_raw % 100;
      if (temp_frac < 0)
        temp_frac = -temp_frac;

      uint32_t press_pa = press_raw / 256;
      uint32_t press_hpa_int = press_pa / 100;
      uint32_t press_hpa_frac = press_pa % 100;

      printf("Temp: %ld.%02ld C | Press: %lu.%02lu hPa\r\n", temp_int,
             temp_frac, press_hpa_int, press_hpa_frac);
    }
    HAL_Delay(500);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
     file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
