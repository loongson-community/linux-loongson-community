# $Id: Makefile,v 1.7 1999/03/09 21:33:21 tsbogend Exp $
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

all: vmlinux.ecoff addinitrd

vmlinux.ecoff:	$(CONFIGURE) elf2ecoff $(TOPDIR)/vmlinux
	./elf2ecoff $(TOPDIR)/vmlinux vmlinux.ecoff

elf2ecoff: elf2ecoff.c
	$(HOSTCC) -o $@ $^

addinitrd: addinitrd.c
	$(HOSTCC) -o $@ $^

# Don't build dependencies, this may die if $(CC) isn't gcc
dep:

clean:
	rm -f vmlinux.ecoff

dummy:

include $(TOPDIR)/Rules.make
