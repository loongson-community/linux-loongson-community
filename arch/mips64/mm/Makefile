# $Id: Makefile,v 1.2 1999/08/20 21:13:33 ralf Exp $
#
# Makefile for the Linux/MIPS-specific parts of the memory manager.
#

O_TARGET := mm.o
O_OBJS	 := extable.o init.o fault.o r4xx0.o tfp.o andes.o loadmmu.o

ifdef CONFIG_SGI_IP22
O_OBJS   += umap.o
endif

ifdef CONFIG_BAGET_MIPS
O_OBJS   += umap.o
endif

include $(TOPDIR)/Rules.make
