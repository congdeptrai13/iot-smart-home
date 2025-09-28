/*
 * UART_lib.h
 *
 *  Created on: Sep 12, 2025
 *      Author: Administrator
 */

#ifndef INC_UART_LIB_H_
#define INC_UART_LIB_H_

#include "stdint.h"
#include "string.h"
#include "driver/uart.h"
#include "ring_buf.h"

#define START_BYTE 	0x7E
#define END_BYTE 	0x7F

typedef enum{
	RX_WAIT_START,
	RX_WAIT_LENGTH,
	RX_RECEIVING,
	RX_WAIT_END
} RX_STATE_t;

typedef enum{
	CMD_TIME,
	CMD_TEMP,
	CMD_LED
} CMD_STATE_t;

typedef enum{
	CMD_TIME_RESP,
	CMD_TEMP_RESP,
	CMD_LED_RESP
}CMD_RESP_t;

typedef void (*FrameCallback_t)(CMD_RESP_t cmd, uint8_t *data, uint8_t len);

void UART_SendFrame(CMD_STATE_t cmd, uint8_t *data, uint8_t len);
uint16_t crc16_ccitt(uint8_t *data, uint8_t len);
void UART_ReceiveFrame(circ_bbuf_t *circ_buf);
void UART_RegisterCallback(FrameCallback_t cb);
void circ_bbuf_reset(circ_bbuf_t *c);

#endif /* INC_UART_LIB_H_ */
