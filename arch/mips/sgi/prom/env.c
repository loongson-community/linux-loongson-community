/* $Id: env.c,v 1.2 1996/06/08 04:48:41 dm Exp $
 * env.c: ARCS environment variable routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */

#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/sgialib.h>

char *prom_getenv(char *name)
{
	return romvec->get_evar(name);
}

long prom_setenv(char *name, char *value)
{
	return romvec->set_evar(name, value);
}
