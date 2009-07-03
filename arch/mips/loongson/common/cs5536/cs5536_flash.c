/*
 * the FLASH Virtual Support Module of AMD CS5536
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
 */

#include <cs5536/cs5536.h>
#include <cs5536/cs5536_pci.h>

/*
 * enable the region of flashs(NOR or NAND)
 *
 * the same as the DIVIL other modules above, two groups of regs should be
 * modified here to control the region. DIVIL flash LBAR and the
 * RCONFx(6~9 reserved).
 */
static void flash_lbar_enable(void)
{
	u32 hi, lo;
	int offset;

	for (offset = DIVIL_LBAR_FLSH0; offset <= DIVIL_LBAR_FLSH3; offset++) {
		_rdmsr(DIVIL_MSR_REG(offset), &hi, &lo);
		hi |= 0x1;
		_wrmsr(DIVIL_MSR_REG(offset), hi, lo);
	}

	for (offset = SB_R6; offset <= SB_R9; offset++) {
		_rdmsr(SB_MSR_REG(offset), &hi, &lo);
		lo |= 0x1;
		_wrmsr(SB_MSR_REG(offset), hi, lo);
	}

	return;
}

/*
 * disable the region of flashs(NOR or NAND)
 */
static void flash_lbar_disable(void)
{
	u32 hi, lo;
	int offset;

	for (offset = DIVIL_LBAR_FLSH0; offset <= DIVIL_LBAR_FLSH3; offset++) {
		_rdmsr(DIVIL_MSR_REG(offset), &hi, &lo);
		hi &= ~0x01;
		_wrmsr(DIVIL_MSR_REG(offset), hi, lo);
	}
	for (offset = SB_R6; offset <= SB_R9; offset++) {
		_rdmsr(SB_MSR_REG(offset), &hi, &lo);
		lo &= ~0x01;
		_wrmsr(SB_MSR_REG(offset), hi, lo);
	}

	return;
}

#ifndef	CONFIG_CS5536_NOR_FLASH	/* for nand flash */

void pci_flash_write_bar(int n, u32 value)
{
	u32 hi = 0, lo = value;

	if (value == PCI_BAR_RANGE_MASK) {
		/* make the flag for reading the bar length. */
		_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
		lo |= (SOFT_BAR_FLSH0_FLAG << n);
		_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
	} else if ((value & 0x01) == 0x00) {
		/* mem space nand flash native reg base addr */
		hi = 0xfffff007;
		lo &= CS5536_FLSH_RANGE;
		_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_FLSH0 + n), hi, lo);

		/* RCONFx is 4KB in units for mem space. */
		hi = ((value & 0xfffff000) << 12) |
		    ((CS5536_FLSH_LENGTH & 0xfffff000) - (1 << 12)) | 0x00;
		lo = ((value & 0xfffff000) << 12) | 0x01;
		_wrmsr(SB_MSR_REG(SB_R6 + n), hi, lo);
	}
	return;
}

void pci_flash_write_reg(int reg, u32 value)
{
	u32 hi = 0, lo = value;

	switch (reg) {
	case PCI_COMMAND:
		if (value & PCI_COMMAND_MEMORY)
			flash_lbar_enable();
		else
			flash_lbar_disable();
		break;
	case PCI_STATUS:
		if (value & PCI_STATUS_PARITY) {
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if (lo & SB_PARE_ERR_FLAG) {
				lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
				_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
			}
		}
		break;
	case PCI_BAR0_REG:
		pci_flash_write_bar(0, value);
		break;
	case PCI_BAR1_REG:
		pci_flash_write_bar(1, value);
		break;
	case PCI_BAR2_REG:
		pci_flash_write_bar(2, value);
		break;
	case PCI_BAR3_REG:
		pci_flash_write_bar(3, value);
		break;
	case PCI_FLASH_INT_REG:
		_rdmsr(DIVIL_MSR_REG(PIC_YSEL_LOW), &hi, &lo);
		/* disable all the flash interrupt in PIC */
		lo &= ~(0xf << PIC_YSEL_LOW_FLASH_SHIFT);
		if (value)	/* enable all the flash interrupt in PIC */
			lo |= (CS5536_FLASH_INTR << PIC_YSEL_LOW_FLASH_SHIFT);
		_wrmsr(DIVIL_MSR_REG(PIC_YSEL_LOW), hi, lo);
		break;
	case PCI_NAND_FLASH_TDATA_REG:
		_wrmsr(DIVIL_MSR_REG(NANDF_DATA), hi, lo);
		break;
	case PCI_NAND_FLASH_TCTRL_REG:
		lo &= 0x00000fff;
		_wrmsr(DIVIL_MSR_REG(NANDF_CTRL), hi, lo);
		break;
	case PCI_NAND_FLASH_RSVD_REG:
		_wrmsr(DIVIL_MSR_REG(NANDF_RSVD), hi, lo);
		break;
	case PCI_FLASH_SELECT_REG:
		if (value == CS5536_IDE_FLASH_SIGNATURE) {
			_rdmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), &hi, &lo);
			lo &= ~0x01;
			_wrmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), hi, lo);
		}
		break;
	default:
		break;
	}

	return;
}

u32 pci_flash_read_bar(int n)
{
	u32 hi, lo;
	u32 conf_data = 0;

	_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
	if (lo & (SOFT_BAR_FLSH0_FLAG << n)) {
		conf_data = CS5536_FLSH_RANGE | PCI_BASE_ADDRESS_SPACE_MEMORY;
		lo &= ~(SOFT_BAR_FLSH0_FLAG << n);
		_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
	} else {
		_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_FLSH0 + n), &hi, &lo);
		conf_data = lo;
		conf_data &= ~0x0f;
	}

	return conf_data;
}

u32 pci_flash_read_reg(int reg)
{
	u32 conf_data = 0;
	u32 hi, lo;

	switch (reg) {
	case PCI_VENDOR_ID:
		conf_data =
		    CFG_PCI_VENDOR_ID(CS5536_FLASH_DEVICE_ID, CS5536_VENDOR_ID);
		break;
	case PCI_COMMAND:
		/* we just read one lbar for returning. */
		_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_FLSH0), &hi, &lo);
		if (hi & 0x1)
			conf_data |= PCI_COMMAND_MEMORY;
		break;
	case PCI_STATUS:
		conf_data |= PCI_STATUS_66MHZ;
		conf_data |= PCI_STATUS_FAST_BACK;
		_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
		if (lo & SB_PARE_ERR_FLAG)
			conf_data |= PCI_STATUS_PARITY;
		conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
		break;
	case PCI_CLASS_REVISION:
		_rdmsr(DIVIL_MSR_REG(DIVIL_CAP), &hi, &lo);
		conf_data = lo & 0x000000ff;
		conf_data |= (CS5536_FLASH_CLASS_CODE << 8);
		break;
	case PCI_CACHE_LINE_SIZE:
		conf_data =
		    CFG_PCI_CACHE_LINE_SIZE(PCI_NORMAL_HEADER_TYPE,
					    PCI_NORMAL_LATENCY_TIMER);
		break;
	case PCI_BAR0_REG:
		return pci_flash_read_bar(0);
		break;
	case PCI_BAR1_REG:
		return pci_flash_read_bar(1);
		break;
	case PCI_BAR2_REG:
		return pci_flash_read_bar(2);
		break;
	case PCI_BAR3_REG:
		return pci_flash_read_bar(3);
		break;
	case PCI_CARDBUS_CIS:
		conf_data = PCI_CARDBUS_CIS_POINTER;
		break;
	case PCI_SUBSYSTEM_VENDOR_ID:
		conf_data =
		    CFG_PCI_VENDOR_ID(CS5536_FLASH_SUB_ID,
				      CS5536_SUB_VENDOR_ID);
		break;
	case PCI_ROM_ADDRESS:
		conf_data = PCI_EXPANSION_ROM_BAR;
		break;
	case PCI_CAPABILITY_LIST:
		conf_data = PCI_CAPLIST_POINTER;
		break;
	case PCI_INTERRUPT_LINE:
		conf_data =
		    CFG_PCI_INTERRUPT_LINE(PCI_DEFAULT_PIN, CS5536_FLASH_INTR);
		break;
	case PCI_NAND_FLASH_TDATA_REG:
		_rdmsr(DIVIL_MSR_REG(NANDF_DATA), &hi, &lo);
		conf_data = lo;
		break;
	case PCI_NAND_FLASH_TCTRL_REG:
		_rdmsr(DIVIL_MSR_REG(NANDF_CTRL), &hi, &lo);
		conf_data = lo & 0x00000fff;
		break;
	case PCI_NAND_FLASH_RSVD_REG:
		_rdmsr(DIVIL_MSR_REG(NANDF_RSVD), &hi, &lo);
		conf_data = lo;
		break;
	case PCI_FLASH_SELECT_REG:
		_rdmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), &hi, &lo);
		conf_data = lo & 0x01;
		break;
	default:
		break;
	}
	return 0;
}

#else				/* CONFIG_CS5536_NOR_FLASH */

void pci_flash_write_bar(int n, u32 value)
{
	u32 hi = 0, lo = value;

	if (value == PCI_BAR_RANGE_MASK) {
		_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
		lo |= (SOFT_BAR_FLSH0_FLAG << n);
		_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
	} else if (value & 0x01) {
		/* IO space of 16bytes nor flash */
		hi = 0x0000fff1;
		lo &= CS5536_FLSH_RANGE;
		_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_FLSH0 + n), hi, lo);

		/* RCONFx used for 16bytes reserved. */
		hi = ((value & 0x000ffffc) << 12) | ((CS5536_FLSH_LENGTH - 4)
						     << 12) | 0x01;
		lo = ((value & 0x000ffffc) << 12) | 0x01;
		_wrmsr(SB_MSR_REG(SB_R6 + n), hi, lo);
	}
	return;
}

void pci_flash_write_reg(int reg, u32 value)
{
	u32 hi = 0, lo = value;

	switch (reg) {
	case PCI_COMMAND:
		if (value & PCI_COMMAND_IO)
			flash_lbar_enable();
		else
			flash_lbar_disable();
		break;
	case PCI_STATUS:
		if (value & PCI_STATUS_PARITY) {
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if (lo & SB_PARE_ERR_FLAG) {
				lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
				_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
			}
		}
		break;
	case PCI_BAR0_REG:
		pci_flash_write_bar(0, value);
		break;
	case PCI_BAR1_REG:
		pci_flash_write_bar(1, value);
		break;
	case PCI_BAR2_REG:
		pci_flash_write_bar(2, value);
		break;
	case PCI_BAR3_REG:
		pci_flash_write_bar(3, value);
		break;
	case PCI_FLASH_INT_REG:
		_rdmsr(DIVIL_MSR_REG(PIC_YSEL_LOW), &hi, &lo);
		/* disable all the flash interrupt in PIC */
		lo &= ~(0xf << PIC_YSEL_LOW_FLASH_SHIFT);
		if (value)	/* enable all the flash interrupt in PIC */
			lo |= (CS5536_FLASH_INTR << PIC_YSEL_LOW_FLASH_SHIFT);
		_wrmsr(DIVIL_MSR_REG(PIC_YSEL_LOW), hi, lo);
		break;
	case PCI_NOR_FLASH_CTRL_REG:
		lo &= 0x000000ff;
		_wrmsr(DIVIL_MSR_REG(NORF_CTRL), hi, lo);
		break;
	case PCI_NOR_FLASH_T01_REG:
		_wrmsr(DIVIL_MSR_REG(NORF_T01), hi, lo);
		break;
	case PCI_NOR_FLASH_T23_REG:
		_wrmsr(DIVIL_MSR_REG(NORF_T23), hi, lo);
		break;
	case PCI_FLASH_SELECT_REG:
		if (value == CS5536_IDE_FLASH_SIGNATURE) {
			_rdmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), &hi, &lo);
			lo &= ~0x01;
			_wrmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), hi, lo);
		}
		break;
	default:
		break;
	}
	return;
}

u32 pci_flash_read_bar(int n)
{
	u32 hi, lo;
	u32 conf_data = 0;

	_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
	if (lo & (SOFT_BAR_FLSH0_FLAG << n)) {
		conf_data = CS5536_FLSH_RANGE | PCI_BASE_ADDRESS_SPACE_IO;
		lo &= ~(SOFT_BAR_FLSH0_FLAG << n);
		_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
	} else {
		_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_FLSH0 + n), &hi, &lo);
		conf_data = lo & 0x0000ffff;
		conf_data |= 0x01;
		conf_data &= ~0x02;
	}

	return conf_data;
}

u32 pci_flash_read_reg(int reg)
{
	u32 conf_data = 0;
	u32 hi, lo;

	switch (reg) {
	case PCI_VENDOR_ID:
		conf_data =
		    CFG_PCI_VENDOR_ID(CS5536_FLASH_DEVICE_ID, CS5536_VENDOR_ID);
		break;
	case PCI_COMMAND:
		/* we just check one flash bar for returning. */
		_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_FLSH0), &hi, &lo);
		if (hi & PCI_COMMAND_IO)
			conf_data |= PCI_COMMAND_IO;
		break;
	case PCI_STATUS:
		conf_data |= PCI_STATUS_66MHZ;
		conf_data |= PCI_STATUS_FAST_BACK;
		_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
		if (lo & SB_PARE_ERR_FLAG)
			conf_data |= PCI_STATUS_PARITY;
		conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
		break;
	case PCI_CLASS_REVISION:
		_rdmsr(DIVIL_MSR_REG(DIVIL_CAP), &hi, &lo);
		conf_data = lo & 0x000000ff;
		conf_data |= (CS5536_FLASH_CLASS_CODE << 8);
		break;
	case PCI_CACHE_LINE_SIZE:
		conf_data =
		    CFG_PCI_CACHE_LINE_SIZE(PCI_NORMAL_HEADER_TYPE,
					    PCI_NORMAL_LATENCY_TIMER);
		break;
	case PCI_BAR0_REG:
		return pci_flash_read_bar(0);
		break;
	case PCI_BAR1_REG:
		return pci_flash_read_bar(1);
		break;
	case PCI_BAR2_REG:
		return pci_flash_read_bar(2);
		break;
	case PCI_BAR3_REG:
		return pci_flash_read_bar(3);
		break;
	case PCI_CARDBUS_CIS:
		conf_data = PCI_CARDBUS_CIS_POINTER;
		break;
	case PCI_SUBSYSTEM_VENDOR_ID:
		conf_data =
		    CFG_PCI_VENDOR_ID(CS5536_FLASH_SUB_ID,
				      CS5536_SUB_VENDOR_ID);
		break;
	case PCI_ROM_ADDRESS:
		conf_data = PCI_EXPANSION_ROM_BAR;
		break;
	case PCI_CAPABILITY_LIST:
		conf_data = PCI_CAPLIST_POINTER;
		break;
	case PCI_INTERRUPT_LINE:
		conf_data =
		    CFG_PCI_INTERRUPT_LINE(PCI_DEFAULT_PIN, CS5536_FLASH_INTR);
		break;
	case PCI_NOR_FLASH_CTRL_REG:
		_rdmsr(DIVIL_MSR_REG(NORF_CTRL), &hi, &lo);
		conf_data = lo & 0x000000ff;
		break;
	case PCI_NOR_FLASH_T01_REG:
		_rdmsr(DIVIL_MSR_REG(NORF_T01), &hi, &lo);
		conf_data = lo;
		break;
	case PCI_NOR_FLASH_T23_REG:
		_rdmsr(DIVIL_MSR_REG(NORF_T23), &hi, &lo);
		conf_data = lo;
		break;
	default:
		break;
	}
	return conf_data;
}
#endif				/* CONFIG_CS5536_NOR_FLASH */
