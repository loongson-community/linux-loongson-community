
all: sb1kern.a

SB1_OBJS := tlb.o cache.o 

ifdef CONFIG_SIBYTE_IKOS
SB1_OBJS += ikos.o ikos_irq_handler.o
endif

sb1kern.a: $(SB1_OBJS)
	$(AR) rcs sb1kern.a $(SB1_OBJS)

dep:
	$(CPP) $(CPPFLAGS) -M *.c > .depend

include $(TOPDIR)/Rules.make
