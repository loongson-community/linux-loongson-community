#
#  Copyright 2000,2001 MontaVista Software Inc.
#  Author: MontaVista Software, Inc.
#     	ppopov@mvista.com or source@mvista.com
#
# Makefile for the Alchemy Semiconductor Pb1100 board.
#

O_TARGET := pb1100.o

obj-y := init.o setup.o

obj-$(CONFIG_PCI) += pci_fixup.o pci_ops.o

include $(TOPDIR)/Rules.make
