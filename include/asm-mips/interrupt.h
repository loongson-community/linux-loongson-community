#ifndef _ASM_MIPS_INTERRUPT_H
#define _ASM_MIPS_INTERRUPT_H

extern inline void mark_bh(int nr)
{
	__asm__ __volatile__(
		"1:\tll\t$8,(%0)\n\t"
		"or\t$8,$8,%1\n\t"
		"sc\t$8,(%0)\n\t"
		"beq\t$0,$8,1b\n\t"
		: "=m" (bh_active)
		: "r" (1<<nr)
		: "$8","memory");
}

extern inline void disable_bh(int nr)
{
	__asm__ __volatile__(
		"1:\tll\t$8,(%0)\n\t"
		"and\t$8,$8,%1\n\t"
		"sc\t$8,(%0)\n\t"
		"beq\t$0,$8,1b\n\t"
		: "=m" (bh_mask)
		: "r" (1<<nr)
		: "$8","memory");
}

extern inline void enable_bh(int nr)
{
	__asm__ __volatile__(
		"1:\tll\t$8,(%0)\n\t"
		"or\t$8,$8,%1\n\t"
		"sc\t$8,(%0)\n\t"
		"beq\t$0,$8,1b\n\t"
		: "=m" (bh_mask)
		: "r" (1<<nr)
		: "$8","memory");
}

#endif /* _ASM_MIPS_INTERRUPT_H */
