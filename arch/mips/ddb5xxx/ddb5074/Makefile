#
# Makefile for the NEC DDB Vrc-5074 specific kernel interface routines
# under Linux.
#

O_TARGET = ddb5074.o

obj-y				+= setup.o irq.o int-handler.o pci.o pci_ops.o nile4_pic.o time.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
