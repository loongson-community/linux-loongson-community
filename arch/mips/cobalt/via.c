/*
 * VIA chipset irq handling
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997 by Ralf Baechle
 * Copyright (C) 2001 by Liam Davies (ldavies@agile.tv)
 *
 */

#include <linux/kernel.h>
#include <asm/cobalt/cobalt.h>
#include <asm/ptrace.h>
#include <asm/io.h>
#include <asm/mipsregs.h>

extern void do_IRQ(int irq, struct pt_regs * regs);

/* Cached values of VIA PIC masks, we start with cascade enabled */
static unsigned int cached_irq_mask = 0xfffb;

#define __byte(x,y)	(((unsigned char *)&(y))[x])
#define cached_21	(__byte(0,cached_irq_mask))
#define cached_A1	(__byte(1,cached_irq_mask))


void mask_irq(unsigned int irq)
{
	unsigned int mask = 1 << irq;

	cached_irq_mask |= mask;
	if (irq & 8) {
		VIA_PORT_WRITE(0xA1, cached_A1);
	} else {
		VIA_PORT_WRITE(0x21, cached_21);
	}
}

void unmask_irq(unsigned int irq)
{
	unsigned int mask = ~(1 << irq);

	cached_irq_mask &= mask;
	if (irq & 8) {
		VIA_PORT_WRITE(0xA1, cached_A1);
	} else {
		VIA_PORT_WRITE(0x21, cached_21);
	}
}

asmlinkage void via_irq(struct pt_regs *regs)
{
	char mstat, sstat;
  
	/* Read Master Status */
	VIA_PORT_WRITE(0x20, 0x0C);
	mstat = VIA_PORT_READ(0x20);
 
	if (mstat < 0) {
		mstat &= 0x7f;
		if (mstat != 2) {     	
			do_IRQ(mstat, regs);
			VIA_PORT_WRITE(0x20, mstat | 0x20);
		} else {
			sstat = VIA_PORT_READ(0xA0);

			/* Slave interrupt */
			VIA_PORT_WRITE(0xA0, 0x0C);
			sstat = VIA_PORT_READ(0xA0);
   
			if (sstat < 0) {
				do_IRQ((sstat + 8) & 0x7f, regs);
				VIA_PORT_WRITE(0x20, 0x22);       
				VIA_PORT_WRITE(0xA0, (sstat & 0x7f) | 0x20);
			} else {
				printk("Spurious slave interrupt...\n");
			}
		}
	} else
		printk("Spurious master interrupt...");
}

#define GALILEO_INTCAUSE	0xb4000c18
#define GALILEO_T0EXP		0x00000100

asmlinkage void galileo_irq(struct pt_regs *regs)
{
	unsigned long irq_src = *((unsigned long *) GALILEO_INTCAUSE); 

	/* Check for timer irq ... */
	if (irq_src & GALILEO_T0EXP) {
		/* Clear the int line */
		*((volatile unsigned long *) GALILEO_INTCAUSE) = 0;
		do_IRQ(0, regs);
	} else
		printk("Spurious Galileo interrupt...\n");
}
