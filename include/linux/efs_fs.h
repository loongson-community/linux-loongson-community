/*
 *   linux/include/linux/efs_fs.h
 *
 * Copyright (C) 1997, 1998 Mike Shaver (shaver@netscape.com)
 *
 * Based on work Copyright (C) 1995, 1996 Christian Vogelgsang.
 *
 * $Id: efs_fs.h,v 1.4 1998/08/25 09:22:46 ralf Exp $
 */

#ifndef __LINUX_EFS_FS_H
#define __LINUX_EFS_FS_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/stat.h>

/* SuperMagic: need a unique Identifier for EFS */
#define EFS_SUPER_MAGIC	    0x280273
#define EFS_ROOT_INODE	    2
#define EFS_BLK_VOLDESC     0
#define EFS_BLK_SUPER	    1

#define EFS_BLOCK_SIZE	    512
#define EFS_BLOCK_SIZE_BITS 9

/* EFS Magic IDs */
#define EFS_MAGIC1	    0x72959
#define EFS_MAGIC2	    0x7295a

/* Offsets in VolumeDescriptor */
#define EFS_VD_FS_START     0x190  /* First FS block */
#define EFS_VD_ENTRYFIRST   0x48   /* Begin of Entry list */
#define EFS_VD_ENTRYPOS     8      /* offset for the entry position */
#define EFS_VD_ENTRYLEN     12     /* offset for the entry length */
#define EFS_VD_ENTRYSIZE    16     /* length of an entry */

/* Offsets in Superblock */
#define EFS_SB_TOTAL	    0	   /* Number of Blocks used for filesystem */
#define EFS_SB_FIRST	    4	   /* BB of Begin First Cylinder Group */
#define EFS_SB_GROUP	    8	   /* BBs per Group */
#define EFS_SB_INODE	    12	   /* BBs used for Inodes at  begin of group */
#define EFS_SB_TOGRP	    18	   /* Number of Groups in Filesystem */
#define EFS_SB_MAGIC	    28

struct efs_disk_sb {
	__u32	s_size;
	__u32	s_firstcg;
	__u32	s_cgfsize;
	__u16	s_cgisize;
	__u16	s_sectors;
	__u16	s_heads;
	__u16	s_ncg;
	__u16	s_dirty;
	__u32	s_time;
	__u32	s_magic;
	char	s_fname[6];
	char	s_fpack[6];
	__u32	s_bmsize;
	__u32	s_tfree;
	__u32	s_tinode;
	__u32	s_bmblock;
	__u32	s_replsb;
	__u32	s_lastialloc;
	char	s_spare[20];	/* Must be zero */
	__u32	s_checksum;
};

#define EFS_CLEAN		0x0000	/* clean, not mounted */
#define EFS_ACTIVE		0x7777  /* clean, mounted */
#define EFS_ACTIVEDIRT		0x0BAD	/* dirty when mounted */
#define EFS_DIRTY		0x1234  /* mounted, then made dirty */

#ifdef DEBUG
void efs_dump_super(struct efs_super_block *);
#endif

#define EFS_MAX_EXTENTS          12

/* odev is used by "pre-extended-dev_t" IRIX EFS implementations, and
   later ones use odev and ndev together */

struct efs_devs {
	__u16	odev;
	__u32	ndev;
};

union efs_extent {
	struct {
		__u32	ex_magic:4,	/* must be zero */
			ex_bn:24;
		__u32	ex_length:8,
			ex_offset;
	} ex_ex;
	__u32	ex_bytes[2];
};

#define EFS_INODES_PER_BLOCK	4

struct efs_disk_inode {
	__u16	di_mode;
	__u16	di_nlink;
	__u16	di_uid;
	__u16	di_gid;
	__u32	di_size;
	__u32	di_atime;
	__u32	di_mtime;
	__u32	di_ctime;
	__u32	di_gen;
	__u16	di_numextents;
	__u8	di_version;
	__u8	di_spare;
	union di_addr {
		union efs_extent	di_extents[EFS_MAX_EXTENTS];
		struct efs_devs		di_dev;
	} di_u;
};

/* Offsets in DirBlock */
#define EFS_DB_ENTRIES      3
#define EFS_DB_FIRST        4
#define EFS_DIRBLK_MAGIC	0xBEEF
#define EFS_DIRBLK_HEADERSIZE	4
#define EFS_DIRBLK_SIZE		EFS_BLOCK_SIZE

struct efs_dirblk {
	__u16	db_magic;	/* 0xBEEF */
	__u8	db_firstused;
	__u8	db_slots;
	__u8	db_space[EFS_DIRBLK_SIZE - EFS_DIRBLK_HEADERSIZE];
};

/* Offsets in DirItem */
#define EFS_DI_NAMELEN      4
#define EFS_DI_NAME         5

struct efs_dir_entry {
	union {
		__u32	l;
		__u16	s[2];
	} ud_inum;
	__u8	d_namelen;
	__u8	d_name[3];
};

#define EFS_EXT_PER_BLK_BITS     5
#define EFS_EXT_PER_BLK_MASK     63
#define EFS_EXT_SIZE_BITS        3

#define EFS_SLOT2OFF(dirblk, slot) \
    (((dirblk)->db_space[(slot)]) << 1)

#define EFS_DENT4OFF(dirblk, offset) \
    ((struct efs_dir_entry *)((char *)(dirblk) + (offset)))

/* define a few convenient types */
#ifdef __KERNEL__

#include <linux/fs.h>

#ifdef DEBUG_EFS
#define DB(x)	printk##x
#else
#define DB(x)   do { } while(0);
#endif

extern int
efs_lookup(struct inode *dir, struct dentry *dentry);

extern int
efs_bmap(struct inode *inode, int block);

extern struct buffer_head *
efs_bread(struct inode * inode, int block, int create);

extern void
efs_put_super(struct super_block *sb);

extern struct super_block *
efs_read_super(struct super_block *sb, void *data, int silent);

extern void
efs_read_inode(struct inode *inode);

extern int
efs_remount(struct super_block *sb, int *flags, char *data);

extern int
efs_statfs(struct super_block *sb, struct statfs *buf, int bufsiz);


/* Byte swapping 32/16-bit quantities into little endian format. */
#define efs_need_swab 0
/* extern int efs_need_swab; */

extern __inline__ __u32 efs_swab32(__u32 value)
{
	return (efs_need_swab ? ((value >> 24) |
				((value >> 8) & 0xff00) |
				((value << 8) & 0xff0000) |
				 (value << 24)) : value);
}

extern __inline__ __u16 efs_swab16(__u16 value)
{
	return (efs_need_swab ? ((value >> 8) |
				 (value << 8)) : value);
}

int init_efs_fs(void);

/* Inode Method structures for Dirs, Files and Symlinks */
extern struct inode_operations efs_dir_inode_operations;
extern struct inode_operations efs_file_inode_operations;
extern struct inode_operations efs_symlink_inode_operations;
extern struct dentry_operations efs_dentry_operations;

#endif /* __KERNEL__ */

#endif /* __LINUX_EFS_FS_H */
