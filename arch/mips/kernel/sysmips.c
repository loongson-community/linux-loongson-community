/*
 * MIPS specific syscalls
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 by Ralf Baechle
 */
#include <linux/errno.h>
#include <linux/linkage.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/utsname.h>

#include <asm/cachectl.h>
#include <asm/pgtable.h>
#include <asm/sysmips.h>
#include <asm/uaccess.h>

static inline size_t
strnlen_user(const char *s, size_t count)
{
	return strnlen(s, count);
}

/*
 * How long a hostname can we get from user space?
 *  -EFAULT if invalid area or too long
 *  0 if ok
 *  >0 EFAULT after xx bytes
 */
static inline int
get_max_hostname(unsigned long address)
{
	struct vm_area_struct * vma;

	vma = find_vma(current->mm, address);
	if (!vma || vma->vm_start > address || !(vma->vm_flags & VM_READ))
		return -EFAULT;
	address = vma->vm_end - address;
	if (address > PAGE_SIZE)
		return 0;
	if (vma->vm_next && vma->vm_next->vm_start == vma->vm_end &&
	   (vma->vm_next->vm_flags & VM_READ))
		return 0;
	return address;
}

asmlinkage int
sys_sysmips(int cmd, int arg1, int arg2, int arg3)
{
	int	*p;
	char	*name;
	int	flags, tmp, len, retval = -EINVAL;

	switch(cmd)
	{
	case SETNAME:
		if (!suser())
			return -EPERM;
		name = (char *) arg1;
		len = get_max_hostname((unsigned long)name);
		if (retval < 0)
			return len;
		len = strnlen_user(name, retval);
		if (len == 0 || len > __NEW_UTS_LEN)
			return -EINVAL;
		copy_from_user(system_utsname.nodename, name, len);
		system_utsname.nodename[len] = '\0';
		return 0;
	case MIPS_ATOMIC_SET:
		p = (int *) arg1;
		retval = verify_area(VERIFY_WRITE, p, sizeof(*p));
		if(retval)
			return -EINVAL;
		save_flags(flags);
		cli();
		retval = *p;
		*p = arg2;
		restore_flags(flags);
		return retval;
	case MIPS_FIXADE:
		tmp = current->tss.mflags & ~3;
		current->tss.mflags = tmp | (arg1 & 3);
		retval = 0;
		break;
	case FLUSH_CACHE:
		flush_cache_all();
		break;
	}

	return retval;
}

/*
 * No implemented yet ...
 */
asmlinkage int
sys_cachectl(char *addr, int nbytes, int op)
{
	return -ENOSYS;
}

/* For emulation of various binary types, and their shared libs,
 * we need this.
 */

extern int do_open_namei(const char *pathname, int flag, int mode,
			 struct inode **res_inode, struct inode *base);

/* Only one at this time. */
#define IRIX32_EMUL "/usr/gnemul/irix"

int open_namei(const char *pathname, int flag, int mode,
	       struct inode **res_inode, struct inode *base)
{
	if(!base && (current->personality == PER_IRIX32) &&
	   *pathname == '/') {
		struct inode *emul_ino;
		const char *p = pathname;
		char *emul_path = IRIX32_EMUL;
		int v;

		while (*p == '/')
			p++;

		if(do_open_namei (emul_path, flag, mode, &emul_ino, NULL) >= 0 &&
		   emul_ino) {
			v = do_open_namei (p, flag, mode, res_inode, emul_ino);
			if(v >= 0)
				return v;
		}
	}
	return do_open_namei (pathname, flag, mode, res_inode, base);
}
