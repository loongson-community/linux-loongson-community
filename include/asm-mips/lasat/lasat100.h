/* $Id: lasat100.h,v 1.3 2002/08/10 18:19:12 brm Exp $
 *
 * lasat100.h
 *
 * Thomas Horsten <thh@lasat.com>
 * Copyright (C) 2000 LASAT Networks A/S.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Defines of the LASAT 100 registers and base addresses
 *
 */
#ifndef _MIPS_LASAT100_H
#define _MIPS_LASAT100_H

#include <asm/addrspace.h>

/* 
 * RTC-device register.
 */
#define DS1603_RTC_REG          	(KSEG1ADDR(0x1c810000))
#define DS1603_RTC_MASK         	0x7
#define DS1603_RTC_RST          	(1 << 2)
#define DS1603_RTC_CLK          	(1 << 0)
#define DS1603_RTC_DATA_SHIFT           1
#define DS1603_RTC_DATA                 (1 << DS1603_RTC_DATA_SHIFT)
#define DS1603_RTC_DATA_REG             DS1603_RTC_REG
#define DS1603_RTC_DATA_READ            DS1603_RTC_DATA
#define DS1603_RTC_DATA_READ_SHIFT      DS1603_RTC_DATA_SHIFT
#define DS1603_HUGE_DELAY 0
/* for calibration of delays */
#define TICKS_PER_SECOND		(lasat_get_cpu_hz() / 2)

/*
 * Display register.
 */
#define PVC_REG                 KSEG1ADDR(0x1c820000)
#define PVC_DATA_SHIFT          0
#define PVC_DATA_M              0xFF
#define PVC_E                   (1 << 8)
#define PVC_RW                  (1 << 9)
#define PVC_RS                  (1 << 10)

#define LASAT_DISPLAY_REG	PVC_REG

#define LASAT_DISPLAY_BUSY	( 1 << 7 )
#define LASAT_DISPLAY_ENABLE	PVC_E
#define LASAT_DISPLAY_RW	PVC_RW
#define LASAT_DISPLAY_RS	PVC_RS
#define LASAT_DISPLAY_SHIFT	PVC_DATA_SHIFT
#define LASAT_DISPLAY_MASK	0x7ff


/*
 * FPGA programming registers.
 */
#define LASAT_FPGA_STATUS_REG	(KSEG1ADDR(0x1c8a0000))
#define LASAT_FPGA_OUTPUT_REG	(KSEG1ADDR(0x1c830000))
#define LASAT_FPGA_STARTPGM_REG	(KSEG1ADDR(0x1c850000))
#define LASAT_FPGA_ENDPGM_REG	(KSEG1ADDR(0x1c860000))

/*
 * LAN swapping register.
 */
#define LASAT_LANSWAP_REG	(KSEG1ADDR(0x1c810000))

#define LASAT_LANSWAP_LAN1	( 1 << 6 )
#define LASAT_LANSWAP_LAN2	( 1 << 7 )

/*
 * LED control register.
 */
#define LASAT_LED_REG		(KSEG1ADDR(0x1c820000))

#define LASAT_LED0_ENB		( 1 << 11 )
#define LASAT_LED1_ENB		( 1 << 12 )
#define LASAT_LED2_ENB		( 1 << 13 )
#define LASAT_LED_SHIFT		11
#define LASAT_LED_MASK		0x7

/*
 * Serial EEPROM.
 */
#define AT93C_REG               KSEG1ADDR(0x1c810000)
#define AT93C_RDATA_REG         AT93C_REG
#define AT93C_RDATA_SHIFT       4
#define AT93C_RDATA_M           (1 << AT93C_RDATA_SHIFT)
#define AT93C_WDATA_SHIFT       4
#define AT93C_WDATA_M           (1 << AT93C_WDATA_SHIFT)
#define AT93C_CS_M              ( 1 << 5 )
#define AT93C_CLK_M             ( 1 << 3 )

#define LASAT_EEPROM_REG	AT93C_REG
#define LASAT_EEPROM_READ_OFFSET	0
#define LASAT_EEPROM_MASK	0x38
#define LASAT_EEPROM_CLK	AT93C_CLK_M
#define LASAT_EEPROM_WDATA_SHIFT	AT93C_WDATA_SHIFT
#define LASAT_EEPROM_WDATA	AT93C_WDATA_M
#define LASAT_EEPROM_RDATA_SHIFT	AT93C_RDATA_SHIFT
#define LASAT_EEPROM_RDATA	AT93C_RDATA_M
#define LASAT_EEPROM_CS		AT93C_CS_M


/*
 * The board control register.
 */
#define LASAT_BOARD_CONTROL_REG	(KSEG1ADDR(0x1c800000))

/*
 * Register for finding out if Flash Module is present.
 */
#define LASAT_FLASHPRESENT_REG	(KSEG1ADDR(0x1c800000))

/* Test for flash module */
#define LASAT_FLASHPRESENT	( (*(volatile long *)(LASAT_FLASHPRESENT_REG)) & 2)

/*
 * Write protecting the flash
 */
#define LASAT_FLASHWP_REG	(KSEG1ADDR(0x1c800000))
#define LASAT_FLASHWP_MASK	( 1 << 2 )
#define LASAT_FLASHWP_ENB	0
#define LASAT_FLASHWP_DIS	( 1 << 2 )

/*
 * Interrupt controller status and mask registers.
 */
#define LASAT_INT_STATUS_REG	(KSEG1ADDR(0x1c880000))
#define LASAT_INT_MASK_REG	(KSEG1ADDR(0x1c890000))

/*
 * Reset register
 */
#define LASAT_RESET_REG		(KSEG1ADDR(0xbc840000))

/*
 * LASAT UART register base.
 */
#define LASAT_BASE_BAUD 	( 7372800 / 16 ) 
#define LASAT_UART_REGS_BASE	0x1c8b0000
#define LASAT_UART_REGS_SHIFT	2

#define UART_REGSZ 4
/*
 * Galileo GT64120 system controller register base.
 */
#define LASAT_GT_BASE		(KSEG1ADDR(0x14000000))

/*
 * DMA controller 
 */
#define LASAT_DMA_BASE		LASAT_GT_BASE

/*
 * Hi/fn controller.
 */
#define LASAT_HIFN_BASE		(KSEG1ADDR(0x1c000000))

/*
 * Crypto / Address filter FPGA
 */
#define LASAT_FPGA_BASE		(KSEG1ADDR(0x1d000000))


/*
 * The Flash base addresses.
 */
#define LASAT_FLASH_BASE 	(KSEG1ADDR(0x1e000000))

#define LASAT_BOOTCS_BASE	(KSEG1ADDR(0x1fc00000))

/*
 * This is a macro to convert a BootCS address to the corresponding
 * address in the Flash chipselect space 
 */
#define LASAT_BOOTCS_TO_FLASHCS(x) \
  ((PHYSADDR(x) & 0x00ffffff) | (PHYSADDR(LASAT_FLASH_BASE)))

/* Number of IRQ supported on hw interrupt 0. */
/* The Galileo interrupts are virtualized as Int 32-63 */
#define LASATINT_END	64
#define LASATHWINT_END	32
#define LASATINT_MASK_SHIFT	0

#define LASATINT_GT	0
#define LASATINT_ETH0	1
#define LASATINT_ETH1	2
#define LASATINT_HDC	3
#define LASATINT_UEM	4
#define LASATINT_COMP	5
#define LASATINT_CRY	6
#define LASATINT_ISDN	7
#define LASATINT_UART	8

#define LASATINT_GT_DMA0COMP	36
#define LASATINT_GT_DMA1COMP	37
#define LASATINT_GT_DMA2COMP	38
#define LASATINT_GT_DMA3COMP	39

#endif /* _MIPS_LASAT100_H */
