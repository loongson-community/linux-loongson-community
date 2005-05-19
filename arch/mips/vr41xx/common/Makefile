#
# Makefile for common code of the NEC VR4100 series.
#

obj-y				+= bcu.o cmu.o icu.o init.o int-handler.o pmu.o
obj-$(CONFIG_GPIO_VR41XX)	+= giu.o
obj-$(CONFIG_VRC4171)		+= vrc4171.o
obj-$(CONFIG_VRC4173)		+= vrc4173.o

EXTRA_AFLAGS := $(CFLAGS)
