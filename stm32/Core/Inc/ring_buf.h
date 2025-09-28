/*
 * ring_buf.h
 *
 *  Created on: Sep 12, 2025
 *      Author: Administrator
 */

#ifndef INC_RING_BUF_H_
#define INC_RING_BUF_H_

#include "main.h"
#include "common_defs.h"

#define CIRC_BBUF_DEF(x,y)                \
    uint8_t x##_data_space[y];            \
    circ_bbuf_t x = {                     \
        .buffer = x##_data_space,         \
        .head = 0,                        \
        .tail = 0,                        \
        .maxlen = y                       \
    }

typedef struct {
    uint8_t * const buffer;
    int head;
    int tail;
    const int maxlen;
} circ_bbuf_t;

int circ_bbuf_push(circ_bbuf_t *c, uint8_t data);
int circ_bbuf_pop(circ_bbuf_t *c, uint8_t *data);



#endif /* INC_RING_BUF_H_ */
