#
# Makefile for loongson based machines.
#

obj-y += setup.o init.o cmdline.o env.o time.o reset.o irq.o \
    pci.o bonito-irq.o mem.o machtype.o rtc.o

#
# Early printk support
#
obj-$(CONFIG_EARLY_PRINTK) += early_printk.o dbg.o

#
# Enable fuloong(2f) and yeeloong(2f) cpu frequency scaling support
# This is based on CS5536 mfgpt timer
#
obj-$(CONFIG_LOONGSON2F_CPU_FREQ) += clock.o

#
# Enable serial port
#
obj-$(CONFIG_SERIAL_8250) += serial.o

#
# Enable CS5536 Virtual Support Module(VSM) for virtulize the PCI configure
# space
#
obj-$(CONFIG_CS5536) += cs5536/

#
# Power Management Support
#

obj-$(CONFIG_PM) += pm.o

#
# Enable RTC Class support
#
# please enable CONFIG_RTC_DRV_CMOS
#
obj-$(CONFIG_RTC_DRV_CMOS) += rtc.o
