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
 * Copyright (C) 2002  Maciej W. Rozycki
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/user.h>
#include <linux/utsname.h>
#include <linux/a.out.h>
#include <linux/tty.h>
#include <linux/bootmem.h>
#ifdef CONFIG_BLK_DEV_RAM
#include <linux/blk.h>
#endif

#include <asm/addrspace.h>
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

/*
 * mips_io_port_base is the begin of the address space to which x86 style
 * I/O ports are mapped.
 */
const unsigned long mips_io_port_base = -1;
EXPORT_SYMBOL(mips_io_port_base);

extern void ip22_setup(void);
extern void ip27_setup(void);
extern void ip32_setup(void);

extern void sgi_sysinit(void);
extern void SetUpBootInfo(void);
extern void load_mmu(void);
extern ATTRIB_NORET asmlinkage void start_kernel(void);
extern void prom_init(int, char **, char **, int *);

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
	case CPU_4KC:
	case CPU_4KEC:
	case CPU_4KSC:
	case CPU_5KC:
/*	case CPU_20KC:*/
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

/*
 * Get the FPU Implementation/Revision.
 */
static inline unsigned long cpu_get_fpu_id(void)
{
	unsigned long tmp, fpu_id;

	tmp = read_32bit_cp0_register(CP0_STATUS);
	__enable_fpu();
	fpu_id = read_32bit_cp1_register(CP1_REVISION);
	write_32bit_cp0_register(CP0_STATUS, tmp);
	return fpu_id;
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
	unsigned long config0 = read_32bit_cp0_register(CP0_CONFIG);
	unsigned long config1;

        if (config0 & (1 << 31)) {
		/* MIPS32 compliant CPU. Read Config 1 register. */
		mips_cpu.isa_level = MIPS_CPU_ISA_M32;
		mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX | 
			MIPS_CPU_4KTLB | MIPS_CPU_COUNTER | MIPS_CPU_DIVEC;
		config1 = read_mips32_cp0_config1();
		if (config1 & (1 << 3))
			mips_cpu.options |= MIPS_CPU_WATCH;
		if (config1 & (1 << 2))
			mips_cpu.options |= MIPS_CPU_MIPS16;
		if (config1 & (1 << 1))
			mips_cpu.options |= MIPS_CPU_EJTAG;
		if (config1 & 1)
			mips_cpu.options |= MIPS_CPU_FPU;
		mips_cpu.scache.flags = MIPS_CACHE_NOT_PRESENT;
	}
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

			if ((mips_cpu.processor_id & 0xf0) ==
			    (PRID_REV_TX3927 & 0xf0)) {
				mips_cpu.cputype = CPU_TX3927;
				mips_cpu.tlbsize = 64;
				mips_cpu.icache.ways = 2;
				mips_cpu.dcache.ways = 2;
			} else {
				switch (mips_cpu.processor_id & 0xff) {
				case PRID_REV_TX3912:
					mips_cpu.cputype = CPU_TX3912;
					mips_cpu.tlbsize = 32;
					break;
				case PRID_REV_TX3922:
					mips_cpu.cputype = CPU_TX3922;
					mips_cpu.tlbsize = 64;
					break;
				default:
					mips_cpu.cputype = CPU_UNKNOWN;
					break;
				}
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
			                   MIPS_CPU_32FPR | MIPS_CPU_WATCH;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_R5500:
			mips_cpu.cputype = CPU_R5500;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV; 
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_WATCH;
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
		case PRID_IMP_R12000:
			mips_cpu.cputype = CPU_R12000;
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
			break;
		case PRID_IMP_4KEC:
			mips_cpu.cputype = CPU_4KEC;
			break;
		case PRID_IMP_4KSC:
			mips_cpu.cputype = CPU_4KSC;
			break;
		case PRID_IMP_5KC:
			mips_cpu.cputype = CPU_5KC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M64;
			break;
		case PRID_IMP_20KC:
			mips_cpu.cputype = CPU_20KC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M64;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}		
		break;
	case PRID_COMP_ALCHEMY:
		switch (mips_cpu.processor_id & 0xff00) {
		case PRID_IMP_AU1_REV1:
		case PRID_IMP_AU1_REV2:
			if (mips_cpu.processor_id & 0xff000000)
				mips_cpu.cputype = CPU_AU1500;
			else
				mips_cpu.cputype = CPU_AU1000;
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
	if (mips_cpu.options & MIPS_CPU_FPU)
		mips_cpu.fpu_id = cpu_get_fpu_id();
}

static inline void cpu_report(void)
{
	printk("CPU revision is: %08x\n", mips_cpu.processor_id);
	if (mips_cpu.options & MIPS_CPU_FPU)
		printk("FPU revision is: %08x\n", mips_cpu.fpu_id);
}

asmlinkage void __init init_arch(int argc, char **argv, char **envp,
	int *prom_vec)
{
	/* Determine which MIPS variant we are running on. */
	cpu_probe();

	prom_init(argc, argv, envp, prom_vec);

#ifdef CONFIG_SGI_IP22
	sgi_sysinit();
#endif

	cpu_report();

	/*
	 * Determine the mmu/cache attached to this machine, then flush the
	 * tlb and caches.  On the r4xx0 variants this also sets CP0_WIRED to
	 * zero.
	 */
	load_mmu();

	/*
	 * On IP27, I am seeing the TS bit set when the kernel is loaded.
	 * Maybe because the kernel is in ckseg0 and not xkphys? Clear it
	 * anyway ...
	 */
	clear_cp0_status(ST0_BEV|ST0_TS|ST0_CU1|ST0_CU2|ST0_CU3);
	set_cp0_status(ST0_CU0|ST0_KX|ST0_SX|ST0_FR);

	start_kernel();
}

void __init add_memory_region(phys_t start, phys_t size,
			      long type)
{
	int x = boot_mem_map.nr_map;

	if (x == BOOT_MEM_MAP_MAX) {
		printk("Ooops! Too many entries in the memory map!\n");
		return;
	}

	boot_mem_map.map[x].addr = start;
	boot_mem_map.map[x].size = size;
	boot_mem_map.map[x].type = type;
	boot_mem_map.nr_map++;
}

static void __init print_memory_map(void)
{
	int i;

	for (i = 0; i < boot_mem_map.nr_map; i++) {
		printk(" memory: %08Lx @ %08Lx ",
			(unsigned long long) boot_mem_map.map[i].size,
			(unsigned long long) boot_mem_map.map[i].addr);
		switch (boot_mem_map.map[i].type) {
		case BOOT_MEM_RAM:
			printk("(usable)\n");
			break;
		case BOOT_MEM_ROM_DATA:
			printk("(ROM data)\n");
			break;
		case BOOT_MEM_RESERVED:
			printk("(reserved)\n");
			break;
		default:
			printk("type %lu\n", boot_mem_map.map[i].type);
			break;
		}
	}
}

static inline void parse_mem_cmdline(void)
{
	char c = ' ', *to = command_line, *from = saved_command_line;
	unsigned long start_at, mem_size;
	int len = 0;
	int usermem = 0;

	printk("Determined physical RAM map:\n");
	print_memory_map();

	for (;;) {
		/*
		 * "mem=XXX[kKmM]" defines a memory region from
		 * 0 to <XXX>, overriding the determined size.
		 * "mem=XXX[KkmM]@YYY[KkmM]" defines a memory region from
		 * <YYY> to <YYY>+<XXX>, overriding the determined size.
		 */
		if (c == ' ' && !memcmp(from, "mem=", 4)) {
			if (to != command_line)
				to--;
			/*
			 * If a user specifies memory size, we
			 * blow away any automatically generated
			 * size.
			 */
			if (usermem == 0) {
				boot_mem_map.nr_map = 0;
				usermem = 1;
			}
			mem_size = memparse(from + 4, &from);
			if (*from == '@')
				start_at = memparse(from + 1, &from);
			else
				start_at = 0;
			add_memory_region(start_at, mem_size, BOOT_MEM_RAM);
		}
		c = *(from++);
		if (!c)
			break;
		if (CL_SIZE <= ++len)
			break;
		*(to++) = c;
	}
	*to = '\0';

	if (usermem) {
		printk("User-defined physical RAM map:\n");
		print_memory_map();
	}
}

void bootmem_init(void)
{
#ifdef CONFIG_BLK_DEV_INITRD
	unsigned long tmp;
	unsigned long *initrd_header;
#endif
	unsigned long bootmap_size;
	unsigned long start_pfn, max_pfn;
	int i;
	extern int _end;

#define PFN_UP(x)	(((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((x) << PAGE_SHIFT)

	/*
	 * Partially used pages are not usable - thus
	 * we are rounding upwards.
	 * start_pfn = PFN_UP(__pa(&_end));
	 */
	start_pfn = PFN_UP((unsigned long)&_end - KSEG0);

	/* Find the highest page frame number we have available.  */
	max_pfn = 0;
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long start, end;

		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
			continue;

		start = PFN_UP(boot_mem_map.map[i].addr);
		end = PFN_DOWN(boot_mem_map.map[i].addr
		      + boot_mem_map.map[i].size);

		if (start >= end)
			continue;
		if (end > max_pfn)
			max_pfn = end;
	}

	/* Initialize the boot-time allocator.  */
	bootmap_size = init_bootmem(start_pfn, max_pfn);

	/*
	 * Register fully available low RAM pages with the bootmem allocator.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long curr_pfn, last_pfn, size;

		/*
		 * Reserve usable memory.
		 */
		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
			continue;

		/*
		 * We are rounding up the start address of usable memory:
		 */
		curr_pfn = PFN_UP(boot_mem_map.map[i].addr);
		if (curr_pfn >= max_pfn)
			continue;
		if (curr_pfn < start_pfn)
			curr_pfn = start_pfn;

		/*
		 * ... and at the end of the usable range downwards:
		 */
		last_pfn = PFN_DOWN(boot_mem_map.map[i].addr
				    + boot_mem_map.map[i].size);

		if (last_pfn > max_pfn)
			last_pfn = max_pfn;

		/*
		 * ... finally, did all the rounding and playing
		 * around just make the area go away?
		 */
		if (last_pfn <= curr_pfn)
			continue;

		size = last_pfn - curr_pfn;
		free_bootmem(PFN_PHYS(curr_pfn), PFN_PHYS(size));
	}

	/* Reserve the bootmap memory.  */
	reserve_bootmem(PFN_PHYS(start_pfn), bootmap_size);

#ifdef CONFIG_BLK_DEV_INITRD
#error "Initrd is broken, please fit it."
	tmp = (((unsigned long)&_end + PAGE_SIZE-1) & PAGE_MASK) - 8;
	if (tmp < (unsigned long)&_end)
		tmp += PAGE_SIZE;
	initrd_header = (unsigned long *)tmp;
	if (initrd_header[0] == 0x494E5244) {
		initrd_start = (unsigned long)&initrd_header[2];
		initrd_end = initrd_start + initrd_header[1];
		initrd_below_start_ok = 1;
		if (initrd_end > memory_end) {
			printk("initrd extends beyond end of memory "
			       "(0x%08lx > 0x%08lx)\ndisabling initrd\n",
			       initrd_end,memory_end);
			initrd_start = 0;
		} else
			*memory_start_p = initrd_end;
	}
#endif

#undef PFN_UP
#undef PFN_DOWN
#undef PFN_PHYS

}

void __init setup_arch(char **cmdline_p)
{
#ifdef CONFIG_SGI_IP22
	ip22_setup();
#endif
#ifdef CONFIG_SGI_IP27
	ip27_setup();
#endif
#ifdef CONFIG_SGI_IP32
	ip32_setup();
#endif
#ifdef CONFIG_SIBYTE_SWARM
	swarm_setup();
#endif

	strncpy(command_line, arcs_cmdline, CL_SIZE);
	memcpy(saved_command_line, command_line, CL_SIZE);
	saved_command_line[CL_SIZE-1] = '\0';

	*cmdline_p = command_line;

	parse_mem_cmdline();

	bootmem_init();

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
