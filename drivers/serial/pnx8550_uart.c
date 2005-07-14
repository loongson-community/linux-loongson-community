/*
 * Initially based on linux-2.4.20_mvl31-pnx8550/drivers/char/serial_pnx8550.c
 * Complete rewrite to drivers/serial/pnx8550_uart.c by 
 * Embedded Alley Solutions, source@embeddedalley.com.
 */
/*
 *  drivers/char/serial_pnx8550.c
 *
 *  Author: Per Hallsmark per.hallsmark@mvista.com
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *  
 *  Serial driver for PNX8550
 */

/*
 *  linux/drivers/serial/pnx8550_uart.c
 *
 *  Driver for PNX8550 serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id: pnx8550.c,v 1.50 2002/07/29 14:41:04 rmk Exp $
 *
 */
#include <linux/config.h>

#if defined(CONFIG_SERIAL_PNX8550_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>

#include <asm/io.h>
#include <asm/irq.h>

#include <asm/mach-pnx8550/uart.h>

/* We've been assigned a range on the "Low-density serial ports" major */
#define SERIAL_PNX8550_MAJOR	204
#define MINOR_START		5

#define NR_PORTS		2

#define PNX8550_ISR_PASS_LIMIT	256

/*
 * Convert from ignore_status_mask or read_status_mask to FIFO
 * and interrupt status bits
 */
#define SM_TO_FIFO(x)	((x) >> 10)
#define SM_TO_ISTAT(x)	((x) & 0x000001ff)
#define FIFO_TO_SM(x)	((x) << 10)
#define ISTAT_TO_SM(x)	((x) & 0x000001ff)

/*
 * This is the size of our serial port register set.
 */
#define UART_PORT_SIZE	0x1000

/*
 * This determines how often we check the modem status signals
 * for any change.  They generally aren't connected to an IRQ
 * so we have to poll them.  We also check immediately before
 * filling the TX fifo incase CTS has been dropped.
 */
#define MCTRL_TIMEOUT	(250*HZ/1000)

struct pnx8550_port {
	struct uart_port	port;
	struct timer_list	timer;
	unsigned int		old_status;
};

/*
 * Handle any change of modem status signal since we were last called.
 */
static void pnx8550_mctrl_check(struct pnx8550_port *sport)
{
	unsigned int status, changed;

	status = sport->port.ops->get_mctrl(&sport->port);
	changed = status ^ sport->old_status;

	if (changed == 0)
		return;

	sport->old_status = status;

	if (changed & TIOCM_RI)
		sport->port.icount.rng++;
	if (changed & TIOCM_DSR)
		sport->port.icount.dsr++;
	if (changed & TIOCM_CAR)
		uart_handle_dcd_change(&sport->port, status & TIOCM_CAR);
	if (changed & TIOCM_CTS)
		uart_handle_cts_change(&sport->port, status & TIOCM_CTS);

	wake_up_interruptible(&sport->port.info->delta_msr_wait);
}

/*
 * This is our per-port timeout handler, for checking the
 * modem status signals.
 */
static void pnx8550_timeout(unsigned long data)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)data;
	unsigned long flags;

	if (sport->port.info) {
		spin_lock_irqsave(&sport->port.lock, flags);
		pnx8550_mctrl_check(sport);
		spin_unlock_irqrestore(&sport->port.lock, flags);

		mod_timer(&sport->timer, jiffies + MCTRL_TIMEOUT);
	}
}

/*
 * interrupts disabled on entry
 */
static void pnx8550_stop_tx(struct uart_port *port, unsigned int tty_stop)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	u32 ien;

	/* Disable TX intr */
	ien = UART_GET_IEN(sport);
	UART_PUT_IEN(sport, ien & ~PNX8550_UART_INT_ALLTX);

	/* Clear all pending TX intr */
	UART_PUT_ICLR(sport, PNX8550_UART_INT_ALLTX);

//	sport->port.read_status_mask &= ~UTSR0_TO_SM(UTSR0_TFS); /* FIXME */
}

/*
 * interrupts may not be disabled on entry
 */
static void pnx8550_start_tx(struct uart_port *port, unsigned int tty_start)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	unsigned long flags;
	u32 ien;

	spin_lock_irqsave(&sport->port.lock, flags);

	/* Clear all pending TX intr */
	UART_PUT_ICLR(sport, PNX8550_UART_INT_ALLTX);

	/* Enable TX intr */
	ien = UART_GET_IEN(sport);
	UART_PUT_IEN(sport, ien | PNX8550_UART_INT_ALLTX);

//	sport->port.read_status_mask |= UTSR0_TO_SM(UTSR0_TFS);	/* FIXME */

	spin_unlock_irqrestore(&sport->port.lock, flags);
}

/*
 * Interrupts enabled
 */
static void pnx8550_stop_rx(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	u32 ien;

	/* Disable RX intr */
	ien = UART_GET_IEN(sport);
	UART_PUT_IEN(sport, ien & ~PNX8550_UART_INT_ALLRX);

	/* Clear all pending RX intr */
	UART_PUT_ICLR(sport, PNX8550_UART_INT_ALLRX);
}

/*
 * Set the modem control timer to fire immediately.
 */
static void pnx8550_enable_ms(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;

	mod_timer(&sport->timer, jiffies);
}

static void
pnx8550_rx_chars(struct pnx8550_port *sport, struct pt_regs *regs)
{
	struct tty_struct *tty = sport->port.info->tty;
	unsigned int status, ch, flg, ignored = 0;

	status = FIFO_TO_SM(UART_GET_FIFO(sport)) |
		 ISTAT_TO_SM(UART_GET_ISTAT(sport));
	while (status & FIFO_TO_SM(PNX8550_UART_FIFO_RXFIFO)) {
		ch = UART_GET_FIFO(sport);

		if (tty->flip.count >= TTY_FLIPBUF_SIZE)
			goto ignore_char;
		sport->port.icount.rx++;

		flg = TTY_NORMAL;

		/*
		 * note that the error handling code is
		 * out of the main execution path
		 */
		if (status & FIFO_TO_SM(PNX8550_UART_FIFO_RXFE |
					PNX8550_UART_FIFO_RXPAR))
			goto handle_error;

		if (uart_handle_sysrq_char(&sport->port, ch, regs))
			goto ignore_char;

	error_return:
		tty_insert_flip_char(tty, ch, flg);
	ignore_char:
		UART_PUT_LCR(sport, UART_GET_LCR(sport) |
				    PNX8550_UART_LCR_RX_NEXT);
		status = FIFO_TO_SM(UART_GET_FIFO(sport)) |
			 ISTAT_TO_SM(UART_GET_ISTAT(sport));
	}
 out:
	tty_flip_buffer_push(tty);
	return;

 handle_error:
	if (status & FIFO_TO_SM(PNX8550_UART_FIFO_RXPAR))
		sport->port.icount.parity++;
	else if (status & FIFO_TO_SM(PNX8550_UART_FIFO_RXFE))
		sport->port.icount.frame++;
	if (status & ISTAT_TO_SM(PNX8550_UART_INT_RXOVRN))
		sport->port.icount.overrun++;

	if (status & sport->port.ignore_status_mask) {
		if (++ignored > 100)
			goto out;
		goto ignore_char;
	}

//	status &= sport->port.read_status_mask;

	if (status & FIFO_TO_SM(PNX8550_UART_FIFO_RXPAR))
		flg = TTY_PARITY;
	else if (status & FIFO_TO_SM(PNX8550_UART_FIFO_RXFE))
		flg = TTY_FRAME;

	if (status & ISTAT_TO_SM(PNX8550_UART_INT_RXOVRN)) {
		/*
		 * overrun does *not* affect the character
		 * we read from the FIFO
		 */
		tty_insert_flip_char(tty, ch, flg);
		ch = 0;
		flg = TTY_OVERRUN;
	}
#ifdef SUPPORT_SYSRQ
	sport->port.sysrq = 0;
#endif
	goto error_return;
}

static void pnx8550_tx_chars(struct pnx8550_port *sport)
{
	struct circ_buf *xmit = &sport->port.info->xmit;

	if (sport->port.x_char) {
		UART_PUT_FIFO(sport, sport->port.x_char);
		sport->port.icount.tx++;
		sport->port.x_char = 0;
		return;
	}

	/*
	 * Check the modem control lines before
	 * transmitting anything.
	 */
	pnx8550_mctrl_check(sport);

	if (uart_circ_empty(xmit) || uart_tx_stopped(&sport->port)) {
		pnx8550_stop_tx(&sport->port, 0);
		return;
	}

	/*
	 * TX while bytes available
	 */
	while (((UART_GET_FIFO(sport) & PNX8550_UART_FIFO_TXFIFO) >> 16) < 16) {
		UART_PUT_FIFO(sport, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		sport->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&sport->port);

	if (uart_circ_empty(xmit))
		pnx8550_stop_tx(&sport->port, 0);
}

static irqreturn_t pnx8550_int(int irq, void *dev_id, struct pt_regs *regs)
{
	struct pnx8550_port *sport = dev_id;
	unsigned int status;

	spin_lock(&sport->port.lock);
#if	0	/* FIXME */
	status = UART_GET_UTSR0(sport);
	status &= SM_TO_UTSR0(sport->port.read_status_mask) | ~UTSR0_TFS;
	do {
		if (status & (UTSR0_RFS | UTSR0_RID)) {
			/* Clear the receiver idle bit, if set */
			if (status & UTSR0_RID)
				UART_PUT_UTSR0(sport, UTSR0_RID);
			pnx8550_rx_chars(sport, regs);
		}

		/* Clear the relevant break bits */
		if (status & (UTSR0_RBB | UTSR0_REB))
			UART_PUT_UTSR0(sport, status & (UTSR0_RBB | UTSR0_REB));

		if (status & UTSR0_RBB)
			sport->port.icount.brk++;

		if (status & UTSR0_REB)
			uart_handle_break(&sport->port);

		if (status & UTSR0_TFS)
			pnx8550_tx_chars(sport);
		if (pass_counter++ > PNX8550_ISR_PASS_LIMIT)
			break;
		status = UART_GET_UTSR0(sport);
		status &= SM_TO_UTSR0(sport->port.read_status_mask) |
			  ~UTSR0_TFS;
	} while (status & (UTSR0_TFS | UTSR0_RFS | UTSR0_RID));
#else
	/* Get the interrupts */
	status  = UART_GET_ISTAT(sport) & UART_GET_IEN(sport);

	/* RX Receiver Holding Register Overrun */
	if (status & PNX8550_UART_INT_RXOVRN) {
		sport->port.icount.overrun++;
		UART_PUT_ICLR(sport, PNX8550_UART_INT_RXOVRN);
	}

	/* RX Frame Error */
	if (status & PNX8550_UART_INT_FRERR) {
		sport->port.icount.frame++;
		UART_PUT_ICLR(sport, PNX8550_UART_INT_FRERR);
	}

	/* Break signal received */
	if (status & PNX8550_UART_INT_BREAK) {
		sport->port.icount.brk++;
		UART_PUT_ICLR(sport, PNX8550_UART_INT_BREAK);
	}

	/* RX Parity Error */
	if (status & PNX8550_UART_INT_PARITY) {
		sport->port.icount.parity++;
		UART_PUT_ICLR(sport, PNX8550_UART_INT_PARITY);
	}

	/* Byte received */
	if (status & PNX8550_UART_INT_RX) {
		pnx8550_rx_chars(sport, regs);
		UART_PUT_ICLR(sport, PNX8550_UART_INT_RX);
	}

	/* TX holding register empty - transmit a byte */
	if (status & PNX8550_UART_INT_TX) {
		pnx8550_tx_chars(sport);
		UART_PUT_ICLR(sport, PNX8550_UART_INT_TX);
	}

	/* TX shift register and holding register empty  */
	if (status & PNX8550_UART_INT_EMPTY) {
		UART_PUT_ICLR(sport, PNX8550_UART_INT_EMPTY);
	}

	/* Receiver time out */
	if (status & PNX8550_UART_INT_RCVTO) {
		UART_PUT_ICLR(sport, PNX8550_UART_INT_RCVTO);
	}
#endif
	spin_unlock(&sport->port.lock);
	return IRQ_HANDLED;
}

/*
 * Return TIOCSER_TEMT when transmitter is not busy.
 */
static unsigned int pnx8550_tx_empty(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;

	return UART_GET_FIFO(sport) & PNX8550_UART_FIFO_TXFIFO_STA ? 0 : TIOCSER_TEMT;
}

static unsigned int pnx8550_get_mctrl(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	unsigned int mctrl = TIOCM_DSR;
	unsigned int msr;

	/* REVISIT */

	msr = UART_GET_MCR(sport);

	mctrl |= msr & PNX8550_UART_MCR_CTS ? TIOCM_CTS : 0;
	mctrl |= msr & PNX8550_UART_MCR_DCD ? TIOCM_CAR : 0;

	return mctrl;
}

static void pnx8550_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
#if	0	/* FIXME */
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	unsigned int msr;
#endif
}

/*
 * Interrupts always disabled.
 */
static void pnx8550_break_ctl(struct uart_port *port, int break_state)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	unsigned long flags;
	unsigned int lcr;

	spin_lock_irqsave(&sport->port.lock, flags);
	lcr = UART_GET_LCR(sport);
	if (break_state == -1)
		lcr |= PNX8550_UART_LCR_TXBREAK;
	else
		lcr &= ~PNX8550_UART_LCR_TXBREAK;
	UART_PUT_LCR(sport, lcr);
	spin_unlock_irqrestore(&sport->port.lock, flags);
}

static int pnx8550_startup(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	int retval;

	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(sport->port.irq, pnx8550_int, 0,
			     "pnx8550-uart", sport);
	if (retval)
		return retval;

	/*
	 * Finally, clear and enable interrupts
	 */

	UART_PUT_ICLR(sport, PNX8550_UART_INT_ALLRX |
			     PNX8550_UART_INT_ALLTX);

	UART_PUT_IEN(sport, UART_GET_IEN(sport) |
			    PNX8550_UART_INT_ALLRX |
			    PNX8550_UART_INT_ALLTX);

	/*
	 * Enable modem status interrupts
	 */
	spin_lock_irq(&sport->port.lock);
	pnx8550_enable_ms(&sport->port);
	spin_unlock_irq(&sport->port.lock);

	return 0;
}

static void pnx8550_shutdown(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;

	/*
	 * Stop our timer.
	 */
	del_timer_sync(&sport->timer);

	/*
	 * Disable all interrupts, port and break condition.
	 */
	UART_PUT_IEN(sport, 0);

	/*
	 * Reset the Tx and Rx FIFOS
	 */
	UART_PUT_LCR(sport, UART_GET_LCR(sport) |
			    PNX8550_UART_LCR_TX_RST |
			    PNX8550_UART_LCR_RX_RST);

	/*
	 * Clear all interrupts
	 */
	UART_PUT_ICLR(sport, PNX8550_UART_INT_ALLRX |
			     PNX8550_UART_INT_ALLTX);

	/*
	 * Free the interrupt
	 */
	free_irq(sport->port.irq, sport);
}

static void
pnx8550_set_termios(struct uart_port *port, struct termios *termios,
		   struct termios *old)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	unsigned long flags;
	unsigned int lcr_fcr, old_ien, baud, quot;
	unsigned int old_csize = old ? old->c_cflag & CSIZE : CS8;

	/*
	 * We only support CS7 and CS8.
	 */
	while ((termios->c_cflag & CSIZE) != CS7 &&
	       (termios->c_cflag & CSIZE) != CS8) {
		termios->c_cflag &= ~CSIZE;
		termios->c_cflag |= old_csize;
		old_csize = CS8;
	}

	if ((termios->c_cflag & CSIZE) == CS8)
		lcr_fcr = PNX8550_UART_LCR_8BIT;
	else
		lcr_fcr = 0;

	if (termios->c_cflag & CSTOPB)
		lcr_fcr |= PNX8550_UART_LCR_2STOPB;
	if (termios->c_cflag & PARENB) {
		lcr_fcr |= PNX8550_UART_LCR_PAREN;
		if (!(termios->c_cflag & PARODD))
			lcr_fcr |= PNX8550_UART_LCR_PAREVN;
	}

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16); 
	quot = uart_get_divisor(port, baud);

	spin_lock_irqsave(&sport->port.lock, flags);

#if	0	/* REVISIT */
	sport->port.read_status_mask &= UTSR0_TO_SM(UTSR0_TFS);
	sport->port.read_status_mask |= UTSR1_TO_SM(UTSR1_ROR);
	if (termios->c_iflag & INPCK)
		sport->port.read_status_mask |=
				UTSR1_TO_SM(UTSR1_FRE | UTSR1_PRE);
	if (termios->c_iflag & (BRKINT | PARMRK))
		sport->port.read_status_mask |=
				UTSR0_TO_SM(UTSR0_RBB | UTSR0_REB);

	/*
	 * Characters to ignore
	 */
	sport->port.ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		sport->port.ignore_status_mask |=
				UTSR1_TO_SM(UTSR1_FRE | UTSR1_PRE);
	if (termios->c_iflag & IGNBRK) {
		sport->port.ignore_status_mask |=
				UTSR0_TO_SM(UTSR0_RBB | UTSR0_REB);
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			sport->port.ignore_status_mask |=
				UTSR1_TO_SM(UTSR1_ROR);
	}
#endif

	del_timer_sync(&sport->timer);

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	/*
	 * disable interrupts and drain transmitter
	 */
	old_ien = UART_GET_IEN(sport);
	UART_PUT_IEN(sport, old_ien & ~(PNX8550_UART_INT_ALLTX |
					PNX8550_UART_INT_ALLRX));

	while (UART_GET_FIFO(sport) & PNX8550_UART_FIFO_TXFIFO_STA)
		barrier();

	/* then, disable everything */
	UART_PUT_IEN(sport, 0);

	/* Reset the Rx and Tx FIFOs too */
	lcr_fcr |= PNX8550_UART_LCR_TX_RST;
	lcr_fcr |= PNX8550_UART_LCR_RX_RST;

	/* set the parity, stop bits and data size */
	UART_PUT_LCR(sport, lcr_fcr);

	/* set the baud rate */
	quot -= 1;
	UART_PUT_BAUD(sport, quot);

	UART_PUT_ICLR(sport, -1);

	UART_PUT_IEN(sport, old_ien);

	if (UART_ENABLE_MS(&sport->port, termios->c_cflag))
		pnx8550_enable_ms(&sport->port);

	spin_unlock_irqrestore(&sport->port.lock, flags);
}

static const char *pnx8550_type(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;

	return sport->port.type == PORT_PNX8550 ? "PNX8550" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'.
 */
static void pnx8550_release_port(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;

	release_mem_region(sport->port.mapbase, UART_PORT_SIZE);
}

/*
 * Request the memory region(s) being used by 'port'.
 */
static int pnx8550_request_port(struct uart_port *port)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;

	return request_mem_region(sport->port.mapbase, UART_PORT_SIZE,
			"pnx8550-uart") != NULL ? 0 : -EBUSY;
}

/*
 * Configure/autoconfigure the port.
 */
static void pnx8550_config_port(struct uart_port *port, int flags)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;

	if (flags & UART_CONFIG_TYPE &&
	    pnx8550_request_port(&sport->port) == 0)
		sport->port.type = PORT_PNX8550;
}

/*
 * Verify the new serial_struct (for TIOCSSERIAL).
 * The only change we allow are to the flags and type, and
 * even then only between PORT_PNX8550 and PORT_UNKNOWN
 */
static int
pnx8550_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	struct pnx8550_port *sport = (struct pnx8550_port *)port;
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_PNX8550)
		ret = -EINVAL;
	if (sport->port.irq != ser->irq)
		ret = -EINVAL;
	if (ser->io_type != SERIAL_IO_MEM)
		ret = -EINVAL;
	if (sport->port.uartclk / 16 != ser->baud_base)
		ret = -EINVAL;
	if ((void *)sport->port.mapbase != ser->iomem_base)
		ret = -EINVAL;
	if (sport->port.iobase != ser->port)
		ret = -EINVAL;
	if (ser->hub6 != 0)
		ret = -EINVAL;
	return ret;
}

static struct uart_ops pnx8550_pops = {
	.tx_empty	= pnx8550_tx_empty,
	.set_mctrl	= pnx8550_set_mctrl,
	.get_mctrl	= pnx8550_get_mctrl,
	.stop_tx	= pnx8550_stop_tx,
	.start_tx	= pnx8550_start_tx,
	.stop_rx	= pnx8550_stop_rx,
	.enable_ms	= pnx8550_enable_ms,
	.break_ctl	= pnx8550_break_ctl,
	.startup	= pnx8550_startup,
	.shutdown	= pnx8550_shutdown,
	.set_termios	= pnx8550_set_termios,
	.type		= pnx8550_type,
	.release_port	= pnx8550_release_port,
	.request_port	= pnx8550_request_port,
	.config_port	= pnx8550_config_port,
	.verify_port	= pnx8550_verify_port,
};

static struct pnx8550_port pnx8550_ports[NR_PORTS] = {
	[0] = {
		.port   = {
			.type		= PORT_PNX8550,
			.iotype		= SERIAL_IO_MEM,
			.membase	= (void __iomem *)PNX8550_UART_PORT0,
			.mapbase	= PNX8550_UART_PORT0,
			.irq		= PNX8550_UART_INT(0),
			.uartclk	= 3692300,
			.fifosize	= 16,
			.ops		= &pnx8550_pops,
			.flags		= ASYNC_BOOT_AUTOCONF,
			.line		= 0,
		},
	},
	[1] = {
		.port   = {
			.type		= PORT_PNX8550,
			.iotype		= SERIAL_IO_MEM,
			.membase	= (void __iomem *)PNX8550_UART_PORT1,
			.mapbase	= PNX8550_UART_PORT1,
			.irq		= PNX8550_UART_INT(1),
			.uartclk	= 3692300,
			.fifosize	= 16,
			.ops		= &pnx8550_pops,
			.flags		= ASYNC_BOOT_AUTOCONF,
			.line		= 1,
		},
	},
};

/*
 * Setup the PNX8550 serial ports.
 *
 * Note also that we support "console=ttySx" where "x" is either 0 or 1.
 */
static void __init pnx8550_init_ports(void)
{
	static int first = 1;
	int i;

	if (!first)
		return;
	first = 0;

	for (i = 0; i < NR_PORTS; i++) {
		init_timer(&pnx8550_ports[i].timer);
		pnx8550_ports[i].timer.function = pnx8550_timeout;
		pnx8550_ports[i].timer.data     = (unsigned long)&pnx8550_ports[i];
	}
}

#ifdef CONFIG_SERIAL_PNX8550_CONSOLE

/*
 * Interrupts are disabled on entering
 */
static void
pnx8550_console_write(struct console *co, const char *s, unsigned int count)
{
	struct pnx8550_port *sport = &pnx8550_ports[co->index];
	unsigned int old_ien, status, i;

	/*
	 *	First, save IEN and then disable interrupts
	 */
	old_ien = UART_GET_IEN(sport);
	UART_PUT_IEN(sport, old_ien & ~(PNX8550_UART_INT_ALLTX |
					PNX8550_UART_INT_ALLRX));

	/*
	 *	Now, do each character
	 */
	for (i = 0; i < count; i++) {
		do {
			/* Wait for UART_TX register to empty */
			status = UART_GET_FIFO(sport);
		} while (status & PNX8550_UART_FIFO_TXFIFO);
		UART_PUT_FIFO(sport, s[i]);
		if (s[i] == '\n') {
			do {
				status = UART_GET_FIFO(sport);
			} while (status & PNX8550_UART_FIFO_TXFIFO);
			UART_PUT_FIFO(sport, '\r');
		}
	}

	/*
	 *	Finally, wait for transmitter to become empty
	 *	and restore IEN
	 */
	do {
		/* Wait for UART_TX register to empty */
		status = UART_GET_FIFO(sport);
	} while (status & PNX8550_UART_FIFO_TXFIFO);

	/* Clear TX and EMPTY interrupt */
	UART_PUT_ICLR(sport, PNX8550_UART_INT_TX |
			     PNX8550_UART_INT_EMPTY);

	UART_PUT_IEN(sport, old_ien);
}

static int __init
pnx8550_console_setup(struct console *co, char *options)
{
	struct pnx8550_port *sport;
	int baud = 38400;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (co->index == -1 || co->index >= NR_PORTS)
		co->index = 0;
	sport = &pnx8550_ports[co->index];

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(&sport->port, co, baud, parity, bits, flow);
}

extern struct uart_driver pnx8550_reg;
static struct console pnx8550_console = {
	.name		= "ttyS",
	.write		= pnx8550_console_write,
	.device		= uart_console_device,
	.setup		= pnx8550_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &pnx8550_reg,
};

static int __init pnx8550_rs_console_init(void)
{
	pnx8550_init_ports();
	register_console(&pnx8550_console);
	return 0;
}
console_initcall(pnx8550_rs_console_init);

#define PNX8550_CONSOLE	&pnx8550_console
#else
#define PNX8550_CONSOLE	NULL
#endif

static struct uart_driver pnx8550_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "ttyS",
	.dev_name		= "ttyS",
	.devfs_name		= "tts/",
	.major			= SERIAL_PNX8550_MAJOR,
	.minor			= MINOR_START,
	.nr			= NR_PORTS,
	.cons			= PNX8550_CONSOLE,
};

static int pnx8550_serial_suspend(struct device *_dev, u32 state, u32 level)
{
	struct pnx8550_port *sport = dev_get_drvdata(_dev);

	if (sport && level == SUSPEND_DISABLE)
		uart_suspend_port(&pnx8550_reg, &sport->port);

	return 0;
}

static int pnx8550_serial_resume(struct device *_dev, u32 level)
{
	struct pnx8550_port *sport = dev_get_drvdata(_dev);

	if (sport && level == RESUME_ENABLE)
		uart_resume_port(&pnx8550_reg, &sport->port);

	return 0;
}

static int pnx8550_serial_probe(struct device *_dev)
{
	struct platform_device *dev = to_platform_device(_dev);
	struct resource *res = dev->resource;
	int i;

	for (i = 0; i < dev->num_resources; i++, res++) {
		if (!(res->flags & IORESOURCE_MEM))
			continue;

		for (i = 0; i < NR_PORTS; i++) {
			if (pnx8550_ports[i].port.mapbase != res->start)
				continue;

			pnx8550_ports[i].port.dev = _dev;
			uart_add_one_port(&pnx8550_reg, &pnx8550_ports[i].port);
			dev_set_drvdata(_dev, &pnx8550_ports[i]);
			break;
		}
	}

	return 0;
}

static int pnx8550_serial_remove(struct device *_dev)
{
	struct pnx8550_port *sport = dev_get_drvdata(_dev);

	dev_set_drvdata(_dev, NULL);

	if (sport)
		uart_remove_one_port(&pnx8550_reg, &sport->port);

	return 0;
}

static struct device_driver pnx8550_serial_driver = {
	.name		= "pnx8550-uart",
	.bus		= &platform_bus_type,
	.probe		= pnx8550_serial_probe,
	.remove		= pnx8550_serial_remove,
	.suspend	= pnx8550_serial_suspend,
	.resume		= pnx8550_serial_resume,
};

static int __init pnx8550_serial_init(void)
{
	int ret;

	printk(KERN_INFO "Serial: PNX8550 driver $Revision: 1.50 $\n");

	pnx8550_init_ports();

	ret = uart_register_driver(&pnx8550_reg);
	if (ret == 0) {
		ret = driver_register(&pnx8550_serial_driver);
		if (ret)
			uart_unregister_driver(&pnx8550_reg);
	}
	return ret;
}

static void __exit pnx8550_serial_exit(void)
{
	driver_unregister(&pnx8550_serial_driver);
	uart_unregister_driver(&pnx8550_reg);
}

module_init(pnx8550_serial_init);
module_exit(pnx8550_serial_exit);

MODULE_AUTHOR("Embedded Alley Solutions, Inc.");
MODULE_DESCRIPTION("PNX8550 generic serial port driver $Revision: 1.50 $");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CHARDEV_MAJOR(SERIAL_PNX8550_MAJOR);
