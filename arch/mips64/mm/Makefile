#
# Makefile for the Linux/MIPS-specific parts of the memory manager.
#

USE_STANDARD_AS_RULE := true

O_TARGET := mm.o

export-objs			+= umap.o
obj-y				:= extable.o init.o fault.o loadmmu.o

obj-$(CONFIG_CPU_R4300)		+= r4xx0.o tlbex-r4k.o tlb-glue-r4k.o
obj-$(CONFIG_CPU_R4X00)		+= r4xx0.o tlbex-r4k.o tlb-glue-r4k.o
obj-$(CONFIG_CPU_R5000)		+= r4xx0.o tlbex-r4k.o tlb-glue-r4k.o
obj-$(CONFIG_CPU_NEVADA)	+= r4xx0.o tlbex-r4k.o tlb-glue-r4k.o
obj-$(CONFIG_CPU_R10000)	+= andes.o tlbex-r4k.o tlb-glue-r4k.o
obj-$(CONFIG_CPU_SB1)		+= pg-sb1.o c-sb1.o tlb-sb1.o tlbex-r4k.o \
				   tlb-glue-r4k.o

#
# Debug TLB exception handler, currently unused
#
#obj-y				+=  tlb-dbg-r4k.o tlb-glue-r4k.o

obj-$(CONFIG_SGI_IP22)		+= umap.o

CFLAGS_tlb-glue-r4k.o := -P

include $(TOPDIR)/Rules.make
