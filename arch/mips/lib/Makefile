#
# Makefile for MIPS-specific library files..
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

OBJS  = dump_tlb.o tinycon.o watch.o

include ../../../.config

lib.a: $(OBJS)
	$(AR) rcs lib.a $(OBJS)
	sync

dep:
	$(CPP) -M *.[cS] > .depend

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif

