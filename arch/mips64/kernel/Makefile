#
# Makefile for the Linux/MIPS kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

all:	kernel.o head.o init_task.o

O_TARGET := kernel.o
O_OBJS	:= entry.o proc.o r4k_fpu.o setup.o signal.o traps.o
OX_OBJS := mips64_ksyms.o

clean:

include $(TOPDIR)/Rules.make
