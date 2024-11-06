/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Adrian "asie" Siekierka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef _NRIO_D12_
#define _NRIO_D12_

#include <stdint.h>

// NRIO "D12" (PDIUSBD12)

#define NRIO_D12_DATA (*((volatile uint16_t*) 0x9FDFFFE))
#define NRIO_D12_CMD  (*((volatile uint16_t*) 0x9FFFFFE))

#define NRIO_D12_ADDRESS_ENABLE  BIT(7)
#define NRIO_D12_ADDRESS(n)      (n)
#define NRIO_D12_CMD_SET_ADDRESS     0xD0

#define NRIO_D12_ENDPOINT_ENABLE     BIT(0)
#define NRIO_D12_CMD_SET_ENDPOINT    0xD8

#define NRIO_D12_MODE_CFG_ISO_NONE   (0)
#define NRIO_D12_MODE_CFG_ISO_OUT    (1 << 6)
#define NRIO_D12_MODE_CFG_ISO_IN     (2 << 6)
#define NRIO_D12_MODE_CFG_ISO_IO     (3 << 6)
#define NRIO_D12_MODE_CFG_SOFTCT     BIT(4)
#define NRIO_D12_MODE_CFG_INT_NAK    BIT(3)
#define NRIO_D12_MODE_CFG_INT_ACK    0
#define NRIO_D12_MODE_CFG_CLKAON     BIT(2)
#define NRIO_D12_MODE_CFG_NO_LAZYCLK BIT(1)
#define NRIO_D12_MODE_CLK_SOF_ONLY   BIT(15)
#define NRIO_D12_MODE_CLK_INITIAL    BIT(14)
#define NRIO_D12_MODE_CLK_DIV(n)     (((n) - 1) << 8)
#define NRIO_D12_CMD_SET_MODE        0xF3

#define NRIO_D12_DMA_CFG_IRQ_VALID   BIT(7)
#define NRIO_D12_DMA_CFG_IRQ_PKT     BIT(6)
#define NRIO_D12_DMA_CFG_IRQ_ANY     0
#define NRIO_D12_DMA_CFG_IRQ_SOF     BIT(5)
#define NRIO_D12_DMA_CFG_REPEAT      BIT(4)
#define NRIO_D12_DMA_CFG_READ        0
#define NRIO_D12_DMA_CFG_WRITE       BIT(3)
#define NRIO_D12_DMA_CFG_ENABLE      BIT(2)
#define NRIO_D12_DMA_CFG_BURST_1     0x00
#define NRIO_D12_DMA_CFG_BURST_4     0x01
#define NRIO_D12_DMA_CFG_BURST_8     0x02
#define NRIO_D12_DMA_CFG_BURST_16    0x03
#define NRIO_D12_CMD_SET_DMA         0xFB

#define NRIO_D12_INT_DMA_EOT         BIT(8)
#define NRIO_D12_INT_SUSPEND_CHANGE  BIT(7)
#define NRIO_D12_INT_BUS_RESET       BIT(6)
#define NRIO_D12_INT_EP(n)           BIT(n)
#define NRIO_D12_CMD_READ_INT        0xF4

#define NRIO_D12_SELECT_EP_STALL     BIT(1)
#define NRIO_D12_SELECT_EP_FULL      BIT(0)
#define NRIO_D12_CMD_SELECT_EP(n)    (n)

#define NRIO_D12_STATUS_STALL        BIT(7)
#define NRIO_D12_STATUS_BUF1_FULL    BIT(6)
#define NRIO_D12_STATUS_BUF0_FULL    BIT(5)
#define NRIO_D12_STATUS_SETUP        BIT(2)
#define NRIO_D12_CMD_STATUS_EP(n)    ((n) | 0x80)

#define NRIO_D12_T_STATUS_OVERRUN    BIT(7)
#define NRIO_D12_T_STATUS_DATA1      BIT(6)
#define NRIO_D12_T_STATUS_SETUP      BIT(5)
#define NRIO_D12_T_ERROR_NONE        0x00
#define NRIO_D12_T_ERROR_PID_ENCODE  0x02
#define NRIO_D12_T_ERROR_PID_UNKNOWN 0x04
#define NRIO_D12_T_ERROR_UNEXPECTED  0x06
#define NRIO_D12_T_ERROR_TOKEN_CRC   0x08
#define NRIO_D12_T_ERROR_DATA_CRC    0x0A
#define NRIO_D12_T_ERROR_TIMEOUT     0x0C
#define NRIO_D12_T_ERROR_UNEXP_END   0x10
#define NRIO_D12_T_ERROR_NAK         0x12
#define NRIO_D12_T_ERROR_STALL       0x14
#define NRIO_D12_T_ERROR_MASK        0x1E
#define NRIO_D12_T_STATUS_SUCCESS    BIT(0)
#define NRIO_D12_CMD_T_STATUS_EP(n)  ((n) | 0x40)

#define NRIO_D12_SET_STATUS_STALL    BIT(0)
#define NRIO_D12_CMD_SET_STATUS_EP(n) ((n) | 0x40)

#define NRIO_D12_CMD_READ_BUFFER     0xF0
#define NRIO_D12_CMD_WRITE_BUFFER    0xF0
#define NRIO_D12_CMD_ACK_SETUP       0xF1
#define NRIO_D12_CMD_CLEAR_BUFFER    0xF2
#define NRIO_D12_CMD_VALIDATE_BUFFER 0xFA
#define NRIO_D12_CMD_SEND_RESUME     0xF6
#define NRIO_D12_CMD_READ_FRAME_NUM  0xF5
#define NRIO_D12_CMD_READ_ID         0xFD

__attribute__((always_inline))
static inline void nrio_d12_set_address(uint8_t val) {
    NRIO_D12_CMD = NRIO_D12_CMD_SET_ADDRESS;
    NRIO_D12_DATA = val;
}

__attribute__((always_inline))
static inline void nrio_d12_set_endpoint(uint8_t val) {
    NRIO_D12_CMD = NRIO_D12_CMD_SET_ENDPOINT;
    NRIO_D12_DATA = val;
}

__attribute__((always_inline))
static inline void nrio_d12_set_dma(uint8_t val) {
    NRIO_D12_CMD = NRIO_D12_CMD_SET_DMA;
    NRIO_D12_DATA = val;
}

__attribute__((always_inline))
static inline uint16_t nrio_d12_read_int(void) {
    uint16_t result;
    NRIO_D12_CMD = NRIO_D12_CMD_READ_INT;
    result = NRIO_D12_DATA & 0xFF;
    result |= (NRIO_D12_DATA & 0xFF) << 8;
    return result;
}

__attribute__((always_inline))
static inline void nrio_d12_acknowledge_setup(void) {
    NRIO_D12_CMD = NRIO_D12_CMD_ACK_SETUP;
}

__attribute__((always_inline))
static inline void nrio_d12_clear_buffer(void) {
    NRIO_D12_CMD = NRIO_D12_CMD_CLEAR_BUFFER;
}

__attribute__((always_inline))
static inline void nrio_d12_validate_buffer(void) {
    NRIO_D12_CMD = NRIO_D12_CMD_VALIDATE_BUFFER;
}

__attribute__((always_inline))
static inline void nrio_d12_send_resume(void) {
    NRIO_D12_CMD = NRIO_D12_CMD_SEND_RESUME;
}

__attribute__((always_inline))
static inline void nrio_d12_select_endpoint(uint8_t nrio_addr) {
    NRIO_D12_CMD = NRIO_D12_CMD_SELECT_EP(nrio_addr);
}

__attribute__((always_inline))
static inline uint8_t nrio_d12_select_query_endpoint(uint8_t nrio_addr) {
    NRIO_D12_CMD = NRIO_D12_CMD_SELECT_EP(nrio_addr);
    return NRIO_D12_DATA & 0xFF;
}

__attribute__((always_inline))
static inline void nrio_d12_set_endpoint_status(uint8_t nrio_addr, uint8_t status) {
    NRIO_D12_CMD = NRIO_D12_CMD_SET_STATUS_EP(nrio_addr);
    NRIO_D12_DATA = status;
}

__attribute__((always_inline))
static inline uint8_t nrio_d12_read_endpoint_t_status(uint8_t nrio_addr) {
    NRIO_D12_CMD = NRIO_D12_CMD_T_STATUS_EP(nrio_addr);
    return NRIO_D12_DATA & 0xFF;
}

__attribute__((always_inline))
static inline uint8_t nrio_d12_read_endpoint_status(uint8_t nrio_addr) {
    NRIO_D12_CMD = NRIO_D12_CMD_STATUS_EP(nrio_addr);
    return NRIO_D12_DATA & 0xFF;
}

__attribute__((always_inline))
static inline uint16_t nrio_d12_read_chip_id(void) {
    uint16_t result;
    NRIO_D12_CMD = NRIO_D12_CMD_READ_ID;
    result = NRIO_D12_DATA & 0xFF;
    result |= (NRIO_D12_DATA & 0xFF) << 8;
    return result;
}

#endif /* _NRIO_D12_ */
