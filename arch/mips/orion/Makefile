#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
#
# Produce a bootimage for the IPSX
#  Copyright (C) 2000 Cort Dougan <cort@fsmlabs.com>
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

O_TARGET = orion.o
O_OBJS = promcon.o char.o serial.8530.o orion.hw.init.o setup.o int-handler.o \
	 zs.o initrd.o #no_initrd.o
OX_OBJS = irq.o

all: orion.o

#
# I usually have this mounted through loopback so I can
# change it and let the makefile include it automatically
#  -- Cort <cort@fsmlabs.com>
#
#ramdisk.image.gz: ramdisk.image
#	gzip -vf9 < ramdisk.image > ramdisk.image.gz

initrd.c: piggyback ramdisk.image
	gzip -vf9 < ramdisk.image | ./piggyback initrd > initrd.c
	rm -f ramdisk.image.gz

piggyback: piggyback.c
	$(HOSTCC) $(HOSTCFLAGS) -o piggyback piggyback.c

orionboot: orion.ctl

patchapp: patchapp.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $^

orion.ctl: ../../../vmlinux patchapp
	$(OBJCOPY) -Obinary ../../../vmlinux orion.nosym
	./patchapp orion.nosym orion
	cp -f orion.bin orion.ctl

# Don't build dependencies, this may die if $(CC) isn't gcc
dep:

clean:
	rm -f patchapp orion.bin orion.nosym orion.ctl initrd.c piggyback

dummy:

include $(TOPDIR)/Rules.make
