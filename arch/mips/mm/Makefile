#
# Makefile for the Linux/MIPS-specific parts of the memory manager.
#

export-objs			+= ioremap.o loadmmu.o umap.o
obj-y				+= extable.o init.o ioremap.o fault.o \
				   pgtable.o loadmmu.o

obj-$(CONFIG_HIGHMEM)		+= highmem.o

obj-$(CONFIG_CPU_R3000)		+= pg-r3k.o c-r3k.o tlb-r3k.o tlbex-r3k.o
obj-$(CONFIG_CPU_TX39XX)	+= pg-r3k.o c-tx39.o tlb-r3k.o tlbex-r3k.o
obj-$(CONFIG_CPU_TX49XX)	+= pg-r4k.o c-tx49.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_R4300)		+= pg-r4k.o c-r4k.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_R4X00)		+= pg-r4k.o c-r4k.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_VR41XX)	+= pg-r4k.o c-r4k.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_R5000)		+= pg-r4k.o c-r4k.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_NEVADA)	+= pg-r4k.o c-r4k.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_R5000_CPU_SCACHE)  += r5k-sc.o
obj-$(CONFIG_CPU_R5432)		+= pg-r5432.o c-r5432.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_RM7000)	+= pg-rm7k.o c-rm7k.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_MIPS32)	+= pg-mips32.o c-mips32.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_MIPS64)	+= pg-mips32.o c-mips32.o tlb-r4k.o tlbex-r4k.o
obj-$(CONFIG_CPU_SB1)		+= c-sb1.o cex-sb1.o cerr-sb1.o pg-sb1.o \
				   tlb-sb1.o tlbex-r4k.o

obj-$(CONFIG_SGI_IP22)		+= umap.o
obj-$(CONFIG_BAGET_MIPS)	+= umap.o

EXTRA_AFLAGS := $(CFLAGS)
