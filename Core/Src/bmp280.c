 #include "bmp280.h"

 bool BMP280_Init(BMP280_HandleTypedef *dev, I2C_HandleTypeDef *hi2c) {
   dev->hi2c = hi2c;
   uint8_t chip_id = 0;
   uint8_t calib_buf[24];

   if (HAL_I2C_Mem_Read(dev->hi2c, BMP280_I2C_ADDR, 0xD0, I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 100) != HAL_OK) {
     return false;
   }

   if (chip_id != 0x58) {
     return false;
   }

   if (HAL_I2C_Mem_Read(dev->hi2c, BMP280_I2C_ADDR, 0x88, I2C_MEMADD_SIZE_8BIT, calib_buf, 24, 100) != HAL_OK) {
     return false;
   }

   dev->calib.dig_T1 = (uint16_t)(calib_buf[1] << 8 | calib_buf[0]);
   dev->calib.dig_T2 = (int16_t)(calib_buf[3] << 8 | calib_buf[2]);
   dev->calib.dig_T3 = (int16_t)(calib_buf[5] << 8 | calib_buf[4]);

   dev->calib.dig_P1 = (uint16_t)(calib_buf[7] << 8 | calib_buf[6]);
   dev->calib.dig_P2 = (int16_t)(calib_buf[9] << 8 | calib_buf[8]);
   dev->calib.dig_P3 = (int16_t)(calib_buf[11] << 8 | calib_buf[10]);
   dev->calib.dig_P4 = (int16_t)(calib_buf[13] << 8 | calib_buf[12]);
   dev->calib.dig_P5 = (int16_t)(calib_buf[15] << 8 | calib_buf[14]);
   dev->calib.dig_P6 = (int16_t)(calib_buf[17] << 8 | calib_buf[16]);
   dev->calib.dig_P7 = (int16_t)(calib_buf[19] << 8 | calib_buf[18]);
   dev->calib.dig_P8 = (int16_t)(calib_buf[21] << 8 | calib_buf[20]);
   dev->calib.dig_P9 = (int16_t)(calib_buf[23] << 8 | calib_buf[22]);

   uint8_t osrs_t = 0x01 << 5;
   uint8_t osrs_p = 0x03 << 2;
   uint8_t mode   = 0x03;
   uint8_t ctrl_meas_reg = osrs_t | osrs_p | mode;

   uint8_t t_sb   = 0x04 << 5;
   uint8_t filter = 0x02 << 2;
   uint8_t config_reg = t_sb | filter;

   HAL_I2C_Mem_Write(dev->hi2c, BMP280_I2C_ADDR, 0xF5, I2C_MEMADD_SIZE_8BIT, &config_reg, 1, 100);
   HAL_I2C_Mem_Write(dev->hi2c, BMP280_I2C_ADDR, 0xF4, I2C_MEMADD_SIZE_8BIT, &ctrl_meas_reg, 1, 100);

   return true;
 }

 bool BMP280_ReadRaw(BMP280_HandleTypedef *dev, int32_t *adc_T, int32_t *adc_P) {
   uint8_t raw_data[6];
   if (HAL_I2C_Mem_Read(dev->hi2c, BMP280_I2C_ADDR, 0xF7, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 100) == HAL_OK) {
     *adc_P = (int32_t)((raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4));
     *adc_T = (int32_t)((raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4));
     return true;
   }
   return false;
 }

 int32_t BMP280_Compensate_T(BMP280_HandleTypedef *dev, int32_t adc_T) {
   int32_t var1, var2, T;
   var1 = ((((adc_T >> 3) - ((int32_t)dev->calib.dig_T1 << 1))) * ((int32_t)dev->calib.dig_T2)) >> 11;
   var2 = (((((adc_T >> 4) - ((int32_t)dev->calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->calib.dig_T1))) >> 12) * ((int32_t)dev->calib.dig_T3)) >> 14;
   dev->t_fine = var1 + var2;
   T = (dev->t_fine * 5 + 128) >> 8;
   return T;
 }

 uint32_t BMP280_Compensate_P(BMP280_HandleTypedef *dev, int32_t adc_P) {
   int64_t var1, var2, p;
   var1 = ((int64_t)dev->t_fine) - 128000;
   var2 = var1 * var1 * (int64_t)dev->calib.dig_P6;
   var2 = var2 + ((var1 * (int64_t)dev->calib.dig_P5) << 17);
   var2 = var2 + (((int64_t)dev->calib.dig_P4) << 35);
   var1 = ((var1 * var1 * (int64_t)dev->calib.dig_P3) >> 8) + ((var1 * (int64_t)dev->calib.dig_P2) << 12);
   var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib.dig_P1) >> 33;

   if (var1 == 0) return 0;

   p = 1048576 - adc_P;
   p = (((p << 31) - var2) * 3125) / var1;
   var1 = (((int64_t)dev->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
   var2 = (((int64_t)dev->calib.dig_P8) * p) >> 19;
   p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib.dig_P7) << 4);
   return (uint32_t)p;
 }
