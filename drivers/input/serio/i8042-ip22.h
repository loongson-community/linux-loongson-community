#ifndef _I8042_IP22_H
#define _I8042_IP22_H

#include <asm/sgihpc.h>

#define sgi_kh ((struct hpc_keyb *) &(hpc3mregs->kbdmouse0))

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

#define I8042_KBD_IRQ SGI_KEYBD_IRQ
#define I8042_AUX_IRQ SGI_KEYBD_IRQ

static inline int i8042_read_data(void)
{
	return sgi_kh->data;
}

static inline int i8042_read_status(void)
{
	return jazz_kh->command;
}

static inline void i8042_write_data(int val)
{
	int status;

	do {
		status = sgi_kh->command;
	} while (status & KBD_STAT_IBF);
	sgi_kh->data = val;
}

static inline void i8042_write_command(int val)
{
	int status;

	do {
		status = sgi_kh->command;
	} while (status & KBD_STAT_IBF);
	sgi_kh->command = val;
}

static inline int i8042_platform_init(void)
{
#if 0
	/* XXX sgi_kh is a virtual address */
	if (!request_mem_region(sgi_kh, sizeof(struct hpc_keyb), "i8042"))
		return 0;

	return 1;
#else
	return 0;
#endif
}

static inline void i8042_platform_exit(void)
{
#if 0
	release_mem_region(JAZZ_KEYBOARD_ADDRESS, sizeof(struct hpc_keyb));
#endif
}

#endif /* _I8042_IP22_H */
