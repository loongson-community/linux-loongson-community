# Makefile for gdium

obj-y += irq.o reset.o platform.o

obj-$(CONFIG_MFD_SM501) += sm501-pwm.o
obj-$(CONFIG_GDIUM_PWM_CLOCK) += gdium-clock.o
obj-$(CONFIG_LOONGSON_SUSPEND) += pm.o pm-asm.o pm-funcs.o
