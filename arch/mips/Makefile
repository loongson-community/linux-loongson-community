#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994, 1995, 1996 by Ralf Baechle
# DECStation modifications by Paul M. Antoine, 1996
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
tool-prefix	= mipsel-linux-
JIFFIES32	= jiffies_64
else
tool-prefix	= mips-linux-
JIFFIES32	= jiffies_64 + 4
endif

ifdef CONFIG_CROSSCOMPILE
CROSS_COMPILE	= $(tool-prefix)
endif

#
# GCC uses -G 0 -mabicalls -fpic as default.  We don't want PIC in the kernel
# code since it only slows down the whole thing.  At some point we might make
# use of global pointer optimizations but their use of $28 conflicts with
# the current pointer optimization.
#
# The DECStation requires an ECOFF kernel for remote booting, other MIPS
# machines may also.  Since BFD is incredibly buggy with respect to
# crossformat linking we rely on the elf2ecoff tool for format conversion.
#
cflags-y			:= -I $(TOPDIR)/include/asm/gcc
cflags-y			+= -G 0 -mno-abicalls -fno-pic -pipe
LDFLAGS_vmalinux		+= -G 0 -static # -N
MODFLAGS			+= -mlong-calls

cflags-$(CONFIG_REMOTE_DEBUG)	+= -g

#
# CPU-dependent compiler/assembler options for optimization.
#
cflags-$(CONFIG_CPU_R3000)	+= -mcpu=r3000 -mips1
cflags-$(CONFIG_CPU_TX39XX)	+= -mcpu=r3000 -mips1
cflags-$(CONFIG_CPU_R6000)	+= -mcpu=r6000 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_R4300)	+= -mcpu=r4300 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_VR41XX)	+= -mcpu=r4600 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_R4X00)	+= -mcpu=r4600 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_TX49XX)	+= -mcpu=r4600 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS32)	+= -mcpu=r4600 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS64)	+= -mcpu=r4600 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_R5000)	+= -mcpu=r5000 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_R5432)	+= -mcpu=r5000 -mips2 -Wa,--trap
# Cannot use -mmad with currently recommended tools
cflags-$(CONFIG_CPU_NEVADA)	+= -mcpu=r5000 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_RM7000)	+= -mcpu=r5000 -mips2 -Wa,--trap
cflags-$(CONFIG_CPU_SB1)	+= -mcpu=r8000 -mips2 -Wa,--trap
ifdef CONFIG_CPU_SB1
ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
MODFLAGS	+= -msb1-pass1-workarounds
endif
endif

AFLAGS		+= $(cflags-y)
CFLAGS		+= $(cflags-y)


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
# Acer PICA 61, Mips Magnum 4000 and Olivetti M700.
#
core-$(CONFIG_MIPS_JAZZ)	+= arch/mips/jazz/
load-$(CONFIG_MIPS_JAZZ)	+= 0x80080000

#
# Au1000 (Alchemy Semi PB1000) eval board
#
core-$(CONFIG_ALCHEMY_COMMON)	+= arch/mips/au1000/common/
core-$(CONFIG_MIPS_PB1000)	+= arch/mips/au1000/pb1000/
load-$(CONFIG_MIPS_PB1000)	+= 0x80100000

#
# Au1100 (Alchemy Semi PB1100) eval board
#
core-$(CONFIG_MIPS_PB1100)	+= arch/mips/au1000/pb1100/
load-$(CONFIG_MIPS_PB1100)	+= 0x80100000

#
# Au1500 (Alchemy Semi PB1500) eval board
#
core-$(CONFIG_MIPS_PB1500)	+= arch/mips/au1000/pb1500/
load-$(CONFIG_MIPS_PB1500)	+= 0x80100000

#
# Baget/MIPS
#
libs-$(CONFIG_BAGET_MIPS)	+= arch/mips/baget/ arch/mips/baget/prom/
load-$(CONFIG_BAGET_MIPS)	+= 0x80001000

#
# Cobalt Server
#
core-$(CONFIG_MIPS_COBALT)	+= arch/mips/cobalt/
load-$(CONFIG_MIPS_COBALT)	+= 0x80080000

#
# DECstation family
#
core-$(CONFIG_DECSTATION)	+= arch/mips/dec/
libs-$(CONFIG_DECSTATION)	+= arch/mips/dec/prom/
load-$(CONFIG_DECSTATION)	+= 0x80040000
CLEAN_FILES			+= drivers/tc/lk201-map.c

#
# Galileo EV64120 Board
#
core-$(CONFIG_MIPS_EV64120)	+= arch/mips/galileo-boards/ev64120/
load-$(CONFIG_MIPS_EV64120)	+= 0x80100000

#
# Galileo EV96100 Board
#
core-$(CONFIG_MIPS_EV96100)	+= arch/mips/galileo-boards/generic/ \
				   arch/mips/galileo-boards/ev96100/
load-$(CONFIG_MIPS_EV96100)	+= 0x80100000

#
# Globespan IVR eval board with QED 5231 CPU
#
core-$(CONFIG_MIPS_IVR)		+= arch/mips/ite-boards/generic/ \
				   arch/mips/ite-boards/ivr/
load-$(CONFIG_MIPS_IVR)		+= 0x80100000

#
# HP LaserJet
#
core-$(CONFIG_HP_LASERJET)	+= arch/mips/hp-lj/
load-$(CONFIG_HP_LASERJET)	+= 0x80030000

#
# ITE 8172 eval board with QED 5231 CPU
#
core-$(CONFIG_MIPS_ITE8172)	+= arch/mips/ite-boards/generic/ \
				   arch/mips/ite-boards/qed-4n-s01b/
load-$(CONFIG_MIPS_ITE8172)	+= 0x80100000

#
# MIPS Atlas board
#
core-$(CONFIG_MIPS_ATLAS)	+= arch/mips/mips-boards/generic/ \
				   arch/mips/mips-boards/atlas/
load-$(CONFIG_MIPS_ATLAS)	+= 0x80100000

#
# MIPS Malta board
#
core-$(CONFIG_MIPS_MALTA)	+= arch/mips/mips-boards/malta/ \
				   arch/mips/mips-boards/generic/
load-$(CONFIG_MIPS_MALTA)	:= 0x80100000

#
# MIPS SEAD board
#
core-$(CONFIG_MIPS_SEAD)	+= arch/mips/mips-boards/generic/ \
				   arch/mips/mips-boards/sead/
load-$(CONFIG_MIPS_SEAD)	+= 0x80100000

#
# Momentum Ocelot board
#
# The Ocelot setup.o must be linked early - it does the ioremap() for the
# mips_io_port_base.
#
core-$(CONFIG_MOMENCO_OCELOT)	+= arch/mips/gt64120/common/ \
				   arch/mips/gt64120/momenco_ocelot/
load-$(CONFIG_MOMENCO_OCELOT)	+= 0x80100000

#
# Momentum Ocelot-G board
#
# The Ocelot-G setup.o must be linked early - it does the ioremap() for the
# mips_io_port_base.
#
core-$(CONFIG_MOMENCO_OCELOT_G)	+= arch/mips/momentum/ocelot_g/
load-$(CONFIG_MOMENCO_OCELOT_G)	+= 0x80100000

#
# NEC DDB Vrc-5074
#
core-$(CONFIG_DDB5074)		+= arch/mips/ddb5xxx/common/ \
				   arch/mips/ddb5xxx/ddb5074/
load-$(CONFIG_DDB5074)		+= 0x80080000

#
# NEC DDB Vrc-5476
#
core-$(CONFIG_DDB5476)		+= arch/mips/ddb5xxx/common/ \
				   arch/mips/ddb5xxx/ddb5476/
load-$(CONFIG_DDB5476)		+= 0x80080000

#
# NEC DDB Vrc-5477
#
core-$(CONFIG_DDB5477)		+= arch/mips/ddb5xxx/common/ \
				   arch/mips/ddb5xxx/ddb5477/
load-$(CONFIG_DDB5477)		+= 0x80080000

core-$(CONFIG_LASAT)		+= arch/mips/lasat/
load-$(CONFIG_LASAT)		+= 0x80000000

#
# NEC Osprey (vr4181) board
#
core-$(CONFIG_NEC_OSPREY)	+= arch/mips/vr4181/common/ \
				   arch/mips/vr4181/osprey/
load-$(CONFIG_NEC_OSPREY)	+= 0x80002000

#
# NEC Eagle/Hawk (VR4122/VR4131) board
#
core-$(CONFIG_NEC_EAGLE)	+= arch/mips/vr41xx/common/ \
				   arch/mips/vr41xx/nec-eagle/
load-$(CONFIG_NEC_EAGLE)	+= 0x80000000

#
# ZAO Networks Capcella (VR4131)
#
core-$(CONFIG_ZAO_CAPCELLA)	+= arch/mips/vr41xx/common/ \
				   arch/mips/vr41xx/zao-capcella/
load-$(CONFIG_ZAO_CAPCELLA)	+= 0x80000000

#
# Victor MP-C303/304 (VR4122)
#
core-$(CONFIG_VICTOR_MPC30X)	+= arch/mips/vr41xx/common/ \
				   arch/mips/vr41xx/victor-mpc30x/
load-$(CONFIG_VICTOR_MPC30X)	+= 0x80001000

#
# IBM WorkPad z50 (VR4121)
#
core-$(CONFIG_IBM_WORKPAD)	+= arch/mips/vr41xx/common/ \
				   arch/mips/vr41xx/ibm-workpad/
load-$(CONFIG_IBM_WORKPAD)	+= 0x80004000

#
# CASIO CASSIPEIA E-55/65 (VR4111)
#
core-$(CONFIG_CASIO_E55)	+= arch/mips/vr41xx/common/ \
				   arch/mips/vr41xx/casio-e55/
load-$(CONFIG_CASIO_E55)	+= 0x80004000

#
# Philips Nino
#
core-$(CONFIG_NINO)		+= arch/mips/philips/nino/
load-$(CONFIG_NINO)		+= 0x80000000

#
# SGI IP22 (Indy/Indigo2)
#
# Set the load address to >= 0x88069000 if you want to leave space for symmon,
# 0x88002000 for production kernels.  Note that the value must be 8kb aligned
# or the handling of the current variable will break.
#
core-$(CONFIG_SGI_IP22)		+= arch/mips/sgi-ip22/
load-$(CONFIG_SGI_IP22)		+= 0x88002000

#
# Sibyte SB1250 SOC
#
# This is a LIB so that it links at the end, and initcalls are later
# the sequence; but it is built as an object so that modules don't get
# removed (as happens, even if they have __initcall/module_init)
#
libs-$(CONFIG_SIBYTE_SB1250)	+= arch/mips/sibyte/sb1250/

#
# Sibyte SWARM board
#
libs-$(CONFIG_SIBYTE_SWARM)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_SWARM)	:= 0x80100000

#
# SNI RM200 PCI
#
core-$(CONFIG_SNI_RM200_PCI)	+= arch/mips/sni/
load-$(CONFIG_SNI_RM200_PCI)	+= 0x80080000

#
# Toshiba JMR-TX3927 board
#
core-$(CONFIG_TOSHIBA_JMR3927)	+= arch/mips/jmr3927/rbhma3100/ \
				   arch/mips/jmr3927/common/
load-$(CONFIG_TOSHIBA_JMR3927)	+= 0x80050000


#
# Choosing incompatible machines durings configuration will result in
# error messages during linking.  Select a default linkscript if
# none has been choosen above.
#

AFLAGS_vmlinux.lds.o := -D"LOADADDR=$(load-y)" -D"JIFFIES32=$(JIFFIES32)"

HEAD := arch/mips/kernel/head.o arch/mips/kernel/init_task.o

libs-y		+= arch/mips/lib/
core-y		+= arch/mips/kernel/ arch/mips/mm/ arch/mips/math-emu/


ifdef CONFIG_BAGET_MIPS

BAGETBOOT = $(MAKE) -C arch/$(ARCH)/baget

balo: vmlinux
	$(BAGETBOOT) balo

endif

ifdef CONFIG_MIPS_EV64120
GALILEOBOOT = $(MAKE) -C arch/$(ARCH)/galileo-boards/ev64120

gboot: vmlinux
	$(MAKE) -C arch/$(ARCH)/galileo-boards/ev64120/compressed
endif

ifdef CONFIG_LASAT
rom.bin rom.sw: vmlinux
	$(MAKE) -C arch/$(ARCH)/lasat/image $@
endif

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

vmlinux.ecoff: vmlinux
	@$(MAKEBOOT) $@

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

arch/$(ARCH)/kernel/offset.s: include/asm include/linux/version.h \
				   include/config/MARKER

include/asm-$(ARCH)/offset.h.tmp: arch/$(ARCH)/kernel/offset.s
	@$(generate-asm-offset.h) < $< > $@

include/asm-$(ARCH)/offset.h: include/asm-$(ARCH)/offset.h.tmp
	@echo -n '  Generating $@'
	@$(update-if-changed)

CLEAN_FILES += include/asm-$(ARCH)/offset.h.tmp \
	       include/asm-$(ARCH)/offset.h
