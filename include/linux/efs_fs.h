/*
 * efs_fs.h
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang.
 */

#ifndef __EFS_FS_H__
#define __EFS_FS_H__

#define VERSION "0.97"

static const char cprt[] = "EFS: version "VERSION" - (c) 1999 Al Smith <Al.Smith@aeschi.ch.eu.org>";

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

#define	EFS_BLOCKSIZE		512
#define	EFS_BLOCKSIZE_BITS	9

#include <linux/efs_fs_i.h>
#include <linux/efs_dir.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/* efs superblock information in memory */
struct efs_spb {
	int32_t	fs_magic;	/* superblock magic number */
	int32_t	fs_start;	/* first block of filesystem */
	int32_t	first_block;	/* first data block in filesystem */
	int32_t	total_blocks;	/* total number of blocks in filesystem */
	int32_t	group_size;	/* # of blocks a group consists of */ 
	int32_t	data_free;	/* # of free data blocks */
	int32_t	inode_free;	/* # of free inodes */
	short	inode_blocks;	/* # of blocks used for inodes in every grp */
	short	total_groups;	/* # of groups */
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

extern int init_efs_fs(void);

#endif /* __EFS_FS_H__ */

