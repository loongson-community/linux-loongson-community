/*
 * include/asm-mips/processor.h
 *
 * Copyright (C) 1994, 1995 by Waldorf Electronics
 * Copyright (C) 1995, 1996 by Ralf Baechle
 * Modified further for R[236]000 compatibility by Paul M. Antoine
 */
#ifndef __ASM_MIPS_PROCESSOR_H
#define __ASM_MIPS_PROCESSOR_H

#include <asm/sgidefs.h>

#if !defined (__LANGUAGE_ASSEMBLY__)
#include <asm/mipsregs.h>
#include <asm/reg.h>
#include <asm/system.h>

/*
 * System setup and hardware bug flags..
 */
extern char wait_available;		/* only available on R4[26]00 */

extern unsigned long intr_count;
extern unsigned long event;

/*
 * Bus types (default is ISA, but people can check others with these..)
 * There are no Microchannel MIPS machines.
 *
 * This needs to be extended since MIPS systems are being delivered with
 * numerous different types of bus systems.
 */
extern int EISA_bus;
#define MCA_bus 0

/*
 * User space process size: 2GB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.  TASK_SIZE
 * for a 64 bit kernel is expandable to 8192PB, of which the current MIPS
 * implementations will "only" be able to use 1TB ...
 */
#define TASK_SIZE	(0x80000000UL)

/*
 * Size of io_bitmap in longwords: 32 is ports 0-0x3ff.
 */
#define IO_BITMAP_SIZE	32

#define NUM_FPU_REGS	32

struct mips_fpu_hard_struct {
	double fp_regs[NUM_FPU_REGS];
	unsigned int control;
};

/*
 * FIXME: no fpu emulator yet (but who cares anyway?)
 */
struct mips_fpu_soft_struct {
	long	dummy;
	};

union mips_fpu_union {
        struct mips_fpu_hard_struct hard;
        struct mips_fpu_soft_struct soft;
};

#define INIT_FPU { \
	{{0,},} \
}

/*
 * If you change thread_struct remember to change the #defines below too!
 */
struct thread_struct {
        /*
         * saved main processor registers
         */
        __register_t   reg16, reg17, reg18, reg19, reg20, reg21, reg22, reg23;
        __register_t                               reg28, reg29, reg30, reg31;
	/*
	 * saved cp0 stuff
	 */
	unsigned int cp0_status;
	/*
	 * saved fpu/fpu emulator stuff
	 */
	union mips_fpu_union fpu;
	/*
	 * Other stuff associated with the thread
	 */
	long cp0_badvaddr;
	unsigned long error_code;
	unsigned long trap_no;
	long ksp;			/* Top of kernel stack   */
	long pg_dir;			/* L1 page table pointer */
#define MF_FIXADE 1			/* Fix address errors in software */
#define MF_LOGADE 2			/* Log address errors to syslog */
	unsigned long mflags;
	unsigned long segment;
};

#endif /* !defined (__LANGUAGE_ASSEMBLY__) */

/*
 * If you change the #defines remember to change thread_struct above too!
 */
#define TOFF_REG16		0
#define TOFF_REG17		(TOFF_REG16+SZREG)
#define TOFF_REG18		(TOFF_REG17+SZREG)
#define TOFF_REG19		(TOFF_REG18+SZREG)
#define TOFF_REG20		(TOFF_REG19+SZREG)
#define TOFF_REG21		(TOFF_REG20+SZREG)
#define TOFF_REG22		(TOFF_REG21+SZREG)
#define TOFF_REG23		(TOFF_REG22+SZREG)
#define TOFF_REG28		(TOFF_REG23+SZREG)
#define TOFF_REG29		(TOFF_REG28+SZREG)
#define TOFF_REG30		(TOFF_REG29+SZREG)
#define TOFF_REG31		(TOFF_REG30+SZREG)
#define TOFF_CP0_STATUS		(TOFF_REG31+SZREG)
/*
 * Pad for 8 byte boundary!
 */
#define TOFF_FPU		(((TOFF_CP0_STATUS+SZREG)+(8-1))&~(8-1))
#define TOFF_CP0_BADVADDR	(TOFF_FPU+264)
#define TOFF_ERROR_CODE		(TOFF_CP0_BADVADDR+4)
#define TOFF_TRAP_NO		(TOFF_ERROR_CODE+4)
#define TOFF_KSP		(TOFF_TRAP_NO+4)
#define TOFF_PG_DIR		(TOFF_KSP+4)
#define TOFF_MFLAGS		(TOFF_PG_DIR+4)
#define TOFF_EX			(TOFF_PG_MFLAGS+4)

#define INIT_MMAP { &init_mm, KSEG0, KSEG1, PAGE_SHARED, \
                    VM_READ | VM_WRITE | VM_EXEC }

#define INIT_TSS  { \
        /* \
         * saved main processor registers \
         */ \
	0, 0, 0, 0, 0, 0, 0, 0, \
	            0, 0, 0, 0, \
	/* \
	 * saved cp0 stuff \
	 */ \
	0, \
	/* \
	 * saved fpu/fpu emulator stuff \
	 */ \
	INIT_FPU, \
	/* \
	 * Other stuff associated with the process \
	 */ \
	0, 0, 0, sizeof(init_kernel_stack) + (unsigned long)init_kernel_stack - 8, \
	(unsigned long) swapper_pg_dir - PT_OFFSET, \
	/* \
	 * For now the default is to fix address errors \
	 */ \
	MF_FIXADE, \
	KERNEL_DS \
}

/*
 * Paul, please check if 4kb stack are sufficient for the R3000.  With 4kb
 * I never had a stack overflows on 32 bit R4000 kernels but the almost
 * completly 64bit R4000 kernels from 1.3.63 on need more stack even in
 * trivial situations.
 */
#define KERNEL_STACK_SIZE 8192

#if !defined (__LANGUAGE_ASSEMBLY__)
extern unsigned long alloc_kernel_stack(void);
extern void free_kernel_stack(unsigned long stack);

/*
 * Return saved PC of a blocked thread.
 */
extern unsigned long (*thread_saved_pc)(struct thread_struct *t);

/*
 * Do necessary setup to start up a newly executed thread.
 */
extern void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long sp);

/*
 * Does the process account for user or for system time?
 */
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
#define USES_USER_TIME(regs) (!((regs)->cp0_status & 0x4))
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
#define USES_USER_TIME(regs) (!((regs)->cp0_status & 0x18))
#endif

/*
 * Return_address is a replacement for __builtin_return_address(count)
 * which on certain architectures cannot reasonably be implemented in GCC
 * (MIPS, Alpha) or is unuseable with -fomit-frame-pointer (i386).
 * Note that __builtin_return_address(x>=1) is forbidden because GCC
 * aborts compilation on some CPUs.  It's simply not possible to unwind
 * some CPU's stackframes.
 */
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
/*
 * __builtin_return_address works only for non-leaf functions.  We avoid the
 * overhead of a function call by forcing the compiler to save the return
 * address register on the stack.
 */
#define return_address() ({__asm__ __volatile__("":::"$31");__builtin_return_address(0);})
#else
/*
 * __builtin_return_address is not implemented at all.  Calling it
 * will return senseless values.  Return NULL which at least is an obviously
 * senseless value.
 */
#define return_address() NULL
#endif
#endif /* !defined (__LANGUAGE_ASSEMBLY__) */

#endif /* __ASM_MIPS_PROCESSOR_H */
