#
# Makefile for TOSHIBA JMR-TX3927 board
#

obj-y	 += init.o int-handler.o irq.o setup.o rtc.o pci_fixup.o pci_ops.o

obj-$(CONFIG_LL_DEBUG) 		+= debug.o
obj-$(CONFIG_REMOTE_DEBUG)	+= kgdb_io.o

EXTRA_AFLAGS := $(CFLAGS)
