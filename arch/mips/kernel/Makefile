#
# Makefile for the Linux/MIPS kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

all:	kernel.o head.o
EXTRA_ASFLAGS = -mips3 -mcpu=r4000
O_TARGET := kernel.o
O_OBJS	:= branch.o process.o signal.o entry.o traps.o irq.o ptrace.o vm86.o \
	   ioport.o setup.o syscall.o sysmips.o time.o bios32.o ipc.o ksyms.o \
	   unaligned.o tags.o

#
# Kernel debugging
#
ifdef CONFIG_REMOTE_DEBUG
O_OBJS += gdb-low.o gdb-stub.o 
endif

#
# Depending from some other kernel option
#
ifdef CONFIG_PROC_FS
O_OBJS += proc.o
endif

#
# Since we add the same object files to O_OBJS for different configurations.
# O_OBJS might contain duplicate files.  We correct this by filtering out
# duplicate files.  Just to avoid users having to know about all the
# compatibility stuff between various boards and boards.
#
O_OBJS := $(sort $(O_OBJS))

all: kernel.o head.o

entry.o: entry.S
exception.o: exception.S
head.o: head.S

clean:

include $(TOPDIR)/Rules.make
