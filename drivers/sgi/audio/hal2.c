/* $Id: hal2.c,v 1.4 1999/01/27 21:13:01 ulfc Exp $
 * 
 * drivers/sgi/audio/hal2.c
 *
 * Copyright (C) 1998-1999 Ulf Carlsson (ulfc@bun.falkenberg.se)
 * 
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/page.h>
#include <asm/io.h>
#include <linux/errno.h>

#include <linux/soundcard.h>
#include <asm/sgihpc.h>
#include <asm/indy_pbus.h>

#include "hal2.h"

#define DEBUG

struct hal2_buffer {
	struct hpc_dma_desc desc;

	struct hal2_buffer *next;
	unsigned long buf;		/* pointer in KSEG1 address space */
};

/* As you might know, dma descriptors must be 8 byte aligned, that's why we have
 * them in this special way.
 */

struct hal2_channel {
	struct hal2_buffer *ring;

	int pbus;
	int stereo;
	int little_end;
	int bres_mod;
	int bres_master;		/* 0=48.0k 1=44.1k */

	enum {DAC, ADC} type;

	int pbus_requested;
	int pbus_enabled;
	int hal2_enabled;

	int bufs;
	int free_bufs;
};

struct hal2_private {
	struct hal2_buffer *dac_head;
	struct hal2_buffer *adc_tail;

	int dac_underrun;

	unsigned long empty_buf;	/* in case of underrun */

	struct hal2_channel dac_chan;
	struct hal2_channel adc_chan;
};

struct sgiaudio_chan {
	int started;
	int rp;
	int wp;
	void *buf;
	int bufsz;			/* multiple of PAGE_SIZE */

	struct sgiaudio_chan_ops *ops;

	struct wait_queue *queue;
};

struct sgiaudio {
	struct sgiaudio_chan dac;
	struct sgiaudio_chan adc;

	void *private;			/* for internal hal2 usage .. */
};

struct sgiaudio_chan_ops {
	int (*init) (struct sgiaudio *);
	int (*stereo) (struct sgiaudio *, int);
	int (*endian) (struct sgiaudio *, int);
	int (*freq) (struct sgiaudio *, double);
	int (*configure) (struct sgiaudio *);
	void (*start) (struct sgiaudio *);
	void (*stop) (struct sgiaudio *);
	void (*free) (struct sgiaudio *);
};

#if 0
#define INDIRECT_WAIT(regs) while(regs->isr & H2_ISR_TSTATUS);
#endif

#define INDIRECT_WAIT(regs)							\
{										\
	int cnt = 1000;								\
	udelay(1000);								\
	printk("hal2: waiting isr:%04hx ", regs->isr);				\
	printk("idr0:%04hx idr1:%04hx idr2:%04hx idr3:%04hx\n",			\
	       regs->idr0, regs->idr1, regs->idr2, regs->idr3);			\
										\
	while(regs->isr & H2_ISR_TSTATUS && --cnt)				\
		udelay(1000);							\
	if (!cnt)								\
		printk("hal2: failed while waiting for indirect trans.\n");	\
										\
	printk("hal2: finished waiting at cnt:%d isr:%04hx ", cnt, regs->isr);	\
	printk("idr0:%04hx idr1:%04hx idr2:%04hx idr3:%04hx\n",			\
	       regs->idr0, regs->idr1, regs->idr2, regs->idr3);			\
	udelay(1000);								\
}										\

static unsigned short ireg_read(unsigned short address)
{
	unsigned short tmp;

	h2_ctrl->iar = address;
	INDIRECT_WAIT(h2_ctrl)
	tmp = h2_ctrl->idr0;
	return tmp;
}

static void ireg_write(unsigned short address, unsigned short val)
{
	h2_ctrl->idr0 = val;
	udelay(1000);
	h2_ctrl->iar = address;
	INDIRECT_WAIT(h2_ctrl)
}

static void ireg_write2(unsigned short address, unsigned short val0, unsigned
			short val1)
{
	h2_ctrl->idr0 = val0;
	h2_ctrl->idr1 = val1;
	udelay(1000);
	h2_ctrl->iar = address;
	INDIRECT_WAIT(h2_ctrl)
}

static void ireg_write4(unsigned short address, unsigned short val0, unsigned
			short val1, unsigned short val2, unsigned short val3)
{
	h2_ctrl->idr0 = val0;
	h2_ctrl->idr1 = val1;
	h2_ctrl->idr2 = val2;
	h2_ctrl->idr3 = val3;
	udelay(1000);
	h2_ctrl->iar = address;
	INDIRECT_WAIT(h2_ctrl)
}

static void ireg_setbit(unsigned short write_address, unsigned short
			read_address, int bit)
{
	int tmp;

	h2_ctrl->iar = read_address;
	INDIRECT_WAIT(h2_ctrl);
	tmp = h2_ctrl->idr0;
	h2_ctrl->idr0 = tmp | bit;
	udelay(1000);
	h2_ctrl->iar = write_address;
	INDIRECT_WAIT(h2_ctrl);
}

static void ireg_clearbit(unsigned short write_address, unsigned short
			  read_address, int bit)
{
	int tmp;

	h2_ctrl->iar = read_address;
	INDIRECT_WAIT(h2_ctrl);
	tmp = h2_ctrl->idr0;
	h2_ctrl->idr0 = tmp & ~bit;
	udelay(1000);
	h2_ctrl->iar = write_address;
	INDIRECT_WAIT(h2_ctrl);
}

static void hal2_reset(void)
{
#if 0
	h2_ctrl->isr &= ~H2_ISR_CODEC_RESET_N;
	h2_ctrl->isr |= H2_ISR_CODEC_RESET_N;
#endif
	h2_ctrl->isr &= ~H2_ISR_GLOBAL_RESET_N;		/* reset the card */
	udelay(1000);
	h2_ctrl->isr |= H2_ISR_GLOBAL_RESET_N;		/* and reactivate it */
	udelay(1000);
}

static int hal2_probe(void)
{
	unsigned short board, major, minor;

	unsigned short tmp;

	hal2_reset();

	if (h2_ctrl->rev & H2_REV_AUDIO_PRESENT) {

		printk("hal2: there was no device?\n");
		return -ENODEV;
	}
	board = (h2_ctrl->rev & H2_REV_BOARD_M) >> 12;
	major = (h2_ctrl->rev & H2_REV_MAJOR_CHIP_M) >> 4;
	minor = (h2_ctrl->rev & H2_REV_MINOR_CHIP_M);

	printk("SGI HAL2 Processor, Revision %i.%i.%i\n",
	       board, major, minor);

	if (board != 4 || major != 1 || minor != 0) {
		printk("hal2: Other revision than 4.1.0 detected\n");
		printk("hal2: Your card is probably not supported\n");
	}

#ifdef DEBUG
	printk("hal2: checking registers\n");
#endif

	/* check that the indirect registers are working by writing some bogus
	 * stuff and then reading it back */

	ireg_write(H2IW_DAC_C1, 0x123);			/* 16 bit register */
	printk("hal2: wrote #1\n");
	ireg_write2(H2IW_BRES2_C2, 0x132, 0x231);	/* 32 bit register */
	printk("hal2: wrote #2\n");

	if ((tmp = ireg_read(H2IR_DAC_C1)) != 0x123) {
		printk("hal2: Didn't pass register check #1 (%04hx)\n", tmp);
		return -ENODEV;
	}
	printk("hal2: read #1\n");
	if ((tmp = ireg_read(H2IR_BRES2_C2_0)) != 0x132) {
		printk("hal2: Didn't pass register check #2 (%04hx)\n", tmp);
		return -ENODEV;
	}
	printk("hal2: read #2\n");
	if ((tmp = ireg_read(H2IR_BRES2_C2_1)) != 0x231) {
		printk("hal2: Didn't pass register check #3 (%04hx)\n", tmp);
		return -ENODEV;
	}
	printk("hal2: read #3\n");

#ifdef DEBUG
	printk("hal2: card found\n");
#endif


#if 0
	hp->volume = (struct hal2_volume_regs *)
		&hpc3c0->pbus_extregs[H2_VOLUME_PIO][0];
#endif

	return 0;
}

static int hal2_init(struct sgiaudio *sa)
{
	static int initialized = 0;

	if (initialized) {
		printk("hal2_init: already initialized?\n");
		goto out;
	}

	if (!sa->private) {
#ifdef DEBUG
		printk("hal2: allocating private hal2 pointer\n");
#endif
		sa->private = kmalloc(sizeof(struct hal2_private), GFP_KERNEL);
		if (!sa->private)
			return -ENOMEM;
		memset(sa->private, 0, sizeof(struct hal2_private));
	}

#ifdef DEBUG
	printk("hal2: resetting chip\n");
#endif

	hal2_reset();

	/* setup indigo codec mode */
	h2_ctrl->isr &= ~H2_ISR_CODEC_MODE;
	printk("hal2 init done..\n");

	initialized = 1;
out:
	return 0;
}

static int hal2_setup_ring(struct hal2_private *hp, struct hal2_channel *chan)
{
	struct hal2_buffer *hbuf;
	unsigned long buffer;
	int j = 0;

	/* FIXME: Clear up the memory leaks here */

	chan->ring = kmalloc(sizeof (struct hal2_buffer), GFP_KERNEL);
	if (!chan->ring)
		return -ENOMEM;

	hbuf = chan->ring;
	buffer = get_free_page(GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	hbuf->desc.pbuf = PHYSADDR(buffer);
	hbuf->buf       = KSEG1ADDR(buffer);

	while (j++ < chan->bufs - 1) {
		hbuf->next = kmalloc(sizeof (struct hal2_buffer), GFP_KERNEL);
		if (!chan->ring)
			return -ENOMEM;

		buffer = get_free_page(GFP_KERNEL);
		if (!buffer)
			return -ENOMEM;

		/* we have to link the physical DMA buffers with phyical
		 * addresses first so that the HPC3 knows what to do */

		hbuf->desc.pnext = PHYSADDR(buffer);

		hbuf = hbuf->next;
		hbuf->buf       = KSEG1ADDR(buffer);
		hbuf->desc.pbuf = PHYSADDR(buffer);
	}

	hbuf->next = chan->ring;
	hbuf->desc.pnext = chan->ring->desc.pbuf;

	return 0;
}

static int hal2_init_dac(struct sgiaudio *sa)
{
	struct hal2_private *hp;
	struct hal2_channel *chan;
	int err;

	printk("initializing dac\n");
	err = hal2_init(sa);
	if (err)
		return err;

	hp = (struct hal2_private *) sa->private;
	chan = &hp->dac_chan;

	err = hal2_setup_ring(hp, chan);
	if (err)
		return err;

	hp->adc_tail = chan->ring;

	return 0;
}

static int hal2_init_adc(struct sgiaudio *sa)
{
	struct hal2_private *hp;
	struct hal2_channel *chan;
	int err;

	printk("initializing adc\n");

	err = hal2_init(sa);
	if (err)
		return err;

	hp = (struct hal2_private *) sa->private;
	chan = &hp->dac_chan;

	err = hal2_setup_ring(hp, chan);
	if (err)
		return err;

	hp->dac_head = chan->ring;

	return 0;
}

static __inline__ int hal2_sample_size(struct hal2_channel *chan)
{
	return (chan->stereo ? 4 : 2);
}

static int hal2_setup_pbus(struct hal2_private *hp, struct hal2_channel *chan)
{
	int sample_sz, highwater, fifosize;
	unsigned long dmacfg;
	unsigned long flags;
	char *device_id;
	char *s1 = "hal2 ";
	char *s2 = (chan->type == ADC ? "adc" : "dac");
	int i;
	int err;

	sample_sz = hal2_sample_size(chan);	/* bytes/sample */

	/* an audio fifo should be 4 samples long and doubleword aligned,
	   highwater should be half the fifo size */

	fifosize  = (sample_sz * 4) >> 3;	/* doublewords */
	highwater = (sample_sz * 2) >> 1;	/* halfwords */

	device_id = (char *) kmalloc(strlen(s1) + strlen(s2), GFP_KERNEL);
	if (!device_id)
		return -ENOMEM;

	device_id[0] = 0;

	strcat(device_id, s1);
	strcat(device_id, s2);

	if (!chan->pbus_requested) {
		for (i = 0; i < MAX_PBUS_CHANNELS; i++) {
			if(sgipbus_request(i, device_id) == 0) {
				chan->pbus = i;
				chan->pbus_requested = 1;
				printk("got pbus %d\n", chan->pbus);
				break;
			}
		}

		if (i == MAX_PBUS_CHANNELS)
			return -EBUSY;
	}

	flags = HPC3_PDMACTRL_RT | (chan->type==ADC ? HPC3_PDMACTRL_RCV : 0) |
		(chan->little_end ? HPC3_PDMACTRL_SEL : 0);
	dmacfg = 0x8248844; /* realtime, 16-bit, cycles to spend etc.. */

	err = sgipbus_setup(chan->pbus, fifosize, highwater, flags, dmacfg);
	if (err)
		return err;

	return 0;
}

static void hal2_update_dac_buf(struct sgiaudio *sa, struct hal2_private *hp)
{
	int rp = sa->dac.rp;
	int wp = sa->dac.wp;
	int bufsz = sa->dac.bufsz;
	struct hal2_buffer *hbuf = hp->dac_head;
	struct hal2_channel *chan = &hp->dac_chan;
	int left;
	int count = 0;

	printk("updating dac buffer!\n");

	if (!hbuf) {
		printk("Oops, ADC head wasn't set..\n");
		return;
	}

	/* bytes left to read in the fifo */
	left = (wp - rp + bufsz) % bufsz;

	if (hp->dac_underrun) {
		if (left < hal2_sample_size(&hp->dac_chan)) {
			/* we're still in underrun... */
			return;
		}
		else {
			/* new enforcements have arrived, go back to normal
			 * ring buffer behavior */
			hbuf->desc.pbuf  = PHYSADDR(hbuf->buf);
			hbuf->desc.pnext = PHYSADDR(hbuf->next->buf);

			hbuf = hbuf->next;
		}
	}

	/* oops, a newly discovered underrun .. can this happen? */
	if (left < hal2_sample_size(&hp->dac_chan))
		goto out;

	if (chan->free_bufs < 1) {
		printk("Oops, lost DAC buffer count!\n");
		return;
	} else
		chan->free_bufs--;

	count = (left>PAGE_SIZE)?PAGE_SIZE:left;

	if (rp + count < bufsz)
		memcpy((void *) hbuf->buf, sa->dac.buf + rp, count);
	else {
		/* crosses the buffer boundary */
		int first = bufsz - rp - 1;

		memcpy((void *) hbuf->buf, sa->dac.buf + rp, first);
		memcpy((void *) hbuf->buf + first, sa->dac.buf, count - first);
	}

	sa->dac.rp = (rp + count) % bufsz;
	hbuf->desc.cntinfo = HPCDMA_XIE | (count & HPCDMA_BCNT);

	hbuf = hbuf->next;

	wake_up_interruptible(&sa->dac.queue);
out:

	/* was this the last buffer before underrun? */
	if (left - count < hal2_sample_size(&hp->dac_chan)) {
		/* ok, loop the empty buf.. */
		hbuf = hbuf->next;
		hbuf->desc.pnext = hbuf->desc.pbuf = PHYSADDR(hp->empty_buf);
		hbuf->desc.cntinfo = (PAGE_SIZE & HPCDMA_BCNT);

		hp->dac_underrun = 1;
	}
	hp->dac_head = hbuf;
}

static void hal2_update_adc_buf(struct sgiaudio *sa, struct hal2_private *hp)
{
	int rp = sa->adc.rp;
	int wp = sa->adc.wp;
	int bufsz = sa->adc.bufsz;
	struct hal2_buffer *hbuf = hp->dac_head;
	struct hal2_channel *chan = &hp->dac_chan;
	int left;
	int count;

	if (!hbuf) {
		printk("Oops, ADC head wasn't set..\n");
		return;
	}

	/* bytes left to write in the fifo */
	left = bufsz - (wp - rp + bufsz) % bufsz;

	if (chan->free_bufs >= chan->bufs) {
		printk("Oops, lost ADC buffer count!\n");
		return;
	} else
		chan->free_bufs++;

	/* check overrun */
	if (left < PAGE_SIZE)
		return;		/* XXX some better way? */

	count = PAGE_SIZE;

	memcpy(sa->adc.buf + wp, (void *) hbuf->buf, count);
	sa->adc.wp = (wp + count) % bufsz;

	wake_up_interruptible(&sa->adc.queue);
}

static void hal2_reset_adc_ring(struct hal2_private *hp, struct hal2_channel
				*chan)
{
	struct hal2_buffer *first, *hbuf; 

	first = chan->ring;

	for (hbuf = first; hbuf->next != first; hbuf = hbuf->next)
		hbuf->desc.cntinfo = HPCDMA_XIE;
}

static void hal2_enable_port(struct hal2_private *hp, struct hal2_channel *chan)
{
	if (!chan->hal2_enabled) {
		/* pick a dma port */
		ireg_setbit(H2IW_DMA_DRV, H2IR_DMA_DRV, (1 << chan->pbus));

		/* activate it! */
		switch (chan->type) {
		case DAC:
			ireg_setbit(H2IW_DMA_PORT_EN, H2IR_DMA_PORT_EN,
				    H2I_DMA_PORT_EN_CODECTX);
			break;
		case ADC:
			ireg_setbit(H2IW_DMA_PORT_EN, H2IR_DMA_PORT_EN,
				    H2I_DMA_PORT_EN_CODECR);
			break;
		}
		chan->hal2_enabled = 1;
	}
}

static void hal2_disable_port(struct hal2_private *hp,
			      struct hal2_channel *chan)
{
	if (chan->hal2_enabled) {
		switch (chan->type) {
		case DAC:
			ireg_clearbit(H2IW_DMA_PORT_EN, H2IR_DMA_PORT_EN,
				      H2I_DMA_PORT_EN_CODECTX);
			break;
		case ADC:
			ireg_clearbit(H2IW_DMA_PORT_EN, H2IR_DMA_PORT_EN,
				      H2I_DMA_PORT_EN_CODECR);
			break;
		}
		chan->hal2_enabled = 0;
	}
}

/* We have three bresenham clock generators, which we can use independantly.
 *
 * There is one 44.1k and one 48.0k master clock for each of them. We can
 * adjust the inc and the mod values for those clocks, and thus reduce the
 * frequency.
 * 
 * freq = master_clock * (inc / mod); where (inc / mod) is a positive fraction
 * in range [0,1]. Inc should always be set to 4 for codecs
 */
static __inline__ int hal2_calc_mod(int *mod, int master, double freq)
{
	double master_freq = (master == 0 ? 44800 : 44100);
	int tmp;

	tmp = (4.0 * master_freq) / freq;
	if (tmp > 65536 || tmp < 4)
		return -EINVAL;

	*mod = tmp;

	return 0;
}

static __inline__ double hal2_calc_freq(double *freq, int master, int mod)
{
	double master_freq = (master == 0 ? 44800 : 44100);
	double tmp;

	tmp = (4.0 * master_freq) / mod;
	if (tmp < 0 || tmp > 48000)
		return -EINVAL;

	*freq = tmp;

	return 0;
}

static int hal2_set_freq(struct hal2_channel *chan, double freq)
{
	int mod;
	int err;

	/* Try both 44.1k master and 48.0k master .. */

	err = hal2_calc_mod(&mod, 0, freq);
	if (!err) {
		chan->bres_master = 0;
		chan->bres_mod = mod;
		return 0;
	}

	err = hal2_calc_mod(&mod, 1, freq);
	if (!err) {
		chan->bres_master = 1;
		chan->bres_mod = mod;
		return 0;
	}

	/* We failed to find a matching mod value for either 44.1 master or
	 * 48.0k master, try something else..
	 */
	return -EINVAL;
}


/* Exploit this little neat gcc extension to build our three functions :) */

#define __BUILD_CONF_BRESN_CLOCK(clock)					\
static void hal2_configure_bres##clock(struct hal2_private *hp,		\
				       struct hal2_channel *chan)	\
{									\
	int master = chan->bres_master;					\
		int mod = chan->bres_mod;				\
		int inc = 4;						\
		\
		ireg_write(H2IW_BRES##clock##_C1 , master);		\
		ireg_write2(H2IW_BRES##clock##_C2 , inc, mod);		\
}									\

	__BUILD_CONF_BRESN_CLOCK(1)
	__BUILD_CONF_BRESN_CLOCK(2)
__BUILD_CONF_BRESN_CLOCK(3)

static int hal2_configure_dac(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;
	int datatype = (chan->stereo?2:0) << 8;
	int clock = 1 << 3;

	/* Let's be sure that the dma port is disabled */
	hal2_disable_port(hp, chan);

	hal2_configure_bres1(hp, chan);

	if (chan->little_end)
		ireg_setbit(H2IW_DMA_END, H2IR_DMA_END,
			    H2I_DMA_PORT_EN_CODECTX);
	else
		ireg_clearbit(H2IW_DMA_END, H2IR_DMA_END,
			      H2I_DMA_PORT_EN_CODECTX);

	ireg_write(H2IW_DAC_C1, chan->pbus | clock | datatype);

	hal2_setup_pbus(hp, chan);

	/* dac ctrl 2? */

	/* Enable the dma port (note: we're not starting the PBUS yet) */
	hal2_enable_port(hp, chan);

	return 0;
}

static int hal2_configure_adc(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;
	int datatype = (chan->stereo?2:0) << 8;
	int clock = 2 << 3;

	hal2_disable_port(hp, chan);

	hal2_configure_bres2(hp, chan);

	if (chan->little_end)
		ireg_setbit(H2IW_DMA_END, H2IR_DMA_END, H2I_DMA_PORT_EN_CODECR);
	else
		ireg_clearbit(H2IW_DMA_END, H2IR_DMA_END,
			      H2I_DMA_PORT_EN_CODECR);

	ireg_write(H2IW_ADC_C1, chan->pbus | clock | datatype);

	hal2_setup_pbus(hp, chan);

	/* adc ctrl 2? */

	/* Enable the dma port (note: we're not starting the PBUS yet) */
	hal2_enable_port(hp, chan);

	return 0;
}

static int hal2_set_dac_stereo(struct sgiaudio *sa, int stereo)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;

	chan->stereo = stereo;

	return 0;
}

static int hal2_set_adc_stereo(struct sgiaudio *sa, int stereo)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;

	chan->stereo = stereo;

	return 0;
}

static int hal2_set_dac_endian(struct sgiaudio *sa, int little_end)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;

	chan->little_end = little_end;

	return 0;
}

static int hal2_set_adc_endian(struct sgiaudio *sa, int little_end)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->adc_chan;

	chan->little_end = little_end;

	return 0;
}

static int hal2_set_dac_freq(struct sgiaudio *sa, double freq)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;
	int err;

	err = hal2_set_freq(chan, freq);
	if (err)
		return err;

	return 0;
}

static int hal2_set_adc_freq(struct sgiaudio *sa, double freq)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->adc_chan;
	int err;

	err = hal2_set_freq(chan, freq);
	if (err)
		return err;
	return 0;
}

static void hal2_go(struct hal2_channel *chan)
{
	/* The hal2 *has* to be enabled before we enable PBUS DMA transfers,
	 * atleast I say so.. (we need some kind of "order") */
	if (!chan->hal2_enabled)
		return;

	if (!chan->pbus_enabled) {
		sgipbus_enable(chan->pbus, PHYSADDR(&chan->ring->desc));
		chan->pbus_enabled = 1;
	}
#ifdef DEBUG
	else
		printk("Attempt to enable the HAL2 DMA channel twice\n");
#endif
}

static void hal2_stop_pbus(struct hal2_channel *chan)
{
	if (chan->pbus_enabled) {
		sgipbus_enable(chan->pbus, PHYSADDR(&chan->ring->desc));
		chan->pbus_enabled = 0;
	}
#ifdef DEBUG
	else
		printk("Trying to stop already stopped DMA channel\n");
#endif
}

static void hal2_start_adc(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->adc_chan;

	hal2_reset_adc_ring(hp, chan);

	hal2_go(chan);
}

static void hal2_start_dac(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;

	/* fill da funky ring buffer */
	while (chan->free_bufs && !hp->dac_underrun)
		hal2_update_dac_buf(sa, hp);

	hal2_go(chan);
}

static void hal2_stop_dac(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;

	hal2_disable_port(hp, chan);

	hal2_stop_pbus(chan);
}

static void hal2_stop_adc(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->adc_chan;

	hal2_disable_port(hp, chan);

	hal2_stop_pbus(chan);
}

static void hal2_free_dac(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->dac_chan;

	if (!hp)
		return;

	if (chan->pbus_requested)
		sgipbus_free(chan->pbus);

	kfree(hp);
}

static void hal2_free_adc(struct sgiaudio *sa)
{
	struct hal2_private *hp = (struct hal2_private *) sa->private;
	struct hal2_channel *chan = &hp->adc_chan;

	if (!hp)
		return;

	if (chan->pbus_requested)
		sgipbus_free(chan->pbus);

	kfree(hp);
}

void hal2_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct sgiaudio *sa = (struct sgiaudio *) dev_id;
	struct hal2_private *hp = (struct hal2_private *) sa->private;

	struct hal2_channel *chan;

	chan = &hp->dac_chan;
	if (chan->pbus_enabled && sgipbus_interrupted(chan->pbus))
		while (chan->free_bufs && !hp->dac_underrun)
			hal2_update_dac_buf(sa, hp);

	chan = &hp->adc_chan;
	if (chan->pbus_enabled && sgipbus_interrupted(chan->pbus))
		;
}

/*
 * My bogus OSS interface, please excuse my sins..
 */

static struct sgiaudio_chan_ops dac_ops = {
	hal2_init_dac,
	hal2_set_dac_stereo,
	hal2_set_dac_endian,
	hal2_set_dac_freq,
	hal2_configure_dac,
	hal2_start_dac,
	hal2_stop_dac,
	hal2_free_dac,
};

static struct sgiaudio_chan_ops adc_ops = {
	hal2_init_adc,
	hal2_set_adc_stereo,
	hal2_set_adc_endian,
	hal2_set_adc_freq,
	hal2_configure_adc,
	hal2_start_adc,
	hal2_stop_adc,
	hal2_free_adc,
};

static ssize_t sgiaudio_dsp_read(struct file *filp,
				 char *buf, size_t count, loff_t *ppos)
{
	return count;
}

static ssize_t sgiaudio_dsp_write(struct file *filp,
				  const char *buf, size_t count, loff_t *ppos)
{
	struct sgiaudio *sa = (struct sgiaudio *) filp->private_data;
	struct sgiaudio_chan *chan = &sa->dac;

	int wp = chan->wp;
	int rp = chan->rp;
	int bufsz = chan->bufsz;
	int left;

	unsigned long flags;

	printk("attempt to write..\n");

	if (count >= bufsz) {
		printk("hal: short write?\n");
		return -EINVAL;	/* XXX we should support this, but not yet */
	}

	if (!chan->started) {
		printk("configuring dac.\n");
		chan->ops->configure(sa);
		printk("dac configured\n");
	}

	save_and_cli(flags);

	left = (rp - wp + bufsz) % bufsz - 1;

	if (filp->f_flags | O_NONBLOCK) {
		count = count > left ? left : count;
	} else {
		/* assume that the channel is started since we're here */
		while (left < count) {
			printk("oups, overflow, let's sleep..\n");
			interruptible_sleep_on(&chan->queue);
			left = (rp - wp + bufsz) % bufsz - 1;
		}
	}

	restore_flags(flags);

	printk("copying buffer\n");

	if (!access_ok(VERIFY_READ, buf, count))
		return -EFAULT;

	if (wp + count < bufsz)
		__copy_from_user(chan->buf + wp, buf, count);
	else {
		/* crosses the buffer boundary */
		int first = bufsz - wp - 1;

		__copy_from_user(chan->buf + wp, buf, first);
		__copy_from_user(chan->buf + first, buf, count - first);
	}

	chan->wp = (wp + count) % bufsz;

	if (!chan->started) {
		printk("starting channel\n");
		chan->ops->start(sa);
		printk("channel started\n");
		sa->dac.started = 1;
	}

	return count;
}

static int sgiaudio_dsp_ioctl(struct inode *inode, struct file *filp,
			      unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int sgiaudio_init_dsp(struct sgiaudio *sa, struct sgiaudio_chan *chan,
			     struct sgiaudio_chan_ops *ops)
{
	int err;

	chan->ops = ops;

	chan->bufsz = 4 * PAGE_SIZE;
	chan->wp = 0;
	chan->rp = 1;

	chan->buf = kmalloc(chan->bufsz, GFP_KERNEL);
	if (!chan->buf)
		return -ENOMEM;

	err = chan->ops->init(sa);
	if (err)
		goto cleanup;

	err = chan->ops->stereo(sa, 0);
	if (err)
		goto cleanup;

	err = chan->ops->endian(sa, 0);
	if (err)
		goto cleanup;

	err = chan->ops->freq(sa, 8000);
	if (err)
		goto cleanup;

	return 0;

cleanup:
	kfree(chan->buf);
	return err;
}

static int sgiaudio_dsp_open(struct inode *inode, struct file *filp)
{
	struct sgiaudio *sa = (struct sgiaudio *) filp->private_data;
	struct sgiaudio_chan *dac, *adc;
	int err;

	dac = &sa->dac;
	adc = &sa->adc;

	if (filp->f_flags & O_RDONLY || filp->f_flags & O_RDWR) {
		printk("opened with read perm\n");
		err = sgiaudio_init_dsp(sa, &sa->adc, &adc_ops);
		if (err)
			return err;
	}
	if (filp->f_flags & O_WRONLY || filp->f_flags & O_RDWR) {
		printk("opened with write perm\n");
		err = sgiaudio_init_dsp(sa, &sa->dac, &dac_ops);
		if (err)
			return err;
	}

	MOD_INC_USE_COUNT;
#ifdef DEBUG
	printk("DSP opened successfully\n");
#endif
	return 0;

}

static int sgiaudio_dsp_release(struct inode *inode, struct file *filp)
{
	struct sgiaudio *sa = (struct sgiaudio *) filp->private_data;
	struct sgiaudio_chan *dac, *adc;

	printk("in dsp release..\n");

	dac = &sa->dac;
	adc = &sa->adc;

	if (filp->f_flags & O_RDONLY || filp->f_flags & O_RDWR)
		adc->ops->free(sa);

	if (filp->f_flags & O_WRONLY || filp->f_flags & O_RDWR)
		dac->ops->free(sa);

	/* we have to call the default release file operation, to free up
	 * structures and decrease usage, this is a little workaround .. */
	printk("dispatching release..");
	inode->i_op->default_file_ops->release(inode, filp);

	return 0;
}

struct file_operations sgiaudio_dsp_ops = {
	NULL,		/* sgiaudio_dsp_lseek */
	sgiaudio_dsp_read,
	sgiaudio_dsp_write,
	NULL,		/* sgiaudio_dsp_readdir */
	NULL,		/* sgiaudio_dsp_select */
	sgiaudio_dsp_ioctl,
	NULL,		/* sgiaudio_dsp_mmap */
	sgiaudio_dsp_open,
	NULL,		/* sgiaudio_dsp_flush */
	sgiaudio_dsp_release,
};

static int sgiaudio_mixer_ioctl(struct inode *inode, struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int sgiaudio_mixer_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int sgiaudio_mixer_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations sgiaudio_mixer_ops = {
	NULL,		/* sgiaudio_mixer_lseek */
	NULL,		/* sgiaudio_mixer_read */
	NULL,		/* sgiaudio_mixer_write */
	NULL,		/* sgiaudio_mixer_readdir */
	NULL,		/* sgiaudio_mixer_select */
	sgiaudio_mixer_ioctl,
	NULL,		/* sgiaudio_mixer_mmap */
	sgiaudio_mixer_open,
	NULL,		/* sgiaudio_mixer_flush */
	sgiaudio_mixer_release,
};

static int sgiaudio_open(struct inode *inode, struct file *filp)
{
	static struct sgiaudio *sa;
	int minor = MINOR(inode->i_rdev);

	static int initialized = 0;

	if (!initialized) {
		sa = kmalloc(sizeof(struct sgiaudio), GFP_KERNEL);
		if (!sa)
			return -ENOMEM;

		memset(sa, 0, sizeof(struct sgiaudio));

		filp->private_data = (void *) sa;

		initialized++;
	}

	switch (minor) {
	case 3:
		filp->f_op = &sgiaudio_dsp_ops;

		/* dispatch the call to the specific open routine .. */
		return filp->f_op->open(inode, filp);

		/* not reached */
		break;
	default:
		return -ENODEV;

		/* not reached */
		break;
	}
	return 0;
}

static int sgiaudio_release(struct inode *inode, struct file *filp)
{
	struct sgiaudio *sa = (struct sgiaudio *) filp->private_data;

	printk("release dispatched!\n");

	if (sa)
		kfree(sa);

	MOD_DEC_USE_COUNT;

	return 0;
}

static struct file_operations sgiaudio_ops = {
	NULL,		/* sgiaudio_lseek */
	NULL,		/* sgiaudio_read */
	NULL,		/* sgiaudio_write */
	NULL,		/* sgiaudio_readdir */
	NULL,		/* sgiaudio_select */
	NULL,		/* sgiaudio_ioctl */
	NULL,		/* sgiaudio_mmap */
	sgiaudio_open,
	NULL,		/* sgiaudio_flush */
	sgiaudio_release,
};

__initfunc(void sgiaudio_init(void))
{
	int result;
	int err;


	err = hal2_probe();
	if (err) {
		printk("sgiaudio: unable to find hal2 subsystem\n");
		return;
	}

	printk("sgiaudio: initializing\n");
	result = register_chrdev(SOUND_MAJOR, "sound", &sgiaudio_ops);
	if (result < 0) {
		printk("sgiaudio: unable to get major %d", SOUND_MAJOR);
	}
}

#ifdef MODULE

int init_module(void)
{
	int err;
	int result;

	err = hal2_probe();
	if (err) {
		printk("sgiaudio: unable to find hal2 subsystem\n");
		return -ENODEV;
	}

	result = register_chrdev(SOUND_MAJOR, "sound", &sgiaudio_ops);
	if (result < 0) {
		printk("sgiaudio: unable to get major %d", SOUND_MAJOR);
		return result;
	}

	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(SOUND_MAJOR, "sound");
}

#endif
