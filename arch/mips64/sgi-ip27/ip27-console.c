/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/init.h>
#include <linux/console.h>
#include <asm/sn/addrs.h>
#include <asm/sn/sn0/hub.h>
#include <asm/sn/klconfig.h>
#include <asm/ioc3.h>
#include <asm/sn/sn_private.h>

void ip27_putchar(char c)
{
	struct ioc3 *ioc3;
	struct ioc3_uartregs *uart;

	ioc3 = (struct ioc3 *)KL_CONFIG_CH_CONS_INFO(master_nasid)->memory_base;
	uart = &ioc3->sregs.uarta;

	while ((uart->iu_lsr & 0x20) == 0);
	uart->iu_thr = c;
}

static void
ip27prom_console_write(struct console *con, const char *s, unsigned n)
{
	char c;

	while (n--) {
		c = *(s++);
		if (c == '\n')
			ip27_putchar('\r');
		ip27_putchar(c);
	}
}

static struct console ip27_prom_console = {
	"prom",
	ip27prom_console_write,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	CON_PRINTBUFFER,
	-1,
	0,
	NULL
};

__init void ip27_setup_console(void)
{
	register_console(&ip27_prom_console);
}
