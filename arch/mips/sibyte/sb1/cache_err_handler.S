/*
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/config.h>
#include <linux/threads.h>

#include <asm/asm.h>
#include <asm/cacheops.h>
#include <asm/current.h>
#include <asm/offset.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/cachectl.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#define __ASSEMBLY__
#include <asm/bootinfo.h>

	.text
	/* Special Cache Error handler for SB1 for now*/
	LEAF(except_vec2_sb1)
	.set	noat
	.set    mips0
	/*
	 * This is a very bad place to be.  Our cache error
	 * detection has triggered.  If we have write-back data
	 * in the cache, we may not be able to recover.  As a
	 * first-order desperate measure, turn off KSEG0 cacheing.
	 */
        .set    push
        .set    mips64
        .set    reorder
        # look for signature of spurious CErr
        mfc0    k1,$26,0
        lui     k0,0x4000
        bne     k0,k1,real_cerr
        mfc0    k1,$27,1
        lui     k0,0xffe0
        and     k1,k0,k1
        lui     k0,0x0200
        bne     k0,k1,real_cerr
        # clear/unlock the registers
        mtc0    zero,$26,0
        mtc0    zero,$27,1
        eret

        # XXXXPK -  this is a real error.  set the LEDs and spin for now
real_cerr:
        lui     k0,0xb00A
        li      k1,'C'
        sb      k1,56(k0)
        li      k1,'E'
        sb      k1,48(k0)
        li      k1,'R'
        sb      k1,40(k0)
        li      k1,'R'
        sb      k1,32(k0)
2:
        b       2b
        .set    pop
	END(except_vec2_sb1)
