/*
 * linux/fs/efs/namei.c
 *
 * Copyright (C) 1998  Mike Shaver
 *
 * Portions derived from work (C) 1995,1996 Christian Vogelgsang.
 * ``Inspired by'' fs/minix/namei.c.
 */

#include <linux/efs_fs.h>
#include <linux/errno.h>

static struct buffer_head *
efs_find_entry(struct inode *dir, const char *oname, int onamelen,
	       struct efs_dir_entry **res_dir)
{
    struct buffer_head *bh;
    struct efs_sb_info *sbi;
    struct efs_dirblk *dirblk;
    __u32 offset, block, maxblk;
    __u16 i, namelen;
    char *name;

    *res_dir = NULL;
    if (!dir || !dir->i_sb)
	return NULL;
    sbi = &dir->i_sb->u.efs_sb;
    bh = NULL;
    block = offset = 0;
    maxblk = dir->i_size >> EFS_BLOCK_SIZE_BITS;
    DB(("EFS: dir has %d blocks\n", maxblk));
    for (block = 0; block < maxblk; block++) {

	bh = bread(dir->i_dev, efs_bmap(dir, block), EFS_BLOCK_SIZE);
	if (!bh) {
	    DB(("EFS: find_entry: skip blk %d (ino %#lx): bread\n",
		block, dir->i_ino));
	    continue;
	}

	dirblk = (struct efs_dirblk *)bh->b_data;

	if (efs_swab32(dirblk->db_magic) != EFS_DIRBLK_MAGIC) {
	    printk("EFS: dirblk %d (ino %#lx) has bad magic (%#x)!\n",
		   block, dir->i_ino, efs_swab32(dirblk->db_magic));
	    brelse(bh);
	    continue;
	}

	DB(("EFS: db %d has %d entries\n", block, dirblk->db_slots));
	
	for (i = 0; i < dirblk->db_slots; i++) {
	    struct efs_dir_entry *dent;
	    __u16 off = EFS_SLOT2OFF(dirblk, i);
	    if (!off) {
		DB(("skipping empty slot %d\n", i));
		continue;	/* skip empty slot */
	    }
	    dent = EFS_DENT4OFF(dirblk, off);
	    namelen = dent->d_namelen;
	    name = dent->d_name;

	    if ((namelen == onamelen) &&
		!memcmp(oname, name, onamelen)) {
		*res_dir = dent;
		return bh;
	    }
	}

	brelse(bh);
    }
    DB(("EFS: find_entry didn't find inode for \"%s\"/%d\n",
	oname, onamelen));
    return NULL;
}

int
efs_lookup(struct inode *dir, struct dentry *dentry)
{
    struct buffer_head *bh;
    struct inode *in = NULL;
    struct efs_dir_entry *dent;
    
    bh = efs_find_entry(dir, dentry->d_name.name, dentry->d_name.len, &dent);
    if (bh) {
	int ino = efs_swab32(dent->ud_inum.l);

	brelse(bh);
	in = iget(dir->i_sb, ino);
	if (!in)
	    return -EACCES;
    }
    
    d_add(dentry, in);
    return 0;
}
