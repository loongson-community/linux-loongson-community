#
# Makefile for the PCI specific kernel interface routines under Linux.
#
# This is all organized on a per system base which is horribly wrong and
# really wants a cleanup.  You have been warned.
#

#
# PCI bus host bridge specific code
#
obj-$(CONFIG_ITE_BOARD_GEN)	+= ops-it8172.o
obj-$(CONFIG_MIPS_NILE4)	+= ops-nile4.o
obj-$(CONFIG_MIPS_BONITO64)	+= ops-bonito64.o
obj-$(CONFIG_MIPS_GT64120)	+= ops-gt64120.o
obj-$(CONFIG_MIPS_MSC)		+= ops-msc.o

#
# These are still pretty much in the old state, watch, go blind.
#
obj-$(CONFIG_DDB5074)		+= pci-ddb5074.o ops-ddb5074.o
obj-$(CONFIG_DDB5476)		+= pci-ddb5476.o ops-ddb5476.o
obj-$(CONFIG_DDB5477)		+= pci-ddb5477.o ops-ddb5477.o
obj-$(CONFIG_HP_LASERJET)	+= pci-hplj.o
obj-$(CONFIG_MIPS_COBALT)	+= pci-cobalt.o
obj-$(CONFIG_MIPS_EV96100)	+= fixup-ev96100.o ops-ev96100.o
obj-$(CONFIG_MIPS_ITE8172)	+= fixup-ite8172g.o
obj-$(CONFIG_MIPS_IVR)		+= fixup-ivr.o
obj-$(CONFIG_MIPS_PB1500)	+= fixups-au1000.o ops-au1000.o
obj-$(CONFIG_MOMENCO_OCELOT)	+= fixups-ocelot.o ops-ocelot.o
obj-$(CONFIG_NEC_EAGLE)		+= fixup-eagle.o ops-vrc4173.o
obj-$(CONFIG_SGI_IP27)		+= pci-ip27.o
obj-$(CONFIG_SGI_IP32)		+= ops-mace.o pci-ip32.o
obj-$(CONFIG_SIBYTE_SB1250)	+= pci-sb1250.o
obj-$(CONFIG_SNI_RM200_PCI)	+= pci-sni.o
obj-$(CONFIG_TANBAC_TB0226)	+= fixup-tb0226.o
obj-$(CONFIG_TANBAC_TB0229)	+= fixup-tb0229.o
obj-$(CONFIG_TOSHIBA_JMR3927)	+= fixup-jmr3927.o ops-jmr3927.o
#obj-$(CONFIG_MOMENCO_OCELOT_C)	+= pci-ocelot-c.o
obj-$(CONFIG_MOMENCO_OCELOT_G)	+= pci-ocelot-g.o
obj-$(CONFIG_VICTOR_MPC30X)	+= fixup-capcella.o
obj-$(CONFIG_VR41XX_COMMON)	+= pci-vr41xx.o
obj-$(CONFIG_ZAO_CAPCELLA)	+= fixup-victor-mpc30x.o

#
# Choosen on a per system base not per firmware
#
obj-$(CONFIG_NEW_PCI)		+= pci.o
obj-$(CONFIG_MIPS_ATLAS)	+= common.o
obj-$(CONFIG_MIPS_MALTA)	+= common.o
obj-$(CONFIG_LASAT)		+= common.o
