/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */

/*
 * klhwinit_gfx.h -  Graphics defines used during IO6prom init.
 *			Some of the stuff in here may later get put into
 *	 		or be included from other files.
 */
#ifndef _ASM_SN_SN1_KLHWINIT_GFX_H
#define _ASM_SN_SN1_KLHWINIT_GFX_H

#include <asm/xtalk/xg.h>				/* gets  XG PART_NUM, NIC */
#include <asm/xtalk/hq4.h>				/* gets HQ4 PART_NUM, NIC */

#define KONA_WIDGET_PART_NUM 	 XG_WIDGET_PART_NUM     /*  xtalk/xg.h   0xC102 */
#define MGRAS_WIDGET_PART_NUM	 HQ4_WIDGET_PART_NUM 	/* xtalk/hq4.h   0xC003 */

#define HILO_GE16_2_PART_NUM     "030-1397-"
#define HILO_GE16_4_PART_NUM     "030-1398-"
#define HILO_GE16_8_PART_NUM     "030-1399-"
  
#define HILO_GE14_8_PART_NUM     "030-1052-"
#define HILO_GE14_4_PART_NUM     "030-1129-"
#define HILO_GE14_2_PART_NUM     "030-1051-"

#define MGRAS_GM10_PART_NUM       "030-0938-"  	
#define MGRAS_GM20_PART_NUM       "030-0957-"  


#endif /* _ASM_SN_SN1_KLHWINIT_GFX_H */
