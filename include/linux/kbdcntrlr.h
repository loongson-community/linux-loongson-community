/*
 * Keyboard controller definitions.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef __LINUX_KBDCTRLR_H
#define __LINUX_KBDCTRLR_H

/*
 * keyboard controller registers
 */
#define KBD_STATUS_REG      (unsigned int) 0x64
#define KBD_CNTL_REG        (unsigned int) 0x64
#define KBD_DATA_REG	    (unsigned int) 0x60
/*
 * controller commands
 */
#define KBD_READ_MODE	    (unsigned int) 0x20
#define KBD_WRITE_MODE	    (unsigned int) 0x60
#define KBD_SELF_TEST	    (unsigned int) 0xAA
#define KBD_SELF_TEST2	    (unsigned int) 0xAB
#define KBD_CNTL_ENABLE	    (unsigned int) 0xAE
/*
 * keyboard commands
 */
#define KBD_ENABLE	    (unsigned int) 0xF4
#define KBD_DISABLE	    (unsigned int) 0xF5
#define KBD_RESET	    (unsigned int) 0xFF
/*
 * keyboard replies
 */
#define KBD_ACK		    (unsigned int) 0xFA
#define KBD_POR		    (unsigned int) 0xAA
/*
 * status register bits
 */
#define KBD_OBF		    (unsigned int) 0x01
#define KBD_IBF		    (unsigned int) 0x02
#define KBD_GTO		    (unsigned int) 0x40
#define KBD_PERR	    (unsigned int) 0x80
/*
 * keyboard controller mode register bits
 */
#define KBD_EKI		    (unsigned int) 0x01
#define KBD_SYS		    (unsigned int) 0x04
#define KBD_DMS		    (unsigned int) 0x20
#define KBD_KCC		    (unsigned int) 0x40

/*
 * Had to increase this value - the speed ratio host cpu/keyboard
 * processor on some MIPS machines make the controller initialization
 * fail otherwise.  -- Ralf
 */
#define TIMEOUT_CONST	1000000

/*
 * These will be the new keyboard controller access macros.  I don't use
 * them yet since I want to talk about some details with Linus first.
 */
#define kbd_read_control() kbd_inb_p(0x64)
#define kbd_read_status() kbd_inb_p(0x64)
#define kbd_read_data() kbd_inb_p(0x60)
#define kbd_write_data() kbd_inb_p(0x60)

#endif /* __LINUX_KBDCTRLR_H */
