#
#  Copyright 2000 MontaVista Software Inc.
#  Author: MontaVista Software, Inc.
#     	ppopov@mvista.com or source@mvista.com
#
# Makefile for the Alchemy Au1000 CPU, generic files.
#

export-objs		= prom.o serial.o clocks.o power.o usbdev.o

obj-y := prom.o int-handler.o dma.o irq.o puts.o time.o reset.o \
	clocks.o power.o

obj-$(CONFIG_AU1000_UART) += serial.o
obj-$(CONFIG_AU1000_USB_DEVICE) += usbdev.o
obj-$(CONFIG_REMOTE_DEBUG) += dbg_io.o
obj-$(CONFIG_RTC) += rtc.o

EXTRA_AFLAGS := $(CFLAGS)
