/* $Id: sgicons.c,v 1.6 1996/07/29 11:10:22 dm Exp $
 * sgicons.c: Setting up and registering console I/O on the SGI.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/kernel.h>

extern void newport_init(void);
extern unsigned long video_mem_base, video_screen_size, video_mem_term;

unsigned long con_type_init(unsigned long start_mem, const char **name)
{
	extern int serial_console;

	*name = "NEWPORT";

	if(!serial_console) {
		printk("Video screen size is %08lx at %08lx\n",
		       video_screen_size, start_mem);
		video_mem_base = start_mem;
		start_mem += (video_screen_size * 2);
		video_mem_term = start_mem;

		newport_init();
	}

	return start_mem;
}
