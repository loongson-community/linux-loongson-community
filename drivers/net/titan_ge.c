/*
 * drivers/net/titan_ge.c - Driver for Titan ethernet ports
 *
 * Copyright (C) 2003 PMC-Sierra Inc.
 * Author : Manish Lachwani (lachwani@pmc-sierra.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/fcntl.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/ip.h>
#include <linux/init.h>
#include <linux/in.h>
#include <linux/pci.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/mii.h>
/* For MII specifc registers, titan_mdio.h should be included */
#include <net/ip.h>

#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/pgtable.h>
#include <asm/system.h>

#include "titan_ge.h"
#include "titan_mdio.h"

/* Some constants */
#define EXTRA_BYTES 	32
#define WRAP       	ETH_HLEN + 2 + 4 + 16
#define	BUFFER_MTU	netdev->mtu + WRAP

/* Debugging info only */
#define	TITAN_DEBUG

/* Static Function Declarations  */
static int titan_ge_eth_open(struct net_device *);
static int titan_ge_eth_stop(struct net_device *);
static int titan_ge_change_mtu(struct net_device *, int);
static struct net_device_stats *titan_ge_get_stats(struct net_device *);
static int titan_ge_init_rx_desc_ring(titan_ge_port_info *, int, int, unsigned long,
                                        unsigned long, unsigned long);
static int titan_ge_init_tx_desc_ring(titan_ge_port_info *, int, unsigned long, unsigned long);

static int titan_ge_open(struct net_device *);
static int titan_ge_start_xmit(struct sk_buff *, struct net_device *);
static int titan_ge_stop(struct net_device *);
static void titan_ge_int_handler(int, void *, struct pt_regs *);
static int titan_ge_set_mac_address(struct net_device *, void *);

static unsigned long titan_ge_tx_coal(unsigned long);
static unsigned long titan_ge_rx_coal(unsigned long);

static void titan_ge_port_reset(unsigned int);
static int titan_ge_free_tx_queue(struct net_device *, unsigned long);
static void titan_ge_rx_task(void *);
static int titan_ge_port_start(titan_ge_port_info *);

static int titan_ge_init(int);
static int titan_ge_return_tx_desc(titan_ge_port_info *, titan_ge_packet *);
int titan_ge_receive_queue(struct net_device *, unsigned int);

/* MAC Address */
/* XXX For testing purposes, use any value */
unsigned char titan_ge_mac_addr_base[6] = "00:11:22:33:44:55";

/*
 * Reset the Marvel PHY 
 */
static void titan_ge_reset_phy(void)
{
	int 		err = 0;
	unsigned int	reg_data;

	/* Reset the PHY for the above values to take effect */
        err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, MII_BMCR, &reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0 \n");
                return TITAN_GE_ERROR;
        }

        reg_data |= 0x8000;
        err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, MII_BMCR, reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0 \n");
                return TITAN_GE_ERROR;
        }
}

/*
 * Wait for sometime to check if Auto-Neg has completed 
 */
static int titan_ge_wait_autoneg(void)
{
	int 		err = 0, i = 0;
	unsigned int	reg_data;

	for (i = 0; i < PHY_ANEG_TIME_WAIT; i++) {
		err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, MII_BMSR, &reg_data);
        	if (err == TITAN_GE_MDIO_ERROR) {
                	printk(KERN_ERR "Could not read PHY status register 0x1 \n");
	                return TITAN_GE_MDIO_ERROR;
        	}

		if (reg_data & 0x10)
			return TITAN_GE_MDIO_GOOD;

		msec_delay(100);
	}

	if ( !(reg_data & 0x10)) {
              printk(KERN_ERR "Auto-Negotiation did not complete successfully \n");
	      return TITAN_GE_MDIO_ERROR;
        }

	return TITAN_GE_MDIO_GOOD;
}

/* 
 * Get the speed/duplex from the PHY and put it into the main Titan structure
 */
static int titan_ge_get_speed(titan_ge_port_info *titan_ge_eth)
{
	int		err = 0;
	unsigned int	reg_data = 0;

	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_MII_EXTENDED, &reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
        	printk(KERN_ERR "Could not read PHY status register 0x0f \n");
		return TITAN_GE_MDIO_ERROR;
	}

	/* Check for 1000 Mbps first */
	if (reg_data & 0x1010) {
		titan_ge_eth->duplex = TITAN_GE_FULL_DUPLEX;
		titan_ge_eth->speed = TITAN_GE_SPEED_1000;
		return TITAN_GE_MDIO_GOOD;
	}

	if (reg_data & 0x0101) {
		titan_ge_eth->duplex = TITAN_GE_HALF_DUPLEX;
		titan_ge_eth->speed = TITAN_GE_SPEED_1000;
		return TITAN_GE_MDIO_GOOD;
	}

	/* Check for 100 Mbps next */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, MII_BMSR, &reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY status register 0x01 \n");
		return TITAN_GE_MDIO_ERROR;
	}

	if (reg_data & 0x4000) {
		titan_ge_eth->duplex = TITAN_GE_FULL_DUPLEX;
		titan_ge_eth->speed = TITAN_GE_SPEED_100;
		return TITAN_GE_MDIO_GOOD;
	}

	if (reg_data & 0x2000) {
		titan_ge_eth->duplex = TITAN_GE_HALF_DUPLEX;
		titan_ge_eth->speed = TITAN_GE_SPEED_100;
		return TITAN_GE_MDIO_GOOD;
	}

	/* Check for 10 Mbps */
	if (reg_data & 0x1000) {
		titan_ge_eth->duplex = TITAN_GE_FULL_DUPLEX;
		titan_ge_eth->speed = TITAN_GE_SPEED_10;
		return TITAN_GE_MDIO_GOOD;
	}

	if (reg_data & 0x0800) {
		titan_ge_eth->duplex = TITAN_GE_HALF_DUPLEX;
		titan_ge_eth->speed = TITAN_GE_SPEED_10;
		return TITAN_GE_MDIO_GOOD;
	}

	return TITAN_GE_MDIO_ERROR;
}

/*
 * Configures flow control
 */
static int titan_ge_fc_config(titan_ge_port_info *titan_ge_eth)
{
	int		err = 0;
	unsigned int	adv_reg_data, lp_reg_data;

	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_PHY_AUTONEG_ADV, &adv_reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
		printk(KERN_ERR "Could not read PHY control register 0x4 \n");
		return TITAN_GE_MDIO_ERROR;
	}

	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_PHY_LP_ABILITY, &lp_reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x4 \n");
                return TITAN_GE_MDIO_ERROR;
        }

	/*
	 * Check if both the PAUSE bits are set to 1. If that is the case, then we have
	 * Symmetric Flow Control enabled at both the ends. 
	 * 
	 * For the Marvel PHY, the bits 8 and 7 in the TITAN_PHY_AUTONEG_ADV register
	 * need to be checked here. For the LP, the bits 11 and 10 of the TITAN_PHY_LP_ABILITY
	 * register need to be checked here
	 */
	if ( (adv_reg_data & 0x0180) && (lp_reg_data & 0x0400))
		titan_ge_eth->fc = TITAN_GE_FC_FULL;

	else
		/* For the Transmit Pause Frames only */
		if (!(adv_reg_data & 0x0080) && (adv_reg_data & 0x0100)	&&
			(lp_reg_data & 0x0800) && (lp_reg_data & 0x0400)) 
				titan_ge_eth->fc = TITAN_GE_FC_TX_PAUSE;

	else 
		/* For the Receive Pause Frames only */
		if ( (adv_reg_data & 0x0180) && !(lp_reg_data & 0x0800) &&
			(lp_reg_data & 0x0400))
				titan_ge_eth->fc = TITAN_GE_FC_RX_PAUSE;
	else 
		/* No Flow Control */
		titan_ge_eth->fc = TITAN_GE_FC_NONE;

	return TITAN_GE_MDIO_GOOD;
}

/*
 * Configure the GMII block of the Titan
 */
static void titan_ge_gmii_config(void)
{
	unsigned int	reg_data = 0;
	/*
	 * Description:
	 *
	 * Enable Full Duplex mode
	 * Select the GMII/TBI mode
	 * Set the speed to 1000 Mbps
	 */
        TITAN_GE_WRITE(TITAN_GE_GMII_CONFIG_MODE, 0x1201);

        reg_data = TITAN_GE_READ(TITAN_GE_GMII_CONFIG_GENERAL);
        reg_data |= 0x2;        /* Tx datapath enable */
        reg_data |= 0x1;        /* Rx datapatch enable */
        TITAN_GE_WRITE(TITAN_GE_GMII_CONFIG_GENERAL, reg_data);
}
	
/*
 * Configure the PHY. No support for TBI (10 Bit Interface) as yet. Hence, we need to 
 * make use of MDIO for communication with the external PHY. The PHY uses GMII
 */
static int titan_phy_setup(titan_ge_port_info * titan_ge_eth)
{
	titan_ge_mdio_config	*titan_ge_mdio;
	unsigned int		reg_data;
	int			err = 0;
	
	titan_ge_mdio->clka = 0x001f;		/* Clock */
	titan_ge_mdio->mdio_spre = 0x0000;	/* Generate 32-bit Preamble */
	titan_ge_mdio->mdio_mode = 0x4000;	/* Direct mode */
	titan_ge_mdio_setup(titan_ge_mdio);	/* Setup the SCMB and the MDIO */

	/* PHY control register 0 */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, MII_BMCR, &reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
		printk(KERN_ERR "Could not read PHY control register 0 \n");
		return TITAN_ERROR;
	}

	/* Set the necessary control parameters and then reset the PHY for it to take place */
	reg_data &= ~(0x8000);

	/* Description :
	 *
	 * Reset bit is set to 0
	 * Speed is set to 1000 Mbps
	 * Autonegotiation is enabled
	 * Duplex setting is Full Duplex
	 * Restart the Auto-Negotiation process
	 */
	reg_data |= 0x1160;
	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, MII_BMCR, reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
		printk(KERN_ERR "Could not write to PHY control register 0 \n");
		return TITAN_ERROR;
	}

	/* Now reset the PHY for the above to take effect */
	reg_data |= 0x8000;
	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, MII_BMCR, reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0 \n");
                return TITAN_ERROR;
        }

	/* Now read the PHY status register */
	err = titan_ge_wait_autoneg();
	if (err == TITAN_GE_MDIO_ERROR) 
		return TITAN_ERROR;

	/* Now do the important Flow control related configuration */
	err = titan_ge_fc_config(titan_ge_eth);
	if (err == TITAN_GE_MDIO_ERROR) {
		printk(KERN_ERR "Could not configure Flow Control Settings \n");
		return TITAN_ERROR;
	}

	/* Now read the Model of the PHY */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, MII_PHYSID2, &reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY status register 0x3 \n");
		return TITAN_ERROR;
	}

	printk("Marvel PHY Model Number : %d  \n", ( (reg_data & 0x01f0) >> 4));	

	/* Now the Auto-negotiate Advertisement register */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, MII_ADVERTISE, &reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY status register 0x4 \n");
                return TITAN_ERROR;
        }

	/*
	 * Description :
	 *
	 * Set the Selector field to IEEE 802.3
	 * Set the Asymmetric Pause
	 * Set the Pause 
	 * Set the Next Page to Advertise
	 */
	reg_data |= 0x8c01;

	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, MII_ADVERTISE, reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0x4 \n");
                return TITAN_ERROR;
        }

	/* Reset the PHY for the above values to take effect */
	titan_ge_reset_phy();

	/* 1000BASE-T control register */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_MII_CTRL, &reg_data);
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x9 \n");
		return TITAN_ERROR;
        }

	/* 
	 * Description :
	 *
	 * Manual MASTER/SLAVE Configuration Enable
	 * Manual configure as MASTER
	 * 1000BASE-T Full Duplex
	 * 1000BASE-T Half Duplex
	 */
	reg_data |= 0x1b00;
	
	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, MII_BMCR, reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0x9 \n");
                return TITAN_ERROR;
        }	

	titan_ge_reset_phy();

	/* Check the PHY speed and duplex capability */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_MII_EXTENDED, &reg_data);

	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x0f \n");
                return TITAN_ERROR;
        }

	if (reg_data & 0xa000)
		printk("PHY 1000 Mbps, Full Duplex  \n");

	if (reg_data & 0x5000)
		printk("PHY 1000 Mbps, Half Duplex  \n");

	/* PHY Specific control register */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_CTRL, &reg_data);
	
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x10 \n");
                return TITAN_ERROR;
        }

	/* 
	 * Description:
	 * Assert CRS on transmit in half duplex mode only 
	 * Disable polarity reversal
	 * Disable Jabber func
	 */
	reg_data |= 0x0103;
	
	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_CTRL, reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0x10 \n");
                return TITAN_ERROR;
        }

#ifdef TITAN_DEBUG
	/* PHY Status Register, Lots of messages here */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_STATUS, &reg_data);

        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x11 \n");
                return TITAN_ERROR;
        }

	if (reg_data & 0x0800)
		printk(KERN_INFO "Auto-Negotiation completed \n");

	if (reg_data & 0x8000)
		printk(KERN_INFO "Speed is 1000 Mbps \n");

	if (reg_data & 0x4000)
		printk(KERN_INFO "Speed is 100 Mbps \n");

	if (reg_data & 0x0400)
		printk(KERN_INFO "Link is up \n");

	if (reg_data & 0xc)
		printk(KERN_INFO "Transmit and Receive Pause Enabled  \n");

	/* Cable Length spec */
	if (! (reg_data & 0x0380) )
		printk(KERN_INFO "Cable Length < 50 metres \n");

	if (reg_data & 0x0080)
		printk(KERN_INFO "Cable Length 50 - 80 metres \n");

	if (reg_data & 0x0100)
		printk(KERN_INFO "Cable Length 80 - 110 metres \n");

	if (reg_data & 0x0180)
		printk(KERN_INFO "Cable Length 110 - 140 metres \n");

	if (reg_data & 0x0200)
		printk(KERN_INFO "Cable Length is > 140 metres \n");

	/* Transmit and Receive Pause */
	if (reg_data & 0x8)
		printk(KERN_INFO "Transmit Pause enabled  \n");

	if (reg_data & 0x4)
		printk(KERN_INFO "Receive Pause enabled  \n");
#endif

	/* Interrupt Enable Registers for the PHY */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_IE, &reg_data);

	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x11 \n");
                return TITAN_ERROR;
        }

	/*
	 * Description:
	 *
	 * Speed Change interrupt Enable
	 * Duplex Change interrupt Enable
	 * Link status change interrupt enable
	 */
	reg_data |= 0x34;

	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, MII_BMCR, reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0x9 \n");
                return TITAN_ERROR;
        }

	/* LED Control */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_LED, &reg_data);

        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x18 \n");
                return TITAN_ERROR;
        }

	/*
	 * Description :
	 *
	 * LED_TX_CONTROL set to 11
	 * LED_DUPLEX_CONTROL set to 1
	 */
	reg_data |= 0x45;

	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_LED, reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0x18 \n");
                return TITAN_ERROR;
        }
	
	/* LED Override register */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_LED_OVER, &reg_data);

        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x19 \n");
                return TITAN_ERROR;
        }

        /*
         * Description :
         *
	 * LED_LINK10, LED_LINK100, LED_LINK1000 set to blink mode
	 * LED_RX, LED_TX set to blink mode
	 */
	reg_data |= 0x03ff;

	err = titan_ge_mdio_write(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_LED_OVER, reg_data);
        if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not write to PHY control register 0x19 \n");
                return TITAN_ERROR;
        }

	titan_ge_gmii_config();

	return TITAN_OK;
}
	
/*
 * Enable the TMAC if it is not
 */
static void titan_ge_enable_tx(unsigned int port_num)
{
	unsigned long	reg_data;

	reg_data = TITAN_GE_READ(TITAN_GE_TMAC_CONFIG_1);
	if (! (reg_data & 0x0001)) {
		reg_data |= 0x0001; /* Enable TMAC */
	        reg_data |= 0x4000; /* CRC Check Enable */
        	reg_data |= 0x2000; /* Padding enable */
	        reg_data |= 0x0800; /* CRC Add enable */
        	reg_data |= 0x0080; /* PAUSE frame */
	
		TITAN_GE_WRITE(TITAN_GE_TMAC_CONFIG_1, reg_data);
	}
}

/*
 * Tx Timeout function
 */
static void titan_ge_tx_timeout(struct net_device *netdev)
{
	titan_ge_port_info	*titan_ge_eth = netdev->priv;

	printk(KERN_INFO "%s: TX timeout  ", netdev->name);
	printk(KERN_INFO "Resetting card \n");

	/* Do the reset outside of interrupt context */
        schedule_task(&titan_ge_eth->tx_timeout_task);
}

/*
 * Update the AFX tables for UC and MC for slice 0 only
 */
static void titan_ge_update_afx(titan_ge_port_info *titan_ge_eth)
{
	int		i;
	unsigned long	reg_data = 0;
	unsigned char	*p_addr = NULL;

	memcpy(p_addr, titan_ge_eth->port_mac_addr, 6);

	/* Configure the eight address filters */
	for (i = 0; i < 8; i++) {
		/* Select each of the eight filters */
		TITAN_GE_WRITE(TITAN_GE_AFX_ADDRS_FILTER_CTRL_2, i);
	
		/* Configure the match */
		reg_data |= 0x8; 	/* Forward Enable Bit */
		reg_data |= 0x1;	/* Filter enable*/
		
		TITAN_GE_WRITE(TITAN_GE_AFX_ADDRS_FILTER_CTRL_0, reg_data);

		/* Finally, AFX Exact Match Address Registers */
		TITAN_GE_WRITE(TITAN_GE_AFX_EXACT_MATCH_LOW, ( (p_addr[0] << 8) | p_addr[1]));
		TITAN_GE_WRITE(TITAN_GE_AFX_EXACT_MATCH_MID, ( (p_addr[2] << 8) | p_addr[3]));
		TITAN_GE_WRITE(TITAN_GE_AFX_EXACT_MATCH_HIGH, ( (p_addr[4] << 8) | p_addr[5]));

		/* VLAN id set to 0 */
		TITAN_GE_WRITE(TITAN_GE_AFX_EXACT_MATCH_VID, 0);

		/* Disable Multicast, Promiscuous and All Multi mode */
		TITAN_GE_WRITE(TITAN_GE_AFX_ADDRS_FILTER_CTRL_1, 0);	
	}
}

/*
 * Actual Routine to reset the adapter when the timeout occurred
 */
static void titan_ge_tx_timeout_task(struct net_device *netdev)
{
	titan_ge_port_info      *titan_ge_eth = netdev->priv;

	netif_device_detach(netdev);
	titan_ge_port_reset(titan_ge_eth->port_num);
	titan_ge_port_start(titan_ge_eth);
	netif_device_attach(netdev);
}

/*
 * Change the MTU of the Ethernet Device
 */
static int titan_ge_change_mtu(struct net_device *netdev, int new_mtu)
{
        titan_ge_port_info      *titan_ge_eth;
        struct titan_ge         *port_private;
        unsigned int            port_num;
        unsigned long           flags;

        titan_ge_eth = netdev->priv;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;
        port_num = port_private->port_num;

        spin_lock_irqsave(&port_private->lock, flags);

        if ((new_mtu > 9500) || (new_mtu < 64)) {
                spin_unlock_irqrestore(&port_private->lock, flags);
                return -EINVAL;
        }

        netdev->mtu = new_mtu;

        /* Now we have to reopen the interface so that SKBs with the new
         * size will be allocated */

        if (netif_running(netdev)) {
                if (titan_ge_eth_stop(netdev) == TITAN_OK)
                        printk(KERN_ERR
                               "%s: Fatal error on stopping device\n",
                               netdev->name);

                if (titan_ge_eth_open(netdev) == TITAN_OK)
                        printk(KERN_ERR
                               "%s: Fatal error on opening device\n",
                               netdev->name);
        }

	return 0;
}

/*
 * Titan Gbe Interrupt Handler. No Rx NAPI support yet. Currently, we check channel
 * 0 only
 */
static void titan_ge_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	struct net_device	*netdev = (struct net_device *)dev_id;
	titan_ge_port_info	*titan_ge_eth;
	struct titan_ge		*port_private;
	unsigned int		port_num, reg_data;
	unsigned long		eth_int_cause, eth_int_cause_error;
	int			err = 0;
	
	titan_ge_eth = netdev->priv;
	port_private = titan_ge_eth->port_private;
	port_num = port_private->port_num;

	eth_int_cause = TITAN_GE_READ(TITAN_GE_INTR_XDMA_CORE_A);
	if (eth_int_cause == 0x0) 
		/* False alarm */
		return;

	/* Ack the interrupts quickly */
	TITAN_GE_WRITE(TITAN_GE_INTR_XDMA_CORE_A, ~eth_int_cause);

	/* Check for Channel 0 Tx errors */
	eth_int_cause_error = TITAN_GE_READ(TITAN_GE_CHANNEL0_INTERRUPT);
	if (eth_int_cause_error & 0x00040000) {
		/* Unrecoverable error */
		printk(KERN_ERR "Unrecoverable error in the Tx XDMA channel 0 \n");
		TITAN_GE_WRITE(TITAN_GE_CHANNEL0_INTERRUPT, eth_int_cause_error);
	}

	if (eth_int_cause & 0x2) {
		/* Tx interrupt to the CPU */
		if (titan_ge_free_tx_queue(netdev, eth_int_cause) == 0) {
			if (netif_queue_stopped(netdev) 
			&& (netif_running(netdev))
			&& 
			(TITAN_GE_TX_QUEUE > port_private->tx_ring_skbs + 1)) {
				netif_wake_queue(netdev);
			}
		}
	}

	/*
	 * First get the Rx packets off the queue and then refill the queue
	 */	
	if (eth_int_cause & 0x1) {
		/* Rx interrupt to the CPU */
		unsigned int	total_received;

		total_received = titan_ge_receive_queue(netdev, 0);
		titan_ge_rx_task(netdev);
	}

	/* 
	 * PHY interrupt to inform abt the changes. Reading the PHY Status register will
	 * clear the interrupt
	 */
	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_IS, &reg_data);
	if (!(reg_data & 0x0400)) {
		/* Link status change */
		titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_STATUS, &reg_data);
		if (!(reg_data & 0x0400)) {
			/* Link is down */
			netif_carrier_off(netdev);
			netif_stop_queue(netdev);
		}
		else {
			/* Link is up */
			netif_carrier_on(netdev);
			netif_wake_queue(netdev);

			/* Enable the queue */
			titan_ge_enable_tx(port_num);
		}
	}

	return;
}

/*
 * Open the network device 
 */
static int titan_ge_open(struct net_device *netdev) 
{
	int			retval;
	titan_ge_port_info	*titan_ge_eth;
	struct titan_ge		*port_private;
	unsigned int		port_num;

	titan_ge_eth = netdev->priv;
	port_private = (struct titan_ge *)titan_ge_eth->port_private;
	port_num = port_private->port_num;

	spin_lock_irq(&(port_private->lock));

	retval = request_irq(TITAN_ETH_PORT_IRQ, titan_ge_int_handler,
			(SA_INTERRUPT | SA_SAMPLE_RANDOM), netdev->name, netdev);

	if (retval != 0) {
		printk(KERN_ERR "Cannot assign IRQ number to TITAN GE \n");
		return -1;
	}
	else {
		netdev->irq = TITAN_ETH_PORT_IRQ;
	}

	if (titan_ge_eth_open(netdev)) {
		printk("%s: Error opening interface \n", netdev->name);
		free_irq(netdev->irq, netdev);
		spin_unlock_irq(&(port_private->lock));
		return -EBUSY;
	}

	MOD_INC_USE_COUNT;
	spin_unlock_irq(&(port_private->lock));
	return 0;
}

/* 
 * Return the Rx buffer back to the Rx ring
 */
static int titan_ge_rx_return_buff(titan_ge_port_info *titan_ge_port,
					titan_ge_packet *packet)
{
	int				rx_used_desc;
	volatile titan_ge_rx_desc	*rx_desc;

	rx_used_desc = titan_ge_port->rx_used_desc_q;
	rx_desc = &(titan_ge_port->rx_desc_area[rx_used_desc]);

	rx_desc->buffer_addr = packet->buffer; /* There is no checksum here */
	rx_desc->cmd_sts = packet->len | TITAN_GE_RX_BUFFER_OWNED | TITAN_GE_RX_STP;
	titan_ge_port->rx_skb[rx_used_desc] = packet->skb;
	
	mb();

	titan_ge_port->rx_used_desc_q = (rx_used_desc + 1) % TITAN_GE_RX_QUEUE;

	return TITAN_OK;
}

/*
 * Allocate the SKBs for the Rx ring. Also used for refilling the queue
 */
static void titan_ge_rx_task(void *data)
{
	struct net_device 	*netdev = (struct net_device *) data;
	titan_ge_port_info 	*titan_ge_eth;
	struct titan_ge		*port_private;
	unsigned int		port_num;
	titan_ge_packet		packet;
	struct sk_buff		*skb;

	titan_ge_eth = netdev->priv;
	port_private = (struct titan_ge *)titan_ge_eth->port_private;
	port_num = port_private->port_num;

	while (port_private->rx_ring_skbs < (port_private->rx_ring_size - 5)) {
		skb = dev_alloc_skb(BUFFER_MTU + 8 + EXTRA_BYTES);
                if (!skb)
                        break;

                port_private->rx_ring_skbs++;
                packet.cmd_sts = TITAN_GE_RX_STP;
		packet.len = netdev->mtu + ETH_HLEN + 4 + 2 + EXTRA_BYTES;
		if (packet.len & ~0x7) {
                        packet.len &= ~0x7;
                        packet.len += 8;
                }
                packet.buffer =
                    pci_map_single(0, skb->data,
                                   netdev->mtu + ETH_HLEN + 4 + 2 + EXTRA_BYTES,
                                   PCI_DMA_FROMDEVICE);

                packet.skb = skb;
                if (titan_ge_rx_return_buff(titan_ge_eth, &packet) != TITAN_OK) {
                        printk(KERN_ERR
                               "%s: Error allocating RX Ring\n", netdev->name);
                        break;
                }
                skb_reserve(skb, 2);
        }
}

/*
 * Actual init of the Tital GE port 
 */
static void titan_port_init(struct net_device *netdev, titan_ge_port_info *titan_ge_eth)
{
	unsigned long 	reg_data;
	int		ticks = 50;

	/* Configure the PHY via the GMII for 1000 Mbps */
	if (titan_phy_setup(titan_ge_eth) != TITAN_OK)
		printk(KERN_ERR "Could not configure PHY \n");

	titan_ge_port_reset(titan_ge_eth->port_num);

	/* First reset the TMAC */
	reg_data = TITAN_GE_READ(TITAN_GE_CHANNEL0_CONFIG);
	reg_data |= 0x80000000;
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);

	while (ticks > 0)
		ticks--;

	reg_data = TITAN_GE_READ(TITAN_GE_CHANNEL0_CONFIG);
	reg_data &= ~(0x80000000);
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);

	/* Now  reset the RMAC */
	ticks = 50;
	reg_data = TITAN_GE_READ(TITAN_GE_CHANNEL0_CONFIG);
	reg_data |= 0x0008000;
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);

	while (ticks > 0)
		ticks--;

	reg_data = TITAN_GE_READ(TITAN_GE_CHANNEL0_CONFIG);
	reg_data &= ~(0x00080000);
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);

	/* Init the Address Filter Match table */
	titan_ge_update_afx(titan_ge_eth);

	titan_ge_reset_phy();
}

/*
 * Start the port 
 */
static int titan_ge_port_start(titan_ge_port_info *titan_port)
{
	unsigned int	reg_data;

	/* We need to write the descriptors for Tx and Rx */
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_TX_DESC, (unsigned long)titan_port->tx_dma);
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_RX_DESC, (unsigned long)titan_port->rx_dma);

	/* Clear the Performance Monitoring packet counter */
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_PACKET, 0);
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_BYTE, 0);

	/* Start the Tx activity */
	reg_data = TITAN_GE_READ(TITAN_GE_TMAC_CONFIG_1);
	reg_data |= 0x0001; /* Enable TMAC */
	reg_data |= 0x4000; /* CRC Check Enable */
	reg_data |= 0x2000; /* Padding enable */
	reg_data |= 0x0800; /* CRC Add enable */
	if (titan_port->fc != TITAN_GE_FC_NONE)
		reg_data |= 0x0080; /* PAUSE frame */
	
	TITAN_GE_WRITE(TITAN_GE_TMAC_CONFIG_1, reg_data);

	/* Now check the TMAC status */
	reg_data = TITAN_GE_READ(TITAN_GE_TMAC_CONFIG_1);

	if (reg_data & 0x8000)
		printk(KERN_NOTICE "Enabled for Tx   ");
	else	
		printk(KERN_NOTICE "Tx disabled \n");

	/* Set the Tx line speed. Should check the PHY here */
	if (titan_ge_get_speed(titan_port) == TITAN_GE_MDIO_ERROR)
		return TITAN_ERROR;

	/*
	 * This is how it needs to be done. If the Gigabit is not enabled in the 
	 * PHY, we disable the HW_GIGA_EN bit and also disable the SW_GIGE_EN
	 * bit
	 */	
	reg_data = TITAN_GE_READ(TITAN_GE_TMAC_CONFIG_2);
	if (titan_port->speed == TITAN_GE_SPEED_1000)
		reg_data |= 0x8000; /* Gigabit */
	else
		if ( (titan_port->speed == TITAN_GE_SPEED_100) || 
				(titan_port->speed == TITAN_GE_SPEED_10))
			reg_data &= ~(8000);

	TITAN_GE_WRITE(TITAN_GE_TMAC_CONFIG_2, reg_data);

	/* Start the Rx activity */
	reg_data = TITAN_GE_READ(TITAN_GE_RMAC_CONFIG_1);
	reg_data |= 0x0001; /* RMAC Enable */
	reg_data |= 0x0010; /* CRC Check enable */
	reg_data |= 0x0040; /* Min Frame check enable */
	reg_data |= 0x0400; /* Max Frame check enable */

        TITAN_GE_WRITE(TITAN_GE_RMAC_CONFIG_1, reg_data);

	/* Destination Address drop bit */
        reg_data = TITAN_GE_READ(TITAN_GE_RMAC_CONFIG_2);
	reg_data |= 0x1; /* DA_DROP bit */
	TITAN_GE_WRITE(TITAN_GE_RMAC_CONFIG_2, reg_data);

	reg_data = TITAN_GE_READ(TITAN_GE_RMAC_CONFIG_1);
	if (reg_data & 0x8000)
		printk(KERN_NOTICE "Enabled for Rx  ");
	else
		printk(KERN_ERR "Rx Disabled \n");

	reg_data = TITAN_GE_READ(TITAN_GE_RMAC_LINK_CONFIG);
	if (reg_data & 0x8000)
		printk(KERN_NOTICE "1000 Mbps \n");
	else
		printk(KERN_NOTICE "10/100 Mbps \n");

	/* WCIMODE bit set to 1 */
	reg_data = TITAN_GE_READ(TITAN_GE_TSB_CTRL_0);
	reg_data |= 0x00000100;
	TITAN_GE_WRITE(TITAN_GE_TSB_CTRL_0, reg_data); 

	/* Enable the Interrupts for Tx and Rx on channel 0 only */
	TITAN_GE_WRITE(TITAN_GE_INTR_XDMA_IE, 0x3);

	/* Start the Tx XDMA controller */
	reg_data = 0x00000000; /* Tx reset bit set to 0 & Tx DMA disabled bit to 0 */
	
	/* Start the Rx XDMA controller */
	reg_data |= 0x00000000; /* Rx reset bit is set to 0 &  Rx Disabled bit is set to 0 */
	reg_data |= 0x00000800; /* Rx TCP Checksum computation */

	/* Note that direct deposit is disabled */
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data); 

	/* Writeback enable on the Rx and Tx descriptor */
	reg_data = TITAN_GE_READ(TITAN_GE_XDMA_CONFIG);
	reg_data |= 0x2008000;
	TITAN_GE_WRITE(TITAN_GE_XDMA_CONFIG, reg_data);

	return TITAN_OK;
}

/*
 * Function to queue the packet for the Ethernet device
 */
static int titan_ge_tx_queue(titan_ge_port_info *titan_ge_eth, titan_ge_packet *packet)
{
	int 				curr_desc, used_desc;
	volatile titan_ge_tx_desc	*tx_curr;

	curr_desc = titan_ge_eth->tx_curr_desc_q;
	used_desc = titan_ge_eth->tx_used_desc_q;

	if (curr_desc == used_desc) {
		printk(KERN_ERR "Queue is full \n");
		return -1;
	}

	tx_curr = &(titan_ge_eth->tx_desc_area[curr_desc]);
	if (tx_curr == NULL) {
		printk(KERN_ERR "Current Descriptor is NULL !! \n");
		return -1;
	}

	tx_curr->buffer_len = packet->len;
	tx_curr->buffer_addr = packet->buffer;
	titan_ge_eth->tx_skb[curr_desc] = (struct sk_buff *)packet->skb;

	/* For weak memory ordered systems only */
	wmb();

	/* Last descriptor enables interrupt and changes ownership */
	tx_curr->cmd_sts |= TITAN_GE_TX_BUFFER_OWNED | TITAN_GE_TX_ENABLE_INTERRUPT;

	/* Enable the Tx channel or slice */
	titan_ge_enable_tx(titan_ge_eth->port_num);
	
	/* Current descriptor updated */
	curr_desc = (curr_desc + 1) % TITAN_GE_TX_QUEUE;
	titan_ge_eth->tx_curr_desc_q = curr_desc;

	if (curr_desc == used_desc) {
		printk(KERN_ERR "Queue is full \n");
		return -1;
	}

	return 1;
}

/* 
 * Actually does the open of the Ethernet device
 */
static int titan_ge_eth_open(struct net_device *netdev)
{
        titan_ge_port_info      *titan_ge_eth;
        struct titan_ge         *port_private;
        unsigned int            port_num, size, phy_reg;
	unsigned long		phy_reg_data, reg_data;
	int			err = 0;

        titan_ge_eth = netdev->priv;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;
        port_num = port_private->port_num;

	/* Stop the Rx activity */
	reg_data = TITAN_GE_READ(TITAN_GE_RMAC_CONFIG_1);
        reg_data &= ~(0x00000001);
        TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);

	/* Clear the port interrupts */
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_INTERRUPT, 0);
	TITAN_GE_WRITE(TITAN_GE_INTR_XDMA_CORE_A, 0);

	/* Set the MAC Address */
	memcpy(titan_ge_eth->port_mac_addr, netdev->dev_addr, 6);
	titan_port_init(netdev, titan_ge_eth);

	/* Allocate the Tx ring now */
	port_private->tx_ring_skbs = 0;
        port_private->tx_ring_size = TITAN_GE_TX_QUEUE;
	size = port_private->tx_ring_size * sizeof(titan_ge_tx_desc);
	titan_ge_eth->tx_desc_area =
            (titan_ge_tx_desc *)pci_alloc_consistent(NULL, size, &titan_ge_eth->tx_dma);

	if (!titan_ge_eth->tx_desc_area) {
		printk(KERN_ERR "%s: Cannot allocate Tx Ring (size %d bytes)\n",
                       netdev->name, size);
                return -ENOMEM;
        } 

	memset((void *)titan_ge_eth->tx_desc_area, 0, titan_ge_eth->tx_desc_area_size);

	/* Now initialize the Tx descriptor ring */
	titan_ge_init_tx_desc_ring(titan_ge_eth, 
				port_private->tx_ring_size,
				(unsigned long)titan_ge_eth->tx_desc_area,
				(unsigned long)titan_ge_eth->tx_dma);

	/* Allocate the Rx ring now */
	port_private->rx_ring_size = TITAN_GE_RX_QUEUE;
	port_private->rx_ring_skbs = 0;
        size = port_private->rx_ring_size * sizeof(titan_ge_rx_desc);
	titan_ge_eth->rx_desc_area = 
		(titan_ge_rx_desc *)pci_alloc_consistent(NULL, size, &titan_ge_eth->rx_dma);

	if (!titan_ge_eth->rx_desc_area) {
		printk(KERN_ERR "%s: Cannot allocate Rx Ring (size %d bytes)\n",
                       netdev->name, size);

		printk(KERN_ERR
                       "%s: Freeing previously allocated TX queues...",
                       netdev->name);
		
		pci_free_consistent(0, titan_ge_eth->tx_desc_area_size,
                                    (void *)titan_ge_eth->tx_desc_area,
					titan_ge_eth->tx_dma);

                return -ENOMEM;
        }

	memset((void *) titan_ge_eth->rx_desc_area, 0, 
			titan_ge_eth->tx_desc_area_size);

	/* Now initialize the Rx ring */
	if ((titan_ge_init_rx_desc_ring(titan_ge_eth, port_private->rx_ring_size, 1536,
		(unsigned long) titan_ge_eth->rx_desc_area, 0,
				(unsigned long)titan_ge_eth->rx_dma)) == 0) 
			panic("%s: Error initializing RX Ring\n", netdev->name);

	/* Fill the Rx ring with the SKBs */
	titan_ge_rx_task(netdev);
	titan_ge_port_start(titan_ge_eth);

	/* Check if Interrupt Coalscing needs to be turned on */
	if (TITAN_GE_RX_COAL) {
		port_private->rx_int_coal =
			titan_ge_rx_coal(TITAN_GE_RX_COAL);
	}

	if (TITAN_GE_TX_COAL) {
		port_private->tx_int_coal =
			titan_ge_tx_coal(TITAN_GE_TX_COAL);
	}

	err = titan_ge_mdio_read(TITAN_GE_MARVEL_PHY_ID, TITAN_GE_MDIO_PHY_STATUS, &phy_reg);
	if (err == TITAN_GE_MDIO_ERROR) {
                printk(KERN_ERR "Could not read PHY control register 0x11 \n");
                return TITAN_ERROR;
        }
	if (!(phy_reg & 0x0400)) {
		/* PHY related problem */
		netif_carrier_off(netdev);
		netif_stop_queue(netdev);
	}
	else {	
		phy_reg_data = TITAN_GE_READ(TITAN_GE_RMAC_CONFIG_1);
		if (phy_reg_data & 0x00008000) {
			/* RMAC has been successfully enabled. So, check the TMAC now */
			phy_reg_data = TITAN_GE_READ(TITAN_GE_TMAC_CONFIG_1);
			if (phy_reg_data & 0x00008000) {
				/* TMAC has also been successfull enabled */
				netif_carrier_on(netdev);
				netif_start_queue(netdev);
			}
			else {
				/* Problem with the TMAC */
				netif_carrier_off(netdev);
				netif_stop_queue(netdev);
			}
		}
		else {
			/* Problem with RMAC */
			netif_carrier_off(netdev);
			netif_stop_queue(netdev);
        	}
	}

        return TITAN_OK;
}
	
/*
 * Queue the packet for Tx. Currently no support for Zerocopy, checksum offload
 * and Scatter Gather. Does Titan support Tx side checksum offload at all? 
 */
int titan_ge_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
        titan_ge_port_info      *titan_ge_eth;
        struct titan_ge         *port_private;
        unsigned int            port_num;
	titan_ge_packet		packet;
	unsigned long		flags;
	unsigned long		status;
	struct net_device_stats	*stats;

        titan_ge_eth = netdev->priv;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;
        port_num = port_private->port_num;
	stats = &port_private->stats;

        if (netif_queue_stopped(netdev)) 
                return 1;

	if ((TITAN_GE_TX_QUEUE - port_private->tx_ring_skbs) <=
            (skb_shinfo(skb)->nr_frags + 1)) {
                netif_stop_queue(netdev);
		return 1;
	}

        spin_lock_irqsave(&port_private->lock, flags);
	packet.cmd_sts = TITAN_GE_TX_ENABLE_INTERRUPT;

	packet.len = skb->len;
	packet.buffer = pci_map_single(0, skb->data, skb->len, PCI_DMA_TODEVICE);
	packet.skb = skb;
	status = titan_ge_tx_queue(titan_ge_eth, &packet);

	if (status < 0) 
		printk(KERN_ERR "Error on transmitting packet\n");

	port_private->tx_ring_skbs++;

	if (TITAN_GE_TX_QUEUE <= (port_private->tx_ring_skbs + 1))
		netif_stop_queue(netdev);

	stats->tx_bytes += skb->len;
        stats->tx_packets++;
        netdev->trans_start = jiffies;

        spin_unlock_irqrestore(&port_private->lock, flags);
        return 0;         
}

/*
 * Actually does the Rx 
 */
static int titan_ge_rx(titan_ge_port_info *titan_ge_port, titan_ge_packet *packet)
{
        int                             rx_curr_desc, rx_used_desc;
        volatile titan_ge_rx_desc       *rx_desc;

        rx_curr_desc = titan_ge_port->rx_curr_desc_q;
        rx_used_desc = titan_ge_port->rx_used_desc_q;

        if (( (rx_curr_desc + 1) % TITAN_GE_RX_QUEUE) == rx_used_desc) {
                printk(KERN_ERR "Rx queue is full   \n");
                return TITAN_ERROR;
        }

        rx_desc = &(titan_ge_port->rx_desc_area[rx_curr_desc]);
        if (rx_desc->cmd_sts & TITAN_GE_RX_BUFFER_OWNED)
                return TITAN_ERROR;

        packet->cmd_sts = rx_desc->cmd_sts;
	packet->buffer = rx_desc->buffer_addr; /* This is not important */
	packet->checksum = ( (rx_desc->buffer_addr & 0xffffffff) >> 15); /* Only for non TCP packets */
	packet->skb = titan_ge_port->rx_skb[rx_curr_desc];
	packet->len = (rx_desc->cmd_sts & 0x7fff);

	titan_ge_port->rx_skb[rx_curr_desc] = NULL;
	titan_ge_port->rx_curr_desc_q = (rx_curr_desc + 1) % TITAN_GE_RX_QUEUE;
	
	return TITAN_OK;
}

/*
 * Free the Tx queue of the used SKBs
 */
static int titan_ge_free_tx_queue(struct net_device *netdev, unsigned long eth_int_cause)
{
	titan_ge_port_info      *titan_ge_eth;
        struct titan_ge         *port_private;
        unsigned int            port_num;
	struct net_device_stats	*stats;
	titan_ge_packet		packet;
	int 			released =1;

	titan_ge_eth = netdev->priv;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;

	/* Take the lock */
        spin_lock(&(port_private->lock));
        port_num = port_private->port_num;
        stats = &port_private->stats;

	/* Check only channel 0 */
	while (titan_ge_return_tx_desc(titan_ge_eth, &packet) == 0) {
		if (packet.skb) {
			dev_kfree_skb_irq((struct sk_buff *)
						packet.skb);
			released = 0;
			if (port_private->tx_ring_skbs != 1)
				port_private->tx_ring_skbs--;
		}

		if (port_private->tx_ring_skbs == 0)
			panic("TX SKB Counter is 0 \n");
	}

	spin_unlock(&(port_private->lock));
        return released;
}

/*
 * Receive the packets and send it to the kernel. No support for Rx NAPI yet.
 */
int titan_ge_receive_queue(struct net_device *netdev, unsigned int max)
{
        titan_ge_port_info      *titan_ge_eth;
        struct titan_ge         *port_private;
        unsigned int            port_num;
        titan_ge_packet         packet;
        struct net_device_stats *stats;
	struct sk_buff		*skb;
	unsigned long		received_packets = 0;

	titan_ge_eth = netdev->priv;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;
        port_num = port_private->port_num;
	stats = &port_private->stats;

	while ((--max)
	&& (titan_ge_rx(titan_ge_eth, &packet) == TITAN_OK )) {	
		port_private->rx_ring_skbs--;
                received_packets++;
                stats->rx_packets++;

		skb = (struct sk_buff *) packet.skb;
		if ( (packet.cmd_sts & TITAN_GE_RX_PERR) ||
			(packet.cmd_sts & TITAN_GE_RX_OVERFLOW_ERROR) ||
			(packet.cmd_sts & TITAN_GE_RX_TRUNC) ||
			(packet.cmd_sts & TITAN_GE_RX_CRC_ERROR) ) {
				stats->rx_dropped++;
				dev_kfree_skb_irq(skb);
		}

		if ( !(packet.cmd_sts & TITAN_GE_RX_STP) ) {
			stats->rx_dropped++;
			printk(KERN_ERR
                         "%s: Received packet spread on multiple"
                         " descriptors\n", netdev->name);

			dev_kfree_skb_irq(skb);
		}
		else {
			struct ethhdr	*eth_h;
			struct iphdr	*ip_h;

			skb_put(skb, packet.len - 4);
			skb->dev = netdev;
			
			eth_h = (struct ethhdr *) skb->data;
			ip_h = (struct iphdr *) (skb->data + ETH_HLEN);

			/* Titan supports Rx checksum offload for TCP only */
			if (ip_h->protocol == IPPROTO_TCP) {
				skb->ip_summed = CHECKSUM_UNNECESSARY;
				skb->csum = htons((packet.cmd_sts & 0xffff0000) >> 15);
			}
			else
				skb->ip_summed = CHECKSUM_NONE;

			skb->protocol = eth_type_trans(skb, netdev);

			/* No Rx NAPI here */
			netif_rx(skb);
		}
	}
	return received_packets;
}

/*
 * Close the network device
 */
int titan_ge_stop(struct net_device *netdev)
{
	titan_ge_port_info	*titan_ge_eth;
	struct titan_ge		*port_private;
	unsigned int		port_num;

	titan_ge_eth = netdev->priv;
	port_private = (struct titan_ge *)titan_ge_eth->port_private;
	port_num = port_private->port_num;

	spin_lock_irq(&(port_private->lock));
	titan_ge_eth_stop(netdev);
	free_irq(netdev->irq, netdev);
        MOD_DEC_USE_COUNT;
        spin_unlock_irq(&port_private->lock);

        return TITAN_OK;
}

/*
 * Free the Tx ring
 */
static void titan_ge_free_tx_rings(struct net_device *netdev)
{	
	titan_ge_port_info	*titan_ge_eth;
	struct titan_ge		*port_private;
	unsigned int		port_num, curr;
	unsigned long		reg_data;
	
	titan_ge_eth = netdev->priv;
	port_private = (struct titan_ge *)titan_ge_eth->port_private;
	port_num = port_private->port_num;

	/* Stop the Tx DMA */
	reg_data = TITAN_GE_READ(TITAN_GE_CHANNEL0_CONFIG);
	reg_data |= 0xc0000000;
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);

	/* Disable the TMAC */
	reg_data = TITAN_GE_READ(TITAN_GE_TMAC_CONFIG_1);
	reg_data &= ~(0x00000001);
	TITAN_GE_WRITE(TITAN_GE_TMAC_CONFIG_1, reg_data);

	for (curr = 0;
		(port_private->tx_ring_skbs) && (curr < TITAN_GE_TX_QUEUE);
		curr++) {
			if (titan_ge_eth->tx_skb[curr]) {
				dev_kfree_skb(titan_ge_eth->tx_skb[curr]);
				port_private->tx_ring_skbs--;
			}
	}

	if (port_private->tx_ring_skbs != 0) 
		printk("%s: Error on Tx descriptor free - could not free %d"
			" descriptors\n", netdev->name,
			port_private->tx_ring_skbs);

	pci_free_consistent(0, titan_ge_eth->tx_desc_area_size,
                            (void *)titan_ge_eth->tx_desc_area,
                            titan_ge_eth->tx_dma);
}

/*
 * Free the Rx ring
 */
static void titan_ge_free_rx_rings(struct net_device *netdev)
{
	titan_ge_port_info      *titan_ge_eth;
        struct titan_ge         *port_private;
        unsigned int            port_num, curr;
	unsigned long		reg_data;

	titan_ge_eth = netdev->priv;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;
        port_num = port_private->port_num;

	/* Stop the Rx DMA */
	reg_data = TITAN_GE_READ(TITAN_GE_CHANNEL0_CONFIG);
	reg_data |= 0x000c0000;
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);

	/* Disable the RMAC */
	reg_data = TITAN_GE_READ(TITAN_GE_RMAC_CONFIG_1);
	reg_data &= ~(0x00000001);
	TITAN_GE_WRITE(TITAN_GE_CHANNEL0_CONFIG, reg_data);
	
	for (curr = 0;
                port_private->rx_ring_skbs && (curr < TITAN_GE_RX_QUEUE);
		curr++) {
                if (titan_ge_eth->rx_skb[curr]) {
                        dev_kfree_skb(titan_ge_eth->rx_skb[curr]);
                        port_private->rx_ring_skbs--;
                }
        }

        if (port_private->rx_ring_skbs != 0)
                printk(KERN_ERR
                       "%s: Error in freeing Rx Ring. %d skb's still"
                       " stuck in RX Ring - ignoring them\n", netdev->name,
                       port_private->rx_ring_skbs);

        pci_free_consistent(0, titan_ge_eth->rx_desc_area_size,
                            (void *) titan_ge_eth->rx_desc_area,
                            titan_ge_eth->rx_dma);
}	

/* 
 * Actually does the stop of the Ethernet device
 */
static int titan_ge_eth_stop(struct net_device *netdev)
{
	titan_ge_port_info	*titan_ge_eth;
	struct titan_ge         *port_private;
        unsigned int            port_num;

        titan_ge_eth = netdev->priv;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;
        port_num = port_private->port_num;

	netif_stop_queue(netdev);

	titan_ge_port_reset(titan_ge_eth->port_num);

	titan_ge_free_tx_rings(netdev);
	titan_ge_free_rx_rings(netdev);

	/* Disable the Tx and Rx Interrupts for the XDMA channel 0 */
	TITAN_GE_WRITE(TITAN_GE_INTR_XDMA_IE, 0x0);

	return TITAN_OK;
}

/*
 * Update the MAC address. Note that we have to write the address in three station 
 * registers, 16 bits each. And this has to be done for TMAC and RMAC
 */
static void titan_ge_update_mac_address(struct net_device *netdev)
{
	titan_ge_port_info		*titan_ge_eth = netdev->priv;
	struct titan_ge			*port_private;
	unsigned char			*p_addr = NULL;

	port_private = (struct titan_ge *)titan_ge_eth->port_private;
	memcpy(titan_ge_eth->port_mac_addr, netdev->dev_addr, 6);
	memcpy(p_addr, netdev->dev_addr, 6);

	/* Update the Address Filtering Match tables */
	titan_ge_update_afx(titan_ge_eth);

	/* Set the MAC address here for TMAC and RMAC */
	TITAN_GE_WRITE(TITAN_GE_TMAC_STATION_HI, ( (p_addr[4] << 8) | p_addr[5]));
	TITAN_GE_WRITE(TITAN_GE_TMAC_STATION_MID, ( (p_addr[2] << 8) | p_addr[3]));
	TITAN_GE_WRITE(TITAN_GE_TMAC_STATION_LOW, ( (p_addr[0] << 8) | p_addr[1]));

	TITAN_GE_WRITE(TITAN_GE_RMAC_STATION_HI, ( (p_addr[4] << 8) | p_addr[5]));
	TITAN_GE_WRITE(TITAN_GE_RMAC_STATION_MID, ( (p_addr[2] << 8) | p_addr[3]));
	TITAN_GE_WRITE(TITAN_GE_RMAC_STATION_LOW, ( (p_addr[0] << 8) | p_addr[1]));

	return;
}

/*
 * Set the MAC address of the Ethernet device
 */
int titan_ge_set_mac_address(struct net_device *netdev, void *addr)
{
	int 	i;

	for (i = 0; i < 6; i++)
        	netdev->dev_addr[i] = ((unsigned char *) addr)[i + 2];

	titan_ge_update_mac_address(netdev);
	return 0;
}

/*
 * Get the Ethernet device stats
 */
static struct net_device_stats *titan_ge_get_stats(struct net_device *netdev)
{
	titan_ge_port_info	*titan_ge_eth;
	struct titan_ge		*port_private;
	unsigned int		port_num;

	titan_ge_eth = netdev->priv;
	port_private = (struct titan_ge *)titan_ge_eth->port_private;
	port_num = port_private->port_num;

	return &port_private->stats;
}

/*
 * Register the Titan GE with the kernel
 */
static int __init titan_ge_init_module(void)
{
	unsigned long	version, device;

	printk(KERN_NOTICE "PMC-Sierra TITAN 10/100/1000 Ethernet Driver \n");
	device = TITAN_GE_READ(TITAN_GE_DEVICE_ID);
	version = (device & 0x000f0000) >> 16;
	device &= 0x0000ffff;

	printk(KERN_NOTICE "Device Id : %lu,  Version : %lu \n", device, version);

	/* Currently we register only one port */
	if (titan_ge_init(0)) {
		printk(KERN_ERR "Error registering the TITAN Ethernet driver \n");
	}

	return 0;
}

/*
 * Unregister the Titan GE from the kernel
 */
static void __init titan_ge_cleanup_module(void)
{
	/* Nothing to do here */
}

module_init(titan_ge_init_module);
module_exit(titan_ge_cleanup_module);

/*
 * Initialize the Rx descriptor ring for the Titan Ge 
 */
static int titan_ge_init_rx_desc_ring(titan_ge_port_info *titan_eth_port,
					int rx_desc_num,
					int rx_buff_size,
					unsigned long rx_desc_base_addr,
					unsigned long rx_buff_base_addr,
					unsigned long rx_dma)
{
	volatile titan_ge_rx_desc	*rx_desc;
	unsigned long			buffer_addr;
	int				index;
	unsigned long			titan_ge_rx_desc_bus = rx_dma;

	buffer_addr = rx_buff_base_addr;
	rx_desc = (titan_ge_rx_desc *)rx_desc_base_addr;

	/* Check alignment */
	if (rx_buff_base_addr & 0xF)
                return 0;

	/* Check Rx buffer size */
	if ((rx_buff_size < 8) || (rx_buff_size > TITAN_GE_MAX_RX_BUFFER))
		return 0;

	/* 64-bit alignment */
	if ((rx_buff_base_addr + rx_buff_size) & 0x7)
                return 0;

	/* Initialize the Rx desc ring */
	for (index = 0; index < rx_desc_num; index++) {
		titan_ge_rx_desc_bus += sizeof(titan_ge_rx_desc);
		rx_desc[index].cmd_sts = 
			(((TITAN_GE_RX_BUFFER_OWNED | TITAN_GE_RX_STP) >> 15) || rx_buff_size); 
		rx_desc[index].buffer_addr = buffer_addr;
		
		titan_eth_port->rx_skb[index] = NULL;
		buffer_addr += rx_buff_size;
	}

	titan_eth_port->rx_curr_desc_q = 0;
	titan_eth_port->rx_used_desc_q = 0;

	titan_eth_port->rx_desc_area = (titan_ge_rx_desc *)rx_desc_base_addr;
	titan_eth_port->rx_desc_area_size = rx_desc_num * sizeof(titan_ge_rx_desc);
	
	return TITAN_OK;
}

/* Initialize the Tx descriptor ring */
static int titan_ge_init_tx_desc_ring(titan_ge_port_info *titan_ge_port,
					int tx_desc_num,
					unsigned long tx_desc_base_addr,
					unsigned long tx_dma)
{
	titan_ge_tx_desc	*tx_desc;
	int			index;
	unsigned long		titan_ge_tx_desc_bus = tx_dma;

	if (tx_desc_base_addr & 0xF)
                return 0;

	tx_desc = (titan_ge_tx_desc *) tx_desc_base_addr;

	for (index = 0; index < tx_desc_num; index++) {
		titan_ge_port->tx_dma_array[index] = (dma_addr_t)titan_ge_tx_desc_bus;
		titan_ge_tx_desc_bus += sizeof(titan_ge_tx_desc);
                tx_desc[index].buffer_len = 0x0000;
                tx_desc[index].cmd_sts = 0x00000000;
                tx_desc[index].buffer_addr = 0x00000000;
                titan_ge_port->tx_skb[index] = NULL;
        }

        titan_ge_port->tx_curr_desc_q = 0;
        titan_ge_port->tx_used_desc_q = 0;

        titan_ge_port->tx_desc_area = (titan_ge_tx_desc*) tx_desc_base_addr;
        titan_ge_port->tx_desc_area_size = tx_desc_num * sizeof(titan_ge_tx_desc);

        return TITAN_OK;
}

/*
 * Initialize the device as an Ethernet device
 */
static int titan_ge_init(int port)
{
        titan_ge_port_info      *titan_ge_eth;
        struct titan_ge         *port_private;
        struct net_device       *netdev = 0;

        netdev = init_etherdev(netdev, sizeof(titan_ge_port_info));
        if (!netdev) {
		printk(KERN_ERR "No Titan Ethernet Device \n");
                return -ENODEV;
	}

        netdev->open = titan_ge_open;
        netdev->stop = titan_ge_stop;
        netdev->hard_start_xmit = titan_ge_start_xmit;
        netdev->get_stats = titan_ge_get_stats;
        netdev->set_mac_address = titan_ge_set_mac_address;

        /* Tx timeout */
        netdev->tx_timeout = titan_ge_tx_timeout;
        netdev->watchdog_timeo = 2 * HZ;

        netdev->tx_queue_len = TITAN_GE_TX_QUEUE;
	netif_carrier_off(netdev);
        netdev->base_addr = 0;
        netdev->change_mtu = &titan_ge_change_mtu;

        titan_ge_eth = netdev->priv;
        memset(titan_ge_eth, 0, sizeof(titan_ge_port_info));

        /* Allocation of memory for the driver structures */
        titan_ge_eth->port_private = (void *)
            kmalloc(sizeof(struct titan_ge), GFP_KERNEL);

        if (!titan_ge_eth->port_private) {
                kfree(titan_ge_eth);
                kfree(netdev);
                return -ENOMEM;
       }
        titan_ge_eth->port_num = 0;
        port_private = (struct titan_ge *)titan_ge_eth->port_private;
        port_private->port_num = port;

        memset(&port_private->stats, 0, sizeof(struct net_device_stats));

	/* Configure the Tx timeout handler */
	INIT_TQUEUE(&titan_ge_eth->tx_timeout_task,
                        (void (*)(void *))titan_ge_tx_timeout_task, netdev);

        /* Init spinlock */
        spin_lock_init(&port_private->lock);

        /* set MAC addresses */
        memcpy(netdev->dev_addr, titan_ge_mac_addr_base, 6);
        netdev->dev_addr[5] += port;

        printk(KERN_NOTICE "%s: port %d with MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
                netdev->name, port,
                netdev->dev_addr[0], netdev->dev_addr[1], netdev->dev_addr[2],
                netdev->dev_addr[3], netdev->dev_addr[4], netdev->dev_addr[5]);

	/* GMII stuff goes here */

        return 0;
}

/*
 * Reset the Ethernet port
 */
static void titan_ge_port_reset(unsigned int port_num)
{
	unsigned int	reg_data;

	/* Stop the Tx port activity */
	reg_data = TITAN_GE_READ(TITAN_GE_TMAC_CONFIG_1);
	reg_data &= ~(0x0001);
	TITAN_GE_WRITE(TITAN_GE_TMAC_CONFIG_1, reg_data);

	/* Stop the Rx port activity */
	reg_data = TITAN_GE_READ(TITAN_GE_RMAC_CONFIG_1);
	reg_data &= ~(0x0001);
	TITAN_GE_WRITE(TITAN_GE_RMAC_CONFIG_1, reg_data);

	return;
}

/*
 * Return the Tx desc after use by the XDMA 
 */
static int titan_ge_return_tx_desc(titan_ge_port_info *titan_ge_eth,
					 titan_ge_packet *packet) 
{
	int				tx_desc_used, tx_desc_curr;
	volatile titan_ge_tx_desc	*tx_desc;

	tx_desc_curr = titan_ge_eth->tx_curr_desc_q;
	tx_desc_used = titan_ge_eth->tx_used_desc_q;
	
	tx_desc = &(titan_ge_eth->tx_desc_area[tx_desc_curr]);
	
	if (tx_desc == NULL)
		return TITAN_ERROR;

	if (tx_desc->cmd_sts & TITAN_GE_TX_BUFFER_OWNED) 
		return TITAN_ERROR;

	if (tx_desc_used == tx_desc_curr)
		return TITAN_ERROR;

	/* Now the critical stuff */
	packet->cmd_sts = (tx_desc->cmd_sts & ~(0x1f)); /* Get the command and status only */
	packet->skb = titan_ge_eth->tx_skb[tx_desc_curr];
	
	titan_ge_eth->tx_skb[tx_desc_curr] = NULL;
	titan_ge_eth->tx_used_desc_q = (tx_desc_used + 1) % TITAN_GE_TX_QUEUE;
	
	return TITAN_OK;
}

/*
 * Coalescing for the Rx path
 */
static unsigned long titan_ge_rx_coal(unsigned long delay)
{
	TITAN_GE_WRITE(TITAN_GE_INT_COALESCING, delay);
	return delay;
}

/*
 * Coalescing for the Tx path
 */
static unsigned long titan_ge_tx_coal(unsigned long delay)
{
	unsigned long	tx_delay;

	tx_delay = TITAN_GE_READ(TITAN_GE_INT_COALESCING);
	delay = (delay << 16) | tx_delay;

	TITAN_GE_WRITE(TITAN_GE_INT_COALESCING, delay);

	return delay;
}

