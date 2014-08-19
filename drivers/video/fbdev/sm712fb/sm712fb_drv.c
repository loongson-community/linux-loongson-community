/*
 * Silicon Motion SM712 frame buffer device
 *
 * Copyright (C) 2006 Silicon Motion Technology Corp.
 * Authors:  Ge Wang, gewang@siliconmotion.com
 *	     Boyod boyod.yang@siliconmotion.com.cn
 *
 * Copyright (C) 2009 Lemote, Inc.
 * Author:   Wu Zhangjin, wuzhangjin@gmail.com
 *
 * Copyright (C) 2011 Igalia, S.L.
 * Author:   Javier M. Mellid <jmunhoz@igalia.com>
 *
 * Copyright (C) 2014 Tom Li.
 * Author:   Tom Li (Yifeng Li) <biergaizi@member.fsf.org>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 *
 * Framebuffer driver for Silicon Motion SM712 chip
 */

#include <linux/io.h>
#include <linux/fb.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/screen_info.h>

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

#include "sm712fb_drv.h"
#include "sm712fb_accel.h"
#include "sm712fb_modedb.h"

static struct fb_var_screeninfo sm712fb_var = {
	.xres = 1024,
	.yres = 600,
	.xres_virtual = 1024,
	.yres_virtual = 600,
	.bits_per_pixel = 16,
	.red = {16, 8, 0},
	.green = {8, 8, 0},
	.blue = {0, 8, 0},
	.activate = FB_ACTIVATE_NOW,
	.height = -1,
	.width = -1,
	.vmode = FB_VMODE_NONINTERLACED,
	.nonstd = 0,
	.accel_flags = FB_ACCELF_TEXT,
};

static struct fb_fix_screeninfo sm712fb_fix = {
	.id = "smXXXfb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.line_length = 800 * 3,
	.accel = FB_ACCEL_SMI_LYNX,
	.type_aux = 0,
	.xpanstep = 0,
	.ypanstep = 0,
	.ywrapstep = 0,
};

struct vesa_mode {
	char index[6];
	u16 lfb_width;
	u16 lfb_height;
	u16 lfb_depth;
};

static bool accel = 1;

static struct vesa_mode vesa_mode_table[] = {
	{"0x301", 640, 480, 8},
	{"0x303", 800, 600, 8},
	{"0x305", 1024, 768, 8},
	{"0x307", 1280, 1024, 8},

	{"0x311", 640, 480, 16},
	{"0x314", 800, 600, 16},
	{"0x317", 1024, 768, 16},
	{"0x31A", 1280, 1024, 16},

	{"0x312", 640, 480, 24},
	{"0x315", 800, 600, 24},
	{"0x318", 1024, 768, 24},
	{"0x31B", 1280, 1024, 24},
};

struct screen_info sm712_scr_info;

static int sm712fb_setup(char *options)
{
	char *this_opt;

	if (!options || !*options)
		return 0;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;

		if (!strcmp(this_opt, "accel:0"))
			accel = false;
		else if (!strcmp(this_opt, "accel:1"))
			accel = true;
	}
	return 0;
}

/* process command line options, get vga parameter */
static int __init sm712_vga_setup(char *options)
{
	int i;

	if (!options || !*options)
		return -EINVAL;

	sm712_scr_info.lfb_width = 0;
	sm712_scr_info.lfb_height = 0;
	sm712_scr_info.lfb_depth = 0;

	pr_debug("sm712_vga_setup = %s\n", options);

	for (i = 0; i < ARRAY_SIZE(vesa_mode_table); i++) {
		if (strstr(options, vesa_mode_table[i].index)) {
			sm712_scr_info.lfb_width = vesa_mode_table[i].lfb_width;
			sm712_scr_info.lfb_height =
			    vesa_mode_table[i].lfb_height;
			sm712_scr_info.lfb_depth = vesa_mode_table[i].lfb_depth;
			return 0;
		}
	}

	return -1;
}

__setup("vga=", sm712_vga_setup);

static void sm712_setpalette(int regno, unsigned red, unsigned green,
			     unsigned blue, struct fb_info *info)
{
	struct sm712fb_info *sfb = info->par;

	/* set bit 5:4 = 01 (write LCD RAM only) */
	sm712_write_seq(sfb, 0x66, (sm712_read_seq(sfb, 0x66) & 0xC3) | 0x10);

	sm712_writeb(sfb->mmio, DAC_REG, regno);
	sm712_writeb(sfb->mmio, DAC_VAL, red >> 10);
	sm712_writeb(sfb->mmio, DAC_VAL, green >> 10);
	sm712_writeb(sfb->mmio, DAC_VAL, blue >> 10);
}

/* chan_to_field
 *
 * convert a colour value into a field position
 *
 * from pxafb.c
 */

static inline unsigned int chan_to_field(unsigned int chan,
					 struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int sm712_blank(int blank_mode, struct fb_info *info)
{
	struct sm712fb_info *sfb = info->par;

	/* clear DPMS setting */
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		/* Screen On: HSync: On, VSync : On */
		sm712_write_seq(sfb, 0x01,
				(sm712_read_seq(sfb, 0x01) & (~0x20)));
		sm712_write_seq(sfb, 0x6a, 0x16);
		sm712_write_seq(sfb, 0x6b, 0x02);
		sm712_write_seq(sfb, 0x21, (sm712_read_seq(sfb, 0x21) & 0x77));
		sm712_write_seq(sfb, 0x22,
				(sm712_read_seq(sfb, 0x22) & (~0x30)));
		sm712_write_seq(sfb, 0x23,
				(sm712_read_seq(sfb, 0x23) & (~0xc0)));
		sm712_write_seq(sfb, 0x24, (sm712_read_seq(sfb, 0x24) | 0x01));
		sm712_write_seq(sfb, 0x31, (sm712_read_seq(sfb, 0x31) | 0x03));
		break;
	case FB_BLANK_NORMAL:
		/* Screen Off: HSync: On, VSync : On   Soft blank */
		sm712_write_seq(sfb, 0x01,
				(sm712_read_seq(sfb, 0x01) & (~0x20)));
		sm712_write_seq(sfb, 0x6a, 0x16);
		sm712_write_seq(sfb, 0x6b, 0x02);
		sm712_write_seq(sfb, 0x22,
				(sm712_read_seq(sfb, 0x22) & (~0x30)));
		sm712_write_seq(sfb, 0x23,
				(sm712_read_seq(sfb, 0x23) & (~0xc0)));
		sm712_write_seq(sfb, 0x24, (sm712_read_seq(sfb, 0x24) | 0x01));
		sm712_write_seq(sfb, 0x31,
				((sm712_read_seq(sfb, 0x31) & (~0x07)) | 0x00));
		break;
	case FB_BLANK_VSYNC_SUSPEND:
		/* Screen On: HSync: On, VSync : Off */
		sm712_write_seq(sfb, 0x01, (sm712_read_seq(sfb, 0x01) | 0x20));
		sm712_write_seq(sfb, 0x20,
				(sm712_read_seq(sfb, 0x20) & (~0xB0)));
		sm712_write_seq(sfb, 0x6a, 0x0c);
		sm712_write_seq(sfb, 0x6b, 0x02);
		sm712_write_seq(sfb, 0x21, (sm712_read_seq(sfb, 0x21) | 0x88));
		sm712_write_seq(sfb, 0x22,
				((sm712_read_seq(sfb, 0x22) & (~0x30)) | 0x20));
		sm712_write_seq(sfb, 0x23,
				((sm712_read_seq(sfb, 0x23) & (~0xc0)) | 0x20));
		sm712_write_seq(sfb, 0x24,
				(sm712_read_seq(sfb, 0x24) & (~0x01)));
		sm712_write_seq(sfb, 0x31,
				((sm712_read_seq(sfb, 0x31) & (~0x07)) | 0x00));
		sm712_write_seq(sfb, 0x34, (sm712_read_seq(sfb, 0x34) | 0x80));
		break;
	case FB_BLANK_HSYNC_SUSPEND:
		/* Screen On: HSync: Off, VSync : On */
		sm712_write_seq(sfb, 0x01, (sm712_read_seq(sfb, 0x01) | 0x20));
		sm712_write_seq(sfb, 0x20,
				(sm712_read_seq(sfb, 0x20) & (~0xB0)));
		sm712_write_seq(sfb, 0x6a, 0x0c);
		sm712_write_seq(sfb, 0x6b, 0x02);
		sm712_write_seq(sfb, 0x21, (sm712_read_seq(sfb, 0x21) | 0x88));
		sm712_write_seq(sfb, 0x22,
				((sm712_read_seq(sfb, 0x22) & (~0x30)) | 0x10));
		sm712_write_seq(sfb, 0x23,
				((sm712_read_seq(sfb, 0x23) & (~0xc0)) | 0xD8));
		sm712_write_seq(sfb, 0x24,
				(sm712_read_seq(sfb, 0x24) & (~0x01)));
		sm712_write_seq(sfb, 0x31,
				((sm712_read_seq(sfb, 0x31) & (~0x07)) | 0x00));
		sm712_write_seq(sfb, 0x34, (sm712_read_seq(sfb, 0x34) | 0x80));
		break;
	case FB_BLANK_POWERDOWN:
		/* Screen On: HSync: Off, VSync : Off */
		sm712_write_seq(sfb, 0x01, (sm712_read_seq(sfb, 0x01) | 0x20));
		sm712_write_seq(sfb, 0x20,
				(sm712_read_seq(sfb, 0x20) & (~0xB0)));
		sm712_write_seq(sfb, 0x6a, 0x5a);
		sm712_write_seq(sfb, 0x6b, 0x20);
		sm712_write_seq(sfb, 0x21, (sm712_read_seq(sfb, 0x21) | 0x88));
		sm712_write_seq(sfb, 0x22,
				((sm712_read_seq(sfb, 0x22) & (~0x30)) | 0x30));
		sm712_write_seq(sfb, 0x23,
				((sm712_read_seq(sfb, 0x23) & (~0xc0)) | 0xD8));
		sm712_write_seq(sfb, 0x24,
				(sm712_read_seq(sfb, 0x24) & (~0x01)));
		sm712_write_seq(sfb, 0x31,
				((sm712_read_seq(sfb, 0x31) & (~0x07)) | 0x00));
		sm712_write_seq(sfb, 0x34, (sm712_read_seq(sfb, 0x34) | 0x80));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sm712_setcolreg(unsigned regno, unsigned red, unsigned green,
			   unsigned blue, unsigned trans, struct fb_info *info)
{
	struct sm712fb_info *sfb;
	u32 val;

	sfb = info->par;

	if (regno > 255)
		return 1;

	switch (sfb->fb.fix.visual) {
	case FB_VISUAL_DIRECTCOLOR:
	case FB_VISUAL_TRUECOLOR:
		/*
		 * 16/32 bit true-colour, use pseudo-palette for 16 base color
		 */
		if (regno < 16) {
			if (sfb->fb.var.bits_per_pixel == 16) {
				u32 *pal = sfb->fb.pseudo_palette;

				val = chan_to_field(red, &sfb->fb.var.red);
				val |= chan_to_field(green, &sfb->fb.var.green);
				val |= chan_to_field(blue, &sfb->fb.var.blue);
#ifdef __BIG_ENDIAN
				pal[regno] =
				    ((red & 0xf800) >> 8) |
				    ((green & 0xe000) >> 13) |
				    ((green & 0x1c00) << 3) |
				    ((blue & 0xf800) >> 3);
#else
				pal[regno] = val;
#endif
			} else {
				u32 *pal = sfb->fb.pseudo_palette;

				val = chan_to_field(red, &sfb->fb.var.red);
				val |= chan_to_field(green, &sfb->fb.var.green);
				val |= chan_to_field(blue, &sfb->fb.var.blue);
#ifdef __BIG_ENDIAN
				val =
				    (val & 0xff00ff00 >> 8) |
				    (val & 0x00ff00ff << 8);
#endif
				pal[regno] = val;
			}
		}
		break;

	case FB_VISUAL_PSEUDOCOLOR:
		/* color depth 8 bit */
		sm712_setpalette(regno, red, green, blue, info);
		break;

	default:
		return 1;	/* unknown type */
	}

	return 0;

}

#ifdef __BIG_ENDIAN
static ssize_t sm712fb_read(struct fb_info *info, char __user *buf,
			    size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;

	u32 *buffer, *dst;
	u32 __iomem *src;
	int c, i, cnt = 0, err = 0;
	unsigned long total_size;

	if (!info || !info->screen_base)
		return -ENODEV;

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	total_size = info->screen_size;

	if (total_size == 0)
		total_size = info->fix.smem_len;

	if (p >= total_size)
		return 0;

	if (count >= total_size)
		count = total_size;

	if (count + p > total_size)
		count = total_size - p;

	buffer = kmalloc((count > PAGE_SIZE) ? PAGE_SIZE : count, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	src = (u32 __iomem *) (info->screen_base + p);

	if (info->fbops->fb_sync)
		info->fbops->fb_sync(info);

	while (count) {
		c = (count > PAGE_SIZE) ? PAGE_SIZE : count;
		dst = buffer;
		for (i = c >> 2; i--;) {
			*dst = fb_readl(src++);
			*dst =
			    (*dst & 0xff00ff00 >> 8) | (*dst & 0x00ff00ff << 8);
			dst++;
		}
		if (c & 3) {
			u8 *dst8 = (u8 *) dst;
			u8 __iomem *src8 = (u8 __iomem *) src;

			for (i = c & 3; i--;) {
				if (i & 1) {
					*dst8++ = fb_readb(++src8);
				} else {
					*dst8++ = fb_readb(--src8);
					src8 += 2;
				}
			}
			src = (u32 __iomem *) src8;
		}

		if (copy_to_user(buf, buffer, c)) {
			err = -EFAULT;
			break;
		}
		*ppos += c;
		buf += c;
		cnt += c;
		count -= c;
	}

	kfree(buffer);

	return (err) ? err : cnt;
}

static ssize_t
sm712fb_write(struct fb_info *info, const char __user *buf, size_t count,
	      loff_t *ppos)
{
	unsigned long p = *ppos;

	u32 *buffer, *src;
	u32 __iomem *dst;
	int c, i, cnt = 0, err = 0;
	unsigned long total_size;

	if (!info || !info->screen_base)
		return -ENODEV;

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	total_size = info->screen_size;

	if (total_size == 0)
		total_size = info->fix.smem_len;

	if (p > total_size)
		return -EFBIG;

	if (count > total_size) {
		err = -EFBIG;
		count = total_size;
	}

	if (count + p > total_size) {
		if (!err)
			err = -ENOSPC;

		count = total_size - p;
	}

	buffer = kmalloc((count > PAGE_SIZE) ? PAGE_SIZE : count, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	dst = (u32 __iomem *) (info->screen_base + p);

	if (info->fbops->fb_sync)
		info->fbops->fb_sync(info);

	while (count) {
		c = (count > PAGE_SIZE) ? PAGE_SIZE : count;
		src = buffer;

		if (copy_from_user(src, buf, c)) {
			err = -EFAULT;
			break;
		}

		for (i = c >> 2; i--;) {
			fb_writel((*src & 0xff00ff00 >> 8) |
				  (*src & 0x00ff00ff << 8), dst++);
			src++;
		}
		if (c & 3) {
			u8 *src8 = (u8 *) src;
			u8 __iomem *dst8 = (u8 __iomem *) dst;

			for (i = c & 3; i--;) {
				if (i & 1) {
					fb_writeb(*src8++, ++dst8);
				} else {
					fb_writeb(*src8++, --dst8);
					dst8 += 2;
				}
			}
			dst = (u32 __iomem *) dst8;
		}

		*ppos += c;
		buf += c;
		cnt += c;
		count -= c;
	}

	kfree(buffer);

	return (cnt) ? cnt : err;
}
#endif /* ! __BIG_ENDIAN */

static void sm712_set_timing(struct sm712fb_info *sfb)
{
	int i = 0, j = 0;
	u32 m_nScreenStride;

	dev_dbg(&sfb->pdev->dev,
		"sfb->width=%d sfb->height=%d "
		"sfb->fb.var.bits_per_pixel=%d sfb->hz=%d\n",
		sfb->width, sfb->height, sfb->fb.var.bits_per_pixel, sfb->hz);

	for (j = 0; j < numVGAModes; j++) {
		if (VGAMode[j].mmSizeX != sfb->width ||
		    VGAMode[j].mmSizeY != sfb->height ||
		    VGAMode[j].bpp != sfb->fb.var.bits_per_pixel ||
		    VGAMode[j].hz != sfb->hz) {
			continue;
		}

		dev_dbg(&sfb->pdev->dev,
			"VGAMode[j].mmSizeX=%d VGAMode[j].mmSizeY=%d "
			"VGAMode[j].bpp=%d VGAMode[j].hz=%d\n",
			VGAMode[j].mmSizeX, VGAMode[j].mmSizeY,
			VGAMode[j].bpp, VGAMode[j].hz);

		dev_dbg(&sfb->pdev->dev, "VGAMode index=%d\n", j);

		sm712_writeb(sfb->mmio, 0x3c6, 0x0);

		sm712_write_seq(sfb, 0, 0x1);

		sm712_writeb(sfb->mmio, 0x3c2, VGAMode[j].Init_MISC);

		/* init SEQ register SR00 - SR04 */
		for (i = 0; i < SR00_SR04_SIZE; i++)
			sm712_write_seq(sfb, i, VGAMode[j].Init_SR00_SR04[i]);

		/* init SEQ register SR10 - SR24 */
		for (i = 0; i < SR10_SR24_SIZE; i++)
			sm712_write_seq(sfb, i + 0x10,
					VGAMode[j].Init_SR10_SR24[i]);

		/* init SEQ register SR30 - SR75 */
		for (i = 0; i < SR30_SR75_SIZE; i++)
			if ((i + 0x30) != 0x62 &&
			    (i + 0x30) != 0x6a && (i + 0x30) != 0x6b)
				sm712_write_seq(sfb, i + 0x30,
						VGAMode[j].Init_SR30_SR75[i]);

		/* init SEQ register SR80 - SR93 */
		for (i = 0; i < SR80_SR93_SIZE; i++)
			sm712_write_seq(sfb, i + 0x80,
					VGAMode[j].Init_SR80_SR93[i]);

		/* init SEQ register SRA0 - SRAF */
		for (i = 0; i < SRA0_SRAF_SIZE; i++)
			sm712_write_seq(sfb, i + 0xa0,
					VGAMode[j].Init_SRA0_SRAF[i]);

		/* init Graphic register GR00 - GR08 */
		for (i = 0; i < GR00_GR08_SIZE; i++)
			sm712_write_grph(sfb, i, VGAMode[j].Init_GR00_GR08[i]);

		/* init Attribute register AR00 - AR14 */
		for (i = 0; i < AR00_AR14_SIZE; i++)
			sm712_write_attr(sfb, i, VGAMode[j].Init_AR00_AR14[i]);

		/* init CRTC register CR00 - CR18 */
		for (i = 0; i < CR00_CR18_SIZE; i++)
			sm712_write_crtc(sfb, i, VGAMode[j].Init_CR00_CR18[i]);

		/* init CRTC register CR30 - CR4D */
		for (i = 0; i < CR30_CR4D_SIZE; i++)
			sm712_write_crtc(sfb, i + 0x30,
					 VGAMode[j].Init_CR30_CR4D[i]);

		/* init CRTC register CR90 - CRA7 */
		for (i = 0; i < CR90_CRA7_SIZE; i++)
			sm712_write_crtc(sfb, i + 0x90,
					 VGAMode[j].Init_CR90_CRA7[i]);
	}
	sm712_writeb(sfb->mmio, 0x3c2, 0x67);

	/* set VPR registers */
	sm712_writel(sfb->vpr, 0x0C, 0x0);
	sm712_writel(sfb->vpr, 0x40, 0x0);

	/* set data width */
	m_nScreenStride = (sfb->width * sfb->fb.var.bits_per_pixel) / 64;
	switch (sfb->fb.var.bits_per_pixel) {
	case 8:
		sm712_writel(sfb->vpr, 0x0, 0x0);
		break;
	case 16:
		sm712_writel(sfb->vpr, 0x0, 0x00020000);
		break;
	case 24:
		sm712_writel(sfb->vpr, 0x0, 0x00040000);
		break;
	case 32:
		sm712_writel(sfb->vpr, 0x0, 0x00030000);
		break;
	}
	sm712_writel(sfb->vpr, 0x10,
		     (u32) (((m_nScreenStride + 2) << 16) | m_nScreenStride));
}

static void sm712fb_setmode(struct sm712fb_info *sfb)
{
	switch (sfb->fb.var.bits_per_pixel) {
	case 32:
		sfb->fb.fix.visual = FB_VISUAL_TRUECOLOR;
		sfb->fb.fix.line_length = sfb->fb.var.xres * 4;
		sfb->fb.var.red.length = 8;
		sfb->fb.var.green.length = 8;
		sfb->fb.var.blue.length = 8;
		sfb->fb.var.red.offset = 16;
		sfb->fb.var.green.offset = 8;
		sfb->fb.var.blue.offset = 0;
		break;
	case 24:
		sfb->fb.fix.visual = FB_VISUAL_TRUECOLOR;
		sfb->fb.fix.line_length = sfb->fb.var.xres * 3;
		sfb->fb.var.red.length = 8;
		sfb->fb.var.green.length = 8;
		sfb->fb.var.blue.length = 8;
		sfb->fb.var.red.offset = 16;
		sfb->fb.var.green.offset = 8;
		sfb->fb.var.blue.offset = 0;
		break;
	case 8:
		sfb->fb.fix.visual = FB_VISUAL_PSEUDOCOLOR;
		sfb->fb.fix.line_length = sfb->fb.var.xres;
		sfb->fb.var.red.length = 3;
		sfb->fb.var.green.length = 3;
		sfb->fb.var.blue.length = 2;
		sfb->fb.var.red.offset = 5;
		sfb->fb.var.green.offset = 2;
		sfb->fb.var.blue.offset = 0;
		break;
	case 16:
	default:
		sfb->fb.fix.visual = FB_VISUAL_TRUECOLOR;
		sfb->fb.fix.line_length = sfb->fb.var.xres * 2;
		sfb->fb.var.red.length = 5;
		sfb->fb.var.green.length = 6;
		sfb->fb.var.blue.length = 5;
		sfb->fb.var.red.offset = 11;
		sfb->fb.var.green.offset = 5;
		sfb->fb.var.blue.offset = 0;
		break;
	}

	sfb->width = sfb->fb.var.xres;
	sfb->height = sfb->fb.var.yres;
	sfb->hz = 60;
	sm712_set_timing(sfb);
}

static int sm712_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	/* sanity checks */
	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;

	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

	/* set valid default bpp */
	if ((var->bits_per_pixel != 8) && (var->bits_per_pixel != 16) &&
	    (var->bits_per_pixel != 24) && (var->bits_per_pixel != 32))
		var->bits_per_pixel = 16;

	return 0;
}

static int sm712_set_par(struct fb_info *info)
{
	sm712fb_setmode(info->par);

	return 0;
}

static struct fb_ops sm712fb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = sm712_check_var,
	.fb_set_par = sm712_set_par,
	.fb_setcolreg = sm712_setcolreg,
	.fb_blank = sm712_blank,
	.fb_fillrect = cfb_fillrect,
	.fb_imageblit = cfb_imageblit,
	.fb_copyarea = cfb_copyarea,
#ifdef __BIG_ENDIAN
	.fb_read = sm712fb_read,
	.fb_write = sm712fb_write,
#endif
};

/*
 * alloc struct sm712fb_info and assign default values
 */
static struct sm712fb_info *sm712_fb_info_new(struct pci_dev *pdev)
{
	struct sm712fb_info *sfb;

	sfb = kzalloc(sizeof(*sfb), GFP_KERNEL);

	if (!sfb)
		return NULL;

	sfb->pdev = pdev;

	sfb->fb.flags = FBINFO_FLAG_DEFAULT;
	sfb->fb.fbops = &sm712fb_ops;
	sfb->fb.fix = sm712fb_fix;
	sfb->fb.var = sm712fb_var;
	sfb->fb.pseudo_palette = sfb->colreg;
	sfb->fb.par = sfb;
	sfb->accel = accel;

	return sfb;
}

/*
 * free struct sm712fb_info
 */
static void sm712_fb_info_free(struct sm712fb_info *sfb)
{
	kfree(sfb);
}

/*
 * Map in the screen memory
 */

static int sm712_map_smem(struct sm712fb_info *sfb,
			  struct pci_dev *pdev, u_long smem_len)
{

	sfb->fb.fix.smem_start = pci_resource_start(pdev, 0);

#ifdef __BIG_ENDIAN
	if (sfb->fb.var.bits_per_pixel == 32)
		sfb->fb.fix.smem_start += 0x800000;
#endif

	sfb->fb.fix.smem_len = smem_len;

	sfb->fb.screen_base = sfb->lfb;

	if (!sfb->fb.screen_base) {
		dev_err(&pdev->dev,
			"%s: unable to map screen memory\n", sfb->fb.fix.id);
		return -ENOMEM;
	}

	return 0;
}

/*
 * Unmap in the screen memory
 *
 */
static void sm712_unmap_smem(struct sm712fb_info *sfb)
{
	if (sfb && sfb->fb.screen_base) {
		iounmap(sfb->fb.screen_base);
		sfb->fb.screen_base = NULL;
		sfb->lfb = NULL;
	}
}

static inline void sm712_init_hw(struct sm712fb_info *sfb)
{
	/* enable linear memory mode and packed pixel format */
	outb_p(0x18, 0x3c4);
	outb_p(0x11, 0x3c5);

	/* set MCLK = 14.31818 *  (0x16 / 0x2) */
	sm712_write_seq(sfb, 0x6a, 0x16);
	sm712_write_seq(sfb, 0x6b, 0x02);
	sm712_write_seq(sfb, 0x62, 0x3e);

	/* enable PCI burst */
	sm712_write_seq(sfb, 0x17, 0x20);

#ifdef __BIG_ENDIAN
	/* enable word swap */
	if (sfb->fb.var.bits_per_pixel == 32)
		sm712_write_seq(sfb, 0x17, 0x30);
#endif

	if (!sfb->accel) {
		dev_info(&sfb->pdev->dev, "2d acceleration was disabled by user.\n");
		sfb->fb.flags = FBINFO_FLAG_DEFAULT | FBINFO_HWACCEL_NONE;
		return;
	}

	if (sm712fb_init_accel(sfb) < 0) {
		dev_info(&sfb->pdev->dev, "failed to enable 2d accleration.\n");
		sfb->fb.flags = FBINFO_FLAG_DEFAULT | FBINFO_HWACCEL_NONE;
		return;
	} else {
		sm712fb_ops.fb_fillrect = sm712fb_fillrect;
		sm712fb_ops.fb_copyarea = sm712fb_copyarea;
		sm712fb_ops.fb_imageblit = sm712fb_imageblit;
		sfb->fb.flags |= FBINFO_HWACCEL_COPYAREA |
				 FBINFO_HWACCEL_FILLRECT |
				 FBINFO_HWACCEL_IMAGEBLIT |
				 FBINFO_READS_FAST;
		dev_info(&sfb->pdev->dev, "sm712fb: enable 2d acceleration.\n");
	}
}

static int sm712fb_pci_probe(struct pci_dev *pdev,
			     const struct pci_device_id *ent)
{
	struct sm712fb_info *sfb;
	int err;
	unsigned long mmio_base;

#ifndef MODULE
	char *option = NULL;

	if (!fb_get_options("sm712fb", &option))
		sm712fb_setup(option);
#endif

	dev_info(&pdev->dev, "Silicon Motion display driver.");

	err = pci_enable_device(pdev);	/* enable SMTC chip */
	if (err)
		return err;

	sprintf(sm712fb_fix.id, "sm712fb");

	sfb = sm712_fb_info_new(pdev);

	if (!sfb) {
		err = -ENOMEM;
		goto free_fail;
	}

	sfb->chip_id = ent->device;

	pci_set_drvdata(pdev, sfb);

	/* get mode parameter from sm712_scr_info */
	if (sm712_scr_info.lfb_width != 0) {
		sfb->fb.var.xres = sm712_scr_info.lfb_width;
		sfb->fb.var.yres = sm712_scr_info.lfb_height;
		sfb->fb.var.bits_per_pixel = sm712_scr_info.lfb_depth;
	} else {
		/* default resolution 1024x600 16bit mode */
		sfb->fb.var.xres = SM712_DEFAULT_XRES;
		sfb->fb.var.yres = SM712_DEFAULT_YRES;
		sfb->fb.var.bits_per_pixel = SM712_DEFAULT_BPP;
	}

#ifdef __BIG_ENDIAN
	if (sfb->fb.var.bits_per_pixel == 24)
		sfb->fb.var.bits_per_pixel = (sm712_scr_info.lfb_depth = 32);
#endif

	/* Map address and memory detection */
	mmio_base = pci_resource_start(pdev, 0);
	pci_read_config_byte(pdev, PCI_REVISION_ID, &sfb->chip_rev_id);

	if (sfb->chip_id != 0x712) {
		dev_err(&pdev->dev,
			"No valid Silicon Motion display chip was detected!");

		goto fb_fail;
	}

	sfb->fb.fix.mmio_start = mmio_base + SM712_REG_BASE;
	sfb->fb.fix.mmio_len = SM712_REG_SIZE;
#ifdef __BIG_ENDIAN
	sfb->lfb = ioremap(mmio_base, 0x00c00000);
#else
	sfb->lfb = ioremap(mmio_base, 0x00800000);
#endif
	sfb->mmio = sfb->lfb + SM712_MMIO_BASE;
	sfb->dpr = sfb->lfb + SM712_DPR_BASE;
	sfb->vpr = sfb->lfb + SM712_VPR_BASE;
	sfb->dataport = sfb->lfb + SM712_DATAPORT_BASE;
#ifdef __BIG_ENDIAN
	if (sfb->fb.var.bits_per_pixel == 32) {
		sfb->lfb += 0x800000;
		dev_info(&pdev->dev, "sfb->lfb=%p", sfb->lfb);
	}
#endif
	if (!sfb->mmio) {
		dev_err(&pdev->dev,
			"%s: unable to map memory mapped IO!", sfb->fb.fix.id);
		err = -ENOMEM;
		goto fb_fail;
	}

	sm712_init_hw(sfb);

	/* can support 32 bpp */
	if (15 == sfb->fb.var.bits_per_pixel)
		sfb->fb.var.bits_per_pixel = 16;

	sfb->fb.var.xres_virtual = sfb->fb.var.xres;
	sfb->fb.var.yres_virtual = sfb->fb.var.yres;
	err = sm712_map_smem(sfb, pdev, SM712_VRAM_SIZE);
	if (err)
		goto fail;

	sm712fb_setmode(sfb);

	err = register_framebuffer(&sfb->fb);
	if (err < 0)
		goto fail;

	dev_info(&pdev->dev,
		 "Silicon Motion SM%X Rev%X primary display mode %dx%d-%d Init Complete.",
		 sfb->chip_id, sfb->chip_rev_id, sfb->fb.var.xres,
		 sfb->fb.var.yres, sfb->fb.var.bits_per_pixel);

	return 0;

fail:
	dev_err(&pdev->dev, "Silicon Motion, Inc. primary display init fail.");

	sm712_unmap_smem(sfb);
fb_fail:
	sm712_fb_info_free(sfb);
free_fail:
	pci_disable_device(pdev);

	return err;
}

/*
 * 0x712 (LynxEM+)
 */
static const struct pci_device_id sm712fb_pci_table[] = {
	{PCI_DEVICE(0x126f, 0x712),},
	{0,}
};

static void sm712fb_pci_remove(struct pci_dev *pdev)
{
	struct sm712fb_info *sfb;

	sfb = pci_get_drvdata(pdev);
	sm712_unmap_smem(sfb);
	unregister_framebuffer(&sfb->fb);
	sm712_fb_info_free(sfb);
}

#ifdef CONFIG_PM
static int sm712fb_pci_suspend(struct device *device)
{
	struct pci_dev *pdev = to_pci_dev(device);
	struct sm712fb_info *sfb;

	sfb = pci_get_drvdata(pdev);

	/* set the hw in sleep mode use external clock and self memory refresh
	 * so that we can turn off internal PLLs later on
	 */
	sm712_write_seq(sfb, 0x20, (sm712_read_seq(sfb, 0x20) | 0xc0));
	sm712_write_seq(sfb, 0x69, (sm712_read_seq(sfb, 0x69) & 0xf7));

	console_lock();
	fb_set_suspend(&sfb->fb, 1);
	console_unlock();

	/* additionally turn off all function blocks including internal PLLs */
	sm712_write_seq(sfb, 0x21, 0xff);

	return 0;
}

static int sm712fb_pci_resume(struct device *device)
{
	struct pci_dev *pdev = to_pci_dev(device);
	struct sm712fb_info *sfb;

	sfb = pci_get_drvdata(pdev);

	/* reinit hardware */
	sm712_init_hw(sfb);

	sm712_write_seq(sfb, 0x34, (sm712_read_seq(sfb, 0x34) | 0xc0));
	sm712_write_seq(sfb, 0x33, ((sm712_read_seq(sfb, 0x33) | 0x08) & 0xfb));

	sm712fb_setmode(sfb);

	console_lock();
	fb_set_suspend(&sfb->fb, 0);
	console_unlock();

	return 0;
}

static SIMPLE_DEV_PM_OPS(sm712_pm_ops, sm712fb_pci_suspend, sm712fb_pci_resume);
#define SM712_PM_OPS (&sm712_pm_ops)

#else /* !CONFIG_PM */

#define SM712_PM_OPS NULL

#endif /* !CONFIG_PM */

static struct pci_driver sm712fb_driver = {
	.name = "sm712fb",
	.id_table = sm712fb_pci_table,
	.probe = sm712fb_pci_probe,
	.remove = sm712fb_pci_remove,
	.driver.pm = SM712_PM_OPS,
};

module_pci_driver(sm712fb_driver);

module_param(accel, bool, S_IRUGO);
MODULE_PARM_DESC(accel, "Enable or disable 2D Acceleration");

MODULE_AUTHOR("Siliconmotion ");
MODULE_DESCRIPTION("Framebuffer driver for Silicon Motion SM712 Graphic Cards");
MODULE_LICENSE("GPL");
