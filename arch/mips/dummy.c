/*
 * This file handles Systemcalls not available for all CPUs.
 *
 * Written by Ralf Baechle,
 * Copyright (C) 1994 by Waldorf GMBH
 */

unsigned long paging_init(unsigned long start_mem, unsigned long end_mem)
{
	printk("clone_page_tables\n");
	return start_mem;
}

void fake_keyboard_interrupt(void)
{
/*	printk("fake_keyboard_interrupt\n"); */
}
