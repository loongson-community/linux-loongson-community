/*
 * linux/fs/efs/symlink.c
 *
 * Copyright (C) 1998  Mike Shaver
 *
 * Portions derived from work (C) 1995,1996 Christian Vogelgsang.
 * ``Inspired by'' fs/ext2/symlink.c.
 */

#include <linux/efs_fs.h>
#include <asm/uaccess.h>

static struct dentry *
efs_follow_link(struct dentry *dentry, struct dentry *base,
		unsigned int follow)
{
    struct inode *in = dentry->d_inode;
    struct buffer_head *bh;

    bh = bread(in->i_dev, efs_bmap(in, 0), EFS_BLOCK_SIZE);
    if (!bh) {
	dput(base);
	return ERR_PTR(-EIO);
    }
    UPDATE_ATIME(in);
    base = lookup_dentry(bh->b_data, base, follow);
    brelse(bh);
    return base;
}

static int
efs_readlink(struct dentry *dentry, char * buffer, int buflen)
{
    int i;
    struct buffer_head *bh;
    struct inode *in = dentry->d_inode;
    
    if (buflen > 1023)
	buflen = 1023;

    if (in->i_size < buflen)
	buflen = in->i_size;

    bh = bread(in->i_dev, efs_bmap(in, 0), EFS_BLOCK_SIZE);
    if (!bh)
	return 0;
    i = 0;

    /* XXX need strncpy_to_user */
    while (i < buflen && bh->b_data[i])
	i++;

    if (copy_to_user(buffer, bh->b_data, i))
	i = -EFAULT;

    UPDATE_ATIME(in);

    brelse(bh);
    return i;
}

struct inode_operations efs_symlink_inode_operations = {
	NULL,			/* no file-operations */
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	efs_readlink,
	efs_follow_link,
	NULL,			/* readpage */
	NULL,			/* writepage */
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL,			/* permission */
	NULL			/* smap */
};
