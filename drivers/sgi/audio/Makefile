#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

#
# sbus audio drivers
#

O_TARGET := sgiaudio.o
O_OBJS   :=
M_OBJS   :=
M :=
MM :=





ifeq ($(CONFIG_SGI_HAL2),y)
M=y
OX_OBJS += hal2.o
else
  ifeq ($(CONFIG_SGI_HAL2),m)
  MM=y
  MX_OBJS += hal2.o
  endif
endif

include $(TOPDIR)/Rules.make
