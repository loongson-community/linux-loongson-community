#ifndef _ASM_MIPS_SCHED_H
#define _ASM_MIPS_SCHED_H

#include <asm/system.h>

/*
 * System setup and hardware bug flags..
 */
extern int hard_math;
extern int wp_works_ok;		/* doesn't work on a 386 */

extern unsigned long intr_count;
extern unsigned long event;

#define start_bh_atomic() \
{int flags; save_flags(flags); cli(); intr_count++; restore_flags(flags)}

#define end_bh_atomic() \
{int flags; save_flags(flags); cli(); intr_count--; restore_flags(flags)}

/*
 * Bus types (default is ISA, but people can check others with these..)
 * MCA_bus hardcoded to 0 for now.
 */
extern int EISA_bus;
#define MCA_bus 0

/*
 * User space process size: 2GB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.
 */
#define TASK_SIZE	0x80000000

#define NUM_FPA_REGS	32

struct mips_fpa_hard_struct {
	float fp_regs[NUM_FPA_REGS];
	unsigned int control;
};

struct mips_fpa_soft_struct {
	/*
	 * FIXME: no fpa emulator yet, but who cares?
	 */
	long	dummy;
	};

union mips_fpa_union {
        struct mips_fpa_hard_struct hard;
        struct mips_fpa_soft_struct soft;
};

#define INIT_FPA { \
	0, \
}

struct tss_struct {
        /*
         * saved main processor registers
         */
        unsigned long           reg1,  reg2,  reg3,  reg4,  reg5,  reg6,  reg7;
        unsigned long    reg8,  reg9, reg10, reg11, reg12, reg13, reg14, reg15;
        unsigned long   reg16, reg17, reg18, reg19, reg20, reg21, reg22, reg23;
        unsigned long   reg24, reg25, reg26,               reg29, reg30, reg31;
        /*
         * saved cp0 registers
         */
        unsigned int cp0_status;
        unsigned long cp0_epc;
        unsigned long cp0_errorepc;
	unsigned long cp0_context;
	/*
	 * saved fpa/fpa emulator stuff
	 */
	union mips_fpa_union fpa;
	/*
	 * Other stuff associated with the process
	 */
	unsigned long cp0_badvaddr;
	unsigned long errorcode;
	unsigned long trap_no;
	unsigned long fs;		/* "Segment" pointer     */
	unsigned long ksp;		/* Kernel stack pointer  */
	unsigned long pg_dir;		/* L1 page table pointer */
};

#define INIT_TSS  { \
        /* \
         * saved main processor registers \
         */ \
	   0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0,       0, 0, 0, \
	/* \
         * saved cp0 registers \
         */ \
	0, 0, 0, 0, \
	/* \
	 * saved fpa/fpa emulator stuff \
	 */ \
	INIT_FPA, \
	/* \
	 * Other stuff associated with the process\
	 */ \
	0, 0, 0, KERNEL_DS, 0, 0 \
}

struct task_struct {
	/*
	 * these are hardcoded - don't touch
	 */
	volatile long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	long counter;
	long priority;
	unsigned long signal;
	unsigned long blocked;	/* bitmap of masked signals */
	unsigned long flags;	/* per process flags, defined below */
	int errno;
	int debugreg[8];  /* Hardware debugging registers */
	struct exec_domain *exec_domain;
	/*
	 * various fields
	 */
	struct linux_binfmt *binfmt;
	struct task_struct *next_task, *prev_task;
	struct sigaction sigaction[32];
	unsigned long saved_kernel_stack;
	unsigned long kernel_stack_page;
	int exit_code, exit_signal;
	unsigned long personality;
	int dumpable:1;
	int did_exec:1;
	int pid,pgrp,session,leader;
	int groups[NGROUPS];
	/* 
	 * pointers to (original) parent process, youngest child, younger
	 * sibling, older sibling, respectively.  (p->father can be replaced
	 * with p->p_pptr->pid)
	 */
	struct task_struct *p_opptr, *p_pptr, *p_cptr, *p_ysptr, *p_osptr;
	struct wait_queue *wait_chldexit;	/* for wait4() */
	unsigned short uid,euid,suid,fsuid;
	unsigned short gid,egid,sgid,fsgid;
	unsigned long timeout;
	unsigned long it_real_value, it_prof_value, it_virt_value;
	unsigned long it_real_incr, it_prof_incr, it_virt_incr;
	long utime, stime, cutime, cstime, start_time;
	struct rlimit rlim[RLIM_NLIMITS]; 
	unsigned short used_math;
	char comm[16];
	/*
	 * virtual 86 mode stuff
	 */
	struct vm86_struct * vm86_info;
	unsigned long screen_bitmap;
	unsigned long v86flags, v86mask, v86mode;
	/*
	 * file system info
	 */
	int link_count;
	struct tty_struct *tty; /* NULL if no tty */
	/*
	 * ipc stuff
	 */
	struct sem_undo *semundo;
	/*
	 * ldt for this task - used by Wine.  If NULL, default_ldt is used
	 */
	struct desc_struct *ldt;
	/*
	 * tss for this task
	 */
	struct tss_struct tss;
	/*
	 * filesystem information
	 */
	struct fs_struct fs[1];
	/*
	 * open file information
	 */
	struct files_struct files[1];
	/*
	 * memory management info
	 */
	struct mm_struct mm[1];
};

/*
 *  INIT_TASK is used to set up the first task table, touch at
 * your own risk!. Base=0, limit=0x1fffff (=2MB)
 */
#define INIT_TASK \
/* state etc */	{ 0,15,15,0,0,0,0, \
/* debugregs */ { 0, },            \
/* exec domain */&default_exec_domain, \
/* binfmt */	NULL, \
/* schedlink */	&init_task,&init_task, \
/* signals */	{{ 0, },}, \
/* stack */	0,(unsigned long) &init_kernel_stack, \
/* ec,brk... */	0,0,0,0,0, \
/* pid etc.. */	0,0,0,0, \
/* suppl grps*/ {NOGROUP,}, \
/* proc links*/ &init_task,&init_task,NULL,NULL,NULL,NULL, \
/* uid etc */	0,0,0,0,0,0,0,0, \
/* timeout */	0,0,0,0,0,0,0,0,0,0,0,0, \
/* rlimits */   { {LONG_MAX, LONG_MAX}, {LONG_MAX, LONG_MAX},  \
		  {LONG_MAX, LONG_MAX}, {LONG_MAX, LONG_MAX},  \
		  {       0, LONG_MAX}, {LONG_MAX, LONG_MAX}}, \
/* math */	0, \
/* comm */	"swapper", \
/* vm86_info */	NULL, 0, 0, 0, 0, \
/* fs info */	0,NULL, \
/* ipc */	NULL, \
/* ldt */	NULL, \
/* tss */	INIT_TSS, \
/* fs */	{ INIT_FS }, \
/* files */	{ INIT_FILES }, \
/* mm */	{ INIT_MM } \
}

#ifdef __KERNEL__

/*
 * switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 */
#define switch_to(tsk) \
	__asm__(""::);	/* fix me */

/*
 * Does the process account for user or for system time?
 */
#define USES_USER_TIME(regs) (!((regs)->cp0_status & 0x18))

#endif /* __KERNEL__ */

#endif /* _ASM_MIPS_SCHED_H */
