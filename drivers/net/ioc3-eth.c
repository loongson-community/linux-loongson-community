/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Driver for SGI's IOC3 based Ethernet cards as found in the PCI card.
 *
 * Copyright (C) 1999, 2000 Ralf Baechle
 * Copyright (C) 1995, 1999, 2000 by Silicon Graphics, Inc.
 *
 * Reporting bugs:
 *
 * If you find problems with this drivers, then if possible do the
 * following.  Hook up a terminal to the MSC port, send an NMI to the CPUs
 * by typing ^Tnmi (where ^T stands for <CTRL>-T).  You'll see something
 * like:
 * 1A 000: 
 * 1A 000: *** NMI while in Kernel and no NMI vector installed on node 0
 * 1A 000: *** Error EPC: 0xffffffff800265e4 (0xffffffff800265e4)
 * 1A 000: *** Press ENTER to continue.
 *
 * Next enter the command ``lw i:0x86000f0 0x18'' and include this
 * commands output which will look like below with your bugreport.
 *
 * 1A 000: POD MSC Dex> lw i:0x86000f0 0x18
 * 1A 000: 92000000086000f0: 0021f28c 00000000 00000000 00000000
 * 1A 000: 9200000008600100: a5000000 01cde000 00000000 000004e0
 * 1A 000: 9200000008600110: 00000650 00000000 00110b15 00000000
 * 1A 000: 9200000008600120: 006d0005 77bbca0a a5000000 01ce0000
 * 1A 000: 9200000008600130: 80000500 00000500 00002538 05690008
 * 1A 000: 9200000008600140: 00000000 00000000 000003e1 0000786d
 *
 * To do:
 *
 *  - ioc3_close() should attempt to shutdown the adapter somewhat more
 *    gracefully. 
 *  - Free rings and buffers when closing or before re-initializing rings.
 *  - Handle allocation failures in ioc3_alloc_skb() more gracefully.
 *  - Handle allocation failures in ioc3_init_rings().
 *  - Use prefetching for large packets.  What is a good lower limit for
 *    prefetching?
 *  - We're probably allocating a bit too much memory.
 *  - Workarounds for various PHYs.
 *  - Proper autonegotiation.
 *  - What exactly is net_device_stats.tx_dropped supposed to count?
 *  - Use hardware checksums.
 *  - Convert to using the PCI infrastructure / IOC3 meta driver.
 */
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/sn/types.h>
#include <asm/sn/sn0/addrs.h>
#include <asm/sn/sn0/hubni.h>
#include <asm/sn/sn0/hubio.h>
#include <asm/sn/klconfig.h>
#include <asm/ioc3.h>
#include <asm/sn/sn0/ip27.h>
#include <asm/pci/bridge.h>

/* 32 RX buffers.  This is tunable in the range of 16 <= x < 512.  */
#define RX_BUFFS 32

/* Private ioctls that de facto are well known and used for examply
   by mii-tool.  */
#define SIOCGMIIPHY (SIOCDEVPRIVATE)	/* Read from current PHY */
#define SIOCGMIIREG (SIOCDEVPRIVATE+1)	/* Read any PHY register */
#define SIOCSMIIREG (SIOCDEVPRIVATE+2)	/* Write any PHY register */

/* These exist in other drivers; we don't use them at this time.  */
#define SIOCGPARAMS (SIOCDEVPRIVATE+3)	/* Read operational parameters */
#define SIOCSPARAMS (SIOCDEVPRIVATE+4)	/* Set operational parameters */

static int ioc3_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static void ioc3_set_multicast_list(struct net_device *dev);
static int ioc3_open(struct net_device *dev);
static int ioc3_start_xmit(struct sk_buff *skb, struct net_device *dev);
static void ioc3_timeout(struct net_device *dev);
static int ioc3_close(struct net_device *dev);
static inline unsigned int ioc3_hash(const unsigned char *addr);

static const char ioc3_str[] = "IOC3 Ethernet";

/* Private per NIC data of the driver.  */
struct ioc3_private {
	struct ioc3 *regs;
	int phy;
	unsigned long rxr;		/* pointer to receiver ring */
	struct ioc3_etxd *txr;
	struct sk_buff *rx_skbs[512];
	struct sk_buff *tx_skbs[128];
	struct net_device_stats stats;
	int rx_ci;			/* RX consumer index */
	int rx_pi;			/* RX producer index */
	int tx_ci;			/* TX consumer index */
	int tx_pi;			/* TX producer index */
	int txqlen;
	spinlock_t ioc3_lock;
};

/* We use this to acquire receive skb's that we can DMA directly into. */
#define ALIGNED_RX_SKB_ADDR(addr) \
	((((unsigned long)(addr) + (128 - 1)) & ~(128 - 1)) - (unsigned long)(addr))

#define ioc3_alloc_skb(__length, __gfp_flags) \
({	struct sk_buff *__skb; \
	__skb = alloc_skb((__length) + 128, (__gfp_flags)); \
	if (__skb) { \
		int __offset = ALIGNED_RX_SKB_ADDR(__skb->data); \
		if(__offset) \
			skb_reserve(__skb, __offset); \
	} \
	__skb; \
})

/* BEWARE: The IOC3 documentation documents the size of rx buffers as
   1644 while it's actually 1664.  This one was nasty to track down ...  */
#define RX_OFFSET		10
#define RX_BUF_ALLOC_SIZE	(1664 + RX_OFFSET + 128)

/* DMA barrier to separate cached and uncached accesses.  */
#define BARRIER()							\
	__asm__("sync" ::: "memory")


#define IOC3_SIZE 0x100000

#define ioc3_r(reg)							\
({									\
	u32 __res;							\
	__res = ioc3->reg;						\
	__res;								\
})

#define ioc3_w(reg,val)							\
do {									\
	(ioc3->reg = (val));						\
} while(0)

static inline u32
mcr_pack(u32 pulse, u32 sample)
{
	return (pulse << 10) | (sample << 2);
}

static int
nic_wait(struct ioc3 *ioc3)
{
	u32 mcr;

        do {
                mcr = ioc3_r(mcr);
        } while (!(mcr & 2));

        return mcr & 1;
}

static int
nic_reset(struct ioc3 *ioc3)
{
        int presence;

	ioc3_w(mcr, mcr_pack(500, 65));
	presence = nic_wait(ioc3);

	ioc3_w(mcr, mcr_pack(0, 500));
	nic_wait(ioc3);

        return presence;
}

/*
 * Read a byte from an iButton device
 */
static u32
nic_read_byte(struct ioc3 *ioc3)
{
	u32 result = 0;
	int i;

	for (i = 0; i < 8; i++) {
		ioc3_w(mcr, mcr_pack(6, 13));
		result = (result >> 1) | (nic_wait(ioc3) << 7);

		ioc3_w(mcr, mcr_pack(0, 100));
		nic_wait(ioc3);
	}

	return result;
}

/*
 * Write a byte to an iButton device
 */
static void
nic_write_byte(struct ioc3 *ioc3, int byte)
{
	int i, bit;

	for (i = 8; i; i--) {
		bit = byte & 1;
		byte >>= 1;

		if (bit)
			ioc3_w(mcr, mcr_pack(6, 110));
		else
			ioc3_w(mcr, mcr_pack(80, 30));
		nic_wait(ioc3);
	}
}

static void nic_show_regnr(struct ioc3 *ioc3)
{
	const char *type;
	u8 regnr[8];
	int i;

	nic_write_byte(ioc3, 0x33);
	for (i = 0; i < 8; i++)
		regnr[i] = nic_read_byte(ioc3);

	switch (regnr[0]) {
	case 0x01:	type = "DS1990A"; break;
	case 0x91:	type = "DS1981U"; break;
	default:	type = "unknown"; break;
	}

	printk("Found %s NIC, registration number "
	       "%02x:%02x:%02x:%02x:%02x:%02x, CRC %02x.\n", type,
	       regnr[1], regnr[2], regnr[3], regnr[4], regnr[5], regnr[6],
	       regnr[7]);
}

/*
 * Read the NIC (Number-In-a-Can) device.
 */
static void ioc3_get_eaddr(struct net_device *dev, struct ioc3 *ioc3)
{
	u8 nic[14];
	int i;

	ioc3_w(gpcr_s, (1 << 21));

	nic_reset(ioc3);
	nic_show_regnr(ioc3);

	nic_reset(ioc3);
	nic_write_byte(ioc3, 0xcc);
	nic_write_byte(ioc3, 0xf0);
	nic_write_byte(ioc3, 0x00);
	nic_write_byte(ioc3, 0x00);

	for (i = 13; i >= 0; i--)
		nic[i] = nic_read_byte(ioc3);

	printk("Ethernet address is ");
	for (i = 2; i < 8; i++) {
		dev->dev_addr[i - 2] = nic[i];
		printk("%02x", nic[i]);
		if (i < 7)
			printk(":");
	}
	printk(".\n");
}

/* Caller must hold the ioc3_lock ever for MII readers.  This is also
   used to protect the transmitter side but it's low contention.  */
static u16 mii_read(struct ioc3 *ioc3, int phy, int reg)
{
	while (ioc3->micr & MICR_BUSY);
	ioc3->micr = (phy << MICR_PHYADDR_SHIFT) | reg | MICR_READTRIG;
	while (ioc3->micr & MICR_BUSY);

	return ioc3->midr & MIDR_DATA_MASK;
}

static void mii_write(struct ioc3 *ioc3, int phy, int reg, u16 data)
{
	while (ioc3->micr & MICR_BUSY);
	ioc3->midr = data;
	ioc3->micr = (phy << MICR_PHYADDR_SHIFT) | reg;
	while (ioc3->micr & MICR_BUSY);
}

static struct net_device_stats *ioc3_get_stats(struct net_device *dev)
{
	struct ioc3_private *ip = (struct ioc3_private *) dev->priv;
	struct ioc3 *ioc3 = ip->regs;

	ip->stats.collisions += (ioc3->etcdc & ETCDC_COLLCNT_MASK);
	return &ip->stats;
}

static inline void
ioc3_rx(struct net_device *dev, struct ioc3_private *ip, struct ioc3 *ioc3)
{
	struct sk_buff *skb, *new_skb;
	int rx_entry, n_entry, len;
	struct ioc3_erxbuf *rxb;
	unsigned long *rxr;
	u32 w0, err;

	rxr = (unsigned long *) ip->rxr;		/* Ring base */
	rx_entry = ip->rx_ci;				/* RX consume index */
	n_entry = ip->rx_pi;

	skb = ip->rx_skbs[rx_entry];
	rxb = (struct ioc3_erxbuf *) (skb->data - RX_OFFSET);
	w0 = rxb->w0;

	while (w0 & ERXBUF_V) {
		ioc3->eisr = EISR_RXTIMERINT;		/* Ack */
		ioc3->eisr;				/* Flush */

		err = rxb->err;				/* It's valid ...  */
		if (err & ERXBUF_GOODPKT) {
			len = (w0 >> ERXBUF_BYTECNT_SHIFT) & 0x7ff;
			skb_trim(skb, len);
			skb->protocol = eth_type_trans(skb, dev);
			netif_rx(skb);

			new_skb = ioc3_alloc_skb(RX_BUF_ALLOC_SIZE, GFP_ATOMIC);
			if (!new_skb) {
				/* Ouch, drop packet and just recycle packet
				   to keep the ring filled.  */
				ip->stats.rx_dropped++;
				new_skb = skb;
				goto next;
			}

			new_skb->dev = dev;

			/* Because we reserve afterwards. */
			skb_put(new_skb, (1664 + RX_OFFSET));
			rxb = (struct ioc3_erxbuf *) new_skb->data;
			skb_reserve(new_skb, RX_OFFSET);

			ip->stats.rx_packets++;		/* Statistics */
			ip->stats.rx_bytes += len;

			goto next;
		}
		if (err & (ERXBUF_CRCERR | ERXBUF_FRAMERR | ERXBUF_CODERR |
		           ERXBUF_INVPREAMB | ERXBUF_BADPKT | ERXBUF_CARRIER)) {
			/* We don't send the skbuf to the network layer, so
			   just recycle it.  */
			new_skb = skb;

			if (err & ERXBUF_CRCERR)	/* Statistics */
				ip->stats.rx_crc_errors++;
			if (err & ERXBUF_FRAMERR)
				ip->stats.rx_frame_errors++;
			ip->stats.rx_errors++;
		}

next:
		ip->rx_skbs[n_entry] = new_skb;
		rxr[n_entry] = (0xa5UL << 56) |
		                ((unsigned long) rxb & TO_PHYS_MASK);
		rxb->w0 = 0;				/* Clear valid flag */
		n_entry = (n_entry + 1) & 511;		/* Update erpir */
		ioc3->erpir = (n_entry << 3) | ERPIR_ARM;

		/* Now go on to the next ring entry.  */
		rx_entry = (rx_entry + 1) & 511;
		skb = ip->rx_skbs[rx_entry];
		rxb = (struct ioc3_erxbuf *) (skb->data - RX_OFFSET);
		w0 = rxb->w0;
	}
	ip->rx_pi = n_entry;
	ip->rx_ci = rx_entry;

	return;
}

static inline void
ioc3_tx(struct net_device *dev, struct ioc3_private *ip, struct ioc3 *ioc3)
{
	unsigned long packets, bytes;
	int tx_entry, o_entry;
	struct sk_buff *skb;
	u32 etcir;

	spin_lock(&ip->ioc3_lock);
	etcir = ioc3->etcir;
	tx_entry = (etcir >> 7) & 127;
	o_entry = ip->tx_ci;
	packets = 0;
	bytes = 0;

	while (o_entry != tx_entry) {
		ioc3->eisr = EISR_TXEXPLICIT;		/* Ack */
		ioc3->eisr;				/* Flush */

		packets++;
		bytes += skb->len;
		skb = ip->tx_skbs[o_entry];
		dev_kfree_skb_irq(skb);
		ip->tx_skbs[o_entry] = NULL;

		o_entry = (o_entry + 1) & 127;		/* Next */

		etcir = ioc3->etcir;			/* More pkts sent?  */
		tx_entry = (etcir >> 7) & 127;
	}

	ip->stats.tx_packets += packets;
	ip->stats.tx_bytes += bytes;
	ip->txqlen -= packets;

	if (ip->txqlen < 128)
		netif_wake_queue(dev);

	ip->tx_ci = o_entry;
	spin_unlock(&ip->ioc3_lock);
}

/*
 * Deal with fatal IOC3 errors.  For now let's panic.  This condition might
 * be caused by a hard or software problems, so we should try to recover
 * more gracefully if this ever happens.
 */
static void
ioc3_error(struct net_device *dev, struct ioc3_private *ip,
           struct ioc3 *ioc3, u32 eisr)
{
	if (eisr & (EISR_RXMEMERR | EISR_TXMEMERR)) {
		if (eisr & EISR_RXMEMERR) {
			panic("%s: RX PCI error.\n", dev->name);
		}
		if (eisr & EISR_TXMEMERR) {
			panic("%s: TX PCI error.\n", dev->name);
		}
	}
}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread.  */
static void ioc3_interrupt(int irq, void *_dev, struct pt_regs *regs)
{
	struct net_device *dev = (struct net_device *)_dev;
	struct ioc3_private *ip = dev->priv;
	struct ioc3 *ioc3 = ip->regs;
	u32 eisr, eier;

	ip = dev->priv;

	eier = ioc3->eier;				/* Disable eth ints */
	ioc3->eier = 0;
	eisr = ioc3->eisr;
	__sti();

	if (eisr & EISR_RXTIMERINT) {
		ioc3_rx(dev, ip, ioc3);
	}
	if (eisr & EISR_TXEXPLICIT) {
		ioc3_tx(dev, ip, ioc3);
	}
	if (eisr & (EISR_RXMEMERR | EISR_TXMEMERR)) {
		ioc3_error(dev, ip, ioc3, eisr);
	}

	__cli();
	ioc3->eier = eier;

	return;
}

int ioc3_eth_init(struct net_device *dev, struct ioc3_private *ip,
                  struct ioc3 *ioc3)
{
	u16 mii0, mii_status, mii2, mii3, mii4;
	u32 vendor, model, rev;

	spin_lock_irq(&ip->ioc3_lock);

	mii0 = mii_read(ioc3, ip->phy, 0);
	mii_status = mii_read(ioc3, ip->phy, 1);
	mii2 = mii_read(ioc3, ip->phy, 2);
	mii3 = mii_read(ioc3, ip->phy, 3);
	mii4 = mii_read(ioc3, ip->phy, 4);
	vendor = (mii2 << 12) | (mii3 >> 4);
	model  = (mii3 >> 4) & 0x3f;
	rev    = mii3 & 0xf;
	printk("Ok, using PHY %d, vendor 0x%x, model %d, rev %d.\n",
	       ip->phy, vendor, model, rev);
	printk(KERN_INFO "%s:  MII transceiver found at MDIO address "
	       "%d, config %4.4x status %4.4x.\n",
	       dev->name, ip->phy, mii0, mii_status);

	/* Autonegotiate 100mbit and fullduplex. */
	mii_write(ioc3, ip->phy, 0, mii0 | 0x3100);

	spin_unlock_irq(&ip->ioc3_lock);
	mdelay(1000);				/* XXX Yikes XXX */
	spin_lock_irq(&ip->ioc3_lock);

	mii_status = mii_read(ioc3, ip->phy, 1);
	spin_unlock_irq(&ip->ioc3_lock);

	return 0;	/* XXX */
}

/* To do: For reinit of the ring we have to cleanup old skbs first ... */
static void
ioc3_init_rings(struct net_device *dev, struct ioc3_private *p,
	        struct ioc3 *ioc3)
{
	struct ioc3_erxbuf *rxb;
	unsigned long *rxr;
	unsigned long ring;
	int i;

	/* Allocate and initialize rx ring.  4kb = 512 entries  */
	p->rxr = get_free_page(GFP_KERNEL);
	rxr = (unsigned long *) p->rxr;

	/* Now the rx buffers.  The RX ring may be larger but we only
	   allocate 16 buffers for now.  Need to tune this for performance
	   and memory later.  */
	for (i = 0; i < RX_BUFFS; i++) {
		struct sk_buff *skb;

		skb = ioc3_alloc_skb(RX_BUF_ALLOC_SIZE, 0);
		if (!skb) {
			show_free_areas();
			continue;
		}

		p->rx_skbs[i] = skb;
		skb->dev = dev;

		/* Because we reserve afterwards. */
		skb_put(skb, (1664 + RX_OFFSET));
		rxb = (struct ioc3_erxbuf *) skb->data;
		rxb->w0 = 0;				/* Clear valid bit */
		rxr[i] = (0xa5UL << 56) | ((unsigned long) rxb & TO_PHYS_MASK);
		skb_reserve(skb, RX_OFFSET);
	}

	/* Now the rx ring base, consume & produce registers.  */
	ring = (0xa5UL << 56) | (p->rxr & TO_PHYS_MASK);
	ioc3->erbr_h = ring >> 32;
	ioc3->erbr_l = ring & 0xffffffff;
	p->rx_ci = 0;
	ioc3->ercir  = (p->rx_ci << 3);
	p->rx_pi = RX_BUFFS;
	ioc3->erpir  = (p->rx_pi << 3) | ERPIR_ARM;

	/* Allocate and initialize tx rings.  16kb = 128 bufs.  */
	p->txr = (struct ioc3_etxd *)__get_free_pages(GFP_KERNEL, 2);
	ring = (0xa5UL << 56) | ((unsigned long)p->txr & TO_PHYS_MASK);

	/* Now the tx ring base, consume & produce registers.  */
	ioc3->etbr_h = ring >> 32;
	ioc3->etbr_l = ring & 0xffffffff;
	p->tx_pi = 0;
	ioc3->etpir  = (p->tx_pi << 7);
	p->tx_ci = 0;
	ioc3->etcir  = (p->tx_pi << 7);
	ioc3->etcir;					/* Flush */
}

void
ioc3_ssram_disc(struct ioc3_private *ip)
{
	struct ioc3 *ioc3 = ip->regs;
	volatile u32 *ssram0 = &ioc3->ssram[0x0000];
	volatile u32 *ssram1 = &ioc3->ssram[0x4000];
	unsigned int pattern = 0x5555;

	/* Assume the larger size SSRAM and enable parity checking */
	ioc3->emcr |= (EMCR_BUFSIZ | EMCR_RAMPAR);

	*ssram0 = pattern;
	*ssram1 = ~pattern & IOC3_SSRAM_DM;

	if ((*ssram0 & IOC3_SSRAM_DM) != pattern ||
	    (*ssram1 & IOC3_SSRAM_DM) != (~pattern & IOC3_SSRAM_DM)) {
		/* set ssram size to 64 KB */
		ioc3->emcr &= ~EMCR_BUFSIZ;
		printk("IOC3 SSRAM has 64 kbyte.\n");
	} else {
		//ei->ei_ssram_bits = EMCR_BUFSIZ | EMCR_RAMPAR;
		printk("IOC3 SSRAM has 64 kbyte.\n");
	}
}

static int ioc3_probe1(struct pci_dev *pdev)
{
	struct net_device *dev = NULL;	// XXX
	struct ioc3_private *ip;
	struct ioc3 *ioc3;
	unsigned long ioc3_base, ioc3_size;
	int phy;
	u16 word;

	ioc3_base = pdev->resource[0].start;
	ioc3_size = pdev->resource[0].end - ioc3_base;
	ioc3 = (struct ioc3 *) ioremap(ioc3_base, ioc3_size);

	/* Probe for a PHY first ... */

	ioc3->emcr = EMCR_RST;			/* Reset		*/
	ioc3->emcr;				/* flush WB		*/
	udelay(4);				/* Give it time ...	*/
	ioc3->emcr = 0;

	for (phy = 31; phy > 0; phy--) {
		word = mii_read(ioc3, phy, 2);
		if (word == 0x0000)
			return -ENODEV;		/* No PHY connected */
		if (word != 0xffff)
			break;			/* Found a PHY */
	}
	if (phy == -1) {
		printk("No PHY present?\n");
		return -ENODEV;
	}

	dev = init_etherdev(dev, 0);

	ip = (struct ioc3_private *) kmalloc(sizeof(*ip), GFP_KERNEL);
	if (ip == NULL) {
		printk(KERN_ERR "ioc3: Unable to allocate memory\n");
		return -ENOMEM;
	}
	memset(ip, 0, sizeof(*ip));
	dev->priv = ip;
	dev->irq = pdev->irq;

	ip->regs = ioc3;
	ip->phy = phy;

	ioc3_eth_init(dev, ip, ioc3);
	ioc3_ssram_disc(ip);
        ioc3_get_eaddr(dev, ioc3);
	ioc3_init_rings(dev, ip, ioc3);

	spin_lock_init(&ip->ioc3_lock);
	ip->txqlen = 0;

	/* Misc registers  */
	ioc3->erbar = 0;
	ioc3->etcsr = (17<<ETCSR_IPGR2_SHIFT) | (11<<ETCSR_IPGR1_SHIFT) | 21;
	ioc3->etcdc;				/* Clear on read */
	ioc3->ercsr = 15;			/* RX low watermark  */
	ioc3->ertr = 0;				/* Interrupt immediately */
	ioc3->emar_h = (dev->dev_addr[5] << 8) | dev->dev_addr[4];
	ioc3->emar_l = (dev->dev_addr[3] << 24) | (dev->dev_addr[2] << 16) |
	               (dev->dev_addr[1] <<  8) | dev->dev_addr[0];
	ioc3->ehar_h = ioc3->ehar_l = 0;
	ioc3->ersr = 42;	/* XXX should be random */
	//ioc3->erpir = ERPIR_ARM;

	/* The IOC3-specific entries in the device structure. */
	dev->open		= ioc3_open;
	dev->hard_start_xmit	= ioc3_start_xmit;
	dev->tx_timeout		= ioc3_timeout;
	dev->watchdog_timeo	= (400 * HZ) / 1000;
	dev->stop		= ioc3_close;
	dev->get_stats		= ioc3_get_stats;
	dev->do_ioctl		= ioc3_ioctl;
	dev->set_multicast_list	= ioc3_set_multicast_list;

	ether_setup(dev);

	return 0;
}

static int __init ioc3_probe(void)
{
	static int called = 0;
	int cards;

	if (called)
		return -ENODEV;
	called = 1;

	cards = 0;
	if (pci_present()) {
		struct pci_dev *pdev = NULL;

		while ((pdev = pci_find_device(PCI_VENDOR_ID_SGI,
		                               PCI_DEVICE_ID_SGI_IOC3, pdev)))
			if (ioc3_probe1(pdev) == 0)
				cards++;
	}

	return cards ? -ENODEV : 0;
}

static int
ioc3_open(struct net_device *dev)
{
	struct ioc3_private *ip = dev->priv;
	struct ioc3 *ioc3 = ip->regs;
	unsigned long flags;

	save_flags(flags); cli();
	if (request_irq(dev->irq, ioc3_interrupt, 0, ioc3_str, dev)) {
		printk("%s: Can't get irq %d\n", dev->name, dev->irq);
		restore_flags(flags);

		return -EAGAIN;
	}

	//ioc3_eth_init(dev, p, ioc3);

	ioc3->emcr = ((RX_OFFSET / 2) << EMCR_RXOFF_SHIFT) | EMCR_TXDMAEN |
	             EMCR_TXEN | EMCR_RXDMAEN | EMCR_RXEN;
	ioc3->eier = EISR_RXTIMERINT | EISR_TXEXPLICIT | /* Interrupts ...  */
	             EISR_RXMEMERR | EISR_TXMEMERR;

	netif_start_queue(dev);
	restore_flags(flags);

	MOD_INC_USE_COUNT;

        return 0;
}

static int
ioc3_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	unsigned long data;
	struct ioc3_private *ip = dev->priv;
	struct ioc3 *ioc3 = ip->regs;
	unsigned int len;
	struct ioc3_etxd *desc;
	int produce;

	spin_lock_irq(&ip->ioc3_lock);

	data = (unsigned long) skb->data;
	len = skb->len;

	produce = ip->tx_pi;
	desc = &ip->txr[produce];

	if (len <= 104) {
		/* Short packet, let's copy it directly into the ring.  */
		memcpy(desc->data, skb->data, skb->len);
		if (len < ETH_ZLEN) {
			/* Very short packet, pad with zeros at the end. */
			memset(desc->data + len, 0, ETH_ZLEN - len);
			len = ETH_ZLEN;
		}
		desc->cmd    = len | ETXD_INTWHENDONE | ETXD_D0V;
		desc->bufcnt = len;
	} else if ((data ^ (data + len)) & 0x4000) {
		unsigned long b2, s1, s2;

		b2 = (data | 0x3fffUL) + 1UL;
		s1 = b2 - data;
		s2 = data + len - b2;

		desc->cmd    = len | ETXD_INTWHENDONE | ETXD_B1V | ETXD_B2V;
		desc->bufcnt = (s1 << ETXD_B1CNT_SHIFT) |
		               (s2 << ETXD_B2CNT_SHIFT);
		desc->p1     = (0xa5UL << 56) | (data & TO_PHYS_MASK);
		desc->p2     = (0xa5UL << 56) | (data & TO_PHYS_MASK);
	} else {
		/* Normal sized packet that doesn't cross a page boundary. */
		desc->cmd    = len | ETXD_INTWHENDONE | ETXD_B1V;
		desc->bufcnt = len << ETXD_B1CNT_SHIFT;
		desc->p1     = (0xa5UL << 56) | (data & TO_PHYS_MASK);
	}

	BARRIER();

	dev->trans_start = jiffies;
	ip->tx_skbs[produce] = skb;			/* Remember skb */
	produce = (produce + 1) & 127;
	ip->tx_pi = produce;
	ioc3->etpir = produce << 7;			/* Fire ... */

	ip->txqlen++;

	if (ip->txqlen > 127)
		netif_stop_queue(dev);

	spin_unlock_irq(&ip->ioc3_lock);

	return 0;
}

static void ioc3_timeout(struct net_device *dev)
{
	printk("%s: transmit timed out, resetting\n", dev->name);
	/* XXX should reset device here.  */

	dev->trans_start = jiffies;
	netif_wake_queue(dev);
}

static int
ioc3_close(struct net_device *dev)
{
	struct ioc3_private *ip = dev->priv;
	struct ioc3 *ioc3 = ip->regs;

	netif_stop_queue(dev);

	ioc3->emcr = 0;				/* Shutup */
	ioc3->eier = 0;				/* Disable interrupts */
	ioc3->eier;				/* Flush */
	free_irq(dev->irq, dev);

	MOD_DEC_USE_COUNT;

	return 0;
}

#if 0
/* Initialize the NIC  */
static int
ioc3_init(struct ioc3_private *p, struct pci_dev *dev)
{
#if 0
	unsigned long base;
	struct ioc3 *ioc3;

	pci_set_master(dev);
	base = dev->base_address[0] & PCI_BASE_ADDRESS_MEM_MASK;
	printk("Base address at %08lx\n", base);

	p->regs = ioc3 = ioremap (base, IOC3_SIZE);
	printk("Remapped base address to %08lx\n", (unsigned long) regs);

	read_nic(ioc3);

	return 0;
#endif
	panic(__FUNCTION__" has been called.\n");
}
#endif

/*
 * Given a multicast ethernet address, this routine calculates the
 * address's bit index in the logical address filter mask
 */
#define CRC_MASK        0xEDB88320

static inline unsigned int
ioc3_hash(const unsigned char *addr)
{
	unsigned int temp = 0;
	unsigned char byte;
	unsigned int crc;
	int bits, len;

	len = ETH_ALEN;
	for (crc = ~0; --len >= 0; addr++) {
		byte = *addr;
		for (bits = 8; --bits >= 0; ) {
			if ((byte ^ crc) & 1)
				crc = (crc >> 1) ^ CRC_MASK;
			else
				crc >>= 1;
			byte >>= 1;
		}
	}

	crc &= 0x3f;    /* bit reverse lowest 6 bits for hash index */
	for (bits = 6; --bits >= 0; ) {
		temp <<= 1;
		temp |= (crc & 0x1);
		crc >>= 1;
	}

	return temp;
}

/* Provide ioctl() calls to examine the MII xcvr state. */
static int ioc3_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct ioc3_private *ip = (struct ioc3_private *) dev->priv;
	u16 *data = (u16 *)&rq->ifr_data;
	struct ioc3 *ioc3 = ip->regs;
	int phy = ip->phy;

	switch (cmd) {
	case SIOCGMIIPHY:	/* Get the address of the PHY in use.  */
		if (phy == -1)
			return -ENODEV;
		data[0] = phy;
		return 0;

	case SIOCGMIIREG:	/* Read any PHY register.  */
		spin_lock_irq(&ip->ioc3_lock);
		data[3] = mii_read(ioc3, data[0], data[1]);
		spin_unlock_irq(&ip->ioc3_lock);
		return 0;

	case SIOCSMIIREG:	/* Write any PHY register.  */
		spin_lock_irq(&ip->ioc3_lock);
		mii_write(ioc3, data[0], data[1], data[2]);
		spin_unlock_irq(&ip->ioc3_lock);
		return 0;

	default:
		return -EOPNOTSUPP;
	}

	return -EOPNOTSUPP;
}

static void ioc3_set_multicast_list(struct net_device *dev)
{
	struct dev_mc_list *dmi = dev->mc_list;
	char *addr = dmi->dmi_addr;
	struct ioc3_private *p;
	struct ioc3 *ioc3;
	u64 ehar = 0;
	int i;

	p = dev->priv;
	ioc3 = p->regs;

	if (dev->flags & IFF_PROMISC) {			/* Set promiscuous.  */
		/* Unconditionally log net taps.  */
		printk(KERN_INFO "%s: Promiscuous mode enabled.\n", dev->name);
		ioc3->emcr |= EMCR_PROMISC;
		ioc3->emcr;
	} else {
		ioc3->emcr &= ~EMCR_PROMISC;		/* Clear promiscuous. */
		ioc3->emcr;

		if ((dev->flags & IFF_ALLMULTI) || (dev->mc_count > 64)) {
			/* Too many for hashing to make sense or we want all
			   multicast packets anyway,  so skip computing all the
			   hashes and just accept all packets.  */
			ioc3->ehar_h = 0xffffffff;
			ioc3->ehar_l = 0xffffffff;
		} else {
			for (i = 0; i < dev->mc_count; i++) {
				dmi = dmi->next;

				if (!(*addr & 1))
					continue;

				ehar |= (1 << ioc3_hash(addr));
			}
			ioc3->ehar_h = ehar >> 32;
			ioc3->ehar_l = ehar & 0xffffffff;
		}
	}
}

#ifdef MODULE
MODULE_AUTHOR("Ralf Baechle <ralf@oss.sgi.com>");
MODULE_DESCRIPTION("SGI IOC3 Ethernet driver");
#endif /* MODULE */

module_init(ioc3_probe);
//module_exit(ioc3_cleanup_module);	/* Not yet ...  */
