/*
 * dir.c
 *
 * Copyright (c) 1999 Al Smith
 */

#include <linux/efs_fs.h>

static int efs_readdir(struct file *, void *, filldir_t);

static struct file_operations efs_dir_operations = {
	NULL,		/* lseek */
	NULL,		/* read */
	NULL,		/* write */
	efs_readdir,
	NULL,		/* poll */
	NULL,		/* ioctl */
	NULL,		/* no special open code */
	NULL,		/* flush */
	NULL,		/* no special release code */
	NULL		/* fsync */
};

struct inode_operations efs_dir_inode_operations = {
	&efs_dir_operations,
	NULL,		/* create */
	efs_lookup,
	NULL,		/* link */
	NULL,		/* unlink */
	NULL,		/* symlink */
	NULL,		/* mkdir */
	NULL,		/* rmdir */
	NULL,		/* mknod */
	NULL,		/* rename */
	NULL,		/* readlink */
	NULL,		/* follow_link */
	NULL,		/* readpage */
	NULL,		/* writepage */
	efs_bmap,
	NULL,		/* truncate */
	NULL		/* permission */
};

/* read the next entry for a given directory */

static int efs_readdir(struct file *filp, void *dirent, filldir_t filldir) {
	struct inode *inode = filp->f_dentry->d_inode;
	struct efs_inode_info *ini = (struct efs_inode_info *) &inode->u.generic_ip;
	struct buffer_head *bh;

	struct efs_dir		*dirblock;
	struct efs_dentry	*dirslot;
	efs_ino_t		inodenum;
	efs_block_t		block;
	int			slot, namelen;
	char			*nameptr;

	if (!inode || !S_ISDIR(inode->i_mode))
		return -EBADF;

	if (ini->numextents != 1)
		printk("EFS: WARNING: readdir(): more than one extent\n");

	if (inode->i_size & (EFS_BLOCKSIZE-1))
		printk("EFS: WARNING: readdir(): directory size not a multiple of EFS_BLOCKSIZE\n");

	/* work out where this entry can be found */
	block = filp->f_pos >> EFS_BLOCKSIZE_BITS;
	slot  = filp->f_pos & 0xff;

	/* look at all blocks */
	while (block <= inode->i_blocks) {
		/* read the dir block */
		bh = bread(inode->i_dev, efs_bmap(inode, block), EFS_BLOCKSIZE);

		if (!bh) {
			printk("EFS: readdir(): failed to read dir block %d\n", block);
			break;
		}

		dirblock = (struct efs_dir *) bh->b_data; 

		if (be16_to_cpu(dirblock->magic) != EFS_DIRBLK_MAGIC) {
			printk("EFS: readdir(): invalid directory block\n");
			brelse(bh);
			break;
		}

		while(slot < dirblock->slots) {
			dirslot  = (struct efs_dentry *) (((char *) bh->b_data) + EFS_SLOTAT(dirblock, slot));

			inodenum = be32_to_cpu(dirslot->inode);
			namelen  = dirslot->namelen;
			nameptr  = dirslot->name;

#ifdef DEBUG
			printk("EFS: block %d slot %d/%d: inode %u, name \"%s\", namelen %u\n", block, slot, numslots, inodenum, nameptr, namelen);
#endif
			if (namelen > 0) {
				/* found the next entry */
				filp->f_pos = (block << EFS_BLOCKSIZE_BITS) | slot;

				/* copy filename and data in dirslot */
				filldir(dirent, nameptr, namelen, filp->f_pos, inodenum);

				/* store position of next slot */
				if (++slot == dirblock->slots) {
					slot = 0;
					block++;
				}
				brelse(bh);
				filp->f_pos = (block << EFS_BLOCKSIZE_BITS) | slot;
				return 0;
			}
			slot++;
		}
		brelse(bh);

		slot = 0;
		block++;
	}

	filp->f_pos = (block << EFS_BLOCKSIZE_BITS) | slot;
	return 0;
}

