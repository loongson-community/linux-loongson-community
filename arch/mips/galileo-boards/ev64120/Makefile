#
#  Copyright 2000 RidgeRun, Inc.
#  Author: RidgeRun, Inc.
#     	glonnon@ridgerun.com, skranz@ridgerun.com, stevej@ridgerun.com
#
# Makefile for the Galileo EV64120 board.
#

export-objs	= i2o.o

obj-y		:= serialGT.o int-handler.o promcon.o reset.o setup.o irq.o \
		   irq-handler.o i2o.o pci_bios.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
