/*
 * arch/mips/mips3/memcpy.S
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1996 by Ralf Baechle
 *
 * Less stupid memcpy/user_copy implementation for 64 bit MIPS CPUs.
 * Much faster than the old memcpy but definately work in progress.
 * The list of tricks for a good memcpy is long ...
 */
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>

#define BLOCK_SIZE	(SZREG*4)

#define SWAP(x,y)                               \
		subu	x,x,y;                  \
		addu	y,x,y;                  \
		subu	x,y,x

#define EX(addr,handler)                        \
		.section	__ex_table,"a"; \
		PTR		addr, handler;  \
		.text
#define UEX(addr,handler)                       \
		EX(addr,handler);               \
		EX(addr+4,handler)

		.set		noreorder
		.set		noat

/* ---------------------------------------------------------------------- */

not_dw_aligned:
/*
 * At least one address is missaligned.
 * Let's see if we can fix the alignment.
 */
		LONG_SUBU	v1,zero,a0
		andi		v1,SZREG-1
		sltu		t0,v0,v1
		MOVN(v1,v0,t0)
		beqz		v1,still_not_aligned
		LONG_ADDU	v1,a0			# delay slot
1:		lb		$1,(a1)
		EX(1b, fault)
		LONG_ADDIU	a1,1
2:		sb		$1,(a0)
		EX(2b, fault)
		LONG_ADDIU	a0,1
		bne		a0,v1,1b
		LONG_SUBU	v0,1			# delay slot

/*
 * Ok, now the destination address is 8-byte aligned.
 * Is the source also aligned?
 */
		andi		t0,a1,SZREG-1
		beqz		t0,align8		# fine ...

/*
 * Bad.  We could only fix the alignment of the destination address.
 * Now let's copy in the usual BLOCK_SIZE byte blocks using unaligned
 * load and aligned stores.
 */
still_not_aligned:
		ori		v1,v0,BLOCK_SIZE-1	# delay slot
		xori		v1,BLOCK_SIZE-1
		beqz		v1,copy_left_over
		nop					# delay slot
		LONG_SUBU	v0,v1
		LONG_ADDU	v1,a0

1:		uld		t0,(a1)
		UEX(1b, fault)
2:		uld		t1,8(a1)
		UEX(2b, fault)
2:		uld		t2,16(a1)
		UEX(2b, fault)
2:		uld		t3,24(a1)
		UEX(2b, fault)
2:		sd		t0,(a0)
		EX(2b, fault)
2:		sd		t1,8(a0)
		EX(2b, fault_plus_one_reg)
2:		sd		t2,16(a0)
		EX(2b, fault_plus_two_reg)
2:		sd		t3,24(a0)
		EX(2b, fault_plus_three_reg)
		LONG_ADDIU	a0,BLOCK_SIZE
		bne		a0,v1,1b
		LONG_ADDIU	a1,BLOCK_SIZE		# delay slot
9:
		b		copy_left_over		# < BLOCK_SIZE bytes left
		nop					# delay slot

/* ---------------------------------------------------------------------- */

LEAF(__copy_user)

		or		t1,a0,a1
		andi		t1,SZREG-1
		bnez		t1,not_dw_aligned
		move		v0,a2			# delay slot

align8:
		ori		v1,v0,BLOCK_SIZE-1
		xori		v1,BLOCK_SIZE-1
		beqz		v1,copy_left_over
		nop					# delay slot
		LONG_SUBU	v0,v1
		LONG_ADDU	v1,a0

1:		ld		t0,(a1)
		EX(1b, fault)
2:		ld		t1,8(a1)
		EX(2b, fault)
2:		ld		t2,16(a1)
		EX(2b, fault)
2:		ld		t3,24(a1)
		EX(2b, fault)
2:		sd		t0,(a0)
		EX(2b, fault)
2:		sd		t1,8(a0)
		EX(2b, fault_plus_one_reg)
2:		sd		t2,16(a0)
		EX(2b, fault_plus_two_reg)
2:		sd		t3,24(a0)
		EX(2b, fault_plus_three_reg)
		LONG_ADDIU	a0,BLOCK_SIZE
		bne		a0,v1,1b
		LONG_ADDIU	a1,BLOCK_SIZE		# delay slot
9:

/*
 * We've got upto 31 bytes left to copy ...
 */
copy_left_over:
		beqz		v0,3f
		nop					# delay slot
1:		lb		$1,(a1)
		EX(1b, fault)
		LONG_ADDIU	a1,1
2:		sb		$1,(a0)
		EX(2b, fault)
		LONG_SUBU	v0,1
		bnez		v0,1b
		LONG_ADDIU	a0,1
3:		jr		ra
		nop				# delay slot

		END(__copy_user)
		.set		at
		.set		reorder

/* ---------------------------------------------------------------------- */

/*
 * Access fault.  The number of not copied bytes is in v0.  We have to
 * correct the number of the not copied bytes in v0 in case of a access
 * fault in an unrolled loop, then return.
 */

fault:			jr	ra
fault_plus_one_reg:	LONG_ADDIU	v0,SZREG
			jr	ra
fault_plus_two_reg:	LONG_ADDIU	v0,SZREG*2
			jr	ra
fault_plus_three_reg:	LONG_ADDIU	v0,SZREG*3
			jr	ra

/* ---------------------------------------------------------------------- */

/*
 * For now we use __copy_user for __memcpy, too.  This is efficient (one
 * instruction penatly) and smaller but adds unwanted error checking we don't
 * need.  This hopefully doesn't cover any bugs.  The memcpy() wrapper in
 * <asm/string.h> takes care of the return value in a way GCC can optimize.
 */
			.globl	__memcpy
__memcpy		=	__copy_user
