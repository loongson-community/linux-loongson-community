
all: sb1kern.a

OBJS-y := 

OBJS-$(CONFIG_SIBYTE_IKOS) += ikos.o ikos_irq_handler.o

OBJS-$(CONFIG_SB1_CACHE_ERROR) += cache_err_handler.o 

sb1kern.a: $(OBJS-y)
	$(AR) rcs sb1kern.a $^

dep:
	$(CPP) $(CPPFLAGS) -M *.c > .depend

include $(TOPDIR)/Rules.make
