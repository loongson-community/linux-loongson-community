/*
 * This file is subject to the terms and conditions of the GNU General Public+ 
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * PROM library initialisation code.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <asm/sgialib.h>

#undef DEBUG_PROM_INIT

/* Master romvec interface. */
struct linux_romvec *romvec;
int prom_argc;
char **prom_argv, **prom_envp;

extern void prom_testtree(void);

void __init prom_init(int argc, char **argv, char **envp, int *prom_vec)
{
	PSYSTEM_PARAMETER_BLOCK pb;
	unsigned short prom_vers, prom_rev;

	romvec = ROMVECTOR;
	pb = PROMBLOCK;
	prom_argc = argc;
	prom_argv = argv;
	prom_envp = envp;

	if (pb->magic != 0x53435241) {
		prom_printf("Aieee, bad prom vector magic %08lx\n", pb->magic);
		while(1)
			;
	}

	prom_init_cmdline();

	prom_vers = pb->ver;
	prom_rev = pb->rev;
	prom_identify_arch();
	printk("PROMLIB: ARC firmware Version %d Revision %d\n",
		    prom_vers, prom_rev);
	prom_meminit();

#ifdef DEBUG_PROM_INIT
	{
		prom_printf("Press a key to reboot\n");
		(void) prom_getchar();
		romvec->imode();
	}
#endif
}
