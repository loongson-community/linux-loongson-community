
all: sb1250.a

OBJS-y := setup.o irq.o irq_handler.o time.o 


OBJS-$(CONFIG_SMP) += smp.o
OBJS-$(CONFIG_PCI) += pci.o pci-dma.o

sb1250.a: $(OBJS-y)
	$(AR) rcs sb1250.a $^

dep:
	$(CPP) $(CPPFLAGS) -M *.c > .depend

include $(TOPDIR)/Rules.make
