.S.s:
	$(CPP) $(AFLAGS) $< -o $@
.S.o:
	$(CC) $(AFLAGS) -c $< -o $@

O_TARGET := sb1250.o

obj-y := setup.o irq.o irq_handler.o time.o lib_hssubr.o

obj-$(CONFIG_PCI) += pci.o
obj-$(CONFIG_SMP) += smp.o
obj-$(CONFIG_BCM1250_TBPROF) += bcm1250_tbprof.o

include $(TOPDIR)/Rules.make
