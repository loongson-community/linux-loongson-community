/* $Id: hal2.c,v 1.13 1999/02/07 22:18:37 ulfc Exp $
 * 
 * drivers/sgi/audio/hal2.c
 *
 * Copyright (C) 1998-1999 Ulf Carlsson (ulfc@bun.falkenberg.se)
 * 
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/soundcard.h>
#include <linux/sound.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/sgihpc.h>

#include "hal2.h"

#define DEBUG

static const unsigned sample_size[] = { 2, 4 };
static const unsigned sample_shift[] = { 1, 2 };

struct hal2_ring_buf {
	struct hpc_dma_desc desc;

	struct hal2_ring_buf *next;
	unsigned long buf;		/* pointer in KSEG1 address space */
};

/* As you might know, dma descriptors must be 8 byte aligned, that's why we have
 * them in this special way.
 */

struct hal2_state {
	/* soundcore stuff */
	int dev_audio;
	int dev_mixer;
	int dev_dac;

	int irq;

	unsigned int dacrate, adcrate;

	spinlock_t lock;
	struct semaphore open_sem;
	mode_t open_mode;
	struct wait_queue *open_wait;
	
	struct dmabuf {
		void *rawbuf;
		unsigned int buforder;
		struct hal2_ring_buf *ringbuf;

		struct hpc3_pbus_dmacregs *pbus;
		int pbusnr;

		unsigned int numfrag;
		unsigned int fragshift;
		int swptr;
		int hwptr;

		unsigned int total_bytes;

		int count;
		unsigned int error;	/* over/underrun */
		struct wait_queue *wait;


		/* Internal flags */
		unsigned little_end:1;
		unsigned stereo:1;
		unsigned receive:1;
		unsigned pbus_setup:1;
		unsigned pbus_en:1;
		unsigned port_en:1;

		/* redundant, but makes calculations easier */
		unsigned fragsize;
		unsigned dmasize;
		unsigned fragsamples;

		/* OSS stuff */
		unsigned mapped:1;
		unsigned ready:1;
		unsigned endcleared:1;
		unsigned ossfragshift;
		int ossmaxfrags;
		unsigned subdivision;
	} dma_dac, dma_adc;
};

struct hal2_state *dev;

/* ------------------------------------------------------------------------ */

extern inline unsigned ld2(unsigned int x)
{
	unsigned r = 0;
	
	if (x >= 0x10000) {
		x >>= 16;
		r += 16;
	}
	if (x >= 0x100) {
		x >>= 8;
		r += 8;
	}
	if (x >= 0x10) {
		x >>= 4;
		r += 4;
	}
	if (x >= 4) {
		x >>= 2;
		r += 2;
	}
	if (x >= 2)
		r++;
	return r;
}

/* ------------------------------------------------------------------ */

#define INDIRECT_WAIT(regs) while(regs->isr & H2_ISR_TSTATUS);

#if 0
#define INDIRECT_WAIT(regs)						\
{									\
	int cnt = 1000;							\
	printk("hal2: waiting isr:%04hx ", regs->isr);			\
	printk("idr0:%04hx idr1:%04hx idr2:%04hx idr3:%04hx\n",		\
	       regs->idr0, regs->idr1, regs->idr2, regs->idr3);		\
									\
	while(regs->isr & H2_ISR_TSTATUS && --cnt)			\
		udelay(5);						\
	if (!cnt)							\
		printk("hal2: failed waiting for indirect trans.\n");	\
									\
	printk("hal2: finished waiting at cnt:%d isr:%04hx ",		\
	       cnt, regs->isr);						\
	printk("idr0:%04hx idr1:%04hx idr2:%04hx idr3:%04hx\n",		\
	       regs->idr0, regs->idr1, regs->idr2, regs->idr3);		\
}
#endif

#define READ_ADDR(addr)		(addr | (1<<7))
#define WRITE_ADDR(addr)	(addr)

static unsigned short ireg_read16(unsigned short address)
{

	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	return h2_ctrl->idr0;
}

static unsigned long ireg_read32(unsigned short address)
{
	unsigned long ret;

	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	ret = h2_ctrl->idr0;
	h2_ctrl->iar = READ_ADDR(address | 0x1);
	INDIRECT_WAIT(h2_ctrl);
	ret |= h2_ctrl->idr0 << 16;
	return ret;
}

static void ireg_write16(unsigned short address, unsigned short val)
{
	h2_ctrl->idr0 = val;
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl)
}

static void ireg_write32(unsigned short address, unsigned long val)
{
	h2_ctrl->idr0 = val & 0xffff;
	h2_ctrl->idr1 = val >> 16;
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl)
}

static void ireg_write64(unsigned short address, unsigned long long val)
{
	h2_ctrl->idr0 = val & 0xffff;
	h2_ctrl->idr1 = (val >> 16) & 0xffff;
	h2_ctrl->idr2 = (val >> 32) & 0xffff;
	h2_ctrl->idr3 = (val >> 48);
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl)
}

static void ireg_setbit16(unsigned short address, unsigned short bit)
{
	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	h2_ctrl->idr0 = h2_ctrl->idr0 | bit;
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
}

static void ireg_setbit32(unsigned short address, unsigned long bit)
{
	unsigned long tmp;

	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	tmp = h2_ctrl->idr0 | (h2_ctrl->idr1 << 16) | bit;
	h2_ctrl->idr0 = tmp & 0xffff;
	h2_ctrl->idr1 = (tmp >> 16) & 0xffff;
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
}

static void ireg_clearbit16(unsigned long address, unsigned short bit)
{
	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	h2_ctrl->idr0 = h2_ctrl->idr0 & ~bit;
	h2_ctrl->iar = address;
	INDIRECT_WAIT(h2_ctrl);
}

static void ireg_clearbit32(unsigned short address, unsigned long bit)
{
	unsigned long tmp;

	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	tmp = (h2_ctrl->idr0 | (h2_ctrl->idr1 << 16)) & ~bit;
	h2_ctrl->idr0 = tmp & 0xffff;
	h2_ctrl->idr1 = (tmp >> 16) & 0xffff;
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
}

static void ireg_setmask16(unsigned short address, int shift,
			   unsigned short mask, unsigned short bits)
{
	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	h2_ctrl->idr0 = (h2_ctrl->idr0 & ~mask) | (bits << shift);
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
}

static void ireg_setmask32(unsigned long address, int shift,
			   unsigned long mask, unsigned long bits)
{
	unsigned long tmp;

	h2_ctrl->iar = READ_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
	tmp = ((h2_ctrl->idr1 | h2_ctrl->idr0) & ~mask) | (bits << shift);
	h2_ctrl->idr0 = tmp & 0xffff;
	h2_ctrl->idr1 = tmp >> 16;
	h2_ctrl->iar = WRITE_ADDR(address);
	INDIRECT_WAIT(h2_ctrl);
}

/* -----------------------------------------------------------------------*/

/* enable/disable a specific PBUS dma channel */
static __inline__ void hpcpbus_enable(struct hpc3_pbus_dmacregs *pbus,
				      struct hpc_dma_desc *desc)
{
	pbus->pbdma_dptr = PHYSADDR(desc);
	pbus->pbdma_ctrl |= HPC3_PDMACTRL_ACT;
}

static __inline__ void hpcpbus_disable(struct hpc3_pbus_dmacregs *pbus)
{
	pbus->pbdma_ctrl &= ~HPC3_PDMACTRL_ACT;
}

static __inline__ int hpcpbus_int(struct hpc3_pbus_dmacregs *pbus)
{
	/* When we read pbdma_ctrl, bit 0 indicates interrupt signal.
	 * The interrupt signal is also cleared after read
	 */

	return (pbus->pbdma_ctrl & 0x01);
}

/* ---------------------------------------------------------------------- */

/* We have three bresenham clock generators, which we can use independantly.
 *
 * There is one 44.1k and one 48.0k master clock for each of them. We can
 * adjust the inc and the mod values for those clocks, and thus reduce the
 * frequency.
 * 
 * freq = master * (inc / mod)
 * freq * mod = master * inc
 * mod = (master * inc) / freq
 * 
 * where (inc / mod) is a positive fraction in range [0,1]. Inc should always be
 * set to 4 for codecs
 *
 */

#define __BUILD_SET_BRESN_CLOCK(clock)					\
static __inline__ void hal2_set_bres##clock(unsigned short master,	\
					    unsigned long mod)		\
{									\
	unsigned long incmod = (4 << 16) | mod;				\
	ireg_write16(H2I_BRES##clock##_C1 , master);			\
	ireg_write32(H2I_BRES##clock##_C2 , incmod);			\
}

__BUILD_SET_BRESN_CLOCK(1)
__BUILD_SET_BRESN_CLOCK(2)
__BUILD_SET_BRESN_CLOCK(3)

static int hal2_set_adc_rate(struct hal2_state *s, unsigned int rate)
{
	/* XXX how do i set this one? */

	/* Try both 44.1k master and 48.0k master .. */

	if ((48000 << 2) % rate == 0)
		hal2_set_bres1(0, (48000 << 2) % rate);
	else if ((44100 << 2) % rate == 0)
		hal2_set_bres1(1, (44100 << 2) % rate);
	else
		/* this was no valid rate */
		return -EINVAL;

	return 0;
}

static int hal2_set_dac_rate(struct hal2_state *s, unsigned int rate)
{
	/* Try both 44.1k master and 48.0k master .. */

	if ((48000 << 2) % rate == 0) {
		hal2_set_bres1(0, (48000 << 2) % rate);
		s->dacrate = rate;
	} else if ((44100 << 2) % rate == 0) {
		hal2_set_bres1(1, (44100 << 2) % rate);
		s->dacrate = rate;
	} else
		/* this was no valid rate */
		return -EINVAL;

	return 0;
}

/* --------------------------------------------------------------------- */

static void hal2_stop(struct dmabuf *db)
{
	if (db->pbus_en) {
		hpcpbus_disable(db->pbus);
		db->pbus_en = 0;
	}
#ifdef DEBUG
	else
		printk("Trying to stop already stopped DMA channel\n");
#endif
}

static void hal2_stop_adc(struct hal2_state *s)
{
	struct dmabuf *db = &s->dma_adc;

	hal2_stop(db);
}

static void hal2_stop_dac(struct hal2_state *s)
{
	struct dmabuf *db = &s->dma_dac;

	hal2_stop(db);
}


static void hal2_start_adc(struct hal2_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	if (!s->dma_adc.port_en && (s->dma_adc.mapped || s->dma_adc.count < (signed)(s->dma_adc.dmasize - 2*s->dma_adc.fragsize)) && s->dma_adc.ready) {
		hpcpbus_enable(s->dma_adc.pbus, &s->dma_adc.ringbuf->desc);
		s->dma_adc.port_en = 1;
	}
	spin_unlock_irqrestore(&s->lock, flags);
}

static void hal2_start_dac(struct hal2_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	if (!s->dma_dac.port_en && (s->dma_dac.mapped || s->dma_dac.count > 0)
	    && s->dma_dac.ready) {
		hpcpbus_enable(s->dma_dac.pbus, &s->dma_dac.ringbuf->desc);
		s->dma_dac.port_en = 1;
	}
	spin_unlock_irqrestore(&s->lock, flags);

}

/* ----------------------------------------------------------------------- */ 

#define DMABUF_DEFAULTORDER (17-PAGE_SHIFT)
#define DMABUF_MINORDER 1

static void hal2_dealloc_dmabuf(struct dmabuf *db)
{
	unsigned long map, mapend;

	if (db->rawbuf) {
		/* undo marking the pages as reserved */
		mapend = MAP_NR(db->rawbuf + (PAGE_SIZE << db->buforder) -1);
		for (map = MAP_NR(db->rawbuf); map <= mapend; map++)
			clear_bit(PG_reserved, &mem_map[map].flags);
		free_pages((unsigned long)db->rawbuf, db->buforder);
	}
	db->rawbuf = NULL;
	db->mapped = db->ready = 0;
}

static int hal2_setup_ring(struct dmabuf *db)
{
	int order;
	unsigned long buffer;
	unsigned long map, mapend;
	struct hal2_ring_buf *rbuf;
	int i = 0;

	/* FIXME: Clear up the memory leaks here */

	for (order = DMABUF_DEFAULTORDER; order >= DMABUF_MINORDER; order--) {
		buffer = __get_free_pages(GFP_KERNEL, db->buforder);
		if (buffer)
			break;
	}

	if (!buffer)
		return -ENOMEM;

	db->buforder = order;
	db->rawbuf = (void *) buffer;

	/* we have to mark the pages as reserver, otherwise remap_page_range
	 * doesn't do what we want for some */

	mapend = MAP_NR(db->rawbuf + (PAGE_SIZE << db->buforder) - 1);
	for (map = MAP_NR(db->rawbuf); map <= mapend; map++)
		set_bit(PG_reserved, &mem_map[map].flags);

	db->ringbuf = kmalloc(sizeof (struct hal2_ring_buf), GFP_KERNEL);
	if (!db->ringbuf)
		return -ENOMEM;

	rbuf = db->ringbuf;
	rbuf->desc.pbuf = PHYSADDR(buffer + i * PAGE_SIZE);
	rbuf->buf       = KSEG1ADDR(buffer + i * PAGE_SIZE);
	i++;

	while (i++ < (1 << db->buforder)) {
		rbuf->next = kmalloc(sizeof (struct hal2_ring_buf), GFP_KERNEL);
		if (!db->ringbuf)
			return -ENOMEM;

		buffer = get_free_page(GFP_KERNEL);
		if (!buffer)
			return -ENOMEM;

		/* we have to link the physical DMA buffers with phyical
		 * addresses first so that the HPC3 knows what to do */

		rbuf->desc.pnext = PHYSADDR(buffer + i * PAGE_SIZE);

		rbuf = rbuf->next;
		rbuf->buf       = KSEG1ADDR(buffer + i * PAGE_SIZE);
		rbuf->desc.pbuf = PHYSADDR(buffer + i * PAGE_SIZE);
	}

	rbuf->next = db->ringbuf;
	rbuf->desc.pnext = db->ringbuf->desc.pbuf;	/* make it a ring! */

	return 0;
}

static void hal2_reset_adc_ring(struct hal2_state *s)
				
{
	struct hal2_ring_buf *first, *rbuf; 

	first = s->dma_adc.ringbuf;

	for (rbuf = first; rbuf->next != first; rbuf = rbuf->next)
		rbuf->desc.cntinfo = HPCDMA_XIE | PAGE_SIZE;

	rbuf->desc.cntinfo = HPCDMA_XIE | PAGE_SIZE;
}

static void hal2_reset_dac_ring(struct hal2_state *s)
				
{
	struct hal2_ring_buf *first, *rbuf; 

	first = s->dma_dac.ringbuf;

	for (rbuf = first; rbuf->next != first; rbuf = rbuf->next)
		rbuf->desc.cntinfo = HPCDMA_XIE | PAGE_SIZE;

	rbuf->desc.cntinfo = HPCDMA_XIE | PAGE_SIZE;
}

static int hal2_setup_pbus(struct hal2_state *s, struct dmabuf *db, int pbusnr)
{
	struct hpc3_pbus_dmacregs *pbus = &hpc3c0->pbdma0;
	int sampsz, highwater, fifosize;
	unsigned long ctrl;
	unsigned long fifobeg, fifoend;

	db->pbusnr = pbusnr;
	db->pbus = &pbus[pbusnr];

	sampsz = (db->stereo ? 4 : 2);

	/* an audio fifo should be 4 samples long and doubleword aligned,
	   highwater should be half the fifo size */

	fifosize  = (sampsz * 4) >> 3;	/* doublewords */
	highwater = (sampsz * 2) >> 1;	/* halfwords */

	/* DAC is always before ADC in our PBUS DMA FIFO. Reserve space for the
	 * maximum fifo length, we might change to stereo later if we're in mono
	 * now
	 */

	fifobeg = (db->pbusnr == 1) ? 0 : (4 * 4) >> 3;

	fifoend = fifobeg + fifosize;

	ctrl = HPC3_PDMACTRL_RT | (db->receive ? HPC3_PDMACTRL_RCV : 0) |
		(db->little_end ? HPC3_PDMACTRL_SEL : 0);

	pbus->pbdma_ctrl = (highwater << 8) | (fifobeg << 16) |
		(fifoend << 24) | ctrl;

	/* Realtime, 16-bit, cycles to spend and more default settings for
	 * soundcards, taken directly from the spec. */
	hpc3c0->pbus_dmacfgs[db->pbusnr][0] = 0x8248844;

	return 0;
}

static int hal2_prog_dmabuf(struct hal2_state *s, struct dmabuf *db,
			    unsigned rate)
{
	unsigned bytespersec;
	unsigned bufs;
	int err;

	db->hwptr = db->swptr = db->total_bytes = db->count = db->error =
		db->endcleared = 0;

	if (!db->rawbuf) {
		db->ready = db->mapped = 0;
		err = hal2_setup_ring(db);
		if (err)
			return err;
	}

	bytespersec = rate << sample_shift[db->stereo];
	bufs = PAGE_SIZE << db->buforder;
	if (db->ossfragshift) {
		if ((1000 << db->ossfragshift) < bytespersec)
			db->fragshift = ld2(bytespersec/1000);
		else
			db->fragshift = db->ossfragshift;
	} else {
		db->fragshift = ld2(bytespersec/100/(db->subdivision ? db->subdivision : 1));
		if (db->fragshift < 3)
			db->fragshift = 3;
	}
	db->numfrag = bufs >> db->fragshift;
	while (db->numfrag < 4 && db->fragshift > 3) {
		db->fragshift--;
		db->numfrag = bufs >> db->fragshift;
	}
	db->fragsize = 1 << db->fragshift;
	if (db->ossmaxfrags >= 4 && db->ossmaxfrags < db->numfrag)
		db->numfrag = db->ossmaxfrags;
	db->fragsamples = db->fragsize >> sample_shift[db->stereo];
	db->dmasize = db->numfrag << db->fragshift;
	memset(db->rawbuf, 0, db->dmasize);
	return 0;
}

static __inline__ int hal2_prog_dmabuf_adc(struct hal2_state *s)
{
	int err;

	hal2_stop_adc(s);

	err = hal2_setup_pbus(s, &s->dma_adc, 1);
	if (err)
		return err;

	err = hal2_prog_dmabuf(s, &s->dma_adc, s->adcrate);
	if (err)
		return err;

	hal2_reset_adc_ring(s);

	hal2_set_adc_rate(s, s->adcrate);

	ireg_write16(H2I_ADC_C1,
		   (0 << H2I_ADC_C1_DMA_SHIFT) |
		   (1 << H2I_ADC_C1_CLKID_SHIFT) |
		   (s->dma_adc.stereo << H2I_ADC_C1_DATAT_SHIFT));
	ireg_setbit16(H2I_DMA_PORT_EN, H2I_DMA_PORT_EN_CODECR);

	ireg_setbit16(H2I_DMA_DRV, (1 << 0));

	s->dma_adc.ready = 1;

	return 0;
}

static __inline__ int hal2_prog_dmabuf_dac(struct hal2_state *s)
{
	int err;

	hal2_stop_adc(s);

	err = hal2_setup_pbus(s, &s->dma_dac, 2);
	if (err)
		return err;

	err = hal2_prog_dmabuf(s, &s->dma_dac, s->dacrate);
	if (err)
		return err;

	hal2_reset_dac_ring(s);

	hal2_set_dac_rate(s, s->dacrate);

	ireg_write32(H2I_DAC_C1,
		   (1 << H2I_DAC_C1_DMA_SHIFT) |
		   (2 << H2I_DAC_C1_CLKID_SHIFT) |
		   (s->dma_dac.stereo << H2I_ADC_C1_DATAT_SHIFT));

	ireg_setbit16(H2I_DMA_PORT_EN, H2I_DMA_PORT_EN_CODECR);

	ireg_setbit16(H2I_DMA_DRV, (1 << 1));

	s->dma_dac.ready = 1;

	return 0;
}

static __inline__ unsigned int hal2_get_hwptr(struct hal2_state *s,
					      struct dmabuf *db)
{
	return db->pbus->pbdma_bptr;
}

extern __inline__ void clear_advance(void *buf, unsigned bsize, unsigned bptr,
				     unsigned len, unsigned char c)
{
	if (bptr + len > bsize) {
		unsigned x = bsize - bptr;
		memset(((char *)buf) + bptr, c, x);
		bptr = 0;
		len -= x;
	}
	memset(((char *)buf) + bptr, c, len);
}

static void hal2_update_ptr(struct hal2_state *s)
{
	int diff;

	/* update ADC pointer */
	if (s->dma_adc.port_en) {
		diff = hal2_get_hwptr(s, &s->dma_adc);
		s->dma_adc.total_bytes += diff;
		s->dma_adc.count += diff;
		if (s->dma_adc.mapped) {
			if (s->dma_adc.count >= s->dma_adc.fragsize) 
				wake_up(&s->dma_adc.wait);
		} else {
			if (s->dma_adc.count > (signed)(s->dma_adc.dmasize - ((3 * s->dma_adc.fragsize) >> 1))) {
				hal2_stop_adc(s);
				s->dma_adc.error++;
			}
			if (s->dma_adc.count > 0)
				wake_up(&s->dma_adc.wait);
		}
	}

	/* update DAC1 pointer */
	if (s->dma_dac.port_en) {
		diff = hal2_get_hwptr(s, &s->dma_dac);
		s->dma_dac.total_bytes += diff;
		if (s->dma_dac.mapped) {
			s->dma_dac.count += diff;
			if (s->dma_dac.count >= (signed)s->dma_dac.fragsize)
				wake_up(&s->dma_dac.wait);
		} else {
			s->dma_dac.count -= diff;
			if (s->dma_dac.count <= 0) {
				hal2_stop_dac(s);
				s->dma_dac.error++;
			} else if (s->dma_dac.count <=
				   (signed)s->dma_dac.fragsize &&
				   !s->dma_dac.endcleared) {
				clear_advance(s->dma_dac.rawbuf,
					      s->dma_dac.dmasize,
					      s->dma_dac.swptr,
					      s->dma_dac.fragsize, 0);
				s->dma_dac.endcleared = 1;
			}
			if (s->dma_dac.count < (signed)s->dma_dac.dmasize)
				wake_up(&s->dma_dac.wait);
		}
	}
}

void hal2_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct hal2_state *s = (struct hal2_state *) dev_id;

	spin_lock(&s->lock);

	if ((s->dma_adc.pbus_en && hpcpbus_int(s->dma_adc.pbus))||
	    (s->dma_dac.pbus_en && hpcpbus_int(s->dma_dac.pbus)))
		hal2_update_ptr(s);

	spin_unlock(&s->lock);
}


static __inline__ void hal2_clear_advance(void *buf, unsigned long bsize,
					unsigned long bptr, unsigned int len,
					unsigned char c)
{
	if (bptr + len > bsize) {
		unsigned long x = bsize - bptr;
		memset(((char *)buf) + bptr, c, x);
		bptr = 0;
		len -= x;
	}
	memset(((char *)buf) + bptr, c, len);
}


/* ----------------------------------------------------------------------- */

static int hal2_drain_dac(struct hal2_state *s, int nonblock)
{
        struct wait_queue wait = { current, NULL };
	unsigned long flags;
	int count, tmo;
	
	if (s->dma_dac.mapped)
		return 0;
        current->state = TASK_INTERRUPTIBLE;
        add_wait_queue(&s->dma_dac.wait, &wait);
        for (;;) {
                spin_lock_irqsave(&s->lock, flags);
		count = s->dma_dac.count;
                spin_unlock_irqrestore(&s->lock, flags);
		if (count <= 0)
			break;
		if (signal_pending(current))
                        break;
                if (nonblock) {
                        remove_wait_queue(&s->dma_dac.wait, &wait);
                        current->state = TASK_RUNNING;
                        return -EBUSY;
                }
		tmo = (count * HZ) / s->dacrate;
		tmo >>= sample_shift[(s->dma_dac.stereo)?0:1];

		if (!schedule_timeout(tmo ? : 1) && tmo)
			printk(KERN_DEBUG ": dma timed out??\n");
        }
        remove_wait_queue(&s->dma_dac.wait, &wait);
        current->state = TASK_RUNNING;
        if (signal_pending(current))
                return -ERESTARTSYS;
        return 0;
}

static ssize_t hal2_read(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	struct hal2_state *s = (struct hal2_state *)file->private_data;
	ssize_t ret;
	unsigned long flags;
	unsigned swptr;
	int cnt;

	if (!s)
		return -ENXIO;
	if (ppos != &file->f_pos)
		return -ESPIPE;
	if (s->dma_adc.mapped)
		return -ENXIO;
	if (!s->dma_adc.ready && (ret = hal2_prog_dmabuf_adc(s)))
		return ret;
	if (!access_ok(VERIFY_WRITE, buffer, count))
		return -EFAULT;
	ret = 0;
	while (count > 0) {
		spin_lock_irqsave(&s->lock, flags);
		swptr = s->dma_adc.swptr;
		cnt = s->dma_adc.dmasize-swptr;
		if (s->dma_adc.count < cnt)
			cnt = s->dma_adc.count;
		spin_unlock_irqrestore(&s->lock, flags);
		if (cnt > count)
			cnt = count;
		if (cnt <= 0) {
			hal2_start_adc(s);
			if (file->f_flags & O_NONBLOCK)
				return ret ? ret : -EBUSY;
			interruptible_sleep_on(&s->dma_adc.wait);
			if (signal_pending(current))
				return ret ? ret : -ERESTARTSYS;
			continue;
		}
		if (copy_to_user(buffer, s->dma_adc.rawbuf + swptr, cnt))
			return ret ? ret : -EFAULT;
		swptr = (swptr + cnt) % s->dma_adc.dmasize;
		spin_lock_irqsave(&s->lock, flags);
		s->dma_adc.swptr = swptr;
		s->dma_adc.count -= cnt;
		spin_unlock_irqrestore(&s->lock, flags);
		count -= cnt;
		buffer += cnt;
		ret += cnt;
		hal2_start_adc(s);
	}
	return ret;
}

static ssize_t hal2_write(struct file *file, const char *buffer, size_t count,
			  loff_t *ppos)
{
	struct hal2_state *s = (struct hal2_state *)file->private_data;
	ssize_t ret;
	unsigned long flags;
	unsigned swptr;
	int cnt;

	if (!s)
		return -ENXIO;
	if (ppos != &file->f_pos)
		return -ESPIPE;
	if (s->dma_dac.mapped)
		return -ENXIO;
	if (!s->dma_dac.ready && (ret = hal2_prog_dmabuf_dac(s)))
		return ret;
	if (!access_ok(VERIFY_READ, buffer, count))
		return -EFAULT;
	ret = 0;
	while (count > 0) {
		spin_lock_irqsave(&s->lock, flags);
		if (s->dma_dac.count < 0) {
			s->dma_dac.count = 0;
			s->dma_dac.swptr = s->dma_dac.hwptr;
		}
		swptr = s->dma_dac.swptr;
		cnt = s->dma_dac.dmasize-swptr;
		if (s->dma_dac.count + cnt > s->dma_dac.dmasize)
			cnt = s->dma_dac.dmasize - s->dma_dac.count;
		spin_unlock_irqrestore(&s->lock, flags);
		if (cnt > count)
			cnt = count;
		if (cnt <= 0) {
			hal2_start_dac(s);
			if (file->f_flags & O_NONBLOCK)
				return ret ? ret : -EBUSY;
			interruptible_sleep_on(&s->dma_dac.wait);
			if (signal_pending(current))
				return ret ? ret : -ERESTARTSYS;
			continue;
		}
		if (copy_from_user(s->dma_dac.rawbuf + swptr, buffer, cnt))
			return ret ? ret : -EFAULT;
		swptr = (swptr + cnt) % s->dma_dac.dmasize;
		spin_lock_irqsave(&s->lock, flags);
		s->dma_dac.swptr = swptr;
		s->dma_dac.count += cnt;
		s->dma_dac.endcleared = 0;
		spin_unlock_irqrestore(&s->lock, flags);
		count -= cnt;
		buffer += cnt;
		ret += cnt;
		hal2_start_dac(s);
	}
	return ret;
}


static unsigned int hal2_poll(struct file *file, struct poll_table_struct *wait)
{
	struct hal2_state *s = (struct hal2_state *)file->private_data;
	unsigned long flags;
	unsigned int mask = 0;

	if (!s)
		return -EINVAL;
	if (file->f_flags & FMODE_WRITE)
		poll_wait(file, &s->dma_dac.wait, wait);
	if (file->f_flags & FMODE_READ)
		poll_wait(file, &s->dma_adc.wait, wait);
	spin_lock_irqsave(&s->lock, flags);
	hal2_update_ptr(s);
	if (file->f_flags & FMODE_READ) {
		if (s->dma_adc.mapped) {
			if (s->dma_adc.count >= (signed)s->dma_adc.fragsize)
				mask |= POLLIN | POLLRDNORM;
		} else {
			if (s->dma_adc.count > 0)
				mask |= POLLIN | POLLRDNORM;
		}
	}
	if (file->f_flags & FMODE_WRITE) {
		if (s->dma_dac.mapped) {
			if (s->dma_dac.count >= (signed)s->dma_dac.fragsize) 
				mask |= POLLOUT | POLLWRNORM;
		} else {
			if ((signed)s->dma_dac.dmasize > s->dma_dac.count)
				mask |= POLLOUT | POLLWRNORM;
		}
	}
	spin_unlock_irqrestore(&s->lock, flags);
	return mask;
}

static int hal2_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct hal2_state *s = (struct hal2_state *)file->private_data;
	struct dmabuf *db;
	int ret;
	unsigned long size;

	if (!s)
		return -EINVAL;
	if (vma->vm_flags & VM_WRITE) {
		if ((ret = hal2_prog_dmabuf_dac(s)) != 0)
			return ret;
		db = &s->dma_dac;
	} else if (vma->vm_flags & VM_READ) {
		if ((ret = hal2_prog_dmabuf_adc(s)) != 0)
			return ret;
		db = &s->dma_adc;
	} else 
		return -EINVAL;
	if (vma->vm_offset != 0)
		return -EINVAL;
	size = vma->vm_end - vma->vm_start;
	if (size > (PAGE_SIZE << db->buforder))
		return -EINVAL;
	if (remap_page_range(vma->vm_start, PHYSADDR(db->rawbuf), size, vma->vm_page_prot))
		return -EAGAIN;
	db->mapped = 1;
	vma->vm_file = file;
	file->f_count++;
	return 0;
}

static int hal2_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		      unsigned long arg)
{
	struct hal2_state *s = (struct hal2_state *)file->private_data;
	unsigned long flags;
        audio_buf_info abinfo;
        count_info cinfo;
	int val, mapped, ret;

	if (!s)
		return -EINVAL;
        mapped = ((file->f_mode & FMODE_WRITE) && s->dma_dac.mapped) ||
		((file->f_mode & FMODE_READ) && s->dma_adc.mapped);
	switch (cmd) {
	case OSS_GETVERSION:
		return put_user(SOUND_VERSION, (int *)arg);

	case SNDCTL_DSP_SYNC:
		if (file->f_mode & FMODE_WRITE)
			/* XXX check what drain really should do */
			return hal2_drain_dac(s, 0/*file->f_flags & O_NONBLOCK*/);
		return 0;
		
	case SNDCTL_DSP_SETDUPLEX:
		return 0;

	case SNDCTL_DSP_GETCAPS:
		/* XXX do we really support trigger? */
		return put_user(DSP_CAP_DUPLEX | DSP_CAP_REALTIME |
				DSP_CAP_TRIGGER | DSP_CAP_MMAP, (int *)arg);
		
        case SNDCTL_DSP_RESET:
		if (file->f_mode & FMODE_WRITE) {
			hal2_stop_dac(s);
			synchronize_irq();
			s->dma_dac.swptr = s->dma_dac.hwptr = s->dma_dac.count = s->dma_dac.total_bytes = 0;
		}
		if (file->f_mode & FMODE_READ) {
			hal2_stop_adc(s);
			synchronize_irq();
			s->dma_adc.swptr = s->dma_adc.hwptr = s->dma_adc.count = s->dma_adc.total_bytes = 0;
		}
		return 0;

        case SNDCTL_DSP_SPEED:
                get_user_ret(val, (int *)arg, -EFAULT);
		if (val >= 0) {
			if (file->f_mode & FMODE_READ) {
				hal2_stop_adc(s);
				s->dma_adc.ready = 0;
				hal2_set_adc_rate(s, val);
			}
			if (file->f_mode & FMODE_WRITE) {
				hal2_stop_dac(s);
				s->dma_dac.ready = 0;
				hal2_set_dac_rate(s, val);
			}
		}
		return put_user((file->f_mode & FMODE_READ) ? s->adcrate : s->dacrate, (int *)arg);

        case SNDCTL_DSP_STEREO:
		get_user_ret(val, (int *)arg, -EFAULT);
		if (file->f_mode & FMODE_READ) {
			hal2_stop_adc(s);
			s->dma_adc.ready = 0;
			spin_lock_irqsave(&s->lock, flags);
			if (val) {
				ireg_setmask16(H2I_ADC_C1,
					       H2I_ADC_C1_DATAT_SHIFT,
					       H2I_ADC_C1_DATAT_M, 2);
				s->dma_adc.stereo = 1;
			} else {
				ireg_setmask16(H2I_ADC_C1,
					       H2I_ADC_C1_DATAT_SHIFT,
					       H2I_ADC_C1_DATAT_M, 1);
				s->dma_adc.stereo = 0;
			}
			spin_unlock_irqrestore(&s->lock, flags);
		}
		if (file->f_mode & FMODE_WRITE) {
			hal2_stop_dac(s);
			s->dma_dac.ready = 0;
			spin_lock_irqsave(&s->lock, flags);
			if (val) {
				ireg_setmask16(H2I_DAC_C1,
					       H2I_DAC_C1_DATAT_SHIFT,
					       H2I_DAC_C1_DATAT_M, 2);
				s->dma_dac.stereo = 1;
			} else {
				ireg_setmask16(H2I_DAC_C1,
					       H2I_DAC_C1_DATAT_SHIFT,
					       H2I_DAC_C1_DATAT_M, 1);
				s->dma_dac.stereo = 0;
			}
			spin_unlock_irqrestore(&s->lock, flags);
                }
		return 0;

        case SNDCTL_DSP_CHANNELS:
                get_user_ret(val, (int *)arg, -EFAULT);
		if (val != 0) {
			if (file->f_mode & FMODE_READ) {
				hal2_stop_adc(s);
				s->dma_adc.ready = 0;
				spin_lock_irqsave(&s->lock, flags);
				if (val >= 2) {
					ireg_setmask16(H2I_ADC_C1,
						       H2I_ADC_C1_DATAT_SHIFT,
						       H2I_ADC_C1_DATAT_M, 2);
					s->dma_dac.stereo = 1;
				} else {
					ireg_setmask16(H2I_ADC_C1,
						       H2I_ADC_C1_DATAT_SHIFT,
						       H2I_ADC_C1_DATAT_M, 1);
					s->dma_adc.stereo = 0;
				}
				spin_unlock_irqrestore(&s->lock, flags);
			}
			if (file->f_mode & FMODE_WRITE) {
				hal2_stop_dac(s);
				s->dma_dac.ready = 0;
				spin_lock_irqsave(&s->lock, flags);
				if (val >= 2) {
					ireg_setmask16(H2I_DAC_C1,
						       H2I_DAC_C1_DATAT_SHIFT,
						       H2I_DAC_C1_DATAT_M, 2);
					s->dma_dac.stereo = 1;
				} else {
					ireg_setmask16(H2I_DAC_C1,
						       H2I_DAC_C1_DATAT_SHIFT,
						       H2I_DAC_C1_DATAT_M, 1);
					s->dma_dac.stereo = 0;
				}
				spin_unlock_irqrestore(&s->lock, flags);
			}
		}
		return put_user(s->dma_dac.stereo ? 2 : 1, (int *)arg);
		
	case SNDCTL_DSP_GETFMTS: /* Returns a mask */
                return put_user(AFMT_S16_BE, (int *)arg);
		
	case SNDCTL_DSP_SETFMT: /* Maybe Little endian too */
		return put_user(AFMT_S16_BE, (int *)arg);
		
	case SNDCTL_DSP_POST:
                return 0;

        case SNDCTL_DSP_GETTRIGGER:
		val = 0;
		if (file->f_mode & FMODE_READ && s->dma_adc.port_en) 
			val |= PCM_ENABLE_INPUT;
		if (file->f_mode & FMODE_WRITE && s->dma_dac.port_en) 
			val |= PCM_ENABLE_OUTPUT;
		return put_user(val, (int *)arg);
		
	case SNDCTL_DSP_SETTRIGGER:
		get_user_ret(val, (int *)arg, -EFAULT);
		if (file->f_mode & FMODE_READ) {
			if (val & PCM_ENABLE_INPUT) {
				if (!s->dma_adc.ready && (ret = hal2_prog_dmabuf_adc(s)))
					return ret;
				hal2_start_adc(s);
			} else
				hal2_stop_adc(s);
		}
		if (file->f_mode & FMODE_WRITE) {
			if (val & PCM_ENABLE_OUTPUT) {
				if (!s->dma_dac.ready && (ret = hal2_prog_dmabuf_dac(s)))
					return ret;
				hal2_start_dac(s);
			} else
				hal2_stop_dac(s);
		}
		return 0;

	case SNDCTL_DSP_GETOSPACE:
		if (!(file->f_mode & FMODE_WRITE))
			return -EINVAL;
		if (!s->dma_dac.port_en && (val = hal2_prog_dmabuf_dac(s)) != 0)
			return val;
		spin_lock_irqsave(&s->lock, flags);
		hal2_update_ptr(s);
		abinfo.fragsize = s->dma_dac.fragsize;
                abinfo.bytes = s->dma_dac.dmasize - s->dma_dac.count;
                abinfo.fragstotal = s->dma_dac.numfrag;
                abinfo.fragments = abinfo.bytes >> s->dma_dac.fragshift;      
		spin_unlock_irqrestore(&s->lock, flags);
		return copy_to_user((void *)arg, &abinfo, sizeof(abinfo)) ? -EFAULT : 0;

	case SNDCTL_DSP_GETISPACE:
		if (!(file->f_mode & FMODE_READ))
			return -EINVAL;
		if (!(s->dma_adc.port_en) && (val = hal2_prog_dmabuf_adc(s)) != 0)
			return val;
		spin_lock_irqsave(&s->lock, flags);
		hal2_update_ptr(s);
		abinfo.fragsize = s->dma_adc.fragsize;
                abinfo.bytes = s->dma_adc.count;
                abinfo.fragstotal = s->dma_adc.numfrag;
                abinfo.fragments = abinfo.bytes >> s->dma_adc.fragshift;      
		spin_unlock_irqrestore(&s->lock, flags);
		return copy_to_user((void *)arg, &abinfo, sizeof(abinfo)) ? -EFAULT : 0;
		
        case SNDCTL_DSP_NONBLOCK:
                file->f_flags |= O_NONBLOCK;
                return 0;

        case SNDCTL_DSP_GETODELAY:
		if (!(file->f_mode & FMODE_WRITE))
			return -EINVAL;
		spin_lock_irqsave(&s->lock, flags);
		hal2_update_ptr(s);
                val = s->dma_dac.count;
		spin_unlock_irqrestore(&s->lock, flags);
		return put_user(val, (int *)arg);

        case SNDCTL_DSP_GETIPTR:
		if (!(file->f_mode & FMODE_READ))
			return -EINVAL;
		spin_lock_irqsave(&s->lock, flags);
		hal2_update_ptr(s);
                cinfo.bytes = s->dma_adc.total_bytes;
                cinfo.blocks = s->dma_adc.total_bytes >> s->dma_adc.fragshift;
                cinfo.ptr = s->dma_adc.hwptr;
		if (s->dma_adc.mapped)
			s->dma_adc.count &= s->dma_adc.fragsize-1;
		spin_unlock_irqrestore(&s->lock, flags);
                return copy_to_user((void *)arg, &cinfo, sizeof(cinfo));

        case SNDCTL_DSP_GETOPTR:
		if (!(file->f_mode & FMODE_WRITE))
			return -EINVAL;
		spin_lock_irqsave(&s->lock, flags);
		hal2_update_ptr(s);
                cinfo.bytes = s->dma_dac.total_bytes;
                cinfo.blocks = s->dma_dac.total_bytes >> s->dma_dac.fragshift;
                cinfo.ptr = s->dma_dac.hwptr;
		if (s->dma_dac.mapped)
			s->dma_dac.count &= s->dma_dac.fragsize-1;
		spin_unlock_irqrestore(&s->lock, flags);
                return copy_to_user((void *)arg, &cinfo, sizeof(cinfo));

        case SNDCTL_DSP_GETBLKSIZE:
		if (file->f_mode & FMODE_WRITE) {
			if ((val = hal2_prog_dmabuf_dac(s)))
				return val;
			return put_user(s->dma_dac.fragsize, (int *)arg);
		}
		if ((val = hal2_prog_dmabuf_adc(s)))
			return val;
		return put_user(s->dma_adc.fragsize, (int *)arg);

        case SNDCTL_DSP_SETFRAGMENT:
                get_user_ret(val, (int *)arg, -EFAULT);
		if (file->f_mode & FMODE_READ) {
			s->dma_adc.ossfragshift = val & 0xffff;
			s->dma_adc.ossmaxfrags = (val >> 16) & 0xffff;
			if (s->dma_adc.ossfragshift < 4)
				s->dma_adc.ossfragshift = 4;
			if (s->dma_adc.ossfragshift > 15)
				s->dma_adc.ossfragshift = 15;
			if (s->dma_adc.ossmaxfrags < 4)
				s->dma_adc.ossmaxfrags = 4;
		}
		if (file->f_mode & FMODE_WRITE) {
			s->dma_dac.ossfragshift = val & 0xffff;
			s->dma_dac.ossmaxfrags = (val >> 16) & 0xffff;
			if (s->dma_dac.ossfragshift < 4)
				s->dma_dac.ossfragshift = 4;
			if (s->dma_dac.ossfragshift > 15)
				s->dma_dac.ossfragshift = 15;
			if (s->dma_dac.ossmaxfrags < 4)
				s->dma_dac.ossmaxfrags = 4;
		}
		return 0;

        case SNDCTL_DSP_SUBDIVIDE:
		if ((file->f_mode & FMODE_READ && s->dma_adc.subdivision) ||
		    (file->f_mode & FMODE_WRITE && s->dma_dac.subdivision))
			return -EINVAL;
                get_user_ret(val, (int *)arg, -EFAULT);
		if (val != 1 && val != 2 && val != 4)
			return -EINVAL;
		if (file->f_mode & FMODE_READ)
			s->dma_adc.subdivision = val;
		if (file->f_mode & FMODE_WRITE)
			s->dma_dac.subdivision = val;
		return 0;

        case SOUND_PCM_WRITE_FILTER:
        case SNDCTL_DSP_SETSYNCRO:
        case SOUND_PCM_READ_RATE:
        case SOUND_PCM_READ_CHANNELS:
        case SOUND_PCM_READ_BITS:
        case SOUND_PCM_READ_FILTER:
                return -EINVAL;
		
	}
	return -EINVAL;
}

static int hal2_open(struct inode *inode, struct file *file)
{
	struct hal2_state *s = dev;

	if (!s)
		return -ENODEV;
	file->private_data = s;
	/* wait for device to become free */
	down(&s->open_sem);
	while (s->open_mode & file->f_mode) {
		if (file->f_flags & O_NONBLOCK) {
			up(&s->open_sem);
			return -EBUSY;
		}
		up(&s->open_sem);
		interruptible_sleep_on(&s->open_wait);
		if (signal_pending(current))
			return -ERESTARTSYS;
		down(&s->open_sem);
	}
	if (file->f_mode & FMODE_READ) {
		s->dma_adc.ossfragshift = s->dma_adc.ossmaxfrags = s->dma_adc.subdivision = 0;
		hal2_set_adc_rate(s, 8000);
	}
	if (file->f_mode & FMODE_WRITE) {
		s->dma_dac.ossfragshift = s->dma_dac.ossmaxfrags = s->dma_dac.subdivision = 0;
		hal2_set_dac_rate(s, 8000);
	}
	s->open_mode |= file->f_mode & (FMODE_READ | FMODE_WRITE);
	up(&s->open_sem);
	MOD_INC_USE_COUNT;
	return 0;
}

static int hal2_release(struct inode *inode, struct file *file)
{
	struct hal2_state *s = (struct hal2_state *)file->private_data;

	if (!s)
		return -ENODEV;
	if (file->f_mode & FMODE_WRITE)
		hal2_drain_dac(s, file->f_flags & O_NONBLOCK);
	down(&s->open_sem);
	if (file->f_flags & FMODE_WRITE) {
		hal2_stop_dac(s);
		hal2_dealloc_dmabuf(&s->dma_dac);
	}
	if (file->f_flags & FMODE_READ) {
		hal2_stop_adc(s);
		hal2_dealloc_dmabuf(&s->dma_adc);
	}
	s->open_mode &= (~file->f_mode) & (FMODE_READ|FMODE_WRITE);
	up(&s->open_sem);
	wake_up(&s->open_wait);
	MOD_DEC_USE_COUNT;
	return 0;
}

static struct file_operations hal2_audio_fops = {
	NULL,	/* hal2_llseek */
	hal2_read,
	hal2_write,
	NULL,	/* hal2_readdir */
	hal2_poll,
	hal2_ioctl,
	hal2_mmap,
	hal2_open,
	NULL,	/* hal2_flush */
	hal2_release,
	NULL,	/* hal2_fsync */
	NULL,	/* hal2_fasync */
	NULL,	/* hal2_check_media_change */
	NULL,	/* hal2_revalidate */
	NULL,	/* hal2_lock */
};

/* ----------------------------------------------------------------------- */

static void hal2_reset(void)
{
	printk("resetting global isr:%04hx\n", h2_ctrl->isr);
#if 0
	/* try to leave the card in the bootup state so that the mixer stuff
	 * isn't gone */
	h2_ctrl->isr = 0;		/* reset the card */
#endif
	printk("reset done isr:%04hx\n", h2_ctrl->isr);
	h2_ctrl->isr = H2_ISR_GLOBAL_RESET_N | H2_ISR_CODEC_RESET_N;
	printk("reactivation done isr:%04hx\n", h2_ctrl->isr);
}

static int hal2_probe(void)
{
	unsigned short board, major, minor;

	unsigned long tmp;

	hal2_reset();

	if (h2_ctrl->rev & H2_REV_AUDIO_PRESENT) {

		printk("hal2: there was no device?\n");
		return -ENODEV;
	}
	
	board = (h2_ctrl->rev & H2_REV_BOARD_M) >> 12;
	major = (h2_ctrl->rev & H2_REV_MAJOR_CHIP_M) >> 4;
	minor = (h2_ctrl->rev & H2_REV_MINOR_CHIP_M);

	printk("SGI H2 Processor, Revision %i.%i.%i\n",
	       board, major, minor);

	if (board != 4 || major != 1 || minor != 0) {
		printk("hal2: Other revision than 4.1.0 detected\n");
		printk("hal2: Your card is probably not supported\n");
	}

#ifdef DEBUG
	printk("hal2: probing card\n");
#endif

	/* check that the indirect registers are working by writing some bogus
	 * stuff and then reading it back */

	ireg_write16(H2I_DAC_C1, 0x4084);		/* 16 bit register */
	ireg_write32(H2I_BRES2_C2, 0x20C0084);		/* 32 bit register */

#ifdef DEBUG
	printk("hal2: write done\n");
#endif

	if ((tmp = ireg_read16(H2I_DAC_C1)) != 0x4084) {
		printk("hal2: Didn't pass register check #1 (%04lx)\n", tmp);
		return -ENODEV;
	}
	if ((tmp = ireg_read32(H2I_BRES2_C2)) != 0x20C0084) {
		printk("hal2: Didn't pass register check #2 (%08lx)\n", tmp);
		return -ENODEV;
	}

#ifdef DEBUG
	printk("hal2: read done\n");
#endif
	printk("hal2: card found\n");

	return 0;
}

static int hal2_init(void)
{
	static int initialized = 0;

	if (initialized) {
		printk("hal2: already initialized?\n");
		return 0;
	}

	if (!dev) {
#ifdef DEBUG
		printk("hal2: allocating hal2 device pointer\n");
#endif
		dev = kmalloc(sizeof(struct hal2_state), GFP_KERNEL);
		if (!dev)
			return -ENOMEM;
		memset(dev, 0, sizeof(struct hal2_state));
	}

#ifdef DEBUG
	printk("hal2: resetting chip\n");
#endif
	hal2_reset();

	/* setup indigo codec mode */

#ifdef DEBUG
	printk("hal2: init done\n");
#endif

	initialized = 1;

	return 0;
}


#ifdef MODULE
__initfunc(int init_module(void))
#else
__initfunc(int init_hal2(void))
#endif
{
	struct hal2_state *s;
	int err;

	err = hal2_probe();
	if (err)
		return err;

	err = hal2_init();
	if (err)
		return err;

	s = dev;

	memset(s, 0, sizeof(struct hal2_state));
	init_waitqueue(&s->dma_adc.wait);
	init_waitqueue(&s->dma_dac.wait);
	init_waitqueue(&s->open_wait);
	s->open_sem = MUTEX;

	/* Indigo CODEC mode (!QUAD) */
	h2_ctrl->isr &= ~H2_ISR_QUAD_MODE;

	s->irq = 12;	/* wild guess! */
	err = request_irq(s->irq, hal2_interrupt, SA_INTERRUPT, "hal2", s);
	if (err) {
		printk(KERN_ERR "hal2: irq %u in use\n", s->irq);
		return err;
	}
	s->dev_audio = register_sound_dsp(&hal2_audio_fops);
	if (s->dev_audio < 0)
		printk(KERN_ERR "hal2: cannot register misc device\n");

	hal2_set_adc_rate(s, 22050);
	hal2_set_dac_rate(s, 22050);

	return 0;
}

#ifdef MODULE

void cleanup_module(void)
{
	struct hal2_state *s = dev;

	synchronize_irq();
	free_irq(s->irq, s);
	unregister_sound_dsp(s->dev_audio);
	kfree_s(s, sizeof(struct hal2_state));
	printk(KERN_INFO "hal2: unloading\n");
}

#endif
