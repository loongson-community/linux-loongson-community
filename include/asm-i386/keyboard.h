/*
 * CPU specific parts of the keyboard driver
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef __ASM_i386_KEYBOARD_H
#define __ASM_i386_KEYBOARD_H

#include <asm/io.h>

#define KEYBOARD_IRQ 1

#define kbd_inb_p(port) inb_p(port)
#define kbd_inb(port) inb(port)
#define kbd_outb_p(data,port) outb_p(data,port)
#define kbd_outb(data,port) outb(data,port)

extern __inline__ void
keyboard_setup()
{
	request_region(0x60,16,"keyboard");
}

#endif /* __ASM_i386_KEYBOARD_H */
