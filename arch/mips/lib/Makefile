#
# Makefile for MIPS-specific library files..
#
# Some of these routines are just left over debugging trash of ancient
# times when I just could make my Tyne beep and so ...
#
# ...and for when I need to get the DECStation to use the boot prom to
# do things... Paul M. Antoine.
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

L_TARGET = lib.a
L_OBJS	= beep.o bitags.o checksum.o csum.o dump_tlb.o io.o memmove.o \
	  strncpy_user.o strlen_user.o watch.o
ifdef CONFIG_MIPS_DECSTATION
L_OBJS += pmaxcon.o pmaxio.o
else
L_OBJS += tinycon.o
endif

include $(TOPDIR)/Rules.make
