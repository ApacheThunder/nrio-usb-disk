/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018, hathach (tinyusb.org)
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

#include "nds/ndstypes.h"
#include "tusb_option.h"

#if CFG_TUD_ENABLED && CFG_TUSB_MCU == OPT_MCU_NRIO

#include <nds.h>
#include "device/dcd.h"

/**
 * Configuration.
 */

// #define CFG_TUD_NRIO_DMA_CHANNEL 3

/**
 * NRIO register defines.
 *
 * To match official firmware naming, "D14" refers to the ISP1581/compatible,
 * while "D12" refers to the PDIUSBD12.
 */

#include "nrio_d12.h"
#include "nrio_d14.h"

/**
 * Helper macros, defines, etc.
 */

// Convert a TinyUSB ep_addr to an NRIO index.
#define tu_edpt_nrio_idx(e) ((((e) << 1) & 0xFE) | ((e) >> 7))
// Convert an NRIO index to a TinyUSB ep_addr.
#define tu_edpt_from_nrio_idx(e) ((((e) << 7) & 0x80) | ((e) >> 1))

#define NRIO_TYPE_D14 1
#define NRIO_TYPE_D12 2

#define NRIO_D14_MAX_PACKET_SIZE 512
#define NRIO_D12_MAX_PACKET_SIZE(index) (((index) < 4) ? 16 : 64)

#define NRIO_D12_MODE_CLK_DEFAULT (NRIO_D12_MODE_CLK_INITIAL | NRIO_D12_MODE_CLK_DIV(8))

#define DCD_BUFFER_LAST_TX 0xFFFFFFFD
#define DCD_BUFFER_NONE    0xFFFFFFFF

#ifndef ARM9
#ifndef REG_EXMEMCNT
#define REG_EXMEMCNT REG_EXMEMSTAT
#endif
#endif

// Device driver state.
static struct {
  uint16_t chip_id;
  uint8_t *buffer[CFG_TUD_ENDPPOINT_MAX * 2];
  uint16_t buffer_length[CFG_TUD_ENDPPOINT_MAX * 2];
  uint16_t buffer_total[CFG_TUD_ENDPPOINT_MAX * 2];
  /**
   * Transfers (particularly host->device) which we have received an IRQ for,
   * but did not yet have a buffer configured; or which have a buffer configured,
   * but did not yet have data on the USB controller.
   */
  uint16_t pending_xfers;
  uint8_t type; // Controller type
  uint8_t d12_mode; // D12 mode register
} _dcd;

// Two endpoint 0 descriptor definition for unified dcd_edpt_open()
static const tusb_desc_endpoint_t ep0OUT_desc =
{
  .bLength          = sizeof(tusb_desc_endpoint_t),
  .bDescriptorType  = TUSB_DESC_ENDPOINT,

  .bEndpointAddress = 0x00,
  .bmAttributes     = { .xfer = TUSB_XFER_CONTROL },
  .wMaxPacketSize   = CFG_TUD_ENDPOINT0_SIZE,
  .bInterval        = 0
};

static const tusb_desc_endpoint_t ep0IN_desc =
{
  .bLength          = sizeof(tusb_desc_endpoint_t),
  .bDescriptorType  = TUSB_DESC_ENDPOINT,

  .bEndpointAddress = 0x80,
  .bmAttributes     = { .xfer = TUSB_XFER_CONTROL },
  .wMaxPacketSize   = CFG_TUD_ENDPOINT0_SIZE,
  .bInterval        = 0
};

TU_ATTR_FAST_FUNC
void dcd_d14_int_handler (void);
TU_ATTR_FAST_FUNC
void dcd_d12_int_handler (void);

/*------------------------------------------------------------------*/
/* Device API
 *------------------------------------------------------------------*/

extern void nrio_d14_tx_bytes(const void *buffer, uint32_t len);
extern void nrio_d14_rx_bytes(void *buffer, uint32_t len);

TU_ATTR_FAST_FUNC
static void nrio_d12_tx_bytes(const void *_buffer, uint32_t len) {
  const uint8_t *buffer = (const uint8_t*) _buffer;
  NRIO_D12_CMD = NRIO_D12_CMD_WRITE_BUFFER;
  NRIO_D12_DATA = 0;
  NRIO_D12_DATA = len;
  while (len--)
    NRIO_D12_DATA = *(buffer++);
}

TU_ATTR_FAST_FUNC
static void nrio_d12_rx_bytes(void *_buffer, uint32_t len) {
  uint8_t *buffer = (uint8_t*) _buffer;
  NRIO_D12_CMD = NRIO_D12_CMD_READ_BUFFER;
  NRIO_D12_DATA;
  uint8_t buf_len = NRIO_D12_DATA;
  if (buf_len > len)
    buf_len = len;
  while (len--)
    *(buffer++) = NRIO_D12_DATA;
}

static void nrio_d12_update_mode(void) {
    NRIO_D12_CMD = NRIO_D12_CMD_SET_MODE;
    NRIO_D12_DATA = _dcd.d12_mode;
    NRIO_D12_DATA = NRIO_D12_MODE_CLK_DEFAULT >> 8;
}

__attribute__((always_inline))
static inline void nrio_d12_set_mode(uint8_t val) {
  if (_dcd.d12_mode != val) {
    _dcd.d12_mode = val;
    nrio_d12_update_mode();
  }
}

static void nrio_d14_tx_packet(uint32_t idx, uint32_t ep_addr, bool after_tx) {
  uint32_t total_bytes = _dcd.buffer_length[idx];
  uint32_t bytes_to_tx = total_bytes;
  if (bytes_to_tx > NRIO_D14_MAX_PACKET_SIZE)
    bytes_to_tx = NRIO_D14_MAX_PACKET_SIZE;

  if (after_tx) {
    total_bytes -= bytes_to_tx;
    _dcd.buffer[idx] += bytes_to_tx;
    _dcd.buffer_length[idx] = total_bytes;

    bytes_to_tx = total_bytes;
    if (!bytes_to_tx) {
      _dcd.buffer[idx] = (void*) DCD_BUFFER_NONE;
      dcd_event_xfer_complete(0, ep_addr, _dcd.buffer_total[idx], XFER_RESULT_SUCCESS, true);
      return;
    }
    if (bytes_to_tx > NRIO_D14_MAX_PACKET_SIZE)
      bytes_to_tx = NRIO_D14_MAX_PACKET_SIZE;
  }

  NRIO_D14_EP_IDX = idx;
  NRIO_D14_EP_BUFLEN = bytes_to_tx;

#ifdef CFG_TUD_NRIO_DMA_CHANNEL
  while (DMA_CR(CFG_TUD_NRIO_DMA_CHANNEL) & DMA_BUSY);

  dmaSetParams(CFG_TUD_NRIO_DMA_CHANNEL, _dcd.buffer[idx], (void*) &NRIO_D14_EP_DATA, DMA_COPY_WORDS | DMA_SRC_INC | DMA_DST_FIX | ((bytes_to_tx + 3) >> 2));
#else
  nrio_d14_tx_bytes(_dcd.buffer[idx], bytes_to_tx);
#endif
}

__attribute__((noinline))
TU_ATTR_FAST_FUNC
static void nrio_d12_tx_packet(uint32_t idx, uint32_t ep_addr, bool after_tx) {
  uint32_t total_bytes = _dcd.buffer_length[idx];

  if (after_tx) {
    if (!total_bytes) {
      // Handle double buffering
      if (idx < 4 || (uintptr_t) _dcd.buffer[idx] == DCD_BUFFER_LAST_TX) {
        _dcd.buffer[idx] = (void*) DCD_BUFFER_NONE;
        dcd_event_xfer_complete(0, ep_addr, _dcd.buffer_total[idx], XFER_RESULT_SUCCESS, true);
      } else {
        _dcd.buffer[idx] = (void*) DCD_BUFFER_LAST_TX;
      }

      return;
    }
  }

  int oldIME = enterCriticalSection();

d12_tx_packet:
  uint32_t bytes_to_tx = total_bytes;
  if (bytes_to_tx > NRIO_D12_MAX_PACKET_SIZE(idx))
    bytes_to_tx = NRIO_D12_MAX_PACKET_SIZE(idx);

  nrio_d12_select_endpoint(idx);
  nrio_d12_tx_bytes(_dcd.buffer[idx], bytes_to_tx);
  nrio_d12_validate_buffer();

  total_bytes -= bytes_to_tx;
  _dcd.buffer[idx] += bytes_to_tx;
  _dcd.buffer_length[idx] = total_bytes;

  // Handle double buffering
  if (idx >= 4) {
    if (!after_tx) {
      if (total_bytes) {
        // Queue second packet
        after_tx = true;
        goto d12_tx_packet;
      } else {
        // Mark transaction as last in queue
        _dcd.buffer[idx] = (void*) DCD_BUFFER_LAST_TX;
      }
    }
  }

  leaveCriticalSection(oldIME);
}

static void dcd_clear_after_bus_reset (void) {
  int oldIME = enterCriticalSection();
  memset(_dcd.buffer, 0xFF, sizeof(_dcd.buffer));
  _dcd.pending_xfers = 0;
  leaveCriticalSection(oldIME);
}

static void nrio_d14_bus_init (void) {
  dcd_clear_after_bus_reset();

  // Configure interrupts
  NRIO_D14_INT_CFG = NRIO_D14_INT_CFG_CDBGMOD(1) | NRIO_D14_INT_CFG_DDBGMODIN(1) | NRIO_D14_INT_CFG_DDBGMODOUT(1)
    | NRIO_D14_INT_CFG_SIG_LEVEL | NRIO_D14_INT_CFG_POL_HIGH;

  // Default interrupt enable masks.
  // SOF is enabled on demand; PSOF and DMA are not used.
  NRIO_D14_INT_ENL = NRIO_D14_INTL_BRESET | NRIO_D14_INTL_SUSP | NRIO_D14_INTL_RESUME
    | NRIO_D14_INTL_EP0SETUP | NRIO_D14_INTL_EP0TX | NRIO_D14_INTL_EP0RX;
  NRIO_D14_INT_ENH = 0;

  // Configure control endpoints
  dcd_edpt_open(0, &ep0OUT_desc);
  dcd_edpt_open(0, &ep0IN_desc);

  // Enable device
  NRIO_D14_DEVEN = NRIO_D14_DEVEN_ENABLE;
}

static void nrio_d12_bus_init (void) {
  dcd_clear_after_bus_reset();
}

static void nrio_d14_reset (void) {
  // Reset USB controller
  NRIO_D14_MODE = NRIO_D14_MODE_SFRESET;
  swiDelay(8378); // 1ms
  NRIO_D14_MODE = 0;
  swiDelay(8378); // 1ms

  NRIO_D14_MODE = NRIO_D14_MODE_GLINTENA;
}

uint32_t dcd_read_chip_id (void) {
  if (_dcd.type == NRIO_TYPE_D14) {
    return ((NRIO_D14_CHIP_IDH << 16) | NRIO_D14_CHIP_IDL) & 0xFFFFFF;
  } else { /* NRIO_TYPE_D12 */
    return nrio_d12_read_chip_id() << 8;
  }
}

// Initialize controller to device mode
bool dcd_init(uint8_t rhport, const tusb_rhport_init_t* rh_init) {
  if (isDSiMode())
    return false;

  // Configure GBA cartridge bus
#ifdef ARM9
  sysSetCartOwner(BUS_OWNER_ARM9);
#endif
  REG_EXMEMCNT = (REG_EXMEMCNT & ~0x1F) | EXMEMCNT_ROM_TIME1_6_CYCLES
    | EXMEMCNT_ROM_TIME2_4_CYCLES | EXMEMCNT_SRAM_TIME_6_CYCLES;

  // Check if the chip is an ISP1581/82/83
  uint32_t d14_chip_id = ((NRIO_D14_CHIP_IDH << 16) | NRIO_D14_CHIP_IDL) & 0xFFFFFF;
  if (d14_chip_id >= 0x158100 && d14_chip_id < 0x158400) {
    _dcd.type = NRIO_TYPE_D14;
    _dcd.chip_id = d14_chip_id >> 8;
    TU_LOG(3, "Detected D14 chip (%06X)\n", (int) d14_chip_id);
  } else {
    // The D12 supports a maximum readout speed of 2 MHz.
    REG_EXMEMCNT = (REG_EXMEMCNT & ~0x1F) | EXMEMCNT_ROM_TIME1_18_CYCLES
      | EXMEMCNT_ROM_TIME2_6_CYCLES | EXMEMCNT_SRAM_TIME_18_CYCLES;

    // Check if the chip is an PDIUSBD12 (do other compatible chips exist?)
    uint16_t d12_chip_id = nrio_d12_read_chip_id();
    uint8_t d12_open_bus = NRIO_D12_DATA;
    if (!((d12_chip_id >> 8) == d12_open_bus && (d12_chip_id & 0xFF) == d12_open_bus)) {
      _dcd.type = NRIO_TYPE_D12;
      _dcd.chip_id = d12_chip_id;

      TU_LOG(3, "Detected D12 chip (%04X)\n", chip_id);
    } else {
      return false;
    }
  }

  irqSet(IRQ_CART, (VoidFn) (_dcd.type == NRIO_TYPE_D14 ? dcd_d14_int_handler : dcd_d12_int_handler));
  irqEnable(IRQ_CART);

  if (_dcd.type == NRIO_TYPE_D14) {
    nrio_d14_reset();
    nrio_d14_bus_init();
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_bus_init();

    _dcd.d12_mode = 0xFF;
    nrio_d12_set_mode(0);
    nrio_d12_set_dma(NRIO_D12_DMA_CFG_IRQ_PKT | NRIO_D12_DMA_CFG_IRQ_VALID);
#if defined(ARM9) && defined(__BLOCKSDS__)
    for (int i = 0; i < 60; i++) cothread_yield_irq(IRQ_VBLANK);
#else
    for (int i = 0; i < 60; i++) swiWaitForVBlank();
#endif
  }
  dcd_connect(rhport);
  return true;
}

bool dcd_deinit(uint8_t rhport) {
  // Check if the chip is an ISP1581/82/83
  if (_dcd.type == 0)
    return true;

  irqDisable(IRQ_CART);

  if (_dcd.type == NRIO_TYPE_D14) {
    // Reset USB controller
    NRIO_D14_MODE = NRIO_D14_MODE_SFRESET;
    swiDelay(8378); // 1ms
    NRIO_D14_MODE = 0;
    swiDelay(8378); // 1ms

    // Suspend USB controller
    NRIO_D14_MODE = NRIO_D14_MODE_GOSUSP;
    swiDelay(8378); // 1ms
    NRIO_D14_MODE = 0;
    swiDelay(8378); // 1ms
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_set_endpoint(0);
    nrio_d12_set_address(0);
  }

  _dcd.type = 0;
  return true;
}

// Enable device interrupt
void dcd_int_enable (uint8_t rhport) {
  (void) rhport;

  if (_dcd.type == NRIO_TYPE_D14) {
    if (_dcd.pending_xfers) {
      int oldIME = enterCriticalSection();
      NRIO_D14_MODE |= NRIO_D14_MODE_GLINTENA;
      dcd_d14_int_handler();
      leaveCriticalSection(oldIME);
    } else {
      NRIO_D14_MODE |= NRIO_D14_MODE_GLINTENA;
    }
  } else { /* NRIO_TYPE_D12 */
    if (_dcd.pending_xfers) {
      int oldIME = enterCriticalSection();
      irqEnable(IRQ_CART);
      dcd_d12_int_handler();
      leaveCriticalSection(oldIME);
    } else {
      irqEnable(IRQ_CART);
    }
  }
}

// Disable device interrupt
void dcd_int_disable (uint8_t rhport) {
  (void) rhport;

  if (_dcd.type == NRIO_TYPE_D14) {
    NRIO_D14_MODE &= ~NRIO_D14_MODE_GLINTENA;
  } else { /* NRIO_TYPE_D12 */
    irqDisable(IRQ_CART);
  }
}

/**
 * @brief Attempt handling a TX/RX event.
 * 
 * @param index NRIO-format address.
 * @param ep_addr TinyUSB-format address.
 * @param in_isr True if executed in IRQ handler.
 * @return bool Whether or not the event was processed.
 */
TU_ATTR_FAST_FUNC
static bool nrio_d14_xfer_handle (uint32_t index, uint32_t ep_addr) {
  if (((uintptr_t) _dcd.buffer[index]) == DCD_BUFFER_NONE)
    return false;

  if (tu_edpt_dir(ep_addr) == TUSB_DIR_OUT) {
    // Handle Host->Device transfers
    uint16_t total_left_to_read = _dcd.buffer_length[index];
    NRIO_D14_EP_IDX = index;
    uint16_t bytes_in_buffer = NRIO_D14_EP_BUFLEN;
    if (total_left_to_read && !bytes_in_buffer)
      return false;
    if (total_left_to_read > bytes_in_buffer)
      total_left_to_read = bytes_in_buffer;

    // Copy data from NRIO to buffer
    nrio_d14_rx_bytes(_dcd.buffer[index], total_left_to_read);

    // Signal end of transfer
    _dcd.buffer_length[index] -= total_left_to_read;
    if (bytes_in_buffer < NRIO_D14_MAX_PACKET_SIZE || !_dcd.buffer_length[index]) {
      _dcd.buffer[index] = (void*) DCD_BUFFER_NONE;
      dcd_event_xfer_complete(0, ep_addr, _dcd.buffer_total[index] - _dcd.buffer_length[index], XFER_RESULT_SUCCESS, true);
    } else {
      _dcd.buffer[index] += total_left_to_read;
    }
  } else {
    // Handle Device->Host transfers
    nrio_d14_tx_packet(index, ep_addr, true);
  }

  return true;
}

TU_ATTR_FAST_FUNC
static bool nrio_d12_xfer_handle (uint32_t index, uint32_t ep_addr) {
  if (((uintptr_t) _dcd.buffer[index]) == DCD_BUFFER_NONE)
    return false;

  if (tu_edpt_dir(ep_addr) == TUSB_DIR_OUT) {
    // Handle Host->Device transfers
    uint8_t query = nrio_d12_select_query_endpoint(index);
    uint8_t bytes_in_buffer = 0;

    bool two_buffers = false;
    if (index >= 4) {
      uint8_t status = nrio_d12_read_endpoint_status(index);
      two_buffers = (status & (NRIO_D12_STATUS_BUF0_FULL | NRIO_D12_STATUS_BUF1_FULL)) == (NRIO_D12_STATUS_BUF0_FULL | NRIO_D12_STATUS_BUF1_FULL);
    }

d12_read_next_buffer:
    uint16_t total_left_to_read = _dcd.buffer_length[index];
    if (query & NRIO_D12_SELECT_EP_FULL) {
      NRIO_D12_CMD = NRIO_D12_CMD_READ_BUFFER;
      NRIO_D12_DATA;
      bytes_in_buffer = NRIO_D12_DATA;
      if (total_left_to_read > bytes_in_buffer)
        total_left_to_read = bytes_in_buffer;

      // Copy data from NRIO to buffer
      for (int i = 0; i < total_left_to_read; i++)
        _dcd.buffer[index][i] = NRIO_D12_DATA;
      nrio_d12_clear_buffer();

      // Signal end of transfer
      _dcd.buffer_length[index] -= total_left_to_read;
    } else { /* NRIO_D12_SELECT_EP_EMPTY */
      if (total_left_to_read)
        return false;
    }

    if (bytes_in_buffer < NRIO_D12_MAX_PACKET_SIZE(index) || !_dcd.buffer_length[index]) {
      _dcd.buffer[index] = (void*) DCD_BUFFER_NONE;
      dcd_event_xfer_complete(0, ep_addr, _dcd.buffer_total[index] - _dcd.buffer_length[index], XFER_RESULT_SUCCESS, true);
    } else {
      _dcd.buffer[index] += total_left_to_read;
      if (two_buffers) {
        two_buffers = false;
        query = nrio_d12_select_query_endpoint(index);
        goto d12_read_next_buffer;
      }
    }
  } else {
    // Handle Device->Host transfers
    nrio_d12_tx_packet(index, ep_addr, true);
  }

  return true;
}

TU_ATTR_FAST_FUNC
void dcd_d14_int_handler (void) {
  uint32_t _setup_packet[2];
  // EPxTX/EPxRX interrupt flags.
  uint32_t xfers = 0;

  uint16_t mask;
  mask = NRIO_D14_INT_STL;
  NRIO_D14_INT_STL = mask;
  if (mask) {
    if (mask & NRIO_D14_INTL_BRESET) {
      NRIO_D14_INT_STL = 0xFFFF;
      NRIO_D14_INT_STH = 0xFFFF;
      nrio_d14_bus_init();
      dcd_event_bus_reset(0, TUSB_SPEED_HIGH, true);
      return;
    } else {
      xfers |= (mask >> 10);

      if (mask & NRIO_D14_INTL_SOF) {
        dcd_event_sof(0, NRIO_D14_FRN, true);
      }
      if (mask & NRIO_D14_INTL_SUSP) {
        dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
      }
      if (mask & NRIO_D14_INTL_RESUME) {
        dcd_event_bus_signal(0, DCD_EVENT_RESUME, true);
        NRIO_D14_UNLOCK = NRIO_D14_UNLOCK_CODE;
      }
      if (mask & NRIO_D14_INTL_EP0SETUP) {
        NRIO_D14_EP_IDX = NRIO_D14_EP_IDX_EP0_SETUP;
        _setup_packet[0] = NRIO_D14_EP_DATA32;
        _setup_packet[1] = NRIO_D14_EP_DATA32;
        dcd_event_setup_received(0, (uint8_t*) _setup_packet, true);
      }
    }
  }

  mask = NRIO_D14_INT_STH;
  NRIO_D14_INT_STH = mask;
  xfers |= (mask << 6) & 0xFFC0;

  xfers |= _dcd.pending_xfers;
  // Handle TX/RX interrupts.
  while (xfers) {
    uint32_t index = __builtin_clz(xfers) ^ 0x1F;
    if (nrio_d14_xfer_handle(index, tu_edpt_from_nrio_idx(index))) {
      _dcd.pending_xfers &= ~(1 << index);
    } else {
      _dcd.pending_xfers |= (1 << index);
    }
    xfers &= ~(1 << index);
  }
}

TU_ATTR_FAST_FUNC
void dcd_d12_int_handler (void) {
  uint32_t _setup_packet[2];
  uint16_t mask = nrio_d12_read_int();

  if (mask & NRIO_D12_INT_BUS_RESET) {
    nrio_d12_bus_init();
    dcd_event_bus_reset(0, TUSB_SPEED_FULL, true);
  }
  if (mask & NRIO_D12_INT_SUSPEND_CHANGE) {
    dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
  }

  // Handle endpoint events
  uint32_t xfers = mask & 0x3F;
  xfers |= _dcd.pending_xfers;
  while (xfers) {
    uint32_t index = __builtin_clz(xfers) ^ 0x1F;
    mask = nrio_d12_read_endpoint_t_status(index);
#if 0
    if (mask & NRIO_D12_T_ERROR_MASK) {
      TU_LOG(1, "DCD EP %02X Error %X\n", (int) tu_edpt_from_nrio_idx(index), (int) ((mask >> 1) & 0xF)); */
#endif
    if (mask & NRIO_D12_T_STATUS_SETUP) {
      nrio_d12_select_endpoint(index);
      nrio_d12_rx_bytes(_setup_packet, 8);
      nrio_d12_acknowledge_setup();
      nrio_d12_clear_buffer();
      // 11.3.10 - acknowledge SETUP on IN buffer too
      nrio_d12_select_endpoint(index | 1);
      nrio_d12_acknowledge_setup();
      nrio_d12_clear_buffer();

      dcd_event_setup_received(0, (uint8_t*) _setup_packet, true);
    } else {
      if (nrio_d12_xfer_handle(index, tu_edpt_from_nrio_idx(index))) {
        _dcd.pending_xfers &= ~(1 << index);
      } else {
        _dcd.pending_xfers |= (1 << index);
      }
    }
    xfers &= ~(1 << index);
  }
}

TU_ATTR_FAST_FUNC
void dcd_int_handler (uint8_t rhport) {
  if (_dcd.type == NRIO_TYPE_D14) {
    dcd_d14_int_handler();
  } else { /* NRIO_TYPE_D12 */
    dcd_d12_int_handler();
  }
}

// Receive Set Address request, mcu port must also include status IN response
void dcd_set_address (uint8_t rhport, uint8_t dev_addr) {
  if (_dcd.type == NRIO_TYPE_D14) {
    NRIO_D14_DEVEN = NRIO_D14_DEVEN_ENABLE | NRIO_D14_DEVEN_ADDR(dev_addr);
  } else {
    nrio_d12_set_address(NRIO_D12_ADDRESS_ENABLE | NRIO_D12_ADDRESS(dev_addr));
  }
  dcd_edpt_xfer(rhport, tu_edpt_addr(0, TUSB_DIR_IN), NULL, 0);
}

// Wake up host
void dcd_remote_wakeup (uint8_t rhport) {
  (void) rhport;

  if (_dcd.type == NRIO_TYPE_D14) {
    NRIO_D14_MODE |= NRIO_D14_MODE_SNDRSU;
    NRIO_D14_MODE &= ~NRIO_D14_MODE_SNDRSU;
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_send_resume();
  }
}

// Connect by enabling internal pull-up resistor on D+/D-
void dcd_connect(uint8_t rhport) {
  (void) rhport;
  if (_dcd.type == NRIO_TYPE_D14) {
    NRIO_D14_MODE |= NRIO_D14_MODE_SOFTCT;
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_set_mode(_dcd.d12_mode | NRIO_D12_MODE_CFG_SOFTCT);
  }
}

// Disconnect by disabling internal pull-up resistor on D+/D-
void dcd_disconnect(uint8_t rhport) {
  (void) rhport;
  if (_dcd.type == NRIO_TYPE_D14) {
    NRIO_D14_MODE &= ~NRIO_D14_MODE_SOFTCT;
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_set_mode(_dcd.d12_mode & ~NRIO_D12_MODE_CFG_SOFTCT);
  }
}

uint8_t dcd_edpt0_get_size(void) {
  return _dcd.type == NRIO_TYPE_D12 ? 16 : 64;
}

void dcd_sof_enable(uint8_t rhport, bool en) {
  (void) rhport;
  (void) en;

  if (_dcd.type == NRIO_TYPE_D14) {
    if (en) {
      NRIO_D14_INT_ENL |= NRIO_D14_INTL_SOF;
    } else {
      NRIO_D14_INT_ENL &= ~NRIO_D14_INTL_SOF;
    }
  } else { /* NRIO_TYPE_D12 */
    // TODO D12
  }
}

#if CFG_TUD_TEST_MODE
void dcd_enter_test_mode(uint8_t rhport, tusb_feature_test_mode_t test_selector) {
  if (_dcd.type != NRIO_TYPE_D14)
    return;

  switch (test_selector) {
    case TUSB_FEATURE_TEST_J:
      NRIO_D14_TEST = NRIO_D14_TEST_JSTATE;
      break;
    case TUSB_FEATURE_TEST_K:
      NRIO_D14_TEST = NRIO_D14_TEST_KSTATE;
      break;
    case TUSB_FEATURE_TEST_SE0_NAK:
      NRIO_D14_TEST = NRIO_D14_TEST_SE0_NAK;
      break;
  }
}
#endif

//--------------------------------------------------------------------+
// Endpoint API
//--------------------------------------------------------------------+

// Configure endpoint's registers according to descriptor
bool dcd_edpt_open (uint8_t rhport, tusb_desc_endpoint_t const * ep_desc) {
  (void) rhport;
  uint32_t idx = tu_edpt_nrio_idx(ep_desc->bEndpointAddress);

  if (_dcd.type == NRIO_TYPE_D14) {
    // Enable EP interrupt
    if (idx >= 6) {
      NRIO_D14_INT_ENH |= (1 << (idx - 6));
    } else if (idx >= 2) {
      NRIO_D14_INT_ENL |= (1 << (10 + idx));
    }

    NRIO_D14_EP_IDX = idx;
    // Enable double-buffering for Host->Device transfers.
    // TODO: Enable double-buffering for Device->Host transfers?
    NRIO_D14_EP_TYPE = ep_desc->bmAttributes.xfer | NRIO_D14_EP_TYPE_NOEMPKT
      | (tu_edpt_dir(ep_desc->bEndpointAddress) == TUSB_DIR_OUT ? NRIO_D14_EP_TYPE_DBLBUF : 0);
    NRIO_D14_EP_PKT = ep_desc->wMaxPacketSize;
    NRIO_D14_EP_CFG = NRIO_D14_EP_CFG_CLBUF;
    NRIO_D14_EP_IDX = idx;
    NRIO_D14_EP_CFG = NRIO_D14_EP_CFG_CLBUF;
    NRIO_D14_EP_TYPE |= NRIO_D14_EP_TYPE_ENABLE;
  } else { /* NRIO_TYPE_D12 */
    uint32_t num = tu_edpt_number(ep_desc->bEndpointAddress);
    
    switch (ep_desc->bmAttributes.xfer) {
    case TUSB_XFER_CONTROL:
      if (num != 0)
        return false;
      break;
    case TUSB_XFER_ISOCHRONOUS:
      if (num != 2)
        return false;
      nrio_d12_set_mode(_dcd.d12_mode | NRIO_D12_MODE_CFG_ISO_IO);
      break;
    case TUSB_XFER_BULK:
    case TUSB_XFER_INTERRUPT:
      if (num == 2)
        nrio_d12_set_mode(_dcd.d12_mode & ~NRIO_D12_MODE_CFG_ISO_IO);
      else if (num != 1)
        return false;
      break;
    }

    if (num == 2)
      nrio_d12_set_endpoint(1);
    nrio_d12_set_endpoint_status(idx, 0);
  }

  return true;
}

void dcd_edpt_close_all (uint8_t rhport) {
  (void) rhport;

  if (_dcd.type == NRIO_TYPE_D14) {
    // Disable EP interrupts
    NRIO_D14_INT_ENL &= 0x0FFF;
    NRIO_D14_INT_ENH = 0;

    // Disable EPs
    for (int i = 15; i >= 2; i--) {
      NRIO_D14_EP_IDX = i;
      NRIO_D14_EP_TYPE = 0;
    }
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_set_endpoint(0);
  }
}

// Submit a transfer, When complete dcd_event_xfer_complete() is invoked to notify the stack
bool dcd_edpt_xfer (uint8_t rhport, uint8_t ep_addr, uint8_t * buffer, uint16_t total_bytes) {
  (void) rhport;
  (void) ep_addr;
  (void) buffer;
  (void) total_bytes;

  uint32_t index = tu_edpt_nrio_idx(ep_addr);

#ifdef DEBUG
  if (((uintptr_t) _dcd.buffer[index]) != DCD_BUFFER_NONE)
    return false;
#endif

  if (_dcd.chip_id >= 0x1582 && _dcd.type == NRIO_TYPE_D14 && !total_bytes) {
    // ISP1582/83 do not emit an IRQ for status transfers.
    NRIO_D14_EP_CFG = NRIO_D14_EP_CFG_STATUS;
    dcd_event_xfer_complete(0, ep_addr, total_bytes, XFER_RESULT_SUCCESS, true);
    return true;
  }

  int oldIME = enterCriticalSection();
  _dcd.buffer[index] = (uint8_t*) buffer;
  _dcd.buffer_length[index] = total_bytes;
  _dcd.buffer_total[index] = total_bytes;
  leaveCriticalSection(oldIME);
  
  if (_dcd.type == NRIO_TYPE_D14) {
    if (!total_bytes) {
      // Handle zero-length packets
      NRIO_D14_EP_CFG = NRIO_D14_EP_CFG_STATUS;
    } else {
      if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) {
        // Device to Host - queue data on FIFO, wait for TX interrupt
#ifdef CFG_TUD_NRIO_DMA_CHANNEL
        DC_FlushRange(buffer, total_bytes);
#endif
        nrio_d14_tx_packet(index, ep_addr, false);
      }
    }
  } else { /* NRIO_TYPE_D12 */
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) {
      // Device to Host - queue data on FIFO, wait for TX interrupt
      nrio_d12_tx_packet(index, ep_addr, false);
    }
  }

  return true;
}

// Submit a transfer where is managed by FIFO, When complete dcd_event_xfer_complete() is invoked to notify the stack - optional, however, must be listed in usbd.c
/* bool dcd_edpt_xfer_fifo (uint8_t rhport, uint8_t ep_addr, tu_fifo_t * ff, uint16_t total_bytes) {
  (void) rhport;
  (void) ep_addr;
  (void) ff;
  (void) total_bytes;

  // TODO
  return false;
} */

// Stall endpoint
void dcd_edpt_stall (uint8_t rhport, uint8_t ep_addr) {
  (void) rhport;

  if (_dcd.type == NRIO_TYPE_D14) {
    NRIO_D14_EP_IDX = tu_edpt_nrio_idx(ep_addr);
    NRIO_D14_EP_CFG = NRIO_D14_EP_CFG_STALL;
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_set_endpoint_status(tu_edpt_nrio_idx(ep_addr), NRIO_D12_SET_STATUS_STALL);
  }
}

// clear stall, data toggle is also reset to DATA0
void dcd_edpt_clear_stall (uint8_t rhport, uint8_t ep_addr) {
  (void) rhport;

  if (_dcd.type == NRIO_TYPE_D14) {
    NRIO_D14_EP_IDX = tu_edpt_nrio_idx(ep_addr);
    NRIO_D14_EP_CFG = 0;

    // Reset data toggle bit
    NRIO_D14_EP_TYPE &= ~NRIO_D14_EP_TYPE_ENABLE;
    NRIO_D14_EP_TYPE |= NRIO_D14_EP_TYPE_ENABLE;
  } else { /* NRIO_TYPE_D12 */
    nrio_d12_set_endpoint_status(tu_edpt_nrio_idx(ep_addr), 0);
  }
}

#endif
