/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * env.c: ARCS environment variable routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/sgialib.h>

char * __init prom_getenv(char *name)
{
	return romvec->get_evar(name);
}

long __init prom_setenv(char *name, char *value)
{
	return romvec->set_evar(name, value);
}
