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
cflags-y			:= -I $(TOPDIR)/include/asm/gcc
cflags-y			+= -mabi=64 -G 0 -mno-abicalls -fno-pic -Wa,--trap -pipe
LDFLAGS_vmlinux			+= -G 0 -static # -N
MODFLAGS			+= -mlong-calls

cflags-$(CONFIG_REMOTE_DEBUG)	+= -g

#
# CPU-dependent compiler/assembler options for optimization.
#
cflags-$(CONFIG_CPU_R4300)	+= -mcpu=r4300 -mips3
cflags-$(CONFIG_CPU_R4X00)	+= -mcpu=r4600 -mips3
cflags-$(CONFIG_CPU_R5000)	+= -mcpu=r8000 -mips4
cflags-$(CONFIG_CPU_NEVADA)	+= -mcpu=r8000 -mips3 -mmad
cflags-$(CONFIG_CPU_R8000)	+= -mcpu=r8000 -mips4
cflags-$(CONFIG_CPU_R10000)	+= -mcpu=r8000 -mips4
ifdef CONFIG_CPU_SB1
ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
MODFLAGS	+= -msb1-pass1-workarounds
endif
endif
# Should be used then we get a MIPS64 compiler
#cflags-$(CONFIG_CPU_MIPS64)	+= -mips64
cflags-$(CONFIG_CPU_MIPS64)	+= -mcpu=r8000 -mips4

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
core-y		+= arch/mips/mips-boards/generic/ arch/mips/mips-boards/atlas/
LOADADDR	:= 0x80100000
endif

#
# MIPS Malta board
#
ifdef CONFIG_MIPS_MALTA
core-y		+= arch/mips/mips-boards/malta/ arch/mips/mips-boards/generic/
LOADADDR	:= 0x80100000
endif

#
# MIPS SEAD board
#
ifdef CONFIG_MIPS_SEAD
core-y		+= arch/mips/mips-boards/generic/ arch/mips/mips-boards/sead/
LOADADDR	:= 0x80100000
endif

#
# SGI IP22 (Indy/Indigo2)
#
# Set LOADADDR to >= 0x88069000 if you want to leave space for symmon,
# 0x88004000 for production kernels.  Note that the value must be
# 16kb aligned or the handling of the current variable will break.
#
ifdef CONFIG_SGI_IP22
core-y		+= arch/mips/sgi-ip22/
libs-y		+= arch/mips/arc/
LOADADDR	:= 0x88004000
endif

#
# SGI-IP27 (Origin200/2000)
#
# Set LOADADDR to >= 0xc000000000300000 if you want to leave space for
# symmon, 0xc00000000001c000 for production kernels.  Note that the value
# must be 16kb aligned or the handling of the current variable will break.
#
ifdef CONFIG_SGI_IP27
core-y		+= arch/mips/sgi-ip27/
libs-y		+= arch/mips/arc/
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
# Set LOADADDR to >= 0x????????? if you want to leave space for symmon,
# 0x80002000 for production kernels.  Note that the value must be
# 16kb aligned or the handling of the current variable will break.
#
ifdef CONFIG_SGI_IP32
core-y		+= arch/mips/sgi-ip32/
libs-y		+= arch/mips/arc/
LOADADDR	:= 0x80002000
endif

#
# Sibyte SB1250 SOC
#
# This is a LIB so that it links at the end, and initcalls are later
# the sequence; but it is built as an object so that modules don't get
# removed (as happens, even if they have __initcall/module_init)
#
ifdef CONFIG_SIBYTE_SB1250
core-y		+= arch/mips/sibyte/sb1250/
endif

#
# Sibyte SWARM board
#
ifdef CONFIG_SIBYTE_SWARM
libs-y		+= arch/mips/sibyte/swarm/
LOADADDR	:= 0x80100000
endif

#
# Sibyte CFE firmware
#
libs-$(CONFIG_SIBYTE_CFE)	+= arch/mips/sibyte/cfe/

#
# Some machines like the Indy need 32-bit ELF binaries for booting purposes.
# Other need ECOFF, so we build a 32-bit ELF binary for them which we then
# convert to ECOFF using elf2ecoff.
#
# The 64-bit ELF tools are pretty broken so at this time we generate 64-bit
# ELF files from 32-bit files by conversion.
#
#AS += -64
#LDFLAGS += -m elf64bmip
CFLAGS-$(CONFIG_BOOT_ELF32)	+= -Wa,-32
cflags-$(CONFIG_BOOT_ELF64)	+= -Wa,-32

AFLAGS_vmlinux.lds.o := -imacros $(srctree)/include/asm-mips64/sn/mapped_kernel.h

AFLAGS		+= $(cflags-y)
CFLAGS		+= $(cflags-y)

LDFLAGS_vmlinux += -Ttext $(LOADADDR)

HEAD := arch/mips64/kernel/head.o arch/mips64/kernel/init_task.o

libs-y		+= arch/mips64/lib
core-y		+= arch/mips64/kernel arch/mips64/mm arch/mips/math-emu

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
	$(MAKE) -C arch/mips/baget clean
	$(MAKE) -C arch/mips/lasat clean

archmrproper:
	@$(MAKEBOOT) mrproper

# Generate <asm/offset.h 
#
# The default rule is suffering from funny problems on MIPS so we using our
# own ...
#
# ---------------------------------------------------------------------------

define generate-asm-offset.h
	(set -e; \
	 echo "#ifndef __ASM_OFFSET_H"; \
	 echo "#define __ASM_OFFSET_H"; \
	 echo "/*"; \
	 echo " * DO NOT MODIFY."; \
	 echo " *"; \
	 echo " * This file was generated by arch/$(ARCH)/Makefile"; \
	 echo " *"; \
	 echo " */"; \
	 echo ""; \
	 sed -ne "/^@@@/s///p"; \
	 echo "#endif /* __ASM_OFFSET_H */" )
endef

prepare: include/asm-$(ARCH)/offset.h

arch/$(ARCH)/tools/offset.s: include/asm include/linux/version.h \
				   include/config/MARKER

include/asm-$(ARCH)/offset.h.tmp: arch/$(ARCH)/tools/offset.s
	@$(generate-asm-offset.h) < $< > $@

include/asm-$(ARCH)/offset.h: include/asm-$(ARCH)/offset.h.tmp
	@echo -n '  Generating $@'
	@$(update-if-changed)

CLEAN_FILES += include/asm-$(ARCH)/offset.h.tmp \
	       include/asm-$(ARCH)/offset.h
