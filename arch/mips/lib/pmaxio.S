#include <asm/regdef.h>
#include <asm/dec/decstation.h>

		.text
		.set	reorder
/*
 * pmax_printf - call the PROM printf() function
 */
		.globl	pmax_printf
pmax_printf:
		lw	v0,pmax_rex_base
		lw	v0,REX_PRINTF(v0)
		j	v0

/*
 * pmax_getchar - call the PROM getchar() function
 */
		.globl	pmax_getch
pmax_getch:
		lw	v0,pmax_rex_base
		lw	v0,REX_GETCHAR(v0)
		j	v0

/*
 * pmax_putchar - call the PROM putchar() function
 */
		.globl	pmax_putch
pmax_putch:
		lw	v0,pmax_rex_base
		lw	v0,REX_PUTCHAR(v0)
		j	v0

/*
 * pmax_callfn - call the PROM function
 */
		.globl	pmax_callfn
pmax_callfn:
		lw	v0,pmax_rex_base
		addu	v0,v0,a0
		lw	v0,(v0)
		j	v0

/*
 * pmax_getbitmap - call the PROM for memory bitmap function
 */
		.globl	pmax_getbitmap
pmax_getbitmap:
		lw	v0,pmax_rex_base
		lw	v0,REX_GETBITMAP(v0)
		j	v0

/*
 * pmax_getgetenv - call the PROM to get environment variable
 */
		.globl	pmax_getenv
pmax_getenv:
		lw	v0,pmax_rex_base
		lw	v0,REX_GETENV(v0)
		j	v0

/*
 * pmax_getsysid - call the PROM to get system id
 */
		.globl	pmax_getsysid
pmax_getsysid:
		lw	v0,pmax_rex_base
		lw	v0,REX_GETSYSID(v0)
		j	v0

/*
 * pmax_halt - call the PROM halt() function
 */
		.globl	pmax_halt
pmax_halt:
		lw	v0,pmax_rex_base
		lw	v0,REX_HALT(v0)
		j	v0

