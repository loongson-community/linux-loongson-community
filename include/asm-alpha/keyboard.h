/*
 * CPU specific parts of the keyboard driver
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef __ASM_ALPHA_KEYBOARD_H
#define __ASM_ALPHA_KEYBOARD_H

#include <asm/io.h>

#define KEYBOARD_IRQ 1

static int initialize_kbd(void);

#define kbd_inb_p(port) inb_p(port)
#define kbd_inb(port) inb(port)
#define kbd_outb_p(data,port) outb_p(data,port)
#define kbd_outb(data,port) outb(data,port)

static int
kbd_wait_for_input(void)
{
        int     n;
        int     status, data;

        n = TIMEOUT_CONST;
        do {
                status = kbd_inb(KBD_STATUS_REG);
                /*
                 * Wait for input data to become available.  This bit will
                 * then be cleared by the following read of the DATA
                 * register.
                 */

                if (!(status & KBD_OBF))
			continue;

		data = kbd_inb(KBD_DATA_REG);

                /*
                 * Check to see if a timeout error has occurred.  This means
                 * that transmission was started but did not complete in the
                 * normal time cycle.  PERR is set when a parity error occurred
                 * in the last transmission.
                 */
                if (status & (KBD_GTO | KBD_PERR)) {
			continue;
                }
		return (data & 0xff);
        } while (--n);
        return (-1);	/* timed-out if fell through to here... */
}

static void
kbd_write(int address, int data)
{
	int status;

	do {
		status = kbd_inb(KBD_STATUS_REG);  /* spin until input buffer empty*/
	} while (status & KBD_IBF);
	kbd_outb(data, address);               /* write out the data*/
}

static int
initialize_kbd(void)
{
	unsigned long flags;

	save_flags(flags); cli();

	/* Flush any pending input. */
	while (kbd_wait_for_input() != -1)
		continue;

	/*
	 * Test the keyboard interface.
	 * This seems to be the only way to get it going.
	 * If the test is successful a x55 is placed in the input buffer.
	 */
	kbd_write(KBD_CNTL_REG, KBD_SELF_TEST);
	if (kbd_wait_for_input() != 0x55) {
		printk(KERN_WARNING "initialize_kbd: "
		       "keyboard failed self test.\n");
		restore_flags(flags);
		return(-1);
	}

	/*
	 * Perform a keyboard interface test.  This causes the controller
	 * to test the keyboard clock and data lines.  The results of the
	 * test are placed in the input buffer.
	 */
	kbd_write(KBD_CNTL_REG, KBD_SELF_TEST2);
	if (kbd_wait_for_input() != 0x00) {
		printk(KERN_WARNING "initialize_kbd: "
		       "keyboard failed self test 2.\n");
		restore_flags(flags);
		return(-1);
	}

	/* Enable the keyboard by allowing the keyboard clock to run. */
	kbd_write(KBD_CNTL_REG, KBD_CNTL_ENABLE);

	/*
	 * Reset keyboard. If the read times out
	 * then the assumption is that no keyboard is
	 * plugged into the machine.
	 * This defaults the keyboard to scan-code set 2.
	 */
	kbd_write(KBD_DATA_REG, KBD_RESET);
	if (kbd_wait_for_input() != KBD_ACK) {
		printk(KERN_WARNING "initialize_kbd: "
		       "reset kbd failed, no ACK.\n");
		restore_flags(flags);
		return(-1);
	}

	/*
	 * Give the keyboard some time to breathe ...
	 *   ... or it fucks up the floppy controller, too.  Wiered.
	 */
	udelay(20);

	if (kbd_wait_for_input() != KBD_POR) {
		printk(KERN_WARNING "initialize_kbd: "
		       "reset kbd failed, not POR.\n");
		restore_flags(flags);
		return(-1);
	}

	/*
	 * now do a DEFAULTS_DISABLE always
	 */
	kbd_write(KBD_DATA_REG, KBD_DISABLE);
	if (kbd_wait_for_input() != KBD_ACK) {
		printk(KERN_WARNING "initialize_kbd: "
		       "disable kbd failed, no ACK.\n");
		restore_flags(flags);
		return(-1);
	}

	/*
	 * Enable keyboard interrupt, operate in "sys" mode,
	 *  enable keyboard (by clearing the disable keyboard bit),
	 *  disable mouse, do conversion of keycodes.
	 */
	kbd_write(KBD_CNTL_REG, KBD_WRITE_MODE);
	kbd_write(KBD_DATA_REG, KBD_EKI|KBD_SYS|KBD_DMS|KBD_KCC);

	/*
	 * now ENABLE the keyboard to set it scanning...
	 */
	kbd_write(KBD_DATA_REG, KBD_ENABLE);
	if (kbd_wait_for_input() != KBD_ACK) {
		printk(KERN_WARNING "initialize_kbd: "
		       "keyboard enable failed.\n");
		restore_flags(flags);
		return(-1);
	}

	restore_flags(flags);

	return (1);
}

extern __inline__ void
keyboard_setup()
{
        request_region(0x60,16,"keyboard");
	initialize_kbd();
}

#endif /* __ASM_ALPHA_KEYBOARD_H */
