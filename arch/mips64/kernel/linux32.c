/* $Id: linux32.c,v 1.5 2000/02/29 22:13:07 kanoj Exp $
 * 
 * Conversion between 32-bit and 64-bit native system calls.
 *
 * Copyright (C) 2000 Silicon Graphics, Inc.
 * Written by Ulf Carlsson (ulfc@engr.sgi.com)
 * sys32_execve from ia64/ia32 code, Feb 2000, Kanoj Sarcar (kanoj@sgi.com)
 */

#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/smp_lock.h>
#include <linux/highuid.h>

#include <asm/uaccess.h>
#include <asm/mman.h>

/*
 * Revalidate the inode. This is required for proper NFS attribute caching.
 */
static __inline__ int
do_revalidate(struct dentry *dentry)
{
	struct inode * inode = dentry->d_inode;
	if (inode->i_op && inode->i_op->revalidate)
		return inode->i_op->revalidate(dentry);
	return 0;
}

static int cp_new_stat32(struct inode * inode, struct stat32 * statbuf)
{
	struct stat32 tmp;
	unsigned int blocks, indirect;

	memset(&tmp, 0, sizeof(tmp));
	tmp.st_dev = kdev_t_to_nr(inode->i_dev);
	tmp.st_ino = inode->i_ino;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	SET_STAT_UID(tmp, inode->i_uid);
	SET_STAT_GID(tmp, inode->i_gid);
	tmp.st_rdev = kdev_t_to_nr(inode->i_rdev);
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
/*
 * st_blocks and st_blksize are approximated with a simple algorithm if
 * they aren't supported directly by the filesystem. The minix and msdos
 * filesystems don't keep track of blocks, so they would either have to
 * be counted explicitly (by delving into the file itself), or by using
 * this simple algorithm to get a reasonable (although not 100% accurate)
 * value.
 */

/*
 * Use minix fs values for the number of direct and indirect blocks.  The
 * count is now exact for the minix fs except that it counts zero blocks.
 * Everything is in units of BLOCK_SIZE until the assignment to
 * tmp.st_blksize.
 */
#define D_B   7
#define I_B   (BLOCK_SIZE / sizeof(unsigned short))

	if (!inode->i_blksize) {
		blocks = (tmp.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
		if (blocks > D_B) {
			indirect = (blocks - D_B + I_B - 1) / I_B;
			blocks += indirect;
			if (indirect > 1) {
				indirect = (indirect - 1 + I_B - 1) / I_B;
				blocks += indirect;
				if (indirect > 1)
					blocks++;
			}
		}
		tmp.st_blocks = (BLOCK_SIZE / 512) * blocks;
		tmp.st_blksize = BLOCK_SIZE;
	} else {
		tmp.st_blocks = inode->i_blocks;
		tmp.st_blksize = inode->i_blksize;
	}
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}
asmlinkage int sys32_newstat(char * filename, struct stat32 *statbuf)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = namei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_new_stat32(dentry->d_inode, statbuf);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}
asmlinkage int sys32_newlstat(char *filename, struct stat32 * statbuf)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = lnamei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_new_stat32(dentry->d_inode, statbuf);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

asmlinkage int sys32_newfstat(unsigned int fd, struct stat32 * statbuf)
{
	struct file * f;
	int err = -EBADF;

	lock_kernel();
	f = fget(fd);
	if (f) {
		struct dentry * dentry = f->f_dentry;

		err = do_revalidate(dentry);
		if (!err)
			err = cp_new_stat32(dentry->d_inode, statbuf);
		fput(f);
	}
	unlock_kernel();
	return err;
}
asmlinkage int sys_mmap2(void) {return 0;}

asmlinkage int sys_truncate64(const char *path, unsigned long high,
			      unsigned long low)
{
	if ((int)high < 0)
		return -EINVAL;
	return sys_truncate(path, (high << 32) | low);
}

asmlinkage int sys_ftruncate64(unsigned int fd, unsigned long high,
			       unsigned long low)
{
	if ((int)high < 0)
		return -EINVAL;
	return sys_ftruncate(fd, (high << 32) | low);
}

asmlinkage int sys_stat64(char * filename, struct stat *statbuf)
{
	return sys_newstat(filename, statbuf);
}

asmlinkage int sys_lstat64(char * filename, struct stat *statbuf)
{
	return sys_newlstat(filename, statbuf);
}

asmlinkage int sys_fstat64(unsigned int fd, struct stat *statbuf)
{
	return sys_fstat(fd, statbuf);
}

static int
nargs(unsigned int arg, char **ap)
{
	char *ptr; 
	int n, err;

	n = 0;
	for (ptr++; ptr; ) {
		if ((err = get_user(ptr, (int *)arg)))
			return(err);
		if (ap)
			*ap++ = ptr;
		arg += sizeof(unsigned int);
		n++;
	}
	return(n - 1);
}

asmlinkage long
sys32_execve(abi64_no_regargs, struct pt_regs regs)
{
	extern asmlinkage int sys_execve(abi64_no_regargs, struct pt_regs regs);
	extern asmlinkage long sys_munmap(unsigned long addr, size_t len);
	unsigned int argv = (unsigned int)regs.regs[5];
	unsigned int envp = (unsigned int)regs.regs[6];
	char **av, **ae;
	int na, ne, r, len;

	na = nargs(argv, NULL);
	ne = nargs(envp, NULL);
	len = (na + ne + 2) * sizeof(*av);
	/*
	 *  kmalloc won't work because the `sys_exec' code will attempt
	 *  to do a `get_user' on the arg list and `get_user' will fail
	 *  on a kernel address (simplifies `get_user').  Instead we
	 *  do an mmap to get a user address.  Note that since a successful
	 *  `execve' frees all current memory we only have to do an
	 *  `munmap' if the `execve' failes.
	 */
	down(&current->mm->mmap_sem);
	lock_kernel();

	av = do_mmap_pgoff(0, NULL, len,
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0);

	unlock_kernel();
	up(&current->mm->mmap_sem);

	if (IS_ERR(av))
		return(av);
	ae = av + na + 1;
	av[na] = (char *)0;
	ae[ne] = (char *)0;
	(void)nargs(argv, av);
	(void)nargs(envp, ae);
	regs.regs[5] = av;
	regs.regs[6] = ae;
	r = sys_execve(__dummy0,__dummy0,__dummy0,__dummy0,__dummy0,__dummy0,__dummy0,__dummy0, regs);
	if (IS_ERR(r))
		sys_munmap(av, len);
	return(r);
}
