#include <asm/asm.h>
#include <asm/regdef.h>

/*
 * Just for debugging...
 */
		LEAF(beep)
		lw	t0,beepflag
		bnez	t0,1f
		lbu	t0,0xb4000061
		xori	t0,3
		sb	t0,0xb4000061
		li	t0,1
		sw	t0,beepflag
1:		jr	ra
		END(beep)

		.bss
beepflag:	.word	0
		.text

