#
# Makefile for TOSHIBA JMR-TX3927 board
#

obj-y	 += init.o int-handler.o irq.o setup.o rtc.o pci_fixup.o pci_ops.o

obj-$(CONFIG_RUNTIME_DEBUG) 	+= debug.o
obj-$(CONFIG_KGDB)		+= kgdb_io.o

EXTRA_AFLAGS := $(CFLAGS)
