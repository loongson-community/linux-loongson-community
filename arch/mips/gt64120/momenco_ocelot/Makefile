#
# Makefile for Momentum's Ocelot board.
#

obj-y	 += int-handler.o irq.o pci.o prom.o reset.o setup.o

obj-$(CONFIG_REMOTE_DEBUG) += dbg_io.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
