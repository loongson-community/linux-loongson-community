/*
 * usema.c: software semaphore driver (see IRIX's usema(7M))
 * written 1997 Mike Shaver (shaver@neon.ingenia.ca)
 *
 * This file contains the implementation of /dev/usema and /dev/usemaclone,
 * the devices used by IRIX's us* semaphore routines.
 *
 * /dev/usemaclone is used to create a new semaphore device, and then
 * the semaphore is manipulated via ioctls on /dev/usema.  I'm sure
 * it'll make more sense when I get it working. =) The devices are
 * only accessed by the libc routines, apparently,
 *
 * At least for the zero-contention case, lock set and unset as well
 * as semaphore P and V are done in userland, which makes things a
 * little bit better.  I suspect that the ioctls are used to register
 * the process as blocking, etc.
 *
 * The minor number for the usema descriptor used to poll and block
 * such determines which semaphore you're manipulating, so I think you're 
 * limited to (1 << MINORBITS - 1) semaphores active at a given time.
 *
 * Much inspiration and structure stolen from Miguel's shmiq work.
 *
 * For more information:
 * usema(7m), usinit(3p), usnewsema(3p)
 * /usr/include/sys/usioctl.h 
 *
*/

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/dcache.h>

#include <asm/smp_lock.h>
#include <asm/usioctl.h>
#include <asm/mman.h>
#include <asm/uaccess.h>

#define NUM_USEMAS (1 << MINORBITS - 1)
#define USEMA_FORMAT "/dev/usema%d"

static struct irix_usema{
	int used;
	struct file *filp;
	struct wait_queue *proc_list;
} usema_list[NUM_USEMAS];

/*
 * Generate and return a descriptor for a new semaphore.
 *
 * We want to find a usema that's not in use, and then allocate it to
 * this descriptor.  The minor number in the returned fd's st_rdev is used
 * to identify the usema, and should correspond to the index in usema_list.
 *
 * What I do right now is pretty ugly, and likely dangerous.
 * Maybe we need generic clone-device support in Linux?
 */

static int
sgi_usemaclone_open(struct inode *inode, struct file *filp)
{
	int semanum;
	char semaname[14];
	struct dentry * semadentry;
	printk("[%s:%d] wants a new usema",
	       current->comm, current->pid);
	for (semanum = 0; 
	     semanum < NUM_USEMAS && !usema_list[semanum].used;
	     semanum++)
		/* nothing */;
	if (usema_list[semanum].used) {
		printk("usemaclone: no more usemas left for process %d",
		       current->pid);
		return -EBUSY;
	}
	/* We find a dentry for the clone device that we're
	 * allocating and pull a switcheroo on filp->d_entry.
	 */
	sprintf(semaname, USEMA_FORMAT, semanum);
	semadentry = namei(semaname);
	if (!semadentry) {
		/* This Shouldn't Happen(tm) */
		printk("[%s:%d] usemaclone_open: can't find dentry for %s",
		       current->comm, current->pid, semaname);
		return -EINVAL;	/* ??? */
	}
	dput(filp->f_dentry);
	filp->f_dentry = semadentry;
	usema_list[semanum].used = 1;
	usema_list[semanum].filp = filp;
	printk("[%s:%d] got usema %d",
	       current->comm, current->pid, semanum);
	return 0;
}	

static int
sgi_usemaclone_release(struct inode *inode, struct file *filp)
{
	int semanum = MINOR(filp->f_dentry->d_inode->i_rdev);
	printk("[%s:%d] released usema %d",
	       current->comm, current->pid, semanum);
	/* There shouldn't be anything waiting on proc_list, right? */
	usema_list[semanum].used = 0;
	return 0;
}

/*
 * Generate another descriptor for an existing semaphore.
 * 
 * The minor number is used to identify usemas, so we have to create
 * a new descriptor in the current process with the appropriate minor number.
 *
 * Can I share the same fd between all the processes, a la clone()?  I can't
 * see how that'd cause any problems, and it'd mean I don't have to do my
 * own reference counting.
 */

static int
sgi_usema_attach(usattach_t * attach, int semanum) {
	int newfd;
	newfd = get_unused_fd();
	if (newfd < 0)
		return newfd;
	current->files->fd[newfd] = usema_list[semanum].filp;
	usema_list[semanum].filp->f_count++;
	/* Is that it? */
	printk("UIOCATTACHSEMA: new usema fd is %d", newfd);
	return newfd;
}

static int
sgi_usema_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int retval;
	printk("[%s:%d] wants ioctl 0x%xd (arg 0x%xd)",
	       current->comm, current->pid, cmd, arg);
	switch(cmd) {
	case UIOCATTACHSEMA: {
		/* They pass us information about the semaphore to
		   which they wish to be attached, and we create&return
		   a new fd corresponding to the appropriate semaphore.
		   */
		usattach_t *attach = (usattach_t *)arg;
		int newfd;
		int semanum;
		retval = verify_area(VERIFY_READ, attach, sizeof(usattach_t));
		if (retval) {
			printk("[%s:%d] sgi_usema_ioctl(UIOCATTACHSEMA): "
			       "verify_area failure",
			       current->comm, current->pid);
			return retval;
		}
		semanum = MINOR(attach->us_dev);
		if (semanum == 0) {
			/* This shouldn't happen (tm).
			   A minor of zero indicates that there's no
			   semaphore allocated for the given descriptor...
			   in which case there's nothing to which to attach.
			   */
			return -EINVAL;
		}
		printk("UIOCATTACHSEMA: attaching usema %d to process %d\n",
		       semanum, current->pid);
		/* XXX what is attach->us_handle for? */
		return sgi_usema_attach(attach, semanum);
		break;
	}
	case UIOCABLOCK:	/* XXX make `async' */
	case UIOCNOIBLOCK:	/* XXX maybe? */
	case UIOCBLOCK: {
		/* Block this process on the semaphore */
		usattach_t *attach = (usattach_t *)arg;
		int semanum;
		retval = verify_area(VERIFY_READ, attach, sizeof(usattach_t));
		if (retval) {
			printk("[%s:%d] sgi_usema_ioctl(UIOC*BLOCK): "
			       "verify_area failure",
			       current->comm, current->pid);
			return retval;
		}
		semanum = MINOR(attach->us_dev);
		printk("UIOC*BLOCK: putting process %d to sleep on usema %d",
		       current->pid, semanum);
		if (cmd == UIOCNOIBLOCK)
			sleep_on_interruptible(&usema_list[semanum].proc_list);
		else
			sleep_on(&usema_list[semanum].proc_list);
		return 0;
	}
	case UIOCAUNBLOCK:	/* XXX make `async' */
	case UIOCUNBLOCK: {
		/* Wake up all process waiting on this semaphore */
		usattach_t *attach = (usattach_t *)arg;
		int semanum;
		retval = verify_area(VERIFY_READ, attach, sizeof(usattach_t));
		if (retval) {
			printk("[%s:%d] sgi_usema_ioctl(UIOC*BLOCK): "
			       "verify_area failure",
			       current->comm, current->pid);
			return retval;
		}
		semanum = MINOR(attach->us_dev);
		printk("[%s:%d] releasing usema %d",
		       current->comm, current->pid, semanum);
		wake_up(&usema_list[semanum].proc_list);
		return 0;
	}
	}
	return -ENOSYS;
}

static unsigned int
sgi_usema_poll(struct file *filp, poll_table *wait)
{
	int semanum = MINOR(filp->f_dentry->d_inode->i_rdev);
	printk("[%s:%d] wants to poll usema %d",
	       current->comm, current->pid, semanum);
	/* I have to figure out what the behaviour is when:
	   - polling and the lock is free
	   - the lock becomes free and there's a process polling it
	   And I have to figure out poll()'s semantics. =)
	   */
	return -ENOSYS;
}

struct file_operations sgi_usema_fops = {
	NULL,			/* llseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	sgi_usema_poll,		/* poll */
	sgi_usema_ioctl,	/* ioctl */
	NULL,			/* mmap */
	NULL,			/* open */
	NULL,			/* release */
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};

struct file_operations sgi_usemaclone_fops = {
	NULL,			/* llseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	NULL,			/* ioctl */
	NULL,			/* mmap */
	sgi_usemaclone_open,	/* open */
	sgi_usemaclone_release,	/* release */
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};


static struct miscdevice dev_usemaclone = {
	SGI_USEMACLONE, "usemaclone", &sgi_usemaclone_fops
};

void
usema_init(void)
{
	printk("usemaclone misc device registered (minor: %d)\n",
	       SGI_USEMACLONE);
	misc_register(&dev_usemaclone);
	register_chrdev(USEMA_MAJOR, "usema", &sgi_usema_fops);
	printk("usema device registered (major %d)\n",
	       USEMA_MAJOR);
	memset((void *)usema_list, 0, sizeof(struct irix_usema) * NUM_USEMAS);
}
