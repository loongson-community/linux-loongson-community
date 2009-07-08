/*
 * Copyright (C) 2009 Lemote, Inc. & Institute of Computing Technology
 * Author: Wu Zhangjin <wuzj@lemote.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_MACH_LOONGSON_MACHINE_H
#define __ASM_MACH_LOONGSON_MACHINE_H

#ifdef CONFIG_LEMOTE_FULOONG2E

#define LOONGSON_UART_IRQ       (MIPS_CPU_IRQ_BASE + 4)
#define LOONGSON_UART_BASE (LOONGSON_PCIIO_BASE + 0x3f8)
#define LOONGSON_UART_BAUD		1843200
#define LOONGSON_UART_IOTYPE		UPIO_PORT
#define LOONGSON_MACHTYPE MACH_LEMOTE_FL2E

#elif defined(CONFIG_LEMOTE_FULOONG2F)

#define LOONGSON_UART_IRQ	(MIPS_CPU_IRQ_BASE + 3)
#define LOONGSON_UART_BASE (LOONGSON_PCIIO_BASE + 0x2f8)
#define LOONGSON_UART_BAUD		1843200
#define LOONGSON_UART_IOTYPE		UPIO_PORT
#define LOONGSON_MACHTYPE MACH_LEMOTE_FL2F

#elif defined(CONFIG_LEMOTE_YEELOONG2F)

/* yeeloong use the CPU serial port of loongson2f */
#define LOONGSON_UART_IRQ	(MIPS_CPU_IRQ_BASE + 3)
#define LOONGSON_UART_BASE	(LOONGSON_LIO1_BASE + 0x3f8)
#define LOONGSON_UART_BAUD	3686400
#define LOONGSON_UART_IOTYPE	UPIO_MEM
#define LOONGSON_MACHTYPE MACH_LEMOTE_YL2F89

#endif

#endif /* __ASM_MACH_LOONGSON_MACHINE_H */
