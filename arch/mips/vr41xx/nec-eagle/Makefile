#
# Makefile for the NEC Eagle/Hawk specific parts of the kernel
#
# Author: Yoichi Yuasa
#         yyuasa@mvista.com or source@mvista.com
#
# Copyright 2001,2002 MontaVista Software Inc.
#

obj-y	:= init.o irq.o setup.o

obj-$(CONFIG_IDE)	+= ide-eagle.o
obj-$(CONFIG_PCI)	+= pci_fixup.o vrc4173.o

include $(TOPDIR)/Rules.make
