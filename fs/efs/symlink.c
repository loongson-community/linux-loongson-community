/* symlink.c
 *
 * Symbolic link handling for EFS
 *
 * (C)1995,96 Christian Vogelgsang
 *
 * Based on the symlink.c from minix-fs by Linus
 */

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/efs_fs.h>
#include <linux/stat.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

static int efs_readlink(struct dentry *, char *, int);
static struct dentry * efs_follow_link(struct dentry *, struct dentry *);

struct inode_operations efs_symlink_in_ops = {
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
	efs_readlink,		/* readlink */
	efs_follow_link,	/* follow_link */
	NULL,
	NULL,
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL			/* permission */
};


/* ----- efs_getlinktarget -----
   read the target of the link from the data zone of the file
*/
static char *efs_getlinktarget(struct inode *in)
{
	struct buffer_head * bh;
	char *name;
	__u32 size = in->i_size;
	__u32 block;
  
#ifdef DEBUG_EFS_SYMLINK
	printk("efs_getlinktarget: size of inode %#0lx is %d\n", in->i_ino,
	       size);
#endif
	/* link data longer than 1024 not supported */
	if(size>2*EFS_BLOCK_SIZE) {
		printk("efs_getlinktarget: name too long: %lu\n",in->i_size);
		return NULL;
	}
  
	/* get some memory from the kernel to store the name */
	name = kmalloc(size+1,GFP_KERNEL);
	if(!name) return NULL;
  
	/* read first 512 bytes of target */
	block = efs_bmap(in,0);
	bh = bread(in->i_dev,block,EFS_BLOCK_SIZE);
	if(!bh) {
		printk("efs_getlinktarget: unable to read first block (%d)\n", block);
		kfree(name);
		return NULL;
	}
	memcpy(name,bh->b_data,(size>EFS_BLOCK_SIZE)?EFS_BLOCK_SIZE:size);
	brelse(bh);
  
	/* if the linktarget is long, read the next block */
	if(size>EFS_BLOCK_SIZE) {
		bh = bread(in->i_dev,block+1,EFS_BLOCK_SIZE);
		if(!bh) {
		  printk("efs_getlinktarget: unable to read second block (%d)\n", block);
		  kfree(name);
		  return NULL;
		}
		memcpy(name+EFS_BLOCK_SIZE,bh->b_data,size-EFS_BLOCK_SIZE);
		brelse(bh);
	}
  
	/* terminate string and return it */
	name[size]=0;
	return name;
}


/* ----- efs_follow_link -----
   get the inode of the link target
*/
static struct dentry * efs_follow_link(struct dentry * dent,
				       struct dentry *base)
{
  struct inode * dir = dent->d_inode;
	char * name;
	UPDATE_ATIME(dir);
	name = efs_getlinktarget(dir);
#ifdef DEBUG_EFS_SYMLINK
	printk("EFS: efs_getlinktarget(%d) returned \"%s\"\n",
	       dir->i_ino, name);
#endif
	base = lookup_dentry(name, base, 1);
	kfree(name);
	return base;
}

/* ----- efs_readlink -----
   read the target of a link and return the name
*/
static int efs_readlink(struct dentry * dent, char * buffer, int buflen)
{
	int i;
	struct buffer_head * bh;
	struct inode * dir = dent->d_inode;

	if (buflen > 1023)
		buflen = 1023;
	bh = bread(dir->i_dev,efs_bmap(dir,0),EFS_BLOCK_SIZE);
	if (!bh)
		return 0;
	/* copy the link target to the given buffer */
	i = 0;
#ifdef DEBUG_EFS_SYMLINK
	printk("EFS: efs_readlink returning ");
#endif
	while (i<buflen && bh->b_data[i] && i < dir->i_size) {
#ifdef DEBUG_EFS_SYMLINK
		printk("%c", bh->b_data[i]);
#endif
		i++;
	}
#ifdef DEBUG_EFS_SYMLINK
	printk("\n");
#endif
	if (copy_to_user(buffer, bh->b_data, i))
		i = -EFAULT;
	
	brelse(bh);
	return i;
}

