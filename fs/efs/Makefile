#
# Makefile for the linux efs-filesystem routines.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

CFLAGS+=-g -DDEBUG_EFS

O_TARGET := efs.o
O_OBJS   := dir.o file.o inode.o namei.o \
		symlink.o
M_OBJS   := $(O_TARGET)

include $(TOPDIR)/Rules.make
