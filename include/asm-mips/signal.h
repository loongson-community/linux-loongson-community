#ifndef _ASM_MIPS_SIGNAL_H
#define _ASM_MIPS_SIGNAL_H

#ifdef __KERNEL__

struct sigcontext_struct {
	/*
	 * In opposite to the SVr4 implentation in Risc/OS the
	 * sc_ra points to an address suitable to "jr ra" to.
	 * Registers that are callee saved by convention aren't
	 * being saved on entry of a signal handler.
	 */
	unsigned long	sc_at;
	unsigned long	sc_v0, sc_v1;
	unsigned long	sc_a0, sc_a1, sc_a2, sc_a3;
	unsigned long	sc_t0, sc_t1, sc_t2, sc_t3, sc_t4;
	unsigned long	sc_t5, sc_t6, sc_t7, sc_t8, sc_t9;
	/*
	 * Old userstack pointer ($29)
	 */
	unsigned long	sc_sp;

	unsigned long	oldmask;
};

#endif

#endif /* _ASM_MIPS_SIGNAL_H */
