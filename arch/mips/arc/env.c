/*
 * env.c: ARCS environment variable routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/sgialib.h>

PCHAR __init
ArcGetEnvironmentVariable(CHAR *name)
{
	return romvec->get_evar(name);
}

LONG __init
ArcSetEnvironmentVariable(PCHAR name, PCHAR value)
{
	return romvec->set_evar(name, value);
}
