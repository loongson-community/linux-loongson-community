#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

SUB_DIRS     :=
MOD_SUB_DIRS := 
ALL_SUB_DIRS := 

L_TARGET := tc.a
L_OBJS   := tc.o

ifdef CONFIG_ZS
L_OBJS   += zs.o
endif

include $(TOPDIR)/Rules.make
