/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/init.h>
#include <linux/console.h>
#include <linux/kdev_t.h>
#include <linux/major.h>

#include <asm/serial.h>

/* SUPERIO uart register map */
typedef volatile struct uartregs {
	union {
		volatile u8	rbr;	/* read only, DLAB == 0 */
		volatile u8	thr;	/* write only, DLAB == 0 */
		volatile u8	dll;	/* DLAB == 1 */
	} u1;
	u8 __pad0[3];
	union {
		volatile u8	ier;	/* DLAB == 0 */
		volatile u8	dlm;	/* DLAB == 1 */
	} u2;
	u8 __pad1[3];
	union {
		volatile u8	iir;	/* read only */
		volatile u8	fcr;	/* write only */
	} u3;
	u8 __pad2[3];
	volatile u8	    iu_lcr;
	u8 __pad3[3];
	volatile u8	    iu_mcr;
	u8 __pad4[3];
	volatile u8	    iu_lsr;
	u8 __pad5[3];
	volatile u8	    iu_msr;
	u8 __pad6[3];
	volatile u8	    iu_scr;
	u8 __pad7[3];
} ioc3_uregs_t;

#define iu_rbr u1.rbr
#define iu_thr u1.thr
#define iu_dll u1.dll
#define iu_ier u2.ier
#define iu_dlm u2.dlm
#define iu_iir u3.iir
#define iu_fcr u3.fcr

void ns_putchar(char c)
{
	struct uartregs *uart = (struct uartregs *) CP7000_SERIAL1_BASE;

	while ((uart->iu_lsr & 0x20) == 0);
	uart->iu_thr = c;
}

char __init prom_getchar(void)
{
	return 0;
}

//static void
void
ns_console_write(struct console *con, const char *s, unsigned n)
{
	int i;

	/* Somewhat oversimplified because only used during early startup.  */
	for (i = 0; i < n; i++, s++) {
		ns_putchar(*s);
		if (*s == 10)
			ns_putchar(13);
	}
}

static kdev_t 
ns_console_dev(struct console *c)
{
	return MKDEV(TTY_MAJOR, 64 + c->index);
}

static struct console ns_console = {
    name:	"ns16552",
    write:	ns_console_write,
    device:	ns_console_dev,
    flags:	CON_PRINTBUFFER,
    index:	-1,
};

__init void ns_setup_console(void)
{
	register_console(&ns_console);
}
