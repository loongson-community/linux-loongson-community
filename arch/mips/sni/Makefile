#
# Makefile for the SNI specific part of the kernel
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

include ../../../.config

all: sni.o
O_TARGET := sni.o
O_OBJS	 := hw-access.o int-handler.o reset.o setup.o

ifdef CONFIG_PCI
O_OBJS += bios32.o
endif

int-handler.o: int-handler.S

clean:

include $(TOPDIR)/Rules.make
