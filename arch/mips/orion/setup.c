/*
 * Catch-all for Orion-specific code that doesn't fit easily elsewhere.
 *   -- Cort
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/init.h>
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
#include <linux/interrupt.h>
#include <linux/fs.h>
#ifdef CONFIG_BLK_DEV_RAM
#include <linux/blk.h>
#endif
#include <linux/ide.h>
#ifdef CONFIG_RTC
#include <linux/timex.h>
#endif

#include <asm/asm.h>
#include <asm/bootinfo.h>
#include <asm/cachectl.h>
#include <asm/io.h>
#include <asm/stackframe.h>
#include <asm/system.h>
#include <asm/cpu.h>
#include <linux/sched.h>
#include <linux/bootmem.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/mc146818rtc.h>
#include <asm/orion.h>

struct app_header_s {
	unsigned long    MAGIC_JMP;
	unsigned long    MAGIC_NOP;
	unsigned long    header_tag;    
	unsigned long    header_flags;
	unsigned long    header_length;
	unsigned long    header_cksum;
     
	void             *load_addr;
	void             *end_addr;
	void             *start_addr;
	char             *app_name_p;
	char             *version_p;
	char             *date_p;
	char             *time_p;
	unsigned long    type;
	unsigned long    crc;
	unsigned long    reserved;
};

typedef struct app_header_s app_header_t;
char linked_app_name[] = "linux";
char *linked_app_name_p = &linked_app_name[0];

char linked_app_ver[] = "2.4 -test1";
char *linked_app_ver_p = &linked_app_ver[0];

char linked_app_date[] = "today";
char *linked_app_date_p = &linked_app_date[0];

char linked_app_time[] = "now";
char *linked_app_time_p = &linked_app_time[0];
extern void *__bss_start;
extern void *kernel_entry;

app_header_t app_header __attribute__ ((section (".app_header"))) = {
	(0x10000000 | (((sizeof(app_header_t)>>2)-1) & 0xffff)) ,
	0 ,
	(((( 0x4321  ) & 0xFFFF) << 16) | ((  0x0100  ) & 0xFFFF))  ,
	0x80000000 ,
	sizeof(app_header_t),
	0,
	&app_header,
	&__bss_start,
	&kernel_entry,
	linked_app_name,
	linked_app_ver,
	linked_app_date,
	linked_app_time,
	0
};

char arcs_cmdline[CL_SIZE] = { "console=ttyS0,19200" };
extern int _end;

static unsigned char orion_rtc_read_data(unsigned long addr)
{
	return 0;
}

static void orion_rtc_write_data(unsigned char data, unsigned long addr)
{
}

static int orion_rtc_bcd_mode(void)
{
	return 0;
}

struct rtc_ops orion_rtc_ops = {
	&orion_rtc_read_data,
	&orion_rtc_write_data,
	&orion_rtc_bcd_mode
};

extern void InitCIB(void);
extern void InitQpic(void);
extern void InitCupid(void);

void __init orion_setup(void)
{
	extern void (*board_time_init)(struct irqaction *irq);
	void orion_time_init(struct irqaction *);
	
	rtc_ops = &orion_rtc_ops;
	board_time_init = orion_time_init;
	mips_io_port_base = GT64120_BASE;

	InitCIB();
	InitQpic();
	InitCupid();
}

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_ALIGN(x)	(((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)

unsigned long mem_size;

int __init prom_init(int a, char **b, char **c, int *d)
{
	unsigned long free_start, free_end, start_pfn, bootmap_size;
	extern unsigned long orion_initrd_start[], orion_initrd_size;
	extern int rd_size;

	mips_machgroup = MACH_GROUP_ORION;
	/* 64 MB non-upgradable */
	mem_size = 64 << 20;
	
	free_start = PHYSADDR(PFN_ALIGN(&_end));
	free_end = mem_size;
	start_pfn = PFN_UP((unsigned long)&_end);
	
	/* Register all the contiguous memory with the bootmem allocator
	   and free it.  Be careful about the bootmem freemap.  */
	bootmap_size = init_bootmem(start_pfn, mem_size >> PAGE_SHIFT);
	
	/* Free the entire available memory after the _end symbol.  */
	free_start += bootmap_size;
	free_bootmem(free_start, free_end-free_start);

	initrd_start = (ulong)orion_initrd_start;
	initrd_end = (ulong)orion_initrd_start + (ulong)orion_initrd_size;
	initrd_below_start_ok = 1;
	real_root_dev = 0x0100; /* ramdisk */
	/*
	 * Change the ramdisk size to 12M since we only have a ramdisk
	 * root on the orion and need a decent size.
	 *  -- Cort <cort@fsmlabs.com>
	 */
	rd_size = 22<<10;

	return 0;
}

void __init prom_free_prom_memory (void)
{
}

int __init page_is_ram(unsigned long pagenr)
{
	if (pagenr < (mem_size >> PAGE_SHIFT))
		return 1;

	return 0;
}
