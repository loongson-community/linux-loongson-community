#
# Makefile for the MIPS I specific parts of the Linux/MIPS kernel.
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
EXTRA_ASFLAGS = -mips1 -mcpu=r3000
O_TARGET := mips.o
O_OBJS	:= cache.o cpu.o memcpy.o memset.o r3000.o pagetables.o showregs.o
	   fp-context.o

r3000.o: r3000.S

fp-context.o: fp-context.S

cache.o: cache.S

clean:

include $(TOPDIR)/Rules.make
