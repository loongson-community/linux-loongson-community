#ifndef __ASM_MIPS_ELF_H
#define __ASM_MIPS_ELF_H

/*
 * ELF register definitions
 * This is "make it compile" stuff!
 */
#define ELF_NGREG	32
#define ELF_NFPREG	32

typedef unsigned long elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef double elf_fpreg_t;
typedef elf_fpreg_t elf_fpregset_t[ELF_NFPREG];

/*
 * This is used to ensure we don't load something for the wrong architecture.
 * Using EM_MIPS is actually wrong - this one is reserved for big endian
 * machines only but there is no EM_ constant for little endian ...
 */
#define elf_check_arch(x) ((x) == EM_MIPS || (x) == EM_MIPS_RS4_BE)

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS32
#ifdef __MIPSEB__
#define ELF_DATA	ELFDATA2MSB;
#elif __MIPSEL__
#define ELF_DATA	ELFDATA2LSB;
#endif
#define ELF_ARCH	EM_MIPS

	/* SVR4/i386 ABI (pages 3-31, 3-32) says that when the program
	starts %edx contains a pointer to a function which might be
	registered using `atexit'.  This provides a mean for the
	dynamic linker to call DT_FINI functions for shared libraries
	that have been loaded before the code runs.

	A value of 0 tells we have no such handler.  */
#define ELF_PLAT_INIT(_r)       _r->regs[2] = 0

#define USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE	4096

#endif /* __ASM_MIPS_ELF_H */
