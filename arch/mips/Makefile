#
# arch/mips/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994, 1995, 1996 by Ralf Baechle
# DECStation modifications by Paul M. Antoine, 1996
# ACN MIPS board modifications by Robin Farine (Robin.Farine@info.unine.ch)
# and Didier Frick (dfrick@dial.eunet.ch), copyright (C) 1996 by ACN S.A.
#

ifdef CONFIG_CPU_LITTLE_ENDIAN
cross-target	= mipsel-linux-
ifdef CONFIG_MIPS_ECOFF
oformat		= ecoff-littlemips
else
oformat		= a.out-mips-little-linux
endif
else
cross-target	= mips-linux-
ifdef CONFIG_MIPS_ECOFF
oformat		= ecoff-bigmips
else
oformat		= a.out-mips-big-linux
endif
endif

ifdef CONFIG_CROSS_COMPILE
CROSS_COMPILE	:= $(cross-target)
else
CROSS_COMPILE	:=
endif

LINKFLAGS	= -N

#
# The GCC uses -G 0 -mabicalls -fpic as default.  We don't need PIC
# code in the kernel since it only slows down the whole thing.  For the
# old GCC these options are just the defaults.  At some point we might
# make use of global pointer optimizations.
#
# We also pass -G 0 to the linker to avoid generation of a .scommon section.
#
# The DECStation requires an ECOFF kernel for remote booting, other MIPS
# machines may also.  We build an ELF kernel and them convert it into an
# ECOFF kernel.
#
CFLAGS		+= -G 0 -mno-abicalls -fno-pic
LINKFLAGS	+= -G 0

ifdef CONFIG_REMOTE_DEBUG
CFLAGS		:= $(CFLAGS) -g
endif

#
# CPU dependand compiler/assembler options for optimization.
#
ifdef CONFIG_CPU_R3000
CFLAGS		:= $(CFLAGS) -mcpu=r3000 -mips1
SUBDIRS		+= arch/mips/mips1
cpu-core	= arch/mips/mips1/mips.o
endif
ifdef CONFIG_CPU_R6000
CFLAGS		:= $(CFLAGS) -mcpu=r6000 -mips2
SUBDIRS		+= arch/mips/mips2
cpu-core	= arch/mips/mips2/mips.o
endif
ifdef CONFIG_CPU_R4X00
ifdef CONFIG_OPTIMIZE_R4600
CFLAGS		:= $(CFLAGS) -mcpu=r4600 -mips3
else
CFLAGS		:= $(CFLAGS) -mcpu=r4400 -mips3
endif
SUBDIRS		+= arch/mips/mips3
cpu-core	= arch/mips/mips3/mips.o
endif
ifdef CONFIG_CPU_R8000
CFLAGS		:= $(CFLAGS) -mcpu=r8000 -mips4
SUBDIRS		+= arch/mips/mips3
cpu-core	= arch/mips/mips3/mips.o
endif
ifdef CONFIG_CPU_R10000
CFLAGS		:= $(CFLAGS) -mcpu=r8000 -mips4
SUBDIRS		+= arch/mips/mips3
cpu-core	= arch/mips/mips3/mips.o
endif

#
# Board dependand options and extra files
#
ifdef CONFIG_ACER_PICA_61
ARCHIVES      += arch/mips/jazz/jazz.o
SUBDIRS       += arch/mips/jazz
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_ACN_MIPS_BOARD
ARCHIVES      += arch/mips/acn/acn.o
SUBDIRS       += arch/mips/acn
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_MIPS_DECSTATION
ARCHIVES      += arch/mips/dec/dec.o
SUBDIRS       += arch/mips/dec
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_DESKSTATION_RPC44
ARCHIVES      += arch/mips/deskstation/deskstation.o
SUBDIRS       += arch/mips/deskstation
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_DESKSTATION_TYNE
ARCHIVES      += arch/mips/deskstation/deskstation.o
SUBDIRS       += arch/mips/deskstation
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_MIPS_MAGNUM_3000
LINKFLAGS     += -Ttext 0x80100000
endif
ifdef CONFIG_MIPS_MAGNUM_4000
ARCHIVES      += arch/mips/jazz/jazz.o
SUBDIRS       += arch/mips/jazz
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_OLIVETTI_M700
ARCHIVES      += arch/mips/jazz/jazz.o
SUBDIRS       += arch/mips/jazz
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_SNI_RM200_PCI
ARCHIVES      += arch/mips/sni/sni.o
SUBDIRS       += arch/mips/sni
LINKSCRIPT    += arch/mips/ld.script
LOADADDR      += 0x80000000
endif

#
# Choosing incompatible machines durings configuration will result in
# error messages during linking
#
ifdef LINKSCRIPT
LINKFLAGS     += -T $(word 1,$(LINKSCRIPT))
endif

ifdef LOADADDR
LINKFLAGS     += -Ttext $(word 1,$(LOADADDR))
endif

#
# The pipe options is bad for low-mem machines
# Uncomment this if you want this.  Helps most on diskless
# Linux machines.
#
CFLAGS		+= #-pipe

HEAD := arch/mips/kernel/head.o

SUBDIRS := $(SUBDIRS) arch/mips/kernel arch/mips/mm arch/mips/lib
ARCHIVES := $(cpu-core) arch/mips/kernel/kernel.o arch/mips/mm/mm.o $(ARCHIVES)
LIBS := arch/mips/lib/lib.a $(LIBS) arch/mips/lib/lib.a

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

zImage: vmlinux
	@$(MAKEBOOT) zImage

compressed: zImage

zdisk: vmlinux
	@$(MAKEBOOT) zdisk

archclean:
	@$(MAKEBOOT) clean
	$(MAKE) -C arch/$(ARCH)/kernel clean

archdep:
	@$(MAKEBOOT) dep
