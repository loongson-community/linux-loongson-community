/*
 * Setup pointers to hardware dependand routines.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */
#include <asm/ptrace.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/jazz.h>
#include <asm/processor.h>
#include <asm/vector.h>
#include <asm/io.h>

/*
 * Initial irq handlers.
 */
static void no_action(int cpl, void *dev_id, struct pt_regs *regs) { }

/*
 * IRQ2 is cascade interrupt to second interrupt controller
 */
static struct irqaction irq2  = { no_action, 0, 0, "cascade", NULL, NULL};

extern asmlinkage void jazz_handle_int(void);
extern asmlinkage void jazz_fd_cacheflush(const void *addr, size_t size);
extern struct feature jazz_feature;
extern void (*ibe_board_handler)(struct pt_regs *regs);
extern void (*dbe_board_handler)(struct pt_regs *regs);

static void
jazz_irq_setup(void)
{
        set_except_vector(0, jazz_handle_int);
	r4030_write_reg16(JAZZ_IO_IRQ_ENABLE,
			  JAZZ_IE_ETHERNET |
			  JAZZ_IE_SERIAL1  |
			  JAZZ_IE_SERIAL2  |
 			  JAZZ_IE_PARALLEL |
			  JAZZ_IE_FLOPPY);
	r4030_read_reg16(JAZZ_IO_IRQ_SOURCE); /* clear pending IRQs */
	r4030_read_reg32(JAZZ_R4030_INVAL_ADDR); /* clear error bits */
	set_cp0_status(ST0_IM, IE_IRQ4 | IE_IRQ3 | IE_IRQ2 | IE_IRQ1);
	request_region(0x20, 0x20, "pic1");
	request_region(0xa0, 0x20, "pic2");
	setup_x86_irq(2, &irq2);
}

void (*board_time_init)(struct irqaction *irq);

static void jazz_time_init(struct irqaction *irq)
{
	/* set the clock to 100 Hz */
	r4030_write_reg32(JAZZ_TIMER_INTERVAL, 9);
	setup_x86_irq(0, irq);
}

/*
 * The ibe/dbe exceptions are signaled by onboard hardware and should get
 * a board specific handlers to get maximum available information. Bus
 * errors are always symptom of hardware malfunction or a kernel error.
 * We should try to handle this case a bit more gracefully than just
 * zapping the process ...
 */
static void jazz_be_board_handler(struct pt_regs *regs)
{
	u32	jazz_is, jazz_ia;

	/*
	 * Give some debugging aid ...
	 */
	jazz_is = r4030_read_reg32(JAZZ_R4030_IRQ_SOURCE);
	jazz_ia = r4030_read_reg32(JAZZ_R4030_INVAL_ADDR);
	printk("Interrupt Source         == %08x\n", jazz_is);
	printk("Invalid Address Register == %08x\n", jazz_ia);
	show_regs(regs);

	/*
	 * Assume it would be too dangerous to continue ...
	 */
	force_sig(SIGBUS, current);
}

void
jazz_setup(void)
{
	tag *atag;

	/*
	 * we just check if a tag_screen_info can be gathered
	 * in setup_arch(), if yes we don't proceed futher...
	 */
	atag = bi_TagFind(tag_screen_info);
	if (!atag) {
		/*
		 * If no, we try to find the tag_arc_displayinfo which is
		 * always created by Milo for an ARC box (for now Milo only
		 * works on ARC boxes :) -Stoned.
		 */
		atag = bi_TagFind(tag_arcdisplayinfo);
		if (atag) {
			screen_info.orig_x = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->cursor_x;
			screen_info.orig_y = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->cursor_y;
			screen_info.orig_video_cols  = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->columns;
			screen_info.orig_video_lines  = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->lines;
		}
	}
	irq_setup = jazz_irq_setup;
	board_time_init = jazz_time_init;
	fd_cacheflush = jazz_fd_cacheflush;
	feature = &jazz_feature;			// Will go away
	port_base = PORT_BASE_JAZZ;
	isa_slot_offset = 0xe3000000;
	request_region(0x00,0x20,"dma1");
	request_region(0x40,0x20,"timer");
	/* The RTC is outside the port address space */

	if (mips_machtype == MACH_MIPS_MAGNUM_4000
	    && mips_machtype == MACH_OLIVETTI_M700)
		EISA_bus = 1;
	/*
	 * The Jazz hardware provides additional information for
	 * bus errors, so we use an special handler.
	 */
	ibe_board_handler = jazz_be_board_handler;
	dbe_board_handler = jazz_be_board_handler;
}
