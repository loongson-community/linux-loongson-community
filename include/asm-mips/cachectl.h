/*
 * include/asm-mips/cachectl.h
 *
 * Written by Ralf Baechle,
 * Copyright (C) 1994 by Waldorf GMBH
 *
 * Defines for Risc/OS compatible cacheflush systemcall
 */
#ifndef	_ASM_MIPS_CACHECTL
#define	_ASM_MIPS_CACHECTL

/*
 * cachectl.h -- defines for MIPS cache control system calls
 */

/*
 * Options for cacheflush system call
 */
#define	ICACHE	(1<<0)		/* flush instruction cache        */
#define	DCACHE	(1<<1)		/* writeback and flush data cache */
#define	BCACHE	(ICACHE|DCACHE)	/* flush both caches              */

#define CACHELINES      512             /* number of cachelines    */

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

extern int sys_cacheflush(void *addr, int nbytes, int cache);

#endif
#endif
#endif	/* _ASM_MIPS_CACHECTL */
