#
# Makefile for the ZAO Networks Capcella  specific parts of the kernel
#

obj-y	:= init.o setup.o

obj-$(CONFIG_IDE)	+= ide-capcella.o
obj-$(CONFIG_PCI)	+= pci_fixup.o

include $(TOPDIR)/Rules.make
