/*
 *	linux/arch/mips/ioport.c
 *
 *  Functions not implemented for Linux/MIPS
 */
#include <linux/linkage.h>
#include <linux/errno.h>

asmlinkage int sys_ioperm(unsigned long from, unsigned long num, int turn_on)
{
	return -ENOSYS;
}

asmlinkage int sys_iopl(long ebx,long ecx,long edx,
	     long esi, long edi, long ebp, long eax, long ds,
	     long es, long fs, long gs, long orig_eax,
	     long eip,long cs,long eflags,long esp,long ss)
{
	return -ENOSYS;
}
