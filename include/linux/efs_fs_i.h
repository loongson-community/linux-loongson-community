/*
 *   linux/include/linux/efs_fs_i.h
 *
 * Copyright (C) 1997
 * Mike Shaver (shaver@neon.ingenia.ca)
 *
 * Based on work Copyright (C) 1995, 1996 Christian Vogelgsang.
 *
 * $Id: efs_fs_i.h,v 1.1 1997/09/16 20:51:18 shaver Exp $
 */

#ifndef __LINUX_EFS_FS_I_H
#define __LINUX_EFS_FS_I_H

#include <linux/efs_fs.h>

/* private Inode part */
struct efs_inode_info {
  __u16 efs_total;
  __u16 efs_current;
  union efs_extent  *efs_extents;
};

#endif /* __LINUX_EFS_FS_I_H */
