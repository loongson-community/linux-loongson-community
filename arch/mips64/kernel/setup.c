/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 Linus Torvalds
 * Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Ralf Baechle
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
#include <asm/cachectl.h>
#include <asm/io.h>
#include <asm/stackframe.h>
#include <asm/system.h>

struct mips_cpuinfo boot_cpu_data;

/*
 * Not all of the MIPS CPUs have the "wait" instruction available.  This
 * is set to true if it is available.  The wait instruction stops the
 * pipeline and reduces the power consumption of the CPU very much.
 */
char wait_available;

/*
 * Do we have a cyclecounter available?
 */
char cyclecounter_available;

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
unsigned long mips_memory_upper = KSEG0; /* this is set by kernel_entry() */
unsigned long mips_cputype = CPU_UNKNOWN;
unsigned long mips_machtype = MACH_UNKNOWN;
unsigned long mips_machgroup = MACH_GROUP_UNKNOWN;

unsigned char aux_device_present;
extern int _end;

extern char empty_zero_page[PAGE_SIZE];

static char command_line[CL_SIZE] = { 0, };
       char saved_command_line[CL_SIZE];
extern char arcs_cmdline[CL_SIZE];

extern void ip22_setup(void);

void __init setup_arch(char **cmdline_p, unsigned long * memory_start_p,
                       unsigned long * memory_end_p)
{
	unsigned long memory_end;
#ifdef CONFIG_BLK_DEV_INITRD
	unsigned long tmp;
	unsigned long *initrd_header;
#endif

#ifdef CONFIG_SGI_IP22
	ip22_setup();
#endif

	memory_end = mips_memory_upper;

	/*
	 * Due to prefetching and similar mechanism the CPU sometimes
	 * generates addresses beyond the end of memory.  We leave the size
	 * of one cache line at the end of memory unused to make shure we
	 * don't catch this type of bus errors.
	 */
	memory_end -= 128;
	memory_end &= PAGE_MASK;

	strncpy (command_line, arcs_cmdline, CL_SIZE);
	memcpy(saved_command_line, command_line, CL_SIZE);
	saved_command_line[CL_SIZE-1] = '\0';

	*cmdline_p = command_line;
	*memory_start_p = (unsigned long) &_end;
	*memory_end_p = memory_end;

#ifdef CONFIG_BLK_DEV_INITRD
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
}
