/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
/*
 *  sb1250_handle_int() is the routine that is actually called when an interrupt
 *  occurs.  It is installed as the exception vector handler in init_IRQ()
 *  in arch/mips/sibyte/sb1250/irq.c
 *
 *  In the handle we figure out which interrupts need handling, and use that to call
 *  the dispatcher, which will take care of actually calling registered handlers 
 * 
 *  Note that we take care of all raised interrupts in one go at the handler.  This
 *  is more BSDish than the Indy code, and also, IMHO, more sane. 
 */
#include <linux/config.h>
#include <asm/addrspace.h>
#include <asm/processor.h>
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/sibyte/sb1250_regs.h>

	.text
	.set    push
	.set	noreorder
	.set	noat
	.align	5
	NESTED(sb1250_irq_handler, PT_SIZE, sp)
	SAVE_ALL
	/* Is the CLI really needed?  If it is, this looks suspiciously
	   like a place that would need a spinlock instead of a CLI.
	*/	   
	CLI                 

	mfc0     s0, $13
	/* Timer interrupt is routed to IP[4] */
	andi     t1, s0, 0x1000
	beqz     t1, 1f
	 nop
	jal      sb1250_timer_interrupt
	 move    a0, sp  /* Pass the registers along */

1:	
#ifdef CONFIG_SMP
	/* Mailbox interrupt is routed to IP[3] */
	andi	 t1, s0, 0x800
	beqz	 t1, 2f
	 nop
	jal	 sb1250_mailbox_interrupt
	 move    a0, sp		
2:
#endif
	and      t1, s0, 0x400
	beqz     t1, 4f
	 nop
	/* Default...we've hit an IP[2] interrupt, which means we've got to check the
	   1250 interrupt registers to figure out what to do */
	la       v0, KSEG1 + A_IMR_CPU0_BASE
	ld       s1, R_IMR_INTERRUPT_MASK(v0)
	ld       s0, R_IMR_INTERRUPT_SOURCE_STATUS(v0)
	ld	 t0, R_IMR_LDT_INTERRUPT(v0)
        nor      s1, s1, zero   /* Negate mask to turn it into an and mask */
	or	 s0, s0, t0     /* Merge pending system and LDT IRQS */
	and      s0, s0, s1     /* Now s0 has a bitfield of unmasked pending IRQs */
	and	 t0, t0, s1     /* LDT interrupts we will service */
	beqz     s0, 4f         /* No interrupts.  Return */
	 daddiu	  a1, sp, 0     /* registers get passed along as the second argument */
	sd	 t0, R_IMR_LDT_INTERRUPT_CLR(v0)  /* Clear what we'll service */ 

3:
	.word    0x72118824	/* find next interrupt, actually a dclz s1,s0 */
	dsubu    a0, zero, s1
	jal      sb1250_dispatch_irq /* Handle the interrupt */
	 daddiu   a0, a0, 63     
	daddiu   s1, s1, 1    /* Get shift amount for clearing of top interrupt */
	dsllv    s0, s0, s1   /* Clear the top interrupt           */
	bnez     s0, 3b       /* More interrupts to service?       */
	 dsrlv    s0, s0, s1   /* realign pending interrupts        */
4:                        
	j        ret_from_irq   /* defined in arch/mips/kernel/entry.S */
	nop

	.set pop
	END(sb1250_irq_handler)
