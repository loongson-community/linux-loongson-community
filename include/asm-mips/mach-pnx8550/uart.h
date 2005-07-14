#ifndef __PNX8550_UART_H
#define __PNX8550_UART_H

#include <asm/mach-pnx8550/int.h>

#define PNX8550_CONSOLE_PORT 1
#define PNX8550_SERIAL_PORT 0

#define PNX8550_UART_BASE	0xbbe4a000

#define PNX8550_UART_PORT0	(PNX8550_UART_BASE)
#define PNX8550_UART_PORT1	(PNX8550_UART_BASE + 0x1000)

#define PNX8550_UART_LCR(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0x000)
#define PNX8550_UART_MCR(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0x004)
#define PNX8550_UART_BAUD(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0x008)
#define PNX8550_UART_CFG(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0x00C)
#define PNX8550_UART_FIFO(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0x028)
#define PNX8550_UART_ISTAT(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0xFE0)
#define PNX8550_UART_IEN(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0xFE4)
#define PNX8550_UART_ICLR(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0xFE8)
#define PNX8550_UART_ISET(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0xFEC)
#define PNX8550_UART_PD(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0xFF4)
#define PNX8550_UART_MID(x)	*(volatile unsigned long *)(PNX8550_UART_BASE+(x*0x1000) + 0xFFC)

#define UART_GET_LCR(sport)	__raw_readl((sport)->port.membase + 0x000)
#define UART_GET_MCR(sport)	__raw_readl((sport)->port.membase + 0x004)
#define UART_GET_BAUD(sport)	__raw_readl((sport)->port.membase + 0x008)
#define UART_GET_CFG(sport)	__raw_readl((sport)->port.membase + 0x00c)
#define UART_GET_FIFO(sport)	__raw_readl((sport)->port.membase + 0x028)
#define UART_GET_ISTAT(sport)	__raw_readl((sport)->port.membase + 0xfe0)
#define UART_GET_IEN(sport)	__raw_readl((sport)->port.membase + 0xfe4)
#define UART_GET_ICLR(sport)	__raw_readl((sport)->port.membase + 0xfe8)
#define UART_GET_ISET(sport)	__raw_readl((sport)->port.membase + 0xfec)
#define UART_GET_PD(sport)	__raw_readl((sport)->port.membase + 0xff4)
#define UART_GET_MID(sport)	__raw_readl((sport)->port.membase + 0xffc)

#define UART_PUT_LCR(sport,v)	__raw_writel((v),(sport)->port.membase + 0x000)
#define UART_PUT_MCR(sport,v)	__raw_writel((v),(sport)->port.membase + 0x004)
#define UART_PUT_BAUD(sport,v)	__raw_writel((v),(sport)->port.membase + 0x008)
#define UART_PUT_CFG(sport,v)	__raw_writel((v),(sport)->port.membase + 0x00c)
#define UART_PUT_FIFO(sport,v)	__raw_writel((v),(sport)->port.membase + 0x028)
#define UART_PUT_ISTAT(sport,v)	__raw_writel((v),(sport)->port.membase + 0xfe0)
#define UART_PUT_IEN(sport,v)	__raw_writel((v),(sport)->port.membase + 0xfe4)
#define UART_PUT_ICLR(sport,v)	__raw_writel((v),(sport)->port.membase + 0xfe8)
#define UART_PUT_ISET(sport,v)	__raw_writel((v),(sport)->port.membase + 0xfec)
#define UART_PUT_PD(sport,v)	__raw_writel((v),(sport)->port.membase + 0xff4)
#define UART_PUT_MID(sport,v)	__raw_writel((v),(sport)->port.membase + 0xffc)

#define PNX8550_UART_LCR_TXBREAK	(1<<30)
#define PNX8550_UART_LCR_PAREVN		0x10000000
#define PNX8550_UART_LCR_PAREN		0x08000000
#define PNX8550_UART_LCR_2STOPB		0x04000000
#define PNX8550_UART_LCR_8BIT		0x01000000
#define PNX8550_UART_LCR_TX_RST		0x00040000
#define PNX8550_UART_LCR_RX_RST		0x00020000
#define PNX8550_UART_LCR_RX_NEXT	0x00010000

#define PNX8550_UART_MCR_SCR		0xFF000000
#define PNX8550_UART_MCR_DCD		0x00800000
#define PNX8550_UART_MCR_CTS		0x00100000
#define PNX8550_UART_MCR_LOOP		0x00000010
#define PNX8550_UART_MCR_RTS		0x00000002
#define PNX8550_UART_MCR_DTR		0x00000001

#define PNX8550_UART_INT_TX		0x00000080
#define PNX8550_UART_INT_EMPTY		0x00000040
#define PNX8550_UART_INT_RCVTO		0x00000020
#define PNX8550_UART_INT_RX		0x00000010
#define PNX8550_UART_INT_RXOVRN		0x00000008
#define PNX8550_UART_INT_FRERR		0x00000004
#define PNX8550_UART_INT_BREAK		0x00000002
#define PNX8550_UART_INT_PARITY		0x00000001
#define PNX8550_UART_INT_ALLRX		0x0000003F
#define PNX8550_UART_INT_ALLTX		0x000000C0

#define PNX8550_UART_FIFO_TXFIFO	0x001F0000
#define PNX8550_UART_FIFO_TXFIFO_STA	(0x1f<<16)
#define PNX8550_UART_FIFO_RXBRK		0x00008000
#define PNX8550_UART_FIFO_RXFE		0x00004000
#define PNX8550_UART_FIFO_RXPAR		0x00002000
#define PNX8550_UART_FIFO_RXFIFO	0x00001F00
#define PNX8550_UART_FIFO_RBRTHR	0x000000FF

#define PNX8550_UART_INT(x)		(PNX8550_INT_GIC_MIN+19+x)
#define IRQ_TO_UART(x)			(x-PNX8550_INT_GIC_MIN-19)

#endif
