#
# Makefile for the MIPS III specific parts of the Linux/MIPS kernel.
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
EXTRA_ASFLAGS = -mips3 -mcpu=r4400
O_TARGET := mips.o
O_OBJS	:= cache.o cpu.o fp-context.o memcpy.o memset.o r4xx0.o pagetables.o \
	   showregs.o

copy_user.o: copy_user.S
r4xx0.o: r4xx0.S

clean:

include $(TOPDIR)/Rules.make
