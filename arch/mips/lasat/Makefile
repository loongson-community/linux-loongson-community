#
# Makefile for the LASAT specific kernel interface routines under Linux.
#

obj-y	 := reset.o setup.o prom.o lasat_board.o \
		lasat_eeprom.o interrupt.o lasatIRQ.o

obj-$(CONFIG_LASAT_SYSCTL) += sysctl.o

obj-$(CONFIG_DS1603) += ds1603.o

obj-$(CONFIG_PICVUE)            += picvue.o
obj-$(CONFIG_PICVUE_PROC)       += picvue_proc.o

ifeq ($(CONFIG_PCI),y)
  obj-y += pci.o pci100.o pci200.o
endif

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make

clean:
	make -C image clean
