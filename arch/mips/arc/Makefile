#
# Makefile for the ARC prom monitor library routines under Linux.
#

L_TARGET = lib.a

obj-y				+= cmdline.o env.o file.o identify.o init.o \
				   misc.o time.o tree.o

obj-$(CONFIG_ARC_MEMORY)	+= memory.o
obj-$(CONFIG_ARC_CONSOLE)	+= arc_con.o
obj-$(CONFIG_ARC_PROMLIB)	+= promlib.o
