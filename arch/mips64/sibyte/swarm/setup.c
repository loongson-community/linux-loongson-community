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
#include <linux/config.h>
#include <linux/spinlock.h>
#include <linux/mc146818rtc.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/blk.h>
#include <asm/irq.h>
#include <asm/bootinfo.h>
#include <asm/addrspace.h>
#include <asm/sibyte/swarm.h>
#include <asm/cfe/cfe_api.h>
#include <asm/cfe/cfe_error.h>
#include <asm/cfe/cfe_xiocb.h>


extern struct rtc_ops swarm_rtc_ops;
extern int cfe_console_handle;


#ifndef CONFIG_SWARM_STANDALONE

long swarm_mem_region_addrs[CONFIG_SIBYTE_SWARM_MAX_MEM_REGIONS];
long swarm_mem_region_sizes[CONFIG_SIBYTE_SWARM_MAX_MEM_REGIONS];
unsigned int swarm_mem_region_count;

#endif

void swarm_setup(void)
{
	rtc_ops = &swarm_rtc_ops;

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
			if (!rd_flag) {
				 add_memory_region(addr, size, BOOT_MEM_RAM);
			}
			swarm_mem_region_addrs[swarm_mem_region_count] = addr;
			swarm_mem_region_sizes[swarm_mem_region_count] = size;
			swarm_mem_region_count++;
			if (swarm_mem_region_count == CONFIG_SIBYTE_SWARM_MAX_MEM_REGIONS) {
				while(1); /* Too many regions.  Need to configure more */
			}
		}
	}
	if (initrd_start) {
		add_memory_region(initrd_pstart, initrd_pend-initrd_pstart, BOOT_MEM_RESERVED);
	}
#endif  /* CONFIG_SWARM_STANDALONE */
}


#if 1
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

//__setup("initrd=", initrd_setup);

/* prom_init is called just after the cpu type is determined, from
   init_arch() */
int prom_init(long argc, char **argv, char **envp, int *prom_vec)
{
#ifndef CONFIG_SWARM_STANDALONE
//	int i;
//	int cmdline_idx = 0;
	char *ptr;
	
	/* 
	 * This should go away.  Detect if we're booting
	 * straight from cfe without a loader.  If we
	 * are, then we've got a prom vector in a0.  Otherwise,
	 * argc (and argv and envp, for that matter) will be 0) 
	 */
	if (argc < 0) {
		prom_vec = (int *)argc;
		strcpy(arcs_cmdline, "root=/dev/ram0 ");
	} else {
		
	}
	cfe_init((unsigned long)prom_vec);
	cfe_open_console();
	if (argc >= 0) {
		if (cfe_getenv("LINUX_CMDLINE", arcs_cmdline, CL_SIZE) < 0) {
			panic("LINUX_CMDLINE not defined in cfe.");
		}
	}

	/* Not sure this is needed, but it's the safe way. */
	arcs_cmdline[CL_SIZE-1] = 0;
	
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
	if (!*ptr) {
		printk("No initrd found\n");
	}

#else 
	strcpy(arcs_cmdline, "root=/dev/ram0 ");
#endif
	mips_machgroup = MACH_GROUP_SIBYTE;
#if 0
#ifndef CONFIG_SWARM_STANDALONE
	for (i = 0; (i < argc) && (cmdline_idx < CL_SIZE); i++) {
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
			if ((strlen(argv[i]) + cmdline_idx + 1) > CL_SIZE) {
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


void do_ibe(struct pt_regs *regs)
{
	printk("do_ibe called\n");
	while(1);
}

void do_dbe(struct pt_regs *regs)
{
	printk("Got dbe at 0x%lx\n", regs->cp0_epc);
	while(1);
}

extern asmlinkage void handle_ibe(void);
extern asmlinkage void handle_dbe(void);


void __init
bus_error_init(void)
{
	set_except_vector(6, handle_ibe);
	set_except_vector(7, handle_dbe);

	/* At this time nothing uses the DBE protection mechanism on the
	   Indy, so this here is needed to make the kernel link.  */
//	get_dbe(dummy, (int *)KSEG0);
}

void machine_restart(char *command)
{
	printk("machine_restart called\n");
	while(1);
}

void machine_halt(void)
{
	printk("machine_halt called\n");
	while(1);
}

void machine_power_off(void)
{
	printk("machine_power_off called\n");
	while(1);
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
static unsigned char default_led_msg[] = "And Justin said...LET THERE BE 64-BIT LINUX!      And he saw that it was mediocre.     But improving!    ";

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

void __add_wait_queue(wait_queue_head_t *head, wait_queue_t *new)
{
#if WAITQUEUE_DEBUG
	if (!head || !new)
		WQ_BUG();
	CHECK_MAGIC_WQHEAD(head);
	CHECK_MAGIC(new->__magic);
	if (!head->task_list.next || !head->task_list.prev)
		WQ_BUG();
#endif
//	printk("%i (%s) adding %i (%s) added to wait queue @ %p (%p %p %p %p)\n", new->task->pid, new->task->comm,
//	       current->pid, current->comm, head, new->task_list.next, new->task_list.prev, head->task_list.next, head->task_list.next->prev);
	list_add(&new->task_list, &head->task_list);
}

void __remove_wait_queue(wait_queue_head_t *head, wait_queue_t *old)
{
#if WAITQUEUE_DEBUG
	if (!old)
		WQ_BUG();
	CHECK_MAGIC(old->__magic);
#endif
//	printk("%i (%s) removing %i (%s) added to wait queue @ %p\n", old->task->pid, old->task->comm,
//	       current->pid, current->comm, head);
	list_del(&old->task_list);
}
