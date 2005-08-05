#ifndef __IP3106_UART_H
#define __IP3106_UART_H

#include <int.h>

/* early macros for kgdb use. fixme: clean this up */

#define UART_BASE		0xbbe4a000	/* PNX8550 */

#define PNX8550_UART_PORT0	(UART_BASE)
#define PNX8550_UART_PORT1	(UART_BASE + 0x1000)

#define IP3106_UART_LCR(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0x000)
#define IP3106_UART_MCR(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0x004)
#define IP3106_UART_BAUD(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0x008)
#define IP3106_UART_CFG(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0x00C)
#define IP3106_UART_FIFO(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0x028)
#define IP3106_UART_ISTAT(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0xFE0)
#define IP3106_UART_IEN(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0xFE4)
#define IP3106_UART_ICLR(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0xFE8)
#define IP3106_UART_ISET(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0xFEC)
#define IP3106_UART_PD(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0xFF4)
#define IP3106_UART_MID(x)	*(volatile u32 *)(UART_BASE+(x*0x1000) + 0xFFC)

#define PNX8550_UART_INT(x)		(PNX8550_INT_GIC_MIN+19+x)
#define IRQ_TO_UART(x)			(x-PNX8550_INT_GIC_MIN-19)

#endif
