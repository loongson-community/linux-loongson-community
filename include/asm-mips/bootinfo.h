/*
 * bootinfo.h -- Definition of the Linux/68K boot information structure
 *
 * Written by Ralf Baechle,
 * Copyright (C) 1994 by Waldorf GMBH
 *
 * Based on Linux/68k linux/include/linux/bootstrap.h
 * Copyright (C) 1992 by Greg Harp
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file README.legal in the main directory of this archive
 * for more details.
 */

#ifndef ASM_MIPS_BOOTINFO_H
#define ASM_MIPS_BOOTINFO_H

/*
 * Valid values for machtype field
 */
#define MACH_DESKSTATION_TYNE 1             /* Deskstation Tyne    */

/*
 * Type of CPU
 */
#define CPU_R4600	1

#define CL_SIZE      (80)

struct bootinfo {
    unsigned long
	machtype;			/* machine type */

    unsigned long
	cputype;			/* system CPU & FPU */

    /*
     * Installed RAM
     */
    unsigned int memlower;
    unsigned int memupper;

    /*
     * Cache Information
     */
    unsigned int sec_cache;
    unsigned int dma_cache;

    unsigned long
	ramdisk_size;			/* ramdisk size in 1024 byte blocks */

    unsigned long
	ramdisk_addr;			/* address of the ram disk in mem */

    char
        command_line[CL_SIZE];		/* kernel command line parameters */

};

extern struct bootinfo
    boot_info;

#endif /* ASM_MIPS_BOOTINFO_H */
