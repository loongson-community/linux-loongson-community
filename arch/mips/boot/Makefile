# $Id: Makefile,v 1.5 1999/01/03 17:50:47 ralf Exp $
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995, 1998 by Ralf Baechle
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

OBJS  = milo.o a.out.o

#
# Drop some uninteresting sections in the kernel.
# This is only relevant for ELF kernels but doesn't hurt a.out
#
drop-sections	= .reginfo .mdebug
strip-flags	= $(addprefix --remove-section=,$(drop-sections))

#
# Fake compressed boot
#
zImage:	$(CONFIGURE) elf2ecoff $(TOPDIR)/vmlinux
	./elf2ecoff $(TOPDIR)/vmlinux zImage

elf2ecoff: elf2ecoff.c
	$(HOSTCC) -o $@ $^

zdisk:	zImage
#	if [ -f /etc/remote-mcopy ]; then \
#		ssh rio mcopy -o - a:vmlinux <zImage; \
#	else \
#		mcopy -o zImage a:vmlinux; \
#	fi
	cp zImage /boot/vmlinux

# Don't build dependencies, this may die if $(CC) isn't gcc
dep:

clean:
	rm -f zImage zImage.tmp mkboot

dummy:

include $(TOPDIR)/Rules.make
