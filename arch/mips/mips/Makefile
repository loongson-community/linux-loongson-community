#
# PROM Entries for the big endian firmware used in the Mips Computer
# System, Inc. machines.  Beware: some of the machines seem to be
# different.
#
# Copyright (C) 1997 Ralf Baechle
#
prom-entries= reset exec restart reinit reboot autoboot open read write ioctl \
	close getchar putchar showchar gets puts printf initproto protoenable \
	protodisable getpkt putpkt orw_rmw orh_rmw orb_rmw andw_rmw andh_rmw \
	andb_rmw flushcache clearcache setjmp longjmp bevutlb getenv setenv \
	atob strcmp strlen strcpy strcat parser range argvize help dumpcmd \
	setenvcmd unsetenvcmd printenvcmd bevexcept enablecmd disablecmd \
	clearnofault notimplement nv_get nv_set
asm-files=$(addsuffix .S,$(prom-entries))
object-files=$(addsuffix .o,$(prom-entries))

CC=mipsel-linux-gcc
AR=mipsel-linux-ar
CFLAGS=-O2 -mno-abicalls -fno-pic -G0 -Wall

all:	libprom.a

libprom.a: $(asm-files)
	set -e;for i in $(prom-entries); do \
		$(CC) $(CFLAGS) -c -o $$i.o $$i.S; \
		$(AR) rcv libprom.a $$i.o; \
	done

$(asm-files): mkprom
	set -e;for i in $(prom-entries); do \
		mkprom $$i; \
	done

clean:
	rm -f $(object-files)

distclean:
	rm -rf $(asm-files) libprom.a
