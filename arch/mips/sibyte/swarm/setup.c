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

/*
 * Setup code for the SWARM board 
 */

#include <linux/spinlock.h>
#include <linux/mc146818rtc.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/blk.h>
#include <asm/irq.h>
#include <asm/bootinfo.h>
#include <asm/addrspace.h>
#include <asm/sibyte/swarm.h>
#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/sb1250_defs.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/reboot.h>
#include <linux/ide.h>
#include "cfe_xiocb.h"
#include "cfe_api.h"
#include "cfe_error.h"

extern struct rtc_ops swarm_rtc_ops;
extern int cfe_console_handle;

#ifdef CONFIG_BLK_DEV_IDE_SWARM
struct ide_ops *ide_ops;
#endif


#ifdef CONFIG_L3DEMO
extern void *l3info;
#endif

/* Max ram addressable in 32-bit segments */
#define MAX_RAM_SIZE (1024*1024*256)

#ifndef CONFIG_SWARM_STANDALONE

long swarm_mem_region_addrs[CONFIG_SIBYTE_SWARM_MAX_MEM_REGIONS];
long swarm_mem_region_sizes[CONFIG_SIBYTE_SWARM_MAX_MEM_REGIONS];
unsigned int swarm_mem_region_count;

#endif


#ifdef CONFIG_BLK_DEV_IDE_SWARM
static int swarm_ide_default_irq(ide_ioreg_t base)
{
        return 0;
}

static ide_ioreg_t swarm_ide_default_io_base(int index)
{
        return 0;
}

static void swarm_ide_init_hwif_ports (hw_regs_t *hw, ide_ioreg_t data_port,
                                     ide_ioreg_t ctrl_port, int *irq)
{
	ide_ioreg_t reg = data_port;
	int i;

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = reg;
		reg += 1;
	}
	if (ctrl_port) {
		hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
	} else {
		hw->io_ports[IDE_CONTROL_OFFSET] = hw->io_ports[IDE_DATA_OFFSET] + 0x206;
	}
	if (irq != NULL)
		*irq = 0;
	hw->io_ports[IDE_IRQ_OFFSET] = 0;
}

static int swarm_ide_request_irq(unsigned int irq,
                                void (*handler)(int,void *, struct pt_regs *),
                                unsigned long flags, const char *device,
                                void *dev_id)
{
	return request_irq(irq, handler, flags, device, dev_id);
}			

static void swarm_ide_free_irq(unsigned int irq, void *dev_id)
{
	free_irq(irq, dev_id);
}

static int swarm_ide_check_region(ide_ioreg_t from, unsigned int extent)
{
    /* Note: "check_region" and friends do conflict management on ISA I/O
       space.  Our disk is not in that space, so this check won't work */
    /* return check_region(from, extent); */
    return 0;
}

static void swarm_ide_request_region(ide_ioreg_t from, unsigned int extent,
                                    const char *name)
{
    /* request_region(from, extent, name); */
}

static void swarm_ide_release_region(ide_ioreg_t from, unsigned int extent)
{
    /* release_region(from, extent); */
}

struct ide_ops swarm_ide_ops = {
	&swarm_ide_default_irq,
	&swarm_ide_default_io_base,
	&swarm_ide_init_hwif_ports,
	&swarm_ide_request_irq,
	&swarm_ide_free_irq,
	&swarm_ide_check_region,
	&swarm_ide_request_region,
	&swarm_ide_release_region
};
#endif


static void stop_this_cpu(void *dummy)
{
	printk("Cpu %d stopping\n", smp_processor_id());
	for (;;);
}

static void smp_cpu0_exit(void)
{
	printk("cpu %d poked\n", smp_processor_id());
	/* XXXKW we are in the mailbox handler... */
	__asm__(".set push\n\t"
		".set mips32\n\t"
		"la $2, swarm_linux_exit\n\t"
		"mtc0 $2, $24\n\t"
		"eret\n\t"
		".set pop"
		::: "$2");
}

extern void (*smp_cpu0_finalize)(void);

static void swarm_linux_exit(void)
{
	if (smp_processor_id()) {
		/* Make cpu 0 do the swarm_linux_exit */
		/* XXXKW this isn't quite there yet */
		smp_cpu0_finalize = smp_cpu0_exit;
		stop_this_cpu(NULL);
	} else {
		printk("swarm_linux_exit called...passing control back to CFE\n");
		cfe_exit(1, 0);
		printk("cfe_exit returned??\n");
		while(1);
	}
}


extern unsigned long mips_io_port_base;

void __init swarm_setup(void)
{
	extern int panic_timeout;
	rtc_ops = &swarm_rtc_ops;
	panic_timeout = 5;  /* For debug.  This should probably be raised later */
	_machine_restart   = (void (*)(char *))swarm_linux_exit;
	_machine_halt      = swarm_linux_exit;
	_machine_power_off = swarm_linux_exit;

#ifdef CONFIG_L3DEMO
	if (l3info != NULL) {
		printk("\n");
	}
#endif
	printk("This kernel optimized for "
#ifdef CONFIG_SIMULATION
	       "simulation"
#else
	       "board"
#endif
	       " runs\n");

#ifdef CONFIG_BLK_DEV_IDE_SWARM
        ide_ops = &swarm_ide_ops;
#endif

}  

/* This is the kernel command line.  Actually, it's 
   copied, eventually, to command_line, and looks to be
   quite redundant.  But not something to fix just now */
extern char arcs_cmdline[];

#ifdef CONFIG_EMBEDDED_RAMDISK
/* These are symbols defined by the ramdisk linker script */
extern unsigned char __rd_start;
extern unsigned char __rd_end;  
#endif

static void prom_meminit(void)
{
#ifndef CONFIG_SWARM_STANDALONE
	unsigned long addr, size;
	long type;
	unsigned int idx;
	int rd_flag;
#endif
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
		setleds("INRD");
		panic("initrd out of addressable memory\n");
	}
       
#endif /* INITRD */
		
#ifdef CONFIG_SWARM_STANDALONE
	/* Standalone compile, memory is hardcoded */

	if (initrd_start) {
		add_memory_region(0, initrd_pstart, BOOT_MEM_RAM);
		add_memory_region(initrd_pstart, initrd_pend-initrd_pstart, BOOT_MEM_RESERVED);
		add_memory_region(initrd_pend, (CONFIG_SIBYTE_SWARM_RAM_SIZE * 1024 * 1024)-initrd_pend, BOOT_MEM_RAM);
	} else {
		add_memory_region(0, CONFIG_SIBYTE_SWARM_RAM_SIZE * 1024 * 1024, BOOT_MEM_RAM);
	}
	
#else  /* Run with the firmware */
	for (idx = 0; cfe_enummem(idx, &addr, &size, &type) != CFE_ERR_NOMORE; idx++) {
		rd_flag = 0;
		if (type == CFE_MI_AVAILABLE) {
			/* See if this block contains (any portion of) the ramdisk */
#ifdef CONFIG_BLK_DEV_INITRD
			if (initrd_start) {
				if ((initrd_pstart > addr) && (initrd_pstart < (addr + size))) {
					add_memory_region(addr, initrd_pstart-addr, BOOT_MEM_RAM);
					rd_flag = 1;
				} 
				if ((initrd_pend > addr) && (initrd_pend < (addr + size))) {
					add_memory_region(initrd_pend, (addr + size)-initrd_pend, BOOT_MEM_RAM);
					rd_flag = 1;
				}
			} 
#endif
			if (!rd_flag) {
				if (addr < MAX_RAM_SIZE) {
					if (size > MAX_RAM_SIZE) {
						size = MAX_RAM_SIZE - addr;
					}
					add_memory_region(addr, size, BOOT_MEM_RAM);
				}
			}
			swarm_mem_region_addrs[swarm_mem_region_count] = addr;
			swarm_mem_region_sizes[swarm_mem_region_count] = size;
			swarm_mem_region_count++;
			if (swarm_mem_region_count == CONFIG_SIBYTE_SWARM_MAX_MEM_REGIONS) {
				while(1); /* Too many regions.  Need to configure more */
			}
		}
	}
#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start) {
		add_memory_region(initrd_pstart, initrd_pend-initrd_pstart, BOOT_MEM_RESERVED);
	}
#endif
#endif  /* CONFIG_SWARM_STANDALONE */
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
		goto fail;
	}
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

/* prom_init is called just after the cpu type is determined, from
   init_arch() */
int prom_init(int argc, char **argv, char **envp, int *prom_vec)
{

#ifdef CONFIG_SWARM_STANDALONE
        strcpy(arcs_cmdline, "root=/dev/ram0 ");
#else	
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
        if (cfe_getenv("LINUX_CMDLINE", arcs_cmdline, COMMAND_LINE_SIZE) < 0) {
                if (argc < 0) {
                        /* It's OK for direct boot to not provide a
                           command line */
                        strcpy(arcs_cmdline, "root=/dev/ram0 ");
                } else {
                        /* The loader should have set the command
                           line */
                        setleds("CMDL");
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
#endif /* CONFIG_SWARM_STANDALONE */

	/* Not sure this is needed, but it's the safe way. */
	arcs_cmdline[COMMAND_LINE_SIZE-1] = 0;

	mips_machgroup = MACH_GROUP_SIBYTE;
#if 0
#ifndef CONFIG_SWARM_STANDALONE
	for (i = 0; (i < argc) && (cmdline_idx < COMMAND_LINE_SIZE); i++) {
		if (!strncmp(argv[i], "initrd=", 7)) {
			/* Handle initrd argument early; we need to know
			   about them for the memory map */
			unsigned char *size_str = argv[i] + 7;
			unsigned char *loc_str = size_str;
			unsigned long size, loc;
			while (*loc_str && (*loc_str != '@')) {
				loc_str++;
			}
			if (!*loc_str) {
				printk("Ignoring malformed initrd argument: %s\n", argv[i]);
				continue;
			}
			*loc_str = '\0';
			loc_str++;
			size = simple_strtoul(size_str, NULL, 16);
			loc = simple_strtoul(loc_str, NULL, 16);
			if (size && loc) {
				printk("Found initrd argument: 0x%lx@0x%lx\n", size, loc);
				initrd_start = loc;
				initrd_end = (loc + size + PAGE_SIZE - 1) & PAGE_MASK;
			} 
		} else {
			if ((strlen(argv[i]) + cmdline_idx + 1) > COMMAND_LINE_SIZE) {
				printk("Command line too long.  Cut these:\n");
				for (; i < argc; i++) {
					printk("  %s\n", argv[i]);
				}
			} else {
				strcpy(arcs_cmdline + cmdline_idx, argv[i]);
			}
		}
	}
#endif
#endif
	prom_meminit();

	return 0;
}

/* Not sure what I'm supposed to do here.  Nothing, I think */
void prom_free_prom_memory(void)
{

} 

static void setled(unsigned int index, char c) 
{
	volatile unsigned char *led_ptr = (unsigned char *)(IO_SPACE_BASE | LED_BASE_ADDR);
	if (index < 4) {
		led_ptr[(3-index)<<3] = c;
	}
}

void setleds(char *str)
{
	int i;
	for (i = 0; i < 4; i++) {
		if (!str[i]) {
			for (; i < 4; i++) { 
				setled(' ', str[i]);
			}
		} else {
			setled(i, str[i]);
		}
	}
}

#include <linux/timer.h>

static struct timer_list led_timer;
/*
#ifdef CONFIG_SMP
static unsigned char led_msg[] = "CSWARM...now in glorious SMP!       ";
#else
static unsigned char led_msg[] = "CSWARM Lives!!!   ";
#endif
*/
static unsigned char default_led_msg[] = "Today: the CSWARM.  Tomorrow: the WORLD!!!!           ";

static unsigned char *led_msg = default_led_msg;
static unsigned char *led_msg_ptr = default_led_msg;


void set_led_msg(char *new_msg)
{
	led_msg = new_msg;
	led_msg_ptr = new_msg;
	setleds("    ");
}

static void move_leds(unsigned long arg) 
{
	int i;
	unsigned char *tmp = led_msg_ptr;
	for (i = 0; i < 4; i++) {
		setled(i, *tmp);
		tmp++;
		if (!*tmp) { 
			tmp = led_msg; 
		}
	}
	led_msg_ptr++;
	if (!*led_msg_ptr) { 
 		led_msg_ptr = led_msg; 
	}
	del_timer(&led_timer);
	led_timer.expires = jiffies + (HZ/8);
	add_timer(&led_timer);
}

void hack_leds(void) 
{
	init_timer(&led_timer);
	led_timer.expires = jiffies + (HZ/8);
	led_timer.data = 0;
	led_timer.function = move_leds;
	add_timer(&led_timer);
}
