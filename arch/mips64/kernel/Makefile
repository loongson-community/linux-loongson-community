#
# Makefile for the Linux/MIPS kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

all:	kernel.o

O_TARGET := kernel.o
O_OBJS	:=
OX_OBJS :=

clean:

include $(TOPDIR)/Rules.make
