#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

SUB_DIRS     :=
MOD_SUB_DIRS := $(SUB_DIRS) char
ifdef CONFIG_SGI_IO
MOD_SUB_DIRS += io
endif
ALL_SUB_DIRS := $(SUB_DIRS) char
ifdef CONFIG_SGI_IO
ALL_SUB_DIRS += io
endif


L_OBJS   := 
L_TARGET := sgi.a
MOD_LIST_NAME := SGI_MODULES

# Character and Audio devices for SGI machines.
#
SUB_DIRS += char
ifdef CONFIG_SGI_IO
SUB_DIRS += io
endif
L_OBJS += char/sgichar.o
ifdef CONFIG_SGI_IO
L_OBJS += io/sgiio.o
endif

include $(TOPDIR)/Rules.make
