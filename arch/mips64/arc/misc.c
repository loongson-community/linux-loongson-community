/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Miscellaneous ARCS PROM routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <asm/bcache.h>
#include <asm/sgialib.h>
#include <asm/bootinfo.h>
#include <asm/system.h>

extern unsigned long mips_cputype;
extern void *sgiwd93_host;
extern void reset_wd33c93(void *instance);

void prom_halt(void)
{
	bcops->bc_disable();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	ARC_CALL0(halt);
never:	goto never;
}

void prom_powerdown(void)
{
	bcops->bc_disable();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	ARC_CALL0(pdown);
never:	goto never;
}

/* XXX is this a soft reset basically? XXX */
void prom_restart(void)
{
	bcops->bc_disable();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	ARC_CALL0(restart);
never:	goto never;
}

void prom_reboot(void)
{
	bcops->bc_disable();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	ARC_CALL0(reboot);
never:	goto never;
}

void prom_imode(void)
{
	bcops->bc_disable();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	ARC_CALL0(imode);
never:	goto never;
}

long prom_cfgsave(void)
{
	return ARC_CALL0(cfg_save);
}

struct linux_sysid *prom_getsysid(void)
{
	return (struct linux_sysid *) ARC_CALL0(get_sysid);
}

void __init prom_cacheflush(void)
{
	ARC_CALL0(cache_flush);
}
