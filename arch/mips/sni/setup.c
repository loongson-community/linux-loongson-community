/*
 * Setup pointers to hardware-dependent routines.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1998, 2000, 2003 by Ralf Baechle
 */
#include <linux/config.h>
#include <linux/eisa.h>
#include <linux/hdreg.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/mc146818rtc.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/ide.h>
#include <linux/tty.h>

#include <asm/bcache.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/pci_channel.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/reboot.h>
#include <asm/sni.h>
#include <asm/time.h>
#include <asm/traps.h>

extern void sni_machine_restart(char *command);
extern void sni_machine_halt(void);
extern void sni_machine_power_off(void);

extern struct ide_ops std_ide_ops;
extern struct rtc_ops std_rtc_ops;

static void __init sni_rm200_pci_timer_setup(struct irqaction *irq)
{
	/* set the clock to 100 Hz */
	outb_p(0x34,0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */
	setup_irq(0, irq);
}


extern unsigned char sni_map_isa_cache;

/*
 * A bit more gossip about the iron we're running on ...
 */
static inline void sni_pcimt_detect(void)
{
	char boardtype[80];
	unsigned char csmsr;
	char *p = boardtype;
	unsigned int asic;

	csmsr = *(volatile unsigned char *)PCIMT_CSMSR;

	p += sprintf(p, "%s PCI", (csmsr & 0x80) ? "RM200" : "RM300");
	if ((csmsr & 0x80) == 0)
		p += sprintf(p, ", board revision %s",
		             (csmsr & 0x20) ? "D" : "C");
	asic = csmsr & 0x80;
	asic = (csmsr & 0x08) ? asic : !asic;
	p += sprintf(p, ", ASIC PCI Rev %s", asic ? "1.0" : "1.1");
	printk("%s.\n", boardtype);
}

struct resource pcimt_io_resources[] = {
	{ "dma1", 0x00, 0x1f, IORESOURCE_BUSY },
	{ "timer", 0x40, 0x5f, IORESOURCE_BUSY },
	{ "keyboard", 0x60, 0x6f, IORESOURCE_BUSY },
	{ "dma page reg", 0x80, 0x8f, IORESOURCE_BUSY },
	{ "dma2", 0xc0, 0xdf, IORESOURCE_BUSY },
	{ "PCI config data", 0xcfc, 0xcff, IORESOURCE_BUSY }
};

#define PCIMT_IO_RESOURCES (sizeof(pcimt_io_resources)/sizeof(struct resource))

static struct resource sni_mem_resource = {
	.name	= "PCIMT PCI MEM",
	.start	= 0x14000000UL,
	.end	= 0x17ffffffUL,
	.flags	= IORESOURCE_MEM,
};

static struct resource sni_io_resource = {
	.name	= "PCIMT IO MEM",
	.start	= 0x00000000UL,
	.end	= 0x03ffffffUL,
	.flags	= IORESOURCE_IO,
};

extern struct pci_ops sni_pci_ops;

static struct pci_controller sni_controller = {
	.pci_ops	= &sni_pci_ops,
	.io_resource	= &sni_io_resource,
	.mem_resource	= &sni_mem_resource
};

void __init sni_rm200_pci_setup(void)
{
	int i;

	sni_pcimt_detect();
	sni_pcimt_sc_init();

	set_io_port_base(SNI_PORT_BASE);
	ioport_resource.end = sni_io_resource.end;

	/*
	 * Setup (E)ISA I/O memory access stuff
	 */
	isa_slot_offset = 0xb0000000;
	// sni_map_isa_cache = 0;
#ifdef CONFIG_EISA
	EISA_bus = 1;
#endif

	/* request I/O space for devices used on all i[345]86 PCs */
	for (i = 0; i < PCIMT_IO_RESOURCES; i++)
		request_resource(&ioport_resource, pcimt_io_resources + i);

	board_timer_setup = sni_rm200_pci_timer_setup;

	_machine_restart = sni_machine_restart;
	_machine_halt = sni_machine_halt;
	_machine_power_off = sni_machine_power_off;

#ifdef CONFIG_BLK_DEV_IDE
	ide_ops = &std_ide_ops;
#endif

#ifdef CONFIG_VT
#if defined(CONFIG_VGA_CONSOLE)
	conswitchp = &vga_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
#endif

	screen_info = (struct screen_info) {
		.orig_x			= 0,		// XXX
		.orig_y			= 0,		// XXX
		.dontuse1		= 0,
		.orig_video_page	= 52,
		.orig_video_mode	= 3,
		.orig_video_cols	= 80,		// XXX
		.unused2		= 4626,
		.orig_video_ega_bx	= 3,
		.unused3		= 9,
		.orig_video_lines	= 25,		// XXX
		.orig_video_isVGA	= VIDEO_TYPE_VGAC,
		.orig_video_points	= 16		// XXX
	};

	rtc_ops = &std_rtc_ops;

	register_pci_controller(&sni_controller);
}
