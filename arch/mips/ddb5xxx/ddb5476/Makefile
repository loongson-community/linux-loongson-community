#
# Makefile for the NEC DDB Vrc-5476 specific kernel interface routines
# under Linux.
#

O_TARGET = ddb5476.o

obj-y				+= setup.o irq.o int-handler.o pci.o pci_ops.o \
				   nile4_pic.o vrc5476_irq.o
obj-$(CONFIG_REMOTE_DEBUG)	+= dbg_io.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
