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
#include <linux/config.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/timex.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/processor.h>
#include <asm/vector.h>

/*
 * Initial irq handlers.
 */
static void no_action(int cpl, void *dev_id, struct pt_regs *regs) { }

/*
 * IRQ2 is cascade interrupt to second interrupt controller
 */
static struct irqaction irq2  = { no_action, 0, 0, "cascade", NULL, NULL};

extern asmlinkage void deskstation_handle_int(void);
extern asmlinkage void deskstation_fd_cacheflush(const void *addr, size_t size);
extern struct feature deskstation_tyne_feature;
extern struct feature deskstation_rpc44_feature;

#ifdef CONFIG_DESKSTATION_TYNE
unsigned long mips_dma_cache_size = 0;
unsigned long mips_dma_cache_base = KSEG0;

static void
tyne_irq_setup(void)
{
	set_except_vector(0, deskstation_handle_int);
	request_region(0x20,0x20, "pic1");
	request_region(0xa0,0x20, "pic2");	
	setup_x86_irq(2, &irq2);
}
#endif

#ifdef CONFIG_DESKSTATION_RPC44
static void
rpc44_irq_setup(void)
{
	/*
	 * For the moment just steal the TYNE support.  In the
	 * future, we need to consider merging the two -- imp
	 */
	set_except_vector(0, deskstation_handle_int);
	request_region(0x20,0x20, "pic1");
	request_region(0xa0,0x20, "pic2");
	setup_x86_irq(2, &irq2);
	set_cp0_status(ST0_IM, IE_IRQ4 | IE_IRQ3 | IE_IRQ2 | IE_IRQ1);
}
#endif

void (*board_time_init)(struct irqaction *irq);

static void deskstation_time_init(struct irqaction *irq)
{
	/* set the clock to 100 Hz */
	outb_p(0x34,0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */
	setup_x86_irq(0, irq);
}

void
deskstation_setup(void)
{
	tag *atag;

	/*
	 * We just check if a tag_screen_info can be gathered
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

	switch(mips_machtype) {
#ifdef CONFIG_DESKSTATION_TYNE
	case MACH_DESKSTATION_TYNE:
		atag = bi_TagFind(tag_dma_cache_size);
		memcpy(&mips_dma_cache_size, TAGVALPTR(atag), atag->size);

		atag = bi_TagFind(tag_dma_cache_base);
		memcpy(&mips_dma_cache_base, TAGVALPTR(atag), atag->size);

		irq_setup = tyne_irq_setup;
		feature = &deskstation_tyne_feature;	// Will go away
		port_base = PORT_BASE_TYNE;
		isa_slot_offset = 0xe3000000;
		break;
#endif
#ifdef CONFIG_DESKSTATION_RPC44
	case MACH_DESKSTATION_RPC44:
		irq_setup = rpc44_irq_setup;
		mips_memory_upper = KSEG0 + (32 << 20); /* xxx fixme imp */
		feature = &deskstation_rpc44_feature;	// Will go away
		port_base = PORT_BASE_RPC44;
		isa_slot_offset = 0xa0000000;
		break;
#endif
	}
	board_time_init = deskstation_time_init;
	fd_cacheflush = deskstation_fd_cacheflush;
	request_region(0x00,0x20,"dma1");
	request_region(0x40,0x20,"timer");
	request_region(0x70,0x10,"rtc");

	if (mips_machtype == MACH_DESKSTATION_RPC44)
		EISA_bus = 1;
}
