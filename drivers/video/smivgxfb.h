/***************************************************************************
 * Silicon Motion VoyagerGX  framebuffer driver
 *
 * 	port to 2.6 by Embedded Alley Solutions, Inc
 * 	Copyright (C) 2005 Embedded Alley Solutions, Inc
 *
 * 		based on:
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

#define smi_mmiowb(dat,reg)	writeb(dat, (SMIRegs + reg))
#define smi_mmioww(dat,reg)	writew(dat, (SMIRegs + reg))
#define smi_mmiowl(dat,reg)	writel(dat, (SMIRegs + reg))

#define smi_mmiorb(reg)	        readb(SMIRegs + reg)
#define smi_mmiorw(reg)	        readw(SMIRegs + reg)
#define smi_mmiorl(reg)	        readl(SMIRegs + reg)

#define NR_PALETTE      256

/* Address space offsets for various control/status registers.
*/
#define MISC_CTRL			0x000004
#define GPIO_LO_CTRL			0x000008
#define GPIO_HI_CTRL			0x00000c
#define DRAM_CTRL			0x000010
#define CURRENT_POWER_GATE		0x000038
#define CURRENT_POWER_CLOCK		0x00003C
#define POWER_MODE1_GATE                0x000048
#define POWER_MODE1_CLOCK               0x00004C
#define POWER_MODE_CTRL			0x000054

#define GPIO_DATA_LO			0x010000
#define GPIO_DATA_HI			0x010004
#define GPIO_DATA_DIR_LO		0x010008
#define GPIO_DATA_DIR_HI		0x01000c
#define I2C_BYTE_COUNT			0x010040
#define I2C_CONTROL			0x010041
#define I2C_STATUS_RESET		0x010042
#define I2C_SLAVE_ADDRESS		0x010043
#define I2C_DATA			0x010044

#define DE_COLOR_COMPARE		0x100020
#define DE_COLOR_COMPARE_MASK		0x100024
#define DE_MASKS			0x100028
#define DE_WRAP				0x10004C

#define PANEL_DISPLAY_CTRL              0x080000
#define PANEL_PAN_CTRL                  0x080004
#define PANEL_COLOR_KEY                 0x080008
#define PANEL_FB_ADDRESS                0x08000C
#define PANEL_FB_WIDTH                  0x080010
#define PANEL_WINDOW_WIDTH              0x080014
#define PANEL_WINDOW_HEIGHT             0x080018
#define PANEL_PLANE_TL                  0x08001C
#define PANEL_PLANE_BR                  0x080020
#define PANEL_HORIZONTAL_TOTAL          0x080024
#define PANEL_HORIZONTAL_SYNC           0x080028
#define PANEL_VERTICAL_TOTAL            0x08002C
#define PANEL_VERTICAL_SYNC             0x080030
#define PANEL_CURRENT_LINE              0x080034
#define VIDEO_DISPLAY_CTRL		0x080040
#define VIDEO_DISPLAY_FB0		0x080044
#define VIDEO_DISPLAY_FBWIDTH		0x080048
#define VIDEO_DISPLAY_FB0LAST		0x08004C
#define VIDEO_DISPLAY_TL		0x080050
#define VIDEO_DISPLAY_BR		0x080054
#define VIDEO_SCALE			0x080058
#define VIDEO_INITIAL_SCALE		0x08005C
#define VIDEO_YUV_CONSTANTS		0x080060
#define VIDEO_DISPLAY_FB1		0x080064
#define VIDEO_DISPLAY_FB1LAST		0x080068
#define VIDEO_ALPHA_CTRL		0x080080
#define PANEL_HWC_ADDRESS		0x0800F0
#define CRT_DISPLAY_CTRL		0x080200
#define CRT_FB_ADDRESS			0x080204
#define CRT_FB_WIDTH			0x080208
#define CRT_HORIZONTAL_TOTAL		0x08020c
#define CRT_HORIZONTAL_SYNC		0x080210
#define CRT_VERTICAL_TOTAL		0x080214
#define CRT_VERTICAL_SYNC		0x080218
#define CRT_HWC_ADDRESS			0x080230
#define CRT_HWC_LOCATION		0x080234

#define ZV_CAPTURE_CTRL			0x090000
#define ZV_CAPTURE_CLIP			0x090004
#define ZV_CAPTURE_SIZE			0x090008
#define ZV_CAPTURE_BUF0			0x09000c
#define ZV_CAPTURE_BUF1			0x090010
#define ZV_CAPTURE_OFFSET		0x090014
#define ZV_FIFO_CTRL			0x090018

#define waitforvsync() udelay(100); udelay(100); udelay(100); udelay(100);

/*
 * Minimum X and Y resolutions
 */
#define MIN_XRES	640
#define MIN_YRES	480

/*
* Private structure
*/
struct smifb_par
{
	/*
	 * Hardware
	 */
	u16		chipID;

	u_int	width;
	u_int	height;
	u_int	hz;
};
