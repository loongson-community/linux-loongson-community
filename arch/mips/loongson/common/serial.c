/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 Ralf Baechle (ralf@linux-mips.org)
 *
 * Copyright (C) 2009 Lemote, Inc. & Institute of Computing Technology
 * Author: yanhua (yanhua@lemote.com)
 * Author: Wu Zhangjin (wuzj@lemote.com)
 */

#include <linux/io.h>
#include <linux/init.h>
#include <linux/serial_8250.h>

#include <loongson.h>
#include <machine.h>

#define PORT(int, base_baud, io_type)				\
{								\
	.irq		= int,					\
	.uartclk	= base_baud,				\
	.iotype		= io_type,				\
	.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,	\
	.regshift	= 0,					\
}

static struct plat_serial8250_port uart8250_data[] = {
	PORT(LOONGSON_UART_IRQ, LOONGSON_UART_BAUD, LOONGSON_UART_IOTYPE),
	{},
};

static struct platform_device uart8250_device = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = uart8250_data,
		},
};

static inline void uart8250_init(void)
{
#if (LOONGSON_UART_IOTYPE == UPIO_MEM)
		uart8250_data[0].membase =
		    ioremap_nocache(LOONGSON_UART_BASE, 8);
#elif (LOONGSON_UART_IOTYPE == UPIO_PORT)
		uart8250_data[0].iobase =
				LOONGSON_UART_BASE - LOONGSON_PCIIO_BASE;
		uart8250_data[0].irq -= MIPS_CPU_IRQ_BASE;
#else
#warning currently, no such iotype of uart used in loongson-based machines

#endif
}

static int __init serial_init(void)
{
	uart8250_init();

	platform_device_register(&uart8250_device);

	return 0;
}

device_initcall(serial_init);
