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
#include <asm/cache.h>
#include <asm/ipc.h>
#include <asm/uaccess.h>
#include <asm/sysmips.h>

static inline size_t
strnlen_user(const char *s, size_t count)
{
	return strnlen(s, count);
}

asmlinkage int
sys_sysmips(int cmd, int arg1, int arg2, int arg3)
{
	int	*p;
	char	*name;
	int	flags, tmp, len, retval;

	switch(cmd)
	{
	case SETNAME:
		if (!suser())
			return -EPERM;
		name = (char *) arg1;
		len = strnlen_user(name, retval);
		if (len < 0)
			retval = len;
			break;
		if (len == 0 || len > __NEW_UTS_LEN)
			retval = -EINVAL;
			break;
		copy_from_user(system_utsname.nodename, name, len);
		system_utsname.nodename[len] = '\0';
		return 0;

	case MIPS_ATOMIC_SET:
		/* This is broken in case of page faults and SMP ...
		   Risc/OS fauls after maximum 20 tries with EAGAIN.  */
		p = (int *) arg1;
		retval = verify_area(VERIFY_WRITE, p, sizeof(*p));
		if (retval)
			return retval;
		save_flags(flags);
		cli();
		retval = *p;
		*p = arg2;
		restore_flags(flags);
		break;

	case MIPS_FIXADE:
		tmp = current->tss.mflags & ~3;
		current->tss.mflags = tmp | (arg1 & 3);
		retval = 0;
		break;

	case FLUSH_CACHE:
		cacheflush(0, ~0, CF_BCACHE|CF_ALL);
		break;

	case MIPS_RDNVRAM:
		retval = -EIO;
		break;

	default:
		retval = -EINVAL;
		break;
	}

	return retval;
}

asmlinkage int
sys_cacheflush(void *addr, int nbytes, int cache)
{
	unsigned int rw;
	int ok;

	if ((cache & ~(DCACHE | ICACHE)) != 0)
		return -EINVAL;
	rw = (cache & DCACHE) ? VERIFY_WRITE : VERIFY_READ;
	if (!access_ok(rw, addr, nbytes))
		return -EFAULT;

	cacheflush((unsigned long)addr, (unsigned long)nbytes, cache|CF_ALL);

	return 0;
}

/*
 * No implemented yet ...
 */
asmlinkage int
sys_cachectl(char *addr, int nbytes, int op)
{
	return -ENOSYS;
}
