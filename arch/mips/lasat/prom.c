/*
 * PROM interface routines.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/ioport.h>
#include <asm/bootinfo.h>
#include <asm/lasat/lasat.h>
#include <asm/cpu.h>

char arcs_cmdline[CL_SIZE];


void __init prom_init(int argc, char **argv, char **envp, int *prom_vec)
{
	if (mips_cpu.cputype == CPU_R5000)
		mips_machtype = MACH_LASAT_200;
	else
		mips_machtype = MACH_LASAT_100;

	lasat_init_board_info();		/* Read info from EEPROM */

	mips_machgroup = MACH_GROUP_LASAT;

	/* Get the command line */
	if (argc>0) {
		strncpy(arcs_cmdline, argv[0], CL_SIZE-1);
		arcs_cmdline[CL_SIZE-1] = '\0';
	}

	/* Set the I/O base address */
	set_io_port_base(KSEG1);

	/* Set memory regions */
	ioport_resource.start = 0;		/* Should be KSEGx ???	*/
	ioport_resource.end = 0xffffffff;	/* Should be ???	*/
#if 0
	/*
	 * We should do it right, i.e. like this, in stead of passing mem=xxxM.
	 */
	add_memory_region(0, lasat_board_info.li_memsize, BOOT_MEM_RAM);
#endif
}

void prom_free_prom_memory(void)
{
}

void prom_printf(const char * fmt, ...)
{
	return;
}

const char *get_system_type(void)
{
	return lasat_board_info.li_bmstr;
}
