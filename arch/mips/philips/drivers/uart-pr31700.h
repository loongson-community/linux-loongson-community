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
 * $Id: r39xx_serial.h,v 1.3 2000/11/01 03:02:27 nop Exp $
 */

#include <linux/serial.h>
#include <linux/generic_serial.h>


/* r39xx UART Register Offsets */
#define UART_R39XX_CTRL1            0
#define UART_R39XX_CTRL2            1
#define UART_R39XX_DMACTRL1         2
#define UART_R39XX_DMACTRL2         3
#define UART_R39XX_DMACNT           4
#define UART_R39XX_DATA             5

/* UART Interrupt (Interrupt 2) bits (UARTA,UARTB) */
#define UART_RX_INT         9  /* receiver holding register full  (31, 21) */
#define UART_RXOVERRUN_INT  8  /* receiver overrun error          (30, 20) */
#define UART_FRAMEERR_INT   7  /* receiver frame error            (29, 19) */
#define UART_BREAK_INT      6  /* received break signal           (28, 18) */
#define UART_PARITYERR_INT  5  /* receiver parity error           (27, 17) */
#define UART_TX_INT         4  /* transmit holding register empty (26, 16) */
#define UART_TXOVERRUN_INT  3  /* transmit overrun error          (25, 15) */
#define UART_EMPTY_INT      2  /* both trans/recv regs empty      (24, 14) */
#define UART_DMAFULL_INT    1  /* DMA at end of buffer            (23, 13) */
#define UART_DMAHALF_INT    0  /* DMA halfway through buffer */   (22, 12) */

#define UARTA_SHIFT        22
#define UARTB_SHIFT        12

#define INT2BIT(interrupttype, intshift)  (1 << (interrupttype + intshift))
#define INTTYPE(interrupttype)            (1 << interrupttype)

/* Driver status flags */
#define RS_STAT_RX_TIMEOUT     0x01
#define RS_STAT_DMA_RX         0x02
#define RS_STAT_DMA_TX         0x04
#define RS_STAT_NEVER_DMA      0x08
#define RS_STAT_USE_PIO        0x10

/* 
   This driver can spew a whole lot of debugging output at you. If you
   need maximum performance, you should disable the DEBUG define. To
   aid in debugging in the field, I'm leaving the compile-time debug
   features enabled, and disable them "runtime". That allows me to
   instruct people with problems to enable debugging without requiring
   them to recompile... 
*/
#define DEBUG


#ifdef DEBUG
#define rs_dprintk(f, str...) if (rs_debug & f) printk (str)
#else
#define rs_dprintk(f, str...) /* nothing */
#endif



#define func_enter() rs_dprintk (RS_DEBUG_FLOW, "rs: enter " __FUNCTION__ "\n")
#define func_exit()  rs_dprintk (RS_DEBUG_FLOW, "rs: exit  " __FUNCTION__ "\n")

#define func_enter2() rs_dprintk (RS_DEBUG_FLOW, "rs: enter " __FUNCTION__ \
                                  "(port %p, base %p)\n", port, port->base)


/* Debug flags. Add these together to get more debug info. */

#define RS_DEBUG_OPEN          0x00000001
#define RS_DEBUG_SETTING       0x00000002
#define RS_DEBUG_FLOW          0x00000004
#define RS_DEBUG_MODEMSIGNALS  0x00000008
#define RS_DEBUG_TERMIOS       0x00000010
#define RS_DEBUG_TRANSMIT      0x00000020
#define RS_DEBUG_RECEIVE       0x00000040
#define RS_DEBUG_INTERRUPTS    0x00000080
#define RS_DEBUG_PROBE         0x00000100
#define RS_DEBUG_INIT          0x00000200
#define RS_DEBUG_CLEANUP       0x00000400
#define RS_DEBUG_CLOSE         0x00000800
#define RS_DEBUG_FIRMWARE      0x00001000
#define RS_DEBUG_MEMTEST       0x00002000
#define RS_DEBUG_THROTTLE      0x00004000

#define RS_DEBUG_ALL           0xffffffff


#define RS_NPORTS  2

struct rs_port { 	
        /* must be first field! */
	struct gs_port          gs; 

	/* rest is private for this driver */
	unsigned long           *base;
	int                     intshift;  /* for interrupt register */
	struct wait_queue       *shutdown_wait; 
	int                     stat_flags;
        struct async_icount     icount;   /* kernel counters for the 4
					     input interrupts */
	int                     read_status_mask;
	int                     ignore_status_mask;
	int                     x_char;   /* xon/xoff character */
}; 



#define SERIAL_MAGIC 0x5301
