#
# Makefile for NEC DDB-Vrc5477 board
#

obj-y	 += int-handler.o irq.o irq_5477.o setup.o pci.o pci_ops.o lcd44780.o

obj-$(CONFIG_DEBUG) 		+= debug.o
obj-$(CONFIG_REMOTE_DEBUG)	+= kgdb_io.o
obj-$(CONFIG_BLK_DEV_INITRD)	+= ramdisk.o

EXTRA_AFLAGS := $(CFLAGS)
