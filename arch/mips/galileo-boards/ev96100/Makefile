#
#  Copyright 2000 MontaVista Software Inc.
#  Author: MontaVista Software, Inc.
#     	ppopov@mvista.com or support@mvista.com
#
# Makefile for the Galileo EV96100 board.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

all: ev96100.o

O_TARGET:= ev96100.o

O_OBJS:= prom.o time.o rtc.o irq.o int-handler.o setup.o puts.o uart.o \
	 memory.o

dep:
	$(CPP) -M *.c > .depend

include $(TOPDIR)/Rules.make
