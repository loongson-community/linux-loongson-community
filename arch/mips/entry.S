/*
 *  linux/kernel/mips/sys_call.S
 *
 *  Copyright (C) 1994  Waldorf GMBH
 *  written by Ralf Baechle
 */

/*
 * sys_call.S  contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch.
 */

#define __ASSEMBLY__

#include <linux/sys.h>
#include <asm/segment.h>
#include <asm/mipsregs.h>
#include <asm/mipsconfig.h>
#include <asm/stackframe.h>
#include <asm/regdef.h>

/*
 * These are offsets into the task-struct.
 */
state		=  0
counter		=  4
priority	=  8
signal		= 12
blocked		= 16
flags		= 20
errno		= 24			#/* MIPS OK */
exec_domain	= 60			#/* ??? */

ENOSYS		= 38

		.globl	_system_call
		.globl	_lcall7
		.globl	_device_not_available
		.globl	_coprocessor_error
		.globl	_divide_error
		.globl	_debug
		.globl	_nmi
		.globl	_int3
		.globl	_overflow
		.globl	_bounds
		.globl	_invalid_op
		.globl	_double_fault
		.globl	_coprocessor_segment_overrun
		.globl	_invalid_TSS
		.globl	_segment_not_present
		.globl	_stack_segment
		.globl	_general_protection
		.globl	_reserved
		.globl	_alignment_check
		.globl	_page_fault
		.globl	ret_from_sys_call
		.globl	_sys_call_table

		.text
		.set	noreorder
		.align 4
handle_bottom_half:
		lw	s0,_intr_count
		addiu	s1,s0,1
		sw	s1,_intr_count
		mfc0	t0,CP0_STATUS			# Enable IRQs
		ori	t0,t0,7
		xori	t0,t0,6
		jal	_do_bottom_half
		mtc0	t0,CP0_STATUS
		j	9f
		sw	s1,_intr_count

		.set	reorder
		.align	4
reschedule:
		la	ra,ret_from_sys_call
		j	_schedule

		.set	noreorder
		.align	4
_system_call:
		li	t1,NR_syscalls
		bge	t0,t1,ret_from_sys_call
		.set	nomacro
		li	t2,-ENOSYS		# must be single instruction!
		.set	macro
		lui	t1,_sys_call_table
		sll	t0,t0,2
		addu	t1,t0,t1
		lw	t0,_sys_call_table(t1)
		lw	s0,_current

		beq	zero,t0,ret_from_sys_call
		lw	t0,flags(s0)
		sll	t0,t0,2			# PF_TRACESYS
		bltz	t0,1f
		sw	zero,errno(s0)		# delay slot

		jal	t0			# do the real work
		nop				# fillme: delay slot

		sw	v0,FR_REG2(sp)		# save the return value
		lw	v0,errno(s0)
		beq	zero,v0,ret_from_sys_call
		subu	v0,zero,v0		# v0 = -v0
						# fixme: indicate error
		j	ret_from_sys_call
		sw	v0,FR_REG2(sp)

		.align	4
1:		jal	_syscall_trace
		nop
#if 0
		movl	ORIG_EAX(%esp),%eax
		call	_sys_call_table(,%eax,4)
		movl	%eax,EAX(%esp)		# save the return value
		movl	_current,%eax
		movl	errno(%eax),%edx
		negl	%edx
		je	1f
		movl	%edx,EAX(%esp)
		orl	$(CF_MASK),EFLAGS(%esp)	# set carry to indicate error
#endif
1:		jal	_syscall_trace
		nop

		.align	4
ret_from_sys_call:
		lw	t0,_intr_count		# bottom half
		bne	zero,t0,2f

		lw	t0,_bh_mask
		lw	t1,_bh_active
		and	t0,t0,t1
		bne	zero,t0,handle_bottom_half
9:
		mfc0	t0,CP0_STATUS		# returning to supervisor ?
		andi	t0,t0,30
		subu	t0,t0,6
		bltz	t0,2f

1:
#if 0
/*
 * Try whether this is needed or not...
 */
		mfc0	t0,CP0_STATUS		# enable irqs
		ori	t0,t0,0x7
		xori	t0,t0,0x6
		mtc0	t0,CP0_STATUS
#endif

		lw	t0,_need_resched
		bne	zero,t0,reschedule

		lw	t0,_current
		la	t1,_task		# task[0] cannot have signals
		lw	t2,state(s0)		# state
		beq	t0,t1,2f
		lw	t0,counter(s0)		# counter
		beq	zero,t2,reschedule	# state == 0 ?
		lw	a0,blocked(s0)
						# save blocked in a0 for
						# signal handling
		beq	zero,t0,reschedule	# counter == 0 ?
		lw	t0,signal(s0)
		nor	t1,zero,t0
		and	t1,a0,t1
		beq	zero,t1,skip_signal_return
		nop
2:
		jal	_do_signal
		move	a1,sp
		
skip_signal_return:
		.set	noreorder
		.set	noat
return:		RESTORE_ALL
		.set	at

/*
 * Assumptions for _handle_int:
 *  - only bank a or b are possible interrupt sources
 */
		.globl	_handle_int
_handle_int:
		.set	noreorder
		.text
		la	s0,PORT_BASE
		li	t1,0x0f
		sb	t1,0x20(s0)			# poll command
		lb	t1,0x20(s0)			# read result
		FILL_LDS
		bgtz	t1,poll_second
		andi	t1,t1,7
		/*
		 * Acknowledge first pic
		 */
		lb	t2,0x21(s0)
		li	s1,1
		sllv	s1,s1,t1
		lb	t4,_cache_21
		or	t4,t4,s1
		sb	t4,_cache_21
		sb	t4,0x21(s0)
		li	t4,0x20
		sb	t4,0x20(s0)
		lw	t0,_intr_count
		addiu	t0,t0,1
		sw	t0,_intr_count
		/*
		 * Now call the real handler
		 */
		la	t0,_IRQ_vectors
		sll	t2,t1,2
		addu	t0,t0,t2
		lw	t0,(t0)
		FILL_LDS
		jalr	t0
		nop
		lw	t0,_intr_count
		subu	t0,t0,1
		sw	t0,_intr_count
		/*
		 * Unblock first pic
		 */
test1:		lbu	t1,0x21(s0)	# tlbl exception?!?
		lb	t1,_cache_21
		nor	s1,zero,s1
		and	t1,t1,s1
		sb	t1,_cache_21
		jr	v0
		sb	t1,0x21(s0)			# delay slot

		.set	at
poll_second:
		li	t1,0x0f
		sb	t1,0xa0(s0)			# poll command
		lb	t1,0xa0(s0)			# read result
		FILL_LDS
		bgtz	t1,spurious_interrupt
		andi	t1,t1,7
		/*
		 * Acknowledge second pic
		 */
		lbu	t2,0xa1(s0)
		lbu	t3,_cache_A1
		li	s1,1
		sllv	s1,s1,t1
		or	t3,t3,s1
		sb	t3,_cache_A1
		sb	t3,0xa1(s0)
		li	t3,0x20
		sb	t3,0xa0(s0)
		lw	t0,_intr_count
		sb	t3,0x20(s0)
		addiu	t0,t0,1
		sw	t0,_intr_count
		/*
		 * Now call the real handler
		 */
		la	t0,_IRQ_vectors
		sll	t2,t1,2
		addu	t0,t0,t2
		lw	t0,32(t0)
		FILL_LDS
		jalr	t0
		nop
		lw	t0,_intr_count
		subu	t0,t0,1
		sw	t0,_intr_count
		/*
		 * Unblock second pic
		 */
		lbu	t1,0xa1(s0)
		lb	t1,_cache_A1
		nor	s1,zero,s1
		and	t1,t1,s1
		sb	t1,_cache_A1
		jr	v0
		sb	t1,0xa1(s0)			# delay slot

		.set	at
spurious_interrupt:
		/*
		 * Nothing happend... (whistle)
		 */
		lw	t0,_spurious_count
		la	v0,return
		addiu	t0,t0,1
		sw	t0,_spurious_count
		jr	ra
		nop
		
		.globl	_IRQ
_IRQ:		move	s2,ra
		mfc0	t0,CP0_STATUS
		ori	t0,t0,0x1f
		xori	t0,t0,0x1e
		mtc0	t0,CP0_STATUS
		move	a1,sp
		jal	_do_IRQ
		move	a0,t1				# Delay slot
		mfc0	t0,CP0_STATUS
		ori	t0,t0,1
		xori	t0,t0,1
		la	v0,ret_from_sys_call
		jr	s2
		mtc0	t0,CP0_STATUS			# Delay slot

		.globl	_fast_IRQ
_fast_IRQ:	move	s2,ra
		move	a1,sp
		jal	_do_fast_IRQ
		move	a0,t1				# Delay slot
		la	v0,return
		jr	s2
		nop

		.globl	_bad_IRQ
_bad_IRQ:
		/*
		 * Don't return & unblock the pic
		 */
		j	return
		nop

		.bss
		.globl	_IRQ_vectors

_IRQ_vectors:
		.fill	16,4,0

/*
 * Dummy handlers
 */
		.text
		.set	noreorder
		.set	at

		.globl	_handle_mod
_handle_mod:
		la	a0,mod_text
		j	_panic
		nop

		.globl	_handle_tlbl
_handle_tlbl:
		la	a0,badvaddr
		mfc0	a1,CP0_BADVADDR
		jal	_printk
		nop
		la	a0,status
		lw	a1,FR_STATUS(sp)
		jal	_printk
		nop
		la	a0,eszero
		move	a1,s0
		jal	_printk
		nop
		la	a0,espe
		move	a1,sp
		jal	_printk
		nop
		la	a0,jifftext
		lw	a1,_jiffies
		jal	_printk
		nop
		la	a0,inttext
		lw	a1,_intr_count
		jal	_printk
		nop
		la	a0,tlbl_msg
		mfc0	a1,CP0_EPC
		jal	_printk
		nop
		la	a0,tlbl_text
		j	_panic
		nop

		.data
tlbl_msg:	.asciz	"tlbl exception at %x\n"
badvaddr:	.asciz	"accessing %x\n"
status:		.asciz	"cp0_status %x\n"
eszero:		.asciz	"s0 %x\n"
espe:		.asciz	"sp %x\n"
jifftext:	.asciz	"jiffies %d\n"
inttext:	.asciz	"IntNest: %d\n"

		.text
		.globl	_handle_tlbs
_handle_tlbs:
		la	a0,tlbs_text
		j	_panic
		nop

		.globl	_handle_adel
_handle_adel:
	la	v0,adel_text
	jal	_printk
	nop
	j	_handle_tlbl
		la	a0,adel_text
		j	_panic
		nop

		.globl	_handle_ades
_handle_ades:
		la	a0,ades_text
		j	_panic
		nop

		.globl	_handle_ibe
_handle_ibe:
		la	a0,ibe_text
		j	_panic
		nop

		.globl	_handle_dbe
_handle_dbe:
		la	a0,dbe_text
		j	_panic
		nop

		.globl	_handle_sys
_handle_sys:
		la	a0,sys_text
		j	_panic
		nop

		.globl	_handle_bp
_handle_bp:
		la	a0,bp_text
		j	_panic
		nop

		.globl	_handle_ri
_handle_ri:
		la	a0,ri_text
		j	_panic
		nop

		.globl	_handle_cpu
_handle_cpu:
		la	a0,cpu_text
		j	_panic
		nop

		.globl	_handle_ov
_handle_ov:
		la	a0,ov_text
		j	_panic
		nop

		.globl	_handle_tr
_handle_tr:
		la	a0,tr_text
		j	_panic
		nop

		.globl	_handle_reserved
_handle_reserved:
		la	a0,reserved_text
		j	_panic
		nop

		.globl	_handle_fpe
_handle_fpe:
		la	a0,fpe_text
		j	_panic
		nop

		.data
spurious_text:	.asciz	"Spurious interrupt"
fpe_text:	.asciz	"fpe exception"
reserved_text:	.asciz	"reserved exception"
tr_text:	.asciz	"tr exception"
ov_text:	.asciz	"ov exception"
cpu_text:	.asciz	"cpu exception"
ri_text:	.asciz	"ri exception"
bp_text:	.asciz	"bp exception"
sys_text:	.asciz	"sys exception"
dbe_text:	.asciz	"dbe exception"
ibe_text:	.asciz	"ibe exception"
ades_text:	.asciz	"ades exception"
adel_text:	.asciz	"adel exception"
tlbs_text:	.asciz	"tlbs exception"
mod_text:	.asciz	"mod exception"
tlbl_text:	.asciz	"tlbl exception"

/*
 * Exception handler table, 256 entries.
 */
		.data
		.globl	_exception_handlers
_exception_handlers:
		.word	_handle_int		/*  0 */
		.word	_handle_mod
		.word	_handle_tlbl
		.word	_handle_tlbs
		.word	_handle_adel
		.word	_handle_ades
		.word	_handle_ibe
		.word	_handle_dbe
		.word	_handle_sys
		.word	_handle_bp
		.word	_handle_ri
		.word	_handle_cpu
		.word	_handle_ov
		.word	_handle_tr
		.word	_handle_reserved
		.word	_handle_fpe             /* 15 */
#if 0
		.fill	240,4,_handle_reserved
#endif

/*
 * Table of syscalls
 */
		.data
_sys_call_table:
		.word	_sys_setup		/* 0 */
		.word	_sys_exit
		.word	_sys_fork
		.word	_sys_read
		.word	_sys_write
		.word	_sys_open		/* 5 */
		.word	_sys_close
		.word	_sys_waitpid
		.word	_sys_creat
		.word	_sys_link
		.word	_sys_unlink		/* 10 */
		.word	_sys_execve
		.word	_sys_chdir
		.word	_sys_time
		.word	_sys_mknod
		.word	_sys_chmod		/* 15 */
		.word	_sys_chown
		.word	_sys_break
		.word	_sys_stat
		.word	_sys_lseek
		.word	_sys_getpid		/* 20 */
		.word	_sys_mount
		.word	_sys_umount
		.word	_sys_setuid
		.word	_sys_getuid
		.word	_sys_stime		/* 25 */
		.word	_sys_ptrace
		.word	_sys_alarm
		.word	_sys_fstat
		.word	_sys_pause
		.word	_sys_utime		/* 30 */
		.word	_sys_stty
		.word	_sys_gtty
		.word	_sys_access
		.word	_sys_nice
		.word	_sys_ftime		/* 35 */
		.word	_sys_sync
		.word	_sys_kill
		.word	_sys_rename
		.word	_sys_mkdir
		.word	_sys_rmdir		/* 40 */
		.word	_sys_dup
		.word	_sys_pipe
		.word	_sys_times
		.word	_sys_prof
		.word	_sys_brk		/* 45 */
		.word	_sys_setgid
		.word	_sys_getgid
		.word	_sys_signal
		.word	_sys_geteuid
		.word	_sys_getegid		/* 50 */
		.word	_sys_acct
		.word	_sys_phys
		.word	_sys_lock
		.word	_sys_ioctl
		.word	_sys_fcntl		/* 55 */
		.word	_sys_mpx
		.word	_sys_setpgid
		.word	_sys_ulimit
		.word	_sys_olduname
		.word	_sys_umask		/* 60 */
		.word	_sys_chroot
		.word	_sys_ustat
		.word	_sys_dup2
		.word	_sys_getppid
		.word	_sys_getpgrp		/* 65 */
		.word	_sys_setsid
		.word	_sys_sigaction
		.word	_sys_sgetmask
		.word	_sys_ssetmask
		.word	_sys_setreuid		/* 70 */
		.word	_sys_setregid
		.word	_sys_sigsuspend
		.word	_sys_sigpending
		.word	_sys_sethostname
		.word	_sys_setrlimit		/* 75 */
		.word	_sys_getrlimit
		.word	_sys_getrusage
		.word	_sys_gettimeofday
		.word	_sys_settimeofday
		.word	_sys_getgroups		/* 80 */
		.word	_sys_setgroups
		.word	_sys_select
		.word	_sys_symlink
		.word	_sys_lstat
		.word	_sys_readlink		/* 85 */
		.word	_sys_uselib
		.word	_sys_swapon
		.word	_sys_reboot
		.word	_sys_readdir
		.word	_sys_mmap		/* 90 */
		.word	_sys_munmap
		.word	_sys_truncate
		.word	_sys_ftruncate
		.word	_sys_fchmod
		.word	_sys_fchown		/* 95 */
		.word	_sys_getpriority
		.word	_sys_setpriority
		.word	_sys_profil
		.word	_sys_statfs
		.word	_sys_fstatfs		/* 100 */
		.word	_sys_ioperm
		.word	_sys_socketcall
		.word	_sys_syslog
		.word	_sys_setitimer
		.word	_sys_getitimer		/* 105 */
		.word	_sys_newstat
		.word	_sys_newlstat
		.word	_sys_newfstat
		.word	_sys_uname
		.word	_sys_iopl		/* 110 */
		.word	_sys_vhangup
		.word	_sys_idle
		.word	_sys_vm86
		.word	_sys_wait4
		.word	_sys_swapoff		/* 115 */
		.word	_sys_sysinfo
		.word	_sys_ipc
		.word	_sys_fsync
		.word	_sys_sigreturn
		.word	_sys_clone		/* 120 */
		.word	_sys_setdomainname
		.word	_sys_newuname
		.word	_sys_modify_ldt
		.word	_sys_adjtimex
		.word	_sys_mprotect		/* 125 */
		.word	_sys_sigprocmask
		.word	_sys_create_module
		.word	_sys_init_module
		.word	_sys_delete_module
		.word	_sys_get_kernel_syms	/* 130 */
		.word	_sys_quotactl
		.word	_sys_getpgid
		.word	_sys_fchdir
		.word	_sys_bdflush
		.word	_sys_sysfs		/* 135 */
		.word	_sys_personality
		.word	0			/* for afs_syscall */
		.word	_sys_setfsuid
		.word	_sys_setfsgid
		.word	_sys_llseek		/* 140 */
		.space	(NR_syscalls-140)*4
