/*
 * console.c: SGI arcs console code.
 *
 * Copyright (C) 1996 David S. Miller (dm@sgi.com)
 * Compability with board caches, Ulf Carlsson
 *
 * $Id: console.c,v 1.3 1999/10/09 00:00:57 ralf Exp $
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <asm/sgialib.h>
#include <asm/bcache.h>

/* The romvec is not compatible with board caches.  Thus we disable it during
 * romvec action.  Since r4xx0.c is always compiled and linked with your kernel,
 * this shouldn't cause any harm regardless what MIPS processor you have.
 *
 * The romvec write and read functions seem to interfere with the serial lines
 * in some way. You should be careful with them.
 */

#ifdef CONFIG_ARC_CONSOLE
void prom_putchar(char c)
#else
void __init prom_putchar(char c)
#endif
{
	long cnt;
	char it = c;

	bc_disable();
	romvec->write(1, &it, 1, &cnt);
	bc_enable();
}

#ifdef CONFIG_ARC_CONSOLE
char prom_getchar(void)
#else
char __init prom_getchar(void)
#endif
{
	long cnt;
	char c;

	bc_disable();
	romvec->read(0, &c, 1, &cnt);
	bc_enable();
	return c;
}

static char ppbuf[1024];

#ifdef CONFIG_ARC_CONSOLE
void prom_printf(char *fmt, ...)
#else
void __init prom_printf(char *fmt, ...)
#endif
{
	va_list args;
	char ch, *bptr;
	int i;

	va_start(args, fmt);
	i = vsprintf(ppbuf, fmt, args);

	bptr = ppbuf;

	while((ch = *(bptr++)) != 0) {
		if(ch == '\n')
			prom_putchar('\r');

		prom_putchar(ch);
	}
	va_end(args);
	return;
}
