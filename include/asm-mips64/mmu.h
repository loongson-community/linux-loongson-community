#ifndef __ASM_MMU_H
#define __ASM_MMU_H

#include <linux/config.h>

#ifdef CONFIG_SMP
typedef unsigned long * mm_context_t;
#else
typedef unsigned long mm_context_t;
#endif

#endif /* __ASM_MMU_H */
