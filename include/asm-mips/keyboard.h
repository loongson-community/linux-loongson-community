/*
 * CPU specific parts of the keyboard driver
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef __ASM_MIPS_KEYBOARD_H
#define __ASM_MIPS_KEYBOARD_H

#include <linux/config.h>

/*
 * The default IO slowdown is doing 'inb()'s from 0x61, which should be
 * safe. But as that is the keyboard controller chip address, we do our
 * slowdowns here by doing short jumps: the keyboard controller should
 * be able to keep up
 */
#define REALLY_SLOW_IO
#define SLOW_IO_BY_JUMPING
#include <asm/io.h>
#include <asm/bootinfo.h>
#include <asm/jazz.h>

/*
 * Not true for Jazz machines, we cheat a bit for 'em.
 */
#define KEYBOARD_IRQ 1

static int initialize_kbd(void);

int (*kbd_inb_p)(unsigned short port);
int (*kbd_inb)(unsigned short port);
void (*kbd_outb_p)(unsigned char data, unsigned short port);
void (*kbd_outb)(unsigned char data, unsigned short port);

#ifdef CONFIG_MIPS_JAZZ
/*
 * We want the full initialization for the keyboard controller.
 */
#define INIT_KBD

static volatile keyboard_hardware *kh = (void *) JAZZ_KEYBOARD_ADDRESS;

static int
jazz_kbd_inb_p(unsigned short port)
{
	int result;

	if(port == KBD_DATA_REG)
		result = kh->data;
	else /* Must be KBD_STATUS_REG */
		result = kh->command;
	inb(0x80);

	return result;
}

static int
jazz_kbd_inb(unsigned short port)
{
	int result;

	if(port == KBD_DATA_REG)
		result = kh->data;
	else /* Must be KBD_STATUS_REG */
		result = kh->command;

	return result;
}

static void
jazz_kbd_outb_p(unsigned char data, unsigned short port)
{
	if(port == KBD_DATA_REG)
		kh->data = data;
	else if(port == KBD_CNTL_REG)
		kh->command = data;
	inb(0x80);
}

static void
jazz_kbd_outb(unsigned char data, unsigned short port)
{
	if(port == KBD_DATA_REG)
		kh->data = data;
	else if(port == KBD_CNTL_REG)
		kh->command = data;
}
#endif /* CONFIG_MIPS_JAZZ */

/*
 * Most other MIPS machines access the keyboard controller via
 * ordinary I/O ports.
 */
static int
port_kbd_inb_p(unsigned short port)
{
	return inb_p(port);
}

static int
port_kbd_inb(unsigned short port)
{
	return inb(port);
}

static void
port_kbd_outb_p(unsigned char data, unsigned short port)
{
	return outb_p(data, port);
}

static void
port_kbd_outb(unsigned char data, unsigned short port)
{
	return outb(data, port);
}

#ifdef INIT_KBD
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

static void kbd_write(int address, int data)
{
	int status;

	do {
		status = kbd_inb(KBD_STATUS_REG);  /* spin until input buffer empty*/
	} while (status & KBD_IBF);
	kbd_outb(data, address);               /* write out the data*/
}

static int initialize_kbd(void)
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
#endif

extern __inline__ void
keyboard_setup(void)
{
#ifdef CONFIG_MIPS_JAZZ
        if (mips_machgroup == MACH_GROUP_JAZZ) {
		kbd_inb_p = jazz_kbd_inb_p;
		kbd_inb = jazz_kbd_inb;
		kbd_outb_p = jazz_kbd_outb_p;
		kbd_outb = jazz_kbd_outb;
		/*
		 * Enable keyboard interrupts.
		 */
		*((volatile u16 *)JAZZ_IO_IRQ_ENABLE) |= JAZZ_IE_KEYBOARD;
		set_cp0_status(IE_IRQ1, IE_IRQ1);
		initialize_kbd();
	} else
#endif
	if (mips_machgroup == MACH_GROUP_ARC ||	/* this is for Deskstation */
	    (mips_machgroup == MACH_GROUP_SNI_RM
	     && mips_machtype == MACH_SNI_RM200_PCI)) {
		/*
		 * These machines address their keyboard via the normal
		 * port address range.
		 *
		 * Also enable Scan Mode 2.
		 */
		kbd_inb_p = port_kbd_inb_p;
		kbd_inb = port_kbd_inb;
		kbd_outb_p = port_kbd_outb_p;
		kbd_outb = port_kbd_outb;
		request_region(0x60,16,"keyboard");

		kb_wait();
		kbd_outb(0x60, 0x64); /* 60 == PS/2 MODE ??  */
		kb_wait();
		kbd_outb(0x41, 0x60); /* 4d:same as freebsd, 41:KCC+EKI */
		kb_wait();
		if (!send_data(0xf0) || !send_data(0x02))
			printk("Scanmode 2 change failed\n");
	}
}

#endif /* __ASM_MIPS_KEYBOARD_H */
