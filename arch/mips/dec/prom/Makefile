#
# Makefile for the DECstation prom monitor library routines
# under Linux.
#

L_TARGET  = rexlib.a

obj-y			+= init.o memory.o cmdline.o identify.o

obj-$(CONFIG_MIPS32)	+= locore.o
obj-$(CONFIG_MIPS64)	+= call_o32.o

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
