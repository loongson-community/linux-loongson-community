/*
 *   linux/include/linux/efs_fs_sb.h
 *
 * Copyright (C) 1997
 * Mike Shaver (shaver@neon.ingenia.ca)
 *
 * Based on work Copyright (C) 1995, 1996 Christian Vogelgsang.
 *
 * $Id: efs_fs_sb.h,v 1.1 1997/12/02 02:28:29 ralf Exp $
 */

#ifndef __LINUX_EFS_FS_SB_H
#define __LINUX_EFS_FS_SB_H

#include <linux/efs_fs.h>

/* EFS Superblock Information */
struct efs_sb_info {
	__u32   fs_start;      /* first block of filesystem */
	__u32   total_blocks;  /* total number of blocks in filesystem */
	__u32   first_block;   /* first data block in filesystem */
	__u32 	group_size;    /* number of blocks a group consists of */ 
	__u16 	inode_blocks;  /* number of blocks used for inodes in 
				* every group */
	__u16   total_groups;  /* number of groups */
};


#endif /* __LINUX_UFS_FS_SB_H */
