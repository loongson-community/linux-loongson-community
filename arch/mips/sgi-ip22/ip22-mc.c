/*
 * ip22-mc.c: Routines for manipulating the INDY memory controller.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1999 Andrew R. Baker (andrewb@uab.edu) - Indigo2 changes
 */

#include <linux/init.h>
#include <linux/kernel.h>

#include <asm/addrspace.h>
#include <asm/ptrace.h>
#include <asm/sgialib.h>
#include <asm/sgi/mc.h>
#include <asm/sgi/hpc3.h>
#include <asm/sgi/ip22.h>

/* #define DEBUG_SGIMC */

struct sgimc_regs *sgimc;

#ifdef DEBUG_SGIMC
static inline char *mconfig_string(unsigned long val)
{
	switch(val & SGIMC_MCONFIG_RMASK) {
	case SGIMC_MCONFIG_FOURMB:
		return "4MB";

	case SGIMC_MCONFIG_EIGHTMB:
		return "8MB";

	case SGIMC_MCONFIG_SXTEENMB:
		return "16MB";

	case SGIMC_MCONFIG_TTWOMB:
		return "32MB";

	case SGIMC_MCONFIG_SFOURMB:
		return "64MB";

	case SGIMC_MCONFIG_OTEIGHTMB:
		return "128MB";

	default:
		return "wheee, unknown";
	}
}
#endif

void __init sgimc_init(void)
{
	u32 tmp;

	sgimc = (struct sgimc_regs *)(KSEG1 + SGIMC_BASE);

	printk(KERN_INFO "MC: SGI memory controller Revision %d\n",
	       (int) sgimc->systemid & SGIMC_SYSID_MASKREV);

#ifdef DEBUG_SGIMC
	prom_printf("sgimc_init: memconfig0<%s> mconfig1<%s>\n",
		    mconfig_string(sgimc->mconfig0),
		    mconfig_string(sgimc->mconfig1));

	prom_printf("mcdump: cpuctrl0<%08lx> cpuctrl1<%08lx>\n",
		    sgimc->cpuctrl0, sgimc->cpuctrl1);
	prom_printf("mcdump: divider<%08lx>, gioparm<%04x>\n",
		    sgimc->divider, sgimc->gioparm);
#endif

	/* Place the MC into a known state.  This must be done before
	 * interrupts are first enabled etc.
	 */

	/* Step 0: Make sure we turn off the watchdog in case it's
	 *         still running (which might be the case after a
	 *         soft reboot).
	 */
	tmp = sgimc->cpuctrl0;
	tmp &= ~SGIMC_CCTRL0_WDOG;
	sgimc->cpuctrl0 = tmp;

	/* Step 1: The CPU/GIO error status registers will not latch
	 *         up a new error status until the register has been
	 *         cleared by the cpu.  These status registers are
	 *         cleared by writing any value to them.
	 */
	sgimc->cstat = sgimc->gstat = 0;

	/* Step 2: Enable all parity checking in cpu control register
	 *         zero.
	 */
	tmp = sgimc->cpuctrl0;
	tmp |= (SGIMC_CCTRL0_EPERRGIO | SGIMC_CCTRL0_EPERRMEM |
		SGIMC_CCTRL0_R4KNOCHKPARR);
	sgimc->cpuctrl0 = tmp;

	/* Step 3: Setup the MC write buffer depth, this is controlled
	 *         in cpu control register 1 in the lower 4 bits.
	 */
	tmp = sgimc->cpuctrl1;
	tmp &= ~0xf;
	tmp |= 0xd;
	sgimc->cpuctrl1 = tmp;

	/* Step 4: Initialize the RPSS divider register to run as fast
	 *         as it can correctly operate.  The register is laid
	 *         out as follows:
	 *
	 *         ----------------------------------------
	 *         |  RESERVED  |   INCREMENT   | DIVIDER |
	 *         ----------------------------------------
	 *          31        16 15            8 7       0
	 *
	 *         DIVIDER determines how often a 'tick' happens,
	 *         INCREMENT determines by how the RPSS increment
	 *         registers value increases at each 'tick'. Thus,
	 *         for IP22 we get INCREMENT=1, DIVIDER=1 == 0x101
	 */
	sgimc->divider = 0x101;

	/* Step 5: Initialize GIO64 arbitrator configuration register.
	 *
	 * NOTE: HPC init code in sgihpc_init() must run before us because
	 *       we need to know Guiness vs. FullHouse and the board
	 *       revision on this machine. You have been warned.
	 */

	/* First the basic invariants across all GIO64 implementations. */
	tmp = SGIMC_GIOPAR_HPC64;    /* All 1st HPC's interface at 64bits. */
	tmp |= SGIMC_GIOPAR_ONEBUS;  /* Only one physical GIO bus exists. */

	if (ip22_is_fullhouse()) {
		/* Fullhouse specific settings. */
		if (SGIOC_SYSID_BOARDREV(sgioc->sysid) < 2) {
			tmp |= SGIMC_GIOPAR_HPC264; /* 2nd HPC at 64bits */
			tmp |= SGIMC_GIOPAR_PLINEEXP0; /* exp0 pipelines */
			tmp |= SGIMC_GIOPAR_MASTEREXP1;/* exp1 masters */
			tmp |= SGIMC_GIOPAR_RTIMEEXP0; /* exp0 is realtime */
		} else {
			tmp |= SGIMC_GIOPAR_HPC264; /* 2nd HPC 64bits */
			tmp |= SGIMC_GIOPAR_PLINEEXP0; /* exp[01] pipelined */
			tmp |= SGIMC_GIOPAR_PLINEEXP1;
			tmp |= SGIMC_GIOPAR_MASTEREISA;/* EISA masters */
			tmp |= SGIMC_GIOPAR_GFX64; 	/* GFX at 64 bits */
		}
	} else {
		/* Guiness specific settings. */
		tmp |= SGIMC_GIOPAR_EISA64;     /* MC talks to EISA at 64bits */
		tmp |= SGIMC_GIOPAR_MASTEREISA; /* EISA bus can act as master */
	}
	sgimc->giopar = tmp; /* poof */
}
