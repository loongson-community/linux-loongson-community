/*
 * MIPS specific syscall handling functions and syscalls
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995, 1996 by Ralf Baechle
 *
 * TODO:  Implement the compatibility syscalls.
 *        Don't waste that much memory for empty entries in the syscall
 *        table.
 */
#undef CONF_PRINT_SYSCALLS

#include <linux/linkage.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <asm/branch.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <asm/signal.h>

extern asmlinkage void syscall_trace(void);
typedef asmlinkage int (*syscall_t)(void *a0,...);
extern asmlinkage int do_syscalls(struct pt_regs *regs, syscall_t fun,
                                  int narg);
extern syscall_t sys_call_table[];
extern unsigned char sys_narg_table[];

/*
 * The pipe syscall has a unusual calling convention.  We return the two
 * filedescriptors in the result registers v0/v1.  The syscall wrapper
 * from libc places these results in the array to which the argument of
 * pipe points to.  This is like other MIPS operating systems and unlike
 * Linux/i386 where the kernel itself places the results in the file
 * descriptor array itself.  This calling convention also has the advantage
 * of lower overhead because we don't need to call verify_area.
 */
asmlinkage int sys_pipe(struct pt_regs *regs)
{
	int fd[2];
	int error;

	error = do_pipe(fd);
	if (error)
		return error;
	regs->regs[3] = fd[1];
	return fd[0];
}

asmlinkage unsigned long sys_mmap(unsigned long addr, size_t len, int prot,
                                  int flags, int fd, off_t offset)
{
	struct file * file = NULL;

	if (!(flags & MAP_ANONYMOUS)) {
		if (fd >= NR_OPEN || !(file = current->files->fd[fd]))
			return -EBADF;
	}
	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);

	return do_mmap(file, addr, len, prot, flags, offset);
}

asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;

	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
		/*
		 * Not all MIPS R-series CPUs have the wait instruction.
		 * FIXME: We should save power by reducing the clock where
		 *        possible.
		 */
		if (wait_available && !need_resched)
			__asm__(".set\tmips3\n\t"
				"wait\n\t"
				".set\tmips0\n\t");
		schedule();
	}
}

#if 0
/*
 * RISC/os compatible SysV flavoured fork(2) syscall.
 *
 * This call has a different calling sequence:
 * child return value: pid of parent, secondary result = 1.
 * parent return value: pid of child, secondary result value = 0.
 * error: errno, secondary result = 0.
 */
asmlinkage int sys_sysv_fork(struct pt_regs *regs)
{
	int pid;

	pid = do_fork(SIGCHLD, regs->regs[29], regs);
	if (pid == 0) {				/* child */
		regs->regs[3] = 1;
		return current->p_pptr->pid;
	}	 				/* parent or error */

	regs->regs[3] = 0;
	return pid;
}
#endif

/*
 * Normal Linux fork(2) syscall
 */
asmlinkage int sys_fork(struct pt_regs *regs)
{
	return do_fork(SIGCHLD, regs->regs[29], regs);
}

asmlinkage int sys_clone(struct pt_regs *regs)
{
	unsigned long clone_flags;
	unsigned long newsp;

	clone_flags = regs->regs[4];
	newsp = regs->regs[5];
	if (!newsp)
		newsp = regs->regs[29];
	return do_fork(clone_flags, newsp, regs);
}

/*
 * sys_execve() executes a new program.
 */
asmlinkage int sys_execve(struct pt_regs *regs)
{
	int error;
	char * filename;

	error = getname((char *) (long)regs->regs[4], &filename);
	if (error)
		return error;
	error = do_execve(filename, (char **) (long)regs->regs[5],
	                  (char **) (long)regs->regs[6], regs);
	putname(filename);

	return error;
}

/*
 * Do the indirect syscall syscall.
 */
asmlinkage int sys_syscall(unsigned long a0, unsigned long a1, unsigned long a2,
                           unsigned long a3, unsigned long a4, unsigned long a5,
                           unsigned long a6)
{
	syscall_t syscall;

	if (a0 > __NR_Linux + __NR_Linux_syscalls)
		return -ENOSYS;

	syscall = sys_call_table[a0];
	/*
	 * Prevent stack overflow by recursive
	 * syscall(__NR_syscall, __NR_syscall,...);
	 */
	if (syscall == (syscall_t) sys_syscall)
		return -EINVAL;

	if (syscall == NULL)
		return -ENOSYS;

	return syscall((void *)a0, a1, a2, a3, a4, a5, a6);
}

/*
 * If we ever come here the user sp is bad.  Zap the process right away.
 * Due to the bad stack signaling wouldn't work.
 */
asmlinkage void bad_stack(void)
{
	do_exit(SIGSEGV);
}

#ifdef CONF_PRINT_SYSCALLS
#define SYS(fun, narg) #fun,
static char *sfnames[] = {
#include "syscalls.h"
};
#endif

asmlinkage void do_sys(struct pt_regs *regs)
{
	unsigned long syscallnr, usp;
	syscall_t syscall;
	int errno, narg;

	/* Skip syscall instruction */
	if (delay_slot(regs)) {
		/*
		 * By convention "li v0,<syscallno>" is always preceeding
		 * the syscall instruction.  So if we're in a delay slot
		 * userland is screwed up.
		 */
		force_sig(SIGILL, current);
		return;
	}
	regs->cp0_epc += 4;

	syscallnr = regs->regs[2];
	if (syscallnr > (__NR_Linux + __NR_Linux_syscalls))
		goto illegal_syscall;
	syscall = sys_call_table[syscallnr];

#ifdef CONF_PRINT_SYSCALLS
	printk("do_sys(): %s()", sfnames[syscallnr - __NR_Linux]);
#endif
	narg = sys_narg_table[syscallnr];
	if (narg > 4) {
		/*
		 * Verify that we can safely get the additional parameters
		 * from the user stack.
		 */
		usp = regs->regs[29];
		if (usp & 3) {
			printk("unaligned usp\n");
			do_exit(SIGBUS);
			return;
		}

		if (!access_ok(VERIFY_READ, (void *) (usp + 16),
		              (narg - 4) * sizeof(unsigned long))) {
			errno = -EFAULT;
			goto syscall_error;
		}
	}

	if ((current->flags & PF_TRACESYS) == 0) {
		errno = do_syscalls(regs, syscall, narg);
		if (errno < 0)
			goto syscall_error;

		regs->regs[2] = errno;
		regs->regs[7] = 0;
	} else {
		syscall_trace();

		errno = do_syscalls(regs, syscall, narg);
		if (errno < 0) {
			regs->regs[2] = -errno;
			regs->regs[7] = 1;
		} else {
			regs->regs[2] = errno;
			regs->regs[7] = 0;
		}

		syscall_trace();
	}
#ifdef CONF_PRINT_SYSCALLS
	printk(" returning: normal\n");
#endif
	return;

syscall_error:
	regs->regs[2] = -errno;
	regs->regs[7] = 1;
#ifdef CONF_PRINT_SYSCALLS
	printk(" returning: syscall_error, errno=%d\n", -errno);
#endif
	return;

illegal_syscall:
	regs->regs[2] = ENOSYS;
	regs->regs[7] = 1;
#ifdef CONF_PRINT_SYSCALLS
	printk(" returning: illegal_syscall\n");
#endif
	return;
}
