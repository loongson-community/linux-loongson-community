#
# Makefile for Momentum Computer's Ocelot-G board.
#

obj-y	 += gt-irq.o pci-irq.o pci.o int-handler.o irq.o prom.o reset.o setup.o

obj-$(CONFIG_REMOTE_DEBUG) += dbg_io.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
