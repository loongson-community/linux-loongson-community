/*
 * file.c
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang.
 */

#include <linux/efs_fs.h>

static struct file_operations efs_file_operations = {
	NULL,			/* lseek */
	generic_file_read,
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	NULL,			/* ioctl */
	generic_file_mmap,
	NULL,			/* flush */
	NULL,			/* no special release code */
	NULL			/* fsync */
};

struct inode_operations efs_file_inode_operations = {
	&efs_file_operations,
	NULL,		/* create */
	NULL,		/* lookup */
	NULL,		/* link */
	NULL,		/* unlink */
	NULL,		/* symlink */
	NULL,		/* mkdir */
	NULL,		/* rmdir */
	NULL,		/* mknod */
	NULL,		/* rename */
	NULL,		/* readlink */
	NULL,		/* follow_link */
	generic_readpage,
	NULL,		/* writepage */
	efs_bmap,
	NULL,		/* truncate */
	NULL		/* permission */
};
 
int efs_bmap(struct inode *inode, efs_block_t block) {

	if (block < 0) {
		printk("EFS: efs_bmap(): block < 0\n");
		return 0;
	}

	/* are we about to read past the end of a file ? */
	if (block > inode->i_blocks) {
#ifdef DEBUG
		/* dunno why this happens */
		printk("EFS: efs_bmap(): block %d > last block %ld (filesize %ld)\n",
			block,
			inode->i_blocks,
			inode->i_size);
#endif
		return 0;
	}

	return efs_read_block(inode, block);
}

