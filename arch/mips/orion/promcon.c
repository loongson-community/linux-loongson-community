/*
 * Wrap-around code for a console using the
 * SGI PROM io-routines.
 *
 * Copyright (c) 1999 Ulf Carlsson
 *
 * Derived from DECstation promcon.c
 * Copyright (c) 1998 Harald Koerfgen 
 */
#include <linux/tty.h>
#include <linux/major.h>
#include <linux/ptrace.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/fs.h>

extern void prom_printf(char *fmt, ...);
unsigned long splx(unsigned long mask){return 0;}
#if 0
unsigned long ramsize=0x100000;
unsigned long RamSize(){return ramsize;}
extern void prom_printf(char *fmt, ...);
unsigned long splx(unsigned long mask){return 0;}
long PssSetIntHandler(unsigned long intnum, void *handler){}
long PssEnableInt(unsigned long intnum){}
long PssDisableInt(unsigned long intnum){}
unsigned long t_ident(char name[4], unsigned long node, unsigned long *tid){}
#endif

extern void  SerialPollConout(unsigned char c);
static void prom_console_write(struct console *co, const char *s,
			       unsigned count)
{
	unsigned i;

	/*
	 *    Now, do each character
	 */
	for (i = 0; i < count; i++) {
		if (*s == 10)
			SerialPollConout(13);
		SerialPollConout(*s++);
	}
}

extern int prom_getchar(void);
static int prom_console_wait_key(struct console *co)
{
	return prom_getchar();
}

extern void SerialPollInit(void);
extern void  SerialSetup(unsigned long  baud, unsigned long  console, unsigned long  host, unsigned long  intr_desc);
static int __init prom_console_setup(struct console *co, char *options)
{
	SerialSetup(19200,1,1,3);
	SerialPollInit();
	SerialPollOn();

	return 0;
}

static kdev_t prom_console_device(struct console *c)
{
	return MKDEV(TTY_MAJOR, 64 + c->index);
}

static struct console sercons =
{
    name:	"ttyS",
    write:	prom_console_write,
    device:	prom_console_device,
    wait_key:	prom_console_wait_key,
    setup:	prom_console_setup,
    flags:	CON_PRINTBUFFER,
    index:	-1,
};

/*
 *    Register console.
 */
extern void zs_serial_console_init();
void serial_console_init(void)
{
	register_console(&sercons);
	/*zs_serial_console_init();*/
}

void prom_putchar(char c);

static char ppbuf[1000];

void prom_printf(char *fmt, ...)
{
	va_list args;
	char ch, *bptr;
	int i;

	va_start(args, fmt);
	i = vsprintf(ppbuf, fmt, args);

	bptr = ppbuf;

	while((ch = *(bptr++)) != 0) {
		if(ch == '\n')
			prom_putchar('\r');

		prom_putchar(ch);
	}
	va_end(args);
	return;
}

#define kSCC_Control	0xbf200008
#define kSCC_Data	(kSCC_Control + 4)

#define Tx_BUF_EMP      0x4     /* Tx Buffer empty */

static inline void RegisterDelay(void)
{
	int delay = 2*125;	/* Assuming 4us clock.  */
	while (delay--);
}

static inline unsigned char read_zsreg(unsigned char reg)
{
	unsigned char retval;

	if (reg != 0) {
		*(volatile unsigned char *)kSCC_Control = reg & 0xf;
		RegisterDelay();
	}

	retval = *(volatile unsigned char *) kSCC_Control;
	RegisterDelay();

	return retval;
}

static inline void write_zsreg(unsigned char reg, unsigned char value)
{
	if (reg != 0) {
		*((volatile unsigned char *) (kSCC_Control)) = reg & 0xf ;
		RegisterDelay();
	}

	*((volatile unsigned char *) (kSCC_Control)) = value ;
	RegisterDelay();
}

static inline unsigned char read_zsdata(void)
{
	unsigned char retval;

	retval = *(volatile unsigned char *)kSCC_Data;
	RegisterDelay();

	return retval;
}

static inline void write_zsdata(unsigned char value)
{
	*(volatile unsigned char *)kSCC_Data = value;
	RegisterDelay();

        return;
}

void prom_putchar(char c)
{
	while ((read_zsreg(0) & Tx_BUF_EMP) == 0)
		RegisterDelay();
	write_zsdata(c);
}

int prom_getchar(void)
{
	return 0;
}
