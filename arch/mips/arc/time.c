/*
 * time.c: Extracting time information from ARCS prom.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>
#include <asm/sgialib.h>

struct linux_tinfo * __init prom_gettinfo(void)
{
	return romvec->get_tinfo();
}

unsigned long __init prom_getrtime(void)
{
	return romvec->get_rtime();
}
