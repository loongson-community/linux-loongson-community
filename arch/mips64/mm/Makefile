#
# Makefile for the Linux/MIPS-specific parts of the memory manager.
#

O_TARGET := mm.o

export-objs			+= umap.o
obj-y				:= extable.o init.o fault.o loadmmu.o \
				   pgtable.o tlb-glue-r4k.o tlbex-r4k.o

obj-$(CONFIG_CPU_R4300)		+= c-r4k.o pg-r4k.o tlb-r4k.o
obj-$(CONFIG_CPU_R4X00)		+= c-r4k.o pg-r4k.o tlb-r4k.o
obj-$(CONFIG_CPU_R5000)		+= c-r4k.o pg-r4k.o tlb-r4k.o r5k-sc.o
obj-$(CONFIG_CPU_NEVADA)	+= c-r4k.o pg-r4k.o tlb-r4k.o r5k-sc.o
obj-$(CONFIG_CPU_R10000)	+= c-andes.o pg-andes.o tlb-andes.o
obj-$(CONFIG_CPU_SB1)		+= c-sb1.o pg-sb1.o tlb-sb1.o
obj-$(CONFIG_CPU_MIPS64)	+= c-mips64.o pg-mips64.o tlb-r4k.o

#
# Debug TLB exception handler, currently unused
#
#obj-y				+=  tlb-dbg-r4k.o

obj-$(CONFIG_SB1_CACHE_ERROR)	+= cex-sb1.o cerr-sb1.o

obj-$(CONFIG_SGI_IP22)		+= umap.o

AFLAGS_tlb-glue-r4k.o := -P

EXTRA_AFLAGS := $(CFLAGS)

include $(TOPDIR)/Rules.make
