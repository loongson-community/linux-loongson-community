#
# Makefile for the Cobalt micro systems family specific parts of the kernel
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

O_TARGET := cobalt.o

obj-y	 := irq.o int-handler.o pci.o reset.o setup.o via.o promcon.o

include $(TOPDIR)/Rules.make
