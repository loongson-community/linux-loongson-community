/*
 * Copyright (C) 2009 Lemote Inc. & Insititute of Computing Technology
 * Author: Wu Zhangjin, wuzj@lemote.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/bootmem.h>

#include <asm/bootinfo.h>

#include <loongson.h>

#if defined(CONFIG_CPU_LOONGSON2F) && defined(CONFIG_64BIT)
unsigned long _loongson_addrwincfg_base;

/* Loongson CPU address windows config space base address */
static inline void set_loongson_addrwincfg_base(unsigned long base)
{
	*(unsigned long *)&_loongson_addrwincfg_base = base;
	barrier();
}
#endif

void __init prom_init(void)
{
    /* init base address of io space */
	set_io_port_base((unsigned long)
		ioremap(LOONGSON_PCIIO_BASE, LOONGSON_PCIIO_SIZE));

#if defined(CONFIG_CPU_LOONGSON2F) && defined(CONFIG_64BIT)
	set_loongson_addrwincfg_base((unsigned long)
				     ioremap(LOONGSON_ADDRWINCFG_BASE,
					     LOONGSON_ADDRWINCFG_SIZE));
#endif

	prom_init_cmdline();
	prom_init_env();
	prom_init_memory();
}

void __init prom_free_prom_memory(void)
{
}
