/*
 * linux/include/asm-mips/ptrace.h
 *
 * machine dependend structs and defines to help the user use
 * the ptrace system call.
 */
#ifndef _ASM_MIPS_PTRACE_H
#define _ASM_MIPS_PTRACE_H

/*
 * use ptrace (3 or 6, pid, PT_EXCL, data); to read or write
 * the processes registers.
 *
 * This defines/structures corrospond to the register layout on stack -
 * if the order here is changed, it needs to be updated in
 * arch/mips/fork.c:copy_process, asm/mips/signal.c:do_signal,
 * asm-mips/ptrace.c, include/asm-mips/ptrace.h.
 */

#define IN_REG1             0
#define IN_REG2             1
#define IN_REG3             2
#define IN_REG4             3
#define IN_REG5             4
#define IN_REG6             5
#define IN_REG7             6
#define IN_REG8             7
#define IN_REG9             8
#define IN_REG10            9
#define IN_REG11            10
#define IN_REG12            11
#define IN_REG13            12
#define IN_REG14            13
#define IN_REG15            14
#define IN_REG16            15
#define IN_REG17            16
#define IN_REG18            17
#define IN_REG19            18
#define IN_REG20            19
#define IN_REG21            20
#define IN_REG22            21
#define IN_REG23            22
#define IN_REG24            23
#define IN_REG25            24

/*
 * k0 and k1 not saved
 */
#define IN_REG28            25
#define IN_REG29            26
#define IN_REG30            27
#define IN_REG31            28

/*
 * Saved special registers
 */
#define FR_LO			((IN_REG31) + 1)
#define FR_HI			((IN_LO) + 1)

/*
 * Saved cp0 registers
 */
#define IN_CP0_STATUS		((IN_LO) + 1)
#define IN_CP0_EPC		((IN_CP0_STATUS) + 1)
#define IN_CP0_ERROREPC		((IN_CP0_EPC) + 1)

/*
 * Some goodies...
 */
#define IN_INTERRUPT		((IN_CP0_ERROREPC) + 1)
#define IN_ORIG_REG2		((IN_INTERRUPT) + 1)

/*
 * this struct defines the way the registers are stored on the 
 * stack during a system call/exception. As usual the registers
 * k0/k1 aren't being saved.
 */

struct pt_regs {
	/*
	 * saved main processor registers
	 */
	long	        reg1,  reg2,  reg3,  reg4,  reg5,  reg6,  reg7;
	long	 reg8,  reg9, reg10, reg11, reg12, reg13, reg14, reg15;
	long	reg16, reg17, reg18, reg19, reg20, reg21, reg22, reg23;
	long	reg24, reg25,               reg28, reg29, reg30, reg31;
	/*
	 * Saved special registers
	 */
	long	lo;
	long	hi;
	/*
	 * saved cp0 registers
	 */
	unsigned long cp0_status;
	unsigned long cp0_epc;
	unsigned long cp0_errorepc;
	/*
	 * Some goodies...
	 */
	unsigned long interrupt;
	long orig_reg2;
};

/*
 * This function computes the interrupt number from the stack frame
 */
#define pt_regs2irq(p) ((int) ((struct pt_regs *)p)->interrupt)        

#endif /* _ASM_MIPS_PTRACE_H */
