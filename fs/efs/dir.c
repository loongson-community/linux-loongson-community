/* dir.c
   
   directory inode operations for EFS filesystem
   
   (C)95,96 Christian Vogelgsang
*/

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/efs_fs.h>
#include <linux/efs_fs_i.h>
#include <linux/stat.h>
#include <asm/uaccess.h>

static int efs_readdir(struct file *,void *,filldir_t);
extern int efs_lookup(struct inode *, struct dentry *);

static struct file_operations efs_dir_ops = {
	NULL, 
	NULL, 
	NULL,
	efs_readdir,
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL
};

struct inode_operations efs_dir_in_ops = {
	&efs_dir_ops,
	NULL,
	efs_lookup,
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	NULL,
	efs_bmap,
	NULL,
	NULL
};


/* ----- efs_readdir -----
   readdir inode operation:
   read the next directory entry of a given dir file
   
   inode     - pointer to inode struct of directory
   filp      - pointer to file struct of directory inode
   dirent    - pointer to dirent struct that has to be filled
   filldir   - function to store values in the directory

   return    - 0 ok, <0 error
*/
static int efs_readdir(struct file *filp,
		       void *dirent, filldir_t filldir)
{
	struct inode *inode = filp->f_dentry->d_inode;
	struct efs_inode_info *ini = (struct efs_inode_info *)&inode->u.efs_i;
	struct buffer_head *bh;
	__u8  *rawdirblk;
	__u32  iteminode;
	__u16 namelen;
	__u8  *nameptr;
	__u32  numitems;
	__u16 itemnum;
	__u32  block;
	__u16 rawdepos;  

	/* some checks */
	if(!inode || !inode->i_sb || !S_ISDIR(inode->i_mode)) 
		return -EBADF;
  
	/* Warnings */
	if(ini->tot!=1) {
		printk("EFS: directory %s has more than one extent.\n", 
		       filp->f_dentry->d_name.name);
		printk("EFS: Mike is lazy, so we can't handle this yet.  Sorry =(\n");
		return 0;
	}
	if(inode->i_size & (EFS_BLOCK_SIZE-1))
		printk("efs_readdir: dirsize != blocksize*n\n");

	/* f_pos contains: dirblock<<BLOCK_SIZE | # of item in dirblock */
	block   = filp->f_pos >> EFS_BLOCK_SIZE_BITS;
	itemnum = filp->f_pos & 0xff; 
  
	/* We found the last entry -> ready */
	if(block == (inode->i_size>>EFS_BLOCK_SIZE_BITS))
		return 0;

	/* get disc block number from dir block num: 0..i_size/BLOCK_SIZE */ 
	bh = bread(inode->i_dev,efs_bmap(inode,block),EFS_BLOCK_SIZE);
	if(!bh) return 0;

	/* dirblock */
	rawdirblk = (__u8 *)bh->b_data; 
	/* number of entries stored in this dirblock */
	numitems = rawdirblk[EFS_DB_ENTRIES]; 
	/* offset in block of #off diritem */
	rawdepos = (__u16)rawdirblk[EFS_DB_FIRST+itemnum]<<1;

	/* diritem first contains the inode number, the namelen and the name */
	iteminode = ConvertLong(rawdirblk,rawdepos);
	namelen = (__u16)rawdirblk[rawdepos+EFS_DI_NAMELEN];
	nameptr = rawdirblk + rawdepos + EFS_DI_NAME;  

#ifdef DEBUG_EFS
	printk("efs: dir #%d @ %0#3x - inode %lx %s namelen %u\n",
	       itemnum,rawdepos,iteminode,nameptr,namelen);
#endif

	/* copy filename and data in direntry */
	filldir(dirent,nameptr,namelen,filp->f_pos,iteminode);

	brelse(bh);

  /* store pos of next item */
	itemnum++;
	if(itemnum==numitems) {
		itemnum = 0;
		block++;
	}
	filp->f_pos = (block<<EFS_BLOCK_SIZE_BITS) | itemnum;
	UPDATE_ATIME(inode);
  
	return 0;
}

