/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * ARC firmware interface.
 *
 * Copyright (C) 1994, 1995, 1996, 1999 Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#include <linux/init.h>
#include <asm/sgialib.h>

long __init prom_getvdirent(unsigned long fd, struct linux_vdirent *ent,
                            unsigned long num, unsigned long *cnt)
{
	return ARC_CALL4(get_vdirent, fd, ent, num, cnt);
}

long __init prom_open(char *name, enum linux_omode md, unsigned long *fd)
{
	return ARC_CALL3(open, name, md, fd);
}

long __init prom_close(unsigned long fd)
{
	return ARC_CALL1(close, fd);
}

long __init prom_read(unsigned long fd, void *buf, unsigned long num,
                      unsigned long *cnt)
{
	return ARC_CALL4(read, fd, buf, num, cnt);
}

long __init prom_getrstatus(unsigned long fd)
{
	return ARC_CALL1(get_rstatus, fd);
}

long __init prom_write(unsigned long fd, void *buf, unsigned long num,
                       unsigned long *cnt)
{
	return ARC_CALL4(write, fd, buf, num, cnt);
}

long __init prom_seek(unsigned long fd, struct linux_bigint *off,
                      enum linux_seekmode sm)
{
	return ARC_CALL3(seek, fd, off, sm);
}

long __init prom_mount(char *name, enum linux_mountops op)
{
	return ARC_CALL2(mount, name, op);
}

long __init prom_getfinfo(unsigned long fd, struct linux_finfo *buf)
{
	return ARC_CALL2(get_finfo, fd, buf);
}

long __init prom_setfinfo(unsigned long fd, unsigned long flags,
                          unsigned long msk)
{
	return ARC_CALL3(set_finfo, fd, flags, msk);
}
