/*
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* Register definitions and offsets for the SB1250 integrated peripherals */

/* Base addresses for each of the cpus */
#include <asm/addrspace.h>

#define SB1250_SCD_BASE        (KSEG1 + 0x10020000)

/* All the SCD registers are duplicated for each cpu. */
#define SB1250_CPU0_OFS                 0
#define SB1250_CPU1_OFS            0x2000

#define SB1250_INT_DIAG         (SB1250_SCD_BASE + 0x10)
#define SB1250_INT_LDT_SET      (SB1250_SCD_BASE + 0x48)
#define SB1250_INT_LDT          (SB1250_SCD_BASE + 0x18)
#define SB1250_INT_LDT_CLR      (SB1250_SCD_BASE + 0x20)
#define SB1250_INT_MASK         (SB1250_SCD_BASE + 0x28)
#define SB1250_INT_STATUS       (SB1250_SCD_BASE + 0x100)
#define SB1250_INT_SRC_STATUS   (SB1250_SCD_BASE + 0x040)
#define SB1250_INT_TRC          (SB1250_SCD_BASE + 0x38)
#define SB1250_INT_MAP          (SB1250_SCD_BASE + 0x200)

/* General purpose timer configuration registers */
#define SB1250_GPT_CFG0         (SB1250_SCD_BASE + 0x090)
#define SB1250_GPT_CFG1         (SB1250_SCD_BASE + 0x098)
#define SB1250_GPT_CFG2         (SB1250_SCD_BASE + 0x190)
#define SB1250_GPT_CFG3         (SB1250_SCD_BASE + 0x198)

#define SB1250_GPT_INT_MAP0     (SB1250_INT_MAP + (2<<3))
#define SB1250_GPT_INT_MAP1     (SB1250_INT_MAP + (3<<3))
#define SB1250_GPT_INT_MAP2     (SB1250_INT_MAP + (4<<3))
#define SB1250_GPT_INT_MAP3     (SB1250_INT_MAP + (5<<3))

/* General purpose timer configuration bitfields */
#define SB1250_GPT_CFG_ENABLE      0x1
#define SB1250_GPT_CFG_CONTINUOUS  0x2

/* General purpose timer count initialization registers */
#define SB1250_GPT_INIT_CNT0    (SB1250_SCD_BASE + 0x070)
#define SB1250_GPT_INIT_CNT1    (SB1250_SCD_BASE + 0x078)
#define SB1250_GPT_INIT_CNT2    (SB1250_SCD_BASE + 0x170)
#define SB1250_GPT_INIT_CNT3    (SB1250_SCD_BASE + 0x178)

/* Interrupt map values */
#define SB1250_MAP_IP2             0
#define SB1250_MAP_IP3             1
#define SB1250_MAP_IP4             2
#define SB1250_MAP_IP5             3
#define SB1250_MAP_IP6             4
#define SB1250_MAP_IP7             5
#define SB1250_MAP_NMI             6
#define SB1250_MAP_DEBUG           7


/* Owners of the various interrupts at the scd level.  Note this
   is the pre mapping values. */

#define SB1250_INT_WATCHDOG_0            0
#define SB1250_INT_WATCHDOG_1            1
#define SB1250_INT_TIMER_0               2
#define SB1250_INT_TIMER_1               3
#define SB1250_INT_TIMER_2               4
#define SB1250_INT_TIMER_3               5
#define SB1250_INT_SMB_0                 6
#define SB1250_INT_SMB_1                 7
#define SB1250_INT_UART_0                8
#define SB1250_INT_UART_1                9

/* FIXME!  Need to add the rest */
 

/*
 * There are 2 UARTS.  Technically you can access them via
 * some unified registers, but the seperated interfaces are 
 * much cleaner 
 */

#define SB1250_UART_0_OFS            0x000  
#define SB1250_UART_1_OFS            0x100  

#define SB1250_DUART_MODE_1   (KSEG1 + 0x10060100)
#define SB1250_DUART_MODE_2   (KSEG1 + 0x10060110)
#define SB1250_DUART_CMD      (KSEG1 + 0x10060150)
#define SB1250_DUART_STATUS   (KSEG1 + 0x10060120)
#define SB1250_DUART_BAUD     (KSEG1 + 0x10060130)
#define SB1250_DUART_RX       (KSEG1 + 0x10060160)
#define SB1250_DUART_TX       (KSEG1 + 0x10060170)
#define SB1250_DUART_CLK_SEL  (KSEG1 + 0x10060130)
#define SB1250_DUART_IMR0     (KSEG1 + 0x10060330)
#define SB1250_DUART_IMR1     (KSEG1 + 0x10060350)

/* 
 *   Mode register 1 fields 
 */
/* Bits per character */
#define SB1250_DM1_8BPC        0x0
#define SB1250_DM1_7BPR        0x1

/* Parity type */
#define SB1250_DM1_EVEN_PAR    0x0
#define SB1250_DM1_LOW_PAR     0x0
#define SB1250_DM1_ODD_PAR     0x4
#define SB1250_DM1_HIGH_PAR    0x4

/* Parity mode */
#define SB1250_DM1_PAR          0x0
#define SB1250_DM1_FIXED_PAR    0x8
#define SB1250_DM1_NO_PAR      0x10

/* Recieve IRQ select */
#define SB1250_DM1_IRQ_READY   0x00
#define SB1250_DM1_IRQ_FULL    0x40

/* RTS flow control */
#define SB1250_DM1_RTS         0x80

/* 
 *   Mode register 2 fields 
 */
/* Stop bit length */
#define SB1250_DM2_1_STOP_BIT  0x0
#define SB1250_DM2_2_STOP_BITS 0x8

/* Transmitter CTS flow control enable */
#define SB1250_DM2_CTS         0x10

/* DUART Channel mode */
#define SB1250_DM2_LCL_LOOP    0x80
#define SB1250_DM2_RMT_LOOP    0xc0

/*
 * Command register fields 
 */
/* Any of these can be written */
#define SB1250_DC_RX_EN        0x1
#define SB1250_DC_RX_DIS       0x2
#define SB1250_DC_TX_EN        0x4
#define SB1250_DC_TX_DIS       0x8

/* Command field possibilities.  At most one of these should be written 
   at a time */
#define SB1250_DC_RX_RST      0x20
#define SB1250_DC_TX_RST      0x30
#define SB1250_DC_BRK_CHG_RST 0x50
#define SB1250_DC_BRK_START   0x60
#define SB1250_DC_BRK_STOP    0x70

/* FINISH ME */

#define SB1250_DS_TX_RDY       0x4
#define SB1250_DS_RX_RDY       0x1
#define SB1250_DS_TX_EMT       0x8

/* DUART interrupt source masks for ISR */
#define SB1250_DIM_TX          0x1
#define SB1250_DIM_RX          0x2
#define SB1250_DIM_BRK         0x4
#define SB1250_DIM_IN          0x8

