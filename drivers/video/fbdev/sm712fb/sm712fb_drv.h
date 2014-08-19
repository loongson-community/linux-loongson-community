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

#ifndef _SM712FB_DRV_H
#define _SM712FB_DRV_H

/*
* Private structure
*/
struct sm712fb_info {
	struct pci_dev *pdev;
	struct fb_info fb;
	u16 chip_id;
	u8  chip_rev_id;

	void __iomem *lfb;	/* linear frame buffer, the base address */

	void __iomem *dpr;	/* drawing processor control regs */
	void __iomem *vpr;	/* video processor control regs */
	void __iomem *cpr;	/* capture processor control regs */
	void __iomem *mmio;	/* memory map IO port */
	void __iomem *dataport; /* 2d drawing engine data port */

	u_int width;
	u_int height;
	u_int hz;

	u32 colreg[17];

	bool accel;
};

/* constants for registers operations */

#include "sm712fb_io.h"

#define FB_ACCEL_SMI_LYNX		88

#define SM712_DEFAULT_XRES		1024
#define SM712_DEFAULT_YRES		600
#define SM712_DEFAULT_BPP		16

#define SM712_VRAM_SIZE			0x00400000

#define	SM712_REG_BASE			0x00400000
#define SM712_REG_SIZE                  0x00400000

#define	SM712_MMIO_BASE			0x00700000

#define	SM712_DPR_BASE			0x00408000
#define SM712_DPR_SIZE                  (0x6C + 1)

#define	DPR_COORDS(x, y)		(((x) << 16) | (y))

#define	DPR_SRC_COORDS			0x00
#define	DPR_DST_COORDS			0x04
#define	DPR_SPAN_COORDS			0x08
#define	DPR_DE_CTRL			0x0c
#define	DPR_PITCH			0x10
#define	DPR_FG_COLOR			0x14
#define	DPR_BG_COLOR			0x18
#define	DPR_STRETCH			0x1c
#define	DPR_COLOR_COMPARE		0x20
#define	DPR_COLOR_COMPARE_MASK		0x24
#define	DPR_BYTE_BIT_MASK		0x28
#define	DPR_CROP_TOPLEFT_COORDS		0x2c
#define	DPR_CROP_BOTRIGHT_COORDS	0x30
#define	DPR_SRC_WINDOW			0x3c
#define	DPR_SRC_BASE			0x40
#define	DPR_DST_BASE			0x44

#define	DE_CTRL_START			0x80000000
#define	DE_CTRL_RTOL			0x08000000
#define	DE_CTRL_COMMAND_MASK		0x001f0000
#define	DE_CTRL_COMMAND_SHIFT			16
#define	DE_CTRL_COMMAND_BITBLT			0x00
#define	DE_CTRL_COMMAND_SOLIDFILL		0x01
#define DE_CTRL_COMMAND_HOST_WRITE              0x08
#define	DE_CTRL_ROP_ENABLE		0x00008000
#define	DE_CTRL_ROP_MASK		0x000000ff
#define	DE_CTRL_ROP_SHIFT			0
#define	DE_CTRL_ROP_SRC				0x0c

#define DE_CTRL_HOST_SHIFT              22
#define DE_CTRL_HOST_MONO               1

#define SCR_DE_STATUS			0x16
#define SCR_DE_STATUS_MASK		0x18
#define SCR_DE_ENGINE_IDLE		0x10

#define	SM712_VPR_BASE			0x0040c000
#define SM712_VPR_SIZE                  (0x44 + 1)

#define SM712_DATAPORT_BASE             0x00400000

#define SR00_SR04_SIZE      (0x04 - 0x00 + 1)
#define SR10_SR24_SIZE      (0x24 - 0x10 + 1)
#define SR30_SR75_SIZE      (0x75 - 0x30 + 1)
#define SR80_SR93_SIZE      (0x93 - 0x80 + 1)
#define SRA0_SRAF_SIZE      (0xAF - 0xA0 + 1)
#define GR00_GR08_SIZE      (0x08 - 0x00 + 1)
#define AR00_AR14_SIZE      (0x14 - 0x00 + 1)
#define CR00_CR18_SIZE      (0x18 - 0x00 + 1)
#define CR30_CR4D_SIZE      (0x4D - 0x30 + 1)
#define CR90_CRA7_SIZE      (0xA7 - 0x90 + 1)

#define DAC_REG	(0x3c8)
#define DAC_VAL	(0x3c9)

#endif
