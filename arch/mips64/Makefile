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
LDFLAGS_BLOB   := --format binary --oformat elf64-tradlittlemips
else
tool-prefix	= mips64-linux-
LDFLAGS_BLOB   := --format binary --oformat elf64-tradbigmips
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

check_gcc = $(shell if $(AS) $(1) -o /dev/null -xc /dev/null > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi)

#
# CPU-dependent compiler/assembler options for optimization.
#
cflags-$(CONFIG_CPU_R4300)	+= -mcpu=r4300 -mips3
cflags-$(CONFIG_CPU_R4X00)	+= -mcpu=r4600 -mips3
cflags-$(CONFIG_CPU_R5000)	+= -mcpu=r8000 -mips4
cflags-$(CONFIG_CPU_NEVADA)	+= -mcpu=r8000 -mips3 -mmad
cflags-$(CONFIG_CPU_RM7000)	+= $(call check_gcc, -mcpu=r7000, -mcpu=r5000) \
				   -mips4 -Wa,--trap
cflags-$(CONFIG_CPU_SB1)	+= $(call check_gcc, -mcpu=sb1, -mcpu=r8000) \
				   -mips2 -Wa,--trap
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
# Firmware support
#
libs-$(CONFIG_ARC)		+= arch/mips/arc/
libs-$(CONFIG_SIBYTE_CFE)	+= arch/mips/sibyte/cfe/

#
# Board-dependent options and extra files
#

#
# MIPS Atlas board
#
core-$(CONFIG_MIPS_BOARDS_GEN)	+= arch/mips/mips-boards/generic/
core-$(CONFIG_MIPS_ATLAS)	+= arch/mips/mips-boards/atlas/
load-$(CONFIG_MIPS_ATLAS)	+= 0x80100000

#
# MIPS Malta board
#
core-$(CONFIG_MIPS_MALTA)	+= arch/mips/mips-boards/malta/
load-$(CONFIG_MIPS_MALTA)	+= 0x80100000

#
# MIPS SEAD board
#
core-$(CONFIG_MIPS_SEAD)	+= arch/mips/mips-boards/sead/
load-$(CONFIG_MIPS_SEAD)	+= 0x80100000

#
# SGI IP22 (Indy/Indigo2)
#
# Set the load address to >= 0x88069000 if you want to leave space for symmon,
# 0x88004000 for production kernels.  Note that the value must be 16kb aligned
# or the handling of the current variable will break.
#
core-$(CONFIG_SGI_IP22)		+= arch/mips/sgi-ip22/
load-$(CONFIG_SGI_IP22)		+= 0x88004000

#
# SGI-IP27 (Origin200/2000)
#
# Set the load address to >= 0xc000000000300000 if you want to leave space for
# symmon, 0xc00000000001c000 for production kernels.  Note that the value
# must be 16kb aligned or the handling of the current variable will break.
#
ifdef CONFIG_SGI_IP27
core-$(CONFIG_SGI_IP27)		+= arch/mips/sgi-ip27/
#load-$(CONFIG_SGI_IP27)	+= 0xa80000000001c000
ifdef CONFIG_MAPPED_KERNEL
load-$(CONFIG_SGI_IP27)		+= 0xc001c000
else
load-$(CONFIG_SGI_IP27)		+= 0x8001c000
endif
endif

#
# SGI-IP32 (O2)
#
# Set the load address to >= 0x????????? if you want to leave space for symmon,
# 0x80002000 for production kernels.  Note that the value must be 16kb aligned
# or the handling of the current variable will break.
#
core-$(CONFIG_SGI_IP32)		+= arch/mips/sgi-ip32/
load-$(CONFIG_SGI_IP32)		+= 0x80002000

#
# Sibyte SB1250 SOC
#
# This is a LIB so that it links at the end, and initcalls are later
# the sequence; but it is built as an object so that modules don't get
# removed (as happens, even if they have __initcall/module_init)
#
core-$(CONFIG_SIBYTE_SB1250)	+= arch/mips/sibyte/sb1250/

#
# Sibyte SWARM board
#
libs-$(CONFIG_SIBYTE_SWARM)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_SWARM)	+= 0x80100000

#
# SNI RM200 PCI
#
core-$(CONFIG_SNI_RM200_PCI)	+= arch/mips/sni/
load-$(CONFIG_SNI_RM200_PCI)	+= 0x80080000

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
cflags-$(CONFIG_BOOT_ELF32)	+= -Wa,-32
cflags-$(CONFIG_BOOT_ELF64)	+= -Wa,-32

AFLAGS_vmlinux.lds.o := -imacros $(srctree)/include/asm-mips64/sn/mapped_kernel.h \
		-D"LOADADDR=$(load-y)"

AFLAGS		+= $(cflags-y)
CFLAGS		+= $(cflags-y)

HEAD := arch/mips64/kernel/head.o arch/mips64/kernel/init_task.o

libs-y		+= arch/mips64/lib/
core-y		+= arch/mips64/kernel/ arch/mips64/mm/ arch/mips/math-emu/

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

makeboot = $(call descend,arch/mips/boot,$(1))

#
# SNI firmware is f*cked in interesting ways ...
#
ifdef CONFIG_SNI_RM200_PCI
all:	vmlinux.rm200
endif

vmlinux.ecoff vmlinux.rm200: vmlinux
	+@$(call makeboot,$@)

CLEAN_FILES += vmlinux.ecoff \
	       vmlinux.rm200.tmp \
	       vmlinux.rm200

archclean:
	@$(MAKE) -f scripts/Makefile.clean obj=arch/mips/boot
	@$(MAKE) -f scripts/Makefile.clean obj=arch/mips/baget
	@$(MAKE) -f scripts/Makefile.clean obj=arch/mips/lasat

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

arch/$(ARCH)/kernel/offset.s: include/asm include/linux/version.h \
				   include/config/MARKER

include/asm-$(ARCH)/offset.h.tmp: arch/$(ARCH)/kernel/offset.s
	@$(generate-asm-offset.h) < $< > $@

include/asm-$(ARCH)/offset.h: include/asm-$(ARCH)/offset.h.tmp
	@echo -n '  Generating $@'
	@$(update-if-changed)

CLEAN_FILES += include/asm-$(ARCH)/offset.h.tmp \
	       include/asm-$(ARCH)/offset.h
