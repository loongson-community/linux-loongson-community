#
# Makefile for PMC Sierra's Ocelot CP7000 board.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

O_TARGET := cp7000.o
O_OBJS	 := i2o.o int-handler.o irq.o irq-handler.o pci.o promcon.o reset.o \
	    serialGT.o setup.o

include $(TOPDIR)/Rules.make
