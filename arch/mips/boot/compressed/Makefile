#
# This file is subject to the terms and conditions of the GNU General Public
# License.
#
# Adapted for MIPS Pete Popov, Dan Malek
#
# Copyright (C) 1994 by Linus Torvalds
# Adapted for PowerPC by Gary Thomas
# modified by Cort (cort@cs.nmt.edu)
#
# Copyright (C) 2009 Lemote Inc. & DSLab, Lanzhou University
# Author: Wu Zhangjin <wuzj@lemote.com>
#

# compressed kernel load addr: VMLINUZ_LOAD_ADDRESS > VMLINUX_LOAD_ADDRESS + VMLINUX_SIZE
VMLINUX_SIZE := $(shell wc -c $(objtree)/$(KBUILD_IMAGE) 2>/dev/null | cut -d' ' -f1)
VMLINUX_SIZE := $(shell [ -n "$(VMLINUX_SIZE)" ] && echo $$(($(VMLINUX_SIZE) + (65536 - $(VMLINUX_SIZE) % 65536))))
VMLINUX_LOAD_ADDRESS = $(shell echo $(LOADADDR) | sed -e 's/0xffffffff//')
VMLINUZ_LOAD_ADDRESS := $(shell [ -n "$(VMLINUX_SIZE)" ] && printf %x $$((0x$(VMLINUX_LOAD_ADDRESS) + $(VMLINUX_SIZE))))
VMLINUZ_LOAD_ADDRESS := 0x$(if $(CONFIG_64BIT),ffffffff,)$(VMLINUZ_LOAD_ADDRESS)
LOADADDR := 0x$(if $(CONFIG_64BIT),ffffffff,)$(VMLINUX_LOAD_ADDRESS)

# set the default size of the mallocing area for decompressing
BOOT_HEAP_SIZE := 0x400000

KBUILD_CFLAGS := $(LINUXINCLUDE) $(KBUILD_CFLAGS) -D__KERNEL__ \
	-DBOOT_HEAP_SIZE=$(BOOT_HEAP_SIZE) -D"LOADADDR=$(LOADADDR)ull" \

# uncomment the -DDEBUG to enable serial port debugging for your machine
# and please ensure there is a suitable serial port address defined in dbg.h
KBUILD_CFLAGS += #-DDEBUG

KBUILD_AFLAGS := $(LINUXINCLUDE) $(KBUILD_AFLAGS) -D__ASSEMBLY__ \
	-DKERNEL_ENTRY=0x$(shell $(NM) $(objtree)/$(KBUILD_IMAGE) 2>/dev/null | grep " kernel_entry" | cut -f1 -d \ ) \
	-DBOOT_HEAP_SIZE=$(BOOT_HEAP_SIZE)

OBJECTS := $(obj)/head.o $(obj)/decompress.o $(obj)/dbg.o

OBJCOPYFLAGS_vmlinux.bin := $(OBJCOPYFLAGS) -O binary -R .comment -S
$(obj)/vmlinux.bin: $(KBUILD_IMAGE)
	$(call if_changed,objcopy)

suffix_$(CONFIG_KERNEL_GZIP)  = gz
suffix_$(CONFIG_KERNEL_BZIP2) = bz2
suffix_$(CONFIG_KERNEL_LZMA)  = lzma
tool_$(CONFIG_KERNEL_GZIP)    = gzip
tool_$(CONFIG_KERNEL_BZIP2)   = bzip2
tool_$(CONFIG_KERNEL_LZMA)    = lzma
$(obj)/vmlinux.$(suffix_y): $(obj)/vmlinux.bin
	$(call if_changed,$(tool_y))
	$(Q)rm -f $<

$(obj)/piggy.o: $(obj)/vmlinux.$(suffix_y) $(obj)/dummy.o
	$(Q)$(OBJCOPY) $(OBJCOPYFLAGS) \
		--add-section=.image=$< \
		--set-section-flags=.image=contents,alloc,load,readonly,data \
		$(obj)/dummy.o $@
	$(Q)rm -f $<

LDFLAGS_vmlinuz := $(LDFLAGS) -Ttext $(VMLINUZ_LOAD_ADDRESS) -T
$(obj)/vmlinuz: $(src)/ld.script $(OBJECTS) $(obj)/piggy.o
	$(call if_changed,ld)
	$(Q)$(OBJCOPY) $(OBJCOPYFLAGS) -R .comment -R .stab -R .stabstr -R .initrd -R .sysmap $@
	$(Q)rm -f $(obj)/piggy.o

zImage: $(obj)/vmlinuz
	$(Q)ln -sf $< $(objtree)/vmlinuz
	$(Q)echo "  SYMLINK $(objtree)/vmlinuz"

clean:
clean-files += *.o \
	       vmlinu*
