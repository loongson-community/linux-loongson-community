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
#

ifdef CONFIG_CPU_LITTLE_ENDIAN
CROSS_COMPILE	= mipsel-
ifdef CONFIG_MIPS_ECOFF
oformat		= ecoff-littlemips
else
oformat		= a.out-mips-big-linux
endif
else
CROSS_COMPILE	= mips-
ifdef CONFIG_MIPS_ECOFF
oformat		= ecoff-bigmips
else
oformat		= a.out-mips-big-linux
endif
endif

ifdef CONFIG_EXTRA_ELF_COMPILER
CROSS_COMPILE	:= $(CROSS_COMPILE)linuxelf-
else
CROSS_COMPILE	:= $(CROSS_COMPILE)linux-
endif

LINKFLAGS	= -static -N

#
# The new ELF GCC uses -G0 -mabicalls -fpic as default.  We don't need PIC
# code in the kernel since it only slows down the whole thing.  For the
# old GCC these options are just the defaults.  At some point we might
# make use of global pointer optimizations.
#
# The DECStation requires an ECOFF kernel for remote booting, other MIPS
# machines may also.
#
ifdef CONFIG_ELF_KERNEL
CFLAGS		+= -G 0 -mno-abicalls -fno-pic
LINKFLAGS	+= -G 0
endif
ifdef CONFIG_ECOFF_KERNEL
CFLAGS		+= -G 0 -mno-abicalls -fno-pic
LINKFLAGS	+= -G 0 -oformat ecoff-littlemips
endif

ifdef CONFIG_REMOTE_DEBUG
CFLAGS		:= $(CFLAGS) -g
endif

CFLAGS		:= $(CFLAGS) -mips2

#
# Board dependand options and extra files
#
ifdef CONFIG_ACER_PICA_61
ARCHIVES      += arch/mips/jazz/jazz.o
SUBDIRS       += arch/mips/jazz
LOADADDR      += 0x80000000
endif
ifdef CONFIG_DECSTATION
ARCHIVES      += arch/mips/dec/dec.o
SUBDIRS       += arch/mips/dec
LINKSCRIPT    += arch/mips/dec/ld.script
LOADADDR      += 0x80000000
endif
ifdef CONFIG_DESKSTATION_RPC44
ARCHIVES      += arch/mips/deskstation/deskstation.o
SUBDIRS       += arch/mips/deskstation
LOADADDR      += 0x80100000
endif
ifdef CONFIG_DESKSTATION_TYNE
ARCHIVES      += arch/mips/deskstation/deskstation.o
SUBDIRS       += arch/mips/deskstation
LOADADDR      += 0x80000000
endif
ifdef CONFIG_MIPS_MAGNUM_3000
LOADADDR      += 0x80000000
endif
ifdef CONFIG_MIPS_MAGNUM_4000
ARCHIVES      += arch/mips/jazz/jazz.o
SUBDIRS       += arch/mips/jazz
LOADADDR      += 0x80000000
endif
ifdef CONFIG_SNI_RM200_PCI
ARCHIVES      += arch/mips/sni/sni.o
SUBDIRS       += arch/mips/sni
LOADADDR      += 0x80000000
endif
ifdef CONFIG_SGI
ARCHIVES      += arch/mips/sgi/kernel/sgikern.a arch/mips/sgi/prom/promlib.a
SUBDIRS       += arch/mips/sgi/kernel arch/mips/sgi/prom
LOADADDR      += 0x88069000
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

#
# The pipe options is bad for my low-mem machine
# Uncomment this if you want this.
#
#CFLAGS		+= -pipe

HEAD := arch/mips/kernel/head.o

SUBDIRS := $(SUBDIRS) $(addprefix arch/mips/, kernel mm lib tools)
ARCHIVES := arch/mips/kernel/kernel.o arch/mips/mm/mm.o $(ARCHIVES)
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
	$(MAKE) -C arch/$(ARCH)/tools clean

archdep:
	@$(MAKEBOOT) dep
