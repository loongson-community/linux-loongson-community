#ifndef _ASM_I386_PTRACE_H
#define _ASM_I386_PTRACE_H

/*
 * linux/include/asm-i386/ptrace.h */
 *
 * machine dependend structs and defines to help the user use
 * the ptrace system call.
 */

/* use ptrace (3 or 6, pid, PT_EXCL, data); to read or write
   the processes registers. */

#define EBX 0
#define ECX 1
#define EDX 2
#define ESI 3
#define EDI 4
#define EBP 5
#define EAX 6
#define DS 7
#define ES 8
#define FS 9
#define GS 10
#define ORIG_EAX 11
#define EIP 12
#define CS  13
#define EFL 14
#define UESP 15
#define SS   16


/* this struct defines the way the registers are stored on the 
   stack during a system call. */

struct pt_regs {
	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	unsigned short ds, __dsu;
	unsigned short es, __esu;
	unsigned short fs, __fsu;
	unsigned short gs, __gsu;
	long orig_eax;
	long eip;
	unsigned short cs, __csu;
	long eflags;
	long esp;
	unsigned short ss, __ssu;
};

/*
 * This function computes the interrupt number from the stack frame
 */
#define pt_regs2irq(p) ((int) -(((struct pt_regs *)p)->orig_eax+2))

#endif /* _ASM_I386_PTRACE_H */
