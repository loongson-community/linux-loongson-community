/*
 *   linux/include/linux/efs_fs_i.h
 *
 * Copyright (C) 1997
 * Mike Shaver (shaver@neon.ingenia.ca)
 *
 * Based on work Copyright (C) 1995, 1996 Christian Vogelgsang.
 *
 * $Id: efs_fs_i.h,v 1.1 1997/12/02 02:28:29 ralf Exp $
 */

#ifndef __LINUX_EFS_FS_I_H
#define __LINUX_EFS_FS_I_H

#include <linux/efs_fs.h>

/* private Inode part */
struct efs_inode_info {
  __u32  extblk;
  
  __u16 tot;
  __u16 cur;

  union efs_extent  extents[EFS_MAX_EXTENTS];
};

#endif /* __LINUX_EFS_FS_I_H */
