#ifndef __ASM_BUG_H
#define __ASM_BUG_H

#include <asm/break.h>

#define BUG()								\
do {									\
	printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__);		\
	__asm__ __volatile__("break %0" : : "i" (BRK_BUG));		\
} while (0)

#define PAGE_BUG(page) do {  BUG(); } while (0)

#endif
