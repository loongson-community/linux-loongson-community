#
# Makefile for MIPS Platform-Specific Drivers
#

obj-$(CONFIG_LEMOTE_YEELOONG2F)	+= yeeloong_laptop.o # yeeloong_ecrom.o
CFLAGS_yeeloong_laptop.o = -I$(srctree)/arch/mips/loongson/lemote-2f

obj-$(CONFIG_LEMOTE_LYNLOONG2F)	+= lynloong_pc.o
obj-$(CONFIG_GDIUM_LAPTOP)	+= gdium_laptop.o

obj-$(CONFIG_MIPS_ACPI) += acpi_init.o
obj-$(CONFIG_CPU_HWMON) += cpu_hwmon.o
