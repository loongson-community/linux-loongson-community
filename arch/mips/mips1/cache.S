# R3000 cache routines lifted from IDT documentation
# by Ruud Riem-Viis. Adapted for linux by Didier Frick
# (dfrick@dial.eunet.ch)

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/cache.h>
#include <asm/mipsregs.h>
#include <asm/mipsconfig.h>
#include <asm/stackframe.h>
#include <asm/bootinfo.h>

#define	MINCACHE	0x00200		/* minimum cache size 512 */
#define MAXCACHE	0x04000		/* maximum cache size 16K */

/*
 * Figure out the size of the I- and D-caches, using the diagnostic isolate
 * swap features. The cache size is left in an environment variable because
 * the system will want to know it later.
 * Flush the cache so that it is in a known state.
 */
NESTED(cache_init,8,ra)
	subu	sp, sp, 24		# keep sp aligned on 8 words
	sw	ra, 20(sp)		# push return address on stack
	sw	s0, 16(sp)		# save s0 on stack
	mfc0	s0, CP0_STATUS		# save sr
	mtc0	zero, CP0_STATUS
	nop
	.set	reorder
	jal	cache_size
	sw	v0, mips_dcache_size
	li	v0, ST0_CE 		# swap caches
	.set	noreorder
	mtc0	v0, CP0_STATUS
	nop
	jal	cache_size
	nop
	sw	v0, mips_icache_size
	mtc0	zero, CP0_STATUS	# swap back caches
	nop
	mtc0	s0, CP0_STATUS		# restore sr
	nop
	.set	reorder
	jal	cache_flush
	lw	s0, 16(sp)		# restore s0
	lw	ra, 20(sp)
	addu	sp, sp, 24
	j	ra
	nop
END(cache_init)

LEAF(cache_size)
	.set	noreorder
	mfc0	t0, CP0_STATUS		# save current SR
	nop
	and	t0, ~ST0_SR		# do not clear parity error bit
	or	v0, t0, ST0_DE		# isolate cache
	mtc0	v0, CP0_STATUS		
	nop

	move	v0, zero
	li	v1, 0xa5a5a5a5
	nop
	sw	v1, KSEG0		# try to write in cache
	lw	t1, KSEG0		# try to read from cache
	nop
	mfc0	t2, CP0_STATUS
	nop
	.set	reorder
	and	t2, (1<<19)
	bne	t2, zero, 3f		# cache miss, must be no cache
	bne	v1, t1, 3f		# data not equal -> no cache

/*
 * Clear cache boundries to known state.
 */
	li	v0, MINCACHE
1:
	sw	zero, KSEG0(v0)
	sll	v0, 1
	ble	v0, MAXCACHE, 1b

	li	v0, -1
	sw	v0, KSEG0(zero)		# store marker in cache
	li	v0, MINCACHE		# MIN cache size
2:
	lw	v1, KSEG0(v0)		# look for marker
	bne	v1, zero, 3f		# found marker
	sll	v0, 1			# cache size * 2
	ble	v0, MAXCACHE, 2b	# keep looking
	move	v0, zero		# must be no cache
	.set noreorder
3:
	mtc0	t0, CP0_STATUS		# restore sr
	nop
	j	ra
	nop
	.set	reorder
END(cache_size)

LEAF(cache_flush)
	lw	t1, mips_icache_size
	lw	t2, mips_dcache_size
	.set	noreorder
	mfc0	t3, CP0_STATUS		# save sr
	nop
	and	t3, ~ST0_SR		# do not clear PE
	beq	t1, zero, check_dcache	# if no icache, check dcache
	nop
	li	v0, ST0_DE | ST0_CE	# isolate and swap
	nop
	mtc0	v0, CP0_STATUS
	nop
	li	t0, KSEG0
	.set 	reorder
	or	t1, t0, t1
1:
	sb	zero, 0(t0)
	sb	zero, 4(t0)
	sb	zero, 8(t0)
	sb	zero, 12(t0)
	sb	zero, 16(t0)
	sb	zero, 20(t0)
	sb	zero, 24(t0)
	addu	t0, 32
	sb	zero, -4(t0)
	bne	t0, t1, 1b		# continue until done

check_dcache:
	li	v0, ST0_DE
	nop
	.set	noreorder
	mtc0	v0, CP0_STATUS
	nop
	beq	t2, zero, flush_done	# if no dcache, done
	.set	reorder
	li	t0, KSEG0
	or	t1, t0, t2
1:
	sb	zero, 0(t0)
	sb	zero, 4(t0)
	sb	zero, 8(t0)
	sb	zero, 12(t0)
	sb	zero, 16(t0)
	sb	zero, 20(t0)
	sb	zero, 24(t0)
	addu	t0, 32
	sb	zero, -4(t0)
	bne	t0, t1, 1b		# continue until done

	.set	noreorder
flush_done:
	mtc0	t3, CP0_STATUS		# restore old sr
	nop
	j	ra
	nop
	.set	reorder
END(cache_flush)	


