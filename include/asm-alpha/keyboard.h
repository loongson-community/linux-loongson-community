/*
 *  linux/include/asm-alpha/keyboard.h
 *
 *  Created 3 Nov 1996 by Geert Uytterhoeven
 *
 * $Id: keyboard.h,v 1.3 1997/07/24 01:55:54 ralf Exp $
 */

/*
 *  This file contains the alpha architecture specific keyboard definitions
 */

#ifndef _ALPHA_KEYBOARD_H
#define _ALPHA_KEYBOARD_H

#ifdef __KERNEL__

#include <linux/config.h>
#include <linux/ioport.h>
#include <asm/io.h>

#define KEYBOARD_IRQ			1
#define DISABLE_KBD_DURING_INTERRUPTS	0

extern int pckbd_setkeycode(unsigned int scancode, unsigned int keycode);
extern int pckbd_getkeycode(unsigned int scancode);
extern int pckbd_pretranslate(unsigned char scancode, char raw_mode);
extern int pckbd_translate(unsigned char scancode, unsigned char *keycode,
			   char raw_mode);
extern char pckbd_unexpected_up(unsigned char keycode);
extern void pckbd_leds(unsigned char leds);
extern void pckbd_init_hw(void);
extern unsigned char pckbd_sysrq_xlate[128];

#define kbd_setkeycode		pckbd_setkeycode
#define kbd_getkeycode		pckbd_getkeycode
#define kbd_pretranslate	pckbd_pretranslate
#define kbd_translate		pckbd_translate
#define kbd_unexpected_up	pckbd_unexpected_up
#define kbd_leds		pckbd_leds
#define kbd_init_hw		pckbd_init_hw
#define kbd_sysrq_xlate		pckbd_sysrq_xlate

#define INIT_KBD

#define SYSRQ_KEY 0x54

/*
 * keyboard controller registers
 */
#define KBD_STATUS_REG      (unsigned int) 0x64
#define KBD_CNTL_REG        (unsigned int) 0x64
#define KBD_DATA_REG        (unsigned int) 0x60

/* How to access the keyboard macros on this platform.  */
#define kbd_read_input() inb(KBD_DATA_REG)
#define kbd_read_status() inb(KBD_STATUS_REG)
#define kbd_write_output(val) outb(val, KBD_DATA_REG)
#define kbd_write_command(val) outb(val, KBD_CNTL_REG)

/* Some stoneage hardware needs delays after some operations.  */
#define kbd_pause() do { } while(0)

#define keyboard_setup()						\
        request_region(0x60, 16, "keyboard")

/*
 * Machine specific bits for the PS/2 driver
 */

#if defined(CONFIG_PCI)
#define AUX_IRQ 12
#else
#define AUX_IRQ 9			/* Jensen is odd indeed */
#endif

#define ps2_request_irq()						\
	request_irq(AUX_IRQ, aux_interrupt, 0, "PS/2 Mouse", NULL)

#define ps2_free_irq(inode) free_irq(AUX_IRQ, NULL)

#endif /* __KERNEL__ */
#endif /* __ASM_ALPHA_KEYBOARD_H */
