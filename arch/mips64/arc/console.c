/* $Id: console.c,v 1.3 1999/10/19 20:51:44 ralf Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * ARC console code.
 *
 * Copyright (C) 1996 David S. Miller (dm@sgi.com)
 */
#include <linux/config.h>
#include <linux/init.h>
#include <asm/sgialib.h>

#ifdef CONFIG_SGI_IP27

#include <asm/sn/addrs.h>
#include <asm/sn/sn0/hub.h>
#include <asm/sn/klconfig.h>
#include <asm/ioc3.h>
#include <asm/sn/sn_private.h>

void prom_putchar(char c)
{
	struct ioc3 *ioc3;
	struct ioc3_uartregs *uart;

	ioc3 = (struct ioc3 *)KL_CONFIG_CH_CONS_INFO(master_nasid)->memory_base;
	uart = &ioc3->sregs.uarta;

	while ((uart->iu_lsr & 0x20) == 0);
	uart->iu_thr = c;
}

#else /* CONFIG_SGI_IP27 */

void prom_putchar(char c)
{
	ULONG cnt;
	CHAR it = c;

	ArcWrite(1, &it, 1, &cnt);
}

#endif /* CONFIG_SGI_IP27 */

char __init prom_getchar(void)
{
	ULONG cnt;
	CHAR c;

	ArcRead(0, &c, 1, &cnt);

	return c;
}
