/*
 * Definitions for the SGI O2 Mace chip.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000 Harald Koerfgen
 */

#ifndef __ASM_MACE_H__
#define __ASM_MACE_H__

#include <asm/addrspace.h>
#include <asm/io.h>

/*
 * Address map
 */
#define MACE_BASE	0x1f000000	/* physical */
#define MACE_PCI	0x00080000
#define MACE_VIN1	0x00100000
#define MACE_VIN2	0x00180000
#define MACE_VOUT	0x00200000
#define MACE_ENET	0x00280000
#define MACE_PERIF	0x00300000
#define MACE_ISA_EXT	0x00380000
                                                                                                  
#define MACE_AUDIO	(MACE_PERIF + 0x00000)
#define MACE_ISA	(MACE_PERIF + 0x10000)
#define MACE_KBDMS	(MACE_PERIF + 0x20000)
#define MACE_I2C	(MACE_PERIF + 0x30000)
#define MACE_UST_MSC	(MACE_PERIF + 0X40000)


#undef BIT
#define BIT(__bit_offset) (1UL << (__bit_offset))

/*
 * MACE PCI interface
 */
#define MACEPCI_ERROR_ADDR		(MACE_PCI	      )
#define MACEPCI_ERROR_FLAGS		(MACE_PCI + 0x00000004)
#define MACEPCI_CONTROL			(MACE_PCI + 0x00000008)
#define MACEPCI_REV			(MACE_PCI + 0x0000000c)
#define MACEPCI_WFLUSH			MACEPCI_REV
#define MACEPCI_CONFIG_ADDR		(MACE_PCI + 0x00000cf8)
#define MACEPCI_CONFIG_DATA		(MACE_PCI + 0x00000cfc)

#define MACEPCI_LOW_MEMORY		0x1a000000
#define MACEPCI_LOW_IO			0x18000000
#define MACEPCI_SWAPPED_VIEW		0
#define MACEPCI_NATIVE_VIEW		0x40000000
#define MACEPCI_IO			0x80000000
#define MACEPCI_HI_MEMORY		0x280000000
#define MACEPCI_HI_IO			0x100000000

/*
 * Bits in the MACEPCI_CONTROL register
 */
#define MACEPCI_CONTROL_INT(x)		BIT(x)
#define MACEPCI_CONTROL_INT_MASK	0xff
#define MACEPCI_CONTROL_SERR_ENA	BIT(8)
#define MACEPCI_CONTROL_ARB_N6		BIT(9)
#define MACEPCI_CONTROL_PARITY_ERR	BIT(10)
#define MACEPCI_CONTROL_MRMRA_ENA	BIT(11)
#define MACEPCI_CONTROL_ARB_N3		BIT(12)
#define MACEPCI_CONTROL_ARB_N4		BIT(13)
#define MACEPCI_CONTROL_ARB_N5		BIT(14)
#define MACEPCI_CONTROL_PARK_LIU	BIT(15)
#define MACEPCI_CONTROL_INV_INT(x)	BIT(16+x)
#define MACEPCI_CONTROL_INV_INT_MASK	0x00ff0000
#define MACEPCI_CONTROL_OVERRUN_INT	BIT(24)
#define MACEPCI_CONTROL_PARITY_INT	BIT(25)
#define MACEPCI_CONTROL_SERR_INT	BIT(26)
#define MACEPCI_CONTROL_IT_INT		BIT(27)
#define MACEPCI_CONTROL_RE_INT		BIT(28)
#define MACEPCI_CONTROL_DPED_INT	BIT(29)
#define MACEPCI_CONTROL_TAR_INT		BIT(30)
#define MACEPCI_CONTROL_MAR_INT		BIT(31)

/*
 * Bits in the MACE_PCI error register
 */
#define MACEPCI_ERROR_MASTER_ABORT		BIT(31)
#define MACEPCI_ERROR_TARGET_ABORT		BIT(30)
#define MACEPCI_ERROR_DATA_PARITY_ERR		BIT(29)
#define MACEPCI_ERROR_RETRY_ERR			BIT(28)
#define MACEPCI_ERROR_ILLEGAL_CMD		BIT(27)
#define MACEPCI_ERROR_SYSTEM_ERR		BIT(26)
#define MACEPCI_ERROR_INTERRUPT_TEST		BIT(25)
#define MACEPCI_ERROR_PARITY_ERR		BIT(24)
#define MACEPCI_ERROR_OVERRUN			BIT(23)
#define MACEPCI_ERROR_RSVD			BIT(22)
#define MACEPCI_ERROR_MEMORY_ADDR		BIT(21)
#define MACEPCI_ERROR_CONFIG_ADDR		BIT(20)
#define MACEPCI_ERROR_MASTER_ABORT_ADDR_VALID	BIT(19)
#define MACEPCI_ERROR_TARGET_ABORT_ADDR_VALID	BIT(18)
#define MACEPCI_ERROR_DATA_PARITY_ADDR_VALID	BIT(17)
#define MACEPCI_ERROR_RETRY_ADDR_VALID		BIT(16)
#define MACEPCI_ERROR_SIG_TABORT		BIT(4)
#define MACEPCI_ERROR_DEVSEL_MASK		0xc0
#define MACEPCI_ERROR_DEVSEL_FAST		0
#define MACEPCI_ERROR_DEVSEL_MED		0x40
#define MACEPCI_ERROR_DEVSEL_SLOW		0x80
#define MACEPCI_ERROR_FBB			BIT(1)
#define MACEPCI_ERROR_66MHZ			BIT(0)

/*
 * Mace timer registers - 64 bit regs (63:32 are UST, 31:0 are MSC)
 */
#define MSC_PART(__reg) ((__reg) & 0x00000000ffffffff)
#define UST_PART(__reg) (((__reg) & 0xffffffff00000000) >> 32)

#define MACE_UST_UST		(MACE_UST_MSC       )	/* Universial system time */
#define MACE_UST_COMPARE1	(MACE_UST_MSC + 0x08)	/* Interrupt compare reg 1 */
#define MACE_UST_COMPARE2	(MACE_UST_MSC + 0x10)	/* Interrupt compare reg 2 */
#define MACE_UST_COMPARE3	(MACE_UST_MSC + 0x18)	/* Interrupt compare reg 3 */
#define MACE_UST_PERIOD_NS	960			/* UST Period in ns  */

#define MACE_UST_AIN_MSC	(MACE_UST_MSC + 0x20)	/* Audio in MSC/UST pair */
#define MACE_UST_AOUT1_MSC	(MACE_UST_MSC + 0x28)	/* Audio out 1 MSC/UST pair */
#define MACE_UST_AOUT2_MSC	(MACE_UST_MSC + 0x30)	/* Audio out 2 MSC/UST pair */
#define MACE_VIN1_MSC_UST	(MACE_UST_MSC + 0x38)	/* Video In 1 MSC/UST pair */
#define MACE_VIN2_MSC_UST	(MACE_UST_MSC + 0x40)	/* Video In 2 MSC/UST pair */
#define MACE_VOUT_MSC_UST	(MACE_UST_MSC + 0x48)	/* Video out MSC/UST pair */

/*
 * Mace "ISA" peripherals
 */
#define MACEISA_EPP	(MACE_ISA_EXT          )
#define MACEISA_ECP	(MACE_ISA_EXT + 0x08000)
#define MACEISA_SER1	(MACE_ISA_EXT + 0x10000)
#define MACEISA_SER2	(MACE_ISA_EXT + 0x18000)
#define MACEISA_RTC	(MACE_ISA_EXT + 0x20000)
#define MACEISA_GAME	(MACE_ISA_EXT + 0x30000)

/*
 * Ringbase address and reset register - 64 bits
 */
#define MACEISA_RINGBASE	MACE_ISA
/* Ring buffers occupy 8 4K buffers */
#define MACEISA_RINGBUFFERS_SIZE 8*4*1024

/*
 * Flash-ROM/LED/DP-RAM/NIC Controller Register - 8 bit
 */
#define MACEISA_FLASH_NIC_REG	(MACE_ISA + 0x008)

/*
 * Bit definitions for that
 */
#define MACEISA_FLASH_WE       BIT(0) /* 1=> Enable FLASH writes */
#define MACEISA_PWD_CLEAR      BIT(1) /* 1=> PWD CLEAR jumper detected */
#define MACEISA_NIC_DEASSERT   BIT(2)
#define MACEISA_NIC_DATA       BIT(3)
#define MACEISA_LED_RED        BIT(4) /* 0=> Illuminate RED LED */
#define MACEISA_LED_GREEN      BIT(5) /* 0=> Illuminate GREEN LED */
#define MACEISA_DP_RAM_ENABLE  BIT(6)

/*
 * ISA interrupt and status registers - 32 bit
 */
#define MACEISA_INT_STAT	(MACE_ISA + 0x014)
#define MACEISA_INT_MASK	(MACE_ISA + 0x01c)

/*
 * Bits in the status/mask registers
 */
#define MACEISA_AUDIO_SW_INT		BIT (0)
#define MACEISA_AUDIO_SC_INT		BIT (1)
#define MACEISA_AUDIO1_DMAT_INT		BIT (2)
#define MACEISA_AUDIO1_OF_INT		BIT (3)
#define MACEISA_AUDIO2_DMAT_INT		BIT (4)
#define MACEISA_AUDIO2_MERR_INT		BIT (5)
#define MACEISA_AUDIO3_DMAT_INT		BIT (6)
#define MACEISA_AUDIO3_MERR_INT		BIT (7)
#define MACEISA_RTC_INT			BIT (8)
#define MACEISA_KEYB_INT		BIT (9)
#define MACEISA_KEYB_POLL_INT		BIT (10)
#define MACEISA_MOUSE_INT		BIT (11)
#define MACEISA_MOUSE_POLL_INT		BIT (12)
#define MACEISA_TIMER0_INT		BIT (13)
#define MACEISA_TIMER1_INT		BIT (14)
#define MACEISA_TIMER2_INT		BIT (15)
#define MACEISA_PARALLEL_INT		BIT (16)
#define MACEISA_PAR_CTXA_INT		BIT (17)
#define MACEISA_PAR_CTXB_INT		BIT (18)
#define MACEISA_PAR_MERR_INT		BIT (19)
#define MACEISA_SERIAL1_INT		BIT (20)
#define MACEISA_SERIAL1_TDMAT_INT	BIT (21)
#define MACEISA_SERIAL1_TDMAPR_INT	BIT (22)
#define MACEISA_SERIAL1_TDMAME_INT	BIT (23)
#define MACEISA_SERIAL1_RDMAT_INT	BIT (24)
#define MACEISA_SERIAL1_RDMAOR_INT	BIT (25)
#define MACEISA_SERIAL2_INT		BIT (26)
#define MACEISA_SERIAL2_TDMAT_INT	BIT (27)
#define MACEISA_SERIAL2_TDMAPR_INT	BIT (28)
#define MACEISA_SERIAL2_TDMAME_INT	BIT (29)
#define MACEISA_SERIAL2_RDMAT_INT	BIT (30)
#define MACEISA_SERIAL2_RDMAOR_INT	BIT (31)

#define MACEI2C_CONFIG		(MACE_I2C       )
#define MACEI2C_CONTROL		(MACE_I2C + 0x10)
#define MACEI2C_DATA		(MACE_I2C + 0x18)

/* Bits for I2C_CONFIG */
#define MACEI2C_RESET           BIT(0)
#define MACEI2C_FAST            BIT(1)
#define MACEI2C_DATA_OVERRIDE   BIT(2)
#define MACEI2C_CLOCK_OVERRIDE  BIT(3)
#define MACEI2C_DATA_STATUS     BIT(4)
#define MACEI2C_CLOCK_STATUS    BIT(5)

#define MACEISA_AUDIO_INT	(MACEISA_AUDIO_SW_INT |		\
				 MACEISA_AUDIO_SC_INT |		\
				 MACEISA_AUDIO1_DMAT_INT |	\
				 MACEISA_AUDIO1_OF_INT |	\
				 MACEISA_AUDIO2_DMAT_INT |	\
				 MACEISA_AUDIO2_MERR_INT |	\
				 MACEISA_AUDIO3_DMAT_INT |	\
				 MACEISA_AUDIO3_MERR_INT)
#define MACEISA_MISC_INT	(MACEISA_RTC_INT |		\
				 MACEISA_KEYB_INT |		\
				 MACEISA_KEYB_POLL_INT |	\
				 MACEISA_MOUSE_INT |		\
				 MACEISA_MOUSE_POLL_INT |	\
				 MACEISA_TIMER0_INT |		\
				 MACEISA_TIMER1_INT |		\
				 MACEISA_TIMER2_INT)
#define MACEISA_SUPERIO_INT	(MACEISA_PARALLEL_INT |		\
				 MACEISA_PAR_CTXA_INT |		\
				 MACEISA_PAR_CTXB_INT |		\
				 MACEISA_PAR_MERR_INT |		\
				 MACEISA_SERIAL1_INT |		\
				 MACEISA_SERIAL1_TDMAT_INT |	\
				 MACEISA_SERIAL1_TDMAPR_INT |	\
				 MACEISA_SERIAL1_TDMAME_INT |	\
				 MACEISA_SERIAL1_RDMAT_INT |	\
				 MACEISA_SERIAL1_RDMAOR_INT |	\
				 MACEISA_SERIAL2_INT |		\
				 MACEISA_SERIAL2_TDMAT_INT |	\
				 MACEISA_SERIAL2_TDMAPR_INT |	\
				 MACEISA_SERIAL2_TDMAME_INT |	\
				 MACEISA_SERIAL2_RDMAT_INT |	\
				 MACEISA_SERIAL2_RDMAOR_INT)

extern void *sgi_mace;

static inline uint8_t mace_read_8(unsigned long offset)
{
	return readb(sgi_mace + offset);
}

static inline uint16_t mace_read_16(unsigned long offset)
{
	return readw(sgi_mace + offset);
}

static inline uint32_t mace_read_32(unsigned long offset)
{
	return readl(sgi_mace + offset);
}

static inline uint64_t mace_read_64(unsigned long offset)
{
	return readq(sgi_mace + offset);
}

static inline void mace_write_8(uint8_t val, unsigned long offset)
{
	writeb(val, sgi_mace + offset);
}

static inline void mace_write_16(uint16_t val, unsigned long offset)
{
	writew(val, sgi_mace + offset);
}

static inline void mace_write_32(uint32_t val, unsigned long offset)
{
	writel(val, sgi_mace + offset);
}

static inline void mace_write_64(uint64_t val, unsigned long offset)
{
	writeq(val, sgi_mace + offset);
}

/* Call it whenever device needs to read data from main memory coherently */
static inline void mace_inv_read_buffers(void)
{
/*	mace_write_32(0xffffffff, MACEPCI_WFLUSH);*/
}

#endif /* __ASM_MACE_H__ */
