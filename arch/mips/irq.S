/*
 *  linux/kernel/mips/sys_call.S
 *
 *  Copyright (C) 1994  Waldorf GMBH
 *  written by Ralf Baechle
 */

/*
 * All code below must be relocatable!
 */

/*
 * sys_call.S  contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call.
 *
 * I changed all the .align's to 4 (16 byte alignment), as that's faster
 * on a 486.
 *
 * Stack layout in 'ret_from_system_call':
 * 	ptrace needs to have all regs on the stack.
 *	if the order here is changed, it needs to be 
 *	updated in fork.c:copy_process, signal.c:do_signal,
 *	ptrace.c and ptrace.h
 *
 *	 0(%esp) - %ebx
 *	 4(%esp) - %ecx
 *	 8(%esp) - %edx
 *       C(%esp) - %esi
 *	10(%esp) - %edi
 *	14(%esp) - %ebp
 *	18(%esp) - %eax
 *	1C(%esp) - %ds
 *	20(%esp) - %es
 *      24(%esp) - %fs
 *	28(%esp) - %gs
 *	2C(%esp) - orig_eax
 *	30(%esp) - %eip
 *	34(%esp) - %cs
 *	38(%esp) - %eflags
 *	3C(%esp) - %oldesp
 *	40(%esp) - %oldss
 */

#include <linux/segment.h>
#include <linux/sys.h>

/*
 * Offsets into the Interrupt stackframe.
 */
FR_REG1		= 0
FR_REG2		= 4
FR_REG3		= 8
FR_REG4		= 12
FR_REG5		= 16
FR_REG6		= 20
FR_REG7		= 24
FR_REG8		= 28
FR_REG9		= 32
FR_REG10	= 36
FR_REG11	= 40
FR_REG12	= 44
FR_REG13	= 48
FR_REG14	= 52
FR_REG15	= 56
FR_REG16	= 60
FR_REG17	= 64
FR_REG18	= 68
FR_REG19	= 72
FR_REG20	= 76
FR_REG21	= 80
FR_REG22	= 84
FR_REG23	= 88
FR_REG24	= 92
FR_REG25	= 96
/* $26 and $27 not saved */
FR_REG28	= 100
FR_REG29	= 104
FR_REG30	= 108
FR_REG31	= 112
/*
 * Saved cp0 registers follow
 */
FR_STATUS	= 116
FR_EPC		= 120
FR_ERROREPC	= 124
FR_SIZE		= 120			/* Size of stack frame */

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
dbgreg6		= 52
dbgreg7		= 56
exec_domain	= 60

ENOSYS		= 38

		.globl _system_call,_lcall7
		.globl _device_not_available, _coprocessor_error
		.globl _divide_error,_debug,_nmi,_int3,_overflow,_bounds,
		.globl _invalid_op,_double_fault,_coprocessor_segment_overrun
		.globl _invalid_TSS,_segment_not_present,_stack_segment
		.globl _general_protection,_reserved
		.globl _alignment_check,_page_fault
		.globl ret_from_sys_call, _sys_call_table

#define SAVE_ALL(which_pc) \
		.set	noreorder \
		.set	noat \
		lui	k0,0x8000 \
		move	k1,$sp \
		lw	sp,_kernelsp-except_vec0(k0) \
		subu	sp,$sp,FR_SIZE \
		sw	sp,_kernelsp-except_vec0(k0) \	/* Kernel SP */
		mfc0	v0,CP0_STATUS \
		sw	v0,FR_STATUS(sp) \
		mfc0	v0,CP0_EPC \
		sw	v0,FR_EPC \
		mfc0	v0,CP0_ERROREPC \
		sw	v0,FR_ERROREPC \
		sw	k1,FR_R27(sp) \
		sw	$2,FR_R1(sp) \
		sw	$2,FR_R2(sp) \
		sw	$3,FR_R3(sp) \
		sw	$4,FR_R4(sp) \
		sw	$5,FR_R5(sp) \
		sw	$6,FR_R6(sp) \
		sw	$7,FR_R7(sp) \
		sw	$8,FR_R8(sp) \
		sw	$9,FR_R9(sp) \
		sw	$10,FR_R10(sp) \
		sw	$11,FR_R11(sp) \
		sw	$12,FR_R12(sp) \
		sw	$13,FR_R13(sp) \
		sw	$14,FR_R14(sp) \
		sw	$15,FR_R15(sp) \
		sw	$16,FR_R16(sp) \
		sw	$17,FR_R17(sp) \
		sw	$18,FR_R18(sp) \
		sw	$19,FR_R19(sp) \
		sw	$20,FR_R20(sp) \
		sw	$21,FR_R21(sp) \
		sw	$22,FR_R22(sp) \
		sw	$23,FR_R23(sp) \
		sw	$24,FR_R24(sp) \
		sw	$25,FR_R25(sp) \
		sw	$28,FR_R28(sp) \
		sw	$30,FR_R30(sp) \
		sw	$31,FR_R31(sp)


#define RESTORE_ALL \
		lui	k1,0x8000 \
		move	k0,$sp \
		lw	v0,FR_ERROREPC(k0) \
		lw	v1,FR_EPC(k0) \
		mtc0	v0,CP0_ERROREPC(k0) \
		mtc0	v1,CP0_EPC(k0) \
		lw	$31,FR_R31(k0) \
		lw	$30,FR_R30(k0) \
		lw	$28,FR_R28(k0) \
		lw	$25,FR_R25(k0) \
		lw	$24,FR_R24(k0) \
		lw	$23,FR_R23(k0) \
		lw	$22,FR_R22(k0) \
		lw	$21,FR_R21(k0) \
		lw	$20,FR_R20(k0) \
		lw	$19,FR_R19(k0) \
		lw	$18,FR_R18(k0) \
		lw	$17,FR_R17(k0) \
		lw	$16,FR_R16(k0) \
		lw	$15,FR_R15(k0) \
		lw	$14,FR_R14(k0) \
		lw	$13,FR_R13(k0) \
		lw	$12,FR_R12(k0) \
		lw	$11,FR_R11(k0) \
		lw	$10,FR_R10(k0) \
		lw	$9,FR_R9(k0) \
		lw	$8,FR_R8(k0) \
		lw	$7,FR_R7(k0) \
		lw	$6,FR_R6(k0) \
		lw	$5,FR_R5(k0) \
		lw	$4,FR_R4(k0) \
		lw	$3,FR_R3(k0) \
		lw	$2,FR_R2(k0) \
		lw	$1,FR_R1(k0) \
		addiu	k0,k0,FR_SIZE \
		sw	k0,_kernelsp-except_vec0(k1) \	/* Kernel SP */
		eret

		.align 4
handle_bottom_half:
		pushfl
		incl	_intr_count
		mtc0	zero,CP0_STATUS
		call	_do_bottom_half
		popfl
		decl	_intr_count
		j	9f
		nop

		.align 4
reschedule:
		pushl	$ret_from_sys_call
		j	_schedule
		nop

		.align 4
_system_call:
		pushl	%eax			# save orig_eax
		SAVE_ALL
		movl	$-ENOSYS,EAX(%esp)
		cmpl	$(NR_syscalls),%eax
		jae	ret_from_sys_call
		movl	_sys_call_table(,%eax,4),%eax
		testl	%eax,%eax
		je	ret_from_sys_call
		movl	_current,%ebx
		andl	$~CF_MASK,EFLAGS(%esp)	# clear carry - assume no errors
		movl	$0,errno(%ebx)
		movl	%db6,%edx
		movl	%edx,dbgreg6(%ebx)	# save current hardware debugging status
		testb	$0x20,flags(%ebx)	# PF_TRACESYS
		jne	1f
		call	*%eax
		movl	%eax,EAX(%esp)		# save the return value
		movl	errno(%ebx),%edx
		negl	%edx
		je	ret_from_sys_call
		movl	%edx,EAX(%esp)
		orl	$(CF_MASK),EFLAGS(%esp)	# set carry to indicate error
		j	ret_from_sys_call
		nop

		.align 4
1:		call	_syscall_trace
		movl	ORIG_EAX(%esp),%eax
		call	_sys_call_table(,%eax,4)
		movl	%eax,EAX(%esp)		# save the return value
		movl	_current,%eax
		movl	errno(%eax),%edx
		negl	%edx
		je	1f
		movl	%edx,EAX(%esp)
		orl	$(CF_MASK),EFLAGS(%esp)	# set carry to indicate error
1:		call	_syscall_trace

		.align	4,0x90
ret_from_sys_call:
		cmpl	$0,_intr_count
		jne	2f
		movl	_bh_mask,%eax
		andl	_bh_active,%eax
		jne	handle_bottom_half
9:		movl	EFLAGS(%esp),%eax	# check VM86 flag: CS/SS are
		testl	$(VM_MASK),%eax		# different then
		jne 1f
		cmpw $(KERNEL_CS),CS(%esp)	# was old code segment supervisor ?
		je	2f
1:		sti
		orl	$(IF_MASK),%eax		# these just try to make sure
		andl	$~NT_MASK,%eax		# the program doesn't do anything
		movl	%eax,EFLAGS(%esp)	# stupid
		cmpl	$0,_need_resched
		jne	reschedule
		movl	_current,%eax
		cmpl	_task,%eax		# task[0] cannot have signals
		je	2f
		cmpl	$0,state(%eax)		# state
		jne	reschedule
		cmpl	$0,counter(%eax)	# counter
		je	reschedule
		movl	blocked(%eax),%ecx
		movl	%ecx,%ebx		# save blocked in %ebx for
						# signal handling
		notl	%ecx
		andl	signal(%eax),%ecx
		jne	signal_return
2:		RESTORE_ALL

		.align 4
signal_return:
		movl	%esp,%ecx
		pushl	%ecx
		testl	$(VM_MASK),EFLAGS(%ecx)
		jne	v86_signal_return
		pushl	%ebx
		call	_do_signal
		popl	%ebx
		popl	%ebx
		RESTORE_ALL

		.align 4
v86_signal_return:
		call	_save_v86_state
		movl	%eax,%esp
		pushl	%eax
		pushl	%ebx
		call	_do_signal
		popl	%ebx
		popl	%ebx
		RESTORE_ALL

		.align	4
_divide_error:
		move	$a1,zero		# no error code
		la	$t0,$_do_divide_error
		.align	4,0x90
error_code:
		push	%fs
		push	%es
		push	%ds
		pushl	%eax
		pushl	%ebp
		pushl	%edi
		pushl	%esi
		pushl	%edx
		pushl	%ecx
		pushl	%ebx
		cld
		movl	$-1, %eax
		xchgl	%eax, ORIG_EAX(%esp)	# orig_eax (get the error code. )
		xorl	%ebx,%ebx		# zero ebx
		mov	%gs,%bx			# get the lower order bits of gs
		xchgl	%ebx, GS(%esp)		# get the address and save gs.
		pushl	%eax			# push the error code
		lea	4(%esp),%edx
		pushl	%edx
		movl	$(KERNEL_DS),%edx
		mov	%dx,%ds
		mov	%dx,%es
		movl	$(USER_DS),%edx
		mov	%dx,%fs
		jal	t0			# call handler
		addl	$8,%esp
		j	ret_from_sys_call

		.align 4
_coprocessor_error:
		move	a1,zero
		la	t0,_do_coprocessor_error
		j	error_code

		.align	4
_device_not_available:
		pushl	$-1			# mark this as an int
		SAVE_ALL
		pushl	$ret_from_sys_call
		movl	%cr0,%eax
		testl	$0x4,%eax		# EM (math emulation bit)
		je	_math_state_restore
		pushl	$0			# temporary storage for ORIG_EIP
		call	_math_emulate
		addl	$4,%esp
		ret

		.set	reorder

		.align 4
_debug:
		move	a1,zero
		la	t0,_do_debug
		j	error_code

		.align 4
_nmi:
		move	a1,zero
		la	t0,_do_nmi
		j	error_code

		.align 4
_int3:
		move	a1,zero
		la	t0,_do_int3
		j	error_code

		.align 4
_overflow:
		move	a1,zero
		la	t0,_do_overflow
		j	error_code

		.align 4
_bounds:
		move	a1,zero
		la	t0,_do_bounds
		j	error_code

		.align 4
_invalid_op:
		move	a1,zero
		la	t0,_do_invalid_op
		j	error_code

		.align	4
_segment_not_present:
		la	t0,_do_segment_not_present
		j	error_code

		.align	4
_stack_segment:
		la	t0,_do_stack_segment
		j	error_code

		.align	4
_general_protection:
		la	t0,_do_general_protection
		j	error_code

		.align	4
_page_fault:
		la	t0,_do_page_fault
		j	error_code
/*
 * TLB Refill exception entry point
 *
 * The mm data is stored in the context register and 
 */
		.text
                .set    noreorder
                .set    noat
		dmfc0	k0,CP0_CONTEXT
		dsrl	k0,k0,2
		lw	k0,(k0)			# Level 1 descriptor
		dmfc0	k1,CP0_BADVADDR
		srl	k1,k1,10
		andi	k1,k1,0xffc
		addu	k1,k1,k1
		lwu	k0,(k1)			# 2 Level 2 entries
		lwu	k1,4(k1)
		dmtc0	k0,CP0_ENTRYLO0
		dmtc0	k0,CP0_ENTRYLO1
		tlbwr
	/*
	 * Now compute the return address. Since this is extremly
	 * timecritical the code is inlined
	 */
		mfc0	k0,CP0_CAUSE
		bgtz	k0,1f
		
	/*
	 * Damn - a branch delay slot. Compute new PC
	 */

	/*
	 * That's it boys - back to work!
	 */
1:		eret




		lui	t0,>_exception_handlers
		mfc0	t1,CP0_CAUSE
		andi	t1,t1,0x3fc
		addu	t0,t0,t1
		lw	t0,<_exception_handlers(t0)
		sw	/* fill delay slot */
		jalr	t0
		sw	/* fill delay slot */
		

/*
 * Exception handler table, 256 entries.
 */
		.data
		.align	4
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
		.word	_handle_fpe
		.fill	240,4,_handle_reserved	/* 16 */

/*
 * Table of syscalls
 */
		.data
		.align	4
_sys_call_table:
		.word	_sys_setup		/* 0 */
		.word	_sys_exit
		.word	_sys_fork
		.word	_sys_read
		.word	_sys_write
		.word	_sys_open		/* 5 */
		.word	_sys_close
		.word	_sys_wordpid
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
		.word	_sys_brk			/* 45 */
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
		.word	_sys_word4
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
		.word	0				/* for afs_syscall */

		.space (NR_syscalls-137)*4
