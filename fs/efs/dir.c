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
static int
efs_readdir(struct file *filp,
	    void *dirent, filldir_t filldir)
{
    struct inode *inode = filp->f_dentry->d_inode;
    struct efs_inode_info *ini = (struct efs_inode_info *)&inode->u.efs_i;
    struct buffer_head *bh;
    struct efs_dirblk *dirblk;
    __u32  iteminode;
    __u16 namelen;
    char *nameptr;
    __u16 itemnum;
    __u32  block;
    int db_offset;
    int error = 0;
    struct efs_dent *de = NULL;
    
    /* some checks -- ISDIR should be done by upper layer */
    if(!inode || !inode->i_sb || !S_ISDIR(inode->i_mode)) {
	printk("EFS: bad inode for readdir!\n");
	return -EBADF;
    }
    
    if(ini->efs_total!=1) {
	printk("EFS: directory %s has more than one extent.\n", 
	       filp->f_dentry->d_name.name);
	printk("EFS: Mike is lazy, so we can't handle this yet.  Sorry =(\n");
	return -EBADF;
    }

    /* Warnings */
    if(inode->i_size & (EFS_BLOCK_SIZE-1)) 
	printk("efs_readdir: dirsize != blocksize*n\n");
    
    /* f_pos contains: dirblock<<BLOCK_SIZE | # of item in dirblock */
    block   = filp->f_pos >> EFS_BLOCK_SIZE_BITS;
    itemnum = filp->f_pos & 0xff; 
    
    /* We found the last entry -> ready */
    if(block == (inode->i_size>>EFS_BLOCK_SIZE_BITS)) {
	printk("EFS: read all entries -- done\n");
	return 0;
    }
    
    /* get disc block number from dir block num: 0..i_size/BLOCK_SIZE */ 
    bh = bread(inode->i_dev,efs_bmap(inode,block),EFS_BLOCK_SIZE);
    if(!bh) {
	/* XXX try next block? */
	return -EBADF;
    }

    /* from here on, we must exit through out: to release bh */
    
    /* dirblock */
    dirblk = (struct efs_dirblk *)bh->b_data;
    if (dirblk->db_magic != EFS_DIRBLK_MAGIC) {
	printk("EFS: bad dirblk magic %#xl\n", dirblk->db_magic);
	error = -EBADF;
	goto out;
    }
    db_offset = ((__u16)dirblk->db_space[itemnum] << 1) 
	- EFS_DIRBLK_HEADERSIZE;
    de = (struct efs_dent *)(&dirblk->db_space[db_offset]);
    if (dirblk->db_slots == 0) {
	error = 0;
	goto out;
    }

#ifdef DEBUG_EFS_DIRS
    if (itemnum == 0) {
	printk("efs: readdir: db %d has %d entries, starting at offset %0#x\n",
	       block, dirblk->db_slots, (__u16)dirblk->db_firstused << 1);
    }
#endif
    
    /* diritem first contains the inode number, the namelen and the name */
    iteminode = EFS_DE_GET_INUM(de);
    namelen = de->d_namelen;
    nameptr = de->d_name;

#ifdef DEBUG_EFS_DIRS
    printk("efs: dir #%d @ %0#x - inode %#0x %*s namelen %u\n",
	   itemnum,db_offset,iteminode, namelen, 
	   nameptr ? nameptr : "(null)", namelen);
#endif

    /* copy filename and data in direntry */
    error = filldir(dirent,nameptr,namelen,filp->f_pos,iteminode);
    if (error)
	goto out;

    /* store pos of next item */
    itemnum++;
    if(itemnum == dirblk->db_slots) {
	itemnum = 0;
	block++;
    }
    filp->f_pos = (block<<EFS_BLOCK_SIZE_BITS) | itemnum;
    UPDATE_ATIME(inode);
    
 out:    
    brelse(bh);

    return error;
}

