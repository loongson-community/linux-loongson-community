/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 Linus Torvalds
 * Copyright (C) 1995 Waldorf Electronics
 * Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001 Ralf Baechle
 * Copyright (C) 1996 Stoned Elipot
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/user.h>
#include <linux/utsname.h>
#include <linux/a.out.h>
#include <linux/tty.h>
#ifdef CONFIG_BLK_DEV_RAM
#include <linux/blk.h>
#endif

#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/system.h>
#include <asm/pgalloc.h>

#ifndef CONFIG_SMP
struct cpuinfo_mips cpu_data[1];
#endif

/*
 * Not all of the MIPS CPUs have the "wait" instruction available. Moreover,
 * the implementation of the "wait" feature differs between CPU families. This
 * points to the function that implements CPU specific wait. 
 * The wait instruction stops the pipeline and reduces the power consumption of
 * the CPU very much.
 */
void (*cpu_wait)(void) = NULL;

#ifdef CONFIG_VT
struct screen_info screen_info;
#endif

/*
 * Not all of the MIPS CPUs have the "wait" instruction available.  This
 * is set to true if it is available.  The wait instruction stops the
 * pipeline and reduces the power consumption of the CPU very much.
 */
char wait_available;

/*
 * Set if box has EISA slots.
 */
int EISA_bus = 0;

#ifdef CONFIG_BLK_DEV_FD
extern struct fd_ops no_fd_ops;
struct fd_ops *fd_ops;
#endif

#ifdef CONFIG_BLK_DEV_IDE
extern struct ide_ops no_ide_ops;
struct ide_ops *ide_ops;
#endif

extern struct rtc_ops no_rtc_ops;
struct rtc_ops *rtc_ops;

extern struct kbd_ops no_kbd_ops;
struct kbd_ops *kbd_ops;

/*
 * Setup information
 *
 * These are initialized so they are in the .data section
 */
unsigned long mips_machtype = MACH_UNKNOWN;
unsigned long mips_machgroup = MACH_GROUP_UNKNOWN;

struct boot_mem_map boot_mem_map;

unsigned char aux_device_present;

extern void load_mmu(void);

static char command_line[CL_SIZE] = { 0, };
       char saved_command_line[CL_SIZE];
extern char arcs_cmdline[CL_SIZE];

extern void ip22_setup(void);
extern void ip27_setup(void);
extern void ip32_setup(void);

static inline void check_wait(void)
{
	printk("Checking for 'wait' instruction... ");
	switch(mips_cpu.cputype) {
	case CPU_R3081:
	case CPU_R3081E:
		cpu_wait = r3081_wait;
		printk(" available.\n");
		break;
	case CPU_TX3927:
	case CPU_TX39XX:
		cpu_wait = r39xx_wait;
		printk(" available.\n");
		break;
	case CPU_R4200: 
/*	case CPU_R4300: */
	case CPU_R4600: 
	case CPU_R4640: 
	case CPU_R4650: 
	case CPU_R4700: 
	case CPU_R5000: 
	case CPU_NEVADA:
	case CPU_RM7000:
	case CPU_TX49XX:
		cpu_wait = r4k_wait;
		printk(" available.\n");
		break;
	default:
		printk(" unavailable.\n");
		break;
	}
}

void __init check_bugs(void)
{
	check_wait();
}

/*
 * Probe whether cpu has config register by trying to play with
 * alternate cache bit and see whether it matters.
 * It's used by cpu_probe to distinguish between R3000A and R3081.
 */
static inline int cpu_has_confreg(void)
{
#ifdef CONFIG_CPU_R3000
	extern unsigned long r3k_cache_size(unsigned long);
	unsigned long size1, size2;
	unsigned long cfg = read_32bit_cp0_register(CP0_CONF);

	size1 = r3k_cache_size(ST0_ISC);
	write_32bit_cp0_register(CP0_CONF, cfg^CONF_AC);
	size2 = r3k_cache_size(ST0_ISC);
	write_32bit_cp0_register(CP0_CONF, cfg);
	return size1 != size2;
#else
	return 0;
#endif
}

/* declaration of the global struct */
struct mips_cpu mips_cpu = {
    processor_id:	PRID_IMP_UNKNOWN,
    fpu_id:		FPIR_IMP_NONE,
    cputype:		CPU_UNKNOWN
};

#define R4K_OPTS (MIPS_CPU_TLB | MIPS_CPU_4KEX | MIPS_CPU_4KTLB \
                | MIPS_CPU_COUNTER | MIPS_CPU_CACHE_CDEX)

static inline void cpu_probe(void)
{
#ifdef CONFIG_CPU_MIPS32
	unsigned long config1;
#endif

	mips_cpu.processor_id = read_32bit_cp0_register(CP0_PRID);
	switch (mips_cpu.processor_id & 0xff0000) {
	case PRID_COMP_LEGACY:
		switch (mips_cpu.processor_id & 0xff00) {
		case PRID_IMP_R2000:
			mips_cpu.cputype = CPU_R2000;
			mips_cpu.isa_level = MIPS_CPU_ISA_I;
			mips_cpu.options = MIPS_CPU_TLB;
			mips_cpu.tlbsize = 64;
			break;
		case PRID_IMP_R3000:
			if ((mips_cpu.processor_id & 0xff) == PRID_REV_R3000A)
				if (cpu_has_confreg())
					mips_cpu.cputype = CPU_R3081E;
				else
					mips_cpu.cputype = CPU_R3000A;
			else
				mips_cpu.cputype = CPU_R3000;
			mips_cpu.isa_level = MIPS_CPU_ISA_I;
			mips_cpu.options = MIPS_CPU_TLB;
			mips_cpu.tlbsize = 64;
			break;
		case PRID_IMP_R4000:
			if ((mips_cpu.processor_id & 0xff) == PRID_REV_R4400)
				mips_cpu.cputype = CPU_R4400SC;
			else
				mips_cpu.cputype = CPU_R4000SC;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_WATCH |
			                   MIPS_CPU_VCE;
			mips_cpu.tlbsize = 48;
			break;
                case PRID_IMP_VR41XX:
                        mips_cpu.cputype = CPU_VR41XX;
                        mips_cpu.isa_level = MIPS_CPU_ISA_III;
                        mips_cpu.options = R4K_OPTS;
                        mips_cpu.tlbsize = 32;
                        break;
		case PRID_IMP_R4300:
			mips_cpu.cputype = CPU_R4300;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU | MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 32;
			break;
		case PRID_IMP_R4600:
			mips_cpu.cputype = CPU_R4600;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU;
			mips_cpu.tlbsize = 48;
			break;
		#if 0
 		case PRID_IMP_R4650:
			/*
			 * This processor doesn't have an MMU, so it's not
			 * "real easy" to run Linux on it. It is left purely
			 * for documentation.  Commented out because it shares
			 * it's c0_prid id number with the TX3900.
			 */
	 		mips_cpu.cputype = CPU_R4650;
		 	mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU;
		        mips_cpu.tlbsize = 48;
			break;
		#endif
		case PRID_IMP_TX39:
			mips_cpu.isa_level = MIPS_CPU_ISA_I;
			mips_cpu.options = MIPS_CPU_TLB;

			switch (mips_cpu.processor_id & 0xff) {
			case PRID_REV_TX3912:
				mips_cpu.cputype = CPU_TX3912;
				mips_cpu.tlbsize = 32;
				break;
			case PRID_REV_TX3922:
				mips_cpu.cputype = CPU_TX3922;
				mips_cpu.tlbsize = 64;
				break;
			case PRID_REV_TX3927:
			case PRID_REV_TX3927B:
				/* check core-mode */
				if ((*(volatile u32 *)0xfffee004 >> 16) == 0x3927)
					mips_cpu.cputype = CPU_TX3927;
				else
					mips_cpu.cputype = CPU_TX39XX;
				mips_cpu.tlbsize = 64;
				mips_cpu.icache.ways = 2;
				mips_cpu.dcache.ways = 2;
				break;
			case PRID_REV_TX39H3TEG:
				/* support core-mode only */
				mips_cpu.cputype = CPU_TX39XX;
				mips_cpu.tlbsize = 32;
				mips_cpu.icache.ways = 2;
				mips_cpu.dcache.ways = 2;
				break;
			default:
				mips_cpu.cputype = CPU_UNKNOWN;
				break;
			}
			break;
		case PRID_IMP_R4700:
			mips_cpu.cputype = CPU_R4700;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_TX49:
			mips_cpu.cputype = CPU_TX49XX;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			mips_cpu.icache.ways = 4;
			mips_cpu.dcache.ways = 4;
			break;
		case PRID_IMP_R5000:
			mips_cpu.cputype = CPU_R5000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV; 
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_R5432:
			mips_cpu.cputype = CPU_R5432;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV; 
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_R5500:
			mips_cpu.cputype = CPU_R5500;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV; 
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_NEVADA:
			mips_cpu.cputype = CPU_NEVADA;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV; 
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_DIVEC;
			mips_cpu.tlbsize = 48;
			mips_cpu.icache.ways = 2;
			mips_cpu.dcache.ways = 2;
			break;
		case PRID_IMP_R6000:
			mips_cpu.cputype = CPU_R6000;
			mips_cpu.isa_level = MIPS_CPU_ISA_II;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_FPU;
			mips_cpu.tlbsize = 32;
			break;
		case PRID_IMP_R6000A:
			mips_cpu.cputype = CPU_R6000A;
			mips_cpu.isa_level = MIPS_CPU_ISA_II;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_FPU;
			mips_cpu.tlbsize = 32;
			break;
		case PRID_IMP_RM7000:
			mips_cpu.cputype = CPU_RM7000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			/*
			 * Undocumented RM7000:  Bit 29 in the info register of
			 * the RM7000 v2.0 indicates if the TLB has 48 or 64
			 * entries.
			 *
			 * 29      1 =>    64 entry JTLB
			 *         0 =>    48 entry JTLB
			 */
			mips_cpu.tlbsize = (get_info() & (1 << 29)) ? 64 : 48;
			break;
		case PRID_IMP_R8000:
			mips_cpu.cputype = CPU_R8000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
				           MIPS_CPU_FPU | MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 384;      /* has wierd TLB: 3-way x 128 */
			break;
		case PRID_IMP_R10000:
			mips_cpu.cputype = CPU_R10000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX | 
				           MIPS_CPU_FPU | MIPS_CPU_32FPR | 
				           MIPS_CPU_COUNTER | MIPS_CPU_WATCH;
			mips_cpu.tlbsize = 64;
			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}
		break;
#ifdef CONFIG_CPU_MIPS32
	case PRID_COMP_MIPS:
		switch (mips_cpu.processor_id & 0xff00) {
		case PRID_IMP_4KC:
			mips_cpu.cputype = CPU_4KC;
			goto cpu_4kc;
		case PRID_IMP_4KEC:
			mips_cpu.cputype = CPU_4KEC;
			goto cpu_4kc;
		case PRID_IMP_4KSC:
			mips_cpu.cputype = CPU_4KSC;
cpu_4kc:
			/*
			 * Why do we set all these options by default, THEN
			 * query them??
			 */
			mips_cpu.isa_level = MIPS_CPU_ISA_M32;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX | 
				           MIPS_CPU_4KTLB | MIPS_CPU_COUNTER | 
				           MIPS_CPU_DIVEC | MIPS_CPU_WATCH |
			                   MIPS_CPU_MCHECK;
			config1 = read_mips32_cp0_config1();
			if (config1 & (1 << 3))
				mips_cpu.options |= MIPS_CPU_WATCH;
			if (config1 & (1 << 2))
				mips_cpu.options |= MIPS_CPU_MIPS16;
			if (config1 & 1)
				mips_cpu.options |= MIPS_CPU_FPU;
			mips_cpu.scache.flags = MIPS_CACHE_NOT_PRESENT;
			break;
		case PRID_IMP_5KC:
			mips_cpu.cputype = CPU_5KC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M64;
			/* See comment above about querying options */
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX | 
				           MIPS_CPU_4KTLB | MIPS_CPU_COUNTER | 
				           MIPS_CPU_DIVEC | MIPS_CPU_WATCH |
			                   MIPS_CPU_MCHECK;
			config1 = read_mips32_cp0_config1();
			if (config1 & (1 << 3))
				mips_cpu.options |= MIPS_CPU_WATCH;
			if (config1 & (1 << 2))
				mips_cpu.options |= MIPS_CPU_MIPS16;
			if (config1 & 1)
				mips_cpu.options |= MIPS_CPU_FPU;
			mips_cpu.scache.flags = MIPS_CACHE_NOT_PRESENT;
			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}		
		break;
	case PRID_COMP_ALCHEMY:
		switch (mips_cpu.processor_id & 0xff00) {
		case PRID_IMP_AU1_REV1:
		case PRID_IMP_AU1_REV2:
			mips_cpu.cputype = CPU_AU1000;
			mips_cpu.isa_level = MIPS_CPU_ISA_M32;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX | 
					   MIPS_CPU_4KTLB | MIPS_CPU_COUNTER | 
					   MIPS_CPU_DIVEC | MIPS_CPU_WATCH;
			config1 = read_mips32_cp0_config1();
			if (config1 & (1 << 3))
				mips_cpu.options |= MIPS_CPU_WATCH;
			if (config1 & (1 << 2))
				mips_cpu.options |= MIPS_CPU_MIPS16;
			if (config1 & 1)
				mips_cpu.options |= MIPS_CPU_FPU;
			mips_cpu.scache.flags = MIPS_CACHE_NOT_PRESENT;
			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}
		break;
#endif /* CONFIG_CPU_MIPS32 */
	case PRID_COMP_SIBYTE:
		switch (mips_cpu.processor_id & 0xff00) {
		case PRID_IMP_SB1:
			mips_cpu.cputype = CPU_SB1;
			mips_cpu.isa_level = MIPS_CPU_ISA_M64;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
			                   MIPS_CPU_COUNTER | MIPS_CPU_DIVEC |
			                   MIPS_CPU_MCHECK;
#ifndef CONFIG_SB1_PASS_1_WORKAROUNDS
			/* FPU in pass1 is known to have issues. */
			mips_cpu.options |= MIPS_CPU_FPU;
#endif
			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}
		break;
	default:
		mips_cpu.cputype = CPU_UNKNOWN;
	}
}

void __init setup_arch(char **cmdline_p)
{
	cpu_probe();
	load_mmu();

#ifdef CONFIG_SGI_IP22
	ip22_setup();
#endif
#ifdef CONFIG_SGI_IP27
	ip27_setup();
#endif
#ifdef CONFIG_SGI_IP32
	ip32_setup();
#endif

#ifdef CONFIG_ARC_MEMORY
	bootmem_init ();
#endif

	strncpy(command_line, arcs_cmdline, CL_SIZE);
	memcpy(saved_command_line, command_line, CL_SIZE);
	saved_command_line[CL_SIZE-1] = '\0';

	*cmdline_p = command_line;

	paging_init();
}

void r3081_wait(void) 
{
	unsigned long cfg = read_32bit_cp0_register(CP0_CONF);
	write_32bit_cp0_register(CP0_CONF, cfg|CONF_HALT);
}

void r39xx_wait(void)
{
	unsigned long cfg = read_32bit_cp0_register(CP0_CONF);
	write_32bit_cp0_register(CP0_CONF, cfg|TX39_CONF_HALT);
}

void r4k_wait(void)
{
	__asm__(".set\tmips3\n\t"
		"wait\n\t"
		".set\tmips0");
}

int __init fpu_disable(char *s)
{
	mips_cpu.options &= ~MIPS_CPU_FPU;
	return 1;
}
__setup("nofpu", fpu_disable);
