/*
 * include/asm-mips/mipsregs.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle
 * Modified for further R[236]000 support by Paul M. Antoine, 1996.
 */
#ifndef __ASM_MIPS_MIPSREGS_H
#define __ASM_MIPS_MIPSREGS_H

#include <linux/linkage.h>

/*
 * The following macros are especially useful for __asm__
 * inline assembler.
 */
#ifndef __STR
#define __STR(x) #x
#endif
#ifndef STR
#define STR(x) __STR(x)
#endif

/*
 * Assigned values for the product ID register.  In order to detect a
 * certain CPU type exactly eventually additional registers may need to
 * be examined.
 */
#define PRID_R3000A 0x02
#define PRID_R4000 0x04
#define PRID_R4400 0x04
#define PRID_R4300 0x0b
#define PRID_R4600 0x20
#define PRID_R4700 0x21
#define PRID_R4640 0x22
#define PRID_R4650 0x22
#define PRID_R5000 0x23
#define PRID_SONIC 0x24
#define PRID_R10000 0x09

/*
 * Coprocessor 0 register names
 */
#define CP0_INDEX $0
#define CP0_RANDOM $1
#define CP0_ENTRYLO0 $2
#define CP0_ENTRYLO1 $3
#define CP0_CONTEXT $4
#define CP0_PAGEMASK $5
#define CP0_WIRED $6
#define CP0_BADVADDR $8
#define CP0_COUNT $9
#define CP0_ENTRYHI $10
#define CP0_COMPARE $11
#define CP0_STATUS $12
#define CP0_CAUSE $13
#define CP0_EPC $14
#define CP0_PRID $15
#define CP0_CONFIG $16
#define CP0_LLADDR $17
#define CP0_WATCHLO $18
#define CP0_WATCHHI $19
#define CP0_XCONTEXT $20
#define CP0_FRAMEMASK $21
#define CP0_DIAGNOSTIC $22
#define CP0_PERFORMANCE $25
#define CP0_ECC $26
#define CP0_CACHEERR $27
#define CP0_TAGLO $28
#define CP0_TAGHI $29
#define CP0_ERROREPC $30

/*
 * R4640/R4650 cp0 register names.  These registers are listed
 * here only for completeness; without MMU these CPUs are not useable
 * by Linux.  A future ELKS port might take make Linux run on them
 * though ...
 */
#define CP0_IBASE $0
#define CP0_IBOUND $1
#define CP0_DBASE $2
#define CP0_DBOUND $3
#define CP0_CALG $17
#define CP0_IWATCH $18
#define CP0_DWATCH $19

/*
 * Coprocessor 1 (FPU) register names
 */
#define CP1_REVISION   $0
#define CP1_STATUS     $31

/*
 * Values for PageMask register
 */
#define PM_4K   0x00000000
#define PM_16K  0x00006000
#define PM_64K  0x0001e000
#define PM_256K 0x0007e000
#define PM_1M   0x001fe000
#define PM_4M   0x007fe000
#define PM_16M  0x01ffe000

/*
 * Values used for computation of new tlb entries
 */
#define PL_4K   12
#define PL_16K  14
#define PL_64K  16
#define PL_256K 18
#define PL_1M   20
#define PL_4M   22
#define PL_16M  24

/*
 * Macros to access the system control coprocessor
 */
#define read_32bit_cp0_register(source)                         \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
        "mfc0\t%0,"STR(source)                                  \
        : "=r" (__res));                                        \
        __res;})

#define read_64bit_cp0_register(source)                         \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
        ".set\tmips3\n\t"                                       \
        "dmfc0\t%0,"STR(source)"\n\t"                           \
        ".set\tmips0"                                           \
        : "=r" (__res));                                        \
        __res;})

#define write_32bit_cp0_register(register,value)                \
        __asm__ __volatile__(                                   \
        "mtc0\t%0,"STR(register)                                \
        : : "r" (value));

#define write_64bit_cp0_register(register,value)                \
        __asm__ __volatile__(                                   \
        ".set\tmips3\n\t"                                       \
        "dmtc0\t%0,"STR(register)"\n\t"                         \
        ".set\tmips0"                                           \
        : : "r" (value))
/*
 * R4x00 interrupt enable / cause bits
 */
#define IE_SW0          (1<< 8)
#define IE_SW1          (1<< 9)
#define IE_IRQ0         (1<<10)
#define IE_IRQ1         (1<<11)
#define IE_IRQ2         (1<<12)
#define IE_IRQ3         (1<<13)
#define IE_IRQ4         (1<<14)
#define IE_IRQ5         (1<<15)

/*
 * R4x00 interrupt cause bits
 */
#define C_SW0           (1<< 8)
#define C_SW1           (1<< 9)
#define C_IRQ0          (1<<10)
#define C_IRQ1          (1<<11)
#define C_IRQ2          (1<<12)
#define C_IRQ3          (1<<13)
#define C_IRQ4          (1<<14)
#define C_IRQ5          (1<<15)

#ifndef __LANGUAGE_ASSEMBLY__
/*
 * Manipulate the status register.
 * Mostly used to access the interrupt bits.
 */
#define BUILD_SET_CP0(name,register)                            \
extern __inline__ unsigned int                                  \
set_cp0_##name(unsigned int change, unsigned int new)           \
{                                                               \
	unsigned int res;                                       \
                                                                \
	res = read_32bit_cp0_register(register);                \
	res &= ~change;                                         \
	res |= (new & change);                                  \
	if(change)                                              \
		write_32bit_cp0_register(register, res);        \
                                                                \
	return res;                                             \
}

BUILD_SET_CP0(status,CP0_STATUS)
BUILD_SET_CP0(cause,CP0_CAUSE)

#endif /* defined (__LANGUAGE_ASSEMBLY__) */

/*
 * Inline code for use of the ll and sc instructions
 *
 * FIXME: This instruction is only available on MIPS ISA >=2.
 * Since these operations are only being used for atomic operations
 * the easiest workaround for the R[23]00 is to disable interrupts.
 * This fails for R3000 SMP machines which use that many different
 * technologies as replacement that it is difficult to create even
 * just a hook for for all machines to hook into.  The only good
 * thing is that there is currently no R3000 SMP machine on the
 * Linux/MIPS target list ...
 */
#define load_linked(addr)                                       \
({                                                              \
	unsigned int __res;                                     \
                                                                \
	__asm__ __volatile__(                                   \
	"ll\t%0,(%1)"                                           \
	: "=r" (__res)                                          \
	: "r" ((unsigned long) (addr)));                        \
                                                                \
	__res;                                                  \
})

#define store_conditional(addr,value)                           \
({                                                              \
	int	__res;                                          \
                                                                \
	__asm__ __volatile__(                                   \
	"sc\t%0,(%2)"                                           \
	: "=r" (__res)                                          \
	: "0" (value), "r" (addr));                             \
                                                                \
	__res;                                                  \
})

/*
 * Bitfields in the R4xx0 cp0 status register
 */
#define ST0_IE			(1   <<  0)
#define ST0_EXL			(1   <<  1)
#define ST0_ERL			(1   <<  2)
#define ST0_KSU			(3   <<  3)
#  define KSU_USER		(2  <<   3)
#  define KSU_SUPERVISOR	(1  <<   3)
#  define KSU_KERNEL		(0  <<   3)
#define ST0_UX			(1   <<  5)
#define ST0_SX			(1   <<  6)
#define ST0_KX 			(1   <<  7)

/*
 * Bitfields in the R[23]000 cp0 status register.
 */
#define ST0_IEC			(1   <<  0)
#define ST0_KUC			(1   <<  1)
#define ST0_IEP			(1   <<  2)
#define ST0_KUP			(1   <<  3)
#define ST0_IEO			(1   <<  4)
#define ST0_KUO			(1   <<  5)
/* bits 6 & 7 are reserved on R[23]000 */

/*
 * Bits specific to the R4640/R4650
 */
#define ST0_UM			<1   <<  4)
#define ST0_IL			(1   << 23)
#define ST0_DL			(1   << 24)

/*
 * Status register bits available in all MIPS CPUs.
 */
#define ST0_IM			(255 <<  8)
#define ST0_DE			(1   << 16)
#define ST0_CE			(1   << 17)
#define ST0_CH			(1   << 18)
#define ST0_SR			(1   << 20)
#define ST0_BEV			(1   << 22)
#define ST0_RE			(1   << 25)
#define ST0_FR			(1   << 26)
#define ST0_CU			(15  << 28)
#define ST0_CU0			(1   << 28)
#define ST0_CU1			(1   << 29)
#define ST0_CU2			(1   << 30)
#define ST0_CU3			(1   << 31)
#define ST0_XX			(1   << 31)	/* MIPS IV naming */

/*
 * Bitfields and bit numbers in the coprocessor 0 cause register.
 *
 * Refer to to your MIPS R4xx0 manual, chapter 5 for explanation.
 */
#define  CAUSEB_EXCCODE		2
#define  CAUSEF_EXCCODE		(31  <<  2)
#define  CAUSEB_IP		8
#define  CAUSEF_IP		(255 <<  8)
#define  CAUSEB_IP0		8
#define  CAUSEF_IP0		(1   <<  8)
#define  CAUSEB_IP1		9
#define  CAUSEF_IP1		(1   <<  9)
#define  CAUSEB_IP2		10
#define  CAUSEF_IP2		(1   << 10)
#define  CAUSEB_IP3		11
#define  CAUSEF_IP3		(1   << 11)
#define  CAUSEB_IP4		12
#define  CAUSEF_IP4		(1   << 12)
#define  CAUSEB_IP5		13
#define  CAUSEF_IP5		(1   << 13)
#define  CAUSEB_IP6		14
#define  CAUSEF_IP6		(1   << 14)
#define  CAUSEB_IP7		15
#define  CAUSEF_IP7		(1   << 15)
#define  CAUSEB_CE		28
#define  CAUSEF_CE		(3   << 28)
#define  CAUSEB_BD		31
#define  CAUSEF_BD		(1   << 31)

/*
 * Bits in the coprozessor 0 config register.
 */
#define CONFIG_DB		(1<<4)
#define CONFIG_IB		(1<<5)
#define CONFIG_SC		(1<<17)

/*
 * R10000 performance counter definitions.
 *
 * FIXME: The R10000 performance counter opens a nice way to implement CPU
 *        time accounting with a precission of one cycle.  I don't have
 *        R10000 silicon but just a manual, so ...
 */

/*
 * Events counted by counter #0
 */
#define CE0_CYCLES			0
#define CE0_INSN_ISSUED			1
#define CE0_LPSC_ISSUED			2
#define CE0_S_ISSUED			3
#define CE0_SC_ISSUED			4
#define CE0_SC_FAILED			5
#define CE0_BRANCH_DECODED		6
#define CE0_QW_WB_SECONDARY		7
#define CE0_CORRECTED_ECC_ERRORS	8
#define CE0_ICACHE_MISSES		9
#define CE0_SCACHE_I_MISSES		10
#define CE0_SCACHE_I_WAY_MISSPREDICTED	11
#define CE0_EXT_INTERVENTIONS_REQ	12
#define CE0_EXT_INVALIDATE_REQ		13
#define CE0_VIRTUAL_COHERENCY_COND	14
#define CE0_INSN_GRADUATED		15

/*
 * Events counted by counter #1
 */
#define CE1_CYCLES			0
#define CE1_INSN_GRADUATED		1
#define CE1_LPSC_GRADUATED		2
#define CE1_S_GRADUATED			3
#define CE1_SC_GRADUATED		4
#define CE1_FP_INSN_GRADUATED		5
#define CE1_QW_WB_PRIMARY		6
#define CE1_TLB_REFILL			7
#define CE1_BRANCH_MISSPREDICTED	8
#define CE1_DCACHE_MISS			9
#define CE1_SCACHE_D_MISSES		10
#define CE1_SCACHE_D_WAY_MISSPREDICTED	11
#define CE1_EXT_INTERVENTION_HITS	12
#define CE1_EXT_INVALIDATE_REQ		13
#define CE1_SP_HINT_TO_CEXCL_SC_BLOCKS	14
#define CE1_SP_HINT_TO_SHARED_SC_BLOCKS	15

/*
 * These flags define in which priviledge mode the counters count events
 */
#define CEB_USER	8	/* Count events in user mode, EXL = ERL = 0 */
#define CEB_SUPERVISOR	4	/* Count events in supvervisor mode EXL = ERL = 0 */
#define CEB_KERNEL	2	/* Count events in kernel mode EXL = ERL = 0 */
#define CEB_EXL		1	/* Count events with EXL = 1, ERL = 0 */

#ifndef __LANGUAGE_ASSEMBLY__
/*
 * Functions to access the performance counter and control registers
 */
extern asmlinkage unsigned int read_perf_cntr(unsigned int counter);
extern asmlinkage void write_perf_cntr(unsigned int counter, unsigned int val);
extern asmlinkage unsigned int read_perf_cntl(unsigned int counter);
extern asmlinkage void write_perf_cntl(unsigned int counter, unsigned int val);
#endif

#endif /* __ASM_MIPS_MIPSREGS_H */
