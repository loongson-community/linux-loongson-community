#
# Makefile for the Linux/MIPS-specific parts of the memory manager.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

O_TARGET := mm.o

export-objs			+= ioremap.o umap.o
obj-y				+= extable.o init.o ioremap.o fault.o loadmmu.o

obj-$(CONFIG_CPU_R3000)		+= pg-r2300.o  r2300.o
obj-$(CONFIG_CPU_R4300)		+= pg-r4xx0.o  r4xx0.o
obj-$(CONFIG_CPU_R4X00)		+= pg-r4xx0.o  r4xx0.o
obj-$(CONFIG_CPU_VR41XX)	+= pg-r4xx0.o  r4xx0.o
obj-$(CONFIG_CPU_R5000)		+= pg-r4xx0.o  r4xx0.o
obj-$(CONFIG_CPU_NEVADA)	+= pg-r4xx0.o  r4xx0.o
obj-$(CONFIG_CPU_R5432)		+= pg-r5432.o  r5432.o
obj-$(CONFIG_CPU_RM7000)	+= pg-rm7k.o   rm7k.o
obj-$(CONFIG_CPU_MIPS32)	+= pg-mips32.o mips32.o
obj-$(CONFIG_CPU_MIPS64)	+= pg-mips32.o mips32.o

obj-$(CONFIG_SGI_IP22)		+= umap.o
obj-$(CONFIG_BAGET_MIPS)	+= umap.o

include $(TOPDIR)/Rules.make
