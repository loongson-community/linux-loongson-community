/*
 * streamable.c: streamable devices. /dev/gfx
 * (C) 1997 Miguel de Icaza (miguel@nuclecu.unam.mx)
 *
 * Major 10 is the streams clone device.  The IRIX Xsgi server just
 * opens /dev/gfx and closes it inmediately.
 *
 */

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <asm/shmiq.h>
#include "graphics.h"

#include <asm/keyboard.h>
#include <linux/kbd_kern.h>
#include <linux/vt_kern.h>

extern struct kbd_struct kbd_table [MAX_NR_CONSOLES];

/* console number where forwarding is enabled */
int forward_chars;

/* To which shmiq this keyboard is assigned */
int kbd_assigned_device;

/* previous kbd_mode for the forward_chars terminal */
int kbd_prev_mode;

static int
sgi_gfx_open (struct inode *inode, struct file *file)
{
	printk ("GFX: Opened by %d\n", current->pid);
	return 0;
}

static int
sgi_gfx_close (struct inode *inode, struct file *file)
{
	printk ("GFX: Closed by %d\n", current->pid);
	return 0;
}

static int
sgi_gfx_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	printk ("GFX: ioctl 0x%x %ld called\n", cmd, arg);
	return -EINVAL;
}

static int
sgi_keyb_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	
	/* IRIX calls I_PUSH on the opened device, go figure */
	if (cmd == I_PUSH)
		return 0;
	
	if (cmd == SHMIQ_ON){
		kbd_assigned_device = arg;
		forward_chars = fg_console + 1;
		kbd_prev_mode = kbd_table [fg_console].kbdmode;
		
	        kbd_table [fg_console].kbdmode = VC_RAW;
	} else if (cmd == SHMIQ_OFF && forward_chars){
		kbd_table [forward_chars-1].kbdmode = kbd_prev_mode;
		forward_chars = 0;
	} else
		return -EINVAL;
	return 0;
}

void
kbd_forward_char (int ch)
{
	static struct shmqevent ev;

	ev.data.flags  = (ch & 0200) ? 0 : 1;
	ev.data.which  = ch;
	ev.data.device = kbd_assigned_device + 0x11;
	shmiq_push_event (&ev);
}

struct file_operations sgi_gfx_fops = {
	NULL,			/* llseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	sgi_gfx_ioctl,		/* ioctl */
	NULL,			/* mmap */
	sgi_gfx_open,		/* open */
	sgi_gfx_close,		/* release */
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};
 
/* /dev/gfx */
static struct miscdevice dev_gfx = {
	SGI_GFX_MINOR, "sgi-gfx", &sgi_gfx_fops
};

static int
sgi_keyb_open (struct inode *inode, struct file *file)
{
	/* Nothing, but required by the misc driver */
	return 0;
}

struct file_operations sgi_keyb_fops = {
	NULL,			/* llseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	sgi_keyb_ioctl,		/* ioctl */
	NULL,			/* mmap */
	sgi_keyb_open,		/* open */
	NULL,			/* release */
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};

/* /dev/input/keyboard */
static struct miscdevice dev_input_keyboard = {
	SGI_STREAMS_KEYBOARD, "streams-keyboard", &sgi_keyb_fops
};

void
streamable_init (void)
{
	printk ("streamable misc devices registered (keyb:%d, gfx:%d)\n",
		SGI_STREAMS_KEYBOARD, SGI_GFX_MINOR);
	
	misc_register (&dev_gfx);
	misc_register (&dev_input_keyboard);
}
