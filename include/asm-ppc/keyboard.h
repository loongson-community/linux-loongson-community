/*
 *  linux/include/asm-ppc/keyboard.h
 *
 *  Created 3 Nov 1996 by Geert Uytterhoeven
 *
 * $Id: keyboard.h,v 1.9 1999/06/10 10:08:56 ralf Exp $
 * Modified for Power Macintosh by Paul Mackerras
 */

/*
 * This file contains the ppc architecture specific keyboard definitions -
 * like the intel pc for prep systems, different for power macs.
 */

#ifndef __ASMPPC_KEYBOARD_H
#define __ASMPPC_KEYBOARD_H

#ifdef __KERNEL__

#include <asm/io.h>

#include <linux/config.h>
#include <asm/adb.h>
#include <asm/machdep.h>
#ifdef CONFIG_APUS
#include <asm-m68k/keyboard.h>
#else

#define KEYBOARD_IRQ			1
#define DISABLE_KBD_DURING_INTERRUPTS	0
#define INIT_KBD

static inline int kbd_setkeycode(unsigned int scancode, unsigned int keycode)
{
	return ppc_md.kbd_setkeycode(scancode, keycode);
}

static inline int kbd_getkeycode(unsigned int scancode)
{
	return ppc_md.kbd_getkeycode(scancode);
}

static inline int kbd_translate(unsigned char keycode, unsigned char *keycodep,
				char raw_mode)
{
	return ppc_md.kbd_translate(keycode, keycodep, raw_mode);
}

static inline int kbd_unexpected_up(unsigned char keycode)
{
	return ppc_md.kbd_unexpected_up(keycode);
}

static inline void kbd_leds(unsigned char leds)
{
	ppc_md.kbd_leds(leds);
}

static inline void kbd_init_hw(void)
{
	ppc_md.kbd_init_hw();
}

#define kbd_sysrq_xlate	(ppc_md.ppc_kbd_sysrq_xlate)

extern unsigned long SYSRQ_KEY;

#endif /* CONFIG_APUS */

/* How to access the keyboard macros on this platform.  */
#define kbd_read_input() inb(KBD_DATA_REG)
#define kbd_read_status() inb(KBD_STATUS_REG)
#define kbd_write_output(val) outb(val, KBD_DATA_REG)
#define kbd_write_command(val) outb(val, KBD_CNTL_REG)

/* Some stoneage hardware needs delays after some operations.  */
#define kbd_pause() do { } while(0)

#endif /* CONFIG_MAC_KEYBOARD */

#define keyboard_setup()						\
	request_region(0x60, 16, "keyboard")

/*
 * Machine specific bits for the PS/2 driver
 *
 * FIXME: does any PPC machine use the PS/2 driver at all?  If so,
 *        this should work, if not it's dead code ...
 */

#define AUX_IRQ 12

#define ps2_request_irq()						\
	request_irq(AUX_IRQ, aux_interrupt, 0, "PS/2 Mouse", NULL)

#define ps2_free_irq(inode) free_irq(AUX_IRQ, NULL)

#endif /* __KERNEL__ */

#endif /* __ASMPPC_KEYBOARD_H */
