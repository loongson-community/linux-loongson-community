/*
 * Pb1200/DBAu1200 board platform device registration
 *
 * Copyright (C) 2008 MontaVista Software Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mmc/host.h>

#include <asm/mach-au1x00/au1xxx.h>
#include <asm/mach-au1x00/au1xxx_dbdma.h>
#include <asm/mach-au1x00/au1100_mmc.h>

#if defined(CONFIG_MIPS_PB1200)
#include <asm/mach-pb1x00/pb1200.h>
#elif defined(CONFIG_MIPS_DB1200)
#include <asm/mach-db1x00/db1200.h>
#endif

static struct resource ide_resources[] = {
	[0] = {
		.start	= IDE_PHYS_ADDR,
		.end 	= IDE_PHYS_ADDR + IDE_PHYS_LEN - 1,
		.flags	= IORESOURCE_MEM
	},
	[1] = {
		.start	= IDE_INT,
		.end	= IDE_INT,
		.flags	= IORESOURCE_IRQ
	}
};

static u64 ide_dmamask = ~(u32)0;

static struct platform_device ide_device = {
	.name		= "au1200-ide",
	.id		= 0,
	.dev = {
		.dma_mask 		= &ide_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ide_resources),
	.resource	= ide_resources
};

static struct resource smc91c111_resources[] = {
	[0] = {
		.name	= "smc91x-regs",
		.start	= SMC91C111_PHYS_ADDR,
		.end	= SMC91C111_PHYS_ADDR + 0xf,
		.flags	= IORESOURCE_MEM
	},
	[1] = {
		.start	= SMC91C111_INT,
		.end	= SMC91C111_INT,
		.flags	= IORESOURCE_IRQ
	},
};

static struct platform_device smc91c111_device = {
	.name		= "smc91x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smc91c111_resources),
	.resource	= smc91c111_resources
};

static const struct {
	u16 bcsrpwr;
	u16 bcsrstatus;
	u16 wpstatus;
} au1xmmc_card_table[] = {
	{
		.bcsrpwr	= BCSR_BOARD_SD0PWR,
		.bcsrstatus	= BCSR_INT_SD0INSERT,
		.wpstatus	= BCSR_STATUS_SD0WP
	},
#ifndef CONFIG_MIPS_DB1200
	{
		.bcsrpwr	= BCSR_BOARD_SD1PWR,
		.bcsrstatus	= BCSR_INT_SD1INSERT,
		.wpstatus	= BCSR_STATUS_SD1WP
	}
#endif
};

static void pb1200mmc_set_power(void *mmc_host, int state)
{
	struct au1xmmc_host *host = mmc_priv((struct mmc_host *)mmc_host);
	u32 val = au1xmmc_card_table[host->id].bcsrpwr;

	if (state)
		bcsr->board |= val;
	else
		bcsr->board &= ~val;

	au_sync_delay(1);
}

static int pb1200mmc_card_readonly(void *mmc_host)
{
	struct au1xmmc_host *host = mmc_priv((struct mmc_host *)mmc_host);

	return (bcsr->status & au1xmmc_card_table[host->id].wpstatus) ? 1 : 0;
}

static int pb1200mmc_card_inserted(void *mmc_host)
{
	struct au1xmmc_host *host = mmc_priv((struct mmc_host *)mmc_host);

	return (bcsr->sig_status & au1xmmc_card_table[host->id].bcsrstatus)
		? 1 : 0;
}

static struct au1xmmc_platdata db1xmmcpd = {
	.set_power	= pb1200mmc_set_power,
	.card_inserted	= pb1200mmc_card_inserted,
	.card_readonly	= pb1200mmc_card_readonly,
	.cd_setup	= NULL,		/* use poll-timer in driver */
};

static u64 au1xxx_mmc_dmamask =  ~(u32)0;

struct resource au1200_sd0_res[] = {
	[0] = {
		.start	= CPHYSADDR(SD0_BASE),
		.end	= CPHYSADDR(SD0_BASE) + 0x40,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= AU1200_SD_INT,
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
		.start	= DSCR_CMD0_SDMS_TX0,
		.flags	= IORESOURCE_DMA,
	},
	[4] = {
		.start	= DSCR_CMD0_SDMS_RX0,
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device au1xxx_sd0_device = {
	.name	= "au1xxx-mmc",
	.id	= 0,        /* index into au1xmmc_card_table[] */
	.dev	= {
		.dma_mask		= &au1xxx_mmc_dmamask,
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &db1xmmcpd,
	},
	.num_resources	= ARRAY_SIZE(au1200_sd0_res),
	.resource	= au1200_sd0_res,
};

#ifndef CONFIG_MIPS_DB1200
struct resource au1200_sd1_res[] = {
	[0] = {
		.start  = CPHYSADDR(SD1_BASE),
		.end    = CPHYSADDR(SD1_BASE) + 0x40,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		.start  = AU1200_SD_INT,
		.flags  = IORESOURCE_IRQ,
	},
	[3] = {
		.start  = DSCR_CMD0_SDMS_TX1,
		.flags  = IORESOURCE_DMA,
	},
	[4] = {
		.start  = DSCR_CMD0_SDMS_RX1,
		.flags  = IORESOURCE_DMA,
	},
};

static struct platform_device au1xxx_sd1_device = {
	.name = "au1xxx-mmc",
	.id = 1,		/* index into au1xmmc_card_table[] */
	.dev = {
		.dma_mask		= &au1xxx_mmc_dmamask,
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &db1xmmcpd,
	},
	.num_resources	= ARRAY_SIZE(au1200_sd1_res),
	.resource	= au1200_sd1_res,
};
#endif

static struct platform_device *board_platform_devices[] __initdata = {
	&ide_device,
	&smc91c111_device,
	&au1xxx_sd0_device,
#ifndef CONFIG_MIPS_DB1200
	&au1xxx_sd1_device,
#endif
};

static int __init board_register_devices(void)
{
	return platform_add_devices(board_platform_devices,
				    ARRAY_SIZE(board_platform_devices));
}

arch_initcall(board_register_devices);
