/*
 *  linux/lib/open.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#define __LIBRARY__
#include <linux/unistd.h>
#include <stdarg.h>

int open(const char * filename, int flag, ...)
{
	register int res;
	va_list arg;

	va_start(arg,flag);
#if defined (__i386__)
	__asm__("movl %2,%%ebx\n\t"
		"int $0x80"
		:"=a" (res)
		:"0" (__NR_open),"g" ((long)(filename)),"c" (flag),
		"d" (va_arg(arg,int)));
#elif defined (__mips__)
	__asm__(".set	noat\n\t"
		"move	$2,%2\n\t"
		"move	$3,%3\n\t"
		"move	$4,%4\n\t"
		"li	$1,%1\n\t"
		"syscall\n\t"
		".set	at"
		:"=r" (res)
		:"i" (__NR_open),
		 "r" ((long)(filename)),
		 "r" (flag),
		 "r" (va_arg(arg,int))
		:"$1","$2","$3","$4");
#endif
	if (res>=0)
		return res;
	errno = -res;
	return -1;
}
