#ifndef _ASM_COMPAT_H
#define _ASM_COMPAT_H
/*
 * Architecture specific compatibility types
 */
#include <linux/types.h>

#define COMPAT_USER_HZ	100

typedef u32		compat_size_t;
typedef s32		compat_ssize_t;
typedef s32		compat_time_t;
typedef s32		compat_clock_t;
typedef s32		compat_suseconds_t;

struct compat_timespec {
	compat_time_t	tv_sec;
	s32		tv_nsec;
};

struct compat_timeval {
	compat_time_t	tv_sec;
	s32		tv_usec;
};

struct compat_stat {
	__kernel_dev_t32	st_dev;
	int			st_pad1[3];
	__kernel_ino_t32	st_ino;
	__kernel_mode_t32	st_mode;
	__kernel_nlink_t32	st_nlink;
	__kernel_uid_t32	st_uid;
	__kernel_gid_t32	st_gid;
	__kernel_dev_t32	st_rdev;
	int			st_pad2[2];
	__kernel_off_t32	st_size;
	int			st_pad3;
	compat_time_t		st_atime;
	int			st_atime_nsec;
	compat_time_t		st_mtime;
	int			st_mtime_nsec;
	compat_time_t		st_ctime;
	int			st_ctime_nsec;
	int			st_blksize;
	int			st_blocks;
	int			st_pad4[14];
};

#endif /* _ASM_COMPAT_H */
