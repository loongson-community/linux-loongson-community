/*
 * efs.h
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang.
 */

#ifndef __EFS_H__
#define __EFS_H__

#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/locks.h>
#include <linux/malloc.h>
#include <linux/errno.h>
#include <linux/cdrom.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>

#include "efs_vh.h"
#include "efs_super.h"
#include "efs_inode.h"
#include "efs_dir.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/* private inode storage */
struct efs_in_info {
	int		numextents;
	int		lastextent;

	efs_extent	extents[EFS_DIRECTEXTENTS];
};

extern struct inode_operations efs_dir_inode_operations;
extern struct inode_operations efs_file_inode_operations;
extern struct inode_operations efs_symlink_inode_operations;

extern int init_module(void);
extern void cleanup_module(void);
extern struct super_block *efs_read_super(struct super_block *, void *, int);
extern void efs_put_super(struct super_block *);
extern int efs_statfs(struct super_block *, struct statfs *, int);

extern void efs_read_inode(struct inode *);
extern efs_block_t efs_read_block(struct inode *, efs_block_t);

extern int efs_lookup(struct inode *, struct dentry *);
extern int efs_bmap(struct inode *, int);

#endif /* __EFS_H__ */

