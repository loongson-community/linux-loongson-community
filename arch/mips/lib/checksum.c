/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		MIPS specific IP/TCP/UDP checksumming routines
 *
 * Authors:	Ralf Baechle, <ralf@waldorf-gmbh.de>
 *		Lots of code moved from tcp.c and ip.c; see those files
 *		for more names.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#include <net/checksum.h>
#include <asm/string.h>

/*
 * computes a partial checksum, e.g. for TCP/UDP fragments
 */
unsigned int csum_partial(const unsigned char *buff, int len, unsigned int sum)
{
	unsigned long	scratch1;
	unsigned long	scratch2;

	/*
	 * This is for 32-bit MIPS processors.
	 */
    __asm__("
	.set	noreorder
	.set	noat
	andi	$1,%4,2		# Check alignment
	beqz	$1,2f		# Branch if ok
	nop			# delay slot
	subu	$1,%3,2		# Alignment uses up two bytes
	bgez	$1,1f		# Jump if we had at least two bytes
	move	%3,$1		# delay slot
	j	4f
	addiu	%3,2		# delay slot; len was < 2.  Deal with it

1:	lhu	%2,(%4)
	addiu	%4,2
	addu	%0,%2
	sltu	$1,%0,%2
	addu	%0,$1

2:	srl	%1,%3,5
	beqz	%1,2f
	sll	%1,%1,5		# delay slot

	addu	%1,%4
1:	lw	%2,0(%4)
	addu	%4,32
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-28(%4)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-24(%4)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-20(%4)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-16(%4)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-12(%4)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-8(%4)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-4(%4)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	bne	%4,%1,1b
	addu	%0,$1		# delay slot

2:	andi	%1,%3,0x1c
	beqz	%1,4f
	addu	%1,%4		# delay slot
3:	lw	%2,0(%4)
	addu	%4,4
	addu	%0,%2
	sltu	$1,%0,%2
	bne	%4,%1,3b
	addu	%0,$1		# delay slot

4:	andi	$1,%3,3
	beqz	$1,7f
	andi	$1,%3,2		# delay slot
	beqz	$1,5f
	move	%2,$0		# delay slot
	lhu	%2,(%4)
	addiu	%4,2		# delay slot

5:	andi	$1,%3,1
	beqz	$1,6f
	nop			# delay slot
	lbu	%1,(%4)
	sll	%2,16\n\t"
#ifdef __MIPSEB__
	"sll	%1,8\n\t"
#endif
	"or	%2,%1
6:	addu	%0,%2
	sltu	$1,%0,%2
	addu	%0,$1
7:	.set	at
	.set	reorder"
	: "=r"(sum),
	  "=&r" (scratch1),
	  "=&r" (scratch2),
	  "=r" (len),
	  "=r" (buff)
	: "0"(sum), "3"(len), "4"(buff)
	: "$1");

	return sum;
}

/*
 * copy while checksumming, otherwise like csum_partial
 */
unsigned int csum_partial_copy(const char *src, char *dst, 
				  int len, unsigned int sum)
{
	/*
	 * It's 2:30 am and I don't feel like doing it right ...
	 * This is lots slower than the real thing (tm)
	 *
	 * XXX This may nuke the kernel for unaligned src addresses!!!
	 *     (Due to software address error fixing no longer true, but
	 *     seems to happen very rarely only anyway.)
	 */
	sum = csum_partial(src, len, sum);
	memcpy(dst, src, len);

	return sum;
}
