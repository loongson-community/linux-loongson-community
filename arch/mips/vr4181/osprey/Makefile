#
# Makefile for common code of NEC Osprey board
#

O_TARGET := osprey.o

obj-y	 := setup.o prom.o reset.o

obj-$(CONFIG_REMOTE_DEBUG)	+= dbg_io.o

include $(TOPDIR)/Rules.make
