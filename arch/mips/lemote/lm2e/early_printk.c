/*  early printk support
 *
 *  Copyright (c) 2009 Philippe Vachon <philippe@cowpig.ca>
 *  Copyright (c) 2009 Lemote Inc. & Insititute of Computing Technology
 *  Author: Wu Zhangjin, wuzj@lemote.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/io.h>
#include <linux/init.h>
#include <linux/serial_reg.h>

#include <asm/mips-boards/bonito64.h>

#define UART_BASE (BONITO_PCIIO_BASE + 0x3f8)

#define PORT(base, offset) (u8 *)(base + offset)

static inline unsigned int serial_in(unsigned char *base, int offset)
{
	return readb(PORT(base, offset));
}

static inline void serial_out(unsigned char *base, int offset, int value)
{
	writeb(value, PORT(base, offset));
}

void prom_putchar(char c)
{
	unsigned char *uart_base =
		(unsigned char *) ioremap_nocache(UART_BASE, 8);

	while ((serial_in(uart_base, UART_LSR) & UART_LSR_THRE) == 0)
		;

	serial_out(uart_base, UART_TX, c);
}
