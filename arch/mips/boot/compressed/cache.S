/*
 * arch/mips/boot/compressed/cache.S
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics
 * Written by Ralf Baechle
 *
 * Flush instruction/data caches
 *
 * Parameters: a0 - starting address to flush
 *             a1 - size of area to be flushed
 *             a2 - which caches to be flushed
 *
 * FIXME:      - ignores parameters in a0/a1
 *             - doesn't know about second level caches
 */
#include <linux/autoconf.h>

#include <asm/asm.h>
#include <asm/cachectl.h>
#include <asm/mipsregs.h>
#include <asm/segment.h>

/*
 * Some bits in the config register
 */
#define CONFIG_IB       (1<<5)
#define CONFIG_DB       (1<<4)

 /*
  * Flush instruction/data caches
  *
  * Parameters: a0 - starting address to flush
  *             a1 - size of area to be flushed
  *             a2 - which caches to be flushed
  *
  */

		.text

		.set	noreorder
		LEAF(cacheflush)
		andi	t1,a2,DCACHE

		beqz	t1,do_icache
		 li	t0,KSEG0

		/*
		 * Writeback data cache, even lines
		 */
		li	t1,CACHELINES-1
1:
		cache	Index_Writeback_Inv_D,0(t0)
		cache	Index_Writeback_Inv_D,32(t0)
		cache	Index_Writeback_Inv_D,64(t0)
		cache	Index_Writeback_Inv_D,96(t0)
		cache	Index_Writeback_Inv_D,128(t0)
		cache	Index_Writeback_Inv_D,160(t0)
		cache	Index_Writeback_Inv_D,192(t0)
		cache	Index_Writeback_Inv_D,224(t0)
		cache	Index_Writeback_Inv_D,256(t0)
		cache	Index_Writeback_Inv_D,288(t0)
		cache	Index_Writeback_Inv_D,320(t0)
		cache	Index_Writeback_Inv_D,352(t0)
		cache	Index_Writeback_Inv_D,384(t0)
		cache	Index_Writeback_Inv_D,416(t0)
		cache	Index_Writeback_Inv_D,448(t0)
		cache	Index_Writeback_Inv_D,480(t0)
		addiu	t0,512

		bnez	t1,1b
		 subu	t1,1

		/*
		 * Writeback data cache, odd lines
		 * Only needed for 16 byte line size
		 */
		mfc0	t1,CP0_CONFIG
		andi	t1,CONFIG_IB
		bnez	t1,do_icache
		 li	t1,CACHELINES-1
1:
		cache	Index_Writeback_Inv_D,16(t0)
		cache	Index_Writeback_Inv_D,48(t0)
		cache	Index_Writeback_Inv_D,80(t0)
		cache	Index_Writeback_Inv_D,112(t0)
		cache	Index_Writeback_Inv_D,144(t0)
		cache	Index_Writeback_Inv_D,176(t0)
		cache	Index_Writeback_Inv_D,208(t0)
		cache	Index_Writeback_Inv_D,240(t0)
		cache	Index_Writeback_Inv_D,272(t0)
		cache	Index_Writeback_Inv_D,304(t0)
		cache	Index_Writeback_Inv_D,336(t0)
		cache	Index_Writeback_Inv_D,368(t0)
		cache	Index_Writeback_Inv_D,400(t0)
		cache	Index_Writeback_Inv_D,432(t0)
		cache	Index_Writeback_Inv_D,464(t0)
		cache	Index_Writeback_Inv_D,496(t0)
		addiu	t0,512

		bnez	t1,1b
		 subu	t1,1

do_icache:
		andi	t1,a2,ICACHE
		beqz	t1,done
		 lui	t0,0x8000

		/*
		 * Flush instruction cache, even lines
		 */
		li	t1,CACHELINES-1
1:
		cache	Index_Invalidate_I,0(t0)	
		cache	Index_Invalidate_I,32(t0)
		cache	Index_Invalidate_I,64(t0)
		cache	Index_Invalidate_I,96(t0)
		cache	Index_Invalidate_I,128(t0)
		cache	Index_Invalidate_I,160(t0)
		cache	Index_Invalidate_I,192(t0)
		cache	Index_Invalidate_I,224(t0)
		cache	Index_Invalidate_I,256(t0)
		cache	Index_Invalidate_I,288(t0)
		cache	Index_Invalidate_I,320(t0)
		cache	Index_Invalidate_I,352(t0)
		cache	Index_Invalidate_I,384(t0)
		cache	Index_Invalidate_I,416(t0)
		cache	Index_Invalidate_I,448(t0)
		cache	Index_Invalidate_I,480(t0)
		addiu	t0,512

		bnez	t1,1b
		 subu	t1,1

		/*
		 * Flush instruction cache, even lines
		 * Only needed for 16 byte line size
		 */
		mfc0	t1,CP0_CONFIG
		andi	t1,CONFIG_DB
		bnez	t1,done
		 li	t1,CACHELINES-1

1:
		cache	Index_Invalidate_I,16(t0)
		cache	Index_Invalidate_I,48(t0)
		cache	Index_Invalidate_I,80(t0)
		cache	Index_Invalidate_I,112(t0)
		cache	Index_Invalidate_I,144(t0)
		cache	Index_Invalidate_I,176(t0)
		cache	Index_Invalidate_I,208(t0)
		cache	Index_Invalidate_I,240(t0)
		cache	Index_Invalidate_I,272(t0)
		cache	Index_Invalidate_I,304(t0)
		cache	Index_Invalidate_I,336(t0)
		cache	Index_Invalidate_I,368(t0)
		cache	Index_Invalidate_I,400(t0)
		cache	Index_Invalidate_I,432(t0)
		cache	Index_Invalidate_I,464(t0)
		cache	Index_Invalidate_I,496(t0)
		addiu	t0,512

		bnez	t1,1b
		 subu	t1,1

done:
		j	ra
		 nop
		END(sys_cacheflush)
