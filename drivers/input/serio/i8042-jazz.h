#ifndef _I8042_JAZZ_H
#define _I8042_JAZZ_H

#include <asm/jazz.h>

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by 
 * the Free Software Foundation.
 */

/*
 * Names.
 */

#define I8042_KBD_PHYS_DESC "isa0060/serio0"
#define I8042_AUX_PHYS_DESC "isa0060/serio1"

/*
 * IRQs.
 */

#define I8042_KBD_IRQ JAZZ_KEYBOARD_IRQ
#define I8042_AUX_IRQ JAZZ_MOUSE_IRQ

static inline int i8042_read_data(void)
{
	return jazz_kh->data;
}

static inline int i8042_read_status(void)
{
	return jazz_kh->command;
}

static inline void i8042_write_data(int val)
{
	int status;

	do {
		status = jazz_kh->command;
	} while (status & KBD_STAT_IBF);
	jazz_kh->data = val;
}

static inline void i8042_write_command(int val)
{
	int status;

	do {
		status = jazz_kh->command;
	} while (status & KBD_STAT_IBF);
	jazz_kh->command = val;
}

static inline int i8042_platform_init(void)
{
#if 0
	/* XXX JAZZ_KEYBOARD_ADDRESS is a virtual address */
	if (!request_mem_region(JAZZ_KEYBOARD_ADDRESS, 2, "i8042"))
		return 0;

	return 1;
#else
	return 0;
#endif
}

static inline void i8042_platform_exit(void)
{
#if 0
	release_mem_region(JAZZ_KEYBOARD_ADDRESS, 2);
#endif
}

#endif /* _I8042_JAZZ_H */
