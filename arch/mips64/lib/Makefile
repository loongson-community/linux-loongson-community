#
# Makefile for MIPS-specific library files..
#

L_TARGET = lib.a

obj-y	+= csum_partial.o csum_partial_copy.o dump_tlb.o promlib.o rtc-std.o \
	  rtc-no.o memset.o memcpy.o strlen_user.o strncpy_user.o \
	  strnlen_user.o watch.o

obj-$(CONFIG_BLK_DEV_FD)	+= floppy-no.o floppy-std.o
obj-$(subst m,y,$(CONFIG_IDE))	+= ide-std.o ide-no.o   # needed for ide module

EXTRA_AFLAGS := $(CFLAGS)
