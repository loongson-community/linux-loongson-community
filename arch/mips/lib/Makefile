#
# Makefile for MIPS-specific library files..
#

lib-y	+= csum_partial_copy.o promlib.o rtc-std.o

lib-$(CONFIG_BLK_DEV_FD)	+= floppy-no.o
lib-$(subst m,y,$(CONFIG_IDE))	+= ide-std.o ide-no.o   # needed for ide module

EXTRA_AFLAGS := $(CFLAGS)
