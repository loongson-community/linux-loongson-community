/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * This file contains the MIPS architecture specific IDE code.
 *
 * Copyright (C) 1994-1996  Linus Torvalds & authors
 */

#ifndef __ASM_IDE_H
#define __ASM_IDE_H

#ifdef __KERNEL__

#include <linux/config.h>
#include <asm/byteorder.h>
#include <asm/io.h>

struct ide_ops {
	int (*ide_default_irq)(ide_ioreg_t base);
	ide_ioreg_t (*ide_default_io_base)(int index);
	void (*ide_init_hwif_ports)(hw_regs_t *hw, ide_ioreg_t data_port,
	                            ide_ioreg_t ctrl_port, int *irq);
};

extern struct ide_ops *ide_ops;

static __inline__ int ide_default_irq(ide_ioreg_t base)
{
	return ide_ops->ide_default_irq(base);
}

static __inline__ ide_ioreg_t ide_default_io_base(int index)
{
	return ide_ops->ide_default_io_base(index);
}

static inline void ide_init_hwif_ports(hw_regs_t *hw, ide_ioreg_t data_port,
                                       ide_ioreg_t ctrl_port, int *irq)
{
	ide_ops->ide_init_hwif_ports(hw, data_port, ctrl_port, irq);
}

static __inline__ void ide_init_default_hwifs(void)
{
#ifndef CONFIG_PCI
	hw_regs_t hw;
	int index;

	for(index = 0; index < MAX_HWIFS; index++) {
		ide_init_hwif_ports(&hw, ide_default_io_base(index), 0, NULL);
		hw.irq = ide_default_irq(ide_default_io_base(index));
		ide_register_hw(&hw, NULL);
	}
#endif
}

static __inline__ int ide_request_irq(unsigned int irq, void (*handler)(int,void *, struct pt_regs *),
			unsigned long flags, const char *device, void *dev_id)
{
	return ide_ops->ide_request_irq(irq, handler, flags, device, dev_id);
}

static __inline__ void ide_free_irq(unsigned int irq, void *dev_id)
{
	ide_ops->ide_free_irq(irq, dev_id);
}

static __inline__ int ide_check_region (ide_ioreg_t from, unsigned int extent)
{
	return ide_ops->ide_check_region(from, extent);
}

static __inline__ void ide_request_region(ide_ioreg_t from,
                                          unsigned int extent, const char *name)
{
	ide_ops->ide_request_region(from, extent, name);
}

static __inline__ void ide_release_region(ide_ioreg_t from,
                                          unsigned int extent)
{
	ide_ops->ide_release_region(from, extent);
}

#ifdef __BIG_ENDIAN

/* get rid of defs from io.h - ide has its private and conflicting versions */
#ifdef insw
#undef insw
#endif
#ifdef outsw
#undef outsw
#endif
#ifdef insl
#undef insl
#endif
#ifdef outsl
#undef outsl
#endif

#define insw(port, addr, count) ide_insw(port, addr, count)
#define insl(port, addr, count) ide_insl(port, addr, count)
#define outsw(port, addr, count) ide_outsw(port, addr, count)
#define outsl(port, addr, count) ide_outsl(port, addr, count)

#endif /* __BIG_ENDIAN */

#endif /* __KERNEL__ */

#endif /* __ASM_IDE_H */
