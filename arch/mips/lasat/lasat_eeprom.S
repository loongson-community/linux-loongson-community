/*
 * Thomas Horsten <thh@lasat.com>
 * Copyright (C) 2000 LASAT Networks A/S.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Atmel AT93C46 serial EEPROM functions.
 */

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/lasat/lasat.h>

#if defined(CONFIG_LASAT_100)
# define DELAYBASE 1
#elif defined(CONFIG_LASAT_200)
# define DELAYBASE 3
#endif

#define DELAYn(n, tmp) ; \
	li	tmp, n * DELAYBASE ; \
15:	bnez	tmp, 15b ; \
	 sub	tmp, 1

#define DELAY50(tmp) DELAYn(2, tmp)
#define DELAY100(tmp) DELAYn(4, tmp)
#define DELAY150(tmp) DELAYn(6, tmp)
#define DELAY200(tmp) DELAYn(7, tmp)
#define DELAY250(tmp) DELAYn(9, tmp)
#define DELAY1000(tmp) DELAYn(34, tmp)

/* Set up for an access */
#define SETUPCS(tmp,ct) ; \
	lw	tmp, 0(t0) /* CS high, SK, data low */ ; \
	and	tmp, ~LASAT_EEPROM_MASK ; \
	or	tmp, LASAT_EEPROM_CS; \
	sw	tmp, 0(t0) ; \
	/* tCSS from CS to first clock */ ; \
	DELAY50(ct)

/* Done with access */
#define DONECS(tmp,ct) ; \
	/* CS hold 0 */ ; \
	lw	tmp, 0(t0) /* CS, CLK, DATA low */ ; \
	and	tmp, ~LASAT_EEPROM_MASK ; \
	sw	tmp, 0(t0) ; \
	/* tCS */ ; \
	DELAY250(ct)

/* Wait for the operation to complete */
#define DONEWAIT(tmp,timeout,ct) ; \
	/* CS high */ ; \
	lw	tmp, 0(t0) ; \
	or	tmp, LASAT_EEPROM_CS ; \
	sw	tmp, 0(t0) ; \
	DELAY250(ct) ; \
10:	lw	tmp, LASAT_EEPROM_READ_OFFSET(t0) ; \
	and	tmp, LASAT_EEPROM_RDATA ; \
	bnez	tmp, 11f ; \
	 nop ; \
	bnez	timeout, 10b ; \
	 subu	timeout, 1 ; \
11:	/* CS low again */ ; \
	lw	tmp, 0(t0) ; \
	and	tmp, ~LASAT_EEPROM_CS ; \
	sw	tmp, 0(t0)

/* Write bit to the eeprom */
/* Assume register address in t0 */
/* Destroys data and tmp registers */
#define WRBIT(data,tmp,ct) ; \
	/* WRBIT */ ; \
	lw	tmp, 0(t0) ; \
	and	tmp, ~LASAT_EEPROM_WDATA ; \
	/* Put the data */ ; \
	sll	data, LASAT_EEPROM_WDATA_SHIFT ; \
	and	data, LASAT_EEPROM_WDATA ; \
	or	tmp, tmp, data ; \
	sw	tmp, 0(t0) ; \
	/* Clock goes high after tDIS, 100ns */ ; \
	DELAY100(ct) ; \
	or	tmp, LASAT_EEPROM_CLK ; \
	sw	tmp, 0(t0) ; \
	/* Clock stays high for tSKH, 250ns */ ; \
	DELAY250(ct) ; \
	/* Clock goes low and stays low for 250ns */ ; \
	and	tmp, ~LASAT_EEPROM_CLK ; \
	sw	tmp, 0(t0) ; \
	DELAY250(ct) ; \
	/* That's that */

/* Read bit from the eeprom */
/* Assume register address in t0 */
/* Destroys data and tmp registers */
#define RDBIT(data,tmp, ct) ; \
	/* RDBIT */ ; \
	/* Clock goes high */ ; \
	lw	tmp, 0(t0) ; \
	or	tmp, LASAT_EEPROM_CLK ; \
	sw	tmp, 0(t0) ; \
	/* Data ready after tPD0, 250 ns */ ; \
	DELAY250(ct) ; \
	/* Get the data */ ; \
	lw	tmp, LASAT_EEPROM_READ_OFFSET(t0) ; \
	and	data, tmp, LASAT_EEPROM_RDATA ; \
	srl	data, LASAT_EEPROM_RDATA_SHIFT ; \
	/* Clock goes low */ ; \
	lw	tmp, 0(t0) ; \
	and	tmp, ~LASAT_EEPROM_CLK ; \
	sw	tmp, 0(t0) ; \
	/* Hold for tSKL, 250 */ ; \
	DELAY250(ct) ; \
	/* That's that */


/* Write n+1 bits to eeprom, nbits is a constant */
#define WRBITS(data, tmp, ctmp, tmp2, nbits, ct) ; \
	li	ctmp, nbits ; \
10:	move	tmp, data ; \
	srl	tmp, nbits ; \
	WRBIT(tmp,tmp2,ct) ; \
	sll	data, 1 ; \
	bnez	ctmp, 10b ; \
	 sub	ctmp, 1

#define RDBITS(data, tmp, ctmp, tmp2, nbits, ct) ; \
	li	ctmp, nbits ; \
10:	RDBIT(tmp,tmp2,ct) ; \
	sll	data, 1 ; \
	or	data, tmp ; \
	bnez	ctmp, 10b ; \
	 sub	ctmp, 1

	.set	noreorder
	.set	at
	
	/* Read 1 byte at address a0, result in v0 */
LEAF(lasat_eeprom_read)
	la	t0, LASAT_EEPROM_REG

	SETUPCS(t1, t5)
	li	t1, 0x00000300	/* Read command */
	or	t1, t1, a0	/* Mask in address */
	WRBITS(t1, t2, t3, t4, 9, t5)
	RDBITS(v0, t1, t2, t3, 7, t5)
	DONECS(t1, t5)
	j	ra
	 nop
END(lasat_eeprom_read)

	/* Write 1 byte at address a0, data = a1 */
	/* Return -1 in v0 on error, otherwise 0 */
LEAF(lasat_eeprom_write)
	la	t0, LASAT_EEPROM_REG

	SETUPCS(t1, t5)
	li	t1, 0x00000280	/* Write command */
	or	t1, t1, a0	/* Mask in address */
	WRBITS(t1, t2, t3, t4, 9, t5)
	WRBITS(a1, t2, t3, t4, 7, t5)
	DONECS(t1, t5)
	/* Timeout */
	li	t2, 0xa2c2b
	DONEWAIT(t1,t2, t5)
	bnez	t2, 1f
	 li	v0, 0
	li	v0, -1
1:	j	ra
	 nop
END(lasat_eeprom_write)
	
	/* Enable/disable write protect */
	/* a0: 1 = write protect, 0 = write enable */
LEAF(lasat_eeprom_wp)
	la	t0, LASAT_EEPROM_REG

	bnez	a0, 1f
	 li	t1, 0x00000200
	li	t1, 0x00000260
1:	SETUPCS(t2, t5)
	WRBITS(t1, t2, t3, t4, 9, t5)
	DONECS(t1, t5)
	j	ra
	 nop
END(lasat_eeprom_wp)

	/* Get the first two configuration words into k0/k1 */
LEAF(lasat_eeprom_getcfg)
	move	a1, ra
	move	k0, zero
	move	k1, zero

	/* First byte */
	li	a0, 4
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	or	k0, v0

	/* 2nd byte */
	li	a0, 5
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	sll	v0, 8
	or	k0, v0

	/* 3rd byte */
	li	a0, 6
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	sll	v0, 16
	or	k0, v0

	/* 4th byte */
	li	a0, 7
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	sll	v0, 24
	or	k0, v0
	
	/* First byte, 2nd word */
	li	a0, 8
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	or	k1, v0

	/* 2nd byte */
	li	a0, 9
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	sll	v0, 8
	or	k1, v0

	/* 3rd byte */
	li	a0, 10
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	sll	v0, 16
	or	k1, v0

	/* 4th byte */
	li	a0, 11
	jal	lasat_eeprom_read
	 nop
	andi	v0, 0xff
	sll	v0, 24
	or	k1, v0

	move	ra, a1
	jr	ra
	 nop	
END(lasat_eeprom_getcfg)
