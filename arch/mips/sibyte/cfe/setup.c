/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/blk.h>
#include <linux/bootmem.h>
#include <linux/smp.h>
#include <linux/console.h>

#include <asm/bootinfo.h>
#include <asm/reboot.h>
#include <asm/sibyte/board.h>

#include "cfe_xiocb.h"
#include "cfe_api.h"
#include "cfe_error.h"

/* Max ram addressable in 32-bit segments */
#ifdef CONFIG_MIPS64
#define MAX_RAM_SIZE (~0ULL)
#else
#ifdef CONFIG_HIGHMEM
#ifdef CONFIG_64BIT_PHYS_ADDR
#define MAX_RAM_SIZE (~0ULL)
#else
#define MAX_RAM_SIZE (0xffffffffULL)
#endif
#else
#define MAX_RAM_SIZE (0x1fffffffULL)
#endif
#endif

#define SB1250_DUART_MINOR_BASE		192 /* XXXKW put this somewhere sane */
#define SB1250_PROMICE_MINOR_BASE	191 /* XXXKW put this somewhere sane */
kdev_t cfe_consdev;

#define SIBYTE_MAX_MEM_REGIONS 8
phys_t board_mem_region_addrs[SIBYTE_MAX_MEM_REGIONS];
phys_t board_mem_region_sizes[SIBYTE_MAX_MEM_REGIONS];
unsigned int board_mem_region_count;

/* This is the kernel command line.  Actually, it's
   copied, eventually, to command_line, and looks to be
   quite redundant.  But not something to fix just now */
extern char arcs_cmdline[];

#ifdef CONFIG_EMBEDDED_RAMDISK
/* These are symbols defined by the ramdisk linker script */
extern unsigned char __rd_start;
extern unsigned char __rd_end;
#endif

#ifdef CONFIG_SMP
static int reboot_smp = 0;
#endif

static void cfe_linux_exit(void)
{
#ifdef CONFIG_SMP
	if (smp_processor_id()) {
		if (reboot_smp) {
			/* Don't repeat the process from another CPU */
			for (;;);
		} else {
			/* Get CPU 0 to do the cfe_exit */
			reboot_smp = 1;
			smp_call_function((void *)_machine_restart, NULL, 1, 0);
			for (;;);
		}
	}
#endif
	printk("passing control back to CFE\n");
	cfe_exit(1, 0);
	printk("cfe_exit returned??\n");
	while(1);
}

static __init void prom_meminit(void)
{
	u64 addr, size; /* regardless of 64BIT_PHYS_ADDR */
	long type;
	unsigned int idx;
	int rd_flag;
#ifdef CONFIG_BLK_DEV_INITRD
	unsigned long initrd_pstart;
	unsigned long initrd_pend;

#ifdef CONFIG_EMBEDDED_RAMDISK
	/* If we're using an embedded ramdisk, then __rd_start and __rd_end
	   are defined by the linker to be on either side of the ramdisk
	   area.  Otherwise, initrd_start should be defined by kernel command
	   line arguments */
	if (initrd_start == 0) {
		initrd_start = (unsigned long)&__rd_start;
		initrd_end = (unsigned long)&__rd_end;
	}
#endif

	initrd_pstart = __pa(initrd_start);
	initrd_pend = __pa(initrd_end);
	if (initrd_start &&
	    ((initrd_pstart > MAX_RAM_SIZE)
	     || (initrd_pend > MAX_RAM_SIZE))) {
		panic("initrd out of addressable memory");
	}

#endif /* INITRD */

	for (idx = 0; cfe_enummem(idx, &addr, &size, &type) != CFE_ERR_NOMORE;
	     idx++) {
		rd_flag = 0;
		if (type == CFE_MI_AVAILABLE) {
			/*
			 * See if this block contains (any portion of) the
			 * ramdisk
			 */
#ifdef CONFIG_BLK_DEV_INITRD
			if (initrd_start) {
				if ((initrd_pstart > addr) &&
				    (initrd_pstart < (addr + size))) {
					add_memory_region(addr,
					                  initrd_pstart - addr,
					                  BOOT_MEM_RAM);
					rd_flag = 1;
				}
				if ((initrd_pend > addr) &&
				    (initrd_pend < (addr + size))) {
					add_memory_region(initrd_pend,
						(addr + size) - initrd_pend,
						 BOOT_MEM_RAM);
					rd_flag = 1;
				}
			}
#endif
			if (!rd_flag) {
				if (addr > MAX_RAM_SIZE)
					continue;
				if (addr+size > MAX_RAM_SIZE)
					size = MAX_RAM_SIZE - (addr+size) + 1;
				/*
				 * memcpy/__copy_user prefetch, which
				 * will cause a bus error for
				 * KSEG/KUSEG addrs not backed by RAM.
				 * Hence, reserve some padding for the
				 * prefetch distance.
				 */
				if (size > 512)
					size -= 512;
				add_memory_region(addr, size, BOOT_MEM_RAM);
			}
			board_mem_region_addrs[board_mem_region_count] = addr;
			board_mem_region_sizes[board_mem_region_count] = size;
			board_mem_region_count++;
			if (board_mem_region_count ==
			    SIBYTE_MAX_MEM_REGIONS) {
				/*
				 * Too many regions.  Need to configure more
				 */
				while(1);
			}
		}
	}
#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start) {
		add_memory_region(initrd_pstart, initrd_pend - initrd_pstart,
				  BOOT_MEM_RESERVED);
	}
#endif
}

#ifdef CONFIG_BLK_DEV_INITRD
static int __init initrd_setup(char *str)
{
	/*
	 *Initrd location comes in the form "<hex size of ramdisk in bytes>@<location in memory>"
	 *  e.g. initrd=3abfd@80010000.  This is set up by the loader.
	 */
	char *tmp, *endptr;
	unsigned long initrd_size;
	for (tmp = str; *tmp != '@'; tmp++) {
		if (!*tmp) {
			goto fail;
		}
	}
	*tmp = 0;
	tmp++;
	if (!*tmp) {
		goto fail;
	}
	initrd_size = simple_strtol(str, &endptr, 16);
	if (*endptr) {
		*(tmp-1) = '@';
		goto fail;
	}
	*(tmp-1) = '@';
	initrd_start = simple_strtol(tmp, &endptr, 16);
	if (*endptr) {
		goto fail;
	}
	initrd_end = initrd_start + initrd_size;
	printk("Found initrd of %lx@%lx\n", initrd_size, initrd_start);
	return 1;
 fail:
	printk("Bad initrd argument.  Disabling initrd\n");
	initrd_start = 0;
	initrd_end = 0;
	return 1;
}

#endif

/*
 * prom_init is called just after the cpu type is determined, from init_arch()
 */
__init int prom_init(int argc, char **argv, char **envp, int *prom_vec)
{
	_machine_restart   = (void (*)(char *))cfe_linux_exit;
	_machine_halt      = cfe_linux_exit;
	_machine_power_off = cfe_linux_exit;

	/*
	 * This should go away.  Detect if we're booting
	 * straight from cfe without a loader.  If we
	 * are, then we've got a prom vector in a0.  Otherwise,
	 * argc (and argv and envp, for that matter) will be 0)
	 */
	if (argc < 0) {
		prom_vec = (int *)argc;
	}
	cfe_init((long)prom_vec);
	cfe_open_console();
	if (cfe_getenv("LINUX_CMDLINE", arcs_cmdline, CL_SIZE) < 0) {
		if (argc < 0) {
			/*
			 * It's OK for direct boot to not provide a
			 *  command line
			 */
			strcpy(arcs_cmdline, "root=/dev/ram0 ");
#ifdef CONFIG_SIBYTE_PTSWARM
			strcat(arcs_cmdline, "console=ttyS0,115200 ");
#endif
		} else {
			/* The loader should have set the command line */
			panic("LINUX_CMDLINE not defined in cfe.");
		}
	}

#ifdef CONFIG_BLK_DEV_INITRD
	{
		char *ptr;
		/* Need to find out early whether we've got an initrd.  So scan
		   the list looking now */
		for (ptr = arcs_cmdline; *ptr; ptr++) {
			while (*ptr == ' ') {
				ptr++;
			}
			if (!strncmp(ptr, "initrd=", 7)) {
				initrd_setup(ptr+7);
				break;
			} else {
				while (*ptr && (*ptr != ' ')) {
					ptr++;
				}
			}
		}
	}
#endif /* CONFIG_BLK_DEV_INITRD */

	/* Not sure this is needed, but it's the safe way. */
	arcs_cmdline[CL_SIZE-1] = 0;

	mips_machgroup = MACH_GROUP_SIBYTE;
	prom_meminit();

	return 0;
}

void prom_free_prom_memory(void)
{
	/* Not sure what I'm supposed to do here.  Nothing, I think */
}

int page_is_ram(unsigned long pagenr)
{
	phys_t addr = pagenr << PAGE_SHIFT;
	int i;
	for (i = 0; i < board_mem_region_count; i++) {
		if ((addr >= board_mem_region_addrs[i])
		    && (addr < (board_mem_region_addrs[i] + board_mem_region_sizes[i]))) {
			return 1;
		}
	}
	return 0;
}

#ifdef CONFIG_SIBYTE_CFE_CONSOLE

static void cfe_console_write(struct console *cons, const char *str,
                              unsigned int count)
{
	int i, last;

	for (i=0,last=0; i<count; i++) {
		if (!str[i])
			/* XXXKW can/should this ever happen? */
			panic("cfe_console_write with NULL");
		if (str[i] == '\n') {
			cfe_console_print(&str[last], i-last);
			cfe_console_print("\r", 1);
			last = i;
		}
	}
	if (last != count)
		cfe_console_print(&str[last], count-last);
}

static kdev_t cfe_console_device(struct console *c)
{
	return cfe_consdev;
}

static int cfe_console_setup(struct console *cons, char *str)
{
	char consdev[32];
	cfe_open_console();
	/* XXXKW think about interaction with 'console=' cmdline arg */
	/* If none of the console options are configured, the build will break. */
	if (cfe_getenv("BOOT_CONSOLE", consdev, 32) >= 0) {
#ifdef CONFIG_SIBYTE_SB1250_DUART
		if (!strcmp(consdev, "uart0")) {
			setleds("u0cn");
			cfe_consdev = MKDEV(TTY_MAJOR, SB1250_DUART_MINOR_BASE + 0);
#ifndef CONFIG_SIBYTE_SB1250_DUART_NO_PORT_1
		} else if (!strcmp(consdev, "uart1")) {
			setleds("u1cn");
			cfe_consdev = MKDEV(TTY_MAJOR, SB1250_DUART_MINOR_BASE + 1);
#endif
#endif
#ifdef CONFIG_VGA_CONSOLE
		} else if (!strcmp(consdev, "pcconsole0")) {
			setleds("pccn");
			cfe_consdev = MKDEV(TTY_MAJOR, 0);
#endif
#ifdef CONFIG_PROMICE
		} else if (!strcmp(consdev, "promice0")) {
			setleds("picn");
			cfe_consdev = MKDEV(TTY_MAJOR, SB1250_PROMICE_MINOR_BASE);
#endif
		} else
			return -ENODEV;
	}
	return 0;
}

static struct console sb1250_cfe_cons = {
	name:		"cfe",
	write:		cfe_console_write,
	device:		cfe_console_device,
	setup:		cfe_console_setup,
	flags:		CON_PRINTBUFFER,
	index:		-1,
};

void __init sb1250_cfe_console_init(void)
{
	register_console(&sb1250_cfe_cons);
}

#endif /* CONFIG_SIBYTE_CFE_CONSOLE */
