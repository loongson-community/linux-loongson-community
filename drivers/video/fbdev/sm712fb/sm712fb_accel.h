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

#ifndef _SM712FB_ACCEL_H
#define _SM712FB_ACCEL_H

int sm712fb_init_accel(struct sm712fb_info *fb);
int sm712fb_wait(struct sm712fb_info *fb);
void sm712fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect);
void sm712fb_copyarea(struct fb_info *info, const struct fb_copyarea *area);
void sm712fb_imageblit(struct fb_info *info, const struct fb_image *image);

#endif
