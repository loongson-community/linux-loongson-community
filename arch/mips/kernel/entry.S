/*
 * arch/mips/kernel/entry.S
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics
 * written by Ralf Baechle and Andreas Busse
 */

/*
 * entry.S contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch. The ISA dependend TLB
 * code is in arch/mips/kernel/tlb.S
 */

#include <linux/sys.h>

#include <asm/asm.h>
#include <asm/errno.h>
#include <asm/segment.h>
#include <asm/mipsregs.h>
#include <asm/mipsconfig.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/stackframe.h>
#include <asm/processor.h>

/*
 * These are offsets into the task-struct.
 */
state		=  0
counter		=  4
priority	=  8
signal		= 12
blocked		= 16
flags		= 20
errno		= 24
exec_domain	= 60

		.text
		.set	noreorder
		.align 4
handle_bottom_half:
		lui	s0,%hi(intr_count)
		lw	s1,%lo(intr_count)(s0)
		mfc0	s3,CP0_STATUS		# Enable IRQs
		addiu	s2,s1,1
		sw	s2,%lo(intr_count)(s0)
		ori	t0,s3,0x1f
		xori	t0,0x1e
		jal	do_bottom_half
		mtc0	t0,CP0_STATUS		# delay slot
		mtc0	s3,CP0_STATUS		# Restore old IRQ state
		j	9f
		sw	s1,%lo(intr_count)(s0)	# delay slot

reschedule:
		lui	ra,%hi(ret_from_sys_call)
		j	schedule
		addiu	ra,%lo(ret_from_sys_call)	# delay slot

		.align	5
		NESTED(handle_sys, FR_SIZE, sp)
		.set	noat
		SAVE_ALL
		STI
		.set	at
		/*
		 * Compute return address. We assume that syscalls never
		 * appear in delay slots. For the Linux/MIPS libc this
		 * assumption is always true.
		 */
		lw	t3,FR_EPC(sp)
		lw	s1,FR_REG2(sp)
		li	t0,-ENOSYS
		addiu	t3,4
		sw	t3,FR_EPC(sp)
		li	t2,NR_syscalls
		bge	s1,t2,ret_from_sys_call
		sw	t0,FR_REG2(sp)		# delay slot
		sll	s1,PTRLOG
		lw	s1,sys_call_table(s1)
		lw	s0,current

		beqz	s1,ret_from_sys_call
		lw	t0,flags(s0)		# delay slot
		sll	t0,26			# PF_TRACESYS
		bltz	t0,1f
		sw	zero,errno(s0)		# delay slot

#if 0
	lw	t0,FR_ORIG_REG2(sp)
	beq	t0,4,1f
	nop
	la	t0,sys_call_names
	lw	t1,FR_ORIG_REG2(sp)
	sll	t1,2
	addu	t0,t1
	lw	a1,(t0)
	PRINT("%s(")
	lw	a1,FR_REG4(sp)
	lw	a2,FR_REG5(sp)
	lw	a3,FR_REG6(sp)
	PRINT("%08lx, %08lx, %08lx, ")
	lw	a1,FR_REG7(sp)
	lw	a2,FR_EPC(sp)
	lw	a3,FR_REG31(sp)
	PRINT("%08lx) epc %08lx ra %08lx ")
1:
#endif
		lw	a0,FR_REG4(sp)
		lw	a1,FR_REG5(sp)
		lw	a2,FR_REG6(sp)
		lw	a3,FR_REG7(sp)
		lw	t0,FR_REG3(sp)
		jalr	s1			# do the real work
		sw	t0,PTRSIZE*4(sp)	# delay slot

#if 0
	lw	t0,FR_ORIG_REG2(sp)
	beq	t0,4,1f
	nop
	sw	v0,xxx
	lw	a1,xxx
	PRINT("res %08lx\n")
	lw	v0,xxx
	.data
xxx:	.word	0
	.text
1:
#endif

		lw	t0,errno(s0)
		sw	v0,FR_REG2(sp)		# save return value
		subu	t0,zero,t0
		beqz	t0,ret_from_sys_call
		nop				# delay slot
		/*
		 * Fixme: should set error flag
		 */
		j	ret_from_sys_call
		sw	t0,FR_REG2(sp)		# delay slot

		.align	4
1:		jal	syscall_trace
		nop				# delay slot

		lw	a0,FR_REG4(sp)
		lw	a1,FR_REG5(sp)
		lw	a2,FR_REG6(sp)
		lw	a3,FR_REG7(sp)
		lw	t0,FR_REG3(sp)
		jalr	s1			# do the real work
		sw	t0,PTRSIZE*4(sp)	# delay slot

		lw	t0,errno(s0)
		sw	v0,FR_REG2(sp)
		subu	t0,zero,t0		# delay slot
		beqz	t0,1f
		nop				# delay slot
		/*
		 * Fixme: should set error flag
		 */
1:		jal	syscall_trace
		sw	t0,FR_REG2(sp)		# delay slot

		.align	4
		.globl	ret_from_sys_call
ret_from_sys_call:
		lw	t0,intr_count		# bottom half
		bnez	t0,return
9:
		lw	t0,bh_mask		# delay slot
		lw	t1,bh_active		# unused delay slot
		and	t0,t1
		bnez	t0,handle_bottom_half

		lw	t0,FR_STATUS(sp)	# returning to kernel mode?
		andi	t1,t0,0x10
		beqz	t1,return		# -> yes

		mfc0	t0,CP0_STATUS		# delay slot
		lw	t1,need_resched
		ori	t0,0x1f			# enable irqs
		xori	t0,0x1e
		bnez	t1,reschedule
		mtc0	t0,CP0_STATUS		# delay slot

		lw	s0,current
		lw	t0,task
		lw	t1,state(s0)		# state
		beq	s0,t0,return		# task[0] cannot have signals
		lw	t0,counter(s0)		# counter
		bnez	t1,reschedule		# state == 0 ?
		lw	a0,blocked(s0)
						# save blocked in a0 for
						# signal handling
		beqz	t0,reschedule		# counter == 0 ?
		lw	t0,signal(s0)
		nor	t1,zero,a0
		and	t1,t0,t1
		beqz	t1,return
		nop

		jal	do_signal
		move	a1,sp			# delay slot
		
		.set	noat
		.globl	return
return:		RESTORE_ALL
		ERET
		.set	at
		END(handle_sys)

/*
 * Beware: interrupt, fast_interrupt and bad_interrupt have unusal
 * calling conventions!
 *
 * t1 - interrupt number
 * s2 - destroyed
 * return values:
 * v0 - return routine
 */
		.text
		.set	at
		.align	5
		NESTED(interrupt, FR_SIZE, sp)
		move	s2,ra
		mfc0	t0,CP0_STATUS			# enable IRQs
		ori	t0,0x1f
		xori	t0,0x1e
		mtc0	t0,CP0_STATUS
		move	a0,t1
		jal	do_IRQ
		move	a1,sp				# delay slot
		mfc0	t0,CP0_STATUS			# disable IRQs
		ori	t0,1
		xori	t0,1
		la	v0,ret_from_sys_call
		jr	s2
		mtc0	t0,CP0_STATUS			# delay slot
		END(interrupt)

		.align	5
		NESTED(fast_interrupt, FR_SIZE, sp)
		move	s2,ra
		move	a0,t1
		jal	do_fast_IRQ
		move	a1,sp				# delay slot
		lui	v0,%hi(return)
		jr	s2
		addiu	v0,%lo(return)			# delay slot
		END(fast_interrupt)

		LEAF(bad_interrupt)
		/*
		 * Don't return & unblock the pic
		 */
		j	return
		nop
		END(bad_interrupt)

		.align	5
		LEAF(spurious_interrupt)
		/*
		 * Nothing happened... (whistle)
		 */
		lui	t1,%hi(spurious_count)
		lw	t0,%lo(spurious_count)(t1)
		la	v0,return
		addiu	t0,1
		jr	ra
		sw	t0,%lo(spurious_count)(t1)
		END(spurious_interrupt)



/*
 * Build a default exception handler for the other R4x00 exceptions
 */
#define BUILD_HANDLER(exception)                                        \
		.align	5;                                              \
		NESTED(handle_##exception, FR_SIZE, sp);                \
		.set	noat;                                           \
		SAVE_ALL;                                               \
		STI;                                                    \
		.set	at;                                             \
		la	a1,8f;                                          \
		TEXT	(#exception);                                   \
		lw	a2,FR_EPC(sp);                                  \
		PRINT("Got %s at %08x.\n");                             \
		li	a0,0;                                           \
		li	t0,-1;                   /* not a sys call */   \
		sw	t0,FR_ORIG_REG2(sp);                            \
		jal	do_##exception;                                 \
		move	a0,sp;                   /* delay slot */       \
		j	ret_from_sys_call;                              \
		nop;                             /* delay slot */       \
		END(handle_##exception)

		BUILD_HANDLER(adel)
		BUILD_HANDLER(ades)
		BUILD_HANDLER(ibe)
		BUILD_HANDLER(dbe)
		BUILD_HANDLER(ov)
		BUILD_HANDLER(fpe)
		BUILD_HANDLER(bp)
		BUILD_HANDLER(tr)
		BUILD_HANDLER(ri)
		BUILD_HANDLER(cpu)
		BUILD_HANDLER(vcei)
		BUILD_HANDLER(vced)
		BUILD_HANDLER(watch)
		BUILD_HANDLER(reserved)


/*
 * Exception handler table with 32 entries.
 * This might be extended to handle software exceptions
 */
		.bss
		.align	2
		EXPORT(exception_handlers)
		.fill	32,4,0

/*
 * Table of syscalls
 */
		.data
		EXPORT(sys_call_table)
		PTR	sys_setup			/* 0 */
		PTR	sys_exit
		PTR	sys_fork
		PTR	sys_read
		PTR	sys_write
		PTR	sys_open			/* 5 */
		PTR	sys_close
		PTR	sys_waitpid
		PTR	sys_creat
		PTR	sys_link
		PTR	sys_unlink			/* 10 */
		PTR	sys_execve
		PTR	sys_chdir
		PTR	sys_time
		PTR	sys_mknod
		PTR	sys_chmod			/* 15 */
		PTR	sys_chown
		PTR	sys_break
		PTR	sys_stat
		PTR	sys_lseek
		PTR	sys_getpid			/* 20 */
		PTR	sys_mount
		PTR	sys_umount
		PTR	sys_setuid
		PTR	sys_getuid
		PTR	sys_stime			/* 25 */
		PTR	sys_ptrace
		PTR	sys_alarm
		PTR	sys_fstat
		PTR	sys_pause
		PTR	sys_utime			/* 30 */
		PTR	sys_stty
		PTR	sys_gtty
		PTR	sys_access
		PTR	sys_nice
		PTR	sys_ftime			/* 35 */
		PTR	sys_sync
		PTR	sys_kill
		PTR	sys_rename
		PTR	sys_mkdir
		PTR	sys_rmdir			/* 40 */
		PTR	sys_dup
		PTR	sys_pipe
		PTR	sys_times
		PTR	sys_prof
		PTR	sys_brk				/* 45 */
		PTR	sys_setgid
		PTR	sys_getgid
		PTR	sys_signal
		PTR	sys_geteuid
		PTR	sys_getegid			/* 50 */
		PTR	sys_acct
		PTR	sys_phys
		PTR	sys_lock
		PTR	sys_ioctl
		PTR	sys_fcntl			/* 55 */
		PTR	sys_mpx
		PTR	sys_setpgid
		PTR	sys_ulimit
		PTR	sys_olduname
		PTR	sys_umask			/* 60 */
		PTR	sys_chroot
		PTR	sys_ustat
		PTR	sys_dup2
		PTR	sys_getppid
		PTR	sys_getpgrp			/* 65 */
		PTR	sys_setsid
		PTR	sys_sigaction
		PTR	sys_sgetmask
		PTR	sys_ssetmask
		PTR	sys_setreuid			/* 70 */
		PTR	sys_setregid
		PTR	sys_sigsuspend
		PTR	sys_sigpending
		PTR	sys_sethostname
		PTR	sys_setrlimit			/* 75 */
		PTR	sys_getrlimit
		PTR	sys_getrusage
		PTR	sys_gettimeofday
		PTR	sys_settimeofday
		PTR	sys_getgroups			/* 80 */
		PTR	sys_setgroups
		PTR	sys_select
		PTR	sys_symlink
		PTR	sys_lstat
		PTR	sys_readlink			/* 85 */
		PTR	sys_uselib
		PTR	sys_swapon
		PTR	sys_reboot
		PTR	old_readdir
		PTR	sys_mmap			/* 90 */
		PTR	sys_munmap
		PTR	sys_truncate
		PTR	sys_ftruncate
		PTR	sys_fchmod
		PTR	sys_fchown			/* 95 */
		PTR	sys_getpriority
		PTR	sys_setpriority
		PTR	sys_profil
		PTR	sys_statfs
		PTR	sys_fstatfs			/* 100 */
		PTR	sys_ioperm
		PTR	sys_socketcall
		PTR	sys_syslog
		PTR	sys_setitimer
		PTR	sys_getitimer			/* 105 */
		PTR	sys_newstat
		PTR	sys_newlstat
		PTR	sys_newfstat
		PTR	sys_uname
		PTR	sys_iopl			/* 110 */
		PTR	sys_vhangup
		PTR	sys_idle
		PTR	sys_vm86
		PTR	sys_wait4
		PTR	sys_swapoff			/* 115 */
		PTR	sys_sysinfo
		PTR	sys_ipc
		PTR	sys_fsync
		PTR	sys_sigreturn
		PTR	sys_clone			/* 120 */
		PTR	sys_setdomainname
		PTR	sys_newuname
		PTR	0 #sys_modify_ldt
		PTR	sys_adjtimex
		PTR	sys_mprotect			/* 125 */
		PTR	sys_sigprocmask
		PTR	sys_create_module
		PTR	sys_init_module
		PTR	sys_delete_module
		PTR	sys_get_kernel_syms		/* 130 */
		PTR	sys_quotactl
		PTR	sys_getpgid
		PTR	sys_fchdir
		PTR	sys_bdflush
		PTR	sys_sysfs			/* 135 */
		PTR	sys_personality
		PTR	0			/* for afs_syscall */
		PTR	sys_setfsuid
		PTR	sys_setfsgid
		PTR	sys_llseek			/* 140 */
		PTR	sys_getdents
		PTR	sys_select
		PTR	sys_flock
		.space	(NR_syscalls-140)*4

		.bss
		EXPORT(IRQ_vectors)
		.fill	16,4,0

		.text
sys_call_names:
		TTABLE	("setup")
		TTABLE	("exit")
		TTABLE	("fork")
		TTABLE	("read")
		TTABLE	("write")
		TTABLE	("open")
		TTABLE	("close")
		TTABLE	("waitpid")
		TTABLE	("creat")
		TTABLE	("link")
		TTABLE	("unlink")
		TTABLE	("execve")
		TTABLE	("chdir")
		TTABLE	("time")
		TTABLE	("mknod")
		TTABLE	("chmod")
		TTABLE	("chown")
		TTABLE	("break")
		TTABLE	("stat")
		TTABLE	("lseek")
		TTABLE	("getpid")
		TTABLE	("mount")
		TTABLE	("umount")
		TTABLE	("setuid")
		TTABLE	("getuid")
		TTABLE	("stime")
		TTABLE	("ptrace")
		TTABLE	("alarm")
		TTABLE	("fstat")
		TTABLE	("pause")
		TTABLE	("utime")
		TTABLE	("stty")
		TTABLE	("gtty")
		TTABLE	("access")
		TTABLE	("nice")
		TTABLE	("ftime")
		TTABLE	("sync")
		TTABLE	("kill")
		TTABLE	("rename")
		TTABLE	("mkdir")
		TTABLE	("rmdir")
		TTABLE	("dup")
		TTABLE	("pipe")
		TTABLE	("times")
		TTABLE	("prof")
		TTABLE	("brk")
		TTABLE	("setgid")
		TTABLE	("getgid")
		TTABLE	("signal")
		TTABLE	("geteuid")
		TTABLE	("getegid")
		TTABLE	("acct")
		TTABLE	("phys")
		TTABLE	("lock")
		TTABLE	("ioctl")
		TTABLE	("fcntl")
		TTABLE	("mpx")
		TTABLE	("setpgid")
		TTABLE	("ulimit")
		TTABLE	("olduname")
		TTABLE	("umask")
		TTABLE	("chroot")
		TTABLE	("ustat")
		TTABLE	("dup2")
		TTABLE	("getppid")
		TTABLE	("getpgrp")
		TTABLE	("setsid")
		TTABLE	("sigaction")
		TTABLE	("sgetmask")
		TTABLE	("ssetmask")
		TTABLE	("setreuid")
		TTABLE	("setregid")
		TTABLE	("sigsuspend")
		TTABLE	("sigpending")
		TTABLE	("sethostname")
		TTABLE	("setrlimit")
		TTABLE	("getrlimit")
		TTABLE	("getrusage")
		TTABLE	("gettimeofday")
		TTABLE	("settimeofday")
		TTABLE	("getgroups")
		TTABLE	("setgroups")
		TTABLE	("select")
		TTABLE	("symlink")
		TTABLE	("lstat")
		TTABLE	("readlink")
		TTABLE	("uselib")
		TTABLE	("swapon")
		TTABLE	("reboot")
		TTABLE	("readdir")
		TTABLE	("mmap")
		TTABLE	("munmap")
		TTABLE	("truncate")
		TTABLE	("ftruncate")
		TTABLE	("fchmod")
		TTABLE	("fchown")
		TTABLE	("getpriority")
		TTABLE	("setpriority")
		TTABLE	("profil")
		TTABLE	("statfs")
		TTABLE	("fstatfs")
		TTABLE	("ioperm")
		TTABLE	("socketcall")
		TTABLE	("syslog")
		TTABLE	("setitimer")
		TTABLE	("getitimer")
		TTABLE	("newstat")
		TTABLE	("newlstat")
		TTABLE	("newfstat")
		TTABLE	("uname")
		TTABLE	("iopl")
		TTABLE	("vhangup")
		TTABLE	("idle")
		TTABLE	("vm86")
		TTABLE	("wait4")
		TTABLE	("swapoff")
		TTABLE	("sysinfo")
		TTABLE	("ipc")
		TTABLE	("fsync")
		TTABLE	("sigreturn")
		TTABLE	("clone")
		TTABLE	("setdomainname")
		TTABLE	("newuname")
		TTABLE	("modify_ldt (unused)")
		TTABLE	("adjtimex")
		TTABLE	("mprotect")
		TTABLE	("sigprocmask")
		TTABLE	("create_module")
		TTABLE	("init_module")
		TTABLE	("delete_module")
		TTABLE	("get_kernel_syms")
		TTABLE	("quotactl")
		TTABLE	("getpgid")
		TTABLE	("fchdir")
		TTABLE	("bdflush")
		TTABLE	("sysfs")
		TTABLE	("personality")
		TTABLE	("afs_syscall")		/* for afs_syscall */
		TTABLE	("setfsuid")
		TTABLE	("setfsgid")
		TTABLE	("llseek")
		TTABLE	("sys_getdents")
		TTABLE	("sys_select")
		TTABLE	("sys_flock")
