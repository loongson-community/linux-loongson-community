#
# Makefile for the MIPS II specific parts of the Linux/MIPS kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

all:	mips.o
EXTRA_ASFLAGS = -mips2 -mcpu=r6000
O_TARGET := mips.o
O_OBJS	:= cpu.o pagetables.o showregs.o

clean:

include $(TOPDIR)/Rules.make
