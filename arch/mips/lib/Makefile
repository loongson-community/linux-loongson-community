# $Id: Makefile,v 1.5 1998/05/04 09:12:51 ralf Exp $
#
# Makefile for MIPS-specific library files..
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

L_TARGET = lib.a
L_OBJS	= csum_partial.o csum_partial_copy.o dump_tlb.o ide-std.o ide-no.o \
	  memset.o memcpy.o strlen_user.o strncpy_user.o tags.o watch.o

include $(TOPDIR)/Rules.make
