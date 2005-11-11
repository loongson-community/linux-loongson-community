/*
 * linux/drivers/ide/mips/au1xxx-ide.c  version 01.30.00        Aug. 02 2005
 *
 * BRIEF MODULE DESCRIPTION
 * AMD Alchemy Au1xxx IDE interface routines over the Static Bus
 *
 * Copyright (c) 2003-2005 AMD, Personal Connectivity Solutions
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Note: for more information, please refer "AMD Alchemy Au1200/Au1550 IDE
 *       Interface and Linux Device Driver" Application Note.
 */
#undef REALLY_SLOW_IO           /* most systems can safely undef this */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/init.h>
#include <linux/ide.h>
#include <linux/sysdev.h>

#include <linux/dma-mapping.h>

#include <asm/io.h>
#include <asm/mach-au1x00/au1xxx.h>
#include <asm/mach-au1x00/au1xxx_dbdma.h>

#ifdef CONFIG_PM
#include <asm/mach-au1x00/au1xxx_pm.h>
#endif

#include <asm/mach-au1x00/au1xxx_ide.h>

#define DRV_NAME	"au1200-ide"
#define DRV_VERSION	"1.0"
#define DRV_AUTHOR	"AMD PCS / Pete Popov <ppopov@embeddedalley.com>"

static _auide_hwif auide_hwif;
static int dbdma_init_done = 0;

/*
 * local I/O functions
 */

#if defined(CONFIG_BLK_DEV_IDE_AU1XXX_PIO_DBDMA)

void auide_insw(unsigned long port, void *addr, u32 count)
{
	_auide_hwif *ahwif = &auide_hwif;
	chan_tab_t *ctp;
	au1x_ddma_desc_t *dp;

	if(!put_dest_flags(ahwif->rx_chan, (void*)addr, count << 1, 
			   DDMA_FLAGS_NOIE)) {
		printk(KERN_ERR "%s failed %d\n", __FUNCTION__, __LINE__);
		return;
	}
	ctp = *((chan_tab_t **)ahwif->rx_chan);
	dp = ctp->cur_ptr;
	while (dp->dscr_cmd0 & DSCR_CMD0_V)
		;
	ctp->cur_ptr = au1xxx_ddma_get_nextptr_virt(dp);
}

void auide_outsw(unsigned long port, void *addr, u32 count)
{
	_auide_hwif *ahwif = &auide_hwif;
	chan_tab_t *ctp;
	au1x_ddma_desc_t *dp;

	if(!put_source_flags(ahwif->tx_chan, (void*)addr,
			     count << 1, DDMA_FLAGS_NOIE)) {
		printk(KERN_ERR "%s failed %d\n", __FUNCTION__, __LINE__);
		return;
	}
	ctp = *((chan_tab_t **)ahwif->tx_chan);
	dp = ctp->cur_ptr;
	while (dp->dscr_cmd0 & DSCR_CMD0_V)
		;
	ctp->cur_ptr = au1xxx_ddma_get_nextptr_virt(dp);
}

#endif

#define SBC_IDE_TIMING(mode) \
         SBC_IDE_##mode##_TWCS | \
         SBC_IDE_##mode##_TCSH | \
         SBC_IDE_##mode##_TCSOFF | \
         SBC_IDE_##mode##_TWP | \
         SBC_IDE_##mode##_TCSW | \
         SBC_IDE_##mode##_TPM | \
         SBC_IDE_##mode##_TA

static void auide_tune_drive(ide_drive_t *drive, byte pio)
{
	int mem_sttime;
	int mem_stcfg;
	u8 speed;

	/* get the best pio mode for the drive */
	pio = ide_get_best_pio_mode(drive, pio, 4, NULL);

	printk(KERN_INFO "%s: setting Au1XXX IDE to PIO mode%d\n",
	       drive->name, pio);

	mem_sttime = 0;
	mem_stcfg  = au_readl(MEM_STCFG2);

	/* set pio mode! */
	switch(pio) {
	case 0:
		mem_sttime = SBC_IDE_TIMING(PIO0);

		/* set configuration for RCS2# */
		mem_stcfg |= TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_PIO0_TCSOE | SBC_IDE_PIO0_TOECS;
		break;

	case 1:
		mem_sttime = SBC_IDE_TIMING(PIO1);

		/* set configuration for RCS2# */
		mem_stcfg |= TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_PIO1_TCSOE | SBC_IDE_PIO1_TOECS;
		break;

	case 2:
		mem_sttime = SBC_IDE_TIMING(PIO2);

		/* set configuration for RCS2# */
		mem_stcfg &= ~TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_PIO2_TCSOE | SBC_IDE_PIO2_TOECS;
		break;

	case 3:
		mem_sttime = SBC_IDE_TIMING(PIO3);

		/* set configuration for RCS2# */
		mem_stcfg |= TS_MASK;
		mem_stcfg &= ~TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_PIO3_TCSOE | SBC_IDE_PIO3_TOECS;

		break;

	case 4:
		mem_sttime = SBC_IDE_TIMING(PIO4);

		/* set configuration for RCS2# */
		mem_stcfg &= ~TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_PIO4_TCSOE | SBC_IDE_PIO4_TOECS;
		break;
	}

	au_writel(mem_sttime,MEM_STTIME2);
	au_writel(mem_stcfg,MEM_STCFG2);

	speed = pio + XFER_PIO_0;
	ide_config_drive_speed(drive, speed);
}

static int auide_tune_chipset (ide_drive_t *drive, u8 speed)
{
	int mem_sttime;
	int mem_stcfg;
	unsigned long mode;

#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA
	if (ide_use_dma(drive))
		mode = ide_dma_speed(drive, 0);
#endif

	mem_sttime = 0;
	mem_stcfg  = au_readl(MEM_STCFG2);

	if (speed >= XFER_PIO_0 && speed <= XFER_PIO_4) {
		auide_tune_drive(drive, speed - XFER_PIO_0);
		return 0;
	}
	      
	switch(speed) {
#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA
	case XFER_MW_DMA_2:
		mem_sttime = SBC_IDE_TIMING(MDMA2);

		/* set configuration for RCS2# */
		mem_stcfg &= ~TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_MDMA2_TCSOE | SBC_IDE_MDMA2_TOECS;

		mode = XFER_MW_DMA_2;
		break;
	case XFER_MW_DMA_1:
		mem_sttime = SBC_IDE_TIMING(MDMA1);

		/* set configuration for RCS2# */
		mem_stcfg &= ~TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_MDMA1_TCSOE | SBC_IDE_MDMA1_TOECS;

		mode = XFER_MW_DMA_1;
		break;
	case XFER_MW_DMA_0:
		mem_sttime = SBC_IDE_TIMING(MDMA0);

		/* set configuration for RCS2# */
		mem_stcfg |= TS_MASK;
		mem_stcfg &= ~TCSOE_MASK;
		mem_stcfg &= ~TOECS_MASK;
		mem_stcfg |= SBC_IDE_MDMA0_TCSOE | SBC_IDE_MDMA0_TOECS;

		mode = XFER_MW_DMA_0;
		break;
#endif
	default:
		return 1;
	}
	
	if (ide_config_drive_speed(drive, mode))
		return 1;

	au_writel(mem_sttime,MEM_STTIME2);
	au_writel(mem_stcfg,MEM_STCFG2);

	return 0;
}

/*
 * Multi-Word DMA + DbDMA functions
 */

#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA

static int auide_build_sglist(ide_drive_t *drive,  struct request *rq)
{
	ide_hwif_t *hwif = drive->hwif;
	_auide_hwif *ahwif = (_auide_hwif*)hwif->hwif_data;
	struct scatterlist *sg = hwif->sg_table;

	ide_map_sg(drive, rq);

	if (rq_data_dir(rq) == READ)
		hwif->sg_dma_direction = DMA_FROM_DEVICE;
	else
		hwif->sg_dma_direction = DMA_TO_DEVICE;

	return dma_map_sg(ahwif->dev, sg, hwif->sg_nents,
			  hwif->sg_dma_direction);
}

static int auide_build_dmatable(ide_drive_t *drive)
{
	int i, iswrite, count = 0;
	ide_hwif_t *hwif = HWIF(drive);

	struct request *rq = HWGROUP(drive)->rq;

	_auide_hwif *ahwif = (_auide_hwif*)hwif->hwif_data;
	struct scatterlist *sg;

	iswrite = (rq_data_dir(rq) == WRITE);
	/* Save for interrupt context */
	ahwif->drive = drive;

	/* Build sglist */
	hwif->sg_nents = i = auide_build_sglist(drive, rq);

	if (!i)
		return 0;

	/* fill the descriptors */
	sg = hwif->sg_table;
	while (i && sg_dma_len(sg)) {
		u32 cur_addr;
		u32 cur_len;

		cur_addr = sg_dma_address(sg);
		cur_len = sg_dma_len(sg);

		while (cur_len) {
			u32 flags = DDMA_FLAGS_NOIE;
			unsigned int tc = (cur_len < 0xfe00)? cur_len: 0xfe00;

			if (++count >= PRD_ENTRIES) {
				printk(KERN_WARNING "%s: DMA table too small\n",
				       drive->name);
				goto use_pio_instead;
			}

			/* Lets enable intr for the last descriptor only */
			if (1==i)
				flags = DDMA_FLAGS_IE;
			else
				flags = DDMA_FLAGS_NOIE;

			if (iswrite) {
				if(!put_source_flags(ahwif->tx_chan, 
						     (void*)(page_address(sg->page) 
							     + sg->offset), 
						     tc, flags)) { 
					printk(KERN_ERR "%s failed %d\n", 
					       __FUNCTION__, __LINE__);
				}
			} else 
			{
				if(!put_dest_flags(ahwif->rx_chan, 
						   (void*)(page_address(sg->page) 
							   + sg->offset), 
						   tc, flags)) { 
					printk(KERN_ERR "%s failed %d\n", 
					       __FUNCTION__, __LINE__);
				}
			}

			cur_addr += tc;
			cur_len -= tc;
		}
		sg++;
		i--;
	}

	if (count)
		return 1;

 use_pio_instead:
	dma_unmap_sg(ahwif->dev,
		     hwif->sg_table,
		     hwif->sg_nents,
		     hwif->sg_dma_direction);

	return 0; /* revert to PIO for this request */
}

static int auide_dma_end(ide_drive_t *drive)
{
	ide_hwif_t *hwif = HWIF(drive);
	_auide_hwif *ahwif = (_auide_hwif*)hwif->hwif_data;

	if (hwif->sg_nents) {
		dma_unmap_sg(ahwif->dev, hwif->sg_table, hwif->sg_nents,
			     hwif->sg_dma_direction);
		hwif->sg_nents = 0;
	}

	return 0;
}

static void auide_dma_exec_cmd(ide_drive_t *drive, u8 command)
{
	/* issue cmd to drive */
	ide_execute_command(drive, command, &ide_dma_intr,
			    (2*WAIT_CMD), NULL);
}

static int auide_dma_setup(ide_drive_t *drive)
{       	
	struct request *rq = HWGROUP(drive)->rq;

	if (!auide_build_dmatable(drive)) {
		ide_map_sg(drive, rq);
		return 1;
	}

	drive->waiting_for_dma = 1;
	return 0;
}

static int auide_dma_check(ide_drive_t *drive)
{
	u8 speed;

#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA

	if( dbdma_init_done == 0 ){
		auide_hwif.white_list = ide_in_drive_list(drive->id,
							  dma_white_list);
		auide_hwif.black_list = ide_in_drive_list(drive->id,
							  dma_black_list);
		auide_hwif.drive = drive;
		auide_ddma_init(&auide_hwif);
		dbdma_init_done = 1;
	}
#endif

	/* Is the drive in our DMA black list? */

	if ( auide_hwif.black_list ) {
		drive->using_dma = 0;

		/* Borrowed the warning message from ide-dma.c */

		printk(KERN_WARNING "%s: Disabling DMA for %s (blacklisted)\n",
		       drive->name, id->model);	       
	}
	else
		drive->using_dma = 1;


	speed = ide_find_best_mode(drive, XFER_PIO | XFER_MWDMA);
	
	if (drive->autodma && (speed & XFER_MODE) != XFER_PIO)
		return HWIF(drive)->ide_dma-on(drive);

	return HWIF(drive)->ide_dma_off_quietly(drive);
}

static int auide_dma_test_irq(ide_drive_t *drive)
{	
	if (drive->waiting_for_dma == 0)
		printk(KERN_WARNING "%s: ide_dma_test_irq \
                                     called while not waiting\n", drive->name);

	/* If dbdma didn't execute the STOP command yet, the
	 * active bit is still set
	 */
	drive->waiting_for_dma++;
	if (drive->waiting_for_dma >= DMA_WAIT_TIMEOUT) {
		printk(KERN_WARNING "%s: timeout waiting for ddma to \
                                     complete\n", drive->name);
		return 1;
	}
	udelay(10);
	return 0;
}

static int auide_dma_host_on(ide_drive_t *drive)
{
	return 0;
}

static int auide_dma_on(ide_drive_t *drive)
{
	drive->using_dma = 1;
	return auide_dma_host_on(drive);
}


static int auide_dma_host_off(ide_drive_t *drive)
{
	return 0;
}

static int auide_dma_off_quietly(ide_drive_t *drive)
{
	drive->using_dma = 0;
	return auide_dma_host_off(drive);
}

static int auide_dma_lostirq(ide_drive_t *drive)
{
	printk(KERN_ERR "%s: IRQ lost\n", drive->name);
	return 0;
}

static void auide_ddma_tx_callback(int irq, void *param, struct pt_regs *regs)
{
	_auide_hwif *ahwif = (_auide_hwif*)param;
	ahwif->drive->waiting_for_dma = 0;
	return;
}

static void auide_ddma_rx_callback(int irq, void *param, struct pt_regs *regs)
{
	_auide_hwif *ahwif = (_auide_hwif*)param;
	ahwif->drive->waiting_for_dma = 0;
	return;
}

#endif /* end CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA */

static void auide_init_source_device( _auide_hwif *auide, u32 dev_id, u32 tsize, u32 width) {
  
	dbdev_tab_t dev;
  
	dev.dev_id = dev_id;	
	dev.dev_physaddr    = (u32)AU1XXX_ATA_PHYS_ADDR;
	dev.dev_intlevel    = 0;
	dev.dev_intpolarity = 0;
  
	dev.dev_tsize       = tsize;
	dev.dev_devwidth    = width;
  
	dev.dev_flags = DEV_FLAGS_SYNC;
  
#if CONFIG_BLK_DEV_IDE_AU1XXX_BURSTABLE_ON
	dev.dev_flags |= DEF_FLAGS_BURSTABLE;
#endif
  
	/* Make the tx device */
	dev.dev_flags |= DEV_FLAGS_OUT;
	auide->tx_dev_id = au1xxx_ddma_add_device( &dev );
 
	/* Make the rx device */
	dev.dev_flags &= ~DEV_FLAGS_OUT;
	dev.dev_flags |= DEV_FLAGS_IN;
 	
	auide->rx_dev_id = au1xxx_ddma_add_device( &dev );
}
  
#if defined(CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA)
  
static int auide_ddma_init(_auide_hwif *auide) {
  
	dbdev_tab_t dev;
	u32 size, width;
  
	if (auide->white_list || auide->black_list) {
		size = 8;
		width = 32;
	}
	else {
		size = 1;
		width = 16;
 
		printk(KERN_ERR "au1xxx-ide: %s is not on ide driver whitelist.\n",
		       auide_hwif.drive->id->model);
		printk(KERN_ERR "            please read 'Documentation/mips/AU1xxx_IDE.README'");
	}
 	       
	auide_init_source_device(auide, AU1XXX_ATA_DDMA_REQ, 8, 32);
  
	/* We also need to add a target device for the DMA */
  
	dev.dev_id          = DSCR_CMD0_ALWAYS;
	dev.dev_physaddr    = (u32)AU1XXX_ATA_PHYS_ADDR;
	dev.dev_intlevel    = 0;
	dev.dev_intpolarity = 0;
	dev.dev_flags       = DEV_FLAGS_ANYUSE;
 	
	dev.dev_tsize       = size;
	dev.dev_devwidth    = width;
 	
	auide->target_dev_id = au1xxx_ddma_add_device(&dev);	
 	
	/* Get a channel for TX */
	auide->tx_chan = au1xxx_dbdma_chan_alloc(auide->target_dev_id,
						 auide->tx_dev_id,
						 auide_ddma_tx_callback,
						 (void*)auide);
 
	/* Get a channel for RX */
	auide->rx_chan = au1xxx_dbdma_chan_alloc(auide->rx_dev_id,
						 auide->target_dev_id,
						 auide_ddma_rx_callback,
						 (void*)auide);
 
	auide->tx_desc_head = (void*)au1xxx_dbdma_ring_alloc(auide->tx_chan,
							     NUM_DESCRIPTORS);
	auide->rx_desc_head = (void*)au1xxx_dbdma_ring_alloc(auide->rx_chan,
							     NUM_DESCRIPTORS);
 
	hwif->dmatable_cpu = dma_alloc_coherent(auide->dev,
						PRD_ENTRIES * PRD_BYTES,        /* 1 Page */
						&hwif->dmatable_dma, GFP_KERNEL);
 	
	au1xxx_dbdma_start( auide->tx_chan );
	au1xxx_dbdma_start( auide->rx_chan );
 
	return 0;
} 
#else
 
static int auide_ddma_init( _auide_hwif *auide )
{
	auide_init_source_device(auide, DSCR_CMD0_ALWAYS, 1, 16);
 
 
	/*
	 * Note: if call back is not enabled, update ctp->cur_ptr manually
	 */
 
	auide->tx_chan = au1xxx_dbdma_chan_alloc(DSCR_CMD0_ALWAYS,
						 auide->tx_dev_id,
						 NULL,
						 (void*)auide);
 
	auide->rx_chan = au1xxx_dbdma_chan_alloc(auide->rx_dev_id,
						 DSCR_CMD0_ALWAYS,
						 NULL,
						 (void*)auide);
 
       
	auide->tx_desc_head = (void*)au1xxx_dbdma_ring_alloc(auide->tx_chan,
							     NUM_DESCRIPTORS);
 
	auide->rx_desc_head = (void*)au1xxx_dbdma_ring_alloc(auide->rx_chan,
							     NUM_DESCRIPTORS);
  
	au1xxx_dbdma_start( auide->tx_chan );
	au1xxx_dbdma_start( auide->rx_chan );
 	
	return 0;
}
#endif

static void auide_setup_ports(hw_regs_t *hw, _auide_hwif *ahwif)
{
	int i;
#define ide_ioreg_t unsigned long
	ide_ioreg_t *ata_regs = hw->io_ports;

	/* FIXME? */
	for (i = 0; i < IDE_CONTROL_OFFSET; i++) {
		*ata_regs++ = (ide_ioreg_t) ahwif->regbase 
			+ (ide_ioreg_t)(i << AU1XXX_ATA_REG_OFFSET);
	}

	/* set the Alternative Status register */
	*ata_regs = (ide_ioreg_t) ahwif->regbase
		+ (ide_ioreg_t)(14 << AU1XXX_ATA_REG_OFFSET);
}

static int au_ide_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	_auide_hwif *ahwif = &auide_hwif;
	ide_hwif_t *hwif;
	struct resource *res;
	int ret = 0;

#if defined(CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA)
	char *mode = "MWDMA2";
#elif defined(CONFIG_BLK_DEV_IDE_AU1XXX_PIO_DBDMA)
	char *mode = "PIO+DDMA(offload)";
#endif

	memset(&auide_hwif, 0, sizeof(_auide_hwif));
	auide_hwif.dev                  = 0;

	ahwif->dev = dev;
	ahwif->irq = platform_get_irq(pdev, 0);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res == NULL) {
		pr_debug("%s %d: no base address\n", DRV_NAME, pdev->id);
		ret = -ENODEV;
		goto out;
	}

	if (!request_mem_region (res->start, res->end-res->start, pdev->name)) {
		pr_debug("%s: request_mem_region failed\n", DRV_NAME);
		ret =  -EBUSY;
		goto out;
	}

	ahwif->regbase = (u32)ioremap(res->start, res->end-res->start);
	if (ahwif->regbase == 0) {
		ret = -ENOMEM;
		goto out;
	}

	/* FIXME:  This might possibly break PCMCIA IDE devices */

	hwif                            = &ide_hwifs[pdev->id];
	hw_regs_t *hw 			= &hwif->hw;
	hwif->irq = hw->irq             = ahwif->irq;
	hwif->chipset                   = ide_au1xxx;

	auide_setup_ports(hw, ahwif);
	memcpy(hwif->io_ports, hw->io_ports, sizeof(hwif->io_ports));

#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_SEQTS_PER_RQ
	hwif->rqsize = CONFIG_BLK_DEV_IDE_AU1XXX_SEQTS_PER_RQ;
	hwif->rqsize                    = ((hwif->rqsize > AU1XXX_ATA_RQSIZE) 
					   || (hwif->rqsize < 32)) ? AU1XXX_ATA_RQSIZE : hwif->rqsize;
#else /* if kernel config is not set */
	hwif->rqsize                    = AU1XXX_ATA_RQSIZE;
#endif

	hwif->ultra_mask                = 0x0;  /* Disable Ultra DMA */
#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA
	hwif->mwdma_mask                = 0x07; /* Multimode-2 DMA  */
	hwif->swdma_mask                = 0x07;
#else
	hwif->mwdma_mask                = 0x0;
	hwif->swdma_mask                = 0x0;
#endif

	hwif->noprobe = 0;
	hwif->drives[0].unmask          = 1;
	hwif->drives[1].unmask          = 1;

	/* hold should be on in all cases */
	hwif->hold                      = 1;
	hwif->mmio                      = 2;

	/* If the user has selected DDMA assisted copies,
	   then set up a few local I/O function entry points 
	*/

#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_PIO_DBDMA	
	hwif->INSW                      = auide_insw;
	hwif->OUTSW                     = auide_outsw;
#endif

	hwif->tuneproc                  = &auide_tune_drive;
	hwif->speedproc                 = &auide_tune_chipset;

#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA
	hwif->ide_dma_off_quietly       = &auide_dma_off_quietly;
	hwif->ide_dma_timeout           = &__ide_dma_timeout;

	hwif->ide_dma_check             = &auide_dma_check;
	hwif->dma_exec_cmd              = &auide_dma_exec_cmd;

	hwif->ide_dma_end               = &auide_dma_end;
	hwif->dma_setup                 = &auide_dma_setup;
	hwif->ide_dma_test_irq          = &auide_dma_test_irq;
	hwif->ide_dma_host_off          = &auide_dma_host_off;
	hwif->ide_dma_host_on           = &auide_dma_host_on;
	hwif->ide_dma_lostirq           = &auide_dma_lostirq;
	hwif->ide_dma_on                = &auide_dma_on;

	hwif->autodma                   = 1;
	hwif->drives[0].autodma         = hwif->autodma;
	hwif->drives[1].autodma         = hwif->autodma;
	hwif->atapi_dma                 = 1;

#else /* !CONFIG_BLK_DEV_IDE_AU1XXX_MDMA2_DBDMA */
	hwif->autodma                   = 0;
	hwif->channel                   = 0;
	hwif->hold                      = 1;
	hwif->select_data               = 0;    /* no chipset-specific code */
	hwif->config_data               = 0;    /* no chipset-specific code */

	hwif->drives[0].autodma         = 0;
	hwif->drives[0].autotune        = 1;    /* 1=autotune, 2=noautotune, 0=default */
#endif
	hwif->drives[0].no_io_32bit     = 1;   
	
	/*Register Driver with PM Framework*/
#ifdef CONFIG_PM
	auide_hwif.pm.lock    = SPIN_LOCK_UNLOCKED;
	auide_hwif.pm.stopped = 0;

	auide_hwif.pm.dev = new_au1xxx_power_device( "ide",
						     &au1200ide_pm_callback,
						     NULL);
	if ( auide_hwif.pm.dev == NULL )
		printk(KERN_INFO "Unable to create a power management \
                                device entry for the au1200-IDE.\n");
	else
		printk(KERN_INFO "Power management device entry for the \
                                au1200-IDE loaded.\n");
#endif

	auide_hwif.hwif                 = hwif;
	hwif->hwif_data                 = &auide_hwif;

#ifdef CONFIG_BLK_DEV_IDE_AU1XXX_PIO_DBDMA           
	auide_ddma_init(&auide_hwif);
	dbdma_init_done = 1;
#endif

	probe_hwif_init(hwif);
	dev_set_drvdata(dev, hwif);

	printk(KERN_INFO "Au1xxx IDE(builtin) configured for %s\n", mode );

 out:
	return ret;
}

static int au_ide_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;
	ide_hwif_t *hwif = dev_get_drvdata(dev);
	_auide_hwif *ahwif = &auide_hwif;

	ide_unregister(hwif - ide_hwifs);

	iounmap((void *)ahwif->regbase);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, res->end - res->start);

	return 0;
}

static struct device_driver au1200_ide_driver = {
	.name		= "au1200-ide",
	.bus		= &platform_bus_type,
	.probe 		= au_ide_probe,
	.remove		= au_ide_remove,
};

static int __init au_ide_init(void)
{
	return driver_register(&au1200_ide_driver);
}

static void au_ide_exit(void)
{
	driver_unregister(&au1200_ide_driver);
}

#ifdef CONFIG_PM
int au1200ide_pm_callback( au1xxx_power_dev_t *dev,\
			   au1xxx_request_t request, void *data) {

	unsigned int d, err = 0;
	unsigned long flags;

	spin_lock_irqsave(auide_hwif.pm.lock, flags);

	switch (request){
	case AU1XXX_PM_SLEEP:
		err = au1xxxide_pm_sleep(dev);
		break;
	case AU1XXX_PM_WAKEUP:
		d = *((unsigned int*)data);
		if ( d > 0 && d <= 99) {
			err = au1xxxide_pm_standby(dev);
		}
		else {
			err = au1xxxide_pm_resume(dev);
		}
		break;
	case AU1XXX_PM_GETSTATUS:
		err = au1xxxide_pm_getstatus(dev);
		break;
	case AU1XXX_PM_ACCESS:
		err = au1xxxide_pm_access(dev);
		break;
	case AU1XXX_PM_IDLE:
		err = au1xxxide_pm_idle(dev);
		break;
	case AU1XXX_PM_CLEANUP:
		err = au1xxxide_pm_cleanup(dev);
		break;
	default:
		err = -1;
		break;
	}

	spin_unlock_irqrestore(auide_hwif.pm.lock, flags);

	return err;	
}

static int au1xxxide_pm_standby( au1xxx_power_dev_t *dev ) {
	return 0; 
}

static int au1xxxide_pm_sleep( au1xxx_power_dev_t *dev ) {

	int retval;
	ide_hwif_t *hwif = auide_hwif.hwif;
	struct request rq;
	struct request_pm_state rqpm;
	ide_task_t args;

	if(auide_hwif.pm.stopped)
		return -1;

	/* 
	 * wait until hard disc is ready
	 */
	if ( wait_for_ready(&hwif->drives[0], 35000) ) {
		printk("Wait for drive sleep timeout!\n");
		retval = -1;
	}

	/*
	 * sequenz to tell the high level ide driver that pm is resuming
	 */
	memset(&rq, 0, sizeof(rq));
	memset(&rqpm, 0, sizeof(rqpm));
	memset(&args, 0, sizeof(args));
	rq.flags = REQ_PM_SUSPEND;
	rq.special = &args;
	rq.pm = &rqpm;
	rqpm.pm_step = ide_pm_state_start_suspend;
	rqpm.pm_state = PMSG_SUSPEND;

	retval = ide_do_drive_cmd(&hwif->drives[0], &rq, ide_wait);

	if (wait_for_ready (&hwif->drives[0], 35000)) {
		printk("Wait for drive sleep timeout!\n");
		retval = -1;
	}

	/*
	 * stop dbdma channels
	 */
	au1xxx_dbdma_reset(auide_hwif.tx_chan);
	au1xxx_dbdma_reset(auide_hwif.rx_chan);

	auide_hwif.pm.stopped = 1;

	return retval;
}

static int au1xxxide_pm_resume( au1xxx_power_dev_t *dev ) {

	int retval;
	ide_hwif_t *hwif = auide_hwif.hwif;
	struct request rq;
	struct request_pm_state rqpm;
	ide_task_t args;

	if(!auide_hwif.pm.stopped)
		return -1;

	/*
	 * start dbdma channels
	 */	
	au1xxx_dbdma_start(auide_hwif.tx_chan);
	au1xxx_dbdma_start(auide_hwif.rx_chan);

	/*
	 * wait until hard disc is ready
	 */
	if (wait_for_ready ( &hwif->drives[0], 35000)) {
		printk("Wait for drive wake up timeout!\n");
		retval = -1;
	}

	/*
	 * sequenz to tell the high level ide driver that pm is resuming
	 */
	memset(&rq, 0, sizeof(rq));
	memset(&rqpm, 0, sizeof(rqpm));
	memset(&args, 0, sizeof(args));
	rq.flags = REQ_PM_RESUME;
	rq.special = &args;
	rq.pm = &rqpm;
	rqpm.pm_step = ide_pm_state_start_resume;
	rqpm.pm_state = PMSG_ON;

	retval = ide_do_drive_cmd(&hwif->drives[0], &rq, ide_head_wait);

	/*
	 * wait for hard disc
	 */
	if ( wait_for_ready(&hwif->drives[0], 35000) ) {
		printk("Wait for drive wake up timeout!\n");
		retval = -1;
	}

	auide_hwif.pm.stopped = 0;

	return retval;
}

static int au1xxxide_pm_getstatus( au1xxx_power_dev_t *dev ) {
	return dev->cur_state;
}

static int au1xxxide_pm_access( au1xxx_power_dev_t *dev ) {
	if (dev->cur_state != AWAKE_STATE)
		return 0;
	else
		return -1;
}

static int au1xxxide_pm_idle( au1xxx_power_dev_t *dev ) {
	return 0;
}

static int au1xxxide_pm_cleanup( au1xxx_power_dev_t *dev ) {
	return 0;
}
#endif /* CONFIG_PM */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AU1200 IDE driver");

module_init(au_ide_init);
module_exit(au_ide_exit);
