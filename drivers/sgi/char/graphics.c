/*
 * gfx.c: support for SGI's /dev/graphics, /dev/opengl
 * (C) 1997 Miguel de Icaza (miguel@nuclecu.unam.mx)
 *
 * On IRIX, /dev/graphics is [57, 0]
 *          /dev/opengl   is [57, 1]
 *
 * From a mail with Mark Kilgard, /dev/opengl and /dev/graphics are
 * the same thing, the use of /dev/graphics seems deprecated though.
 */
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include "gconsole.h"
#include "graphics.h"
#include <asm/gfx.h>

/* The boards */
#include "newport.h"

static int boards;
static struct graphics_ops cards [MAXCARDS];

int
sgi_graphics_open (struct inode *inode, struct file *file)
{
	return 0;
}

int
sgi_graphics_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int i;
	
	switch (cmd){
	case GFX_GETNUM_BOARDS:
		return boards;

	case GFX_GETBOARD_INFO: {
		struct gfx_getboardinfo_args *bia = (void *) arg;
		int    board;
		void   *dest_buf;
		int    max_len;
		
		i = verify_area (VERIFY_READ, (void *) arg, sizeof (struct gfx_getboardinfo_args));
		if (i) return i;
		
		__get_user_ret (board,    &bia->board, -EFAULT);
		__get_user_ret (dest_buf, &bia->buf,   -EFAULT);
		__get_user_ret (max_len,  &bia->len,   -EFAULT);

		if (board >= boards)
			return -EINVAL;
		if (max_len < sizeof (struct gfx_getboardinfo_args))
			return -EINVAL;
		if (max_len > cards [board].board_info_len)
			max_len = cards [boards].board_info_len;
		i = verify_area (VERIFY_WRITE, dest_buf, max_len);
		if (i) return i;
		if (copy_to_user (dest_buf, cards [board].board_info, max_len))
			return -EFAULT;
		return max_len;
	}
	} /* ioctl switch (cmd) */
	
	return -EINVAL;
}

int
sgi_graphics_close (struct inode *inode, struct file *file)
{
	return 0;
}

/* Do any post card-detection setup on graphics_ops */
static void
graphics_ops_post_init (int slot)
{
	/* There is no owner for the card initially */
	cards [slot].owner = (struct task_struct *) 0;
}

struct file_operations sgi_graphics_fops = {
	NULL,			/* llseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	sgi_graphics_ioctl,	/* ioctl */
	NULL,			/* mmap */
	sgi_graphics_open,	/* open */
	sgi_graphics_close,	/* release */
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};

/* /dev/graphics */
static struct miscdevice dev_graphics = {
        SGI_GRAPHICS_MINOR, "sgi-graphics", &sgi_graphics_fops
};

/* /dev/opengl */
static struct miscdevice dev_opengl = {
        SGI_OPENGL_MINOR, "sgi-opengl", &sgi_graphics_fops
};

void 
gfx_init (char **name)
{
	struct console_ops *console;
	struct graphics_ops *g;

	console = &gconsole;
	
	if ((g = newport_probe (boards, console, name)) != 0){
		cards [boards] = *g;
		graphics_ops_post_init (boards);
		boards++;
		console = 0;
	}
	/* Add more graphic drivers here */
	/* Keep passing console around */
	
	if (boards > MAXCARDS){
		printk ("Too many cards found on the system\n");
		prom_halt ();
	}
}

void
gfx_register (void)
{
	misc_register (&dev_graphics);
	misc_register (&dev_opengl);
}
