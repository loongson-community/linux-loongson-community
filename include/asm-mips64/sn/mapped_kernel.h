/*
 * File created by Kanoj Sarcar 06/06/00.
 * Copyright 2000 Silicon Graphics, Inc.
 */
#ifndef __ASM_SN_MAPPED_KERNEL_H
#define __ASM_SN_MAPPED_KERNEL_H

#include <linux/config.h>
#include <asm/addrspace.h>

#ifdef CONFIG_MAPPED_KERNEL

#define MAPPED_KERN_RO_TO_PHYS(x)	(x - CKSSEG)

#else /* CONFIG_MAPPED_KERNEL */

#define MAPPED_KERN_RO_TO_PHYS(x)	(x - CKSEG0)

#endif /* CONFIG_MAPPED_KERNEL */

#define MAPPED_KERN_RO_TO_K0(x)	PHYS_TO_K0(MAPPED_KERN_RO_TO_PHYS(x))

#endif __ASM_SN_MAPPED_KERNEL_H
