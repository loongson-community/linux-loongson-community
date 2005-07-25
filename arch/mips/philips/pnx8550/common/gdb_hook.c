/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * This is the interface to the remote debugger stub.
 *
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/serial.h>
#include <linux/serialP.h>
#include <linux/serial_reg.h>

#include <asm/serial.h>
#include <asm/io.h>

#include <asm/mach-pnx8550/uart.h>

static struct serial_state rs_table[RS_TABLE_SIZE] = {
	SERIAL_PORT_DFNS	/* Defined in serial.h */
};

static struct async_struct kdb_port_info = {
	.port	= PNX8550_CONSOLE_PORT,
};

static __inline__ unsigned int serial_in(struct async_struct *info, int offset)
{
	return inb(info->port + offset);
}

static __inline__ void serial_out(struct async_struct *info, int offset,
				  int value)
{
	outb(value, info->port + offset);
}

void rs_kgdb_hook(int tty_no)
{
	struct serial_state *ser = &rs_table[tty_no];

	kdb_port_info.state = ser;
	kdb_port_info.magic = SERIAL_MAGIC;
	kdb_port_info.port  = tty_no;
	kdb_port_info.flags = ser->flags;

	/*
	 * Clear all interrupts
	 */
	/* Clear all the transmitter FIFO counters (pointer and status) */
	PNX8550_UART_LCR(tty_no) |= PNX8550_UART_LCR_TX_RST;
	/* Clear all the receiver FIFO counters (pointer and status) */
	PNX8550_UART_LCR(tty_no) |= PNX8550_UART_LCR_RX_RST;
	/* Clear all interrupts */
	PNX8550_UART_ICLR(tty_no) = PNX8550_UART_INT_ALLRX |
		PNX8550_UART_INT_ALLTX;

	/*
	 * Now, initialize the UART
	 */
	PNX8550_UART_LCR(tty_no) = PNX8550_UART_LCR_8BIT;
	PNX8550_UART_BAUD(tty_no) = 5; // 38400 Baud
}

int putDebugChar(char c)
{
	/* Wait until FIFO not full */
	while (((PNX8550_UART_FIFO(kdb_port_info.port) & PNX8550_UART_FIFO_TXFIFO) >> 16) >= 16)
		;
	/* Send one char */
	PNX8550_UART_FIFO(kdb_port_info.port) = c;

	return 1;
}

char getDebugChar(void)
{
	char ch;

	/* Wait until there is a char in the FIFO */
	while (!((PNX8550_UART_FIFO(kdb_port_info.port) &
					PNX8550_UART_FIFO_RXFIFO) >> 8))
		;
	/* Read one char */
	ch = PNX8550_UART_FIFO(kdb_port_info.port) & PNX8550_UART_FIFO_RBRTHR;
	/* Advance the RX FIFO read pointer */
	PNX8550_UART_LCR(kdb_port_info.port) |= PNX8550_UART_LCR_RX_NEXT;
	return (ch);
}

void rs_disable_debug_interrupts(void)
{
	PNX8550_UART_IEN(kdb_port_info.port) = 0; /* Disable all interrupts */
}

void rs_enable_debug_interrupts(void)
{
	/* Clear all the transmitter FIFO counters (pointer and status) */
	PNX8550_UART_LCR(kdb_port_info.port) |= PNX8550_UART_LCR_TX_RST;
	/* Clear all the receiver FIFO counters (pointer and status) */
	PNX8550_UART_LCR(kdb_port_info.port) |= PNX8550_UART_LCR_RX_RST;
	/* Clear all interrupts */
	PNX8550_UART_ICLR(kdb_port_info.port) = PNX8550_UART_INT_ALLRX |
		PNX8550_UART_INT_ALLTX;
	PNX8550_UART_IEN(kdb_port_info.port)  = PNX8550_UART_INT_ALLRX;	/* Enable RX interrupts */
}
