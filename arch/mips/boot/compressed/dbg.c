/*
 * MIPS-specific debug support for pre-boot environment
 */

#ifdef DEBUG

#include <linux/types.h>
#include <linux/serial_reg.h>
#include <linux/init.h>

#include "dbg.h"

static inline unsigned int serial_in(int offset)
{
	return *((char *)PORT(offset));
}

static inline void serial_out(int offset, int value)
{
	*((char *)PORT(offset)) = value;
}

int putc(char c)
{
	int timeout = 1024;

	while (((serial_in(UART_LSR) & UART_LSR_THRE) == 0) && (timeout-- > 0))
		;

	serial_out(UART_TX, c);

	return 1;
}

void puts(const char *s)
{
	char c;
	while ((c = *s++) != '\0') {
		putc(c);
		if (c == '\n')
			putc('\r');
	}
}

void puthex(unsigned long long val)
{

	unsigned char buf[10];
	int i;
	for (i = 7; i >= 0; i--) {
		buf[i] = "0123456789ABCDEF"[val & 0x0F];
		val >>= 4;
	}
	buf[8] = '\0';
	puts(buf);
}


#else
void puts(const char *s)
{
}
void puthex(unsigned long long val)
{
}
#endif
