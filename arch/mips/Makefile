#
# Makefile.mips
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Waldorf GMBH,
# written by Ralf Baechle
#

AS		= mips-linux-as
ASFLAGS		= -mips3 -mcpu=r4000
LD		= mips-linux-ld
HOSTCC		= gcc
CC		= mips-linux-gcc -V 2.5.8 -Wa,-mips3 -mcpu=r4000 -D__KERNEL__ -I$(TOPDIR)/include
#CC		= mips-linux-gcc -V 2.6.2 -Wa,-mips3 -mcpu=r4600 -D__KERNEL__ -I$(TOPDIR)/include
MAKE		= make    
CPP		= $(CC) -E
AR		= mips-linux-ar
RANLIB		= mips-linux-ranlib
STRIP		= strip
KERNELHDRS	= /home/ralf/src/linux

zBoot/zSystem: zBoot/*.c zBoot/*.S tools/zSystem
		$(MAKE) -C zBoot

zImage:		$(CONFIGURE) tools/zSystem
		cp tools/System zImage
		sync

#zImage:	$(CONFIGURE) zBoot/zSystem tools/build
#		tools/build zBoot/zSystem $(ROOT_DEV) > zImage
#		sync

zdisk:		zImage
		mcopy -n zImage a:vmlinux

tools/zSystem:	boot/head.o init/main.o init/init.o tools/version.o linuxsubdirs
		$(LD) $(LOWLDFLAGS) boot/head.o init/main.o init/init.o \
		tools/version.o \
		$(ARCHIVES) \
		$(FILESYSTEMS) \
		$(DRIVERS) \
		$(LIBS) \
		-N -Ttext 0x80000000 \
		-o tools/System
		nm tools/System | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | \
		sort > System.map

#tools/system:	boot/head.o init/main.o init/init.o tools/version.o linuxsubdirs
#		$(LD) $(LOWLDFLAGS) boot/head.o init/main.o tools/version.o \
#		$(ARCHIVES) \
#		$(FILESYSTEMS) \
#		$(DRIVERS) \
#		$(LIBS) \
#		-N -Ttext 0x80000000 \
#		-o tools/system
#		nm tools/zSystem | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | \
#		sort > System.map

#tools/zSystem:	boot/head.o init/main.o tools/version.o linuxsubdirs
#		$(LD) $(HIGHLDFLAGS) boot/head.o init/main.o tools/version.o \
#		$(ARCHIVES) \
#		$(FILESYSTEMS) \
#		$(DRIVERS) \
#		$(LIBS) \
#		-N -Ttext 0x80600000 \
#		-o tools/zSystem
#		nm tools/zSystem | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | \
#		sort > zSystem.map
