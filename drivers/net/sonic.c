/*
 * sonic.c
 *
 * (C) 1995 by Andreas Busse (andy@waldorf-gmbh.de)
 *
 * A driver for the onboard Sonic ethernet controller on Mips Jazz
 * systems (Acer Pica-61, Mips Magnum 4000, Olivetti M700 and
 * perhaps others, too)
 *
 * NOTE: THIS DRIVER IS NOT OPERATIONAL YET! DO NOT TRY TO REALLY USE IT!
 */


static char *version =
	"sonic.c:v0.01 6/16/95 Andreas Busse (andy@waldorf-gmbh.de)\n";

#include <linux/config.h>

/*
 * Sources: Olivetti M700-10 Risc Personal Computer hardware handbook,
 * National Semiconductors data sheet for the DP83932B Sonic Ethernet
 * controller, and the files "8390.c" and "skeleton.c" in this directory.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/segment.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/jazz.h>
#include <asm/jazzdma.h>
#include <linux/errno.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include "sonic.h"

extern struct device *init_etherdev(struct device *dev, int sizeof_private,
				    unsigned long *mem_startp);

/* use 0 for production, 1 for verification, >2 for debug */
#ifndef NET_DEBUG
#define NET_DEBUG 3
#endif
static unsigned int sonic_debug = NET_DEBUG;

/*
 * set to 1 if you just want to debug the driver without
 * having a real network connection.
 */
#define SONIC_LOOPBACK     0
#define SONIC_DRIVER_TEST  1

/* Information that need to be kept for each board. */
struct sonic_local {
  char *memptr;					        /* ptr to memory used for all buffers */
  SONIC_CAM_DESCRIPTOR_AREA *cda_vaddr; /* virtual CPU address of CDA */
  unsigned int cda_laddr;		        /* logical DMA address of CDA */
  SONIC_TRANSMIT_DESCRIPTOR *tda_vaddr;	/* virtual CPU address of TDA */
  unsigned int tda_laddr;		        /* logical DMA address of TDA */
  SONIC_RECEIVE_RESOURCE *rra_vaddr;    /* virtual CPU address of RRA */
  unsigned int rra_laddr;		        /* logical DMA address of RRA */
  SONIC_RECEIVE_DESCRIPTOR *rda_vaddr;  /* virtual CPU address of RDA */
  unsigned int rda_laddr;		        /* logical DMA address of RDA */
  unsigned int rd_next;                 /* the next RD where we expect data */
  unsigned char *rba_vaddr; 	        /* virtual CPU address of RBA */
  unsigned int rba_laddr;		        /* logical DMA address of RBA */
  unsigned char *pkt_vaddr;		        /* virtual CPU address of tx buffer */
  unsigned int pkt_laddr;		        /* logical DMA address of tx buffer */
  struct enet_statistics stats;
};

/*
 * We cannot use station (ethernet) address prefixes to detect the
 * sonic controller since these are board manufacturer depended.
 * So we check for known Silicon Revision IDs instead. 
 */
static unsigned short known_revisions[] =
{
  0x04,				/* Mips Magnum 4000 */
  0xffff			/* end of list */
};

/* Index to functions, as function prototypes. */

extern int sonic_probe(struct device *dev);

static int sonic_open(struct device *dev);
static int sonic_send_packet(struct sk_buff *skb, struct device *dev);
static void sonic_interrupt(int irq, struct pt_regs *regs);
static void sonic_rx(struct device *dev);
static int sonic_close(struct device *dev);
static struct enet_statistics *sonic_get_stats(struct device *dev);
static void set_multicast_list(struct device *dev, int num_addrs, void *addrs);
static int sonic_init(struct device *dev, int startp);
static void sonic_init_tda(struct device *dev);
static void sonic_init_rra(struct device *dev);
static void sonic_init_rda(struct device *dev);
static void sonic_init_cam(struct device *dev);
static void sonic_load_cam(struct device *dev);
static void sonic_dump_cam(void);
static void sonic_test(struct device *dev);
static void hardware_send_packet(struct device *dev, char *buf, int length);

#define tx_done(dev) 1

/*
 * Probe for a SONIC ethernet controller on a Mips Jazz board.
 * Either we find the Sonic controller at the usual location,
 * or we don't deal with a Jazz board (or it's broken...)
 */

int sonic_probe(struct device *dev)
{
	static unsigned version_printed = 0;
	unsigned int silicon_revision;
	unsigned int val;
	struct sonic_local *lp;
	int i;

	/*
	 * get the Silicon Revision ID. If this is one of the known
	 * one assume that we found a SONIC ethernet controller at
	 * the expected location.
	 */
	silicon_revision = sonic_read_register(SONIC_SR);
	if (sonic_debug > 1)
		printk("SONIC Silicon Revision = 0x%04x\n",silicon_revision);

	i = 0;
	while ((known_revisions[i] != 0xffff) &&
	       (known_revisions[i] != silicon_revision))
		i++;
	
	if (known_revisions[i] == 0xffff) {
		printk("SONIC ethernet controller not found (0x%4x)\n",
			silicon_revision);
		return ENODEV;
	}
	
	/* Allocate a new 'dev' if needed. */
	if (dev == NULL)
		dev = init_etherdev(0, sizeof(struct sonic_local), 0);

	if (sonic_debug  &&  version_printed++ == 0)
		printk(version);

	printk("%s: %s found at 0x%08x, ",
		dev->name, "SONIC ethernet", SONIC_BASE_ADDR);

	/* Fill in the 'dev' fields. */
	dev->base_addr = SONIC_BASE_ADDR;


	/*
	 * Put the sonic into software reset, then
	 * retrieve and print the ethernet address.
	 */

	sonic_write_register(SONIC_CMD,SONIC_CR_RST);
	sonic_write_register(SONIC_CEP,0);
	for (i=0; i<3; i++) {
		val = sonic_read_register(SONIC_CAP0-i);
		dev->dev_addr[i*2] = val;
		dev->dev_addr[i*2+1] = val >> 8;
	}

	printk("HW Address ");
	for (i = 0; i < 6; i++) {
		printk("%2.2x", dev->dev_addr[i]);
		if (i<5)
			printk(":");
	}

	/*
	 * We don't need to deal with auto-irq stuff since we
	 * hardwire the sonic interrupt to JAZZ_ETHERNET_IRQ
	 * (currently 15).
	 */

	val = request_irq(JAZZ_ETHERNET_IRQ, &sonic_interrupt, 0, "sonic");
	if (val) {
	  printk ("\n%s: unable to get IRQ %d (code %d).\n", dev->name,
			  JAZZ_ETHERNET_IRQ, val);
	  return EAGAIN;
	}
	dev->irq = JAZZ_ETHERNET_IRQ;
	irq2dev_map[JAZZ_ETHERNET_IRQ] = dev;
	if (sonic_debug > 1)
	  printk(" IRQ %d\n", JAZZ_ETHERNET_IRQ);

	/* Initialize the device structure. */
	
	if (dev->priv == NULL)
		dev->priv = kmalloc(sizeof(struct sonic_local), GFP_KERNEL);
	memset(dev->priv, 0, sizeof(struct sonic_local));

	lp = (struct sonic_local *)dev->priv;
	dev->open = sonic_open;
	dev->stop = sonic_close;
	dev->hard_start_xmit = sonic_send_packet;
	dev->get_stats	= sonic_get_stats;
	dev->set_multicast_list = &set_multicast_list;

	/* Fill in the fields of the device structure with ethernet values. */

	ether_setup(dev);
	return 0;
}


/*
 * Open/initialize the SONIC controller.
 *
 * This routine should set everything up anew at each open, even
 *  registers that "should" only need to be set once at boot, so that
 *  there is non-reboot way to recover if something goes wrong.
 */

static int sonic_open(struct device *dev)
{
	struct sonic_local *lp = (struct sonic_local *)dev->priv;
	struct sk_buff *skb;		/* only for initial checks */
	int i;

	if (sonic_debug)
	  printk("sonic_open: initializing sonic driver.\n");
	
	/*
	 * Initialize the SONIC
	 */
	sonic_init(dev, 1);

	dev->tbusy = 0;
	dev->interrupt = 0;
	dev->start = 1;

	/*
	 * This will be removed some day...
	 */
#if SONIC_DRIVER_TEST
	sonic_test(dev);			/* go into test mode */
#endif

	if (sonic_debug)
	  printk("sonic_open: Initialization done.\n");
	
	return 0;
}

static int sonic_send_packet(struct sk_buff *skb, struct device *dev)
{
  /*	struct sonic_local *lp = (struct sonic_local *)dev->priv; */

  if (sonic_debug > 1)
	printk("sonic_send_packet: skb=%p, dev=%p\n",skb,dev);
  
  if (dev->tbusy) {
	int tickssofar = jiffies - dev->trans_start;

	/* If we get here, some higher level has decided we are broken.
	   There should really be a "kick me" function call instead. */

	if (sonic_debug)
	  printk("sonic_send_packet: called with dev->tbusy = 1 !\n");
	
	if (tickssofar < 5)
	  return 1;
	
	printk("%s: transmit timed out, %s?\n", dev->name,
		   tx_done(dev) ? "IRQ conflict" : "network cable problem");
	
	/* Try to restart the adaptor. */
	sonic_init(dev, 1);
	dev->tbusy=0;
	dev->trans_start = jiffies;
  }

  /* If some higher layer thinks we've missed an tx-done interrupt
	 we are passed NULL. Caution: dev_tint() handles the cli()/sti()
	 itself. */

  if (skb == NULL) {
	dev_tint(dev);
	return 0;
  }

  /* Block a timer-based transmit from overlapping.  This could better be
	 done with atomic_swap(1, dev->tbusy), but set_bit() works as well. */

  printk("sonic_send_packet: calling set_bit()\n");
  
  if (set_bit(0, (void*)&dev->tbusy) != 0)
	printk("%s: Transmitter access conflict.\n", dev->name);
  else {
	short length = ETH_ZLEN < skb->len ? skb->len : ETH_ZLEN;
	unsigned char *buf = skb->data;

	if (sonic_debug)
	  printk("sonic_send_packet: calling hardware_send_packet(%p,%p,%d)\n",
			 dev,buf,length);

	hardware_send_packet(dev, buf, length);
	dev->trans_start = jiffies;
  }
  dev_kfree_skb (skb, FREE_WRITE);

  /* You might need to clean up and record Tx statistics here. */
#if 0
  if (inw(ioaddr) ==			/*RU*/81)
	lp->stats.tx_aborted_errors++;
#endif
	
  return 0;
}

/*
 * The typical workload of the driver:
 * Handle the network interface interrupts.
 */

static void
sonic_interrupt(int irq, struct pt_regs * regs)
{
  struct device *dev = (struct device *)(irq2dev_map[irq]);
  struct sonic_local *lp;
  int status;

  printk("in sonic_interrupt()\n");
  
  if (dev == NULL) {
	printk ("net_interrupt(): irq %d for unknown device.\n", irq);
	return;
  }
  dev->interrupt = 1;
  lp = (struct sonic_local *)dev->priv;

  status = sonic_read_register(SONIC_ISR);
  sonic_write_register(SONIC_ISR,0x7fff); /* clear all bits */
  
  if (sonic_debug > 2)
	printk("sonic_interrupt: ISR=%x\n",status);
  
  if (status & SONIC_INT_PKTRX) {
	sonic_rx(dev);				/* got packet(s) */
  }
  if (status & SONIC_INT_TXDN) {
	lp->stats.tx_packets++;		/* packet transmitted */
	dev->tbusy = 0;
	mark_bh(NET_BH);			/* Inform upper layers. */
  }
  /*
   * check error conditions
   */
  if (status & SONIC_INT_RFO)
	lp->stats.rx_fifo_errors++;
  if (status & (SONIC_INT_RDE | SONIC_INT_RBE | SONIC_INT_RBAE))
	lp->stats.rx_over_errors++;
  
  /*
   * clear interrupt bits and return
   */
  sonic_write_register(SONIC_ISR,status);
  dev->interrupt = 0;
  return;
}

/*
 * We have a good packet(s), get it/them out of the buffers.
 */

static void
sonic_rx(struct device *dev)
{
  struct sonic_local *lp;
  SONIC_RECEIVE_DESCRIPTOR *current;
  SONIC_RECEIVE_DESCRIPTOR *next;
  int status;

  lp = (struct sonic_local *)dev->priv;
  current = lp->rda_vaddr + lp->rd_next;
  next = lp->rda_vaddr + ((lp->rd_next + 1) & ~(SONIC_MAX_RDS));
  
  status = sonic_read_register(SONIC_RCR);
  if (sonic_debug > 2)
	printk("sonic_rx: RCR=%x, current=%p, next=%p, rx_status=%x, rx_pktlen=%x, rx_seqno=%x, link=%x, in_use=%x\n",
		   status,current,next,current->rx_status,current->rx_pktlen,
		   current->rx_seqno,current->link,current->in_use);
#if 0  
  if ((status & SONIC_RCR_PRX) == 0) /* any error ? */
	{
	  if (status & SONIC_RCR_FAER) lp->stats.rx_frame_errors++;
	  if (status & SONIC_RCR_CRCR) lp->stats.rx_crc_errors++;
	}
#endif
  /*
   * Due to a SONIC bug the packet might still be ok,
   * so we need to check the rx_status field first.
   */

  while(current->in_use == 0)
	{
	  struct sk_buff *skb;
	  int pkt_len;
	  char *pkt_ptr;
	  int i;
	  
	  if ((current->rx_status & SONIC_RCR_PRX) == 0)
		{
		  if (status & SONIC_RCR_FAER) lp->stats.rx_frame_errors++;
		  if (status & SONIC_RCR_CRCR) lp->stats.rx_crc_errors++;
		}
	  else
		{
		  pkt_len = current->rx_pktlen;
		  pkt_ptr = (char *)KSEG1ADDR(vdma_log2phys((current->rx_pktptr_h << 16) +
												current->rx_pktptr_l));
		  printk("sonic_rx: packet at %p:\n",pkt_ptr);
		  for (i=0; i<32; i++)
			printk("%02x ",(unsigned char)pkt_ptr[i]);
		  printk("\n");
		  
		  /* Malloc up new buffer. */

		  printk("sonic_rx: allocating sk_buff...\n");
		  skb = alloc_skb(pkt_len, GFP_ATOMIC);
		  if (skb == NULL) {
			printk("%s: Memory squeeze, dropping packet.\n", dev->name);
			lp->stats.rx_dropped++;
			break;
		  }
		  skb->len = pkt_len;
		  skb->dev = dev;

		  /* 'skb->data' points to the start of sk_buff data area. */
		  
		  printk("sonic_rx: copying packet from %p to %p\n",pkt_ptr,skb->data);
		  memcpy(skb->data, (void*)pkt_ptr, pkt_len);

		  printk("sonic_rx: passing packet to upper layer...\n");
		  netif_rx(skb);			/* pass the packet to upper layers */
		  lp->stats.rx_packets++;
		  
		  /*
		   * Mark that RD as available again, increment (and possibly
		   * wrap around) the RD index, and mark the current RD as
		   * the last in the circular buffer. Also unmark the following RD.
		   */

		  current->in_use = 1;
		  current->link |= SONIC_END_OF_LINKS;
		  next->link &= ~SONIC_END_OF_LINKS;
		  lp->rd_next = (lp->rd_next + 1) & (SONIC_MAX_RDS-1);
		}
	}
  
#if 0  
  do {
	int status = inw(ioaddr);
	int pkt_len = inw(ioaddr);
	  
	if (pkt_len == 0)			/* Read all the frames? */
	  break;					/* Done for now */

	if (status & 0x40) {		/* There was an error. */
	  lp->stats.rx_errors++;
	  if (status & 0x20) lp->stats.rx_frame_errors++;
	  if (status & 0x10) lp->stats.rx_over_errors++;
	  if (status & 0x08) lp->stats.rx_crc_errors++;
	  if (status & 0x04) lp->stats.rx_fifo_errors++;
	} else {
	  /* Malloc up new buffer. */
	  struct sk_buff *skb;

	  skb = alloc_skb(pkt_len, GFP_ATOMIC);
	  if (skb == NULL) {
		printk("%s: Memory squeeze, dropping packet.\n", dev->name);
		lp->stats.rx_dropped++;
		break;
	  }
	  skb->len = pkt_len;
	  skb->dev = dev;

	  /* 'skb->data' points to the start of sk_buff data area. */
	  memcpy(skb->data, (void*)dev->rmem_start,
			 pkt_len);
	  /* or */
	  insw(ioaddr, skb->data, (pkt_len + 1) >> 1);

	  netif_rx(skb);			/* pass the packet to upper layers */
	  lp->stats.rx_packets++;
	}
  } while (--boguscount);
#endif
  
  /* If any worth-while packets have been received, dev_rint()
	 has done a mark_bh(NET_BH) for us and will work on them
	 when we get to the bottom-half routine. */
  return;
}


/*
 * Close the SONIC device
 */
static int
sonic_close(struct device *dev)
{
	struct sonic_local *lp = (struct sonic_local *)dev->priv;

/*	lp->open_time = 0; */

	dev->tbusy = 1;
	dev->start = 0;

	/* Flush the Tx and disable Rx here. */
	
#if 0
	disable_dma(dev->dma);

	/* If not IRQ or DMA jumpered, free up the line. */
	
	outw(0x00, ioaddr+0);		/* Release the physical interrupt line. */

	free_irq(dev->irq);			/* release the IRQ */
	free_dma(dev->dma);
	kfree(lp->memptr);			/* free memory */
	
	irq2dev_map[dev->irq] = 0;
#endif

#if 0	
	/* disable ethernet IRQ on JAZZ boards */

	r4030_write_reg16(JAZZ_IO_IRQ_ENABLE,
					  r4030_read_reg16(JAZZ_IO_IRQ_ENABLE) &
					  ~JAZZ_IE_ETHERNET);
#endif
	
	/* Update the statistics here. */

	return 0;

}

/*
 * Get the current statistics.
 * This may be called with the device open or closed.
 */

static struct enet_statistics *
sonic_get_stats(struct device *dev)
{
	struct sonic_local *lp = (struct sonic_local *)dev->priv;
	short ioaddr = dev->base_addr;

	cli();
	/* Update the statistics from the device registers. */
	lp->stats.rx_missed_errors = inw(ioaddr+1);
	sti();

	return &lp->stats;
}

/*
 * Set or clear the multicast filter for this adaptor.
 * num_addrs == -1	Promiscuous mode, receive all packets
 * num_addrs == 0	Normal mode, clear multicast list
 * num_addrs > 0	Multicast mode, receive normal and MC packets, and do
 *			        best-effort filtering.
 */

static void
set_multicast_list(struct device *dev, int num_addrs, void *addrs)
{
  unsigned int rcr;

  /* FIXME: don't know how to handle multicast */
  
  rcr = sonic_read_register(SONIC_RCR);
  rcr |= SONIC_RCR_BRD; /* accept broadcast packets */
  
  if (sonic_debug > 2)
	printk("set_multicast_list: RCR=%x\n",rcr);
  
  if (num_addrs == -1)
	rcr |= SONIC_RCR_PRO;
  else
	rcr &= ~SONIC_RCR_PRO;
  
  if (sonic_debug > 2)
	printk("set_multicast_list: setting RCR=%x\n",rcr);

  sonic_write_register(SONIC_RCR,rcr);
}


/*
 * Send a packet
 */
static void
hardware_send_packet(struct device *dev, char *buf, int length)
{
  struct sonic_local *lp = (struct sonic_local *)dev->priv;
  unsigned int laddr;
  volatile SONIC_TRANSMIT_DESCRIPTOR *td = lp->tda_vaddr;

  /*
   * Map the packet data into the logical DMA address space
   */
  vdma_remap(lp->pkt_laddr,PHYSADDR(buf),length);
  laddr = lp->pkt_laddr + VDMA_OFFSET(buf);
  
#if 0
  if (sonic_debug > 2)
	printk("hardware_send_packet: buf=%p, laddr=%08x, length=%d\n",buf,laddr,length);
#endif
  /*
   * Setup the transmit descriptor and issue the transmit command.
   */

  td->tx_status = 0;			/* clear status */
  td->tx_config = 0;			/* no special configuration */
  td->tx_pktsize = length;		/* length of packet */
  td->tx_frag_count = 1;		/* single fragment */
  td->tx_frag_ptr_l = laddr & 0xffff;
  td->tx_frag_ptr_h = laddr >> 16;
  td->tx_frag_size  = length;
  td->link = (lp->tda_laddr & 0xffff) | SONIC_END_OF_LINKS;
  
  /* set the transmit descriptor pointers */

  if (sonic_debug > 3)
	printk("hardware_send_packet: setting TD pointers: laddr=%08x\n",lp->tda_laddr);

  sonic_write_register(SONIC_UTDA, lp->tda_laddr >> 16);
  sonic_write_register(SONIC_CTDA, lp->tda_laddr & 0xffff);
  
  if (sonic_debug > 3)
	{
	  printk("hardware_send_packet: td->tx_status = %x\n",
			 td->tx_status);
	  printk("hardware_send_packet: CMD=%x, UTDA=%x, CTDA=%x, TCR=%x, ISR=%x\n",
			 sonic_read_register(SONIC_CMD),
			 sonic_read_register(SONIC_UTDA),
			 sonic_read_register(SONIC_CTDA),
			 sonic_read_register(SONIC_TCR),
			 sonic_read_register(SONIC_ISR));
	  printk("hardware_send_packet: TPS=%x, TFC=%x, TSA0=%x, TSA1=%x, TFS=%x, TTDA=%x\n",
			 sonic_read_register(SONIC_TPS),
			 sonic_read_register(SONIC_TFC),
			 sonic_read_register(SONIC_TSA0),
			 sonic_read_register(SONIC_TSA1),
			 sonic_read_register(SONIC_TFS),
			 sonic_read_register(SONIC_TTDA));
	}
  
  if (sonic_debug > 2)
	printk("hardware_send_packet: issueing Tx command\n");

  dev->tbusy=1;					/* XXXX needs to be removed later */
  sonic_write_register(SONIC_CMD,SONIC_CR_TXP);

  udelay(1000);

  if (sonic_debug > 3)
	{
	  printk("hardware_send_packet: td->tx_status = %x\n",
			 td->tx_status);
	  printk("hardware_send_packet: CMD=%x, UTDA=%x, CTDA=%x, TCR=%x, ISR=%x\n",
			 sonic_read_register(SONIC_CMD),
			 sonic_read_register(SONIC_UTDA),
			 sonic_read_register(SONIC_CTDA),
			 sonic_read_register(SONIC_TCR),
			 sonic_read_register(SONIC_ISR));
	  printk("hardware_send_packet: TPS=%x, TFC=%x, TSA0=%x, TSA1=%x, TFS=%x, TTDA=%x\n",
			 sonic_read_register(SONIC_TPS),
			 sonic_read_register(SONIC_TFC),
			 sonic_read_register(SONIC_TSA0),
			 sonic_read_register(SONIC_TSA1),
			 sonic_read_register(SONIC_TFS),
			 sonic_read_register(SONIC_TTDA));
	}
}

/*
 * Initialize the SONIC ethernet controller.
 */
static int
sonic_init(struct device *dev, int startp)
{
  struct sonic_local *lp = (struct sonic_local *)dev->priv;
  unsigned int cda_size,tda_size,rra_size,rda_size,rba_size;
  unsigned int memneed;
  char *memptr;
  unsigned int laddr;
  unsigned int cmd;

  /*
   * put the Sonic into software-reset mode and
   * disable all interrupts
   */
  
  sonic_write_register(SONIC_ISR,0x7fff);
  sonic_write_register(SONIC_IMR,0);
  sonic_write_register(SONIC_CMD,SONIC_CR_RST);
  
  /*
   * allocate space for tx/rx control and buffers plus
   * some pad bytes for alignment. Note that all buffers
   * must be in the same 64k segment, so we alloc() and
   * free() memory until we get such a memory block.
   */
  if (lp->memptr == NULL) {
#if 1	
	cda_size = (sizeof(SONIC_CAM_DESCRIPTOR_AREA) + 15) & ~15;
	tda_size = (sizeof(SONIC_TRANSMIT_DESCRIPTOR) + 15) & ~15;
	rra_size = (sizeof(SONIC_RECEIVE_RESOURCE) + 15) & ~15;
	rda_size = ((sizeof(SONIC_RECEIVE_DESCRIPTOR) * SONIC_MAX_RDS) + 15) & ~15;
	rba_size = SONIC_RXBUFSIZE;
#else
	cda_size = (sizeof(SONIC_CAM_DESCRIPTOR_AREA) + 31) & ~31;
	tda_size = (sizeof(SONIC_TRANSMIT_DESCRIPTOR) + 31) & ~31;
	rra_size = (sizeof(SONIC_RECEIVE_RESOURCE) + 31) & ~31;
	rda_size = ((sizeof(SONIC_RECEIVE_DESCRIPTOR) * SONIC_MAX_RDS) + 31) & ~31;
	rba_size = SONIC_RXBUFSIZE;
#endif
	if (sonic_debug > 2)
	  printk("cda_size=%x, tda_size=%x, rra_size=%x, rda_size=%x, rba_size=%x\n",
			 cda_size,tda_size,rra_size,rda_size,rba_size);

	memneed = cda_size+tda_size+rra_size+rda_size+rba_size;
	lp->memptr = kmalloc(memneed+32,GFP_KERNEL);
	if (lp->memptr == NULL) {
	  printk("sonic_open() failed (out of memory).\n");
	  return -EAGAIN;
	}

	/*
	 * If we don't get a memory block in a single 64kbyte area,
	 * allocate new memory and the free the old block.
	 */
	while ((unsigned int)lp->memptr>>16 != ((unsigned int)lp->memptr+memneed+32)>>16) {
	  memptr = lp->memptr;
	  lp->memptr = kmalloc(memneed+32,GFP_KERNEL);
	  kfree(memptr);
	}
	
	/*
	 * setup the buffer pointers
	 */
	lp->cda_vaddr = (SONIC_CAM_DESCRIPTOR_AREA *)((KSEG1ADDR(lp->memptr) + 15) & ~15);
	lp->tda_vaddr = (SONIC_TRANSMIT_DESCRIPTOR *)(KSEG1ADDR(lp->cda_vaddr) + cda_size);
	lp->rra_vaddr = (SONIC_RECEIVE_RESOURCE *)(KSEG1ADDR(lp->tda_vaddr) + tda_size);
	lp->rda_vaddr = (SONIC_RECEIVE_DESCRIPTOR *)(KSEG1ADDR(lp->rra_vaddr) + rra_size);
	lp->rba_vaddr = (char *)(KSEG1ADDR(lp->rda_vaddr) + rda_size);
#if 0
	if (sonic_debug > 2)
	  printk("cda_v=%p, tda_v=%p, rra_v=%p, rda_v=%p, rba_v=%p\n",
			 lp->cda_vaddr,lp->tda_vaddr,lp->rra_vaddr,lp->rda_vaddr,lp->rba_vaddr);
#endif	
	/*
	 * allocate DMA pagetables
	 */
	laddr = vdma_alloc(PHYSADDR(lp->cda_vaddr),memneed);
	if (laddr == 0xffffffff) {
	  printk("sonic_open() failed (out of dma pagetables).\n");
	  return -EAGAIN;
	}
	
	lp->cda_laddr = laddr;
	lp->tda_laddr = laddr + cda_size;
	lp->rra_laddr = laddr + cda_size+tda_size;
	lp->rda_laddr = laddr + cda_size+tda_size+rra_size;
	lp->rba_laddr = laddr + cda_size+tda_size+rra_size+rda_size;
#if 1	
	if (sonic_debug > 2)
	  printk("cda_l=%08x,tda_l=%08x,rra_l=%08x,rda_l=%08x,rba_l=%08x\n",
			 lp->cda_laddr,lp->tda_laddr,lp->rra_laddr,lp->rda_laddr,lp->rba_laddr);
#endif	
  }

  /*
   * clear software reset flag, disable receiver, clear and
   * enable interrupts, then completely initialize the SONIC
   */
  sonic_write_register(SONIC_CMD,0);
  sonic_write_register(SONIC_CMD,SONIC_CR_RXDIS);
#if 0  
  r4030_write_reg16(JAZZ_IO_IRQ_ENABLE,JAZZ_IE_ETHERNET); /*
					r4030_read_reg16(JAZZ_IO_IRQ_ENABLE) |
					JAZZ_IE_ETHERNET); */
#endif
  if (sonic_debug > 2)
	printk("sonic_init: initializing descriptors\n");

  sonic_init_rra(dev);			/* initialize the receive resource area */
  sonic_init_rda(dev);			/* initialize the receive descriptor area */
  sonic_init_cam(dev);			/* load and init the CAM */
  sonic_load_cam(dev);
  sonic_init_tda(dev);			/* initialize the transmit descriptor */

  /*
   * Now enable the receiver, disable loopback
   * and enable all interrupts
   */
  sonic_write_register(SONIC_CMD,SONIC_CR_RXEN | SONIC_CR_STP);
#if SONIC_LOOPBACK
  sonic_write_register(SONIC_RCR,SONIC_RCR_DEFAULT | SONIC_RCR_LB_ENDEC);
#else
  sonic_write_register(SONIC_RCR,SONIC_RCR_DEFAULT);
#endif  
  sonic_write_register(SONIC_TCR,SONIC_TCR_DEFAULT);
  sonic_write_register(SONIC_ISR,0x7fff);
  sonic_write_register(SONIC_IMR,SONIC_IMR_DEFAULT);

  cmd = sonic_read_register(SONIC_CMD);
  if ((cmd & SONIC_CR_RXEN) == 0 |
	  (cmd & SONIC_CR_STP) == 0)
	printk("sonic_init: failed, status=%x\n",cmd);

  if (sonic_debug > 2)
	printk("sonic_init: new status=%x\n",sonic_read_register(SONIC_CMD));

  return(0);
}

/*
 * Initialize the SONIC CAM
 */

static void
sonic_init_cam(struct device *dev)
{
  struct sonic_local *lp = (struct sonic_local *)dev->priv;  
  SONIC_CAM_DESCRIPTOR_AREA *cam_area = lp->cda_vaddr;
  int i;
  
  cam_area->cam_descriptors[0].cam_entry_pointer = 0;
  cam_area->cam_descriptors[0].cam_frag2 = dev->dev_addr[1] << 8 | dev->dev_addr[0];
  cam_area->cam_descriptors[0].cam_frag1 = dev->dev_addr[3] << 8 | dev->dev_addr[2];
  cam_area->cam_descriptors[0].cam_frag0 = dev->dev_addr[5] << 8 | dev->dev_addr[4];

  /* fill all other CAM ports by some test values */
  
  for (i=1; i<CAM_DESCRIPTORS; i++) {
	cam_area->cam_descriptors[i].cam_entry_pointer = i;
	cam_area->cam_descriptors[i].cam_frag2 = i*4;
	cam_area->cam_descriptors[i].cam_frag1 = i*4+1;
	cam_area->cam_descriptors[i].cam_frag0 = i*4+2;
  }
  cam_area->cam_enable = 0x1;		/* use only CAM entry 0 */
}
  
/*
 * Load the SONIC CAM (Content Adressable Memory)
 * Actually, this isn't needed as long we're dealing
 * with the hardware address configured by the BIOS,
 * but we might want to use multiple hardware addresses.
 * Later...
 */
static void
sonic_load_cam(struct device *dev)
{
  struct sonic_local *lp = (struct sonic_local *)dev->priv;

  /*
   * Clear software reset flag, disable receiver, then load the
   * CAM with our hardware address(es).
   * This assumes that the URRA (Upper Receive Resource Area)
   * pointer has already been set.
   */
  sonic_write_register(SONIC_CMD,0);
  sonic_write_register(SONIC_CMD,SONIC_CR_RXDIS);
  sonic_write_register(SONIC_CE,(1<<CAM_DESCRIPTORS)-1);
  sonic_write_register(SONIC_CDP, lp->cda_laddr & 0xffff);
  sonic_write_register(SONIC_CDC,CAM_DESCRIPTORS);
#if 0
  if (sonic_debug > 2) {
	printk("sonic_loadcam: CMD=%x, CME=%x, CDC=%x, CDP=%x, URRA=%x\n",
		   sonic_read_register(SONIC_CMD),
		   sonic_read_register(SONIC_CE),
		   sonic_read_register(SONIC_CDC),
		   sonic_read_register(SONIC_CDP),
		   sonic_read_register(SONIC_URRA));
	printk("sonic_loadcam: ISR=%x\n",sonic_read_register(SONIC_ISR));
	printk("sonic_loadcam: issuing LCAM command\n");
  }
#endif
  
  /*
   * Now load the CAM
   */
  sonic_write_register(SONIC_CMD,SONIC_CR_LCAM);
  udelay(1000);
  if ((sonic_read_register(SONIC_ISR) & SONIC_INT_LCD) == 0)
	printk("sonic_load_cam: LCAM command failed!\n");
  
  if (sonic_debug > 2) {
	printk("sonic_loadcam: CMD=%x, ISR=%x\n",
		   sonic_read_register(SONIC_CMD),
		   sonic_read_register(SONIC_ISR));
	if (sonic_debug > 3)
	  sonic_dump_cam();
  }
}  

/*
 * initialize the transmit descriptor
 */
static void
sonic_init_tda(struct device *dev)
{
  struct sonic_local *lp = (struct sonic_local *)dev->priv;
  SONIC_TRANSMIT_DESCRIPTOR *td = lp->tda_vaddr;

  td->tx_status = 0;
  td->tx_config = 0;
  td->tx_pktsize = 0;
  td->tx_frag_count = 0;
  td->link = SONIC_END_OF_LINKS;

  sonic_write_register(SONIC_UTDA,lp->tda_laddr >> 16);
  sonic_write_register(SONIC_CTDA,lp->tda_laddr & 0xffff);
#if 0  
  if (sonic_debug > 2)
	printk("sonic_init_tda: UTDA=%x, CTDA=%x\n",
		   sonic_read_register(SONIC_UTDA),
		   sonic_read_register(SONIC_CTDA));
#endif
  /*
   * Allocate two DMA page for mapping transmit data. Two
   * are needed since the tx data may cross a page boundary.
   */
  lp->pkt_vaddr = NULL;			/* no data to send yet */
  lp->pkt_laddr = vdma_alloc(0,VDMA_PAGESIZE*2);

}

/*
 * initialize the receive resource area (RRA)
 */
static void
sonic_init_rra(struct device *dev)
{
  struct sonic_local *lp = (struct sonic_local *)dev->priv;
  unsigned int rra_start;
  unsigned int rra_end;

  rra_start = lp->rra_laddr & 0xffff;
  rra_end   = (rra_start + sizeof(SONIC_RECEIVE_RESOURCE)) & 0xffff;
  
  lp->rra_vaddr->rx_bufadr_l = lp->rba_laddr & 0xffff;
  lp->rra_vaddr->rx_bufadr_h = lp->rba_laddr >> 16;
  lp->rra_vaddr->rx_bufsize_l = SONIC_RXBUFSIZE & 0xffff;
  lp->rra_vaddr->rx_bufsize_h = SONIC_RXBUFSIZE >> 16;

  /* initialize all registers */
  
  sonic_write_register(SONIC_RSA,rra_start);
  sonic_write_register(SONIC_REA,rra_end);
  sonic_write_register(SONIC_RRP,rra_start);
  sonic_write_register(SONIC_RWP,rra_end);
  sonic_write_register(SONIC_URRA,lp->rra_laddr >> 16);

  /* load the resource pointers */

  if (sonic_debug > 2)
	printk("sonic_init_rra: issueing RRRA command\n");
  
  sonic_write_register(SONIC_CMD,SONIC_CR_RRRA);
  udelay(1000);
  while(sonic_read_register(SONIC_CMD) & SONIC_CR_RRRA) {
	if (sonic_debug > 2)
	  printk(".");
  }
  if (sonic_debug > 2)
	printk("sonic_init_rra: status=%x\n",sonic_read_register(SONIC_CMD));
}

  
/*
 * initialize the receive descriptor area (RDA)
 */
static void
sonic_init_rda(struct device *dev)
{
  struct sonic_local *lp = (struct sonic_local *)dev->priv;
  SONIC_RECEIVE_DESCRIPTOR *rd;
  unsigned int laddr = lp->rda_laddr;
  int i;

  /*
   * Initialize the receive descriptors so that they
   * become a circular linked list, ie. let the last
   * descriptor point to the first again.
   */
  rd = lp->rda_vaddr;
  for (i=0; i<SONIC_MAX_RDS; i++) {
	laddr += sizeof(SONIC_RECEIVE_DESCRIPTOR);
	rd[i].rx_status = 0;
	rd[i].rx_pktlen = 0;
	rd[i].rx_pktptr_l = 0;
	rd[i].rx_pktptr_h = 0;
	rd[i].rx_seqno = 0;
	rd[i].link = laddr & 0xffff;
	rd[i].in_use = 1;			/* not in use */
  }
  rd[i-1].link = lp->rda_laddr | SONIC_END_OF_LINKS;
  lp->rd_next = 0;
  
  sonic_write_register(SONIC_URDA,lp->rda_laddr >> 16);
  sonic_write_register(SONIC_CRDA,lp->rda_laddr & 0xffff);

}

static void sonic_dump_cam()
{
  int i,j;
  int val;

  printk("sonic_dump_cam: CME=%x, CDC=%x, CDP=%x, URRA=%x\n",
		 sonic_read_register(SONIC_CE),
		 sonic_read_register(SONIC_CDC),
		 sonic_read_register(SONIC_CDP),
		 sonic_read_register(SONIC_URRA));
  
  sonic_write_register(SONIC_CMD,SONIC_CR_RST);
  for (i=0; i<CAM_DESCRIPTORS; i++) {
	sonic_write_register(SONIC_CEP,i);
	printk("cam entry %d = ",i);
	for (j=0; j<3; j++)
	  printk("%04x ",sonic_read_register(SONIC_CAP0-j));
	printk("\n");
  }
  
  sonic_write_register(SONIC_CEP,0);
  for (i=0; i<3; i++) {
	val = sonic_read_register(SONIC_CAP0-i);
	printk("%2.2x:%2.2x:",val&0xff,val>>8);
  }
  printk("\n");
  sonic_write_register(SONIC_CMD,0); /* clear reset */
}

/*
 * Let the driver check itself. We repeatedly send packets
 * and expect them to be returned thru the Sonic loopback
 * mode.
 */
#if SONIC_DRIVER_TEST
static void sonic_test(struct device *dev)
{
  struct sk_buff *skb;
  int i;
  
  printk("SONIC TEST MODE STARTED\n");

  /*
   * switch the sonic to promiscous mode if we don't
   * use the loopback mode. Otherwise we won't receive
   * anything except occasional broadcast packets.
   */
#if !SONIC_LOOPBACK
  set_multicast_list(dev,-1,0);
#endif
  
  /*
   * allocate a single packet buffer and continously
   * transmit this packet.
   */
  skb = (struct sk_buff *)KSEG1ADDR(kmalloc(sizeof(struct sk_buff)+128,GFP_KERNEL));
  skb->len = 128;
  for (i=0; i<6; i++) {
	skb->data[i]   = dev->dev_addr[i]; /* dst address */
	skb->data[i+6] = dev->dev_addr[i]; /* src address */
  }
  skb->data[12] = 0;
  skb->data[13] = 128;
  
  for (i=0; i<32; i++)
	skb->data[i+14] = i;
  
  printk("sonic_test: Sending: skb=%p, data=%p\n",skb,skb->data);
  
  while(1) {
	hardware_send_packet(dev, skb->data, 128);
	i = 1000;
	while((dev->tbusy) && (--i))
	  udelay(2000);
	if (!i)
	  printk("sonic_test: TRANSMIT FAILED!\n");
	skb->data[14]++;
  }
}	
#endif



/*
 * Local variables:
 *  compile-command: "mips-linux-gcc -D__KERNEL__ -I/usr/src/linux/net/inet -Wall -Wstrict-prototypes -O2 -mcpu=r4000 -c sonic.c"
 *  version-control: t
 *  kept-new-versions: 5
 *  tab-width: 4
 * End:
 */
