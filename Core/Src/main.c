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
#include "cmsis_os.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "aht20.h"
#include "bmp280.h"
#include "mpu6050.h"
#include "st7920.h"
#include <stdio.h>
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
AHT20_HandleTypedef aht20;
MPU6050_HandleTypedef mpu6050;
ST7920_HandleTypedef st7920;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
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
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  BMP280_StatusTypedef bmp_status = BMP280_Init(&bmp280, &hi2c1);
  if (bmp_status == BMP280_OK) {
    printf("BMP280 Initialized!\r\n");
  } else {
    printf("BMP280 Initialization failed with error code: %d\r\n", bmp_status);
  }

  AHT20_StatusTypedef aht_status = AHT20_Init(&aht20, &hi2c1);
  if (aht_status == AHT20_OK) {
    printf("AHT20 Initialized!\r\n");
  } else {
    printf("AHT20 Initialize failed!\r\n");
  }

  MPU6050_StatusTypedef mpu_status = MPU6050_Init(&mpu6050, &hi2c1);
  if (mpu_status == MPU6050_OK) {
    MPU6050_Calibrate(&mpu6050, 200);
    printf("MPU6050 Initialized!\r\n");
  } else {
    printf("MPU6050 Initialize failed!\r\n");
  }

  if (ST7920_Init(&st7920, &hspi2, GPIOB, GPIO_PIN_12) == ST7920_OK) {
    printf("ST7920 Initialized!\r\n");
  }

  HAL_Delay(50);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize(); /* Call init function for freertos objects (in
                           cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char lcd_buf[17]; // 16 char + null terminator
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    int32_t adc_T, adc_P;

    // BMP280 READ
    bmp_status = BMP280_ReadRaw(&bmp280, &adc_T, &adc_P);
    if (bmp_status == BMP280_OK) {
      int32_t temp_raw = BMP280_Compensate_T(&bmp280, adc_T);
      uint32_t press_raw = BMP280_Compensate_P(&bmp280, adc_P);

      int32_t temp_int = temp_raw / 100;
      int32_t temp_frac = temp_raw % 100;
      if (temp_frac < 0)
        temp_frac = -temp_frac;

      uint32_t press_pa = press_raw / 256;
      uint32_t press_hpa_int = press_pa / 100;
      uint32_t press_hpa_frac = press_pa % 100;

      printf("BMP280 -> Temp: %ld.%02ld C | Press: %lu.%02lu hPa\r\n", temp_int,
             temp_frac, press_hpa_int, press_hpa_frac);
      snprintf(lcd_buf, sizeof(lcd_buf), "P:%4u   ",
               (unsigned int)(press_hpa_int % 10000));
      ST7920_SendString(&st7920, 0, 0, lcd_buf);
    } else {
      ST7920_SendString(&st7920, 0, 0, "BMP280 Error    ");
      printf("BMP280 Read Data Failed! Error Code: %d\r\n", bmp_status);
    }

    // AHT20 READ
    aht_status = AHT20_ReadData(&aht20);
    if (aht_status == AHT20_OK) {
      int32_t temp_int = (int32_t)aht20.temperature;
      int32_t temp_frac = (int32_t)((aht20.temperature - temp_int) * 100);
      if (temp_frac < 0)
        temp_frac = -temp_frac;

      int32_t hum_int = (int32_t)aht20.humidity;
      int32_t hum_frac = (int32_t)((aht20.humidity - hum_int) * 100);
      if (hum_frac < 0)
        hum_frac = -hum_frac;

      printf("AHT20  -> Temp: %ld.%02ld C | Humidity: %ld.%02ld %%\r\n",
             temp_int, temp_frac, hum_int, hum_frac);
      snprintf(lcd_buf, sizeof(lcd_buf), "T:%2dC  RH:%2d%%  ",
               (int)(temp_int > 99 ? 99 : (temp_int < -99 ? -99 : temp_int)),
               (int)(hum_int > 99 ? 99 : (hum_int < 0 ? 0 : hum_int)));
      ST7920_SendString(&st7920, 1, 0, lcd_buf);
    } else {
      ST7920_SendString(&st7920, 1, 0, "AHT20 Error     ");
      printf("AHT20 Read Data Failed! Error Code: %d\r\n", aht_status);
    }

    // MPU6050 READ
    mpu_status = MPU6050_ReadAll(&mpu6050);
    if (mpu_status == MPU6050_OK) {
      // Convert floats to int/frac for printing without %f dependency
      int32_t ax_int = (int32_t)mpu6050.Ax;
      int32_t ax_frac = (int32_t)((mpu6050.Ax - ax_int) * 100);
      if (ax_frac < 0)
        ax_frac = -ax_frac;

      int32_t ay_int = (int32_t)mpu6050.Ay;
      int32_t ay_frac = (int32_t)((mpu6050.Ay - ay_int) * 100);
      if (ay_frac < 0)
        ay_frac = -ay_frac;

      int32_t az_int = (int32_t)mpu6050.Az;
      int32_t az_frac = (int32_t)((mpu6050.Az - az_int) * 100);
      if (az_frac < 0)
        az_frac = -az_frac;

      int32_t gx_int = (int32_t)mpu6050.Gx;
      int32_t gx_frac = (int32_t)((mpu6050.Gx - gx_int) * 100);
      if (gx_frac < 0)
        gx_frac = -gx_frac;

      int32_t gy_int = (int32_t)mpu6050.Gy;
      int32_t gy_frac = (int32_t)((mpu6050.Gy - gy_int) * 100);
      if (gy_frac < 0)
        gy_frac = -gy_frac;

      int32_t gz_int = (int32_t)mpu6050.Gz;
      int32_t gz_frac = (int32_t)((mpu6050.Gz - gz_int) * 100);
      if (gz_frac < 0)
        gz_frac = -gz_frac;

      printf("MPU6050 -> Accel (g): X=%ld.%02ld Y=%ld.%02ld Z=%ld.%02ld | Gyro "
             "(deg/s): X=%ld.%02ld Y=%ld.%02ld Z=%ld.%02ld\r\n",
             ax_int, ax_frac, ay_int, ay_frac, az_int, az_frac, gx_int, gx_frac,
             gy_int, gy_frac, gz_int, gz_frac);
      // Line 2: Accelerometer
      int8_t ax_i = (int8_t)(ax_int > 9 ? 9 : (ax_int < -9 ? -9 : ax_int));
      uint8_t ax_f = (uint8_t)(ax_frac % 100);

      int8_t ay_i = (int8_t)(ay_int > 9 ? 9 : (ay_int < -9 ? -9 : ay_int));
      uint8_t ay_f = (uint8_t)(ay_frac % 100);

      int8_t az_i = (int8_t)(az_int > 9 ? 9 : (az_int < -9 ? -9 : az_int));
      uint8_t az_f = (uint8_t)(az_frac % 100);

      snprintf(lcd_buf, sizeof(lcd_buf), "A%2d.%02u%2d.%02u%2d.%02u", ax_i,
               ax_f, ay_i, ay_f, az_i, az_f);
      ST7920_SendString(&st7920, 2, 0, lcd_buf);
      // Line 3: Gyroscope
      snprintf(lcd_buf, sizeof(lcd_buf), "G:%3d%3d%3d d/s",
               (int)(gx_int > 99 ? 99 : (gx_int < -99 ? -99 : gx_int)),
               (int)(gy_int > 99 ? 99 : (gy_int < -99 ? -99 : gy_int)),
               (int)(gz_int > 99 ? 99 : (gz_int < -99 ? -99 : gz_int)));
      ST7920_SendString(&st7920, 3, 0, lcd_buf);
    } else {
      ST7920_SendString(&st7920, 2, 0, "MPU6050 Error   ");
      ST7920_SendString(&st7920, 3, 0, "                ");
      printf("MPU6050 Read Data Failed! Error Code: %d\r\n", mpu_status);
    }

    printf("--------------------------------------------------\r\n");
    HAL_Delay(1000);
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
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
