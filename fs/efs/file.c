/*
 * linux/fs/efs/file.c
 *
 * Copyright (C) 1998 Mike Shaver
 *
 * Portions derived from work (C) 1995,1996 Christian Vogelgsang.
 * ``Inspired by'' fs/minix/file.c.
 */

#include <linux/efs_fs.h>

static struct file_operations efs_file_operations = {
    NULL,			/* lseek */
    generic_file_read,
    NULL,			/* write */
    NULL,			/* readdir */
    NULL,			/* poll */
    NULL,			/* ioctl */
    generic_file_mmap,		/* mmap */
    NULL,			/* open */
    NULL,			/* flush */
    NULL,			/* release */
    NULL			/* fsync */
};

struct inode_operations efs_file_inode_operations = {
    &efs_file_operations,	/* default file ops */
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* rename */
    NULL,			/* readlink */
    NULL,			/* follow_link */
    generic_readpage,		/* readpage */
    NULL,			/* writepage */
    efs_bmap,			/* bmap */
    NULL,			/* truncate */
    NULL			/* permission */
};

static inline __u32
check_extent(union efs_extent *ext, int blk, int fs_start)
{
    int eblk, epos, elen;

    eblk = ext->ex_bytes[0];	/* start disk block of the extent */
    epos = ext->ex_bytes[1] & 0xFFFFFF;	/* starting logical block */
    elen = ext->ex_bytes[1] >> 24; /* length of the extent */

    if ( (blk >= epos) && (blk < epos+elen) )
	return blk - epos + eblk + fs_start;
    return 0;
}

#define CHECK(index)   (check_extent(&ini->extents[index], blk, sbi->fs_start))

static __u32
efs_getblk(struct inode *in, __u32 blk)
{
    struct efs_sb_info *sbi = &in->i_sb->u.efs_sb;
    struct efs_inode_info *ini = &in->u.efs_i;
    int iter;

    __u32 diskblk, total = ini->tot, current = ini->cur;

    if (total <= EFS_MAX_EXTENTS) {
	diskblk = CHECK(current);
	if (diskblk)
	    return diskblk;

	if (total == 1)
	    return 0;

	/*
	 * OPT: start with current and then wrap, to optimize for
	 * read-forward pattern.
	 */
	for (iter = 0; iter < total; iter++) {
	    if (iter == current)
		continue;	/* already checked the current extent */
	    diskblk = CHECK(iter);
	    if (diskblk) {
		ini->cur = iter;
		DB(("EFS: inode %d: found block %d as %d in ext %d\n",
		    in->i_ino, blk, diskblk, iter));
		return diskblk;
	    }
	}

	DB(("EFS: block %d not found in direct inode %d (size %d)\n",
	    blk, in->i_ino, in->i_size));
	return 0;

    } else {
	int indirext = 0, total_extents_checked = 0;
    
	/* indirect inode */
	DB(("EFS: inode %d is indirect (total %d, indir ", in->i_ino, total));
	total = ini->extents[0].ex_ex.ex_offset;
	DB(("%d)\n", total));

	for (indirext = 0; indirext < total; indirext++) {
	    struct buffer_head *extbh;
	    union efs_extent *ptr;
	    int indirblk, indirbn, indirlen;
	    /*
	     * OPT: copy the current direct extent into the inode info for a
	     * quick check before we start reading blocks.
	     * OPT: copy _10_ into the inode info, like the old code did.
	     */
	    
	    indirbn  = ini->extents[indirext].ex_ex.ex_bn;
	    indirlen = ini->extents[indirext].ex_ex.ex_length;
	    for (indirblk = indirbn;
		 indirblk < indirbn + indirlen;
		 indirblk++) {
		extbh = bread(in->i_dev, indirblk, EFS_BLOCK_SIZE);
		if (!extbh) {
		    printk("EFS: ERROR: inode %ld bread of extent block %d failed\n",
			   in->i_ino, indirblk);
		    return 0;
		}

		for (ptr = (union efs_extent *)extbh->b_data;
		     ptr < (union efs_extent *)(extbh->b_data+EFS_BLOCK_SIZE);
		     ptr++, ++total_extents_checked) {
			
		    diskblk = check_extent(ptr, blk, sbi->fs_start);
		    if (diskblk || total_extents_checked > ini->tot) {
			brelse(extbh);
			return diskblk;
		    }
		}

		brelse(extbh);
	    }
	}
	DB(("EFS: inode %d: didn't find block %d (indirect search, size %d)\n",
	    in->i_ino, in->i_size));
	return 0;
    }
}

int efs_bmap(struct inode *in, int block)
{
    if (block < 0)
	return 0;
    
    /*
     * the kernel wants a full page (== 4K == 8 EFS blocks), so be sure that
     * the block number isn't too large for that.
     */
    if (block > ((in->i_size - 1) >> EFS_BLOCK_SIZE_BITS)) {
	DB(("EFS: wacky: block %d > max %d\n", block,
	    ((in->i_size - 1) >> EFS_BLOCK_SIZE_BITS)));
	return 0;
    }

    return efs_getblk(in, block);
}
