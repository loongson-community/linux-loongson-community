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
 
/* dev gfx */
static struct miscdevice dev_gfx = {
	SGI_GFX_MINOR, "sgi-gfx", &sgi_gfx_fops
};

void
streamable_init (void)
{
	misc_register (&dev_gfx);
}
