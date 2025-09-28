/*
 * DS3231.c
 *
 *  Created on: Sep 6, 2025
 *      Author: DELL-M
 */
#include "DS3231.h"

uint8_t BCD2Decimal(uint8_t BCD){
	return (BCD >> 4) * 10 + (BCD & 0x0F);
}

uint8_t Decimal2BCD(uint8_t Decimal){
	return ((Decimal / 10) << 4) | (Decimal % 10);
}

void DS3231_Init(DateTime_t *dt){
	dt->second = 1;
	dt->min = 1;
	dt->hour = 1;
	dt->temp = 0;
	dt->date = 7;
	dt->day = 27;
	dt->month = 9;
	dt->year = 25;
}

void DS3231_ReadTemp(DateTime_t *dt){
    uint8_t data[2];
    uint8_t addr = 0x11;   // bắt đầu từ thanh ghi nhiệt độ MSB
    HAL_I2C_Master_Transmit(&hi2c1, BASE_ADDR, &addr, 1, 100);  // nếu HAL yêu cầu 8-bit
    HAL_I2C_Master_Receive(&hi2c1, BASE_ADDR, data, sizeof(data), 100);

    int8_t msb = (int8_t)data[0];      // signed
    uint8_t frac = data[1] >> 6;       // lấy 2 bit cao
    dt->temp = msb + (frac * 0.25f);
}

void DS3231_ReadTime(DateTime_t *dt){
	uint8_t buff[7];
	uint8_t addr = 0x00;
	HAL_I2C_Master_Transmit(&hi2c1, BASE_ADDR, &addr, 1, 100);
	HAL_I2C_Master_Receive(&hi2c1, BASE_ADDR, buff, sizeof(buff), 100);
	dt->second 	= BCD2Decimal(buff[0]);
	dt->min 	= BCD2Decimal(buff[1]);
	dt->hour	= BCD2Decimal(buff[2]);
	dt->day    = BCD2Decimal(buff[3]);        // 1–7 (thứ)
	dt->date   = BCD2Decimal(buff[4]);        // 1–31
	dt->month  = BCD2Decimal(buff[5] & 0x1F); // bit7 = century
	dt->year   = BCD2Decimal(buff[6]);        // 00–99
}



void DS3231_Write(DateTime_t *dt){
	uint8_t buff[8];
	buff[0] = 0x00; // bắt đầu từ thanh ghi 0x00
	buff[1] = Decimal2BCD(dt->second);
	buff[2] = Decimal2BCD(dt->min);
	buff[3] = Decimal2BCD(dt->hour);
	buff[4] = Decimal2BCD(dt->day);   // 1–7
	buff[5] = Decimal2BCD(dt->date);  // 1–31
	buff[6] = Decimal2BCD(dt->month); // 1–12
	buff[7] = Decimal2BCD(dt->year % 100); // chỉ 2 số cuối (00–99)
	HAL_I2C_Master_Transmit(&hi2c1, BASE_ADDR, buff, sizeof(buff), 100);
}

