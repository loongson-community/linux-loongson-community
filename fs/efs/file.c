/* file.c
   
   read files on EFS filesystems
   now replaced by generic functions of the kernel:
   leaves only mapping of file block number -> disk block number in this file
   
   (C)95,96 Christian Vogelgsang
*/

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/efs_fs.h>
#include <linux/efs_fs_i.h>
#include <linux/efs_fs_sb.h>
#include <linux/stat.h>
#include <asm/uaccess.h>

static struct file_operations efs_file_ops = {
  NULL,
  generic_file_read,
  NULL,
  NULL,
  NULL,
  NULL,
  generic_file_mmap,
  NULL,
  NULL,
  NULL
};

struct inode_operations efs_file_in_ops = {
  &efs_file_ops,
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
  generic_readpage,
  NULL,
  efs_bmap,
  NULL,
  NULL
};

#ifndef MIN 
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/* ----- efs_getblknum -----
   find the disc block number for a given logical file block number
   
   in       - inode of file
   blk      - logical file block number
   
   return   - 0 on error, or unmapped block number
*/
static __u32 efs_getblk(struct inode *in,__u32 blk)
{
  struct efs_sb_info *sbi = &in->i_sb->u.efs_sb;
  struct efs_inode_info *ini = &in->u.efs_i;

  __u32 result = 0;
  __u32 eblk,epos,elen; /* used by the CHECK macro */
  __u16 pos;


  /*
   * This routine is a linear search of the extents, from zero.
   * A smarter implementation would:
   * - check the current extent first
   * - continue from the current extent forward, and then wrap
   */
  
#define CHECK(num) \
  eblk = ini->efs_extents[num].ex_bytes[0]; \
  epos = ini->efs_extents[num].ex_bytes[1] & 0xffffff; \
  elen = ini->efs_extents[num].ex_bytes[1] >> 24; \
  if((blk >= epos)&&(blk < (epos+elen))) \
    result = (blk - epos) + eblk + sbi->fs_start;

  for(pos=0;pos < ini->efs_total; pos++) {
    CHECK(pos)
    if(result) {
	/* ini->efs_current = pos; */
#ifdef DEBUG_EFS_EXTENTS_VERBOSE
	printk("EFS: inode %#0lx blk %d is phys blk %d\n",
	       in->i_ino, blk, result);
#endif
	return result;
    }
  }

  printk("EFS: didn't find physical blk for logical blk %d (inode %0#lx)!\n",
	 blk, in->i_ino);
  return 0;

}  


/* ----- efs_bmap ----- 
   bmap: map a file block number to a device block number
   
   in    - inode owning the block
   block - block number
   
   return - disk block, 0 on error
*/
int efs_bmap(struct inode *in, int block)
{
  /* quickly reject invalid block numbers */
  if(block<0) {
#ifdef DEBUG_EFS_BMAP
    printk("efs_bmap: block < 0\n");
#endif
    return 0;
  }
  /* since the kernel wants to read a full page of data, i.e. 8 blocks
     we must check if the block number is not too large */
  if(block>((in->i_size-1)>>EFS_BLOCK_SIZE_BITS)) {
#ifdef DEBUG_EFS_BMAP
    printk("efs_bmap: block %d > max %d == %d\n",
	   block,in->i_size>>EFS_BLOCK_SIZE_BITS,in->i_blocks);
#endif
    return 0;
  }

  return efs_getblk(in,block);
}
