/***************************************************************************
 *  Silicon Motion VoyaagerGX framebuffer driver
 *
 * 	ported to 2.6 by Embedded Alley Solutions, Inc
 * 	Copyright (C) 2005 Embedded Alley Solutions, Inc
 *
 * 		based on
    copyright            : (C) 2001 by Szu-Tao Huang
    email                : johuang@siliconmotion.com
    Updated to SM501 by Eric.Devolder@amd.com and dan@embeddededge.com
    for the AMD Mirage Portable Tablet.  20 Oct 2003
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/pci.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/uaccess.h>

static char *SMIRegs;		// point to virtual Memory Map IO starting address
static char *SMILFB;		// point to virtual video memory starting address

static struct fb_fix_screeninfo smifb_fix __initdata = {
	.id =		"smivgx",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_TRUECOLOR,
	.ywrapstep = 	0,
	.xpanstep = 	1,
	.ypanstep = 	1,
	.line_length	= 1024 * 2, /* (bbp * xres)/8 */
	.accel =	FB_ACCEL_NONE,
};

static struct fb_var_screeninfo smifb_var = {
	.xres           = 1024,
	.yres           = 768,
	.xres_virtual   = 1024,
	.yres_virtual   = 768,
	.bits_per_pixel = 16,
	.red            = { 11, 5, 0 },
	.green          = {  5, 6, 0 },
	.blue           = {  0, 5, 0 },
	.activate       = FB_ACTIVATE_NOW,
	.height         = -1,
	.width          = -1,
	.vmode          = FB_VMODE_NONINTERLACED,
};      


static struct fb_info info;

#include "smivgxfb.h"

static int initdone = 0;
static int crt_out = 1;


static int
smi_setcolreg(unsigned regno, unsigned red, unsigned green,
	unsigned blue, unsigned transp,
	struct fb_info *info)
{
	if (regno > 255)
		return 1;

	((u16 *)(info->pseudo_palette))[regno] = 
		    ((red & 0xf800) >> 0) |
		    ((green & 0xfc00) >> 5) |
		    ((blue & 0xf800) >> 11);

	return 0;
}

/* This function still needs lots of work to generically support
 * different output devices (CRT or LCD) and resolutions.
 * Currently hard-coded for 1024x768 LCD panel.
 */
void
smi_setmode(void)
{
	if (initdone)
		return;

	initdone = 1;

	/* Just blast in some control values based upon the chip
	 * documentation.  We use the internal memory, I don't know
	 * how to determine the amount available yet.
	 */
	smi_mmiowl(0x07F127C2, DRAM_CTRL);
	smi_mmiowl(0x02000020, PANEL_HWC_ADDRESS);
	smi_mmiowl(0x007FF800, PANEL_HWC_ADDRESS);
	smi_mmiowl(0x00021827, POWER_MODE1_GATE);
	smi_mmiowl(0x011A0A09, POWER_MODE1_CLOCK);
	smi_mmiowl(0x00000001, POWER_MODE_CTRL);
	smi_mmiowl(0x80000000, PANEL_FB_ADDRESS);
	smi_mmiowl(0x08000800, PANEL_FB_WIDTH);
	smi_mmiowl(0x04000000, PANEL_WINDOW_WIDTH);
	smi_mmiowl(0x03000000, PANEL_WINDOW_HEIGHT);
	smi_mmiowl(0x00000000, PANEL_PLANE_TL);
	smi_mmiowl(0x02FF03FF, PANEL_PLANE_BR);
	smi_mmiowl(0x05D003FF, PANEL_HORIZONTAL_TOTAL);
	smi_mmiowl(0x00C80424, PANEL_HORIZONTAL_SYNC);
	smi_mmiowl(0x032502FF, PANEL_VERTICAL_TOTAL);
	smi_mmiowl(0x00060302, PANEL_VERTICAL_SYNC);
	smi_mmiowl(0x00013905, PANEL_DISPLAY_CTRL);
	smi_mmiowl(0x01013105, PANEL_DISPLAY_CTRL);
	waitforvsync();
	smi_mmiowl(0x03013905, PANEL_DISPLAY_CTRL);
	waitforvsync();
	smi_mmiowl(0x07013905, PANEL_DISPLAY_CTRL);
	waitforvsync();
	smi_mmiowl(0x0F013905, PANEL_DISPLAY_CTRL);
	smi_mmiowl(0x0002187F, POWER_MODE1_GATE);
	smi_mmiowl(0x01011801, POWER_MODE1_CLOCK);
	smi_mmiowl(0x00000001, POWER_MODE_CTRL);

	smi_mmiowl(0x80000000, PANEL_FB_ADDRESS);
	smi_mmiowl(0x00000000, PANEL_PAN_CTRL);
	smi_mmiowl(0x00000000, PANEL_COLOR_KEY);

	if (crt_out) {
		/* Just sent the panel out to the CRT for now.
		*/
		smi_mmiowl(0x80000000, CRT_FB_ADDRESS);
		smi_mmiowl(0x08000800, CRT_FB_WIDTH);
		smi_mmiowl(0x05D003FF, CRT_HORIZONTAL_TOTAL);
		smi_mmiowl(0x00C80424, CRT_HORIZONTAL_SYNC);
		smi_mmiowl(0x032502FF, CRT_VERTICAL_TOTAL);
		smi_mmiowl(0x00060302, CRT_VERTICAL_SYNC);
		smi_mmiowl(0x007FF800, CRT_HWC_ADDRESS);
		smi_mmiowl(0x00010305, CRT_DISPLAY_CTRL);
		smi_mmiowl(0x00000001, MISC_CTRL);
	}

}

/*
 * Unmap in the memory mapped IO registers
 *
 */

static void __devinit smi_unmap_mmio(void)
{
	if (SMIRegs) {
		iounmap(SMIRegs);
		SMIRegs = NULL;
	}
}


/*
 * Unmap in the screen memory
 *
 */
static void __devinit smi_unmap_smem(void)
{
	if (SMILFB) {
		iounmap(SMILFB);
		SMILFB = NULL;
	}
}

void
vgxfb_setup (char *options)
{
    
	if (!options || !*options)
		return;

	/* The only thing I'm looking for right now is to disable a
	 * CRT output that mirrors the panel display.
	 */
	if (strcmp(options, "no_crt") == 0)
		crt_out = 0;

	return;
}

static struct fb_ops smifb_ops = {
	.owner =		THIS_MODULE,
	.fb_setcolreg =		smi_setcolreg,
	.fb_fillrect =		cfb_fillrect,
	.fb_copyarea =		cfb_copyarea,
	.fb_imageblit =		cfb_imageblit,
	.fb_cursor =		soft_cursor,
};

static int __devinit vgx_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int err;

	/* Enable the chip.
	*/
	err = pci_enable_device(dev);
	if (err)
		return err;


	/* Set up MMIO space.
	*/
	smifb_fix.mmio_start = pci_resource_start(dev,1);
	smifb_fix.mmio_len = 0x00200000;
	SMIRegs = ioremap(smifb_fix.mmio_start, smifb_fix.mmio_len);

	/* Set up framebuffer.  It's a 64M space, various amount of
	 * internal memory.  I don't know how to determine the real
	 * amount of memory (yet).
	 */
	smifb_fix.smem_start = pci_resource_start(dev,0);
	smifb_fix.smem_len = 0x00800000;
	SMILFB = ioremap(smifb_fix.smem_start, smifb_fix.smem_len);

	memset((void *)SMILFB, 0, smifb_fix.smem_len);

	info.screen_base = SMILFB;
	info.fbops = &smifb_ops;
	info.fix = smifb_fix;

	info.flags = FBINFO_FLAG_DEFAULT;

	info.pseudo_palette = kmalloc(sizeof(u32) * 16, GFP_KERNEL);
	if (!info.pseudo_palette) {
		return -ENOMEM;
	}
	memset((void *)info.pseudo_palette, 0, sizeof(u32) *16);

	fb_alloc_cmap(&info.cmap,256,0);

	smi_setmode();

	info.var = smifb_var;

	if (register_framebuffer(&info) < 0)
		goto failed;

	return 0;

failed:
	smi_unmap_smem();
	smi_unmap_mmio();

	return err;
}

static void __devexit vgx_pci_remove(struct pci_dev *dev)
{
	unregister_framebuffer(&info);
	smi_unmap_smem();
	smi_unmap_mmio();
}

/*
 * Rev. AA is 0x501, Rev. B is 0x510.
 */
static struct pci_device_id vgx_devices[] = {
	{0x126f, 0x510, PCI_ANY_ID, PCI_ANY_ID,0,0,0},
	{0x126f, 0x501, PCI_ANY_ID, PCI_ANY_ID,0,0,0},
	{0,}
};

MODULE_DEVICE_TABLE(pci, vgx_devices);

static struct pci_driver vgxfb_pci_driver = {
	.name	= "vgxfb",
	.id_table= vgx_devices,
	.probe	= vgx_pci_probe,
	.remove	= __devexit_p(vgx_pci_remove),
};

int __init vgxfb_init(void)
{
	char *option = NULL;

	if (fb_get_options("vgxfb", &option))
		return -ENODEV;
	vgxfb_setup(option);

	printk("Silicon Motion Inc. VOYAGER Init complete.\n");
	return pci_module_init(&vgxfb_pci_driver);
}

void __exit vgxfb_exit(void)
{
	pci_unregister_driver(&vgxfb_pci_driver);
}

module_init(vgxfb_init);
module_exit(vgxfb_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("Framebuffer driver for SMI Voyager");
MODULE_LICENSE("GPL");

