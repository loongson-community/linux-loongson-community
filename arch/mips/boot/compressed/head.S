/*
 *  arch/mips/boot/compressed/head.S
 *
 *  Copyright (C) 1995 by Ralf Baechle
 *
 *  Head.S contains the MIPS exception handler and startup code.
 */

#include <asm/asm.h>

		.text
		.set	noreorder
/*
 * Compressed kernel entry
 */
		NESTED(kernel_entry, 16, sp)
		/*
		 * Set EXL in c0_status. The results in the lowest two
		 * gigabytes identity mapped.
		 */
		mfc0	t0,CP0_STATUS
		ori	t0,4
		mtc0	t0,CP0_STATUS

		/*
		 * Clear BSS first so that there are no surprises...
		 */
		la	t0,_edata
		la	t1,_end
		sw	zero,(t0)
1:
		addiu	t0,4

		bnel	t0,t1,1b
		 sw	zero,(t0)
		END(kernel_entry)

		/*
		 * Do the decompression, and jump to the new kernel..
		 */
		jal	C_LABEL(decompress_kernel)
		 nop

		/*
		 * Flush caches
		 */
		jal	C_LABEL(cacheflush)

		/*
		 * Jump into the decompressed kernel
		 */
		la	t0,KSEG0
		jr	t0
		 nop
