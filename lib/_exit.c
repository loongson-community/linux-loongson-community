/*
 *  linux/lib/_exit.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#define __LIBRARY__
#include <linux/unistd.h>

volatile void _exit(int exit_code)
{
fake_volatile:
#if defined (__i386__)
	__asm__("movl %1,%%ebx\n\t"
		"int $0x80"
		: /* no outputs */
		:"a" (__NR_exit),"g" (exit_code));
#elif defined (__mips__)
	__asm__(".set	noat\n\t"
		"move	$2,%1\n\t"
		"li	$1,%0\n\t"
		"syscall\n\t"
		".set	at"
		: /* no outputs */
		: "i" (__NR_exit), "r" (exit_code)
		: "$1","$2");
#endif
	goto fake_volatile;
}

