/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000, 2001 MIPS Technologies, Inc.
 * Copyright (C) 2001 Ralf Baechle
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Routines for generic manipulation of the interrupts found on the MIPS 
 * Malta board.
 * The interrupt controller is located in the South Bridge a PIIX4 device 
 * with two internal 82C95 interrupt controllers.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/random.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mips-boards/malta.h>
#include <asm/mips-boards/maltaint.h>
#include <asm/mips-boards/piix4.h>
#include <asm/gt64120.h>
#include <asm/mips-boards/generic.h>
#include <asm/mips-boards/msc01_pci.h>

extern asmlinkage void mipsIRQ(void);
extern asmlinkage void do_IRQ(int irq, struct pt_regs *regs);
extern void init_i8259_irqs (void);

void enable_mips_irq(unsigned int irq);
void disable_mips_irq(unsigned int irq);

static spinlock_t mips_irq_lock = SPIN_LOCK_UNLOCKED;

static void end_mips_irq (unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_mips_irq(irq);
}

#define shutdown_mips_irq	disable_mips_irq

void mask_and_ack_mips_irq(unsigned int irq);

static unsigned int startup_mips_irq(unsigned int irq)
{ 
	enable_mips_irq(irq);

	return 0; /* never anything pending */
}

static struct hw_interrupt_type mips_irq_type = {
	"XT-PIC",
	startup_mips_irq,
	shutdown_mips_irq,
	enable_mips_irq,
	disable_mips_irq,
	mask_and_ack_mips_irq,
	end_mips_irq,
	NULL
};

/*
 * This contains the interrupt mask for both 82C59 interrupt controllers.
 */
static unsigned int cached_int_mask = 0xffff;

void disable_mips_irq(unsigned int irq_nr)
{
        unsigned long flags;

	if(irq_nr >= MALTAINT_END) {
		printk("whee, invalid irq_nr %d\n", irq_nr);
		panic("IRQ, you lose...");
	}

	spin_lock_irqsave(&mips_irq_lock, flags);
	cached_int_mask |= (1 << irq_nr);
	if (irq_nr & 8) {
	        outb((cached_int_mask >> 8) & 0xff, PIIX4_ICTLR2_OCW1);
	} else {
		outb(cached_int_mask & 0xff, PIIX4_ICTLR1_OCW1);
	}
	spin_unlock_irqrestore(&mips_irq_lock, flags);
}

void enable_mips_irq(unsigned int irq_nr)
{
        unsigned long flags;

	if(irq_nr >= MALTAINT_END) {
		printk("whee, invalid irq_nr %d\n", irq_nr);
		panic("IRQ, you lose...");
	}

	spin_lock_irqsave(&mips_irq_lock, flags);
	cached_int_mask &= ~(1 << irq_nr);
	if (irq_nr & 8) {
		outb((cached_int_mask >> 8) & 0xff, PIIX4_ICTLR2_OCW1);

		/* Enable irq 2 (cascade interrupt). */
	        cached_int_mask &= ~(1 << 2); 
		outb(cached_int_mask & 0xff, PIIX4_ICTLR1_OCW1);
	} else {
		outb(cached_int_mask & 0xff, PIIX4_ICTLR1_OCW1);
	}	
	spin_unlock_irqrestore(&mips_irq_lock, flags);
}

void mask_and_ack_mips_irq(unsigned int irq)
{
        unsigned long flags;

	spin_lock_irqsave(&mips_irq_lock, flags);
	if (irq & 8) {
	        /* Non specific EOI to cascade */
		outb(PIIX4_OCW2_SEL | PIIX4_OCW2_NSEOI | PIIX4_OCW2_ILS_2, 
		     PIIX4_ICTLR1_OCW2);

		outb(PIIX4_OCW2_SEL | PIIX4_OCW2_NSEOI, PIIX4_ICTLR2_OCW2);
	} else {
		outb(PIIX4_OCW2_SEL | PIIX4_OCW2_NSEOI, PIIX4_ICTLR1_OCW2);
	}
	spin_unlock_irqrestore(&mips_irq_lock, flags);
}

static inline int get_int(int *irq)
{
	unsigned char irr;
	
	switch(mips_revision_corid) {
	case MIPS_REVISION_CORID_QED_RM5261:
	case MIPS_REVISION_CORID_CORE_LV:
	case MIPS_REVISION_CORID_CORE_FPGA:
	case MIPS_REVISION_CORID_CORE_MSC:
		/*  
		 * Determine highest priority pending interrupt by performing
		 * a PCI Interrupt Acknowledge cycle.
		 */
		if (mips_revision_corid == MIPS_REVISION_CORID_CORE_MSC)
			MSC_READ(MSC01_PCI_IACK, *irq);
		else
			GT_READ(GT_PCI0_IACK_OFS, *irq);
		*irq &= 0xFF;

		/*  
		 * IRQ7 is used to detect spurious interrupts.
		 * The interrupt acknowledge cycle returns IRQ7, if no 
		 * interrupts is requested.
		 * We can differentiate between this situation and a
		 * "Normal" IRQ7 by reading the ISR.
		 */
		if (*irq == 7) 
		{
			outb(PIIX4_OCW3_SEL | PIIX4_OCW3_ISR, 
			     PIIX4_ICTLR1_OCW3);
			if (!(inb(PIIX4_ICTLR1_OCW3) & (1 << 7))) {
				printk("We got a spurious interrupt from PIIX4.\n");
				atomic_inc(&irq_err_count);
				return -1;    /* Spurious interrupt. */
			}
		}
		break;

	case MIPS_REVISION_CORID_BONITO64:
	case MIPS_REVISION_CORID_CORE_20K:
	        /*
		 * Determine highest priority pending interrupt by reading 
		 * the IRR register of controller 1.
		 */

		outb(PIIX4_OCW3_SEL | PIIX4_OCW3_IRR, PIIX4_ICTLR1_OCW3);
		irr = inb(PIIX4_ICTLR1_OCW3) & ~(cached_int_mask & 0xff);

		if (!irr) {
		        printk("We got a spurious interrupt from PIIX4.\n");
			atomic_inc(&irq_err_count);
			return -1;   /* Spurious interrupt. */
		}
		*irq = 0;
		while ((*irq < 7) && !(irr & 0x01)) {
		        (*irq)++;
			irr >>= 1;
		}

		if (*irq == 2)
		{
		        /*
			 * Determine highest priority pending interrupt by 
			 * reading the IRR register of controller 1.
			 */
		        outb(PIIX4_OCW3_SEL | PIIX4_OCW3_IRR, 
			     PIIX4_ICTLR2_OCW3);
			irr = inb(PIIX4_ICTLR2_OCW3) & 
			~((cached_int_mask >> 8) & 0xff);

			*irq = 8;
			while ((*irq < 15) && !(irr & 0x01)) {
			        (*irq)++;
				irr >>= 1;
			}
		}
		break;
	default:
	        printk("Unknown Core card, don't know the system controller.\n");
		return -1;
	}

	return 0;
}

void malta_hw0_irqdispatch(struct pt_regs *regs)
{
	int irq;
	struct irqaction *action;

	if (get_int(&irq))
	        return;  /* interrupt has already been cleared */

	switch(mips_revision_corid) {
	case MIPS_REVISION_CORID_BONITO64:
	case MIPS_REVISION_CORID_CORE_20K:
		action = irq_desc[irq].action;
		if (!action)
			return;
		action->flags |= SA_INTERRUPT;
		break;
	}
	
	do_IRQ(irq, regs);
}

void corehi_irqdispatch(struct pt_regs *regs)
{
        unsigned int data,datahi;

        printk("CoreHI interrupt, shouldn't happen, so we die here!!!\n");
        printk("epc   : %08lx\nStatus: %08lx\nCause : %08lx\nbadVaddr : %08lx\n"
, regs->cp0_epc, regs->cp0_status, regs->cp0_cause, regs->cp0_badvaddr);
        switch(mips_revision_corid) {
        case MIPS_REVISION_CORID_CORE_MSC:
                break;
        case MIPS_REVISION_CORID_QED_RM5261:
        case MIPS_REVISION_CORID_CORE_LV:
        case MIPS_REVISION_CORID_CORE_FPGA:
                GT_READ(GT_INTRCAUSE_OFS, data);
                printk("GT_INTRCAUSE = %08x\n", data);
                GT_READ(0x70, data);
                GT_READ(0x78, datahi);
                printk("GT_CPU_ERR_ADDR = %0x2%08x\n", datahi,data);
                break;
        case MIPS_REVISION_CORID_BONITO64:
        case MIPS_REVISION_CORID_CORE_20K:
                data = BONITO_INTISR;
                printk("BONITO_INTISR = %08x\n", data);
                data = BONITO_INTEN;
                printk("BONITO_INTEN = %08x\n", data);
                data = BONITO_INTPOL;
                printk("BONITO_INTPOL = %08x\n", data);
                data = BONITO_INTEDGE;
                printk("BONITO_INTEDGE = %08x\n", data);
                data = BONITO_INTSTEER;
                printk("BONITO_INTSTEER = %08x\n", data);
                data = BONITO_PCICMD;
                printk("BONITO_PCICMD = %08x\n", data);
                break;
        }

        /* We die here*/
        die("CoreHi interrupt", regs);
        while (1) ;
}

void __init init_IRQ(void)
{
	unsigned int i;

	set_except_vector(0, mipsIRQ);
	init_generic_irq();
	
	switch(mips_revision_corid) {
	case MIPS_REVISION_CORID_QED_RM5261:
	case MIPS_REVISION_CORID_CORE_LV:
	case MIPS_REVISION_CORID_CORE_FPGA:
	case MIPS_REVISION_CORID_CORE_MSC:
		init_i8259_irqs();
		break;
	case MIPS_REVISION_CORID_BONITO64:
	case MIPS_REVISION_CORID_CORE_20K:
		/* 
		 * Mask out all interrupt by writing "1" to all bit position 
		 * in the IMR register. 
		 */
		outb(cached_int_mask & 0xff, PIIX4_ICTLR1_OCW1);
		outb((cached_int_mask >> 8) & 0xff, PIIX4_ICTLR2_OCW1);

		for (i = 0; i < 16; i++) {
			irq_desc[i].status = IRQ_DISABLED;
			irq_desc[i].action = 0;
			irq_desc[i].depth = 1;
			irq_desc[i].handler = &mips_irq_type;
		}
		break;
	default:
	        printk("Unknown Core card, don't know the system controller.\n");
		return;
	}

#ifdef CONFIG_REMOTE_DEBUG
	if (remote_debug) {
		set_debug_traps();
		breakpoint();
	}
#endif
}


