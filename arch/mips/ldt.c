/*
 * arch/mips/ldt.c
 *
 * Copyright (C) 1994 by Waldorf GMBH,
 * written by Ralf Baechle
 */
#include <linux/linkage.h>
#include <linux/errno.h>

asmlinkage int sys_modify_ldt(int func, void *ptr, unsigned long bytecount)
{
	return -ENOSYS;
}
