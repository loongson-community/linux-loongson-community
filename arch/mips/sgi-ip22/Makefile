#
# Makefile for the SGI specific kernel interface routines
# under Linux.
#

export-objs	:= ip22-setup.o
obj-y		+= ip22-mc.o ip22-hpc.o ip22-int.o ip22-irq.o ip22-time.o \
		   ip22-berr.o ip22-gio.o ip22-rtc.o ip22-reset.o \
		   ip22-system.o ip22-setup.o

obj-$(CONFIG_BOARD_SCACHE)	+= ip22-sc.o
obj-$(CONFIG_IP22_EISA)		+= ip22-eisa.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
