/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		IP/TCP/UDP checksumming routines
 *
 * Authors:	Jorge Cwik, <jorge@laser.satlink.net>
 *		Arnt Gulbrandsen, <agulbra@nvg.unit.no>
 *		Lots of code moved from tcp.c and ip.c; see those files
 *		for more names.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

#include <net/checksum.h>
#ifdef __mips__
#include <asm/string.h>
#endif

/*
 * computes a partial checksum, e.g. for TCP/UDP fragments
 */

unsigned int csum_partial(unsigned char * buff, int len, unsigned int sum) {
#ifdef __i386__
	__asm__("
	    movl %%ecx, %%edx
	    cld
	    shrl $5, %%ecx
	    jz 2f
	    orl %%ecx, %%ecx
1:	    movl (%%esi), %%eax
	    adcl %%eax, %%ebx
	    movl 4(%%esi), %%eax
	    adcl %%eax, %%ebx
	    movl 8(%%esi), %%eax
	    adcl %%eax, %%ebx
	    movl 12(%%esi), %%eax
	    adcl %%eax, %%ebx
	    movl 16(%%esi), %%eax
	    adcl %%eax, %%ebx
	    movl 20(%%esi), %%eax
	    adcl %%eax, %%ebx
	    movl 24(%%esi), %%eax
	    adcl %%eax, %%ebx
	    movl 28(%%esi), %%eax
	    adcl %%eax, %%ebx
	    lea 32(%%esi), %%esi
	    dec %%ecx
	    jne 1b
	    adcl $0, %%ebx
2:	    movl %%edx, %%ecx
	    andl $28, %%ecx
	    je 4f
	    shrl $2, %%ecx
	    orl %%ecx, %%ecx
3:	    adcl (%%esi), %%ebx
	    lea 4(%%esi), %%esi
	    dec %%ecx
	    jne 3b
	    adcl $0, %%ebx
4:	    movl $0, %%eax
	    testw $2, %%dx
	    je 5f
	    lodsw
	    addl %%eax, %%ebx
	    adcl $0, %%ebx
	    movw $0, %%ax
5:	    test $1, %%edx
	    je 6f
	    lodsb
	    addl %%eax, %%ebx
	    adcl $0, %%ebx
6:	    "
	: "=b"(sum)
	: "0"(sum), "c"(len), "S"(buff)
	: "ax", "bx", "cx", "dx", "si" );
#elif defined (__mips__)
	unsigned long	scratch1;
	unsigned long	scratch2;

    __asm__("
	.set	noreorder
	.set	noat
	move	%1,%4
	srl	%1,%1,5
	beqz	%1,2f
	sll	%1,%1,5			# delay slot

	addu	%1,%5
1:	lw	%2,0(%5)
	addu	%5,32
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-28(%5)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-24(%5)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-20(%5)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-16(%5)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-12(%5)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-8(%5)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	lw	%2,-4(%5)
	addu	%0,$1
	addu	%0,%2
	sltu	$1,%0,%2

	bne	%5,%1,1b
	addu	%0,$1			# delay slot

2:	srl	%1,%4,2
	bnez	%1,4f
	addu	%1,%5			# delay slot
3:	lw	%2,0(%5)
	addu	%5,4
	addu	%0,%2
	sltu	$1,%0,%2
	bne	%5,%1,3b
	addu	%0,$1			# delay slot

4:	andi	$1,%4,2
	beqz	%4,5f
	lhu	%2,0(%5)		# delay slot
	addu	%5,2
	addu	%0,%2
	sltu	$1,%0,%2
	addu	%0,$1			# delay slot

5:	andi	$1,%4,1
	beqz	%4,6f
	lbu	%2,0(%5)		# delay slot
	addu	%0,%2
	sltu	$1,%0,%2
	addu	%0,$1			# delay slot
6:	.set	at
	.set	reorder"
	: "=r"(sum), "=r" (scratch1), "=r" (scratch2)
	: "0"(sum), "r"(len), "r"(buff)
	: "$1");
#else
#error Not implemented for this CPU
#endif
	return(sum);
}



/*
 * copy from fs while checksumming, otherwise like csum_partial
 */

unsigned int csum_partial_copyffs( char *src, char *dst, 
				  int len, int sum) {
#ifdef __i386__
    __asm__("
	push %%ds
	push %%es
	movw %%ds, %%dx
	movw %%dx, %%es
	movw %%fs, %%dx
	movw %%dx, %%ds
	cld
	cmpl $32, %%ecx
	jb 2f
	pushl %%ecx
	shrl $5, %%ecx
	orl %%ecx, %%ecx
1:	movl (%%esi), %%eax
	movl 4(%%esi), %%edx
	adcl %%eax, %%ebx
	movl %%eax, %%es:(%%edi)
	adcl %%edx, %%ebx
	movl %%edx, %%es:4(%%edi)

	movl 8(%%esi), %%eax
	movl 12(%%esi), %%edx
	adcl %%eax, %%ebx
	movl %%eax, %%es:8(%%edi)
	adcl %%edx, %%ebx
	movl %%edx, %%es:12(%%edi)

	movl 16(%%esi), %%eax
	movl 20(%%esi), %%edx
	adcl %%eax, %%ebx
	movl %%eax, %%es:16(%%edi)
	adcl %%edx, %%ebx
	movl %%edx, %%es:20(%%edi)

	movl 24(%%esi), %%eax
	movl 28(%%esi), %%edx
	adcl %%eax, %%ebx
	movl %%eax, %%es:24(%%edi)
	adcl %%edx, %%ebx
	movl %%edx, %%es:28(%%edi)

	lea 32(%%esi), %%esi
	lea 32(%%edi), %%edi
	dec %%ecx
	jne 1b
	adcl $0, %%ebx
	popl %%ecx
2:	movl %%ecx, %%edx
	andl $28, %%ecx
	je 4f
	shrl $2, %%ecx
	orl %%ecx, %%ecx
3:	movl (%%esi), %%eax
	adcl %%eax, %%ebx
	movl %%eax, %%es:(%%edi)
	lea 4(%%esi), %%esi
	lea 4(%%edi), %%edi
	dec %%ecx
	jne 3b
	adcl $0, %%ebx
4:	movl $0, %%eax
	testl $2, %%edx
	je 5f
	lodsw
	stosw
	addl %%eax, %%ebx
	movw $0, %%ax
	adcl %%eax, %%ebx
5:	test $1, %%edx
	je 6f
	lodsb
	stosb
	addl %%eax, %%ebx
	adcl $0, %%ebx
6:	pop %%es
	pop %%ds
	"
	: "=b"(sum)
	: "0"(sum), "c"(len), "S"(src), "D"(dst)
	: "ax", "bx", "cx", "dx", "si", "di" );
#elif defined (__mips__)
	/*
	 * It's 2:30 am and I don't feel like doing it real ...
	 * This is lots slower than the real thing (tm)
	 */
	sum = csum_partial(src, len, sum);
	memcpy(dst, src, len);
#else
#error Not implemented for this CPU
#endif
    return(sum);
}



