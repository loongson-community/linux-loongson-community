#
# Makefile for the DECstation family specific parts of the kernel
#

export-objs := rtc-dec.o setup.o wbflush.o

obj-y	 := int-handler.o ioasic-irq.o kn02-irq.o reset.o rtc-dec.o setup.o \
	time.o

obj-$(CONFIG_PROM_CONSOLE)	+= promcon.o
obj-$(CONFIG_CPU_HAS_WB)	+= wbflush.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
