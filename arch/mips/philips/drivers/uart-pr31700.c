/*
 * Serial driver for r39xx
 *
 * Copyright (C) 2000 Jim Pick <jim@jimpick.com>
 *
 * Inspired by, and/or includes bits from:
 *
 * drivers/char/serial.c (standard serial driver)
 * drivers/char/sx.c (use of generic_serial interface)
 * drivers/char/esp.c (another UART that uses DMA)
 * arch/mips/vr41xx/serial.c (another MIPS serial driver)
 * 
 * Please see those files for credits.
 *
 * Also, the original rough serial console code was:
 *
 * Copyright (C) 1999 Harald Koerfgen
 *
 * $Id: r39xx_serial.c,v 1.16 2001/01/11 20:24:47 pavel Exp $
 */

#include <linux/init.h>
#include <linux/config.h>
#include <linux/tty.h>
#include <linux/major.h>
#include <linux/ptrace.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <asm/uaccess.h>
#include <asm/philips/pr31700.h>
#include <asm/delay.h>
#include <asm/wbflush.h>
#include "uart-pr31700.h"

/* Prototypes */

static void rs_disable_tx_interrupts (void * ptr);
static void rs_enable_tx_interrupts (void * ptr); 
static void rs_disable_rx_interrupts (void * ptr); 
static void rs_enable_rx_interrupts (void * ptr); 
static int rs_get_CD (void * ptr); 
static void rs_shutdown_port (void * ptr); 
static int rs_set_real_termios (void *ptr);
static int rs_chars_in_buffer (void * ptr); 
static void rs_hungup (void *ptr);
static void rs_close (void *ptr);



static struct real_driver rs_real_driver = { 
  disable_tx_interrupts: rs_disable_tx_interrupts, 
  enable_tx_interrupts:  rs_enable_tx_interrupts, 
  disable_rx_interrupts: rs_disable_rx_interrupts, 
  enable_rx_interrupts:  rs_enable_rx_interrupts, 
  get_CD:                rs_get_CD, 
  shutdown_port:         rs_shutdown_port,  
  set_real_termios:      rs_set_real_termios,  
  chars_in_buffer:       rs_chars_in_buffer, 
  close:                 rs_close, 
  hungup:                rs_hungup,
}; 

static struct tty_driver rs_driver, rs_callout_driver;

static struct tty_struct * rs_table[RS_NPORTS] = { NULL, };
static struct termios ** rs_termios;
static struct termios ** rs_termios_locked;

struct rs_port *rs_ports;
int rs_refcount;
int rs_initialized = 0;

#ifdef CONFIG_PM
static struct pm_dev *pmdev;
static int pm_request(struct pm_dev* dev, pm_request_t req, void* data);
#endif

#define DEBUG
#undef DEBUG2

#ifdef DEBUG2
int rs_debug = RS_DEBUG_ALL & ~RS_DEBUG_TRANSMIT;
#else
int rs_debug = 0;
#endif


/*
 *  Helper routines
 */


static inline unsigned int serial_in(struct rs_port *port, int offset)
{
	unsigned int tmp;

        tmp = *(volatile unsigned int *)(port->base + offset);
        tmp &= 0xff;
	barrier();
	return tmp;
}

static inline void serial_out(struct rs_port *port, int offset, int value)
{
	*(volatile unsigned int *)(port->base + offset) = (unsigned char)value;

	barrier();
}

/*
 * ----------------------------------------------------------------------
 *
 * Here starts the interrupt handling routines.  All of the following
 * subroutines are declared as inline and are folded into
 * rs_interrupt().  They were separated out for readability's sake.
 *
 * Note: rs_interrupt() is a "fast" interrupt, which means that it
 * runs with interrupts turned off.  People who may want to modify
 * rs_interrupt() should try to keep the interrupt handler as fast as
 * possible.  After you are done making modifications, it is not a bad
 * idea to do:
 * 
 * gcc -S -DKERNEL -Wall -Wstrict-prototypes -O6 -fomit-frame-pointer serial.c
 *
 * and look at the resulting assemble code in serial.s.
 *
 * 				- Ted Ts'o (tytso@mit.edu), 7-Mar-93
 * -----------------------------------------------------------------------
 */



static inline void receive_char_pio(struct rs_port *port)
{
	struct tty_struct *tty = port->gs.tty;
	unsigned char ch;
	int counter = 2048;

	/* While there are characters, get them ... */
	while (counter>0) {
		// printk("R%08x", (int)port->base[UART_R39XX_CTRL1]);
		if (!(port->base[UART_R39XX_CTRL1] & UART_RX_HOLD_FULL)) {
			break;
		}
		ch = serial_in(port, UART_R39XX_DATA);
		if (tty->flip.count < TTY_FLIPBUF_SIZE) {
			*tty->flip.char_buf_ptr++ = ch;
			*tty->flip.flag_buf_ptr++ = 0;
			tty->flip.count++;
		}
		udelay(1); /* Allow things to happen - it take a while */
		counter--;
	}
	if (!counter)
		printk( "Ugh, looped in receive_char_pio!\n" );

	tty_flip_buffer_push(tty);
#if 0		
	/* Now handle error conditions */
	if (*status & (INTTYPE(UART_RXOVERRUN_INT) |
			INTTYPE(UART_FRAMEERR_INT) |
			INTTYPE(UART_PARITYERR_INT) |
			INTTYPE(UART_BREAK_INT))) {

		/*
		 * Now check to see if character should be
		 * ignored, and mask off conditions which
		 * should be ignored.
	       	 */
		if (*status & port->ignore_status_mask) {
			goto ignore_char;
		}
		*status &= port->read_status_mask;
		
		if (*status & INTTYPE(UART_BREAK_INT)) {
			rs_dprintk(RS_DEBUG_INTERRUPTS, "handling break....");
			*tty->flip.flag_buf_ptr = TTY_BREAK;
		}
		else if (*status & INTTYPE(UART_PARITYERR_INT)) {
			*tty->flip.flag_buf_ptr = TTY_PARITY;
		}
		else if (*status & INTTYPE(UART_FRAMEERR_INT)) {
			*tty->flip.flag_buf_ptr = TTY_FRAME;
		}
		if (*status & INTTYPE(UART_RXOVERRUN_INT)) {
			/*
			 * Overrun is special, since it's
			 * reported immediately, and doesn't
			 * affect the current character
			 */
			if (tty->flip.count < TTY_FLIPBUF_SIZE) {
				tty->flip.count++;
				tty->flip.flag_buf_ptr++;
				tty->flip.char_buf_ptr++;
				*tty->flip.flag_buf_ptr = TTY_OVERRUN;
			}
		}
	}

	tty->flip.flag_buf_ptr++;
	tty->flip.char_buf_ptr++;
	tty->flip.count++;

 ignore_char:

	tty_flip_buffer_push(tty);
#endif
}

static inline void transmit_char_pio(struct rs_port *port)
{
	/* While I'm able to transmit ... */
	for (;;) {
		// printk("T%08x", (int)port->base[UART_R39XX_CTRL1]);
		if (!(port->base[UART_R39XX_CTRL1] & UART_TX_EMPTY)) {
			break;
		}
		else if (port->x_char) {
			serial_out(port, UART_R39XX_DATA, port->x_char);
			port->icount.tx++;
			port->x_char = 0;
		}
		else if (port->gs.xmit_cnt <= 0 || port->gs.tty->stopped ||
		    port->gs.tty->hw_stopped) {
			break;
		}
		else {
			serial_out(port, UART_R39XX_DATA, port->gs.xmit_buf[port->gs.xmit_tail++]); 
			port->icount.tx++;
			port->gs.xmit_tail &= SERIAL_XMIT_SIZE-1;
			if (--port->gs.xmit_cnt <= 0) {
				break;
			}
		}
		udelay(10); /* Allow things to happen - it take a while */
	}

	if (port->gs.xmit_cnt <= 0 || port->gs.tty->stopped ||
	     port->gs.tty->hw_stopped) {
		rs_disable_tx_interrupts(port);
	}
	
        if (port->gs.xmit_cnt <= port->gs.wakeup_chars) {
                if ((port->gs.tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
                    port->gs.tty->ldisc.write_wakeup)
                        (port->gs.tty->ldisc.write_wakeup)(port->gs.tty);
                rs_dprintk (RS_DEBUG_TRANSMIT, "Waking up.... ldisc (%d)....\n",
                            port->gs.wakeup_chars); 
                wake_up_interruptible(&port->gs.tty->write_wait);
       	}	
}



static inline void check_modem_status(struct rs_port *port)
{
        /* We don't have a carrier detect line - but just respond
           like we had one anyways so that open() becomes unblocked */
	wake_up_interruptible(&port->gs.open_wait);
}

int count = 0;

/*
 * This is the serial driver's interrupt routine (inlined, because
 * there are two different versions of this, one for each serial port,
 * differing only by the bits used in interrupt status 2 register)
 */

static inline void rs_rx_interrupt(int irq, void *dev_id,
				  struct pt_regs * regs, int intshift)
{
	struct rs_port * port;
	unsigned long int2status;
	unsigned long flags;
	unsigned long ints;

	save_and_cli(flags);

	port = (struct rs_port *)dev_id;
	/* rs_dprintk (RS_DEBUG_INTERRUPTS, "rs_interrupt (port %p, shift %d)...", port, intshift); */

	/* Get the interrrupts we have enabled */
	int2status = IntStatus2 & IntEnable2;

	/* Get interrupts in easy to use form */
	ints = int2status >> intshift;
	// printk("IR%03x%03x", (int)(IntStatus2 >> intshift), (int)(IntEnable2 >> intshift));

	/* Clear any interrupts we might be about to handle */
	IntClear2 = int2status & (
		(INTTYPE(UART_RXOVERRUN_INT) |
		 INTTYPE(UART_FRAMEERR_INT) |
		 INTTYPE(UART_BREAK_INT) |
		 INTTYPE(UART_PARITYERR_INT) |
		 INTTYPE(UART_RX_INT)) << intshift);

	if (!port || !port->gs.tty) {
		restore_flags(flags);
		return;
	}

	/* RX Receiver Holding Register Overrun */
	if (ints & INTTYPE(UART_RXOVERRUN_INT)) {
		rs_dprintk (RS_DEBUG_INTERRUPTS, "overrun");
		port->icount.overrun++;
	}

	/* RX Frame Error */
	if (ints & INTTYPE(UART_FRAMEERR_INT)) {
		rs_dprintk (RS_DEBUG_INTERRUPTS, "frame error");
		port->icount.frame++;
	}

	/* Break signal received */
	if (ints & INTTYPE(UART_BREAK_INT)) {
		rs_dprintk (RS_DEBUG_INTERRUPTS, "break");
		port->icount.brk++;
      	}

	/* RX Parity Error */
	if (ints & INTTYPE(UART_PARITYERR_INT)) {
		rs_dprintk (RS_DEBUG_INTERRUPTS, "parity error");
		port->icount.parity++;
	}

	/* Receive byte (non-DMA) */
	if (ints & INTTYPE(UART_RX_INT)) {
		receive_char_pio(port);
	}

	/*
	check_modem_status();
	*/

	// printk("OR%03x", (int)((IntStatus2 & IntEnable2) >> intshift));

	restore_flags(flags);

/*	rs_dprintk (RS_DEBUG_INTERRUPTS, "end.\n"); */

}

static inline void rs_tx_interrupt(int irq, void *dev_id,
				  struct pt_regs * regs, int intshift)
{
	struct rs_port * port;
	unsigned long int2status;
	unsigned long flags;
	unsigned long ints;

	save_and_cli(flags);

	port = (struct rs_port *)dev_id;
	/* rs_dprintk (RS_DEBUG_INTERRUPTS, "rs_interrupt (port %p, shift %d)...", port, intshift); */

	/* Get the interrrupts we have enabled */
	int2status = IntStatus2 & IntEnable2;

	if (!port || !port->gs.tty) {
		restore_flags(flags);
		return;
	}

	/* Get interrupts in easy to use form */
	ints = int2status >> intshift;

	/* Clear any interrupts we might be about to handle */
	IntClear2 = int2status & (
		(INTTYPE(UART_TX_INT) |
		 INTTYPE(UART_EMPTY_INT) |
		 INTTYPE(UART_TXOVERRUN_INT)) << intshift);

	//printk("IT%03x", (int)ints);

	/* TX holding register empty, so transmit byte (non-DMA) */
	if (ints & (INTTYPE(UART_TX_INT) | INTTYPE(UART_EMPTY_INT))) {
		transmit_char_pio(port);
	}

	/* TX Transmit Holding Register Overrun (shouldn't happen) */
	if (ints & INTTYPE(UART_TXOVERRUN_INT)) {
		printk ( "rs: TX overrun\n");
	}

	/*
	check_modem_status();
	*/

	restore_flags(flags);

/*	rs_dprintk (RS_DEBUG_INTERRUPTS, "end.\n"); */

}

static void rs_rx_interrupt_uarta(int irq, void *dev_id,
					 struct pt_regs * regs)
{
	rs_rx_interrupt(irq, dev_id, regs, UARTA_SHIFT);
}

static void rs_tx_interrupt_uarta(int irq, void *dev_id,
					 struct pt_regs * regs)
{
	rs_tx_interrupt(irq, dev_id, regs, UARTA_SHIFT);
}

#if 0
static void rs_interrupt_uartb(int irq, void *dev_id,
					 struct pt_regs * regs)
{
	rs_interrupt(irq, dev_id, regs, UARTB_SHIFT);
}
#endif

/*
 * -------------------------------------------------------------------
 * Here ends the serial interrupt routines.
 * -------------------------------------------------------------------
 */





/* ********************************************************************** *
 *                Here are the routines that actually                     *
 *              interface with the generic_serial driver                  *
 * ********************************************************************** */

static void rs_disable_tx_interrupts (void * ptr) 
{
	struct rs_port *port = ptr; 
	unsigned long flags;

	save_and_cli(flags);
        port->gs.flags &= ~GS_TX_INTEN;

	IntEnable2 &= ~((INTTYPE(UART_TX_INT) |
			INTTYPE(UART_EMPTY_INT) |
			INTTYPE(UART_TXOVERRUN_INT)) << port->intshift);

	IntClear2 = (INTTYPE(UART_TX_INT) |
			INTTYPE(UART_EMPTY_INT) |
			INTTYPE(UART_TXOVERRUN_INT)) << port->intshift;

	restore_flags(flags);
}


static void rs_enable_tx_interrupts (void * ptr) 
{
	struct rs_port *port = ptr; 
	unsigned long flags;

	save_and_cli(flags);

	IntClear2 = (INTTYPE(UART_TX_INT) |
			INTTYPE(UART_EMPTY_INT) |
			INTTYPE(UART_TXOVERRUN_INT)) << port->intshift;

	IntEnable2 |= (INTTYPE(UART_TX_INT) |
			INTTYPE(UART_EMPTY_INT) |
			INTTYPE(UART_TXOVERRUN_INT)) << port->intshift;

	/* Send a char to start TX interrupts happening */
	transmit_char_pio(port);

	restore_flags(flags);
}


static void rs_disable_rx_interrupts (void * ptr) 
{
	struct rs_port *port = ptr;
	unsigned long flags;

	save_and_cli(flags);

	IntEnable2 &= ~((INTTYPE(UART_RX_INT) |
			 INTTYPE(UART_RXOVERRUN_INT) |
			 INTTYPE(UART_FRAMEERR_INT) |
			 INTTYPE(UART_BREAK_INT) |
			 INTTYPE(UART_PARITYERR_INT)) << port->intshift);

	IntClear2 = (INTTYPE(UART_RX_INT) |
			 INTTYPE(UART_RXOVERRUN_INT) |
			 INTTYPE(UART_FRAMEERR_INT) |
			 INTTYPE(UART_BREAK_INT) |
			 INTTYPE(UART_PARITYERR_INT)) << port->intshift;

	restore_flags(flags);
}

static void rs_enable_rx_interrupts (void * ptr) 
{
	struct rs_port *port = ptr;
	unsigned long flags;

	save_and_cli(flags);

	IntEnable2 |= (INTTYPE(UART_RX_INT) |
			 INTTYPE(UART_RXOVERRUN_INT) |
			 INTTYPE(UART_FRAMEERR_INT) |
			 INTTYPE(UART_BREAK_INT) |
			 INTTYPE(UART_PARITYERR_INT)) << port->intshift;

	/* Empty the input buffer - apparently this is *vital* */
	while (port->base[UART_R39XX_CTRL1] & UART_RX_HOLD_FULL) { 
		serial_in(port, UART_R39XX_DATA);
	}

	IntClear2 = (INTTYPE(UART_RX_INT) |
			 INTTYPE(UART_RXOVERRUN_INT) |
			 INTTYPE(UART_FRAMEERR_INT) |
			 INTTYPE(UART_BREAK_INT) |
			 INTTYPE(UART_PARITYERR_INT)) << port->intshift;

	restore_flags(flags);
}


static int rs_get_CD (void * ptr) 
{
	struct rs_port *port = ptr;
	func_enter2();

	/* No Carried Detect in Hardware - just return true */

	func_exit();
	return (1);
}




static void rs_shutdown_port (void * ptr) 
{
	struct rs_port *port = ptr; 

	func_enter();

	port->gs.flags &= ~ GS_ACTIVE;

	/* Jim: Disable interrupts and power down port? */

	func_exit();
}



static int rs_set_real_termios (void *ptr)
{
	struct rs_port *port = ptr;
	int t;

	func_enter2();

	switch (port->gs.baud) {
		/* Save some typing work... */
#define e(x) case x:t= SER_BAUD_ ## x ; break
		e(300);e(600);e(1200);e(2400);e(4800);e(9600);
		e(19200);e(38400);e(57600);e(76800);e(115200);e(230400);
	case 0      :t = -1;
		break;
	default:
		/* Can I return "invalid"? */
		t = SER_BAUD_9600;
		printk (KERN_INFO "rs: unsupported baud rate: %d.\n", port->gs.baud);
		break;
	}
#undef e
	if (t >= 0) {
		/* Jim: Set Hardware Baud rate - there is some good
		   code in drivers/char/serial.c */

	  	/* Program hardware for parity, data bits, stop bits (note: these are hardcoded to 8N1 */
		UartA_Ctrl1 &= 0xf000000f;
		UartA_Ctrl1 &= ~(UART_DIS_TXD | SER_SEVEN_BIT | SER_EVEN_PARITY | SER_TWO_STOP);

#define CFLAG port->gs.tty->termios->c_cflag
		if (C_PARENB(port->gs.tty))
			if (!C_PARODD(port->gs.tty))
				UartA_Ctrl1 |= SER_EVEN_PARITY;
			else
				UartA_Ctrl1 |= SER_ODD_PARITY;
		if ((CFLAG & CSIZE)==CS6)
			printk(KERN_ERR "6 bits not supported\n");
		if ((CFLAG & CSIZE)==CS5)
			printk(KERN_ERR "5 bits not supported\n");
		if ((CFLAG & CSIZE)==CS7)
			UartA_Ctrl1 |= SER_SEVEN_BIT;
		if (C_CSTOPB(port->gs.tty))
			UartA_Ctrl1 |= SER_TWO_STOP;

		UartA_Ctrl2 = t;
		UartA_DMActl1 = 0;
		UartA_DMActl2 = 0;
		UartA_Ctrl1 |= UART_ON;
	}

	/* Jim: Lots of good stuff in drivers/char/serial.c:change_speed() */

	func_exit ();
        return 0;
}


static int rs_chars_in_buffer (void * ptr) 
{
	struct rs_port *port = ptr;
	int scratch;
/*	func_enter2(); */

	scratch = serial_in(port, UART_R39XX_CTRL1);

/*	func_exit(); */
	return ( (scratch & UART_TX_EMPTY) ? 0 : 1 );
}



/* ********************************************************************** *
 *                Here are the routines that actually                     *
 *               interface with the rest of the system                    *
 * ********************************************************************** */


static int rs_open  (struct tty_struct * tty, struct file * filp)
{
	struct rs_port *port;
	int retval, line;

	func_enter();

	if (!rs_initialized) {
		return -EIO;
	}

	line = MINOR(tty->device) - tty->driver.minor_start;
	rs_dprintk (RS_DEBUG_OPEN, "%d: opening line %d. tty=%p ctty=%p)\n", 
	            (int) current->pid, line, tty, current->tty);

	if ((line < 0) || (line >= RS_NPORTS))
		return -ENODEV;

	/* Pre-initialized already */
	port = & rs_ports[line];

	rs_dprintk (RS_DEBUG_OPEN, "port = %p\n", port);

	tty->driver_data = port;
	port->gs.tty = tty;
	port->gs.count++;

	rs_dprintk (RS_DEBUG_OPEN, "starting port\n");

	/*
	 * Start up serial port
	 */
	retval = gs_init_port(&port->gs);
	rs_dprintk (RS_DEBUG_OPEN, "done gs_init\n");
	if (retval) {
		port->gs.count--;
		return retval;
	}

	port->gs.flags |= GS_ACTIVE;

	rs_dprintk (RS_DEBUG_OPEN, "before inc_use_count (count=%d.\n", 
	            port->gs.count);
	if (port->gs.count == 1) {
		MOD_INC_USE_COUNT;
	}
	rs_dprintk (RS_DEBUG_OPEN, "after inc_use_count\n");

	/* Jim: Initialize port hardware here */

	/* Enable high-priority interrupts for UARTA */
	IntEnable6 |= INT6_UARTARXINT; 
	rs_enable_rx_interrupts(&rs_ports[0]); 

	retval = gs_block_til_ready(&port->gs, filp);
	rs_dprintk (RS_DEBUG_OPEN, "Block til ready returned %d. Count=%d\n", 
	            retval, port->gs.count);

	if (retval) {
		MOD_DEC_USE_COUNT;
		port->gs.count--;
		return retval;
	}
	/* tty->low_latency = 1; */

	if ((port->gs.count == 1) && (port->gs.flags & ASYNC_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = port->gs.normal_termios;
		else 
			*tty->termios = port->gs.callout_termios;
		rs_set_real_termios (port);
	}

	port->gs.session = current->session;
	port->gs.pgrp = current->pgrp;
	func_exit();

	/* Jim */
/*	cli(); */

	return 0;

}



static void rs_close (void *ptr)
{
	func_enter ();

	/* Anything to do here? */

	MOD_DEC_USE_COUNT;
	func_exit ();
}


/* I haven't the foggiest why the decrement use count has to happen
   here. The whole linux serial drivers stuff needs to be redesigned.
   My guess is that this is a hack to minimize the impact of a bug
   elsewhere. Thinking about it some more. (try it sometime) Try
   running minicom on a serial port that is driven by a modularized
   driver. Have the modem hangup. Then remove the driver module. Then
   exit minicom.  I expect an "oops".  -- REW */
static void rs_hungup (void *ptr)
{
	func_enter ();
	MOD_DEC_USE_COUNT;
	func_exit ();
}

static int rs_ioctl (struct tty_struct * tty, struct file * filp, 
                     unsigned int cmd, unsigned long arg)
{
	int rc;
	struct rs_port *port = tty->driver_data;
	int ival;

	/* func_enter2(); */

	rc = 0;
	switch (cmd) {
	case TIOCGSOFTCAR:
		rc = put_user(((tty->termios->c_cflag & CLOCAL) ? 1 : 0),
		              (unsigned int *) arg);
		break;
	case TIOCSSOFTCAR:
		if ((rc = verify_area(VERIFY_READ, (void *) arg,
		                      sizeof(int))) == 0) {
			get_user(ival, (unsigned int *) arg);
			tty->termios->c_cflag =
				(tty->termios->c_cflag & ~CLOCAL) |
				(ival ? CLOCAL : 0);
		}
		break;
	case TIOCGSERIAL:
		if ((rc = verify_area(VERIFY_WRITE, (void *) arg,
		                      sizeof(struct serial_struct))) == 0)
			gs_getserial(&port->gs, (struct serial_struct *) arg);
		break;
	case TIOCSSERIAL:
		if ((rc = verify_area(VERIFY_READ, (void *) arg,
		                      sizeof(struct serial_struct))) == 0)
			rc = gs_setserial(&port->gs, (struct serial_struct *) arg);
		break;
	default:
		rc = -ENOIOCTLCMD;
		break;
	}

	/* func_exit(); */
	return rc;
}


/*
 * This function is used to send a high-priority XON/XOFF character to
 * the device
 */
static void rs_send_xchar(struct tty_struct * tty, char ch)
{
	struct rs_port *port = (struct rs_port *)tty->driver_data;
	func_enter ();
	
	port->x_char = ch;
	if (ch) {
		/* Make sure transmit interrupts are on */
		rs_enable_tx_interrupts(tty);
	}

	func_exit();
}


/*
 * ------------------------------------------------------------
 * rs_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void rs_throttle(struct tty_struct * tty)
{
#ifdef RS_DEBUG_THROTTLE
	char	buf[64];
	
	printk("throttle %s: %d....\n", tty_name(tty, buf),
	       tty->ldisc.chars_in_buffer(tty));
#endif

	func_enter ();
	
	if (I_IXOFF(tty))
		rs_send_xchar(tty, STOP_CHAR(tty));

	func_exit ();
}

static void rs_unthrottle(struct tty_struct * tty)
{
	struct rs_port *port = (struct rs_port *)tty->driver_data;
#ifdef RS_DEBUG_THROTTLE
	char	buf[64];
	
	printk("unthrottle %s: %d....\n", tty_name(tty, buf),
	       tty->ldisc.chars_in_buffer(tty));
#endif

	func_enter();
	
	if (I_IXOFF(tty)) {
		if (port->x_char)
			port->x_char = 0;
		else
			rs_send_xchar(tty, START_CHAR(tty));
	}

	func_exit();
}





/* ********************************************************************** *
 *                    Here are the initialization routines.               *
 * ********************************************************************** */

void * ckmalloc (int size)
{
        void *p;

        p = kmalloc(size, GFP_KERNEL);
        if (p) 
                memset(p, 0, size);
        return p;
}



static int rs_init_portstructs(void)
{
	struct rs_port *port;
	int i;

	func_enter();

	/* Many drivers statically allocate the maximum number of ports
	   There is no reason not to allocate them dynamically. Is there? -- REW */
	rs_ports          = ckmalloc(RS_NPORTS * sizeof (struct rs_port));
	if (!rs_ports)
		return -ENOMEM;

	rs_termios        = ckmalloc(RS_NPORTS * sizeof (struct termios *));
	if (!rs_termios) {
		kfree (rs_ports);
		return -ENOMEM;
	}

	rs_termios_locked = ckmalloc(RS_NPORTS * sizeof (struct termios *));
	if (!rs_termios_locked) {
		kfree (rs_ports);
		kfree (rs_termios);
		return -ENOMEM;
	}

	/* Adjust the values in the "driver" */
	rs_driver.termios = rs_termios;
	rs_driver.termios_locked = rs_termios_locked;

	port = rs_ports;
	for (i=0; i < RS_NPORTS;i++) {
		rs_dprintk (RS_DEBUG_INIT, "initing port %d\n", i);
		port->gs.callout_termios = tty_std_termios;
		port->gs.normal_termios	= tty_std_termios;
		port->gs.magic = SERIAL_MAGIC;
		port->gs.close_delay = HZ/2;
		port->gs.closing_wait = 30 * HZ;
		port->gs.rd = &rs_real_driver;
#ifdef NEW_WRITE_LOCKING
		port->gs.port_write_sem = MUTEX;
#endif
#ifdef DECLARE_WAITQUEUE
		init_waitqueue_head(&port->gs.open_wait);
		init_waitqueue_head(&port->gs.close_wait);
#endif
		port->base = (i == 0) ? (unsigned long *) &UartA_Ctrl1 :
			(unsigned long *) &UartB_Ctrl1;
		port->intshift = (i == 0) ? UARTA_SHIFT : UARTB_SHIFT;
		rs_dprintk (RS_DEBUG_INIT, "base %p intshift %d\n",
			    port->base, port->intshift);
		port++;
	}

	func_exit();
	return 0;
}

static int rs_init_drivers(void)
{
	int error;

	func_enter();

	memset(&rs_driver, 0, sizeof(rs_driver));
	rs_driver.magic = TTY_DRIVER_MAGIC;
	rs_driver.driver_name = "serial";
	rs_driver.name = "ttyS";
	rs_driver.major = TTY_MAJOR;
	rs_driver.minor_start = 64;
	rs_driver.num = RS_NPORTS;
	rs_driver.type = TTY_DRIVER_TYPE_SERIAL;
	rs_driver.subtype = SERIAL_TYPE_NORMAL;
	rs_driver.init_termios = tty_std_termios;
	rs_driver.init_termios.c_cflag =
	  //B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	  B115200 | CS8 | CREAD | HUPCL | CLOCAL;
	rs_driver.flags = TTY_DRIVER_REAL_RAW;
	rs_driver.refcount = &rs_refcount;
	rs_driver.table = rs_table;
	rs_driver.termios = rs_termios;
	rs_driver.termios_locked = rs_termios_locked;

	rs_driver.open	= rs_open;
	rs_driver.close = gs_close;
	rs_driver.write = gs_write;
	rs_driver.put_char = gs_put_char; 
	rs_driver.flush_chars = gs_flush_chars;
	rs_driver.write_room = gs_write_room;
	rs_driver.chars_in_buffer = gs_chars_in_buffer;
	rs_driver.flush_buffer = gs_flush_buffer;
	rs_driver.ioctl = rs_ioctl;
	rs_driver.throttle = rs_throttle;
	rs_driver.unthrottle = rs_unthrottle;
	rs_driver.set_termios = gs_set_termios;
	rs_driver.stop = gs_stop;
	rs_driver.start = gs_start;
	rs_driver.hangup = gs_hangup;

	rs_callout_driver = rs_driver;
	rs_callout_driver.name = "cua";
	rs_callout_driver.major = TTYAUX_MAJOR;
	rs_callout_driver.subtype = SERIAL_TYPE_CALLOUT;

	if ((error = tty_register_driver(&rs_driver))) {
		printk(KERN_ERR "Couldn't register serial driver, error = %d\n",
		       error);
		return 1;
	}
	if ((error = tty_register_driver(&rs_callout_driver))) {
		tty_unregister_driver(&rs_driver);
		printk(KERN_ERR "Couldn't register callout driver, error = %d\n",
		       error);
		return 1;
	}

	func_exit();
	return 0;
}


static void rs_release_drivers(void)
{
	func_enter();
	tty_unregister_driver(&rs_driver);
	tty_unregister_driver(&rs_callout_driver);
#ifdef CONFIG_PM
        pm_unregister(pmdev);
#endif
	func_exit();
}

#if defined(CONFIG_VTECH_HELIO) && defined(CONFIG_PM)
static
int pm_request(struct pm_dev* dev, pm_request_t req, void* data)
{
	static unsigned long ctrlA;
	static unsigned long ctrlB;
	static unsigned long clk;
	static unsigned long out;

	switch (req) {
	 case PM_SUSPEND:
		/* disable both uarts */
		ctrlA = UartA_Ctrl1 & (UART_ENABLE | UART_DIS_TXD);
		UartA_Ctrl1 &= ~UART_ENABLE;
#if 0 /* would be nice if this worked, but it hangs the suspend process for me - 20001030 nop */
		while (UartA_Ctrl1 & UART_ON)
			/*wait till its empty */;
#endif
		UartA_Ctrl1 |= UART_DIS_TXD;
		ctrlB = UartB_Ctrl1 & (UART_ENABLE | UART_DIS_TXD);
		UartB_Ctrl1 &= ~UART_ENABLE;
#if 0 /* would be nice if this worked */
		while (UartB_Ctrl1 & UART_ON)
			/*wait till its empty */;
#endif
		UartB_Ctrl1 |= UART_DIS_TXD;

		/* turn the clocks off */
		clk = ClockControl & (CLK_EN_UART_A | CLK_EN_UART_B);
		ClockControl &= ~(CLK_EN_UART_A | CLK_EN_UART_B);

		/* remember the state of these pins */
		out = MFIOOutput & (MFIO_PIN_UART_TX_ENABLE |
				    MFIO_PIN_UART_RX_DISABLE |
				    MFIO_PIN_MODEM_RTS);

		MFIOOutput    &= ~MFIO_PIN_UART_TX_ENABLE;
		MFIOOutput    |= (MFIO_PIN_UART_RX_DISABLE |  /* Set High (Disable RX) */
				  MFIO_PIN_MODEM_RTS);
		break;

	 case PM_RESUME:

		/* restore the driver pin state */
		MFIOOutput = (MFIOOutput & ~(MFIO_PIN_UART_TX_ENABLE |
					     MFIO_PIN_UART_RX_DISABLE |
					     MFIO_PIN_MODEM_RTS)) | out;

		/* restore the clock */
		ClockControl = (ClockControl & ~(CLK_EN_UART_A | CLK_EN_UART_B)) | clk;

		/* restore the Uart state */
		UartA_Ctrl1 = (UartA_Ctrl1 & ~(UART_ENABLE | UART_DIS_TXD))
			      | ctrlA;
		UartB_Ctrl1 = (UartB_Ctrl1 & ~(UART_ENABLE | UART_DIS_TXD))
			      | ctrlB;
		break;
	}
	return 0;
}
#endif


int __init rs_init(void)
{
	int rc;

	func_enter();
	rs_dprintk (RS_DEBUG_INIT, "Initing serial module... (rs_debug=%d)\n", rs_debug);

	if (abs ((long) (&rs_debug) - rs_debug) < 0x10000) {
		printk (KERN_WARNING "rs: rs_debug is an address, instead of a value. "
		        "Assuming -1.\n");
		printk ("(%p)\n", &rs_debug);
		rs_debug=-1;
	}

	rc = rs_init_portstructs ();
	rs_init_drivers ();
	if (request_irq(2, rs_tx_interrupt_uarta, SA_SHIRQ | SA_INTERRUPT,
			"serial", &rs_ports[0])) {
		printk(KERN_ERR "rs: Cannot allocate irq for UARTA.\n");
		rc = 0;
	}
	if (request_irq(3, rs_rx_interrupt_uarta, SA_SHIRQ | SA_INTERRUPT,
			"serial", &rs_ports[0])) {
		printk(KERN_ERR "rs: Cannot allocate irq for UARTA.\n");
		rc = 0;
	}

	IntEnable6 |= INT6_UARTARXINT; 
	rs_enable_rx_interrupts(&rs_ports[0]); 

#ifndef CONFIG_SERIAL_CONSOLE
	printk( "Initializing uart...\n" );
	earlyInitUartPR31700();
	printk( "okay\n" );
#endif

	/* Note: I didn't do anything to enable the second UART */

	if (rc >= 0) 
		rs_initialized++;

	func_exit();
	return 0;
}


void rs_exit(void)
{
	func_enter();
	rs_dprintk (RS_DEBUG_CLEANUP, "Cleaning up drivers (%d)\n", rs_initialized);
	if (rs_initialized)
		rs_release_drivers ();

	kfree (rs_ports);
	kfree (rs_termios);
	kfree (rs_termios_locked);
	func_exit();

}

module_init(rs_init);
module_exit(rs_exit);


#ifdef DEBUG
void my_hd (unsigned char *addr, int len)
{
	int i, j, ch;

	for (i=0;i<len;i+=16) {
		printk ("%08x ", (int) addr+i);
		for (j=0;j<16;j++) {
			printk ("%02x %s", addr[j+i], (j==7)?" ":"");
		}
		for (j=0;j<16;j++) {
			ch = addr[j+i];
			printk ("%c", (ch < 0x20)?'.':((ch > 0x7f)?'.':ch));
		}
		printk ("\n");
	}
}
#endif




/*
 *
 *  Very simple routines to get UART humming...
 *
 */

/* not static, its called from prom_init() too */
void earlyInitUartPR31700(void)
{
	/* Setup master clock for UART */
	ClockControl &= ~CLK_SIBMCLKDIV_MASK;
	ClockControl |= CLK_SIBMCLKDIR | CLK_ENSIBMCLK |
		((2 << CLK_SIBMCLKDIV_SHIFT) & CLK_SIBMCLKDIV_MASK) |
		CLK_CSERSEL;

	/* Configure UARTA clock */
	ClockControl |= ((3 << CLK_CSERDIV_SHIFT) & CLK_CSERDIV_MASK) |
		CLK_ENCSERCLK | CLK_EN_UART_A;

	/* Setup UARTA for 115200 baud, 8N1 */
	UartA_Ctrl1 &= 0xf000000f;
	UartA_Ctrl1 &= ~UART_DIS_TXD;     /* turn on txd */
	UartA_Ctrl1 &= ~SER_SEVEN_BIT;    /* use 8-bit data */
	UartA_Ctrl1 &= ~SER_EVEN_PARITY;  /* no parity */
	UartA_Ctrl1 &= ~SER_TWO_STOP;     /* 1 stop bit */
	UartA_Ctrl2 = SER_BAUD_115200;
	UartA_DMActl1 = 0;                /* No DMA */
	UartA_DMActl2 = 0;                /* No DMA */
	UartA_Ctrl1 |= UART_ON;           /* Turn UART on */

	while (~UartA_Ctrl1 & UART_ON);
}

void serial_outc(unsigned char c)
{
	int i;
	unsigned long int2;
	#define BUSY_WAIT 10000

	/*
	 * Turn UART A Interrupts off
	 */
	int2 = IntEnable2;
	IntEnable2 &=
		~(INT2_UARTATXINT | INT2_UARTATXOVERRUN | INT2_UARTAEMPTY);

	/*
	 * The UART_TX_EMPTY bit in UartA_Ctrl1 seems
	 * not to be very reliable :-(
	 *
	 * Wait for the Tx register to become empty
	 */
	for (i = 0; !(IntStatus2 & INT2_UARTATXINT) && (i < BUSY_WAIT); i++);

	IntClear2 = INT2_UARTATXINT | INT2_UARTATXOVERRUN | INT2_UARTAEMPTY;

	UartA_Data = c;
	for (i = 0; !(IntStatus2 & INT2_UARTATXINT) && (i < BUSY_WAIT); i++);
	IntClear2 = INT2_UARTATXINT | INT2_UARTATXOVERRUN | INT2_UARTAEMPTY;

	IntEnable2 = int2;
}

void serial_console_read_raw(struct console *co, char *buf, int size)
{
	int i;
	unsigned int int2, flags;

	save_and_cli(flags);

	int2 = IntEnable2;
	IntEnable2 = 0;

	for (i=0; i<size; i++) {
		while (!(UartA_Ctrl1 & UART_RX_HOLD_FULL))
			;
		buf[i] = UartA_Data;
		udelay(10); /* Allow things to happen - it take a while */
	}
	IntEnable2 = int2;
	restore_flags(flags);
}

int serial_console_wait_key(struct console *co)
{
	unsigned int int2, res;

	int2 = IntEnable2;
	IntEnable2 = 0;

	while (!(UartA_Ctrl1 & UART_RX_HOLD_FULL))
		;
	res = UartA_Data;
	udelay(10); /* Allow things to happen - it take a while */
	
	IntEnable2 = int2;
	return res;
}

/* used in slip? in Slip??? SLIP /ought/ to work with sl->tty! (PM2000) */
void serial_console_write_raw(struct console *co, const char *s,
			       unsigned count)
{
    	unsigned int i;

	for (i = 0; i < count; i++) {
		serial_outc(*s++);
    	}
}

#ifdef CONFIG_SERIAL_CONSOLE

void serial_console_write(struct console *co, const char *s,
			       unsigned count)
{
    	unsigned int i;

	for (i = 0; i < count; i++) {
		if (*s == '\n')
			serial_outc('\r');
		serial_outc(*s++);
    	}
}

static kdev_t serial_console_device(struct console *c)
{
    return MKDEV(TTY_MAJOR, 64 + c->index);
}

static __init int serial_console_setup(struct console *co, char *options)
{
	earlyInitUartPR31700();

	return 0;
}


static struct console sercons = {
	name:     "ttyS",
	write:    serial_console_write,
	device:   serial_console_device,
	wait_key: serial_console_wait_key,
	setup:    serial_console_setup,
	flags:    CON_PRINTBUFFER,
	index:    -1
};

/*
 *    Register console.
 */

void __init serial_console_init(void)
{
    register_console(&sercons);
}

#endif
