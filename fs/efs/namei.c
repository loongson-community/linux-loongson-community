/* namei.c
   
   name lookup for EFS filesystem
   
   (C)95,96 Christian Vogelgsang
*/

#include <linux/sched.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/efs_fs.h>
#include <linux/efs_fs_i.h>
#include <linux/efs_fs_sb.h>
#include <linux/stat.h>
#include <asm/uaccess.h>

/* ----- efs_find_entry -----
   search a raw efs dir entry for the given name
   
   dir     - inode of directory
   oname   - name to search for
   onamelen- length of name
   
   return  - inode number of the found entry or 0 on error
*/
static struct buffer_head * efs_find_entry(struct inode *dir,
					   const char *oname,
					   int onamelen, 
					   struct efs_dir_entry *res_dir)
{
	struct efs_inode_info *ini = &dir->u.efs_i;
	struct buffer_head *bh = NULL;
	__u32 inode;
	__u16 i;
	__u8 *name;
	__u16 namelen;
	__u32 blknum,b;
	struct efs_dirblk *db;
 
	/* Warnings */ 
	if(ini->tot!=1)
		printk("efs_find: More than one extent!\n");
	if(dir->i_size & (EFS_BLOCK_SIZE-1))
		printk("efs_find: dirsize != blocklen * n\n");

	/* Search in every dirblock */
	inode = 0;
	blknum = dir->i_size >> EFS_BLOCK_SIZE_BITS;
#ifdef DEBUG_EFS
	printk("EFS: directory with inode %#xd has %d blocks\n",
	       dir->i_ino, blknum);
#endif
	for(b=0;b<blknum;b++) {
		int db_offset;
#ifdef DEBUG_EFS
		printk("EFS: trying block %d\n", b);
#endif
		/* Read a raw dirblock */
		bh = bread(dir->i_dev,efs_bmap(dir,b),EFS_BLOCK_SIZE);
		if(!bh) {
			printk("EFS: efs_bmap returned NULL!\n");
			return 0;
		}
    
		db = (struct efs_dirblk *)bh->b_data;
		if (db->db_magic != EFS_DIRBLK_MAGIC) {
			printk("EFS: dirblk has bad magic (%#xl)!\n",
			       db->db_magic);
			return NULL;
		}
#ifdef DEBUG_EFS
		printk("EFS: db %d has %d entries, starting at offset %#x\n",
		       b, db->db_slots, (__u16)db->db_firstused << 1);
#endif
		for(i = 0 ; i < db->db_slots ; i++) {
			struct efs_dent * de;
			int entry_inode;
			db_offset = ((__u16)db->db_space[i] << 1) 
			  - EFS_DIRBLK_HEADERSIZE;
			de = (struct efs_dent *)(&db->db_space[db_offset]);

			/* inode, namelen and name of direntry */
			entry_inode = efs_swab32(de->ud_inum.l); 
			namelen = de->d_namelen; 
			name = de->d_name;
#ifdef 0
			printk("EFS: entry %d @ %#x has inode %#x, %s/%d\n",
			       i, db_offset, entry_inode, name, namelen);
#endif
			/* we found the name! */
			if((namelen==onamelen)&&
			   (!memcmp(oname,name,onamelen))) {
				res_dir->inode = entry_inode;
#ifdef DEBUG_EFS
				printk("EFS: found inode %d\n", 
				       entry_inode); 
#endif
				return bh;
			}
		}
	}
#ifdef DEBUG_EFS
	printk("EFS: efs_find_entry didn't find inode for \"%s\"/%d!\n",
	       oname, onamelen);
#endif
	return NULL;
	/* not reached */
}


/* ----- efs_lookup -----
   lookup inode operation:
   check if a given name is in the dir directory
   
   dir       - pointer to inode of directory
   name      - name we must search for
   len       - length of name
   result    - pointer to inode struct if we found it or NULL on error
   
   return    - 0 everything is ok or <0 on error
*/
int efs_lookup(struct inode *dir, struct dentry *dentry)
{
  struct inode *inode = NULL;
  struct buffer_head *bh;
  struct efs_dir_entry de;
  
  bh = efs_find_entry(dir, dentry->d_name.name, dentry->d_name.len, &de);
  if (bh) {
    int ino = de.inode;
    brelse(bh);
    inode = iget(dir->i_sb, ino);

    if (!inode) 
      return -EACCES;
  }
    
  d_add(dentry, inode);
  return 0;
}
