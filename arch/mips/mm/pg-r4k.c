/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 98, 99, 2000, 01, 02, 03 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 Kanoj Sarcar (kanoj@sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/cacheflush.h>
#include <asm/cpu-features.h>
#include <asm/inst.h>
#include <asm/mipsregs.h>
#include <asm/page.h>
#include <asm/prefetch.h>
#include <asm/war.h>

/*
 * Maximum sizes:
 *
 * R4000 16 bytes D-cache, 128 bytes S-cache:		0x78 bytes
 * R4600 v1.7:						0x5c bytes
 * R4600 v2.0:						0x60 bytes
 * With prefetching, 16 byte strides			0xa0 bytes
 */

static unsigned int clear_page_array[0xa0 / 4];

void clear_page(void * page) __attribute__((alias("clear_page_array")));

EXPORT_SYMBOL(clear_page);


/*
 * This is suboptimal for 32-bit kernels; we assume that R10000 is only used
 * with 64-bit kernels.  The prefetch offsets have been experimentally tuned
 * an Origin 200.
 */
static int pref_offset __initdata = 512;
static unsigned int pref_mode __initdata;

static int has_scache __initdata = 0;
static int store_offset __initdata = 0;
static unsigned int __initdata *dest, *epc;

static inline void build_pref(void)
{
	if (!(store_offset & (cpu_dcache_line_size() - 1))) {
		union mips_instruction mi;

		mi.i_format.opcode     = pref_op;
		mi.i_format.rs         = 4;		/* $a0 */
		mi.i_format.rt         = pref_mode;
		mi.i_format.simmediate = store_offset;

		*epc++ = mi.word;
	}
}

static inline void build_cdex(void)
{
	union mips_instruction mi;

	if (has_scache && !(store_offset & (cpu_scache_line_size() - 1))) {

		mi.c_format.opcode     = cache_op;
		mi.c_format.rs         = 4;	/* $a0 */
		mi.c_format.c_op       = 3;	/* Create Dirty Exclusive */
		mi.c_format.cache      = 3;	/* Secondary Data Cache */
		mi.c_format.simmediate = store_offset;

		*epc++ = mi.word;
	}

	if (store_offset & (cpu_dcache_line_size() - 1))
		return;

	if (R4600_V1_HIT_CACHEOP_WAR && ((read_c0_prid() & 0xfff0) == 0x2010)) {
		*epc++ = 0;			/* nop */
		*epc++ = 0;			/* nop */
		*epc++ = 0;			/* nop */
		*epc++ = 0;			/* nop */
	}

	mi.c_format.opcode     = cache_op;
	mi.c_format.rs         = 4;		/* $a0 */
	mi.c_format.c_op       = 3;		/* Create Dirty Exclusive */
	mi.c_format.cache      = 1;		/* Data Cache */
	mi.c_format.simmediate = store_offset;

	*epc++ = mi.word;
}

static inline void __build_store_zero_reg(void)
{
	union mips_instruction mi;

	if (cpu_has_64bits)
		mi.i_format.opcode     = sd_op;
	else
		mi.i_format.opcode     = sw_op;
	mi.i_format.rs         = 4;		/* $a0 */
	mi.i_format.rt         = 0;		/* $zero */
	mi.i_format.simmediate = store_offset;

	store_offset += (cpu_has_64bits ? 8 : 4);

	*epc++ = mi.word;
}

static inline void build_store_zero_reg(void)
{
	if (cpu_has_prefetch)
		build_pref();
	else if (cpu_has_cache_cdex)
		build_cdex();

	__build_store_zero_reg();
}

static inline void build_addiu_at_a0(unsigned long offset)
{
	union mips_instruction mi;

	BUG_ON(offset > 0x7fff);

	mi.i_format.opcode     = cpu_has_64bit_addresses ? daddiu_op : addiu_op;
	mi.i_format.rs         = 4;		/* $a0 */
	mi.i_format.rt         = 1;		/* $at */
	mi.i_format.simmediate = offset;

	*epc++ = mi.word;
}

static inline void build_addiu_a0(unsigned long offset)
{
	union mips_instruction mi;

	BUG_ON(offset > 0x7fff);

	mi.i_format.opcode     = cpu_has_64bit_addresses ? daddiu_op : addiu_op;
	mi.i_format.rs         = 4;		/* $a0 */
	mi.i_format.rt         = 4;		/* $at */
	mi.i_format.simmediate = offset;

	store_offset -= offset;

	*epc++ = mi.word;
}

static inline void build_bne(unsigned int *dest)
{
	union mips_instruction mi;

	mi.i_format.opcode = bne_op;
	mi.i_format.rs     = 1;			/* $at */
	mi.i_format.rt     = 4;			/* $a0 */
	mi.i_format.simmediate = dest - epc - 1;

	*epc++ = mi.word;
}

static inline void build_nop(void)
{
	*epc++ = 0;
}

static inline void build_jr_ra(void)
{
	union mips_instruction mi;

	mi.r_format.opcode = spec_op;
	mi.r_format.rs     = 31;
	mi.r_format.rt     = 0;
	mi.r_format.rd     = 0;
	mi.r_format.re     = 0;
	mi.r_format.func   = jr_op;

	*epc++ = mi.word;
}

static void dump(void);
void __init build_clear_page(void)
{
	epc = (unsigned int *) &clear_page_array;

	if (cpu_has_prefetch) {
		switch (current_cpu_data.cputype) {
		case CPU_R10000:
		case CPU_R12000:
			pref_mode = Pref_StoreRetained;
			break;
		default:
			pref_mode = Pref_PrepareForStore;
			break;
		}
	}

	build_addiu_at_a0(PAGE_SIZE - (cpu_has_prefetch ? pref_offset : 0));

	if (R4600_V2_HIT_CACHEOP_WAR && ((read_c0_prid() & 0xfff0) == 0x2020)) {
		*epc++ = 0x40026000;		/* mfc0    $v0, $12	*/
		*epc++ = 0x34410001;		/* ori     $at, v0, 0x1	*/
		*epc++ = 0x38210001;		/* xori    $at, at, 0x1	*/
		*epc++ = 0x40816000;		/* mtc0    $at, $12	*/
		*epc++ = 0x00000000;		/* nop			*/
		*epc++ = 0x00000000;		/* nop			*/
		*epc++ = 0x00000000;		/* nop			*/
		*epc++ = 0x3c01a000;		/* lui     $at, 0xa000  */
		*epc++ = 0x8c200000;		/* lw      $zero, ($at) */
	}

dest = epc;
	build_store_zero_reg();
	build_store_zero_reg();
	build_store_zero_reg();
	build_store_zero_reg();
	if (has_scache && cpu_scache_line_size() == 128) {
		build_store_zero_reg();
		build_store_zero_reg();
		build_store_zero_reg();
		build_store_zero_reg();
	}
	build_addiu_a0(2 * store_offset);
	build_store_zero_reg();
	build_store_zero_reg();
	if (has_scache && cpu_scache_line_size() == 128) {
		build_store_zero_reg();
		build_store_zero_reg();
		build_store_zero_reg();
		build_store_zero_reg();
	}
	build_store_zero_reg();
	build_bne(dest);
	 build_store_zero_reg();

	if (cpu_has_prefetch && pref_offset) {
		build_addiu_at_a0(pref_offset);
	dest = epc;
		__build_store_zero_reg();
		__build_store_zero_reg();
		__build_store_zero_reg();
		__build_store_zero_reg();
		build_addiu_a0(2 * store_offset);
		__build_store_zero_reg();
		__build_store_zero_reg();
		__build_store_zero_reg();
		build_bne(dest);
		 __build_store_zero_reg();
	}

	build_jr_ra();
	if (R4600_V2_HIT_CACHEOP_WAR && ((read_c0_prid() & 0xfff0) == 0x2020))
		*epc++ = 0x40826000;		/* mtc0    $v0, $12	*/
	else
		build_nop();

	flush_icache_range((unsigned long)&clear_page_array,
	                   (unsigned long) epc);

	BUG_ON(epc >= clear_page_array + ARRAY_SIZE(clear_page_array));
}
