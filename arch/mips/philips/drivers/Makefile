#
# Makefile for the Philips Nino specific parts of the kernel
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.s:
	$(CPP) $(AFLAGS) $< -o $@
.S.o:
	$(CC) $(AFLAGS) -c $< -o $@

O_TARGET := drivers.o

all: drivers.o

obj-$(CONFIG_SERIAL)		+= uart-pr31700.o generic_serial.o

include $(TOPDIR)/Rules.make
