#
# Makefile for the IBM WorkPad z50 specific parts of the kernel
#

O_TARGET := workpad.o

obj-y	:= init.o setup.o

obj-$(CONFIG_IDE)	+= ide-workpad.o

include $(TOPDIR)/Rules.make
