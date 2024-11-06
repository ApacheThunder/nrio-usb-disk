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

#ifndef _NRIO_D14_
#define _NRIO_D14_

// NRIO "D14" (ISP1581; TinyUSB driver should also be 1582/1583 compatible)

#define NRIO_D14_REG(index) GBA_BUS[(index) * 0x10000]

#define NRIO_D14_DEVEN_ENABLE  BIT(7)
#define NRIO_D14_DEVEN_ADDR(n) (n)
#define NRIO_D14_DEVEN     NRIO_D14_REG(0x00)

#define NRIO_D14_MODE_CLKAON   BIT(7)
#define NRIO_D14_MODE_SNDRSU   BIT(6)
#define NRIO_D14_MODE_GOSUSP   BIT(5)
#define NRIO_D14_MODE_SFRESET  BIT(4)
#define NRIO_D14_MODE_GLINTENA BIT(3)
#define NRIO_D14_MODE_WKUPCS   BIT(2)
#define NRIO_D14_MODE_PWROFF   BIT(1)
#define NRIO_D14_MODE_SOFTCT   BIT(0)
#define NRIO_D14_MODE      NRIO_D14_REG(0x0C)

#define NRIO_D14_INT_CFG_POL_LOW       (0)
#define NRIO_D14_INT_CFG_POL_HIGH      BIT(0)
#define NRIO_D14_INT_CFG_SIG_LEVEL     (0)
#define NRIO_D14_INT_CFG_SIG_PULSED    BIT(1)
#define NRIO_D14_INT_CFG_DDBGMODOUT(n) ((n) << 2)
#define NRIO_D14_INT_CFG_DDBGMODIN(n)  ((n) << 4)
#define NRIO_D14_INT_CFG_CDBGMOD(n)    ((n) << 6)
#define NRIO_D14_INT_CFG   NRIO_D14_REG(0x10)

#define NRIO_D14_INTL_BRESET   BIT(0)
#define NRIO_D14_INTL_SOF      BIT(1)
#define NRIO_D14_INTL_PSOF     BIT(2)
#define NRIO_D14_INTL_SUSP     BIT(3)
#define NRIO_D14_INTL_RESUME   BIT(4)
#define NRIO_D14_INTL_HS_STAT  BIT(5)
#define NRIO_D14_INTL_DMA      BIT(6)
#define NRIO_D14_INTL_EP0SETUP BIT(8)
#define NRIO_D14_INTL_EP0RX    BIT(10)
#define NRIO_D14_INTL_EP0TX    BIT(11)
#define NRIO_D14_INTL_EP1RX    BIT(12)
#define NRIO_D14_INTL_EP1TX    BIT(13)
#define NRIO_D14_INTL_EP2RX    BIT(14)
#define NRIO_D14_INTL_EP2TX    BIT(15)
#define NRIO_D14_INTL_ALL      0xFD7F
#define NRIO_D14_INTL_ALL_NON_EDPT 0x017F
#define NRIO_D14_INTH_EP3RX    BIT(0)
#define NRIO_D14_INTH_EP3TX    BIT(1)
#define NRIO_D14_INTH_EP4RX    BIT(2)
#define NRIO_D14_INTH_EP4TX    BIT(3)
#define NRIO_D14_INTH_EP5RX    BIT(4)
#define NRIO_D14_INTH_EP5TX    BIT(5)
#define NRIO_D14_INTH_EP6RX    BIT(6)
#define NRIO_D14_INTH_EP6TX    BIT(7)
#define NRIO_D14_INTH_EP7RX    BIT(8)
#define NRIO_D14_INTH_EP7TX    BIT(9)
#define NRIO_D14_INTH_ALL      0x03FF
#define NRIO_D14_INTH_ALL_NON_EDPT 0x0000
#define NRIO_D14_INT_ENL   NRIO_D14_REG(0x14)
#define NRIO_D14_INT_ENH   NRIO_D14_REG(0x16)
#define NRIO_D14_INT_STL   NRIO_D14_REG(0x18)
#define NRIO_D14_INT_STH   NRIO_D14_REG(0x1A)

#define NRIO_D14_EP_IDX_OUT       (0)
#define NRIO_D14_EP_IDX_IN        BIT(0)
#define NRIO_D14_EP_IDX_IDX(n)    ((n) << 1)
#define NRIO_D14_EP_IDX_EP0_DATA  (0)
#define NRIO_D14_EP_IDX_EP0_SETUP BIT(5)
#define NRIO_D14_EP_IDX    NRIO_D14_REG(0x2C)

#define NRIO_D14_EP_CFG_STALL         BIT(0)
#define NRIO_D14_EP_CFG_STATUS        BIT(1)
#define NRIO_D14_EP_CFG_VENDP         BIT(3)
#define NRIO_D14_EP_CFG_CLBUF         BIT(4)
#define NRIO_D14_EP_CFG    NRIO_D14_REG(0x28)

#define NRIO_D14_EP_DATA   NRIO_D14_REG(0x20)
#define NRIO_D14_EP_DATA32 (*((vu32*) &NRIO_D14_REG(0x20)))
#define NRIO_D14_EP_BUFLEN NRIO_D14_REG(0x1C)

#define NRIO_D14_EP_PKT_FIFO_LEN(n) (n)
#define NRIO_D14_EP_PKT_NTRANS(n)   (((n) - 1) << 11)
#define NRIO_D14_EP_PKT    NRIO_D14_REG(0x04)

#define NRIO_D14_EP_TYPE_CTRL    (0)
#define NRIO_D14_EP_TYPE_ISO     (1)
#define NRIO_D14_EP_TYPE_BULK    (2)
#define NRIO_D14_EP_TYPE_INT     (3)
#define NRIO_D14_EP_TYPE_DBLBUF  BIT(2)
#define NRIO_D14_EP_TYPE_ENABLE  BIT(3)
#define NRIO_D14_EP_TYPE_NOEMPKT BIT(4)
#define NRIO_D14_EP_TYPE   NRIO_D14_REG(0x08)

#define NRIO_D14_EP_SHORT_OUT(n) ((n) << 8)
#define NRIO_D14_EP_SHORT_OUT0   NRIO_D14_EP_SHORT_OUT(0)
#define NRIO_D14_EP_SHORT_OUT1   NRIO_D14_EP_SHORT_OUT(1)
#define NRIO_D14_EP_SHORT_OUT2   NRIO_D14_EP_SHORT_OUT(2)
#define NRIO_D14_EP_SHORT_OUT3   NRIO_D14_EP_SHORT_OUT(3)
#define NRIO_D14_EP_SHORT_OUT4   NRIO_D14_EP_SHORT_OUT(4)
#define NRIO_D14_EP_SHORT_OUT5   NRIO_D14_EP_SHORT_OUT(5)
#define NRIO_D14_EP_SHORT_OUT6   NRIO_D14_EP_SHORT_OUT(6)
#define NRIO_D14_EP_SHORT_OUT7   NRIO_D14_EP_SHORT_OUT(7)
#define NRIO_D14_EP_SHORT  NRIO_D14_REG(0x24)

#define NRIO_D14_CHIP_IDL  NRIO_D14_REG(0x70)
#define NRIO_D14_CHIP_IDH  NRIO_D14_REG(0x72)

#define NRIO_D14_FRN_MICROSOF(n) ((n) << 11)
#define NRIO_D14_FRN_SOF(n)      (n)
#define NRIO_D14_FRN       NRIO_D14_REG(0x74)
#define NRIO_D14_SCRATCH   NRIO_D14_REG(0x78)

#define NRIO_D14_UNLOCK_CODE 0xAA37
#define NRIO_D14_UNLOCK    NRIO_D14_REG(0x7C)

#define NRIO_D14_TEST_FORCEHS BIT(7)
#define NRIO_D14_TEST_PHYTEST BIT(6)
#define NRIO_D14_TEST_LPBK    BIT(5)
#define NRIO_D14_TEST_FORCEFS BIT(4)
#define NRIO_D14_TEST_PRBS    BIT(3)
#define NRIO_D14_TEST_KSTATE  BIT(2)
#define NRIO_D14_TEST_JSTATE  BIT(1)
#define NRIO_D14_TEST_SE0_NAK BIT(0)
#define NRIO_D14_TEST      NRIO_D14_REG(0x84)

#endif /* _NRIO_D14_ */
