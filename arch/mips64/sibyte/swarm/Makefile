
all: sbswarm.a

OBJS-y                   = setup.o cmdline.o rtc.o time.o memory.o 

OBJS-$(CONFIG_SMP)      += smp.o



sbswarm.a: $(OBJS-y)
	$(AR) rcs sbswarm.a $^

dep:
	$(CPP) $(CPPFLAGS) -M *.c > .depend

include $(TOPDIR)/Rules.make

