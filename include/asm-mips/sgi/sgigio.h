/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * sgigio.h: Definitions for GIO bus found on SGI IP22 (and others by linux
 *           unsupported) machines.
 *
 * Copyright (C) 2002 Ladislav Michl
 */
#ifndef _ASM_SGI_SGIGIO_H
#define _ASM_SGI_SGIGIO_H

/*
 * There is 10MB of GIO address space for GIO64 slot devices
 * slot#   slot type address range            size
 * -----   --------- ----------------------- -----
 *   0     GFX       0x1f000000 - 0x1f3fffff   4MB
 *   1     EXP0      0x1f400000 - 0x1f5fffff   2MB
 *   2     EXP1      0x1f600000 - 0x1f9fffff   4MB
 *
 * There are un-slotted devices, HPC, I/O and misc devices, which are grouped
 * into the HPC address space.
 *   -     MISC      0x1fb00000 - 0x1fbfffff   1MB
 *
 * Following space is reserved and unused
 *   -     RESERVED  0x18000000 - 0x1effffff 112MB
 *
 * The GIO specification tends to use slot numbers while the MC specification
 * tends to use slot types.
 *
 * slot0  - the "graphics" (GFX) slot but there is no requirement that
 *          a graphics dev may only use this slot
 * slot1  - this is the "expansion"-slot 0 (EXP0), do not confuse with
 *          slot 0 (GFX).
 * slot2  - this is the "expansion"-slot 1 (EXP1), do not confuse with
 *          slot 1 (EXP0).
 */

#define GIO_SLOT_GFX	0
#define GIO_SLOT_GIO1	1
#define GIO_SLOT_GIO2	2
#define GIO_NUM_SLOTS	3

#define GIO_ANY_ID	0xff

#define GIO_VALID_ID_ONLY	0x01
#define GIO_IFACE_64		0x02
#define GIO_HAS_ROM		0x04

struct gio_dev {
	unsigned char	device;
	unsigned char	revision;
	unsigned short	vendor;
	unsigned char	flags;

	unsigned char	slot_number;
	unsigned long	base_addr;
	unsigned int	map_size;

	char		*name;
	char		slot_name[5];
};

extern struct gio_dev* gio_find_device(unsigned char device, const struct gio_dev *from);

extern void sgigio_init(void);

#endif /* _ASM_SGI_SGIGIO_H */
