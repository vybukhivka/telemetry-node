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
#include "gpio.h"
#include "i2c.h"
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
typedef struct {
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;

  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
} BMP280_CalibData;

BMP280_CalibData calib;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
int32_t t_fine;

int32_t bmp280_compensate_T_int32(int32_t adc_T, BMP280_CalibData *calib) {
  int32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((int32_t)calib->dig_T1 << 1))) *
          ((int32_t)calib->dig_T2)) >>
         11;
  var2 = (((((adc_T >> 4) - ((int32_t)calib->dig_T1)) *
            ((adc_T >> 4) - ((int32_t)calib->dig_T1))) >>
           12) *
          ((int32_t)calib->dig_T3)) >>
         14;
  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}

uint32_t bmp280_compensate_P_int64(int32_t adc_P, BMP280_CalibData *calib) {
  int64_t var1, var2, p;
  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)calib->dig_P6;
  var2 = var2 + ((var1 * (int64_t)calib->dig_P5) << 17);
  var2 = var2 + (((int64_t)calib->dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)calib->dig_P3) >> 8) +
         ((var1 * (int64_t)calib->dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib->dig_P1) >> 33;

  if (var1 == 0) {
    return 0;
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)calib->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)calib->dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)calib->dig_P7) << 4);
  return (uint32_t)p;
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
  BMP280_CalibData calib;
  uint8_t calib_buf[24];
  uint8_t chip_id = 0;

  // Chip ID verification
  HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
      &hi2c1, (0x77 << 1), 0xD0, I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 100);

  if (status == HAL_OK && chip_id == 0x58) {
    if (HAL_I2C_Mem_Read(&hi2c1, (0x77 << 1), 0x88, I2C_MEMADD_SIZE_8BIT,
                         calib_buf, 24, 100) == HAL_OK) {
      calib.dig_T1 = (uint16_t)(calib_buf[1] << 8 | calib_buf[0]);
      calib.dig_T2 = (int16_t)(calib_buf[3] << 8 | calib_buf[2]);
      calib.dig_T3 = (int16_t)(calib_buf[5] << 8 | calib_buf[4]);

      calib.dig_P1 = (uint16_t)(calib_buf[7] << 8 | calib_buf[6]);
      calib.dig_P2 = (int16_t)(calib_buf[9] << 8 | calib_buf[8]);
      calib.dig_P3 = (int16_t)(calib_buf[11] << 8 | calib_buf[10]);
      calib.dig_P4 = (int16_t)(calib_buf[13] << 8 | calib_buf[12]);
      calib.dig_P5 = (int16_t)(calib_buf[15] << 8 | calib_buf[14]);
      calib.dig_P6 = (int16_t)(calib_buf[17] << 8 | calib_buf[16]);
      calib.dig_P7 = (int16_t)(calib_buf[19] << 8 | calib_buf[18]);
      calib.dig_P8 = (int16_t)(calib_buf[21] << 8 | calib_buf[20]);
      calib.dig_P9 = (int16_t)(calib_buf[23] << 8 | calib_buf[22]);

      printf("Calibration loaded! T1=%u, P1=%u\r\n", calib.dig_T1,
             calib.dig_P1);
    }

    uint8_t osrs_t = 0x01 << 5;
    uint8_t osrs_p = 0x03 << 2;
    uint8_t mode = 0x03;

    uint8_t ctrl_meas_reg = osrs_t | osrs_p | mode;

    uint8_t t_sb = 0x04 << 5;
    uint8_t filter = 0x02 << 2;

    uint8_t config_reg = t_sb | filter;

    HAL_I2C_Mem_Write(&hi2c1, (0x77 << 1), 0xF5, I2C_MEMADD_SIZE_8BIT,
                      &config_reg, 1, 100);
    HAL_I2C_Mem_Write(&hi2c1, (0x77 << 1), 0xF4, I2C_MEMADD_SIZE_8BIT,
                      &ctrl_meas_reg, 1, 100);
    printf("BMP280 Configured and running!\r\n");
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    uint8_t raw_data[6];
    if (HAL_I2C_Mem_Read(&hi2c1, (0x77 << 1), 0xF7, I2C_MEMADD_SIZE_8BIT,
                         raw_data, 6, 100) == HAL_OK) {
      int32_t adc_P = (int32_t)((raw_data[0] << 12) | (raw_data[1] << 4) |
                                (raw_data[2] >> 4));
      int32_t adc_T = (int32_t)((raw_data[3] << 12) | (raw_data[4] << 4) |
                                (raw_data[5] >> 4));

      int32_t temp_raw = bmp280_compensate_T_int32(adc_T, &calib);
      uint32_t press_raw = bmp280_compensate_P_int64(adc_P, &calib);

      int32_t temp_int = temp_raw / 100;
      int32_t temp_frac = temp_raw % 100;
      if (temp_frac < 0)
        temp_frac = -temp_frac;

      uint32_t press_pa = press_raw / 256;
      uint32_t press_hpa_int = press_pa / 100;
      uint32_t press_hpa_frac = press_pa % 100;

      uint32_t press_mmhg_int = (press_pa * 75) / 10000;

      printf("Temp: %ld.%02ld C | Press: %lu.%02lu hPa (%lu mmHg)\r\n",
             temp_int, temp_frac, press_hpa_int, press_hpa_frac,
             press_mmhg_int);
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
  /* User can add his own implementation to report the HAL error return state */
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
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
