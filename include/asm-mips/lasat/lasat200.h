/*
 *
 * lasat200.h
 *
 * Tommy S. Christensen <tch@lasat.com>
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
 * Defines of the LASAT 200 registers and base addresses
 *
 */
#ifndef _MIPS_LASAT200_H
#define _MIPS_LASAT200_H

#define LASAT_DMA_CHAINED

#include <asm/addrspace.h>

/*
 * System Controller Vrc5074 internal registers.
 */
#define Vrc5074_PHYS_BASE	0x1fa00000
#define Vrc5074_BASE		(KSEG1ADDR(Vrc5074_PHYS_BASE))

/*
 * Boot base address
 */
#define LASAT_BOOT_BASE		(KSEG1ADDR(0x1fc00000))

/*
 * PCI Windows for Memory and I/O accesses
 */
#define PCI_WINDOW0		0x18000000
#define PCI_WINDOW1		0x1a000000

/* 
 * RTC-device register.
 */
#define DS1603_RTC_REG          	(KSEG1ADDR(0x11000000))
#define DS1603_RTC_MASK         	(0x7 << 3)
#define DS1603_RTC_RST          	(1 << 3)
#define DS1603_RTC_CLK          	(1 << 4)
#define DS1603_RTC_DATA_SHIFT           5
#define DS1603_RTC_DATA                 (1 << DS1603_RTC_DATA_SHIFT)
#define DS1603_RTC_DATA_REG             (DS1603_RTC_REG + 0x10000)
#define DS1603_RTC_DATA_READ_SHIFT      9
#define DS1603_RTC_DATA_READ            (1 << DS1603_RTC_DATA_READ_SHIFT)
#define DS1603_HUGE_DELAY 2000
#define DS1603_RTC_DATA_REVERSED
/* for calibration of delays */
#define TICKS_PER_SECOND		(lasat_get_cpu_hz() / 2)


/*
 * Display register.
 */
#define PVC_REG                 KSEG1ADDR(0x11000000)
#define PVC_DATA_SHIFT          24
#define PVC_DATA_M              (0xFF << PVC_DATA_SHIFT)
#define PVC_E                   (1 << 16)
#define PVC_RW                  (1 << 17)
#define PVC_RS                  (1 << 18)

#define LASAT_DISPLAY_REG	PVC_REG

#define LASAT_DISPLAY_BUSY	(1 << 31)
#define LASAT_DISPLAY_ENABLE	PVC_E
#define LASAT_DISPLAY_RW	PVC_RW
#define LASAT_DISPLAY_RS	PVC_RS
#define LASAT_DISPLAY_SHIFT	PVC_DATA_SHIFT
#define LASAT_DISPLAY_MASK	0xff070000


/*
 * Random register
 */
#define LASAT_RANDOM_REG	(KSEG1ADDR(0x11010000))
#define LASAT_RANDOM_BIT	19

/*
 * LED control register.
 */
#define LASAT_LED_REG		(KSEG1ADDR(0x11000000))

#define LASAT_LED0_ENB		( 1 << 19 )
#define LASAT_LED1_ENB		( 1 << 20 )
#define LASAT_LED2_ENB		( 1 << 21 )
#define LASAT_LED_SHIFT		19
#define LASAT_LED_MASK		0x7

#define LASAT_ACT_LED_ENB	0x1
#define LASAT_CON_LED_ENB	0x2
#define LASAT_LINK_LED_ENB	0x4

/*
 * LAN swapping register.
 */
#define LASAT_LANSWAP_REG	(KSEG1ADDR(0x11000000))

#define LASAT_LANSWAP_LAN1	( 1 << 8 )
#define LASAT_LANSWAP_LAN2	( 1 << 9 )

/*
 * External trigger register (for debugging).
 */
#define LASAT_EXTTRIG_REG	(KSEG1ADDR(0x11000000))

#define LASAT_EXTTRIG_MASK	( 1 << 19 )
#define LASAT_EXTTRIG_ENB	( 1 << 19 )
#define LASAT_EXTTRIG_DIS	( 0 )

/*
 * FPGA programming registers.
 */
#define LASAT_FPGA67_STATUS_REG	(KSEG1ADDR(0x11010000))
#define LASAT_FPGA67_OUTPUT_REG	(KSEG1ADDR(0x11070000))

#define LASAT_FPGA27_STATUS_REG	(KSEG1ADDR(0x11000000))
#define LASAT_FPGA27_OUTPUT_REG	(KSEG1ADDR(0x11050000))

/*
 * Serial EEPROM.
 */
#define AT93C_REG		KSEG1ADDR(0x11000000)
#define AT93C_RDATA_REG		(AT93C_REG+0x10000)
#define AT93C_RDATA_SHIFT	8
#define AT93C_RDATA_M		(1 << AT93C_RDATA_SHIFT)
#define AT93C_WDATA_SHIFT	2
#define AT93C_WDATA_M		(1 << AT93C_WDATA_SHIFT)
#define AT93C_CS_M		( 1 << 0 )
#define AT93C_CLK_M		( 1 << 1 )

#define LASAT_EEPROM_REG		(KSEG1ADDR(0x11000000))

#define LASAT_EEPROM_MASK		0x7
#define LASAT_EEPROM_CS			( 1 << 0 )
#define LASAT_EEPROM_CLK		( 1 << 1 )
#define LASAT_EEPROM_WDATA_SHIFT 	2
#define LASAT_EEPROM_WDATA		( 1 << 2 )	/* EEPROM data write */
/*
 * The read bit is in register 4,
 * so we add an offset to LASAT_EEPROM_REG
 */
#define LASAT_EEPROM_READ_OFFSET	0x10000
#define LASAT_EEPROM_RDATA_SHIFT	8
#define LASAT_EEPROM_RDATA		( 1 << 8 )	/* EEPROM data read */

/*
 * The board control register 0.
 */
#define LASAT_BOARD_CONTROL_REG	(KSEG1ADDR(0x11000000))

/*
 * The board control register 4.
 */
#define LASAT_BOARD_CONTROL_REG4 (KSEG1ADDR(0x11010000))

/*
 * Register for finding out if backplane is present.
 */
#define LASAT_BACKPRESENT_REG	(KSEG1ADDR(0x11010000))

/* Test for backplane */
#define LASAT_BACKPRESENT	( (*(volatile long *)(LASAT_BACKPRESENT_REG)) & (1 << 17) )

/*
 * Register for finding out if Flash Module is present.
 */
#define LASAT_FLASHPRESENT_REG	(KSEG1ADDR(0x11010000))

/* Test for flash module */
#define LASAT_FLASHPRESENT	( (*(volatile long *)(LASAT_FLASHPRESENT_REG)) & (0x01 << 16) )

/*
 * The Flash base address.
 */
#define LASAT_FLASH_BASE 	(KSEG1ADDR(0x10000000))
#define LASAT_FLASH_REG 	(KSEG1ADDR(0x11000000))

#define LASAT_BOOTCS_BASE	(KSEG1ADDR(0x1fc00000))

/*
 * Write protecting the flash, WHY IS THIS NOT IN THE SPEC?????
 * It says the bit is unused.
 */
#define LASAT_FLASHWP_REG	(KSEG1ADDR(0x11000000))
#define LASAT_FLASHWP_MASK	(1 << 6)
#define LASAT_FLASHWP_ENB	0
#define LASAT_FLASHWP_DIS	(1 << 6)

/*
 * Interrupt controller status and mask registers.
 */
#define LASAT_INT_STATUS_REG	(KSEG1ADDR(0x1104003c))
#define LASAT_INT_MASK_REG	(KSEG1ADDR(0x1104003c))

/*
 * Reset register
 */
#define LASAT_RESET_REG		(KSEG1ADDR(0x11080000))

/*
 * Noise register
 */
#define LASAT_NOISE_REG		(KSEG1ADDR(0x11010000))

#define LASAT_NOISE		( (*(volatile long *)(LASAT_NOISE_REG)) & (1 << 19) )

/*
 * LASAT UART register base.
 */
#define LASAT_BASE_BAUD		(100000000 / 16 / 12) 
#define LASAT_UART_REGS_BASE	(Vrc5074_PHYS_BASE + 0x0300)
#define LASAT_UART_REGS_SHIFT	3

#define UART_REGSZ 8

/*
 * Hi/fn controller.
 */
#define LASAT_HIFN_BASE		(KSEG1ADDR(0x11030000))
#define LASAT_HIFN_REG		LASAT_BOARD_CONTROL_REG

#define LASAT_HIFN_RST        	( 1 << 10)

/*
 * Crypto FPGA (Virtex)
 */
#define LASAT_FPGA_BASE		(KSEG1ADDR(0x11060000))

#define LASAT_CRYPT_REG		(KSEG1ADDR(0x11010000))
#define LASAT_CRYPT_RST        	( 1 << 2)

/*
 * Address filter FPGA (Spartan)
 */
#define LASAT_FPGA_SPA_BASE	(KSEG1ADDR(0x11040000))

/*
 * This is a macro to convert a BootCS address to the corresponding
 * address in the Flash chipselect space 
 */
#define LASAT_BOOTCS_TO_FLASHCS(x) \
  ((PHYSADDR(x) & 0x00ffffff) | (PHYSADDR(LASAT_FLASH_BASE)))


/* The interrupts are currently delivered to INT0 on the CPU */
#define LASATINT_MASK_SHIFT	16
#define LASATINT_END	16
#define LASATHWINT_END	16

#define LASATINT_ETH1	0
#define LASATINT_ETH0	1
#define LASATINT_HDC	2
#define LASATINT_COMP	3
#define LASATINT_HDLC	4
#define LASATINT_PCIA	5
#define LASATINT_PCIB	6
#define LASATINT_PCIC	7
#define LASATINT_PCID	8
#define LASATINT_POWER_OK 9
#define LASATINT_CRY	10
#define LASATINT_INT0	11	/* HW int 0 on CPU */
#define LASATINT_INT1	12	/* HW int 1 on CPU */
#define LASATINT_INT2	13	/* HW int 2 on CPU */
/* We need to define LASATINT_UART for serial.h */
#define LASATINT_UART	13	/* 0 means use timer instead */
#define LASATINT_DMA	14	/* Interrupt generated when HDLC DMA has transferred data from fifo to mem */

#endif /* _MIPS_LASAT200_H */
