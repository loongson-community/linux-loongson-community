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

#include <linux/fb.h>
#include <linux/screen_info.h>
#include <linux/delay.h>

#include "sm712fb_drv.h"
#include "sm712fb_accel.h"

static inline u32 bytes_to_dword(const u8 *bytes, int length)
{
	u32 dword = 0;

	switch (length) {
	case 4:
#ifdef __BIG_ENDIAN
		dword += bytes[3];
#else
		dword += bytes[3] << 24;
#endif
	case 3:
#ifdef __BIG_ENDIAN
		dword += bytes[2] << 8;
#else
		dword += bytes[2] << 16;
#endif
	case 2:
#ifdef __BIG_ENDIAN
		dword += bytes[1] << 16;
#else
		dword += bytes[1] << 8;
#endif
	case 1:
#ifdef __BIG_ENDIAN
		dword += bytes[0] << 24;
#else
		dword += bytes[0];
#endif
	}
	return dword;
}

int sm712fb_init_accel(struct sm712fb_info *fb)
{
	u8 reg;

	/* reset the 2D engine */
	sm712_write_seq(fb, 0x21, sm712_read_seq(fb, 0x21) & 0xf8);
	reg = sm712_read_seq(fb, 0x15);
	sm712_write_seq(fb, 0x15, reg | 0x30);
	sm712_write_seq(fb, 0x15, reg);

	if (sm712fb_wait(fb) != 0)
		return -1;

	sm712_write_dpr(fb, DPR_CROP_TOPLEFT_COORDS, DPR_COORDS(0, 0));

	/* same width for DPR_PITCH and DPR_SRC_WINDOW */
	sm712_write_dpr(fb, DPR_PITCH,
			DPR_COORDS(fb->fb.var.xres, fb->fb.var.xres));
	sm712_write_dpr(fb, DPR_SRC_WINDOW,
			DPR_COORDS(fb->fb.var.xres, fb->fb.var.xres));

	sm712_write_dpr(fb, DPR_BYTE_BIT_MASK, 0xffffffff);
	sm712_write_dpr(fb, DPR_COLOR_COMPARE_MASK, 0);
	sm712_write_dpr(fb, DPR_COLOR_COMPARE, 0);
	sm712_write_dpr(fb, DPR_SRC_BASE, 0);
	sm712_write_dpr(fb, DPR_DST_BASE, 0);
	sm712_read_dpr(fb, DPR_DST_BASE);

	return 0;
}

int sm712fb_wait(struct sm712fb_info *fb)
{
	int i;
	u8 reg;

	for (i = 0; i < 10000; i++) {
		reg = sm712_read_seq(fb, SCR_DE_STATUS);
		if ((reg & SCR_DE_STATUS_MASK) == SCR_DE_ENGINE_IDLE)
			return 0;
		udelay(1);
	}
	return -EBUSY;
}

void sm712fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	u32 width = rect->width, height = rect->height;
	u32 dx = rect->dx, dy = rect->dy;
	u32 color;

	struct sm712fb_info *sfb = info->par;

	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;
	if ((rect->dx >= info->var.xres_virtual) ||
	    (rect->dy >= info->var.yres_virtual))
		return;

	if (info->fix.visual == FB_VISUAL_TRUECOLOR ||
	    info->fix.visual == FB_VISUAL_DIRECTCOLOR)
		color = ((u32 *) (info->pseudo_palette))[rect->color];
	else
		color = rect->color;

	sm712_write_dpr(sfb, DPR_FG_COLOR, color);
	sm712_write_dpr(sfb, DPR_DST_COORDS, DPR_COORDS(dx, dy));
	sm712_write_dpr(sfb, DPR_SPAN_COORDS, DPR_COORDS(width, height));
	sm712_write_dpr(sfb, DPR_DE_CTRL, DE_CTRL_START | DE_CTRL_ROP_ENABLE |
			(DE_CTRL_COMMAND_SOLIDFILL << DE_CTRL_COMMAND_SHIFT) |
			(DE_CTRL_ROP_SRC << DE_CTRL_ROP_SHIFT));
	sm712_read_dpr(sfb, DPR_DE_CTRL);
	sm712fb_wait(sfb);
}

void sm712fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	u32 sx = area->sx, sy = area->sy;
	u32 dx = area->dx, dy = area->dy;
	u32 height = area->height, width = area->width;
	u32 direction;

	struct sm712fb_info *sfb = info->par;

	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;
	if ((sx >= info->var.xres_virtual) || (sy >= info->var.yres_virtual))
		return;

	if (sy < dy || (sy == dy && sx <= dx)) {
		sx += width - 1;
		dx += width - 1;
		sy += height - 1;
		dy += height - 1;
		direction = DE_CTRL_RTOL;
	} else
		direction = 0;

	sm712_write_dpr(sfb, DPR_SRC_COORDS, DPR_COORDS(sx, sy));
	sm712_write_dpr(sfb, DPR_DST_COORDS, DPR_COORDS(dx, dy));
	sm712_write_dpr(sfb, DPR_SPAN_COORDS, DPR_COORDS(width, height));
	sm712_write_dpr(sfb, DPR_DE_CTRL,
			DE_CTRL_START | DE_CTRL_ROP_ENABLE | direction |
			(DE_CTRL_COMMAND_BITBLT << DE_CTRL_COMMAND_SHIFT) |
			(DE_CTRL_ROP_SRC << DE_CTRL_ROP_SHIFT));
	sm712_read_dpr(sfb, DPR_DE_CTRL);
	sm712fb_wait(sfb);
}

void sm712fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	u32 dx = image->dx, dy = image->dy;
	u32 width = image->width, height = image->height;
	u32 fg_color, bg_color;

	struct sm712fb_info *sfb = info->par;

	u32 imgidx = 0;
	u32 line = image->width >> 3;

	int i, j;
	u32 total_bytes, total_dwords, remain_bytes;

	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;
	if ((image->dx >= info->var.xres_virtual) ||
	    (image->dy >= info->var.yres_virtual))
		return;

	if (unlikely(image->depth != 1)) {
		/* unsupported depth, fallback to draw Tux */
		cfb_imageblit(info, image);
		return;
	}

	if (info->fix.visual == FB_VISUAL_TRUECOLOR ||
	    info->fix.visual == FB_VISUAL_DIRECTCOLOR) {
		fg_color = ((u32 *) (info->pseudo_palette))[image->fg_color];
		bg_color = ((u32 *) (info->pseudo_palette))[image->bg_color];
	} else {
		fg_color = image->fg_color;
		bg_color = image->bg_color;
	}

	/* total bytes we need to write */
	total_bytes = (width + 7) / 8;

	/* split the bytes into dwords and remainder bytes */
	total_dwords = (total_bytes & ~3) / 4;
	remain_bytes = total_bytes & 3;

	sm712_write_dpr(sfb, DPR_SRC_COORDS, 0);
	sm712_write_dpr(sfb, DPR_DST_COORDS, DPR_COORDS(dx, dy));
	sm712_write_dpr(sfb, DPR_SPAN_COORDS, DPR_COORDS(width, height));
	sm712_write_dpr(sfb, DPR_FG_COLOR, fg_color);
	sm712_write_dpr(sfb, DPR_BG_COLOR, bg_color);

	sm712_write_dpr(sfb, DPR_DE_CTRL, DE_CTRL_START | DE_CTRL_ROP_ENABLE |
			(DE_CTRL_COMMAND_HOST_WRITE << DE_CTRL_COMMAND_SHIFT) |
			(DE_CTRL_HOST_MONO << DE_CTRL_HOST_SHIFT) |
			(DE_CTRL_ROP_SRC << DE_CTRL_ROP_SHIFT));

	for (i = 0; i < height; i++) {
		/* cast bytes data into dwords and write to the dataport */
		for (j = 0; j < total_dwords; j++) {
			sm712_write_dataport(sfb,
					     bytes_to_dword(&image->
							    data[imgidx] +
							    j * 4, 4));
		}

		if (remain_bytes) {
			sm712_write_dataport(sfb,
					     bytes_to_dword(&image->
							    data[imgidx] +
							    (total_dwords * 4),
							    remain_bytes));
		}
		imgidx += line;
	}
	sm712_read_dpr(sfb, DPR_DE_CTRL);
	sm712fb_wait(sfb);
}
