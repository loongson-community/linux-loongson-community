#
# Makefile for Momentum Computer's Ocelot-C and -CS boards.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

O_TARGET:= ocelot_c.o

obj-y	 += mv-irq.o cpci-irq.o uart-irq.o int-handler.o irq.o
obj-y    += pci-irq.o pci.o prom.o reset.o setup.o 

obj-$(CONFIG_REMOTE_DEBUG) += dbg_io.o

include $(TOPDIR)/Rules.make
