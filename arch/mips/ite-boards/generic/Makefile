#
#  Copyright 2000 MontaVista Software Inc.
#  Author: MontaVista Software, Inc.
#     	ppopov@mvista.com or source@mvista.com
#
# Makefile for the ITE 8172 (qed-4n-s01b) board, generic files.
#

O_TARGET := it8172.o

obj-y := it8172_rtc.o it8172_setup.o irq.o int-handler.o pmon_prom.o time.o lpc.o puts.o reset.o

obj-$(CONFIG_PCI) += it8172_pci.o
obj-$(CONFIG_IT8172_CIR) += it8172_cir.o
obj-$(CONFIG_REMOTE_DEBUG) += dbg_io.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
