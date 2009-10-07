/*
 * read/write operation to the PCI config space of CS5536
 *
 * Copyright (C) 2007 Lemote, Inc. & Institute of Computing Technology
 * Author : jlliu, liujl@lemote.com
 *
 * Copyright (C) 2009 Lemote, Inc. & Institute of Computing Technology
 * Author: Wu Zhangjin, wuzj@lemote.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 *	the Virtual Support Module(VSM) for virtulizing the PCI
 *	configure space are defined in cs5536_modulename.c respectively,
 *	so you can select modules which have been used in his/her board.
 *	to archive this, you just need to add a "select CS5536_[MODULE]"
 *	option in your board in arch/mips/loongson/Kconfig
 *
 *	after this virtulizing, user can access the PCI configure space
 *	directly as a normal multi-function PCI device which following
 *	the PCI-2.2 spec.
 */

#include <linux/types.h>
#include <cs5536/cs5536_vsm.h>

enum {
	CS5536_FUNC_START = -1,
	CS5536_ISA_FUNC,
	CS5536_FLASH_FUNC,
	CS5536_IDE_FUNC,
	CS5536_ACC_FUNC,
	CS5536_OHCI_FUNC,
	CS5536_EHCI_FUNC,
	CS5536_UDC_FUNC,
	CS5536_OTG_FUNC,
	CS5536_FUNC_END,
};

/*
 * write to PCI config space and transfer it to MSR write.
 */
void cs5536_pci_conf_write4(int function, int reg, u32 value)
{
	if ((function <= CS5536_FUNC_START) || (function >= CS5536_FUNC_END))
		return;
	if ((reg < 0) || (reg > 0x100) || ((reg & 0x03) != 0))
		return;

	switch (function) {
	case CS5536_ISA_FUNC:
		pci_isa_write_reg(reg, value);
		break;
	case CS5536_FLASH_FUNC:
		pci_flash_write_reg(reg, value);
		break;
	case CS5536_IDE_FUNC:
		pci_ide_write_reg(reg, value);
		break;
	case CS5536_ACC_FUNC:
		pci_acc_write_reg(reg, value);
		break;
	case CS5536_OHCI_FUNC:
		pci_ohci_write_reg(reg, value);
		break;
	case CS5536_EHCI_FUNC:
		pci_ehci_write_reg(reg, value);
		break;
	case CS5536_UDC_FUNC:
		pci_udc_write_reg(reg, value);
		break;
	case CS5536_OTG_FUNC:
		pci_otg_write_reg(reg, value);
		break;
	default:
		break;
	}
	return;
}

/*
 * read PCI config space and transfer it to MSR access.
 */
u32 cs5536_pci_conf_read4(int function, int reg)
{
	u32 data = 0;

	if ((function <= CS5536_FUNC_START) || (function >= CS5536_FUNC_END))
		return 0;
	if ((reg < 0) || ((reg & 0x03) != 0))
		return 0;
	if (reg > 0x100)
		return 0xffffffff;

	switch (function) {
	case CS5536_ISA_FUNC:
		data = pci_isa_read_reg(reg);
		break;
	case CS5536_FLASH_FUNC:
		data = pci_flash_read_reg(reg);
		break;
	case CS5536_IDE_FUNC:
		data = pci_ide_read_reg(reg);
		break;
	case CS5536_ACC_FUNC:
		data = pci_acc_read_reg(reg);
		break;
	case CS5536_OHCI_FUNC:
		data = pci_ohci_read_reg(reg);
		break;
	case CS5536_EHCI_FUNC:
		data = pci_ehci_read_reg(reg);
		break;
	case CS5536_UDC_FUNC:
		data = pci_udc_read_reg(reg);
		break;
	case CS5536_OTG_FUNC:
		data = pci_otg_read_reg(reg);
		break;
	default:
		break;
	}
	return data;
}
