/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 1999, 2001 MIPS Technologies, Inc.
 */
#ifndef __ASM_SOFTIRQ_H
#define __ASM_SOFTIRQ_H

#include <linux/preempt.h>
#include <asm/hardirq.h>

#define local_bh_disable() \
	do { preempt_count() += SOFTIRQ_OFFSET; barrier(); } while (0)
#define __local_bh_enable() \
	do { barrier(); preempt_count() -= SOFTIRQ_OFFSET; } while (0)

#define local_bh_enable()						\
do {									\
	__local_bh_enable();						\
	if (unlikely(!in_interrupt() && softirq_pending(smp_processor_id()))) \
		do_softirq();						\
	preempt_check_resched();					\
} while (0)

#define local_bh_enable() do { _local_bh_enable(); preempt_enable(); } while (0)

#endif /* __ASM_SOFTIRQ_H */
