#ifndef __ASM_MIPS_RESOURCE_H
#define __ASM_MIPS_RESOURCE_H

/*
 * Resource limits
 */

#define RLIMIT_CPU	0		/* CPU time in ms */
#define RLIMIT_FSIZE	1		/* Maximum filesize */
#define RLIMIT_DATA	2		/* max data size */
#define RLIMIT_STACK	3		/* max stack size */
#define RLIMIT_CORE	4		/* max core file size */
#define RLIMIT_NOFILE	5		/* max number of open files */
#define RLIMIT_VMEM	6		/* mapped memory */
#define RLIMIT_AS	RLIMIT_VMEM
#define RLIMIT_RSS	7		/* max resident set size */
#define RLIMIT_NPROC	8		/* max number of processes */

#ifdef notdef
#define RLIMIT_MEMLOCK	8		/* max locked-in-memory address space*/
#endif

#define RLIM_NLIMITS	9

#endif /* __ASM_MIPS_RESOURCE_H */
