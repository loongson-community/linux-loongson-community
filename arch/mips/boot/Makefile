#
# arch/mips/boot/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995 by Ralf Baechle
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

OBJS  = milo.o a.out.o

#
# Fake compressed boot
#
zImage:	$(CONFIGURE) $(TOPDIR)/vmlinux
	cp $(TOPDIR)/vmlinux $@
	$(STRIP) --discard-all $@

zdisk:	zImage
	mcopy -n zImage a:vmlinux

dep:
	$(CPP) -M *.[cS] > .depend

clean:
	rm -f zImage

dummy:

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
