/*
 * indy_sc.c: Indy cache managment functions.
 *
 * Copyright (C) 1997 Ralf Baechle (ralf@gnu.org),
 * derived from r4xx0.c by David S. Miller (dm@engr.sgi.com).
 *
 * $Id: indy_sc.c,v 1.4 1998/01/13 04:39:38 ralf Exp $
 */
#include <linux/config.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/autoconf.h>

#include <asm/bcache.h>
#include <asm/sgi.h>
#include <asm/sgimc.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/bootinfo.h>
#include <asm/sgialib.h>
#include <asm/mmu_context.h>

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")

/* Primary cache parameters. */
static int icache_size, dcache_size; /* Size in bytes */
static int ic_lsize, dc_lsize;       /* LineSize in bytes */

/* Secondary cache (if present) parameters. */
static scache_size, sc_lsize;        /* Again, in bytes */

#include <asm/cacheops.h>
#include <asm/r4kcache.h>

#undef DEBUG_CACHE


static void indy_sc_wback_invalidate(unsigned long page, unsigned long size)
{
	unsigned long tmp1, tmp2, flags;

	page &= PAGE_MASK;

#ifdef DEBUG_CACHE
	printk("indy_sc_flush_page_to_ram[%08lx]", page);
#endif
	if (size == 0)
		return;

	save_and_cli(flags);

	__asm__ __volatile__("
		.set noreorder
		.set mips3
		li	%0, 0x1
		dsll	%0, 31
		or	%0, %0, %2
		lui	%1, 0x9000
		dsll32	%1, 0
		or	%0, %0, %1
		daddu	%1, %0, %4
		li	%2, 0x80
		mtc0	%2, $12
		nop; nop; nop; nop;
1:		sw	$0, 0(%0)
		bltu	%0, %1, 1b
		daddu	%0, 32
		mtc0	$0, $12
		nop; nop; nop; nop;
		.set mips0
		.set reorder"
		: "=&r" (tmp1), "=&r" (tmp2),
		  "=&r" (page)
		: "2" (page & 0x0007f000),
		  "r" (size - 32));
	restore_flags(flags);
}

static void indy_sc_enable(void)
{
	unsigned long addr, tmp1, tmp2;

	/* This is really cool... */
	printk("Enabling R4600 SCACHE\n");
	__asm__ __volatile__("
		.set noreorder
		.set mips3
		mfc0	%2, $12
		nop; nop; nop; nop;
		li	%1, 0x80
		mtc0	%1, $12
		nop; nop; nop; nop;
		li	%0, 0x1
		dsll	%0, 31
		lui	%1, 0x9000
		dsll32	%1, 0
		or	%0, %1, %0
		sb	$0, 0(%0)
		mtc0	$0, $12
		nop; nop; nop; nop;
		mtc0	%2, $12
		nop; nop; nop; nop;
		.set mips0
		.set reorder"
		: "=r" (tmp1), "=r" (tmp2), "=r" (addr));
}

static void indy_sc_disable(void)
{
	unsigned long tmp1, tmp2, tmp3;

	if(mips_cputype != CPU_R4600 &&
	   mips_cputype != CPU_R4640 &&
	   mips_cputype != CPU_R4700)
		return;
	printk("Disabling R4600 SCACHE\n");
	__asm__ __volatile__("
		.set noreorder
		.set mips3
		li	%0, 0x1
		dsll	%0, 31
		lui	%1, 0x9000
		dsll32	%1, 0
		or	%0, %1, %0
		mfc0	%2, $12
		nop; nop; nop; nop;
		li	%1, 0x80
		mtc0	%1, $12
		nop; nop; nop; nop;
		sh	$0, 0(%0)
		mtc0	$0, $12
		nop; nop; nop; nop;
		mtc0	%2, $12
		nop; nop; nop; nop;
		.set mips2
		.set reorder
        " : "=r" (tmp1), "=r" (tmp2), "=r" (tmp3));
}

static inline int indy_sc_probe(void)
{
	volatile unsigned int *cpu_control;
	unsigned short cmd = 0xc220;
	unsigned long data = 0;
	unsigned long addr;
	int i, n;

#ifdef __MIPSEB__
	cpu_control = (volatile unsigned int *) KSEG1ADDR(0x1fa00034);
#else
	cpu_control = (volatile unsigned int *) KSEG1ADDR(0x1fa00030);
#endif
#define DEASSERT(bit) (*(cpu_control) &= (~(bit)))
#define ASSERT(bit) (*(cpu_control) |= (bit))
#define DELAY  for(n = 0; n < 100000; n++) __asm__ __volatile__("")
	DEASSERT(SGIMC_EEPROM_PRE);
	DEASSERT(SGIMC_EEPROM_SDATAO);
	DEASSERT(SGIMC_EEPROM_SECLOCK);
	DEASSERT(SGIMC_EEPROM_PRE);
	DELAY;
	ASSERT(SGIMC_EEPROM_CSEL); ASSERT(SGIMC_EEPROM_SECLOCK);
	for(i = 0; i < 11; i++) {
		if(cmd & (1<<15))
			ASSERT(SGIMC_EEPROM_SDATAO);
		else
			DEASSERT(SGIMC_EEPROM_SDATAO);
		DEASSERT(SGIMC_EEPROM_SECLOCK);
		ASSERT(SGIMC_EEPROM_SECLOCK);
		cmd <<= 1;
	}
	DEASSERT(SGIMC_EEPROM_SDATAO);
	for(i = 0; i < (sizeof(unsigned short) * 8); i++) {
		unsigned int tmp;

		DEASSERT(SGIMC_EEPROM_SECLOCK);
		DELAY;
		ASSERT(SGIMC_EEPROM_SECLOCK);
		DELAY;
		data <<= 1;
		tmp = *cpu_control;
		if(tmp & SGIMC_EEPROM_SDATAI)
			data |= 1;
	}
	DEASSERT(SGIMC_EEPROM_SECLOCK);
	DEASSERT(SGIMC_EEPROM_CSEL);
	ASSERT(SGIMC_EEPROM_PRE);
	ASSERT(SGIMC_EEPROM_SECLOCK);
	data <<= PAGE_SHIFT;
	printk("R4600/R5000 SCACHE size %dK ", (int) (data >> 10));
	switch(mips_cputype) {
	case CPU_R4600:
	case CPU_R4640:
		sc_lsize = 32;
		break;

	default:
		sc_lsize = 128;
		break;
	}
	printk("linesize %d bytes\n", sc_lsize);
	scache_size = data;
	if (data == 0) {
		if (mips_cputype == CPU_R5000)
			return -1;
		else
			return 0;
	}

	/* Enable r4600/r5000 cache.  But flush it first. */
	for(addr = KSEG0; addr < (KSEG0 + dcache_size);
	    addr += dc_lsize)
		flush_dcache_line_indexed(addr);
	for(addr = KSEG0; addr < (KSEG0 + icache_size);
	    addr += ic_lsize)
		flush_icache_line_indexed(addr);
	for(addr = KSEG0; addr < (KSEG0 + scache_size);
	    addr += sc_lsize)
		flush_scache_line_indexed(addr);

	if (mips_cputype == CPU_R4600 ||
	    mips_cputype == CPU_R5000)
		return 1;

	return 0;
}

/* XXX Check with wje if the Indy caches can differenciate between
   writeback + invalidate and just invalidate.  */
struct bcache_ops indy_sc_ops = {
	indy_sc_enable,
	indy_sc_disable,
	indy_sc_wback_invalidate,
	indy_sc_wback_invalidate
};

void indy_sc_init(void)
{
	if (indy_sc_probe()) {
		indy_sc_enable();
		bcops = &indy_sc_ops;
	}
}
