/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */
#ifndef _ASM_SN_SN1_BDRKHSPECREGS_NEXT_H
#define _ASM_SN_SN1_BDRKHSPECREGS_NEXT_H

/* HSPEC_SYNERGY0_0 (PIMM PSC) shifts and masks */

#define HS_PIMM_PSC_SHFT(subnode)	(4 * (subnode))
#define HS_PIMM_PSC_MASK(subnode)	(0xf << HS_PIMM_PSC_SHFT(subnode))


/*
 * LED register macros
 */

#ifdef _LANGUAGE_C

#define CPU_LED_ADDR(_nasid, _slice)					   \
	REMOTE_HSPEC_ADDR((_nasid), HSPEC_LED0 + ((_slice) << 3))

#define SET_CPU_LEDS(_nasid, _slice,  _val)				   \
	(HUB_S(CPU_LED_ADDR(_nasid, _slice), (_val)))

#define SET_MY_LEDS(_v) 						   \
	SET_CPU_LEDS(get_nasid(), get_slice(), (_v))

#endif /* _LANGUAGE_C */
#endif	/* _ASM_SN_SN1_BDRKHSPECREGS_NEXT_H */
