/* $Id: linux32.c,v 1.8 2000/03/14 19:28:12 ulfc Exp $
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
#include <linux/dirent.h>
#include <linux/resource.h>
#include <linux/highmem.h>

#include <asm/uaccess.h>
#include <asm/mman.h>


#define A(__x) ((unsigned long)(__x))

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

#if 0
/*
 * count32() counts the number of arguments/envelopes
 */
static int count32(u32 * argv, int max)
{
	int i = 0;

	if (argv != NULL) {
		for (;;) {
			u32 p;
			/* egcs is stupid */
			if (!access_ok(VERIFY_READ, argv, sizeof (u32)))
				return -EFAULT;
			__get_user(p,argv);
			if (!p)
				break;
			argv++;
			if(++i > max)
				return -E2BIG;
		}
	}
	return i;
}


/*
 * 'copy_strings32()' copies argument/envelope strings from user
 * memory to free pages in kernel mem. These are in a format ready
 * to be put directly into the top of new user memory.
 */
int copy_strings32(int argc, u32 * argv, struct linux_binprm *bprm) 
{
	while (argc-- > 0) {
		u32 str;
		int len;
		unsigned long pos;

		if (get_user(str, argv+argc) || !str || !(len = strnlen_user((char *)A(str), bprm->p))) 
			return -EFAULT;
		if (bprm->p < len) 
			return -E2BIG; 

		bprm->p -= len;
		/* XXX: add architecture specific overflow check here. */ 

		pos = bprm->p;
		while (len > 0) {
			char *kaddr;
			int i, new, err;
			struct page *page;
			int offset, bytes_to_copy;

			offset = pos % PAGE_SIZE;
			i = pos/PAGE_SIZE;
			page = bprm->page[i];
			new = 0;
			if (!page) {
				page = alloc_page(GFP_HIGHUSER);
				bprm->page[i] = page;
				if (!page)
					return -ENOMEM;
				new = 1;
			}
			kaddr = (char *)kmap(page);

			if (new && offset)
				memset(kaddr, 0, offset);
			bytes_to_copy = PAGE_SIZE - offset;
			if (bytes_to_copy > len) {
				bytes_to_copy = len;
				if (new)
					memset(kaddr+offset+len, 0, PAGE_SIZE-offset-len);
			}
			err = copy_from_user(kaddr + offset, (char *)A(str), bytes_to_copy);
			flush_page_to_ram(page);
			kunmap(page);

			if (err)
				return -EFAULT; 

			pos += bytes_to_copy;
			str += bytes_to_copy;
			len -= bytes_to_copy;
		}
	}
	return 0;
}


/*
 * sys_execve32() executes a new program.
 */
int do_execve32(char * filename, u32 * argv, u32 * envp, struct pt_regs * regs)
{
	struct linux_binprm bprm;
	struct dentry * dentry;
	int retval;
	int i;

	bprm.p = PAGE_SIZE*MAX_ARG_PAGES-sizeof(void *);
	memset(bprm.page, 0, MAX_ARG_PAGES*sizeof(bprm.page[0])); 

	dentry = open_namei(filename, 0, 0);
	retval = PTR_ERR(dentry);
	if (IS_ERR(dentry))
		return retval;

	bprm.dentry = dentry;
	bprm.filename = filename;
	bprm.sh_bang = 0;
	bprm.loader = 0;
	bprm.exec = 0;
	if ((bprm.argc = count32(argv, bprm.p / sizeof(u32))) < 0) {
		dput(dentry);
		return bprm.argc;
	}

	if ((bprm.envc = count32(envp, bprm.p / sizeof(u32))) < 0) {
		dput(dentry);
		return bprm.envc;
	}

	retval = prepare_binprm(&bprm);
	if (retval < 0) 
		goto out; 

	retval = copy_strings_kernel(1, &bprm.filename, &bprm);
	if (retval < 0) 
		goto out; 

	bprm.exec = bprm.p;
	retval = copy_strings32(bprm.envc, envp, &bprm);
	if (retval < 0) 
		goto out; 

	retval = copy_strings32(bprm.argc, argv, &bprm);
	if (retval < 0) 
		goto out; 

	retval = search_binary_handler(&bprm,regs);
	if (retval >= 0)
		/* execve success */
		return retval;

out:
	/* Something went wrong, return the inode and free the argument pages*/
	if (bprm.dentry)
		dput(bprm.dentry);

	/* Assumes that free_page() can take a NULL argument. */ 
	/* I hope this is ok for all architectures */ 
	for (i = 0 ; i < MAX_ARG_PAGES ; i++)
		if (bprm.page[i])
			__free_page(bprm.page[i]);

	return retval;
}

/*
 * sys_execve() executes a new program.
 */
asmlinkage int sys32_execve(abi64_no_regargs, struct pt_regs regs)
{
	int error;
	char * filename;

	filename = getname((char *) (long)regs.regs[4]);
	printk("Executing: %s\n", filename);
	error = PTR_ERR(filename);
	if (IS_ERR(filename))
		goto out;
	error = do_execve32(filename, (u32 *) (long)regs.regs[5],
	                  (u32 *) (long)regs.regs[6], &regs);
	putname(filename);

out:
	return error;
}
#else
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
	char * filename;

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
	filename = getname((char *) (long)regs.regs[4]);
	r = PTR_ERR(filename);
	if (IS_ERR(filename))
		return(r);

	r = do_execve(filename, av, ae, &regs);
	putname(filename);
	if (IS_ERR(r))
		sys_munmap(av, len);
	return(r);
}
#endif

struct dirent32 {
	unsigned int	d_ino;
	unsigned int	d_off;
	unsigned short	d_reclen;
	char		d_name[NAME_MAX + 1];
};

static void
xlate_dirent(void *dirent64, void *dirent32, long n)
{
	long off;
	struct dirent *dirp;
	struct dirent32 *dirp32;

	off = 0;
	while (off < n) {
		dirp = (struct dirent *)(dirent64 + off);
		dirp32 = (struct dirent32 *)(dirent32 + off);
		off += dirp->d_reclen;
		dirp32->d_ino = dirp->d_ino;
		dirp32->d_off = (unsigned int)dirp->d_off;
		dirp32->d_reclen = dirp->d_reclen;
		strncpy(dirp32->d_name, dirp->d_name, dirp->d_reclen - ((3 * 4) + 2));
	}
	return;
}

asmlinkage long
sys32_getdents(unsigned int fd, void * dirent32, unsigned int count)
{
	long n;
	void *dirent64;

	dirent64 = (unsigned long)(dirent32 + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
	if ((n = sys_getdents(fd, dirent64, count - (dirent64 - dirent32))) < 0)
		return(n);
	xlate_dirent(dirent64, dirent32, n);
	return(n);
}

asmlinkage int
sys32_readdir(unsigned int fd, void * dirent32, unsigned int count)
{
	int n;
	struct dirent dirent64;

	if ((n = old_readdir(fd, &dirent64, count)) < 0)
		return(n);
	xlate_dirent(&dirent64, dirent32, dirent64.d_reclen);
	return(n);
}

struct timeval32
{
    int tv_sec, tv_usec;
};


struct rusage32 {
        struct timeval32 ru_utime;
        struct timeval32 ru_stime;
        int    ru_maxrss;
        int    ru_ixrss;
        int    ru_idrss;
        int    ru_isrss;
        int    ru_minflt;
        int    ru_majflt;
        int    ru_nswap;
        int    ru_inblock;
        int    ru_oublock;
        int    ru_msgsnd; 
        int    ru_msgrcv; 
        int    ru_nsignals;
        int    ru_nvcsw;
        int    ru_nivcsw;
};

static int
put_rusage (struct rusage32 *ru, struct rusage *r)
{
	int err;
	
	err = put_user (r->ru_utime.tv_sec, &ru->ru_utime.tv_sec);
	err |= __put_user (r->ru_utime.tv_usec, &ru->ru_utime.tv_usec);
	err |= __put_user (r->ru_stime.tv_sec, &ru->ru_stime.tv_sec);
	err |= __put_user (r->ru_stime.tv_usec, &ru->ru_stime.tv_usec);
	err |= __put_user (r->ru_maxrss, &ru->ru_maxrss);
	err |= __put_user (r->ru_ixrss, &ru->ru_ixrss);
	err |= __put_user (r->ru_idrss, &ru->ru_idrss);
	err |= __put_user (r->ru_isrss, &ru->ru_isrss);
	err |= __put_user (r->ru_minflt, &ru->ru_minflt);
	err |= __put_user (r->ru_majflt, &ru->ru_majflt);
	err |= __put_user (r->ru_nswap, &ru->ru_nswap);
	err |= __put_user (r->ru_inblock, &ru->ru_inblock);
	err |= __put_user (r->ru_oublock, &ru->ru_oublock);
	err |= __put_user (r->ru_msgsnd, &ru->ru_msgsnd);
	err |= __put_user (r->ru_msgrcv, &ru->ru_msgrcv);
	err |= __put_user (r->ru_nsignals, &ru->ru_nsignals);
	err |= __put_user (r->ru_nvcsw, &ru->ru_nvcsw);
	err |= __put_user (r->ru_nivcsw, &ru->ru_nivcsw);
	return err;
}

extern asmlinkage int sys_wait4(pid_t pid,unsigned int * stat_addr,
				int options, struct rusage * ru);

asmlinkage int
sys32_wait4(__kernel_pid_t32 pid, unsigned int *stat_addr, int options,
	    struct rusage32 *ru)
{
	if (!ru)
		return sys_wait4(pid, stat_addr, options, NULL);
	else {
		struct rusage r;
		int ret;
		unsigned int status;
		mm_segment_t old_fs = get_fs();
		
		ret = sys_wait4(pid, stat_addr ? &status : NULL, options, &r);
		if (put_rusage (ru, &r)) return -EFAULT;
		if (stat_addr && put_user (status, stat_addr))
			return -EFAULT;
		return ret;
	}
}

asmlinkage int
sys32_waitpid(__kernel_pid_t32 pid, unsigned int *stat_addr, int options)
{
	return sys32_wait4(pid, stat_addr, options, NULL);
}

