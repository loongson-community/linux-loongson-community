/* $Id: time.c,v 1.1 1996/06/08 04:47:23 dm Exp $
 * time.c: Extracting time information from ARCS prom.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */

#include <asm/sgialib.h>

struct linux_tinfo *prom_gettinfo(void)
{
	return romvec->get_tinfo();
}

unsigned long prom_getrtime(void)
{
	return romvec->get_rtime();
}
