/*
 *
 * BRIEF MODULE DESCRIPTION
 *	Au1000-based board setup.
 *
 * Copyright 2000 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or source@mvista.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/mc146818rtc.h>
#include <linux/delay.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/keyboard.h>
#include <asm/mipsregs.h>
#include <asm/reboot.h>
#include <asm/pgtable.h>
#include <asm/au1000.h>
#include <asm/pb1000.h>

#if defined(CONFIG_AU1000_SERIAL_CONSOLE)
extern void console_setup(char *, int *);
char serial_console[20];
#endif

#ifdef CONFIG_BLK_DEV_INITRD
extern unsigned long initrd_start, initrd_end;
extern void * __rd_start, * __rd_end;
#endif

#ifdef CONFIG_BLK_DEV_IDE
extern struct ide_ops std_ide_ops;
extern struct ide_ops *ide_ops;
#endif

void (*__wbflush) (void);
extern struct rtc_ops no_rtc_ops;
extern char * __init prom_getcmdline(void);
extern void au1000_restart(char *);
extern void au1000_halt(void);
extern void au1000_power_off(void);
extern struct resource ioport_resource;
extern struct resource iomem_resource;


void au1000_wbflush(void)
{
	__asm__ volatile ("sync");
}

void __init au1000_setup(void)
{
	char *argptr;
	u32 pin_func, static_cfg0, usb_clocks=0;
	
	argptr = prom_getcmdline();

#ifdef CONFIG_AU1000_SERIAL_CONSOLE
	if ((argptr = strstr(argptr, "console=")) == NULL) {
		argptr = prom_getcmdline();
		strcat(argptr, " console=ttyS0,115200");
	}
#endif	  

	rtc_ops = &no_rtc_ops;
        __wbflush = au1000_wbflush;
	_machine_restart = au1000_restart;
	_machine_halt = au1000_halt;
	_machine_power_off = au1000_power_off;

	// IO/MEM resources. 
	mips_io_port_base = 0;
	ioport_resource.start = 0;
	ioport_resource.end = 0xffffffff;
	iomem_resource.start = 0;
	ioport_resource.end = 0xffffffff;

#ifdef CONFIG_BLK_DEV_INITRD
	ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0);
	initrd_start = (unsigned long)&__rd_start;
	initrd_end = (unsigned long)&__rd_end;
#endif

	// set AUX clock to 12MHz * 8 = 96 MHz
	outl(8, AUX_PLL_CNTRL);
	outl(0, PIN_STATE);
	udelay(1000);

#if defined (CONFIG_USB_OHCI) || defined (CONFIG_AU1000_USB_DEVICE)
#ifdef CONFIG_USB_OHCI
	if ((argptr = strstr(argptr, "usb_ohci=")) == NULL) {
	        char usb_args[80];
		argptr = prom_getcmdline();
		memset(usb_args, 0, sizeof(usb_args));
		sprintf(usb_args, " usb_ohci=base:0x%x,len:0x%x,irq:%d",
			USB_OHCI_BASE, USB_OHCI_LEN, AU1000_USB_HOST_INT);
		strcat(argptr, usb_args);
	}
#endif
	// enable CLK2 and/or CLK1 for USB Host and/or Device clocks
	outl(0x00300000, FQ_CNTRL_1);         // FREQ2 = aux/2 = 48 MHz
#ifdef CONFIG_AU1000_USB_DEVICE
	usb_clocks |= 0x00000200; // CLK1 = FREQ2
#endif
#ifdef CONFIG_USB_OHCI
	usb_clocks |= 0x00004000; // CLK2 = FREQ2
#endif
	outl(usb_clocks, CLOCK_SOURCE_CNTRL);
	udelay(1000);

#ifdef CONFIG_USB_OHCI
	// enable host controller and wait for reset done
	outl(0x08, USB_HOST_CONFIG);
	udelay(1000);
	outl(0x0c, USB_HOST_CONFIG);
	udelay(1000);
	while (!(inl(USB_HOST_CONFIG) & 0x10))
	    ;
#endif
	
	// configure pins GPIO[14:9] as GPIO
	pin_func = inl(PIN_FUNCTION) & (u32)(~0x8080);

#ifndef CONFIG_AU1000_USB_DEVICE
	// 2nd USB port is USB host
	pin_func |= 0x8000;
#endif
	outl(pin_func, PIN_FUNCTION);
	outl(0x2800, TSTATE_STATE_SET);
	outl(0x0030, OUTPUT_STATE_CLEAR);
#endif // defined (CONFIG_USB_OHCI) || defined (CONFIG_AU1000_USB_DEVICE)

	// select gpio 15 (for interrupt line) 
	pin_func = inl(PIN_FUNCTION) & (u32)(~0x100);
	// we don't need I2S, so make it available for GPIO[31:29] 
	pin_func |= (1<<5);
	outl(pin_func, PIN_FUNCTION);

	outl(0x8000, TSTATE_STATE_SET);
	
#ifdef CONFIG_FB
	conswitchp = &dummy_con;
#endif

	static_cfg0 = inl(STATIC_CONFIG_0) & (u32)(~0xc00);
	outl(static_cfg0, STATIC_CONFIG_0);

	// configure RCE2* for LCD
	outl(0x00000004, STATIC_CONFIG_2);

	// STATIC_TIMING_2
	outl(0x08061908, STATIC_TIMING_2);

	// Set 32-bit base address decoding for RCE2*
	outl(0x10003ff0, STATIC_ADDRESS_2);

	// PCI CPLD setup
	// expand CE0 to cover PCI
	outl(0x11803e40, STATIC_ADDRESS_1);

	// burst visibility on 
	outl(inl(STATIC_CONFIG_0) | 0x1000, STATIC_CONFIG_0);

	outl(0x83, STATIC_CONFIG_1);         // ewait enabled, flash timing
	outl(0x33030a10, STATIC_TIMING_1);   // slower timing for FPGA

#ifdef CONFIG_FB_E1356
	if ((argptr = strstr(argptr, "video=")) == NULL) {
		argptr = prom_getcmdline();
		strcat(argptr, " video=e1356fb:system:pb1000,mmunalign:1");
	}
#endif // CONFIG_FB_E1356


#ifdef CONFIG_PCI
	outl(0, PCI_BRIDGE_CONFIG); // set extend byte to 0
	outl(0, SDRAM_MBAR);        // set mbar to 0
	outl(0x2, SDRAM_CMD);       // enable memory accesses
	au_sync_delay(1);
#endif

#ifndef CONFIG_SERIAL_NONSTANDARD
	/* don't touch the default serial console */
	outl(0, UART0_ADDR + UART_CLK);
#endif
	outl(0, UART1_ADDR + UART_CLK);
	outl(0, UART2_ADDR + UART_CLK);
	outl(0, UART3_ADDR + UART_CLK);

#ifdef CONFIG_BLK_DEV_IDE
	{
		argptr = prom_getcmdline();
		strcat(argptr, " ide0=noprobe");
	}
	ide_ops = &std_ide_ops;
#endif

	// setup irda clocks
	// aux clock, divide by 2, clock from 2/4 divider
	writel(readl(CLOCK_SOURCE_CNTRL) | 0x7, CLOCK_SOURCE_CNTRL);
	pin_func = inl(PIN_FUNCTION) & (u32)(~(1<<2)); // clear IRTXD
	outl(pin_func, PIN_FUNCTION);

	while (inl(PC_COUNTER_CNTRL) & PC_CNTRL_E0S);
	outl(PC_CNTRL_E0 | PC_CNTRL_EN0, PC_COUNTER_CNTRL);
	au_sync();
	while (inl(PC_COUNTER_CNTRL) & PC_CNTRL_T0S);
	outl(0, PC0_TRIM);

	printk("Alchemy Semi PB1000 Board\n");
	printk("Au1000/PB1000 port (C) 2001 MontaVista Software, Inc. (source@mvista.com)\n");
}
