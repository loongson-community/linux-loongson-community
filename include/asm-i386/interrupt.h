#ifndef _ASM_I386_INTERRUPT_H
#define _ASM_I386_INTERRUPT_H

extern inline void mark_bh(int nr)
{
	__asm__ __volatile__("orl %1,%0":"=m" (bh_active):"ir" (1<<nr));
}

extern inline void disable_bh(int nr)
{
	__asm__ __volatile__("andl %1,%0":"=m" (bh_mask):"ir" (~(1<<nr)));
}

extern inline void enable_bh(int nr)
{
	__asm__ __volatile__("orl %1,%0":"=m" (bh_mask):"ir" (1<<nr));
}

#endif /* _ASM_I386_INTERRUPT_H */
