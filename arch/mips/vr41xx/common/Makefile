#
# Makefile for common code of the NEC VR4100 series.
#

obj-y		:= bcu.o cmu.o giu.o icu.o int-handler.o reset.o

obj-$(CONFIG_PCI)		+= pciu.o
obj-$(CONFIG_SERIAL)		+= serial.o
obj-$(CONFIG_VR41XX_TIME_C)	+= time.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
