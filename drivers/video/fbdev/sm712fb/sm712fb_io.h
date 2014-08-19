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
 */


#define sm712_writeb(base, reg, dat)	writeb(dat, base + reg)
#define sm712_writew(base, reg, dat)	writew(dat, base + reg)
#define sm712_writel(base, reg, dat)	writel(dat, base + reg)

#define sm712_readb(base, reg)	readb(base + reg)
#define sm712_readw(base, reg)	readw(base + reg)
#define sm712_readl(base, reg)	readl(base + reg)


static inline void sm712_write_crtc(struct sm712fb_info *fb, u8 reg, u8 val)
{
	sm712_writeb(fb->mmio, 0x3d4, reg);
	sm712_writeb(fb->mmio, 0x3d5, val);
}

static inline u8 sm712_read_crtc(struct sm712fb_info *fb, u8 reg)
{
	sm712_writeb(fb->mmio, 0x3d4, reg);
	return sm712_readb(fb->mmio, 0x3d5);
}

static inline void sm712_write_grph(struct sm712fb_info *fb, u8 reg, u8 val)
{
	sm712_writeb(fb->mmio, 0x3ce, reg);
	sm712_writeb(fb->mmio, 0x3cf, val);
}

static inline u8 sm712_read_grph(struct sm712fb_info *fb, u8 reg)
{
	sm712_writeb(fb->mmio, 0x3ce, reg);
	return sm712_readb(fb->mmio, 0x3cf);
}

static inline void sm712_write_attr(struct sm712fb_info *fb, u8 reg, u8 val)
{
	sm712_readb(fb->mmio, 0x3da);
	sm712_writeb(fb->mmio, 0x3c0, reg);
	sm712_readb(fb->mmio, 0x3c1);
	sm712_writeb(fb->mmio, 0x3c0, val);
}

static inline void sm712_write_seq(struct sm712fb_info *fb, u8 reg, u8 val)
{
	sm712_writeb(fb->mmio, 0x3c4, reg);
	sm712_writeb(fb->mmio, 0x3c5, val);
}

static inline u8 sm712_read_seq(struct sm712fb_info *fb, u8 reg)
{
	sm712_writeb(fb->mmio, 0x3c4, reg);
	return sm712_readb(fb->mmio, 0x3c5);
}

static inline u32 sm712_read_dpr(struct sm712fb_info *fb, u8 reg)
{
	return sm712_readl(fb->dpr, reg);
}

static inline void sm712_write_dpr(struct sm712fb_info *fb, u8 reg, u32 val)
{
	sm712_writel(fb->dpr, reg, val);
}

static inline void sm712_write_dataport(struct sm712fb_info *fb, u32 val)
{
	sm712_writel(fb->dataport, 0, val);
}
