# $Id$
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
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
CROSS_COMPILE  = $(tool-prefix)
endif

#
# The ELF GCC uses -G0 -mabicalls -fpic as default.  We don't need PIC
# code in the kernel since it only slows down the whole thing.  For the
# old GCC these options are just the defaults.  At some point we might
# make use of global pointer optimizations.
#
# The DECStation requires an ECOFF kernel for remote booting, other MIPS
# machines may also.  Since BFD is incredibly buggy with respect to
# crossformat linking we rely on the elf2ecoff tool for format conversion.
#
CFLAGS		+= -mabi=64 -G 0 -mno-abicalls -fno-pic -pipe
LINKFLAGS	+= -G 0
LINKFLAGS	= -static # -N
MODFLAGS	+= -mlong-calls

ifdef CONFIG_REMOTE_DEBUG
CFLAGS		:= $(CFLAGS) -g
endif

#
# CPU-dependent compiler/assembler options for optimization.
#
ifdef CONFIG_CPU_R4300
CFLAGS		:= $(CFLAGS) -mcpu=r4300 -mips3
endif
ifdef CONFIG_CPU_R4X00
CFLAGS		:= $(CFLAGS) -mcpu=r4600 -mips3
endif
ifdef CONFIG_CPU_R5000
CFLAGS		:= $(CFLAGS) -mcpu=r8000 -mips4
endif
ifdef CONFIG_CPU_NEVADA
CFLAGS		:= $(CFLAGS) -mcpu=r8000 -mips3 -mmad
endif
ifdef CONFIG_CPU_R8000
CFLAGS		:= $(CFLAGS) -mcpu=r8000 -mips4
endif
ifdef CONFIG_CPU_R10000
CFLAGS		:= $(CFLAGS) -mcpu=r8000 -mips4
endif

#
# Board-dependent options and extra files
#
ifdef CONFIG_IP22
LIBS          += arch/mips/sgi/kernel/sgikern.a arch/mips/arc/arclib.a
SUBDIRS       += arch/mips/sgi/kernel arch/mips/arc
#
# Set LOADADDR to >= 0x88069000 if you want to leave space for symmon,
# 0x88002000 for production kernels.  Note that the value must be
# 8kb aligned or the handling of the current variable will break.
#
LOADADDR      += 0x88002000
HOSTCC        = cc
endif

#
# Choosing incompatible machines durings configuration will result in
# error messages during linking.  Select a default linkscript if
# none has been choosen above.
#
ifndef LINKSCRIPT
ifndef CONFIG_CPU_LITTLE_ENDIAN
LINKSCRIPT    = arch/mips/ld.script.big
else
LINKSCRIPT    = arch/mips/ld.script.little
endif
endif
LINKFLAGS     += -T $(word 1,$(LINKSCRIPT))

ifdef LOADADDR
LINKFLAGS     += -Ttext $(word 1,$(LOADADDR))
endif

HEAD := arch/mips/kernel/head.o arch/mips/kernel/init_task.o

SUBDIRS := $(addprefix arch/mips/, tools) $(SUBDIRS) $(addprefix arch/mips/, kernel mm lib)
CORE_FILES := arch/mips/kernel/kernel.o arch/mips/mm/mm.o $(CORE_FILES)
LIBS := arch/mips/lib/lib.a $(LIBS)

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

zImage: vmlinux
	@$(MAKEBOOT) zImage

compressed: zImage

zdisk: vmlinux
	@$(MAKEBOOT) zdisk

archclean:
	@$(MAKEBOOT) clean
	$(MAKE) -C arch/$(ARCH)/kernel clean
	$(MAKE) -C arch/$(ARCH)/tools clean

archmrproper:

archdep:
	@$(MAKEBOOT) dep
