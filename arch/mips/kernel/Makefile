#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.c.s:
	$(CC) $(CFLAGS) -S $<
.s.o:
	$(AS) $(ASFLAGS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c $<
.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

OBJS  = process.o signal.o entry.o traps.o irq.o ptrace.o vm86.o ioport.o \
	setup.o bios32.o tynedma.o

include ../../../.config

#
# Kernel debugging
#

ifdef CONFIG_REMOTE_DEBUG
OBJS += gdb-low.o gdb-stub.o 
endif

#
# Board specific code
#

ifdef CONFIG_MIPS_JAZZ
OBJS += jazzdma.o
endif

ifdef CONFIG_ACER_PICA_61
OBJS += pica.o
endif

ifdef CONFIG_DESKSTATION_TYNE
OBJS += tyne.o
endif

ifdef CONFIG_MIPS_MAGNUM_4000
OBJS += magnum4000.o
endif

#
# CPU model specific code
#
ifdef CONFIG_CPU_R4X00
OBJS += r4xx0.o
endif

ifdef CONFIG_CPU_R4600
OBJS += r4xx0.o
endif

all: kernel.o head.o

entry.o: entry.S

head.o: head.S

magnum4000.o: magnum4000.S

pica.o: pica.S

r4xx0.o: r4xx0.S

tyne.o: tyne.S

kernel.o: $(OBJS)
	$(LD) -r -o kernel.o $(OBJS)
	sync

dep:
	$(CPP) -M *.[cS] > .depend

modules:

dummy:

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
