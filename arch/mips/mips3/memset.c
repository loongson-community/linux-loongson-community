/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 *
 * Less stupid memset for 64 bit MIPS CPUs.
 */
#include <linux/linkage.h>
#include <linux/string.h>

static void __inline__ b_memset(void *s, unsigned long long c, size_t count)
{
	unsigned char *p = s;

	while(count--)
		*(p++) = c;
}

static void __inline__ dw_memset(void *s, unsigned long long c, size_t count)
{
	unsigned long long *p = s;

	count >>= 3;
	while(count--)
		*(p++) = c;
}

asm(	".globl\t__generic_memset_b\n\t"
	".align\t2\n\t"
	".type\t__generic_memset_b,@function\n\t"
	".ent\t__generic_memset_b,0\n\t"
	".frame\t$29,0,$31\n"
	"__generic_memset_b:\n\t"
	"andi\t$5,0xff\n\t"
	"dsll\t$2,$5,8\n\t"
	"or\t$5,$2\n\t"
	"dsll\t$2,$5,16\n\t"
	"or\t$5,$2\n\t"
	"dsll32\t$2,$5,0\n\t"
	"or\t$5,$2\n\t"
	".end\t__generic_memset_b\n\t"
	".size\t__generic_memset_b,.-t__generic_memset_b");

/*
 * Fill small area bytewise.  For big areas fill the source bytewise
 * until the pointer is doubleword aligned, then fill in doublewords.
 * Fill the rest again in single bytes.
 */
void __generic_memset_dw(void *s, unsigned long long c, size_t count)
{
	unsigned long i;

	/*
	 * Fill small areas bytewise.
	 */
	if (count <= 16) {
		b_memset(s, c, count);
		return;
	}

	/*
	 * Pad for 8 byte boundary
	 */
	i = 8 - ((unsigned long)s & 7);
	b_memset(s, c, i);
	s += i;
	count -= i;

	/*
	 * Now start filling with aligned doublewords
	 */
	dw_memset(s, c, count);
	s += (count | 7) ^ 7;
	count &= 7;

	/*
	 * And do what ever is left over again with single bytes.
	 */
	b_memset(s, c, count);
}
