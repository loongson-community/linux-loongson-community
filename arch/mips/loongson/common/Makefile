#
# Makefile for loongson based machines.
#

obj-y += setup.o init.o cmdline.o env.o time.o reset.o irq.o \
    pci.o bonito-irq.o mem.o machtype.o platform.o

#
# Serial port support
#
obj-$(CONFIG_EARLY_PRINTK) += early_printk.o
<<<<<<< HEAD:arch/mips/loongson/common/Makefile
obj-$(CONFIG_SERIAL_8250) += serial.o
obj-$(CONFIG_LOONGSON_UART_BASE) += uart_base.o

#
# Enable CS5536 Virtual Support Module(VSM) to virtulize the PCI configure
# space
#
obj-$(CONFIG_CS5536) += cs5536/

#
# Suspend Support
#

obj-$(CONFIG_LOONGSON_SUSPEND) += pm.o

# Enable RTC Class support
#
# please enable CONFIG_RTC_DRV_CMOS
#
obj-$(CONFIG_RTC_DRV_CMOS) += rtc.o
