/*
 *  linux/include/asm-alpha/keyboard.h
 *
 *  Created 3 Nov 1996 by Geert Uytterhoeven
 */

/*
 *  This file contains the alpha architecture specific keyboard definitions
 */

#ifndef _ALPHA_KEYBOARD_H
#define _ALPHA_KEYBOARD_H

#ifdef __KERNEL__

#include <linux/ioport.h>
#include <asm/io.h>

#define KEYBOARD_IRQ			1
#define DISABLE_KBD_DURING_INTERRUPTS	0

#define KBD_REPORT_ERR
#define KBD_REPORT_UNKN
/* #define KBD_IS_FOCUS_9000 */

extern int pckbd_setkeycode(unsigned int scancode, unsigned int keycode);
extern int pckbd_getkeycode(unsigned int scancode);
extern int pckbd_pretranslate(unsigned char scancode, char raw_mode);
extern int pckbd_translate(unsigned char scancode, unsigned char *keycode,
			   char raw_mode);
extern char pckbd_unexpected_up(unsigned char keycode);
extern void pckbd_leds(unsigned char leds);
extern void pckbd_init_hw(void);

#define kbd_setkeycode		pckbd_setkeycode
#define kbd_getkeycode		pckbd_getkeycode
#define kbd_pretranslate	pckbd_pretranslate
#define kbd_translate		pckbd_translate
#define kbd_unexpected_up	pckbd_unexpected_up
#define kbd_leds		pckbd_leds
#define kbd_init_hw		pckbd_init_hw

#define INIT_KBD

/*
 * keyboard controller registers
 */
#define KBD_STATUS_REG      (unsigned int) 0x64
#define KBD_CNTL_REG        (unsigned int) 0x64
#define KBD_DATA_REG        (unsigned int) 0x60

#define kbd_inb_p(port) inb_p(port)
#define kbd_inb(port) inb(port)
#define kbd_outb_p(data,port) outb_p(data,port)
#define kbd_outb(data,port) outb(data,port)

extern __inline__ void keyboard_setup()
{
        request_region(0x60,16,"keyboard");
}

#endif /* __KERNEL__ */
#endif /* __ASM_ALPHA_KEYBOARD_H */
