#
#  Copyright 2000 MontaVista Software Inc.
#  Author: MontaVista Software, Inc.
#     	ppopov@mvista.com or source@mvista.com
#
# Makefile for the Galileo EV96100 board.
#

O_TARGET:= ev96100.o

obj-y	:= init.o pci_ops.o pci_fixups.o time.o irq.o int-handler.o setup.o \
	   puts.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
