/* sparc_esp.c:  EnhancedScsiProcessor Sun SCSI driver code.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 *
 * Ugly generalization hacks by Jesper Skov. See "esp.c".
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/malloc.h>
#include <linux/blk.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>

#include "scsi.h"
#include "hosts.h"
#include "esp.h"

#include "sparc_esp.h"
#include <asm/sbus.h>
#include <asm/dma.h>
#include <asm/machines.h>
#include <asm/oplib.h>
#include <asm/vaddrs.h>
#include <asm/pgtable.h>

extern struct Sparc_ESP *espchain;

static void dma_barrier(struct Sparc_ESP *esp);
static int  dma_bytes_sent(struct Sparc_ESP *esp, int fifo_count);
static int  dma_can_transfer(struct Sparc_ESP *esp, Scsi_Cmnd *sp);
static void dma_drain(struct Sparc_ESP *esp);
static void dma_dump_state(struct Sparc_ESP *esp);
static void dma_init_read(struct Sparc_ESP *esp, __u32 vaddress, int length);
static void dma_init_write(struct Sparc_ESP *esp, __u32 vaddress, int length);
static void dma_ints_off(struct Sparc_ESP *esp);
static void dma_ints_on(struct Sparc_ESP *esp);
static void dma_invalidate(struct Sparc_ESP *esp);
static void  dma_irq_entry(struct Sparc_ESP *esp);
static void  dma_irq_exit(struct Sparc_ESP *esp);
static int  dma_irq_p(struct Sparc_ESP *esp);
static void dma_poll(struct Sparc_ESP *esp, unsigned char *vaddr);
static int  dma_ports_p(struct Sparc_ESP *esp);
static void dma_reset(struct Sparc_ESP *esp);
static void dma_setup(struct Sparc_ESP *esp, __u32 addr, int count, int write);
static void dma_mmu_get_scsi_one (struct Sparc_ESP *esp, Scsi_Cmnd *sp);
static void dma_mmu_get_scsi_sgl (struct Sparc_ESP *esp, Scsi_Cmnd *sp);
static void dma_mmu_release_scsi_one (struct Sparc_ESP *esp, Scsi_Cmnd *sp);
static void dma_mmu_release_scsi_sgl (struct Sparc_ESP *esp, Scsi_Cmnd *sp);
static void dma_advance_sg (Scsi_Cmnd *sp);

/* Detecting ESP chips on the machine.  This is the simple and easy
 * version.
 */
int esp_detect(Scsi_Host_Template *tpnt)
{
	struct Sparc_ESP *esp, *elink;
	struct Scsi_Host *esp_host;
	struct linux_sbus *sbus;
	struct linux_sbus_device *esp_edev, *esp_dev, *sbdev_iter;
	struct ESP_regs *eregs;
	struct sparc_dma_registers *dregs;
	struct Linux_SBus_DMA *dma, *dlink;
	unchar bsizes, bsizes_more;
	int esp_node;

	if(!SBus_chain)
		panic("No SBUS in esp_detect()");
	for_each_sbus(sbus) {
		for_each_sbusdev(sbdev_iter, sbus) {
			struct linux_sbus_device *espdma = 0;
			int hme = 0;

			/* Is it an esp sbus device? */
			esp_dev = sbdev_iter;
			if(strcmp(esp_dev->prom_name, "esp") &&
			   strcmp(esp_dev->prom_name, "SUNW,esp")) {
				if(!strcmp(esp_dev->prom_name, "SUNW,fas")) {
					hme = 1;
					espdma = esp_dev;
				} else {
					if(!esp_dev->child ||
					   (strcmp(esp_dev->prom_name, "espdma") &&
					    strcmp(esp_dev->prom_name, "dma")))
						continue; /* nope... */
					espdma = esp_dev;
					esp_dev = esp_dev->child;
					if(strcmp(esp_dev->prom_name, "esp") &&
					   strcmp(esp_dev->prom_name, "SUNW,esp"))
						continue; /* how can this happen? */
				}
			}

			esp = esp_allocate(tpnt, (void *) esp_dev);

			esp_host = esp->ehost;

			if(hme)
				esp_host->max_id = 16;
			
			/* Do command transfer with DMA */
			esp->do_pio_cmds = 0;

			/* Required functions */
			esp->dma_bytes_sent = &dma_bytes_sent;
			esp->dma_can_transfer = &dma_can_transfer;
			esp->dma_dump_state = &dma_dump_state;
			esp->dma_init_read = &dma_init_read;
			esp->dma_init_write = &dma_init_write;
			esp->dma_ints_off = &dma_ints_off;
			esp->dma_ints_on = &dma_ints_on;
			esp->dma_irq_p = &dma_irq_p;
			esp->dma_ports_p = &dma_ports_p;
			esp->dma_setup = &dma_setup;

			/* Optional functions */
			esp->dma_barrier = &dma_barrier;
			esp->dma_drain = &dma_drain;
			esp->dma_invalidate = &dma_invalidate;
			esp->dma_irq_entry = &dma_irq_entry;
			esp->dma_irq_exit = &dma_irq_exit;
			esp->dma_led_on = 0;
			esp->dma_led_off = 0;
			esp->dma_poll = &dma_poll;
			esp->dma_reset = &dma_reset;

		        /* virtual DMA functions */
		        esp->dma_mmu_get_scsi_one = &dma_mmu_get_scsi_one;
		        esp->dma_mmu_get_scsi_sgl = &dma_mmu_get_scsi_sgl;
		        esp->dma_mmu_release_scsi_one = &dma_mmu_release_scsi_one;
		        esp->dma_mmu_release_scsi_sgl = &dma_mmu_release_scsi_sgl;
		        esp->dma_advance_sg = &dma_advance_sg;
		    
			/* Get misc. prom information */
#define ESP_IS_MY_DVMA(esp, dma)  \
			((((struct linux_sbus_device*)(esp->edev))->my_bus == dma->SBus_dev->my_bus) && \
			 (((struct linux_sbus_device*)(esp->edev))->slot == dma->SBus_dev->slot) && \
			 (!strcmp(dma->SBus_dev->prom_name, "dma") || \
			  !strcmp(dma->SBus_dev->prom_name, "espdma")))

			esp_node = esp_dev->prom_node;
			prom_getstring(esp_node, "name", esp->prom_name,
				       sizeof(esp->prom_name));
			esp->prom_node = esp_node;
			if(espdma) {
				for_each_dvma(dlink) {
					if(dlink->SBus_dev == espdma)
						break;
				}
			} else {
				for_each_dvma(dlink) {
					if(ESP_IS_MY_DVMA(esp, dlink) &&
					   !dlink->allocated)
						break;
				}
			}
#undef ESP_IS_MY_DVMA
			/* If we don't know how to handle the dvma,
			 * do not use this device.
			 */
			if(!dlink){
				printk ("Cannot find dvma for ESP%d's SCSI\n",
					esp->esp_id);
				scsi_unregister (esp_host);
				continue;
			}
			if (dlink->allocated){
				printk ("esp%d: can't use my espdma\n",
					esp->esp_id);
				scsi_unregister (esp_host);
				continue;
			}
			dlink->allocated = 1;
			dma = dlink;
			esp->dma = (struct Linux_SBus_DMA *) dma;
			esp->dregs = (void *) dregs = dma->regs;

			esp_edev = (struct linux_sbus_device *) esp->edev;

			/* Map in the ESP registers from I/O space */
			if(!hme) {
				prom_apply_sbus_ranges(esp_edev->my_bus, 
									   esp_edev->reg_addrs,
									   1, esp_edev);
				esp->eregs = eregs = (struct ESP_regs *)
					sparc_alloc_io(esp_edev->reg_addrs[0].phys_addr, 0,
								   PAGE_SIZE, "ESP Registers",
								   esp_edev->reg_addrs[0].which_io, 0x0);
			} else {
				/* On HME, two reg sets exist, first is DVMA,
				 * second is ESP registers.
				 */
				esp->eregs = eregs = (struct ESP_regs *)
				sparc_alloc_io(esp_edev->reg_addrs[1].phys_addr, 0,
					       PAGE_SIZE, "ESP Registers",
					       esp_edev->reg_addrs[1].which_io, 0x0);
			}
			if(!eregs)
				panic("ESP registers unmappable");
			esp->esp_command =
				sparc_dvma_malloc(16, "ESP DVMA Cmd Block",
					&esp->esp_command_dvma);
			if(!esp->esp_command || !esp->esp_command_dvma)
				panic("ESP DVMA transport area unmappable");

			/* Set up the irq's etc. */
			esp->ehost->io_port = 
				esp_edev->reg_addrs[0].phys_addr;
			esp_host->n_io_port = (unsigned char)
				esp_edev->reg_addrs[0].reg_size;
			esp->irq = esp_edev->irqs[0].pri;

			/* Allocate the irq only if necessary */
			for_each_esp(elink) {
				if((elink != esp) && (esp->irq == elink->irq)) {
					goto esp_irq_acquired; /* BASIC rulez */
				}
			}
			if(request_irq(esp->irq, do_esp_intr, SA_SHIRQ,
				       "Sparc ESP SCSI", NULL))
				panic("Cannot acquire ESP irq line");
esp_irq_acquired:
			printk("esp%d: IRQ %d ", esp->esp_id, esp->irq);

			/* Figure out our scsi ID on the bus */
			esp->scsi_id = prom_getintdefault(esp->prom_node,
							  "initiator-id",
							  -1);
			if(esp->scsi_id == -1)
				esp->scsi_id = prom_getintdefault(esp->prom_node,
								  "scsi-initiator-id",
								  -1);
			if(esp->scsi_id == -1)
				esp->scsi_id =
					prom_getintdefault(esp_edev->my_bus->prom_node,
							   "scsi-initiator-id",
							   7);

			/* Check for differential SCSI-bus */
			esp->diff = prom_getbool(esp->prom_node, "differential");
			if(esp->diff)
				printk("Differential ");


			/* SCSI chip clock */
			esp->cfreq = prom_getintdefault(esp->prom_node,
						  "clock-frequency",
						  -1);
			if(esp->cfreq==-1)
				esp->cfreq = prom_getintdefault(esp_edev->my_bus->prom_node,
							  "clock-frequency",
							  -1);

			/* Find the burst sizes this dma/sbus/esp supports. */
			bsizes = prom_getintdefault(esp->prom_node, "burst-sizes", 0xff);
			bsizes &= 0xff;
			if(espdma) {
				bsizes_more = prom_getintdefault(
						  espdma->prom_node,
						  "burst-sizes", 0xff);
				if(bsizes_more != 0xff)
					bsizes &= bsizes_more;
			}
			bsizes_more = prom_getintdefault(esp_edev->my_bus->prom_node,
							 "burst-sizes", 0xff);
			if(bsizes_more != 0xff)
				bsizes &= bsizes_more;

			if(bsizes == 0xff || (bsizes & DMA_BURST16)==0 ||
			   (bsizes & DMA_BURST32)==0)
				bsizes = (DMA_BURST32 - 1);

			esp->bursts = bsizes;

			esp_initialize(esp);

		} /* for each sbusdev */
	} /* for each sbus */
	printk("ESP: Total of %d ESP hosts found, %d actually in use.\n", nesps,
	       esps_in_use);
	esps_running = esps_in_use;
	return esps_in_use;
}


/************************************************************* DMA Functions */
/* XXXX UGLY! : Duplicate esp_cmd here so it can remain inline in esp.c */
static inline void esp_cmd(struct Sparc_ESP *esp, 
						   struct ESP_regs *eregs, unchar cmd)
{
#ifdef DEBUG_ESP_CMDS
	esp->espcmdlog[esp->espcmdent] = cmd;
	esp->espcmdent = (esp->espcmdent + 1) & 31;
#endif
	eregs->esp_cmd = cmd;
}

static void dma_barrier(struct Sparc_ESP *esp)
{
	struct sparc_dma_registers *dregs =
		(struct sparc_dma_registers *) esp->dregs;

	/* XXX HME/FAS ATN deassert workaround required,
	 * XXX no DMA flushing, only possible ESP_CMD_FLUSH
	 * XXX to kill the fifo.
	 */
	if(esp->erev != fashme) {
		while(dregs->cond_reg & DMA_PEND_READ)
			udelay(1);
		dregs->cond_reg &= ~(DMA_ENABLE);
		dma_invalidate(esp);
	} else {
		esp_cmd(esp, esp->eregs, ESP_CMD_FLUSH);
	}
}

/* This uses various DMA csr fields and the fifo flags count value to
 * determine how many bytes were successfully sent/received by the ESP.
 */
static int dma_bytes_sent(struct Sparc_ESP *esp, int fifo_count)
{
	struct sparc_dma_registers *dregs = 
		(struct sparc_dma_registers *) esp->dregs;

	int rval = dregs->st_addr - esp->esp_command_dvma;

	if(((struct Linux_SBus_DMA *) esp->dma)->revision == dvmarev1)
		rval -= (4 - ((dregs->cond_reg & DMA_READ_AHEAD)>>11));
	return rval - fifo_count;
}

static int dma_can_transfer(struct Sparc_ESP *esp, Scsi_Cmnd *sp)
{
	__u32 base, end, sz;
	enum dvma_rev drev = ((struct Linux_SBus_DMA *) esp->dma)->revision;

	if(drev == dvmarev3) {
		sz = sp->SCp.this_residual;
		if(sz > 0x1000000)
			sz = 0x1000000;
	} else {
		base = ((__u32)sp->SCp.ptr);
		base &= (0x1000000 - 1);
		end = (base + sp->SCp.this_residual);
		if(end > 0x1000000)
			end = 0x1000000;
		sz = (end - base);
	}
	return sz;
}

static void dma_drain(struct Sparc_ESP *esp)
{
	struct sparc_dma_registers *dregs =
		(struct sparc_dma_registers *) esp->dregs;
	enum dvma_rev drev = ((struct Linux_SBus_DMA *) esp->dma)->revision;

	if(drev == dvmahme)
		return;
	if(dregs->cond_reg & DMA_FIFO_ISDRAIN) {
		switch(drev) {
		default:
			dregs->cond_reg |= DMA_FIFO_STDRAIN;

		case dvmarev3:
		case dvmaesc1:
			while(dregs->cond_reg & DMA_FIFO_ISDRAIN)
				udelay(1);
		};
	}
}

static void dma_dump_state(struct Sparc_ESP *esp)
{
	struct sparc_dma_registers *dregs =
		(struct sparc_dma_registers *) esp->dregs;

	ESPLOG(("esp%d: dma -- cond_reg<%08lx> addr<%p>\n",
		esp->esp_id, dregs->cond_reg, dregs->st_addr));
}

static void dma_flashclear(struct Sparc_ESP *esp)
{
	dma_drain(esp);
	dma_invalidate(esp);
}

static void dma_init_read(struct Sparc_ESP *esp, __u32 vaddress, int length)
{
	struct sparc_dma_registers *dregs = 
		(struct sparc_dma_registers *) esp->dregs;

	dregs->cond_reg |= (DMA_ST_WRITE | DMA_ENABLE);
	if(((struct Linux_SBus_DMA *) esp->dma)->revision == dvmaesc1)
		dregs->cnt = 0x1000;
	dregs->st_addr = vaddress;
}

static void dma_init_write(struct Sparc_ESP *esp, __u32 vaddress, int length)
{
	struct sparc_dma_registers *dregs = 
		(struct sparc_dma_registers *) esp->dregs;

	if(esp->erev == fashme) {
		unsigned long tmp;

		/* Talk about touchy hardware... */
		tmp = dregs->cond_reg;
		tmp |= (DMA_SCSI_DISAB | DMA_ENABLE);
		tmp &= ~(DMA_ST_WRITE);
		dregs->cnt = length;
		dregs->st_addr = vaddress;
		dregs->cond_reg = tmp;
	} else {
		/* Set up the DMA counters */
		dregs->cond_reg = ((dregs->cond_reg & ~(DMA_ST_WRITE)) | DMA_ENABLE);
		if(((struct Linux_SBus_DMA *) esp->dma)->revision == dvmaesc1) {
			if(length) /* Workaround ESC gate array SBUS rerun bug. */
				dregs->cnt = (PAGE_SIZE);
		}
		dregs->st_addr = vaddress;
	}
}

static void dma_ints_off(struct Sparc_ESP *esp)
{
	DMA_INTSOFF((struct sparc_dma_registers *) esp->dregs);
}

static void dma_ints_on(struct Sparc_ESP *esp)
{
	DMA_INTSON((struct sparc_dma_registers *) esp->dregs);
}

static void dma_invalidate(struct Sparc_ESP *esp)
{
	unsigned int tmp;
	struct sparc_dma_registers *dregs = 
		(struct sparc_dma_registers *) esp->dregs;

	if(((struct Linux_SBus_DMA *) esp->dma)->revision == dvmahme) {
		/* SMCC can bite me. */
		tmp = dregs->cond_reg;
		dregs->cond_reg = DMA_RST_SCSI;

		/* This would explain a lot. */
		tmp |= (DMA_PARITY_OFF|DMA_2CLKS|DMA_SCSI_DISAB);

		tmp &= ~(DMA_ENABLE|DMA_ST_WRITE);
		dregs->cond_reg = 0;
		dregs->cond_reg = tmp;
	} else {
		while(dregs->cond_reg & DMA_PEND_READ)
			udelay(1);

		tmp = dregs->cond_reg;
		tmp &= ~(DMA_ENABLE | DMA_ST_WRITE | DMA_BCNT_ENAB);
		tmp |= DMA_FIFO_INV;
		dregs->cond_reg = tmp;
		dregs->cond_reg = (tmp & ~(DMA_FIFO_INV));
	}
}

static void dma_irq_entry(struct Sparc_ESP *esp)
{
	DMA_IRQ_ENTRY((struct Linux_SBus_DMA *) esp->dma, 
				  (struct sparc_dma_registers *)esp->dregs);
}

static void dma_irq_exit(struct Sparc_ESP *esp)
{
	DMA_IRQ_EXIT((struct Linux_SBus_DMA *) esp->dma,
				 (struct sparc_dma_registers *) esp->dregs);
}

static int dma_irq_p(struct Sparc_ESP *esp)
{
	return DMA_IRQ_P((struct sparc_dma_registers *) esp->dregs);
}

static void dma_poll(struct Sparc_ESP *esp, unsigned char *vaddr)
{
	if(esp->erev != fashme) {
		dma_flashclear(esp);

		/* Wait till the first bits settle. */
		while(vaddr[0] == 0xff)
			udelay(1);
	} else {
		vaddr[0] = esp->hme_fifo_workaround_buffer[0];
		vaddr[1] = esp->hme_fifo_workaround_buffer[1];
	}
}	

static int dma_ports_p(struct Sparc_ESP *esp)
{
	return (((struct sparc_dma_registers *) esp->dregs)->cond_reg 
			& DMA_INT_ENAB);
}

/* Resetting various pieces of the ESP scsi driver chipset/buses. */
static void dma_reset(struct Sparc_ESP *esp)
{
	struct sparc_dma_registers *dregs =
		(struct sparc_dma_registers *)esp->dregs;
	struct Linux_SBus_DMA *esp_dma = (struct Linux_SBus_DMA *) esp->dma;
	unsigned long tmp, flags;
	int can_do_burst16, can_do_burst32;

	can_do_burst16 = esp->bursts & DMA_BURST16;
	can_do_burst32 = esp->bursts & DMA_BURST32;

	/* Punt the DVMA into a known state. */
	if(esp_dma->revision != dvmahme) {
		dregs->cond_reg |= DMA_RST_SCSI;
		dregs->cond_reg &= ~(DMA_RST_SCSI);
	}
	switch(esp_dma->revision) {
	case dvmahme:
		/* This is the HME DVMA gate array. */

		save_flags(flags); cli(); /* I really hate this chip. */

		dregs->cond_reg = 0x08000000;   /* Reset interface to FAS */
		dregs->cond_reg = DMA_RST_SCSI; /* Reset DVMA itself */

		tmp = (DMA_PARITY_OFF|DMA_2CLKS|DMA_SCSI_DISAB|DMA_INT_ENAB);
		tmp &= ~(DMA_ENABLE|DMA_ST_WRITE|DMA_BRST_SZ);

		if(can_do_burst32)
			tmp |= DMA_BRST32;

		/* This chip is horrible. */
		while(dregs->cond_reg & DMA_PEND_READ)
			udelay(1);

		dregs->cond_reg = 0;

		dregs->cond_reg = tmp;        /* bite me */
		restore_flags(flags);         /* ugh...  */
		break;
	case dvmarev2:
		/* This is the gate array found in the sun4m
		 * NCR SBUS I/O subsystem.
		 */
		if(esp->erev != esp100)
			dregs->cond_reg |= DMA_3CLKS;
		break;
	case dvmarev3:
		dregs->cond_reg &= ~(DMA_3CLKS);
		dregs->cond_reg |= DMA_2CLKS;
		if(can_do_burst32) {
			dregs->cond_reg &= ~(DMA_BRST_SZ);
			dregs->cond_reg |= DMA_BRST32;
		}
		break;
	case dvmaesc1:
		/* This is the DMA unit found on SCSI/Ether cards. */
		dregs->cond_reg |= DMA_ADD_ENABLE;
		dregs->cond_reg &= ~DMA_BCNT_ENAB;
		if(!can_do_burst32 && can_do_burst16) {
			dregs->cond_reg |= DMA_ESC_BURST;
		} else {
			dregs->cond_reg &= ~(DMA_ESC_BURST);
		}
		break;
	default:
		break;
	};
	DMA_INTSON(dregs);
}

static void dma_setup(struct Sparc_ESP *esp, __u32 addr, int count, int write)
{
	struct sparc_dma_registers *dregs = 
		(struct sparc_dma_registers *) esp->dregs;

	unsigned long nreg = dregs->cond_reg;
	if(write)
		nreg |= DMA_ST_WRITE;
	else
		nreg &= ~(DMA_ST_WRITE);
	nreg |= DMA_ENABLE;
	dregs->cond_reg = nreg;
	if(((struct Linux_SBus_DMA *) esp->dma)->revision == dvmaesc1) {
		/* This ESC gate array sucks! */
		__u32 src = addr;
		__u32 dest = src + count;

		if(dest & (PAGE_SIZE - 1))
			count = PAGE_ALIGN(count);
		dregs->cnt = count;
	}
	dregs->st_addr = addr;
}

static void dma_mmu_get_scsi_one (struct Sparc_ESP *esp, Scsi_Cmnd *sp)
{
    /* Sneaky. */
    sp->SCp.have_data_in = mmu_get_scsi_one((char *)sp->SCp.buffer,
					    sp->SCp.this_residual,
					    ((struct linux_sbus_device *)(esp->edev))->my_bus);
    /* XXX The casts are extremely gross, but with 64-bit kernel
     * XXX and 32-bit SBUS what am I to do? -DaveM
     */
    sp->SCp.ptr = (char *)((unsigned long)sp->SCp.have_data_in);
}

static void dma_mmu_get_scsi_sgl (struct Sparc_ESP *esp, Scsi_Cmnd *sp)
{
    mmu_get_scsi_sgl((struct mmu_sglist *) sp->SCp.buffer,
		     sp->SCp.buffers_residual,
		     ((struct linux_sbus_device *) (esp->edev))->my_bus);
    /* XXX Again these casts are sick... -DaveM */
    sp->SCp.ptr=(char *)((unsigned long)sp->SCp.buffer->dvma_address);
}

static void dma_mmu_release_scsi_one (struct Sparc_ESP *esp, Scsi_Cmnd *sp)
{
    /* Sneaky. */
    mmu_release_scsi_one(sp->SCp.have_data_in,
			 sp->request_bufflen,
			 ((struct linux_sbus_device *) (esp->edev))->my_bus);
}

static void dma_mmu_release_scsi_sgl (struct Sparc_ESP *esp, Scsi_Cmnd *sp)
{
    mmu_release_scsi_sgl((struct mmu_sglist *) sp->buffer,
			 sp->use_sg - 1,
			 ((struct linux_sbus_device *) (esp->edev))->my_bus);
}

static void dma_advance_sg (Scsi_Cmnd *sp)
{
    sp->SCp.ptr = (char *)((unsigned long)sp->SCp.buffer->dvma_address);
}
