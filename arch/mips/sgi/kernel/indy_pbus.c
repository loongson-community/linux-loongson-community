/* $Id$
 *
 * indy_pbus.c: Routines for manipulation of the HPC PBUS DMA controller.
 *
 * Copyright (C) 1998 Ulf Carlsson
 */

#define PBUS_DMA_RAM		384	/* Bytes of PBUS DMA ram */

#include <linux/init.h>
#include <linux/slab.h>

#include <asm/addrspace.h>
#include <asm/sgihpc.h>
#include <asm/indy_pbus.h>

struct pbus_chan {
	int  lock;
	const char *device_id;
};

static struct pbus_chan pbus_chan_busy[MAX_PBUS_CHANNELS] = {
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
};

struct pbus_fifo {
	struct pbus_fifo *next;

	int channel;
	int beg;
	int size;
};

struct pbus_fifo pbus_fifo_busy = {
	NULL,
	-1,
	0,
	0
};
	

int sgipbus_get_list(char *buf)
{
        int i, len = 0;

	for (i = 0 ; i < MAX_PBUS_CHANNELS ; i++) {
		if (pbus_chan_busy[i].lock) {
			len += sprintf(buf+len, "%2d: %s\n",
					i, pbus_chan_busy[i].device_id);
		}
	}
	return len;
}

int sgipbus_request(unsigned int channel, char *device_id)
{
	if (channel > MAX_PBUS_CHANNELS)
		return -EINVAL;

	if (xchg(&pbus_chan_busy[channel].lock, 1) != 0)
		return -EBUSY;

	pbus_chan_busy[channel].device_id = device_id;

	/* old flag was 0, it's now 1 to indicate busy */
	return 0;
}

static int sgipbus_fifo_alloc(unsigned int channel, int fifo_size)
{
	struct pbus_fifo *p, *tmp;
	int beg;
	
	if (!pbus_chan_busy[channel].lock)
		return -EINVAL;
	
	/* fifo_size is in 64 bit words */
	if (fifo_size > PBUS_DMA_RAM / 8)
		goto no_space;

	for (p = &pbus_fifo_busy; p; p = p->next) {
		int used = p->beg + p->size;
		if (PBUS_DMA_RAM / 8 - used > fifo_size) {
			if (p->next && p->next->beg - used <= fifo_size)
				continue;

			beg = used + 1;
			goto found_space;
		}
	}

no_space:

	printk("No memory left for PBUS device %s\n",
	       pbus_chan_busy[channel].device_id);
	return -EBUSY;

found_space:
	tmp = p->next;
	p->next = (struct pbus_fifo *)
		kmalloc(sizeof(struct pbus_fifo), GFP_KERNEL);
	if (!p->next)
		return -ENOMEM;

	p->next->next = tmp;

	p = p->next;
	p->beg = beg;
	p->size = fifo_size;
	p->channel = channel;

	return p->beg;
}

static void sgipbus_fifo_free(unsigned int channel)
{
	struct pbus_fifo *p, *tmp;

	for (p = &pbus_fifo_busy ; p->next ; p = p->next) {
		if (p->next->channel == channel)
			goto out;
	}

	return;	/* nothing to free? */
out:
	tmp = p->next;
	p->next = p->next->next;
	kfree(tmp);
}

/*
 * Setup the pbus channel
 * fifo_size in doublewords (64 bit) , highwater in halfwords (16 bit)
 */
int sgipbus_setup(unsigned int channel, int fifo_size,
		  unsigned long highwater, unsigned long flags,
		  unsigned long dmacfg)
{
	struct hpc3_pbus_dmacregs *pbus = &hpc3c0->pbdma0;
	unsigned int fifobeg, fifoend;
	int ret;

	ret = sgipbus_fifo_alloc(channel, fifo_size);
	if (ret < 0)
		return ret;
	fifobeg = ret;
	fifoend = fifobeg + fifo_size - 1;

	pbus[channel].pbdma_ctrl = (highwater << 8) | (fifobeg << 16) |
		(fifoend << 24) | flags;

	return 0;
}

void sgipbus_free(unsigned int channel)
{
	if (channel > MAX_PBUS_CHANNELS) {
		printk("Trying to free PBUS %d\n", channel);
		return;
	}

	if (xchg(&pbus_chan_busy[channel].lock, 0) == 0) {
		printk("Trying to free free PBUS %d\n", channel);
		return;
	}

	sgipbus_fifo_free(channel);
}

/* enable/disable a specific PBUS dma channel */
__inline__ void sgipbus_enable(unsigned int channel, unsigned long desc)
{
	struct hpc3_pbus_dmacregs *pbus = &hpc3c0->pbdma0;

	pbus[channel].pbdma_dptr = desc;
	pbus[channel].pbdma_ctrl |= HPC3_PDMACTRL_ACT;
}

__inline__ void sgipbus_disable(unsigned int channel)
{
	struct hpc3_pbus_dmacregs *pbus = &hpc3c0->pbdma0;

	pbus[channel].pbdma_ctrl &= ~HPC3_PDMACTRL_ACT;
}

__inline__ int sgipbus_interrupted(unsigned int channel)
{
	struct hpc3_pbus_dmacregs *pbus = &hpc3c0->pbdma0;

	/* When we read pbdma_ctrl, bit 0 indicates interrupt signal.
	 * The interrupt signal is also cleared after read
	 */

	return (pbus[channel].pbdma_ctrl & 0x01);
}
