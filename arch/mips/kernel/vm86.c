/*
 *  arch/mips/kernel/vm86.c
 *
 *  Copyright (C) 1994, 1996  Waldorf GMBH,
 *  written by Ralf Baechle
 */
#include <linux/linkage.h>
#include <linux/errno.h>

struct vm86_struct;

asmlinkage int sys_vm86(struct vm86_struct * v86)
{
	return -ENOSYS;
}
