#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2002  Maciej W. Rozycki
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#

#
# Select the object file format to substitute into the linker script.
#
ifdef CONFIG_CPU_LITTLE_ENDIAN
tool-prefix	= mips64el-linux-
else
tool-prefix	= mips64-linux-
endif

ifdef CONFIG_CROSSCOMPILE
CROSS_COMPILE	= $(tool-prefix)
endif

#
# The ELF GCC uses -G 0 -mabicalls -fpic as default.  We don't need PIC
# code in the kernel since it only slows down the whole thing.  For the
# old GCC these options are just the defaults.  At some point we might
# make use of global pointer optimizations.
#
# The DECStation requires an ECOFF kernel for remote booting, other MIPS
# machines may also.  Since BFD is incredibly buggy with respect to
# crossformat linking we rely on the elf2ecoff tool for format conversion.
#
GCCFLAGS	:= -I $(TOPDIR)/include/asm/gcc
GCCFLAGS	+= -mabi=64 -G 0 -mno-abicalls -fno-pic -Wa,--trap -pipe
LDFLAGS_vmlinux	+= -G 0 -static # -N
MODFLAGS	+= -mlong-calls

ifdef CONFIG_REMOTE_DEBUG
GCCFLAGS	+= -g
endif

#
# CPU-dependent compiler/assembler options for optimization.
#
ifdef CONFIG_CPU_R4300
GCCFLAGS	+= -mcpu=r4300 -mips3
endif
ifdef CONFIG_CPU_R4X00
GCCFLAGS	+= -mcpu=r4600 -mips3
endif
ifdef CONFIG_CPU_R5000
GCCFLAGS	+= -mcpu=r8000 -mips4
endif
ifdef CONFIG_CPU_NEVADA
GCCFLAGS	+= -mcpu=r8000 -mips3 -mmad
endif
ifdef CONFIG_CPU_R8000
GCCFLAGS	+= -mcpu=r8000 -mips4
endif
ifdef CONFIG_CPU_R10000
GCCFLAGS	+= -mcpu=r8000 -mips4
endif
ifdef CONFIG_CPU_SB1
GCCFLAGS	+= -mcpu=r8000 -mips4 -Wa,--trap
ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
MODFLAGS	+= -msb1-pass1-workarounds
endif
endif
ifdef CONFIG_CPU_MIPS64
#CFLAGS		+= -mips64	# Should be used then we get a MIPS64 compiler
CFLAGS		+= -mcpu=r8000 -mips4
endif

#
# We unconditionally build the math emulator
#
CORE_FILES	+= arch/mips/math-emu/fpu_emulator.o
SUBDIRS		+= arch/mips/math-emu

#
# ramdisk/initrd support
# You need a compressed ramdisk image, named ramdisk.gz in
# arch/mips/ramdisk
#
ifdef CONFIG_EMBEDDED_RAMDISK
CORE_FILES	+= arch/mips/ramdisk/ramdisk.o
SUBDIRS		+= arch/mips/ramdisk
endif


#
# Board-dependent options and extra files
#

#
# MIPS Atlas board
#
ifdef CONFIG_MIPS_ATLAS
LIBS		+= arch/mips/mips-boards/atlas/atlas.o \
		   arch/mips/mips-boards/generic/mipsboards.o
SUBDIRS		+= arch/mips/mips-boards/generic arch/mips/mips-boards/atlas
LOADADDR	:= 0x80100000
endif

#
# MIPS Malta board
#
ifdef CONFIG_MIPS_MALTA
LIBS		+= arch/mips/mips-boards/malta/malta.o \
		   arch/mips/mips-boards/generic/mipsboards.o
SUBDIRS		+= arch/mips/mips-boards/malta arch/mips/mips-boards/generic
LOADADDR	:= 0x80100000
endif

#
# MIPS SEAD board
#
ifdef CONFIG_MIPS_SEAD
LIBS		+= arch/mips/mips-boards/sead/sead.o \
		   arch/mips/mips-boards/generic/mipsboards.o
SUBDIRS		+= arch/mips/mips-boards/generic arch/mips/mips-boards/sead
LOADADDR	:= 0x80100000
endif

#
# SGI IP22 (Indy/Indigo2)
#
ifdef CONFIG_SGI_IP22
CORE_FILES	+= arch/mips/sgi-ip22/ip22-kern.o
LIBS		+= arch/mips/arc/arclib.a
SUBDIRS		+= arch/mips/sgi-ip22 arch/mips/arc
#
# Set LOADADDR to >= 0x88069000 if you want to leave space for symmon,
# 0x88004000 for production kernels.  Note that the value must be
# 16kb aligned or the handling of the current variable will break.
#
LOADADDR	:= 0x88004000
endif

#
# SGI-IP27 (Origin200/2000)
#
ifdef CONFIG_SGI_IP27
CORE_FILES	+= arch/mips/sgi-ip27/ip27.o
LIBS		+= arch/mips/arc/arclib.a
SUBDIRS		+= arch/mips/sgi-ip27 arch/mips/arc
#
# Set LOADADDR to >= 0xc000000000300000 if you want to leave space for
# symmon, 0xc00000000001c000 for production kernels.  Note that the value
# must be 16kb aligned or the handling of the current variable will break.
#
#LOADADDR	:= 0xa80000000001c000
ifdef CONFIG_MAPPED_KERNEL
LOADADDR	:= 0xc001c000
else
LOADADDR	:= 0x8001c000
endif
endif

#
# SGI-IP32 (O2)
#
ifdef CONFIG_SGI_IP32
CORE_FILES	+= arch/mips/sgi-ip32/ip32-kern.a
LIBS		+= arch/mips/arc/arclib.a
SUBDIRS		+= arch/mips/sgi-ip32 arch/mips/arc
#
# Set LOADADDR to >= 0x????????? if you want to leave space for symmon,
# 0x80002000 for production kernels.  Note that the value must be
# 16kb aligned or the handling of the current variable will break.
#
LOADADDR	:= 0x80002000
endif

#
# Sibyte SB1250 SOC
#
ifdef CONFIG_SIBYTE_SB1250
# This is a LIB so that it links at the end, and initcalls are later
# the sequence; but it is built as an object so that modules don't get
# removed (as happens, even if they have __initcall/module_init)
LIBS		+= arch/mips/sibyte/sb1250/sb1250.o
SUBDIRS		+= arch/mips/sibyte/sb1250
endif

#
# Sibyte SWARM board
#
ifdef CONFIG_SIBYTE_SWARM
LIBS		+= arch/mips/sibyte/swarm/sbswarm.a
SUBDIRS		+= arch/mips/sibyte/swarm
LOADADDR	:= 0x80100000
endif

#
# Sibyte CFE firmware
#
ifdef CONFIG_SIBYTE_CFE
LIBS		+= arch/mips/sibyte/cfe/cfe.a
SUBDIRS		+= arch/mips/sibyte/cfe
endif

#
# Some machines like the Indy need 32-bit ELF binaries for booting purposes.
# Other need ECOFF, so we build a 32-bit ELF binary for them which we then
# convert to ECOFF using elf2ecoff.
#
ifdef CONFIG_BOOT_ELF32
GCCFLAGS	+= -Wa,-32

AFLAGS_vmlinux.lds.o := -imacros $(srctree)/include/asm-mips64/sn/mapped_kernel.h

endif

#
# The 64-bit ELF tools are pretty broken so at this time we generate 64-bit
# ELF files from 32-bit files by conversion.
#
ifdef CONFIG_BOOT_ELF64
GCCFLAGS += -Wa,-32

AFLAGS_vmlinux.lds.o := -imacros $(srctree)/include/asm-mips64/sn/mapped_kernel.h

#AS += -64
#LDFLAGS += -m elf64bmip
#
#AFLAGS_vmlinux.lds.o := -imacros $(srctree)/include/asm-mips64/sn/mapped_kernel.h

endif

AFLAGS		+= $(GCCFLAGS)
CFLAGS		+= $(GCCFLAGS)

LDFLAGS_vmlinux += -Ttext $(LOADADDR)

HEAD := arch/mips64/kernel/head.o arch/mips64/kernel/init_task.o

SUBDIRS := $(addprefix arch/mips/, tools) $(SUBDIRS) $(addprefix arch/mips64/, kernel mm lib)
CORE_FILES := arch/mips64/kernel/kernel.o arch/mips64/mm/mm.o $(CORE_FILES)
LIBS := arch/mips64/lib/lib.a $(LIBS)

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

ifdef CONFIG_CPU_LITTLE_ENDIAN
64bit-bfd = elf64-littlemips
else
64bit-bfd = elf64-bigmips
endif

ifdef CONFIG_MAPPED_KERNEL
vmlinux.64: vmlinux
	$(OBJCOPY) -O $(64bit-bfd) --change-addresses=0xbfffffff40000000 $< $@
else
vmlinux.64: vmlinux
	$(OBJCOPY) -O $(64bit-bfd) --change-addresses=0xa7ffffff80000000 $< $@
endif

zImage: vmlinux
	@$(MAKEBOOT) zImage

compressed: zImage

zdisk: vmlinux
	@$(MAKEBOOT) zdisk

archclean:
	@$(MAKEBOOT) clean
	$(MAKE) -C arch/mips/tools clean

archmrproper:
	@$(MAKEBOOT) mrproper
	$(MAKE) -C arch/mips/tools mrproper
