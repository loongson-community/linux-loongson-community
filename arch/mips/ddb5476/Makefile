#
# Makefile for the NEC DDB Vrc-5074 specific kernel interface routines
# under Linux.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

O_TARGET = ddb5476.a
O_OBJS  = setup.o irq.o time.o prom.o pci.o pci-dma.o int-handler.o nile4.o

ifdef CONFIG_REMOTE_DEBUG
O_OBJS += dbg_io.o
endif

include $(TOPDIR)/Rules.make
