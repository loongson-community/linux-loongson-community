/*
 * include/asm-mips/slots.h
 *
 * Copyright (C) 1994 by Waldorf Electronics
 * Written by Ralf Baechle
 */
#ifndef __ASM_MIPS_SLOTS_H
#define __ASM_MIPS_SLOTS_H

#include <linux/config.h>

/*
 * SLOTSPACE is the address to which the physical address 0
 * of the Slotspace is mapped by the chipset in the main CPU's
 * address space.
 */
#ifdef CONFIG_DESKSTATION_RPC44
#define SLOTSPACE 0xa0000000
#else
#define SLOTSPACE 0xe1000000
#endif

#endif /* __ASM_MIPS_SLOTS_H */
