/*
 * UART_lib.c
 *
 *  Created on: Sep 12, 2025
 *      Author: Administrator
 */
//0x7E | 0x0005 | 0x01 | 0x09 0xE5 | 0x3C | CRC16 | 0x7F
//LENGTH = 5 (CMD=1 + DATA=3 + CRC=2)
//
//CMD = 0x01 (sensor data)
//
//DATA = [0x09 0xE5] (nhiệt độ dạng int16: 253 = 25.3°C), [0x3C] (độ ẩm 60%)
//
//CRC16 = checksum của (0x01 0x09 0xE5 0x3C)
//
//END = 0x7F

#include "UART_lib.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "UART2";

static FrameCallback_t g_frameCallback = NULL;


uint16_t crc16_ccitt(uint8_t *data, uint8_t len){
	uint16_t crc = 0xFFFF;

	for(uint8_t i = 0; i < len;i++){
		crc ^= (uint16_t)data[i] << 8;
		for(uint8_t j = 0; j < 8; j++){
			if(crc & 0x8000){
				crc = (crc << 1) ^ 0x1021;
			}else{
				crc <<= 1;
			}
		}
	}
	return crc;
}

void UART_SendFrame(CMD_STATE_t cmd, uint8_t *data, uint8_t len){
	uint8_t buf[64], index = 0;

	//START = 0x7E
	buf[index++] = START_BYTE;
	//LENGTH = 5 (CMD=1 + DATA=3 + CRC=2)
	buf[index++] = len + 3;
	//CMD = 0x01 (sensor data)
	buf[index++] = cmd;
	//DATA = [0x09 0xE5] (nhiệt độ dạng int16: 253 = 25.3°C), [0x3C] (độ ẩm 60%)
	memcpy(&buf[index], data, len);
	index += len;
	//CRC16 = checksum của (0x01 0x09 0xE5 0x3C)
    uint16_t crc = crc16_ccitt(&buf[2], len + 1); // CMD + DATA
    buf[index++] = crc & 0xFF;
    buf[index++] = (crc >> 8) & 0xFF;
	//END = 0x7F
	buf[index++] = END_BYTE;

	// ESP_LOGI("UART", "Sending frame (%d bytes):", index);
	// for (int i = 0; i < index; i++) {
	// 	printf("%02X ", buf[i]);   // in dạng hex
	// }
	// printf("\n");
	// HAL_UART_Transmit(&huart2, buf, index, HAL_MAX_DELAY);
	uart_write_bytes(UART_NUM_2, (const char*)buf, index);
}

void UART_ReceiveFrame(circ_bbuf_t *circ_buf){

	static uint8_t frame[128];
	static uint8_t index = 0;
	static uint8_t expectedLen = 0;

	static RX_STATE_t state = RX_WAIT_START;
	uint8_t byte;
	while(circ_bbuf_pop(circ_buf, &byte) == STATE_OK){
		switch (state) {
			case RX_WAIT_START:
				if(byte == START_BYTE){
					index = 0;
					frame[index++] = byte;
					state = RX_WAIT_LENGTH;
				}
				break;
			case RX_WAIT_LENGTH:
				expectedLen = byte;
				frame[index++] = byte;
				state = RX_RECEIVING;
				break;
			case RX_RECEIVING:
				frame[index++] = byte;
				if(index >= expectedLen + 2){
					state = RX_WAIT_END;
				}
				break;
			case RX_WAIT_END:{
				frame[index++] = byte;
				if(byte == END_BYTE){
					uint8_t cmd = frame[2];
					uint8_t *data = &frame[3];
					uint8_t dataLen = expectedLen - 3; // cmd(1) + crc(2)
					uint16_t crcRecv = data[dataLen] | (data[dataLen + 1] << 8);
					uint16_t crcCalc = crc16_ccitt(&frame[2], dataLen + 1); // CMD + DATA
					if(crcRecv == crcCalc){
						if (g_frameCallback) {
							g_frameCallback(cmd, data, dataLen);  // gọi hàm callback do người dùng đăng ký
						}
					}
				}
				state = RX_WAIT_START;
				break;
			}
			default:
				break;
		}
	}
}

void UART_RegisterCallback(FrameCallback_t cb) {
    g_frameCallback = cb;
}

void circ_bbuf_reset(circ_bbuf_t *c) {
    c->head = 0;
    c->tail = 0;
}



