/*
 * linux/fs/efs/dir.c
 *
 * Copyright (C) 1998  Mike Shaver
 *
 * Portions derived from work (C) 1995,1996 Christian Vogelgsang.
 * ``Inspired by'' fs/minix/dir.c.
 */

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/efs_fs.h>
#include <asm/uaccess.h>

static int efs_readdir(struct file *, void *, filldir_t);
int efs_lookup(struct inode *, struct dentry *);

static ssize_t
efs_dir_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
    return -EISDIR;
}

static struct file_operations efs_dir_ops = {
    NULL,			/* lseek */
    efs_dir_read,
    NULL,			/* write */
    efs_readdir,
    NULL,			/* poll */
    NULL,			/* ioctl */
    NULL,			/* mmap */
    NULL,			/* open */
    NULL,			/* flush */
    NULL,			/* release */
    NULL			/* fsync */
};

struct inode_operations efs_dir_inode_operations = {
    &efs_dir_ops,
    NULL,			/* create */
    efs_lookup,
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* rename */
    NULL,			/* readlink */
    NULL,			/* follow_link */
    NULL,			/* readpage */
    NULL,			/* writepage */
    efs_bmap,
    NULL,			/* truncate */
    NULL			/* permission */
};

static int
efs_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
    struct inode *in = filp->f_dentry->d_inode;
    struct efs_inode_info *ini = &in->u.efs_i;
    struct buffer_head *bh;
    __u16 item;
    __u32 block;
    __u16 offset;
    struct efs_dirblk *dirblk;
    struct efs_dir_entry *entry;

    if (!in || !in->i_sb || !S_ISDIR(in->i_mode) || !ini->tot)
	return -EBADF;

    if (ini->tot > 1) {
	printk("EFS: ERROR: directory %s has %d extents\n",
	    filp->f_dentry->d_name.name, ini->tot);
	printk("EFS: ERROR: Mike is lazy, so this is NYI.\n");
	return 0;
    };

    if (in->i_size & (EFS_BLOCK_SIZE - 1))
	printk("EFS: readdir: dirsize %#lx not block multiple\n", in->i_size);

    /* filp->f_pos is (block << BLOCK_SIZE | item) */
    block = filp->f_pos >> EFS_BLOCK_SIZE_BITS;
    item  = filp->f_pos & 0xFF;
    
 start_block:
    if (block == (in->i_size >> EFS_BLOCK_SIZE_BITS))
	return 0;		/* all done! */

    bh = bread(in->i_dev, efs_bmap(in, block), EFS_BLOCK_SIZE);
    if (!bh) {
	printk("EFS: ERROR: readdir: bread of %#lx/%#x\n",
	       in->i_ino, efs_bmap(in, block));
	return 0;
    }

    dirblk = (struct efs_dirblk *)bh->b_data;

    /* skip empty slots */
    do {
	offset = EFS_SLOT2OFF(dirblk, item);
	if (!offset) {
	    DB(("EFS: skipping empty slot %d\n", item));
	}
	item++;
	if (item == dirblk->db_slots) {
	    item = 0;
	    block++;
	    if (!offset) {
		brelse(bh);
		goto start_block;
	    }
	}
    } while(!offset);

    entry = EFS_DENT4OFF(dirblk, offset);
    /*
    DB(("EFS_SLOT2OFF(%d) -> %d, EFS_DENT4OFF(%p, %d) -> %p) || ",
	item-1, offset, dirblk, offset, entry));
#ifdef DEBUG_EFS
    {
	__u8 *rawdirblk, nameptr;
	__u32 iteminode;
	__u16 namelen, rawdepos;
	rawdirblk = (__u8*)bh->b_data;
	rawdepos = (__u16)rawdirblk[EFS_DB_FIRST+item-1] << 1;
	DB(("OLD_WAY: offset = %d, dent = %p ||", rawdepos,
	    (struct efs_dir_entry *)(rawdirblk + rawdepos)));
    }
#endif

    DB(("EFS: filldir(dirent, \"%.*s\", %d, %d, %d)\n",
	entry->d_namelen, entry->d_name, entry->d_namelen,
	filp->f_pos, efs_swab32(entry->ud_inum.l)));
    */
    filldir(dirent, entry->d_name, entry->d_namelen, filp->f_pos,
	    efs_swab32(entry->ud_inum.l));
    
    brelse(bh);

    filp->f_pos = (block << EFS_BLOCK_SIZE_BITS) | item;
    UPDATE_ATIME(in);

    return 0;
}
