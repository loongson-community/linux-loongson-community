#
# Makefile for MIPS-specific library files..
#
# $Id: Makefile,v 1.9 1998/05/03 00:28:00 ralf Exp $
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

L_TARGET = lib.a
L_OBJS	= csum_partial.o csum_partial_copy.o dump_tlb.o ide-std.c ide-no.o \
	  memset.o memcpy.o strlen_user.o strncpy_user.o tags.o watch.o

include $(TOPDIR)/Rules.make
