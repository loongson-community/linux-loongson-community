#
# Makefile for the HP specific kernel interface routines
# under Linux.
#

O_TARGET := hp-lj.o

export-objs := utils.o

obj-y   := init.o setup.o irq.o int-handler.o pci.o utils.o asic.o

obj-$(CONFIG_REMOTE_DEBUG) += gdb_hook.o
obj-$(CONFIG_DIRECT_PRINTK) += gdb_hook.o

obj-$(CONFIG_BLK_DEV_INITRD) += initrd.o

forceit:

# package filesystem from rootfs directory into binary package
romfs.bin: forceit ./rootfs
	@genromfs -d ./rootfs -f $@

# transform rootfs.bin into object file format for linking
initrd.o: romfs.bin
	@echo "" | $(CROSS_COMPILE)as -o $@
	@$(CROSS_COMPILE)objcopy --add-section .initrd=$< $@

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make

.PHONY: forceit
