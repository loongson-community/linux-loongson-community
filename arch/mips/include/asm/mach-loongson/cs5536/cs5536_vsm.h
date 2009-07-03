/*
 * the Virtual Support Module(VSM) read/write interfaces
 *
 * Copyright (C) 2009 Lemote, Inc. & Institute of Computing Technology
 * Author: Wu Zhangjin <wuzj@lemote.com>
 */

#ifndef	_CS5536_VSM_H
#define	_CS5536_VSM_H

#include <linux/types.h>

#define DECLARE_CS5536_MODULE(name) \
extern void pci_##name##_write_reg(int reg, u32 value); \
extern u32 pci_##name##_read_reg(int reg);

#define DEFINE_CS5536_MODULE(name) \
void pci_##name##_write_reg(int reg, u32 value)\
{						\
	return;					\
}						\
u32 pci_##name##_read_reg(int reg)		\
{						\
	return 0xffffffff;			\
}						\

/* core modules of cs5536 */

/* ide module */
DECLARE_CS5536_MODULE(ide)
/* acc module */
DECLARE_CS5536_MODULE(acc)
/* ohci module */
DECLARE_CS5536_MODULE(ohci)
/* isa module */
DECLARE_CS5536_MODULE(isa)
/* ehci module */
DECLARE_CS5536_MODULE(ehci)

/* selective modules of cs5536 */
/* flash(nor or nand flash) module */
#ifdef CONFIG_CS5536_FLASH
    DECLARE_CS5536_MODULE(flash)
#else
    DEFINE_CS5536_MODULE(flash)
#endif
/* otg module */
#ifdef CONFIG_CS5536_OTG
    DECLARE_CS5536_MODULE(otg)
#else
    DEFINE_CS5536_MODULE(otg)
#endif
/* udc module */
#ifdef CONFIG_CS5536_UDC
    DECLARE_CS5536_MODULE(udc)
#else
    DEFINE_CS5536_MODULE(udc)
#endif
#endif				/* _CS5536_VSM_H */
