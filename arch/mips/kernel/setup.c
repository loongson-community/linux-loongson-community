/*
 *  linux/arch/mips/kernel/setup.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Copyright (C) 1995, 1996  Ralf Baechle
 *  Copyright (C) 1996  Stoned Elipot
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/user.h>
#include <linux/utsname.h>
#include <linux/a.out.h>
#include <linux/tty.h>
#ifdef CONFIG_BLK_DEV_RAM
#include <linux/blk.h>
#endif

#include <asm/asm.h>
#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/io.h>
#include <asm/vector.h>
#include <asm/uaccess.h>
#include <asm/stackframe.h>
#include <asm/system.h>

/*
 * How to handle the machine's features
 */
struct feature *feature;

/*
 * What to do to keep the caches consistent with memory
 * We don't use the normal cacheflush routine to keep Tyne caches happier.
 */
void (*fd_cacheflush)(const void *addr, size_t size);

/*
 * Not all of the MIPS CPUs have the "wait" instruction available.  This
 * is set to true if it is available.  The wait instruction stops the
 * pipeline and reduces the power consumption of the CPU very much.
 */
char wait_available;

/*
 * There are several bus types available for MIPS machines.  "RISC PC"
 * type machines have ISA, EISA or PCI available, some DECstations have
 * Turbochannel, SGI has GIO, there are lots of VME boxes ...
 * This flag is set if a EISA slots are available.
 */
int EISA_bus = 0;

/*
 * Do a hardware reset.
 */
void (*hard_reset_now)(void);

/*
 * Milo passes some information to the kernel that looks like as if it
 * had been returned by a Intel PC BIOS.  Milo doesn't fill the passed
 * drive_info and Linux can find out about this anyway, so I'm going to
 * remove this sometime.  screen_info contains information about the 
 * resolution of the text screen.  For VGA graphics based machine this
 * information is being use to continue the screen output just below
 * the BIOS printed text and with the same text resolution.
 */
struct drive_info_struct drive_info = DEFAULT_DRIVE_INFO;
struct screen_info screen_info = DEFAULT_SCREEN_INFO;

/*
 * setup informations
 *
 * These are intialized so they are in the .data section
 */
unsigned long mips_memory_upper = KSEG0; /* this is set by kernel_entry() */
unsigned long mips_cputype = CPU_UNKNOWN;
unsigned long mips_machtype = MACH_UNKNOWN; /* this is set by bi_EarlySnarf() */
unsigned long mips_machgroup = MACH_GROUP_UNKNOWN; /* this is set by bi_EarlySnarf() */
unsigned long mips_tlb_entries = 48; /* this is set by bi_EarlySnarf() */
unsigned long mips_vram_base = KSEG0;

extern int root_mountflags;
extern int _end;

extern char empty_zero_page[PAGE_SIZE];

/*
 * This is set up by the setup-routine at boot-time
 */
#define PARAM	empty_zero_page
#define LOADER_TYPE (*(unsigned char *) (PARAM+0x210))
#define INITRD_START (*(unsigned long *) (PARAM+0x218))
#define INITRD_SIZE (*(unsigned long *) (PARAM+0x21c))

static char command_line[CL_SIZE] = { 0, };
       char saved_command_line[CL_SIZE];

/*
 * The board specific setup routine sets irq_setup to point to a board
 * specific setup routine.
 */
void (*irq_setup)(void);

static void default_irq_setup(void)
{
	panic("Unknown machtype in init_IRQ");
}

static void default_fd_cacheflush(const void *addr, size_t size)
{
}

static asmlinkage void
default_cacheflush(unsigned long addr, unsigned long nbytes, unsigned int flags)
{
	/*
	 * Someone didn't set his cacheflush() handler ...
	 */
	panic("default_cacheflush() called.\n");
}
asmlinkage void (*cacheflush)(unsigned long addr, unsigned long nbytes, unsigned int flags) = default_cacheflush;

static __inline__ void
cpu_init(void)
{
	asmlinkage void handle_reserved(void);
	void mips1_cpu_init(void);
	void mips2_cpu_init(void);
	void mips3_cpu_init(void);
	void mips4_cpu_init(void);
	int i;

	/*
	 * Setup default vectors
	 */
	for (i=0;i<=31;i++)
		set_except_vector(i, handle_reserved);

	switch(mips_cputype) {
#ifdef CONFIG_CPU_R3000
	case CPU_R2000: case CPU_R3000: case CPU_R3000A: case CPU_R3041:
	case CPU_R3051: case CPU_R3052: case CPU_R3081: case CPU_R3081E:
		mips1_cpu_init();
		break;
#endif
#ifdef CONFIG_CPU_R6000
	case CPU_R6000: case CPU_R6000A:
		mips2_cpu_init();
		break;
#endif
#ifdef CONFIG_CPU_R4X00
	case CPU_R4000MC: case CPU_R4400MC: case CPU_R4000SC:
	case CPU_R4400SC: case CPU_R4000PC: case CPU_R4400PC:
	case CPU_R4200: case CPU_R4300:   /* case CPU_R4640: */
	case CPU_R4600: case CPU_R4700:
		mips3_cpu_init();
		break;
#endif
#ifdef CONFIG_CPU_R8000
	case CPU_R8000: case CPU_R5000:
		printk("Detected unsupported CPU type %s.\n",
		       cpu_names[mips_cputype]);
		panic("Can't handle CPU");
		break;
#endif
#ifdef CONFIG_CPU_R10000
	case CPU_R10000:
		mips4_cpu_init();
#endif
	case CPU_UNKNOWN:
	default:
		panic("Unknown or unsupported CPU type");
	}
}

void setup_arch(char **cmdline_p,
                unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	unsigned long memory_end;
	tag* atag;
	void decstation_setup(void);
	void deskstation_setup(void);
	void jazz_setup(void);
	void sni_rm200_pci_setup(void);

	/* Perhaps a lot of tags are not getting 'snarfed' - */
	/* please help yourself */

	atag = bi_TagFind(tag_cputype);
	memcpy(&mips_cputype, TAGVALPTR(atag), atag->size);

	cpu_init();

	atag = bi_TagFind(tag_vram_base);
	memcpy(&mips_vram_base, TAGVALPTR(atag), atag->size);

	irq_setup = default_irq_setup;
	fd_cacheflush = default_fd_cacheflush;

	switch(mips_machgroup)
	{
#ifdef CONFIG_MIPS_DECSTATION
	case MACH_GROUP_DEC:
		decstation_setup();
		break;
#endif
#if defined(CONFIG_MIPS_ARC) 
/* Perhaps arch/mips/deskstation should be renommed arch/mips/arc.
 * For now CONFIG_MIPS_ARC means DeskStation. -Stoned.
 */
	case MACH_GROUP_ARC:
		deskstation_setup();
		break;
#endif
#ifdef CONFIG_MIPS_JAZZ
	case MACH_GROUP_JAZZ:
		jazz_setup();
		break;
#endif
#ifdef CONFIG_SNI_RM200_PCI
	case MACH_GROUP_SNI_RM:
		sni_rm200_pci_setup();
		break;
#endif
	default:
		panic("Unsupported architecture");
	}

	atag = bi_TagFind(tag_drive_info);
	memcpy(&drive_info, TAGVALPTR(atag), atag->size);

	memory_end = mips_memory_upper;
	/*
	 * Due to prefetching and similar mechanism the CPU sometimes
	 * generates addresses beyond the end of memory.  We leave the size
	 * of one cache line at the end of memory unused to make shure we
	 * don't catch this type of bus errors.
	 */
	memory_end -= 32;
	memory_end &= PAGE_MASK;

#ifdef CONFIG_BLK_DEV_RAM
	rd_image_start = RAMDISK_FLAGS & RAMDISK_IMAGE_START_MASK;
	rd_prompt = ((RAMDISK_FLAGS & RAMDISK_PROMPT_FLAG) != 0);
	rd_doload = ((RAMDISK_FLAGS & RAMDISK_LOAD_FLAG) != 0);
#endif
#ifdef CONFIG_MAX_16M
	/*
	 * There is a quite large number of different PC chipset based boards
	 * available and so I include this option here just in case ...
	 */
	if (memory_end > PAGE_OFFSET + 16*1024*1024)
		memory_end = PAGE_OFFSET + 16*1024*1024;
#endif

	atag= bi_TagFind(tag_screen_info);
	if (atag)
	    memcpy(&screen_info, TAGVALPTR(atag), atag->size);

	atag = bi_TagFind(tag_command_line);
	if (atag)
	  memcpy(&command_line, TAGVALPTR(atag), atag->size);	  
	printk("Command line: '%s'\n", command_line);

	memcpy(saved_command_line, command_line, CL_SIZE);
	saved_command_line[CL_SIZE-1] = '\0';

	*cmdline_p = command_line;
	*memory_start_p = (unsigned long) &_end;
	*memory_end_p = memory_end;

#ifdef CONFIG_BLK_DEV_INITRD
	if (LOADER_TYPE) {
		initrd_start = INITRD_START ? INITRD_START + PAGE_OFFSET : 0;
		initrd_end = initrd_start+INITRD_SIZE;
		if (initrd_end > memory_end) {
			printk("initrd extends beyond end of memory "
			       "(0x%08lx > 0x%08lx)\ndisabling initrd\n",
			       initrd_end,memory_end);
		initrd_start = 0;
		}
	}
#endif
}
