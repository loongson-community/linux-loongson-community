/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Extracting time information from ARCS prom.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>
#include <asm/sgialib.h>

struct __init linux_tinfo *prom_gettinfo(void)
{
	return romvec->get_tinfo();
}

unsigned __init long prom_getrtime(void)
{
	return romvec->get_rtime();
}
