/*
 * BRIEF MODULE DESCRIPTION
 *	Au1000 USB Device-Side Serial TTY Driver
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *		stevel@mvista.com or source@mvista.com
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED	  ``AS	IS'' AND   ANY	EXPRESS OR IMPLIED
 *  WARRANTIES,	  INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO	EVENT  SHALL   THE AUTHOR  BE	 LIABLE FOR ANY	  DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED	  TO, PROCUREMENT OF  SUBSTITUTE GOODS	OR SERVICES; LOSS OF
 *  USE, DATA,	OR PROFITS; OR	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN	 CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#define DEBUG
#include <linux/usb.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/au1000.h>
#include <asm/au1000_dma.h>

/* Module information */
MODULE_AUTHOR("Steve Longerbeam, stevel@mvista.com, www.mvista.com");
MODULE_DESCRIPTION("Au1000 USB Device-Side Serial TTY Driver");

#undef USBDEV_PIO

#define SERIAL_TTY_MAJOR 189

#define MAX(a,b)	(((a)>(b))?(a):(b))

#define ALLOC_FLAGS (in_interrupt () ? GFP_ATOMIC : GFP_KERNEL)

#define MAX_NUM_PORTS 2

#define NUM_PORTS 1
#define NUM_EP 2*NUM_PORTS

#define EP0_MAX_PACKET_SIZE 64
#define EP2_MAX_PACKET_SIZE 64
#define EP3_MAX_PACKET_SIZE 64
#define EP4_MAX_PACKET_SIZE 64
#define EP5_MAX_PACKET_SIZE 64

#ifdef USBDEV_PIO
#define EP_FIFO_DEPTH 8
#endif

typedef enum {
	ATTACHED = 0,
	POWERED,
	DEFAULT,
	ADDRESS,
	CONFIGURED
} dev_state_t;

/* local function prototypes */
static int serial_open(struct tty_struct *tty, struct file *filp);
static void serial_close(struct tty_struct *tty, struct file *filp);
static int serial_write(struct tty_struct *tty, int from_user,
			const unsigned char *buf, int count);
static int serial_write_room(struct tty_struct *tty);
static int serial_chars_in_buffer(struct tty_struct *tty);
static void serial_throttle(struct tty_struct *tty);
static void serial_unthrottle(struct tty_struct *tty);
static int serial_ioctl(struct tty_struct *tty, struct file *file,
			unsigned int cmd, unsigned long arg);
static void serial_set_termios (struct tty_struct *tty, struct termios * old);

typedef struct {
	int read_fifo;
	int write_fifo;
	int ctrl_stat;
	int read_fifo_status;
	int write_fifo_status;
} endpoint_reg_t;

typedef struct pkt {
	int size;
	u8 *bufptr;
	struct pkt *next;
	u8 buf[0];
} pkt_t;

typedef struct {
	pkt_t *head;
	pkt_t *tail;
	int count;
} pkt_list_t;

typedef struct {
	struct usb_endpoint_descriptor *desc;
	endpoint_reg_t *reg;
	// Only one of these are used, unless this is a control ep
	pkt_list_t inlist;
	pkt_list_t outlist;
	unsigned int indma, outdma;	// DMA channel numbers for IN, OUT
	int inirq, outirq;	// DMA buffer done irq numbers
	int max_pkt_size;
	spinlock_t lock;
} endpoint_t;

struct usb_serial_port {
	struct usb_serial *serial;	/* ptr back to the owner of this port */
	struct tty_struct *tty;	/* the coresponding tty for this port */
	unsigned char number;
	char active;		/* someone has this device open */
	spinlock_t port_lock;

	endpoint_t ep_bulkin;
	endpoint_t ep_bulkout;

	wait_queue_head_t write_wait;

	/* task queue for line discipline waking up on send packet complete */
	struct tq_struct send_complete_tq;
	/* task queue for line discipline wakeup on receive packet complete */
	struct tq_struct receive_complete_tq;

	int open_count;		/* number of times this port has been opened */
};

struct usb_serial {
	struct tty_driver *tty_driver;	/* the tty_driver for this device */
	unsigned char minor;	/* the minor number for this device */

	endpoint_t ep_ctrl;

	struct usb_device_descriptor *dev_desc;
	struct usb_interface_descriptor *if_desc;
	struct usb_config_descriptor *conf_desc;
	struct usb_string_descriptor *str_desc[6];

	struct usb_serial_port port[NUM_PORTS];

	dev_state_t state;	// device state
	int suspended;		// suspended flag
	int address;		// device address
	int interface;
	u8 alternate_setting;
	u8 configuration;	// configuration value
	int remote_wakeup_en;
};


static struct usb_device_descriptor dev_desc = {
	bLength:USB_DT_DEVICE_SIZE,
	bDescriptorType:USB_DT_DEVICE,
	bcdUSB:0x0110,		//usb rev 1.0
	bDeviceClass:USB_CLASS_PER_INTERFACE,	//class    (none)
	bDeviceSubClass:0x00,	//subclass (none)
	bDeviceProtocol:0x00,	//protocol (none)
	bMaxPacketSize0:EP0_MAX_PACKET_SIZE,	//max packet size for ep0
	idVendor:0x6d04,	//vendor  id
	idProduct:0x0bc0,	//product id
	bcdDevice:0x0001,	//BCD rev 0.1
	iManufacturer:0x01,	//manufactuer string index
	iProduct:0x02,		//product string index
	iSerialNumber:0x03,	//serial# string index
	bNumConfigurations:0x01	//num configurations
};

static struct usb_endpoint_descriptor ep_desc[] = {
	{
	 // EP2, Bulk IN for Port 0
	      bLength:USB_DT_ENDPOINT_SIZE,
	      bDescriptorType:USB_DT_ENDPOINT,
	      bEndpointAddress:USB_DIR_IN | 0x02,
	      bmAttributes:USB_ENDPOINT_XFER_BULK,
	      wMaxPacketSize:EP2_MAX_PACKET_SIZE,
	      bInterval:0x00	// ignored for bulk
	 },
	{
	 // EP4, Bulk OUT for Port 0
	      bLength:USB_DT_ENDPOINT_SIZE,
	      bDescriptorType:USB_DT_ENDPOINT,
	      bEndpointAddress:USB_DIR_OUT | 0x04,
	      bmAttributes:USB_ENDPOINT_XFER_BULK,
	      wMaxPacketSize:EP4_MAX_PACKET_SIZE,
	      bInterval:0x00	// ignored for bulk
	 },
	{
	 // EP3, Bulk IN for Port 1
	      bLength:USB_DT_ENDPOINT_SIZE,
	      bDescriptorType:USB_DT_ENDPOINT,
	      bEndpointAddress:USB_DIR_IN | 0x03,
	      bmAttributes:USB_ENDPOINT_XFER_BULK,
	      wMaxPacketSize:EP3_MAX_PACKET_SIZE,
	      bInterval:0x00	// ignored for bulk
	 },
	{
	 // EP5, Bulk OUT for Port 1
	      bLength:USB_DT_ENDPOINT_SIZE,
	      bDescriptorType:USB_DT_ENDPOINT,
	      bEndpointAddress:USB_DIR_OUT | 0x05,
	      bmAttributes:USB_ENDPOINT_XFER_BULK,
	      wMaxPacketSize:EP5_MAX_PACKET_SIZE,
	      bInterval:0x00	// ignored for bulk
	 },
};

static struct usb_interface_descriptor if_desc = {
	bLength:USB_DT_INTERFACE_SIZE,
	bDescriptorType:USB_DT_INTERFACE,
	bInterfaceNumber:0x00,
	bAlternateSetting:0x00,
	bNumEndpoints:NUM_EP,
	bInterfaceClass:0xff,
	bInterfaceSubClass:0xab,
	bInterfaceProtocol:0x00,
	iInterface:0x05
};

#define CONFIG_DESC_LEN \
 USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE + NUM_EP*USB_DT_ENDPOINT_SIZE

static struct usb_config_descriptor config_desc = {
	bLength:USB_DT_CONFIG_SIZE,
	bDescriptorType:USB_DT_CONFIG,
	wTotalLength:CONFIG_DESC_LEN,
	bNumInterfaces:0x01,
	bConfigurationValue:0x01,
	iConfiguration:0x04,	// configuration string
	bmAttributes:0xc0,	// self-powered
	MaxPower:20		// 40 mA
};

// These strings will be converted to Unicode before sending
static char *strings[5] = {
	"Alchemy Semiconductor",
	"Alchemy Au1000",
	"1.0",
	"Au1000 UART Config",
	"Au1000 UART Interface"
};

// String[0] is a list of Language IDs supported by this device
static struct usb_string_descriptor string_desc0 = {
	bLength:4,
	bDescriptorType:USB_DT_STRING,
	wData:{0x0409}		// English, US
};


static endpoint_reg_t ep_reg[] = {
	// FIFO's 0 and 1 are EP0 default control
	{USBD_EP0RD, USBD_EP0WR, USBD_EP0CS, USBD_EP0RDSTAT, USBD_EP0WRSTAT},
	// FIFO 2 is EP2, Port 0, bulk IN
	{ -1, USBD_EP2WR, USBD_EP2CS, -1, USBD_EP2WRSTAT },
	// FIFO 4 is EP4, Port 0, bulk OUT
	    {USBD_EP4RD, -1, USBD_EP4CS, USBD_EP3WR, -1},
	// FIFO 3 is EP3, Port 1, bulk IN
	{ -1, USBD_EP3WRSTAT, USBD_EP3CS, -1, USBD_EP3WRSTAT },
	// FIFO 5 is EP5, Port 1, bulk OUT
	    {USBD_EP5RD, -1, USBD_EP5CS, USBD_EP5RDSTAT, -1}
};

static struct {
	unsigned int id;
	const char *str;
} ep_dma_id[] = {
	{ DMA_ID_USBDEV_EP0_TX, "USBDev EP0 IN" },
	{ DMA_ID_USBDEV_EP0_RX, "USBDev EP0 OUT" },
	{ DMA_ID_USBDEV_EP2_TX, "USBDev EP2 IN" },
	{ DMA_ID_USBDEV_EP4_RX, "USBDev EP4 OUT" },
	{ DMA_ID_USBDEV_EP3_TX, "USBDev EP3 IN" },
	{ DMA_ID_USBDEV_EP5_RX, "USBDev EP5 OUT" }
};

static int serial_refcount;
static struct tty_driver serial_tty_driver;
static struct tty_struct *serial_tty[1];
static struct termios *serial_termios[1];
static struct termios *serial_termios_locked[1];
static struct usb_serial usbserial;

#define DIR_OUT 0
#define DIR_IN  (1<<3)

static const u32 au1000_config_table[25] __devinitdata = {
	0x00,
	    ((EP0_MAX_PACKET_SIZE & 0x380) >> 7) |
	    (USB_ENDPOINT_XFER_CONTROL << 4),
	(EP0_MAX_PACKET_SIZE & 0x7f) << 1,
	0x00,
	0x01,

	0x10,
	    ((EP2_MAX_PACKET_SIZE & 0x380) >> 7) | DIR_IN |
	    (USB_ENDPOINT_XFER_BULK << 4),
	(EP2_MAX_PACKET_SIZE & 0x7f) << 1,
	0x00,
	0x02,

	0x20,
	    ((EP3_MAX_PACKET_SIZE & 0x380) >> 7) | DIR_IN |
	    (USB_ENDPOINT_XFER_BULK << 4),
	(EP3_MAX_PACKET_SIZE & 0x7f) << 1,
	0x00,
	0x03,

	0x30,
	    ((EP4_MAX_PACKET_SIZE & 0x380) >> 7) | DIR_OUT |
	    (USB_ENDPOINT_XFER_BULK << 4),
	(EP4_MAX_PACKET_SIZE & 0x7f) << 1,
	0x00,
	0x04,

	0x40,
	    ((EP5_MAX_PACKET_SIZE & 0x380) >> 7) | DIR_OUT |
	    (USB_ENDPOINT_XFER_BULK << 4),
	(EP5_MAX_PACKET_SIZE & 0x7f) << 1,
	0x00,
	0x05
};

static inline endpoint_t *
fifonum_to_ep(struct usb_serial* serial, int fifo_num)
{
	switch (fifo_num) {
	case 0:
	case 1:
		return &serial->ep_ctrl;
	case 2:
		return &serial->port[0].ep_bulkin;
	case 3:
		return &serial->port[1].ep_bulkin;
	case 4:
		return &serial->port[0].ep_bulkout;
	case 5:
		return &serial->port[1].ep_bulkout;
	}

	return NULL;
}

static inline struct usb_serial_port *
fifonum_to_port(struct usb_serial* serial, int fifo_num)
{
	switch (fifo_num) {
	case 2:
	case 4:
		return &serial->port[0];
	case 3:
	case 5:
		return &serial->port[1];
	}

	return NULL;
}

static inline endpoint_t *
epnum_to_ep(struct usb_serial* serial, int ep_num)
{
	switch (ep_num) {
	case 0:
		return &serial->ep_ctrl;
	case 2:
		return &serial->port[0].ep_bulkin;
	case 3:
		return &serial->port[1].ep_bulkin;
	case 4:
		return &serial->port[0].ep_bulkout;
	case 5:
		return &serial->port[1].ep_bulkout;
	}

	return NULL;
}


static inline int
port_paranoia_check(struct usb_serial_port *port, const char *function)
{
	if (!port) {
		dbg("%s - port == NULL", function);
		return -1;
	}
	if (!port->serial) {
		dbg("%s - port->serial == NULL", function);
		return -1;
	}
	if (!port->tty) {
		dbg("%s - port->tty == NULL", function);
		return -1;
	}

	return 0;
}

static inline struct usb_serial*
get_usb_serial (struct usb_serial_port *port, const char *function)
{
	/* if no port was specified, or it fails a paranoia check */
	if (!port || port_paranoia_check(port, function)) {
		/* then say that we dont have a valid usb_serial thing,
		 * which will end up genrating -ENODEV return values */
		return NULL;
	}

	return port->serial;
}


static inline pkt_t *
alloc_packet(int data_size)
{
	pkt_t* pkt = (pkt_t *)kmalloc(sizeof(pkt_t) + data_size, ALLOC_FLAGS);
	if (!pkt)
		return NULL;
	pkt->size = data_size;
	pkt->bufptr = pkt->buf;
#ifndef USBDEV_PIO
	pkt->bufptr = KSEG1ADDR(pkt->bufptr);
#endif
	pkt->next = NULL;
	return pkt;
}


/*
 * Link a packet to the tail of the enpoint's packet list.
 */
static void
link_packet(endpoint_t * ep, pkt_list_t * list, pkt_t * pkt)
{
	unsigned long flags;

	spin_lock_irqsave(&ep->lock, flags);

	if (!list->tail) {
		list->head = list->tail = pkt;
		list->count = 1;
	} else {
		list->tail->next = pkt;
		list->tail = pkt;
		list->count++;
	}

	spin_unlock_irqrestore(&ep->lock, flags);
}

/*
 * Unlink and return a packet from the head of the enpoint's packet list.
 */
static pkt_t *
unlink_packet(endpoint_t * ep, pkt_list_t * list)
{
	unsigned long flags;
	pkt_t *pkt;

	spin_lock_irqsave(&ep->lock, flags);

	pkt = list->head;
	if (!pkt || !list->count) {
		spin_unlock_irqrestore(&ep->lock, flags);
		return NULL;
	}

	list->head = pkt->next;
	if (!list->head) {
		list->head = list->tail = NULL;
		list->count = 0;
	} else
		list->count--;

	spin_unlock_irqrestore(&ep->lock, flags);

	return pkt;
}

/*
 * Create and attach a new packet to the tail of the enpoint's
 * packet list.
 */
static pkt_t *
add_packet(endpoint_t * ep, pkt_list_t * list, int size)
{
	pkt_t *pkt = alloc_packet(size);
	if (!pkt)
		return NULL;

	link_packet(ep, list, pkt);
	return pkt;
}


/*
 * Unlink and free a packet from the head of the enpoint's
 * packet list.
 */
static inline void
free_packet(endpoint_t * ep, pkt_list_t * list)
{
	kfree(unlink_packet(ep, list));
}

static inline void
flush_pkt_list(endpoint_t * ep, pkt_list_t * list)
{
	while (list->count)
		free_packet(ep, list);
}


static inline void
flush_write_fifo(endpoint_t * ep)
{
	if (ep->reg->write_fifo_status >= 0) {
		outl_sync(USBDEV_FSTAT_FLUSH, ep->reg->write_fifo_status);
		udelay(100);
		outl_sync(USBDEV_FSTAT_UF | USBDEV_FSTAT_OF,
			  ep->reg->write_fifo_status);
	}
}


static inline void
flush_read_fifo(endpoint_t * ep)
{
	if (ep->reg->read_fifo_status >= 0) {
		outl_sync(USBDEV_FSTAT_FLUSH, ep->reg->read_fifo_status);
		udelay(100);
		outl_sync(USBDEV_FSTAT_UF | USBDEV_FSTAT_OF,
			  ep->reg->read_fifo_status);
	}
}


static void
endpoint_flush(endpoint_t * ep)
{
	unsigned long flags;

	spin_lock_irqsave(&ep->lock, flags);

	// First, flush all packets
	flush_pkt_list(ep, &ep->inlist);
	flush_pkt_list(ep, &ep->outlist);

	// Now flush the endpoint's h/w FIFO(s)
	flush_write_fifo(ep);
	flush_read_fifo(ep);

	spin_unlock_irqrestore(&ep->lock, flags);
}


static void
endpoint_stall(endpoint_t * ep)
{
	unsigned long flags;
	u32 cs;

	dbg(__FUNCTION__);

	spin_lock_irqsave(&ep->lock, flags);

	cs = inl(ep->reg->ctrl_stat) | USBDEV_CS_STALL;
	outl_sync(cs, ep->reg->ctrl_stat);

	spin_unlock_irqrestore(&ep->lock, flags);
}

static void
endpoint_unstall(endpoint_t * ep)
{
	unsigned long flags;
	u32 cs;

	dbg(__FUNCTION__);

	spin_lock_irqsave(&ep->lock, flags);

	cs = inl(ep->reg->ctrl_stat) & ~USBDEV_CS_STALL;
	outl_sync(cs, ep->reg->ctrl_stat);

	spin_unlock_irqrestore(&ep->lock, flags);
}

static void
endpoint_reset_datatoggle(endpoint_t * ep)
{
	// FIXME: is this possible?
}


#ifdef USBDEV_PIO
static int
endpoint_fifo_read(endpoint_t * ep)
{
	unsigned long flags;
	int read_count = 0;
	u8 *bufptr;
	pkt_t *pkt = ep->outlist.tail;

	if (!pkt)
		return -EINVAL;

	spin_lock_irqsave(&ep->lock, flags);

	bufptr = pkt->bufptr;
	while (inl(ep->reg->read_fifo_status) & USBDEV_FSTAT_FCNT_MASK) {
		*bufptr++ = inl(ep->reg->read_fifo) & 0xff;
		read_count++;
		pkt->size++;
	}
	pkt->bufptr = bufptr;

	spin_unlock_irqrestore(&ep->lock, flags);
	return read_count;
}


static int
endpoint_fifo_write(endpoint_t * ep)
{
	unsigned long flags;
	int write_count = 0;
	u8 *bufptr;
	pkt_t *pkt = ep->inlist.head;

	if (!pkt)
		return -EINVAL;

	spin_lock_irqsave(&ep->lock, flags);

	bufptr = pkt->bufptr;
	while ((inl(ep->reg->write_fifo_status) & USBDEV_FSTAT_FCNT_MASK) <
	       EP_FIFO_DEPTH) {
		if (bufptr < pkt->buf + pkt->size) {
			outl_sync(*bufptr++, ep->reg->write_fifo);
			write_count++;
		} else {
			break;
		}
	}
	pkt->bufptr = bufptr;

	spin_unlock_irqrestore(&ep->lock, flags);
	return write_count;
}
#endif				// USBDEV_PIO

/*
 * This routine is called to restart transmission of a packet.
 * The endpoint's TSIZE must be set to the new packet's size,
 * and DMA to the write FIFO needs to be restarted.
 */
static void
kickstart_send_packet(endpoint_t * ep)
{
	u32 cs;
	pkt_t *pkt = ep->inlist.head;

	dbg(__FUNCTION__ ": pkt=%p", pkt);

	if (!pkt)
		return;

	/*
	 * The write fifo should already be drained if things are
	 * working right, but flush it anyway just in case.
	 */
	flush_write_fifo(ep);
	cs = inl(ep->reg->ctrl_stat) & USBDEV_CS_STALL;
	cs |= (pkt->size << USBDEV_CS_TSIZE_BIT);
	outl_sync(cs, ep->reg->ctrl_stat);
#ifdef USBDEV_PIO
	endpoint_fifo_write(ep);
#else
	disable_dma(ep->indma);
	if (get_dma_active_buffer(ep->indma)) {
		set_dma_count1(ep->indma, pkt->size);
		set_dma_addr1(ep->indma, virt_to_phys(pkt->bufptr));
		enable_dma_buffer1(ep->indma);	// reenable
	} else {
		set_dma_count0(ep->indma, pkt->size);
		set_dma_addr0(ep->indma, virt_to_phys(pkt->bufptr));
		enable_dma_buffer0(ep->indma);	// reenable
	}
	enable_dma(ep->indma);
#endif
}


/*
 * This routine is called when a packet in the inlist has been
 * completed. Frees the completed packet and starts sending the
 * next.
 */
static void
send_packet_complete(endpoint_t * ep)
{
	if (ep->inlist.head)
		dbg(__FUNCTION__ ": pkt=%p, ab=%d",
		    ep->inlist.head, get_dma_active_buffer(ep->indma));

	outl_sync(inl(ep->reg->ctrl_stat) & USBDEV_CS_STALL,
		  ep->reg->ctrl_stat);
	//disable_dma(ep->indma);
	free_packet(ep, &ep->inlist);
	// begin transmitting next packet in the inlist
	if (ep->inlist.count)
		kickstart_send_packet(ep);
}


/*
 * Unlink and return a packet from the head of the given ep's packet
 * outlist. It is the responsibility of the caller to free the packet.
 * The receive complete interrupt adds packets to the tail of this list. 
 */
static pkt_t *
receive_packet(endpoint_t * ep)
{
	pkt_t *pkt = unlink_packet(ep, &ep->outlist);
	//dma_cache_inv((unsigned long)pkt->buf, pkt->size);
	return pkt;
}

/*
 * This routine is called to restart reception of a packet.
 */
static void
kickstart_receive_packet(endpoint_t * ep)
{
	pkt_t *pkt;

	// get and link a new packet for next reception
	if (!(pkt = add_packet(ep, &ep->outlist, ep->max_pkt_size))) {
		err(__FUNCTION__ ": could not alloc new packet");
		return;
	}

	/*
	 * The read fifo should already be drained if things are
	 * working right, but flush it anyway just in case.
	 */
	flush_read_fifo(ep);
#ifndef USBDEV_PIO
	if (get_dma_active_buffer(ep->outdma)) {
		set_dma_count1(ep->outdma, ep->max_pkt_size);
		set_dma_addr1(ep->outdma, virt_to_phys(pkt->bufptr));
		enable_dma_buffer1(ep->outdma);	// reenable
	} else {
		set_dma_count0(ep->outdma, ep->max_pkt_size);
		set_dma_addr0(ep->outdma, virt_to_phys(pkt->bufptr));
		enable_dma_buffer0(ep->outdma);	// reenable
	}
	enable_dma(ep->outdma);
#endif
}


/*
 * This routine is called when a packet in the outlist has been
 * completed (received) and we need to prepare for a new packet
 * to be received. Halts DMA and computes the packet size from the
 * remaining DMA counter. Then prepares a new packet for reception
 * and restarts DMA. FIXME: what if another packet comes in
 * on top of the completed packet? Counter would be wrong.
 */
static void
receive_packet_complete(endpoint_t * ep)
{
	pkt_t *pkt = ep->outlist.tail;

	if (!pkt)
		return;

	disable_dma(ep->outdma);
	pkt->size = ep->max_pkt_size - get_dma_residue(ep->outdma);
#ifdef USBDEV_PIO
	pkt->bufptr = pkt->buf;	// reset bufptr
#endif
	dbg(__FUNCTION__ ": size=%d", pkt->size);

	kickstart_receive_packet(ep);
}



/*
 * Add a new packet to the tail of the given ep's packet
 * inlist. The transmit complete interrupt frees packets from
 * the head of this list.
 */
static int
send_packet(endpoint_t * ep, u8 * data, int data_len, int from_user)
{
	unsigned long flags;
	pkt_list_t *list = &ep->inlist;
	pkt_t *pkt;

	if (!data || !data_len)
		return 0;

	if (!(pkt = alloc_packet(data_len))) {
		err(__FUNCTION__ ": could not alloc new packet");
		return -ENOMEM;
	}

	if (from_user)
		copy_from_user(pkt->bufptr, data, data_len);
	else
		memcpy(pkt->bufptr, data, data_len);
	au_sync();

	//dma_cache_wback_inv((unsigned long)pkt->buf, data_len);

	link_packet(ep, list, pkt);

	spin_lock_irqsave(&ep->lock, flags);

	dbg(__FUNCTION__ ": size=%d, list count=%d", pkt->size, list->count);

	if (list->count == 1) {
		/*
		 * if the packet count is one, it means the list was empty,
		 * and no more data will go out this ep until we kick-start
		 * it again.
		 */
		kickstart_send_packet(ep);
	}

	spin_unlock_irqrestore(&ep->lock, flags);
	return data_len;
}


// SETUP packet request parser
static void
process_setup (struct usb_serial* serial, devrequest* setup)
{
	int desc_len, strnum;

	dbg(__FUNCTION__ ": req %d", setup->request);

	switch (setup->request) {
	case USB_REQ_SET_ADDRESS:
		serial->address = le16_to_cpu(setup->value);
		dbg(__FUNCTION__ ": our address=%d", serial->address);
		if (serial->address > 127 || serial->state == CONFIGURED) {
			// usb spec doesn't tell us what to do, so just go to
			// default state
			serial->state = DEFAULT;
			serial->address = 0;
		} else if (serial->address)
			serial->state = ADDRESS;
		else
			serial->state = DEFAULT;
		break;
	case USB_REQ_GET_DESCRIPTOR:
		desc_len = le16_to_cpu(setup->length);
		switch (le16_to_cpu(setup->value) >> 8) {
		case USB_DT_DEVICE:
			// send device descriptor!
			desc_len = desc_len > serial->dev_desc->bLength ?
			    serial->dev_desc->bLength : desc_len;
			dbg("sending device desc, size=%d", desc_len);
			send_packet(&serial->ep_ctrl, (u8*)serial->dev_desc,
				    desc_len, 0);
			break;
		case USB_DT_CONFIG:
			// If the config descr index in low-byte of
			// setup->value	is valid, send config descr,
			// otherwise stall ep0.
			if ((le16_to_cpu(setup->value) & 0xff) == 0) {
				// send config descriptor!
				if (desc_len <= USB_DT_CONFIG_SIZE) {
					dbg("sending partial config desc, size=%d",
					     desc_len);
					send_packet(&serial->ep_ctrl,
						    (u8*)serial->conf_desc,
						    desc_len, 0);
				} else {
					u8 full_conf_desc[CONFIG_DESC_LEN];
					int i, index = 0;
					memcpy(&full_conf_desc[index],
					       serial->conf_desc,
					       USB_DT_CONFIG_SIZE);
					index += USB_DT_CONFIG_SIZE;
					memcpy(&full_conf_desc[index],
					       serial->if_desc,
					       USB_DT_INTERFACE_SIZE);
					index += USB_DT_INTERFACE_SIZE;
					for (i = 0; i < NUM_PORTS; i++) {
						memcpy(&full_conf_desc[index],
						       serial->port[i].ep_bulkin.desc,
						       USB_DT_ENDPOINT_SIZE);
						index += USB_DT_ENDPOINT_SIZE;
						memcpy(&full_conf_desc[index],
						       serial->port[i].ep_bulkout.desc,
						       USB_DT_ENDPOINT_SIZE);
						index += USB_DT_ENDPOINT_SIZE;
					}
					dbg("sending whole config desc, size=%d, our size=%d",
					     desc_len, CONFIG_DESC_LEN);
					desc_len = desc_len > CONFIG_DESC_LEN ?
					    CONFIG_DESC_LEN : desc_len;
					send_packet(&serial->ep_ctrl,
						    full_conf_desc, desc_len, 0);
				}
			} else
				endpoint_stall(&serial->ep_ctrl);
			break;
		case USB_DT_STRING:
			// If the string descr index in low-byte of setup->value
			// is valid, send string descr, otherwise stall ep0.
			strnum = le16_to_cpu(setup->value) & 0xff;
			if (strnum >= 0 && strnum < 6) {
				struct usb_string_descriptor *desc =
				    serial->str_desc[strnum];
				desc_len = desc_len > desc->bLength ?
					desc->bLength : desc_len;
				dbg("sending string desc %d", strnum);
				send_packet(&serial->ep_ctrl, (u8 *) desc,
					    desc_len, 0);
			} else
				endpoint_stall(&serial->ep_ctrl);
			break;
		default:	// Invalid request
			dbg("invalid get desc=%d, stalled",
			    le16_to_cpu(setup->value) >> 8);
			endpoint_stall(&serial->ep_ctrl);	// Stall endpoint 0
			break;
		}
		break;
	case USB_REQ_SET_DESCRIPTOR:
		// FIXME: anything to set here?
		break;
	case USB_REQ_GET_INTERFACE:
		// interface must be zero.
		if ((le16_to_cpu(setup->index) & 0xff) ||
		    serial->state == ADDRESS) {
			// FIXME: respond with "request error". how?
		} else if (serial->state == CONFIGURED) {
			// send serial->alternate_setting
			dbg("sending alt setting");
			send_packet(&serial->ep_ctrl,
				    &serial->alternate_setting, 1, 0);
		}
		break;
	case USB_REQ_SET_INTERFACE:
		if (serial->state == ADDRESS) {
			// FIXME: respond with "request error". how?
		} else if (serial->state == CONFIGURED) {
			serial->interface = le16_to_cpu(setup->index) & 0xff;
			serial->alternate_setting =
			    le16_to_cpu(setup->value) & 0xff;
			// interface and alternate_setting must be zero
			if (serial->interface || serial->alternate_setting) {
				// FIXME: respond with "request error". how?
			}
		}
		break;
	case USB_REQ_SET_CONFIGURATION:
		// set active config to low-byte of serial.value
		serial->configuration = le16_to_cpu(setup->value) & 0xff;
		dbg("set config, config=%d", serial->configuration);
		if (!serial->configuration && serial->state > DEFAULT)
			serial->state = ADDRESS;
		else if (serial->configuration == 1)
			serial->state = CONFIGURED;
		else {
			// FIXME: "respond with request error" - how?
		}
		break;
	case USB_REQ_GET_CONFIGURATION:
		// send serial->configuration
		dbg("sending config");
		send_packet(&serial->ep_ctrl, &serial->configuration, 1, 0);
		break;
	case USB_REQ_GET_STATUS:
		// FIXME: looks like the h/w handles this one
		switch (setup->requesttype) {
		case 0x80:	// Device
			// FIXME: send device status
			break;
		case 0x81:	// Interface
			// FIXME: send interface status
			break;
		case 0x82:	// End Point
			// FIXME: send endpoint status
			break;
		default:	// Invalid Command
			endpoint_stall(&serial->ep_ctrl);	// Stall End Point 0
			break;
		}
		break;
	case USB_REQ_CLEAR_FEATURE:
		switch (setup->requesttype) {
		case 0x00:	// Device
			if ((le16_to_cpu(setup->value) & 0xff) == 1)
				serial->remote_wakeup_en = 0;
			else
				endpoint_stall(&serial->ep_ctrl);
			break;
		case 0x02:	// End Point
			if ((le16_to_cpu(setup->value) & 0xff) == 0) {
				endpoint_t *ep =
				    epnum_to_ep(serial,
						    le16_to_cpu(setup->index) & 0xff);

				endpoint_unstall(ep);
				endpoint_reset_datatoggle(ep);
			} else
				endpoint_stall(&serial->ep_ctrl);
			break;
		}
		break;
	case USB_REQ_SET_FEATURE:
		switch (setup->requesttype) {
		case 0x00:	// Device
			if ((le16_to_cpu(setup->value) & 0xff) == 1)
				serial->remote_wakeup_en = 1;
			else
				endpoint_stall(&serial->ep_ctrl);
			break;
		case 0x02:	// End Point
			if ((le16_to_cpu(setup->value) & 0xff) == 0) {
				endpoint_t *ep =
				    epnum_to_ep(serial,
						    le16_to_cpu(setup->index) & 0xff);

				endpoint_stall(ep);
			} else
				endpoint_stall(&serial->ep_ctrl);
			break;
		}
		break;
	default:
		endpoint_stall(&serial->ep_ctrl);	// Stall End Point 0
		break;
	}
}


/*
 * A complete packet (SETUP, DATA0, or DATA1) has been received
 * on the given endpoint's fifo.
 */
static void
process_complete (struct usb_serial* serial, int fifo_num)
{
	endpoint_t *ep = fifonum_to_ep(serial, fifo_num);
	struct usb_serial_port *port = NULL;
	pkt_t *pkt = 0;
	u32 cs;

	cs = inl(ep->reg->ctrl_stat);

	switch (fifo_num) {
	case 0:
		spin_lock(&ep->lock);
		// complete packet and prepare a new packet
		receive_packet_complete(ep);

		// Get it immediately from endpoint.
		if (!(pkt = receive_packet(ep))) {
			spin_unlock(&ep->lock);
			return;
		}
	
		// SETUP packet received ?
		//if (cs & USBDEV_CS_SU) { FIXME: uncomment!
			if (pkt->size == sizeof(devrequest)) {
				devrequest setup;
			if ((cs & (USBDEV_CS_NAK | USBDEV_CS_ACK)) ==
			    USBDEV_CS_ACK)
					dbg("got SETUP");
				else
					dbg("got NAK SETUP, cs=%08x", cs);
			memcpy(&setup, pkt->bufptr, sizeof(devrequest));
				process_setup(serial, &setup);
			//} else  FIXME: uncomment!
			//dbg(__FUNCTION__ ": wrong size SETUP received");
		} else {
			// DATAx packet received on endpoint 0
			// FIXME: will need a state machine for control
			// OUT transactions
			dbg("got DATAx on EP0, size=%d, cs=%08x",
			    pkt->size, cs);
		}

		spin_unlock(&ep->lock);
		// we're done processing the packet, free it
		kfree(pkt);
		break;

	case 4:
	case 5:
		port = fifonum_to_port(serial, fifo_num);
		dbg("got DATAx on port %d, cs=%08x", port->number, cs);
		spin_lock(&ep->lock);
		receive_packet_complete(ep);
		spin_unlock(&ep->lock);
		// mark a bh to push this data up to the tty
		queue_task(&port->receive_complete_tq, &tq_immediate);
		mark_bh(IMMEDIATE_BH);
		break;

	default:
		break;
	}
}



// This ISR needs to ack both the complete and suspend events
static void
req_sus_intr (int irq, void *dev_id, struct pt_regs *regs)
{
	struct usb_serial *serial = (struct usb_serial *) dev_id;
	int i;
	u32 status;

	status = inl(USB_DEV_INT_STATUS);
	outl_sync(status, USB_DEV_INT_STATUS);	// ack'em

#ifdef USBDEV_PIO
	for (i = 0; i < 6; i++) {
		if (status & (1 << (USBDEV_INT_HF_BIT + i))) {
			endpoint_t *ep = fifonum_to_ep(serial, i);

			if (ep->desc->bEndpointAddress & USB_DIR_IN)
				endpoint_fifo_write(ep);
			else
				endpoint_fifo_read(ep);
		}
	}
#endif

	for (i = 0; i < 6; i++) {
		if (status & (1 << i))
			process_complete(serial, i);
	}
}


static void
dma_done_ctrl(struct usb_serial* serial)
{
	endpoint_t *ep = &serial->ep_ctrl;
	u32 cs0, buff_done;

	spin_lock(&ep->lock);
	cs0 = inl(ep->reg->ctrl_stat);

	// first check packet transmit done
	if ((buff_done = get_dma_buffer_done(ep->indma)) != 0) {
		// transmitted a DATAx packet on control endpoint 0
		// clear DMA done bit
		if (buff_done == DMA_D0)
			clear_dma_done0(ep->indma);
		else
			clear_dma_done1(ep->indma);

		send_packet_complete(ep);
	}

	/*
	 * Now check packet receive done. Shouldn't get these,
	 * the receive packet complete intr should happen
	 * before the DMA done intr occurs.
	 */
	if ((buff_done = get_dma_buffer_done(ep->outdma)) != 0) {
		// clear DMA done bit
		if (buff_done == DMA_D0)
			clear_dma_done0(ep->outdma);
		else
			clear_dma_done1(ep->outdma);
	}

	spin_unlock(&ep->lock);
}

static void
dma_done_port(struct usb_serial_port * port)
{
	endpoint_t *ep;
	u32 buff_done;

	// first check packet transmit done (bulk IN ep)
	ep = &port->ep_bulkin;
	spin_lock(&ep->lock);
	if ((buff_done = get_dma_buffer_done(ep->indma)) != 0) {
		// transmitted a DATAx packet on the port's bulk IN endpoint
		// clear DMA done bit
		if (buff_done == DMA_D0)
			clear_dma_done0(ep->indma);
		else
			clear_dma_done1(ep->indma);

		send_packet_complete(ep);
		// mark a bh to wakeup any tty write system call on the port.
		queue_task(&port->send_complete_tq, &tq_immediate);
		mark_bh(IMMEDIATE_BH);
	}
	spin_unlock(&ep->lock);

	/*
	 * Now check packet receive done (bulk OUT ep). Shouldn't
	 * get these, the receive packet complete intr should happen
	 * before the DMA done intr occurs.
	 */
	ep = &port->ep_bulkout;
	spin_lock(&ep->lock);
	if ((buff_done = get_dma_buffer_done(ep->outdma)) != 0) {
		// received a DATAx packet on the port's bulk OUT endpoint
		// clear DMA done bit
		if (buff_done == DMA_D0)
			clear_dma_done0(ep->outdma);
		else
			clear_dma_done1(ep->outdma);
	}
	spin_unlock(&ep->lock);
}


// This ISR needs to handle dma done events for ALL endpoints!
static void
dma_done_intr (int irq, void *dev_id, struct pt_regs *regs)
{
	struct usb_serial *serial = (struct usb_serial *) dev_id;
	int i;

	dma_done_ctrl(serial);
	for (i = 0; i < NUM_PORTS; i++)
		dma_done_port(&serial->port[i]);
}



/*****************************************************************************
 * Here begins the tty driver interface functions
 *****************************************************************************/

static int serial_open(struct tty_struct *tty, struct file *filp)
{
	int portNumber;
	struct usb_serial_port *port;
	struct usb_serial *serial = &usbserial;
	unsigned long flags;

	/* initialize the pointer incase something fails */
	tty->driver_data = NULL;

	MOD_INC_USE_COUNT;

	/* set up our port structure making the tty driver remember
	   our port object, and us it */
	portNumber = MINOR(tty->device) - serial->minor;
	port = &serial->port[portNumber];
	tty->driver_data = port;
	port->tty = tty;

	if (port_paranoia_check(port, __FUNCTION__))
		return -ENODEV;

	dbg(__FUNCTION__ " - port %d", port->number);

	spin_lock_irqsave(&port->port_lock, flags);

	++port->open_count;

	if (!port->active) {
		port->active = 1;

		/*
		 * force low_latency on so that our tty_push actually forces
		 * the data through, otherwise it is scheduled, and with high
		 * data rates (like with OHCI) data can get lost.
		 */
		port->tty->low_latency = 1;

	}

	spin_unlock_irqrestore(&port->port_lock, flags);

	return 0;
}


static void serial_close(struct tty_struct *tty, struct file *filp)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);
	unsigned long flags;

	if (!serial)
		return;

	dbg(__FUNCTION__ " - port %d", port->number);

	if (!port->active) {
		dbg(__FUNCTION__ " - port not opened");
		return;
	}

	spin_lock_irqsave(&port->port_lock, flags);

	--port->open_count;

	if (port->open_count <= 0) {
		port->active = 0;
		port->open_count = 0;
	}

	spin_unlock_irqrestore(&port->port_lock, flags);
	MOD_DEC_USE_COUNT;
}


static int serial_write(struct tty_struct *tty, int from_user,
			const unsigned char *buf, int count)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);
	endpoint_t *ep = &port->ep_bulkin;

	if (!serial)
		return -ENODEV;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not opened");
		return -EINVAL;
	}

	if (count == 0) {
		dbg(__FUNCTION__ " - write request of 0 bytes");
		return (0);
	}

	count = (count > ep->max_pkt_size) ? ep->max_pkt_size : count;
	send_packet(ep, (u8 *) buf, count, from_user);

	return (count);
}


static int serial_write_room(struct tty_struct *tty)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);
	endpoint_t *ep = &port->ep_bulkin;

	if (!serial)
		return -ENODEV;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not open");
		return -EINVAL;
	}

	return ep->max_pkt_size;
}


static int serial_chars_in_buffer(struct tty_struct *tty)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);
	endpoint_t *ep = &port->ep_bulkin;
	pkt_list_t *list = &ep->inlist;
	pkt_t *scan;
	unsigned long flags;
	int chars = 0;

	if (!serial)
		return -ENODEV;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not open");
		return -EINVAL;
	}

	spin_lock_irqsave(&ep->lock, flags);
	for (scan = list->head; scan; scan = scan->next)
		chars += scan->size;
	spin_unlock_irqrestore(&ep->lock, flags);

	return (chars);
}


static void serial_throttle(struct tty_struct *tty)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);

	if (!serial)
		return;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not open");
		return;
	}
	// FIXME: anything to do?
}


static void serial_unthrottle(struct tty_struct *tty)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);

	if (!serial)
		return;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not open");
		return;
	}
	// FIXME: anything to do?
}


static int serial_ioctl(struct tty_struct *tty, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);

	if (!serial)
		return -ENODEV;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not open");
		return -ENODEV;
	}
	// FIXME: need any IOCTLs?

	return -ENOIOCTLCMD;
}


static void serial_set_termios(struct tty_struct *tty, struct termios *old)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);

	if (!serial)
		return;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not open");
		return;
	}
	// FIXME: anything to do?
}


static void serial_break(struct tty_struct *tty, int break_state)
{
	struct usb_serial_port *port =
	    (struct usb_serial_port *) tty->driver_data;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);

	if (!serial)
		return;

	if (!port->active) {
		dbg(__FUNCTION__ " - port not open");
		return;
	}
	// FIXME: anything to do?
}


static void port_send_complete(void *private)
{
	struct usb_serial_port *port = (struct usb_serial_port *) private;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);
	struct tty_struct *tty;

	dbg(__FUNCTION__ " - port %d", port->number);

	if (!serial) {
		return;
	}

	tty = port->tty;
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc.write_wakeup) {
		dbg(__FUNCTION__ " - write wakeup call.");
		(tty->ldisc.write_wakeup) (tty);
	}

	wake_up_interruptible(&tty->write_wait);
}


static void port_receive_complete(void *private)
{
	struct usb_serial_port *port = (struct usb_serial_port *) private;
	struct usb_serial *serial = get_usb_serial(port, __FUNCTION__);
	struct tty_struct *tty = port->tty;
	pkt_t *pkt;
	int i;

	dbg(__FUNCTION__ " - port %d", port->number);

	if (!serial) {
		return;
	}

	if (!(pkt = receive_packet(&port->ep_bulkout)))
		return;

	for (i = 0; i < pkt->size; i++) {
		/* if we insert more than TTY_FLIPBUF_SIZE characters,
		   we drop them. */
		if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
			tty_flip_buffer_push(tty);
		}
		/* this doesn't actually push the data through
		   unless tty->low_latency is set */
		tty_insert_flip_char(tty, pkt->bufptr[i], 0);
	}
	tty_flip_buffer_push(tty);

	// we're done processing the packet, free it
	kfree(pkt);
}


static struct tty_driver serial_tty_driver = {
	magic:TTY_DRIVER_MAGIC,
	driver_name:"usbdev-serial",
	name:"usb/ttsdev/%d",
	major:SERIAL_TTY_MAJOR,
	minor_start:0,
	num:1,
	type:TTY_DRIVER_TYPE_SERIAL,
	subtype:SERIAL_TYPE_NORMAL,
	flags:TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS,
	refcount:&serial_refcount,
	table:serial_tty,
	termios:serial_termios,
	termios_locked:serial_termios_locked,

	open:serial_open,
	close:serial_close,
	write:serial_write,
	write_room:serial_write_room,
	ioctl:serial_ioctl,
	set_termios:serial_set_termios,
	throttle:serial_throttle,
	unthrottle:serial_unthrottle,
	break_ctl:serial_break,
	chars_in_buffer:serial_chars_in_buffer,
};


void usbdev_serial_exit(void)
{
	endpoint_t *ep;
	int i;

	outl_sync(0, USB_DEV_INT_ENABLE);	// disable usb dev ints
	outl_sync(0, USB_DEV_ENABLE);	// disable usb dev

	// first free all control endpoint resources
	ep = &usbserial.ep_ctrl;
	free_irq(AU1000_USB_DEV_REQ_INT, &usbserial);
	free_irq(AU1000_USB_DEV_SUS_INT, &usbserial);
	free_irq(ep->inirq, &usbserial);
	//free_irq(ep->outirq, &usbserial);
	free_au1000_dma(ep->indma);
	free_au1000_dma(ep->outdma);
	endpoint_flush(ep);

	// now free all port resources
	for (i = 0; i < NUM_PORTS; i++) {
		// free port's bulk IN endpoint resources
		ep = &usbserial.port[i].ep_bulkin;
		free_irq(ep->inirq, &usbserial);
		free_au1000_dma(ep->indma);
		endpoint_flush(ep);

		// free port's bulk OUT endpoint resources
		ep = &usbserial.port[i].ep_bulkout;
		//free_irq(ep->outirq, &usbserial);
		free_au1000_dma(ep->outdma);
		endpoint_flush(ep);

		tty_unregister_devfs(&serial_tty_driver, i);
		info("usbdev serial converter now disconnected from ttyUSBdev%d",
		     i);
	}

	kfree(usbserial.str_desc[0]);
	tty_unregister_driver(&serial_tty_driver);
}

int usbdev_serial_init(void)
{
	struct usb_serial_port *port;
	endpoint_t *ep;
	void *str_desc_buf;
	int str_desc_len;
	int i;

	/* register the tty driver */
	serial_tty_driver.init_termios = tty_std_termios;
	serial_tty_driver.init_termios.c_cflag =
	    B9600 | CS8 | CREAD | HUPCL | CLOCAL;

	if (tty_register_driver(&serial_tty_driver)) {
		err(__FUNCTION__ ": failed to register tty driver");
		return -1;
	}

	memset(&usbserial, 0, sizeof(struct usb_serial));
	usbserial.minor = 0;

	usbserial.state = DEFAULT;

	usbserial.dev_desc = &dev_desc;
	usbserial.if_desc = &if_desc;
	usbserial.conf_desc = &config_desc;

	/*
	 * initialize the string descriptors
	 */

	/* alloc buffer big enough for all string descriptors */
	str_desc_len = string_desc0.bLength;
	for (i = 0; i < 5; i++)
		str_desc_len += 2 + 2 * strlen(strings[i]);
	str_desc_buf = (void *) kmalloc(str_desc_len, ALLOC_FLAGS);
	if (!str_desc_buf) {
		err(__FUNCTION__ ": failed to alloc string descriptors");
		return -1;
	}

	usbserial.str_desc[0] = (struct usb_string_descriptor *)str_desc_buf;
	memcpy(usbserial.str_desc[0], &string_desc0, string_desc0.bLength);
	usbserial.str_desc[1] = (struct usb_string_descriptor *)
	    (str_desc_buf + string_desc0.bLength);
	for (i = 1; i < 6; i++) {
		struct usb_string_descriptor *desc = usbserial.str_desc[i];
		char *str = strings[i - 1];
		int j, str_len = strlen(str);

		desc->bLength = 2 + 2 * str_len;
		desc->bDescriptorType = USB_DT_STRING;
		for (j = 0; j < str_len; j++) {
			desc->wData[j] = (u16) str[j];
		}
		if (i < 5)
			usbserial.str_desc[i + 1] =
			    (struct usb_string_descriptor *)
			    ((u8 *) desc + desc->bLength);
	}

	// request the USB device transfer complete interrupt
	if (request_irq(AU1000_USB_DEV_REQ_INT, req_sus_intr, SA_SHIRQ,
			"USBdev req", &usbserial)) {
		err("Can't get device request intr\n");
		goto err_out;
	}
	// request the USB device suspend interrupt
	if (request_irq(AU1000_USB_DEV_SUS_INT, req_sus_intr, SA_SHIRQ,
			"USBdev sus", &usbserial)) {
		err("Can't get device suspend intr\n");
		goto err_out;
	}

	// Initialize default control endpoint
	ep = &usbserial.ep_ctrl;
	spin_lock_init(&ep->lock);
	ep->desc = NULL;	// ep0 has no ep descriptor
	ep->reg = &ep_reg[0];
	ep->max_pkt_size = usbserial.dev_desc->bMaxPacketSize0;
	ep->indma = ep->outdma = -1;
	if ((ep->indma = request_au1000_dma(ep_dma_id[0].id,
					    ep_dma_id[0].str)) < 0) {
		err("Can't get %s DMA\n", ep_dma_id[0].str);
		goto err_out;
	}
	if ((ep->outdma = request_au1000_dma(ep_dma_id[1].id,
					     ep_dma_id[1].str)) < 0) {
		err("Can't get %s DMA\n", ep_dma_id[1].str);
		goto err_out;
	}
	ep->inirq = get_dma_done_irq(ep->indma);
	ep->outirq = get_dma_done_irq(ep->outdma);
	// allocate EP0's DMA done interrupts.
	if (request_irq(ep->inirq, dma_done_intr, SA_INTERRUPT,
			"USBdev ep0 IN", &usbserial)) {
		err("Can't get ep0 IN dma done irq\n");
		goto err_out;
	}
#if 0
	if (request_irq(ep->outirq, dma_done_intr, SA_INTERRUPT,
			"USBdev ep0 OUT", &usbserial)) {
		err("Can't get ep0 OUT dma done irq\n");
		goto err_out;
	}
#endif

	/* initialize the devfs nodes for this device and let the user
	   know what ports we are bound to */
	for (i = 0; i < NUM_PORTS; ++i) {
		tty_register_devfs(&serial_tty_driver, 0, i);
		info("usbdev serial attached to ttyUSBdev%d (or devfs usb/ttsdev/%d)", 
		     i, i);
		port = &usbserial.port[i];
		port->serial = &usbserial;
		port->number = i;
		port->send_complete_tq.routine = port_send_complete;
		port->send_complete_tq.data = port;
		port->receive_complete_tq.routine = port_receive_complete;
		port->receive_complete_tq.data = port;
		spin_lock_init(&port->port_lock);

		// Initialize the port's bulk IN endpoint
		ep = &port->ep_bulkin;
		spin_lock_init(&ep->lock);
		ep->desc = &ep_desc[NUM_PORTS * i];
		ep->reg = &ep_reg[1 + NUM_PORTS * i];
		ep->max_pkt_size = ep->desc->wMaxPacketSize;
		ep->indma = ep->outdma = -1;
		if ((ep->indma =
		     request_au1000_dma(ep_dma_id[2+NUM_PORTS*i].id,
				 ep_dma_id[2 + NUM_PORTS * i].str)) < 0) {
			err("Can't get %s DMA\n",
			    ep_dma_id[2 + NUM_PORTS * i].str);
			goto err_out;
		}
		ep->inirq = get_dma_done_irq(ep->indma);
		if (request_irq(ep->inirq, dma_done_intr, SA_INTERRUPT,
				"USBdev bulk IN", &usbserial)) {
			err("Can't get port %d bulk IN dma done irq\n", i);
			goto err_out;
		}
		// Initialize the port's bulk OUT endpoint
		ep = &port->ep_bulkout;
		spin_lock_init(&ep->lock);
		ep->desc = &ep_desc[NUM_PORTS * i + 1];
		ep->reg = &ep_reg[1 + NUM_PORTS * i + 1];
		ep->max_pkt_size = ep->desc->wMaxPacketSize;
		ep->indma = ep->outdma = -1;
		if ((ep->outdma =
		     request_au1000_dma(ep_dma_id[2+NUM_PORTS*i + 1].id,
					ep_dma_id[2+NUM_PORTS*i + 1].str)) < 0) {
			err("Can't get %s DMA\n",
			    ep_dma_id[2 + NUM_PORTS * i + 1].str);
			goto err_out;
		}
		ep->outirq = get_dma_done_irq(ep->outdma);
#if 0
		if (request_irq(ep->outirq, dma_done_intr, SA_INTERRUPT,
				"USBdev bulk OUT", &usbserial)) {
			err("Can't get port %d bulk OUT dma done irq\n", i);
			goto err_out;
		}
#endif
	}

	// enable device controller
	outl_sync(0x0002, USB_DEV_ENABLE);
	udelay(100);
	outl_sync(0x0003, USB_DEV_ENABLE);
	udelay(100);
	for (i = 0; i < sizeof(au1000_config_table) / sizeof(u32); ++i)
		outl_sync(au1000_config_table[i], USB_DEV_CONFIG);

	// Flush the endpoint buffers and FIFOs
	ep = &usbserial.ep_ctrl;
	endpoint_flush(ep);
	// start packet reception on control ep
	kickstart_receive_packet(ep);

	for (i = 0; i < NUM_PORTS; ++i) {
		struct usb_serial_port *port = &usbserial.port[i];
		endpoint_flush(&port->ep_bulkin);
		endpoint_flush(&port->ep_bulkout);
		// start packet reception on bulk OUT endpoint
		kickstart_receive_packet(&port->ep_bulkout);
	}

	/*
	 * Enable Receive FIFO Complete interrupts only. Transmit
	 * complete is being handled by the DMA done interrupts.
	 */
	outl_sync(0x31, USB_DEV_INT_ENABLE);

	return 0;

      err_out:
	usbdev_serial_exit();
	return -1;
}


module_init(usbdev_serial_init);
module_exit(usbdev_serial_exit);
