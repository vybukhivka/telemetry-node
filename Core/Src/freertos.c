/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "aht20.h"
#include "bmp280.h"
#include "mpu6050.h"
#include "st7920.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern BMP280_HandleTypedef bmp280;
extern AHT20_HandleTypedef aht20;
extern MPU6050_HandleTypedef mpu6050;
extern ST7920_HandleTypedef st7920;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  uint16_t press_hpa_int;
  uint8_t press_hpa_frac;
  int8_t bmp_temp_int;

  int8_t aht_temp_int;
  uint8_t aht_hum_int;

  int8_t ax_i, ay_i, az_i;
  uint8_t ax_f, ay_f, az_f;

  int16_t gx, gy, gz;
} telemetry_t;

typedef struct {
  char str[64];
} log_msg_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static telemetry_t g_telemetry = {0};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for envTask */
osThreadId_t envTaskHandle;
const osThreadAttr_t envTask_attributes = {
  .name = "envTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for imuTask */
osThreadId_t imuTaskHandle;
const osThreadAttr_t imuTask_attributes = {
  .name = "imuTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for displayTask */
osThreadId_t displayTaskHandle;
const osThreadAttr_t displayTask_attributes = {
  .name = "displayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for loggerTask */
osThreadId_t loggerTaskHandle;
const osThreadAttr_t loggerTask_attributes = {
  .name = "loggerTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for sensorDisplayQueue */
osMessageQueueId_t sensorDisplayQueueHandle;
const osMessageQueueAttr_t sensorDisplayQueue_attributes = {
  .name = "sensorDisplayQueue"
};
/* Definitions for logQueue */
osMessageQueueId_t logQueueHandle;
const osMessageQueueAttr_t logQueue_attributes = {
  .name = "logQueue"
};
/* Definitions for i2cBusMutex */
osMutexId_t i2cBusMutexHandle;
const osMutexAttr_t i2cBusMutex_attributes = {
  .name = "i2cBusMutex"
};
/* Definitions for uartDmaDoneSem */
osSemaphoreId_t uartDmaDoneSemHandle;
const osSemaphoreAttr_t uartDmaDoneSem_attributes = {
  .name = "uartDmaDoneSem"
};
/* Definitions for i2cDmaDoneSem */
osSemaphoreId_t i2cDmaDoneSemHandle;
const osSemaphoreAttr_t i2cDmaDoneSem_attributes = {
  .name = "i2cDmaDoneSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void log_print(const char *fmt, ...);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartEnvTask(void *argument);
void StartImuTask(void *argument);
void StartDisplayTask(void *argument);
void StartLoggerTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of i2cBusMutex */
  i2cBusMutexHandle = osMutexNew(&i2cBusMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of uartDmaDoneSem */
  uartDmaDoneSemHandle = osSemaphoreNew(1, 1, &uartDmaDoneSem_attributes);

  /* creation of i2cDmaDoneSem */
  i2cDmaDoneSemHandle = osSemaphoreNew(1, 1, &i2cDmaDoneSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of sensorDisplayQueue */
  sensorDisplayQueueHandle = osMessageQueueNew (1, 40, &sensorDisplayQueue_attributes);

  /* creation of logQueue */
  logQueueHandle = osMessageQueueNew (16, 64, &logQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of envTask */
  envTaskHandle = osThreadNew(StartEnvTask, NULL, &envTask_attributes);

  /* creation of imuTask */
  imuTaskHandle = osThreadNew(StartImuTask, NULL, &imuTask_attributes);

  /* creation of displayTask */
  displayTaskHandle = osThreadNew(StartDisplayTask, NULL, &displayTask_attributes);

  /* creation of loggerTask */
  loggerTaskHandle = osThreadNew(StartLoggerTask, NULL, &loggerTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartEnvTask */
/**
 * @brief Function implementing the envTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartEnvTask */
void StartEnvTask(void *argument)
{
  /* USER CODE BEGIN StartEnvTask */
  int32_t adc_T, adc_P;

  for (;;) {
    if (osMutexAcquire(i2cBusMutexHandle, osWaitForever) == osOK) {
      /* BMP280 Read & Compensate */
      if (BMP280_ReadRaw(&bmp280, &adc_T, &adc_P) == BMP280_OK) {
        int32_t temp_raw = BMP280_Compensate_T(&bmp280, adc_T);
        uint32_t press_raw = BMP280_Compensate_P(&bmp280, adc_P);

        uint32_t press_pa = press_raw / 256;
        g_telemetry.press_hpa_int = (uint16_t)(press_pa / 100);
        g_telemetry.press_hpa_frac = (uint8_t)(press_pa % 100);
        g_telemetry.bmp_temp_int = (int8_t)(temp_raw / 100);

        log_print("BMP280 -> P:%u.%02u hPa T:%d C\r\n",
                  g_telemetry.press_hpa_int, g_telemetry.press_hpa_frac,
                  g_telemetry.bmp_temp_int);
      }

      /* AHT20 Read */
      if (AHT20_ReadData(&aht20) == AHT20_OK) {
        g_telemetry.aht_temp_int = (int8_t)aht20.temperature;
        g_telemetry.aht_hum_int = (uint8_t)aht20.humidity;

        log_print("AHT20  -> T:%d C H:%u %%\r\n", g_telemetry.aht_temp_int,
                  g_telemetry.aht_hum_int);
      }

      osMutexRelease(i2cBusMutexHandle);
    }

    /* Send update to Display Task */
    osMessageQueuePut(sensorDisplayQueueHandle, &g_telemetry, 0, 0);

    osDelay(500); /* 2 Hz refresh */
  }
  /* USER CODE END StartEnvTask */
}

/* USER CODE BEGIN Header_StartImuTask */
/**
 * @brief Function implementing the imuTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartImuTask */
void StartImuTask(void *argument)
{
  /* USER CODE BEGIN StartImuTask */
  uint32_t last_wake_time = osKernelGetState();

  for (;;) {
    if (osMutexAcquire(i2cBusMutexHandle, osWaitForever) == osOK) {
      if (MPU6050_ReadAll(&mpu6050) == MPU6050_OK) {
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

        g_telemetry.ax_i =
            (int8_t)(ax_int > 9 ? 9 : (ax_int < -9 ? -9 : ax_int));
        g_telemetry.ax_f = (uint8_t)(ax_frac % 100);

        g_telemetry.ay_i =
            (int8_t)(ay_int > 9 ? 9 : (ay_int < -9 ? -9 : ay_int));
        g_telemetry.ay_f = (uint8_t)(ay_frac % 100);

        g_telemetry.az_i =
            (int8_t)(az_int > 9 ? 9 : (az_int < -9 ? -9 : az_int));
        g_telemetry.az_f = (uint8_t)(az_frac % 100);

        g_telemetry.gx = (int16_t)mpu6050.Gx;
        g_telemetry.gy = (int16_t)mpu6050.Gy;
        g_telemetry.gz = (int16_t)mpu6050.Gz;

        log_print(
            "MPU6050 -> Acc: %d.%02u, %d.%02u, %d.%02u | Gyro: %d, %d, %d\r\n",
            g_telemetry.ax_i, g_telemetry.ax_f, g_telemetry.ay_i,
            g_telemetry.ay_f, g_telemetry.az_i, g_telemetry.az_f,
            g_telemetry.gx, g_telemetry.gy, g_telemetry.gz);
      }
      osMutexRelease(i2cBusMutexHandle);
    }

    /* Send frame to Display Task */
    osMessageQueuePut(sensorDisplayQueueHandle, &g_telemetry, 0, 0);

    /* Run at 20 Hz (50ms) */
    osDelayUntil(last_wake_time += pdMS_TO_TICKS(50));
  }
  /* USER CODE END StartImuTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
 * @brief Function implementing the displayTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
  telemetry_t data;
  char lcd_buf[17];

  for (;;) {
    if (osMessageQueueGet(sensorDisplayQueueHandle, &data, NULL,
                          osWaitForever) == osOK) {
      /* Line 0: Pressure */
      snprintf(lcd_buf, sizeof(lcd_buf), "P:%4u   ",
               (unsigned int)(data.press_hpa_int % 10000));
      ST7920_SendString(&st7920, 0, 0, lcd_buf);

      /* Line 1: Temperature & Humidity */
      int temp = (data.aht_temp_int > 99)
                     ? 99
                     : ((data.aht_temp_int < -99) ? -99 : data.aht_temp_int);
      unsigned int hum = (data.aht_hum_int > 99) ? 99 : data.aht_hum_int;
      snprintf(lcd_buf, sizeof(lcd_buf), "T:%2dC H:%2u%% ", temp, hum);
      ST7920_SendString(&st7920, 1, 0, lcd_buf);

      /* Line 2: Accelerometer (Matches main.c clamping) */
      int ax = (data.ax_i > 9) ? 9 : ((data.ax_i < -9) ? -9 : data.ax_i);
      int ay = (data.ay_i > 9) ? 9 : ((data.ay_i < -9) ? -9 : data.ay_i);
      int az = (data.az_i > 9) ? 9 : ((data.az_i < -9) ? -9 : data.az_i);
      snprintf(lcd_buf, sizeof(lcd_buf), "A%2d.%02u%2d.%02u%2d.%02u", ax,
               (unsigned int)(data.ax_f % 100), ay,
               (unsigned int)(data.ay_f % 100), az,
               (unsigned int)(data.az_f % 100));
      ST7920_SendString(&st7920, 2, 0, lcd_buf);

      /* Line 3: Gyroscope */
      int gx = (data.gx > 99) ? 99 : ((data.gx < -99) ? -99 : data.gx);
      int gy = (data.gy > 99) ? 99 : ((data.gy < -99) ? -99 : data.gy);
      int gz = (data.gz > 99) ? 99 : ((data.gz < -99) ? -99 : data.gz);
      snprintf(lcd_buf, sizeof(lcd_buf), "G:%3d%3d%3d d/s", gx, gy, gz);
      ST7920_SendString(&st7920, 3, 0, lcd_buf);
    }
  }
  /* USER CODE END StartDisplayTask */
}

/* USER CODE BEGIN Header_StartLoggerTask */
/**
 * @brief Function implementing the loggerTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartLoggerTask */
void StartLoggerTask(void *argument)
{
  /* USER CODE BEGIN StartLoggerTask */
  log_msg_t msg;

  for (;;) {
    if (osMessageQueueGet(logQueueHandle, &msg, NULL, osWaitForever) == osOK) {
      uint16_t len = (uint16_t)strlen(msg.str);
      if (HAL_UART_Transmit_DMA(&huart2, (uint8_t *)msg.str, len) == HAL_OK) {
        osSemaphoreAcquire(uartDmaDoneSemHandle, pdMS_TO_TICKS(100));
      }
    }
  }
  /* USER CODE END StartLoggerTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void log_print(const char *fmt, ...) {
  log_msg_t msg;
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg.str, sizeof(msg.str), fmt, args);
  va_end(args);

  osMessageQueuePut(logQueueHandle, &msg, 0, 0);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    osSemaphoreRelease(i2cDmaDoneSemHandle);
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    osSemaphoreRelease(i2cDmaDoneSemHandle);
  }
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    osSemaphoreRelease(i2cDmaDoneSemHandle);
  }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    osSemaphoreRelease(i2cDmaDoneSemHandle);
  }
}

HAL_StatusTypeDef I2C_Mem_Read_DMA_Wait(I2C_HandleTypeDef *hi2c,
                                        uint16_t DevAddress,
                                        uint16_t MemAddress,
                                        uint16_t MemAddSize, uint8_t *pData,
                                        uint16_t Size, uint32_t TimeoutMs) {
  HAL_StatusTypeDef status = HAL_I2C_Mem_Read_DMA(hi2c, DevAddress, MemAddress,
                                                  MemAddSize, pData, Size);
  if (status == HAL_OK) {
    if (osSemaphoreAcquire(i2cDmaDoneSemHandle, pdMS_TO_TICKS(TimeoutMs)) !=
        osOK) {
      return HAL_TIMEOUT;
    }
  }
  return status;
}

HAL_StatusTypeDef I2C_Mem_Write_DMA_Wait(I2C_HandleTypeDef *hi2c,
                                         uint16_t DevAddress,
                                         uint16_t MemAddress,
                                         uint16_t MemAddSize, uint8_t *pData,
                                         uint16_t Size, uint32_t TimeoutMs) {
  HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(hi2c, DevAddress, MemAddress,
                                                   MemAddSize, pData, Size);
  if (status == HAL_OK) {
    if (osSemaphoreAcquire(i2cDmaDoneSemHandle, pdMS_TO_TICKS(TimeoutMs)) !=
        osOK) {
      return HAL_TIMEOUT;
    }
  }
  return status;
}
/* USER CODE END Application */

