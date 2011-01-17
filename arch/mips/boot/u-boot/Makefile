targets += vmlinux.bin.gz
quiet_cmd_gzip = GZIP $@
cmd_gzip = gzip -c9 $(VMLINUXBIN) $(obj)/vmlinux.bin.gz
$(obj)/vmlinux.bin.gz: $(obj)/../vmlinux.bin FORCE
	$(call if_changed,gzip)

MKIMAGE = mkimage

targets += uImage
quiet_cmd_uImage = MKIMAGE $@
cmd_uImage = $(MKIMAGE) -A mips -O linux -T kernel -C gzip -a $(VMLINUX_LOAD_ADDRESS) \
-e 0x$(shell $(NM) $(VMLINUX) | grep ' kernel_entry' | cut -f1 -d ' ') \
-n MIPS -d $(obj)/vmlinux.bin.gz $(obj)/uImage
$(obj)/uImage: $(obj)/vmlinux.bin.gz FORCE
	$(call if_changed,uImage)
