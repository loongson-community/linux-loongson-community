#
# Makefile for the LASAT specific kernel interface routines under Linux.
#

obj-y	 := reset.o setup.o prom.o lasat_board.o \
		at93c.o interrupt.o lasatIRQ.o

obj-$(CONFIG_LASAT_SYSCTL) += sysctl.o

obj-$(CONFIG_DS1603) += ds1603.o

obj-$(CONFIG_PICVUE) += picvue.o
obj-$(CONFIG_PICVUE_PROC) += picvue_proc.o

obj-$(CONFIG_PCI) += pci.o

clean:
	make -C image clean
