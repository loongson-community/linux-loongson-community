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
 
#define MIN(a,b) ((a)<(b)?(a):(b))

#define CHECK(num) \
  eblk = ini->extents[num].ex_bytes[0]; \
  epos = ini->extents[num].ex_bytes[1] & 0xffffff; \
  elen = ini->extents[num].ex_bytes[1] >> 24; \
  if((blk >= epos)&&(blk < (epos+elen))) \
    result = (blk - epos) + eblk + sbi->fs_start;


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
  struct buffer_head *bh;

  __u32 result = 0;
  __u32 eblk,epos,elen;
  int num,extnum,readahead;
  __u32 extblk;
  __u16 extoff,pos,cur,tot;
  union efs_extent *ptr;


  /* first check the current extend */
  cur = ini->cur;
  tot = ini->tot;
  CHECK(cur)
  if(result) 
    return result;
    
  /* if only one extent exists and we are here the test failed */
  if(tot==1) {
    printk("efs: bmap failed on one extent!\n");
    return 0;
  }

  /* check the stored extents in the inode */
  num = MIN(tot,EFS_MAX_EXTENTS);
  for(pos=0;pos<num;pos++) {
    /* don't check the current again! */
    if(pos==cur) 
      continue;

    CHECK(pos)
    if(result) {
      ini->cur = pos;
      return result;
    }
  }

  /* If the inode has only direct extents, 
     the above tests must have found the block's extend! */
  if(tot<=EFS_MAX_EXTENTS) {
    printk("efs: bmap failed for direct extents!\n");
    return 0;
  }  

  /* --- search in the indirect extensions list blocks --- */
#ifdef DEBUG
  printk("efs - indirect search for %lu\n",blk);
#endif

  /* calculate block and offset for begin of extent descr and read it */
  extblk = ini->extblk;
  extoff = 0;
  bh = bread(in->i_dev,extblk,EFS_BLOCK_SIZE);
  if(!bh) {
    printk("efs: read error in indirect extents\n");
    return 0;
  }
  ptr = (union efs_extent *)bh->b_data;

  pos = 0; /* number of extend store in the inode */
  extnum = 0; /* count the extents in the indirect blocks */ 
  readahead = 10; /* number of extents to read ahead */
  while(1) {

    /* skip last current extent store in the inode */
    if(pos==cur) pos++;

    /* read new extent in inode buffer */
    ini->extents[pos].ex_bytes[0] = efs_swab32(ptr[pos].ex_bytes[0]);
    ini->extents[pos].ex_bytes[1] = efs_swab32(ptr[pos].ex_bytes[1]);

    /* we must still search */ 
    if(!result) {
      CHECK(pos)
      if(result)
	ini->cur = pos;
    }
    /* we found it already and read ahead */
    else {
      readahead--;
      if(!readahead)
	break;
    }

    /* next storage place */
    pos++;
    extnum++;

    /* last extent checked -> finished */
    if(extnum==tot) {
      if(!result)
	printk("efs: bmap on indirect failed!\n");
      break;
    }

    extoff += 8;
    /* need new block */
    if(extoff==EFS_BLOCK_SIZE) {
      extoff = 0;
      extblk++;
	
      brelse(bh);
      bh = bread(in->i_dev,extblk,EFS_BLOCK_SIZE);
      if(!bh) {
	printk("efs: read error in indirect extents\n");
	return 0;
      }
      ptr = (union efs_extent *)bh->b_data;
    }
  }
  brelse(bh);

  return result;
}  


/* ----- efs_bmap ----- 
   bmap: map a file block number to a device block number
   
   in    - inode owning the block
   block - block number
   
   return - disk block
*/
int efs_bmap(struct inode *in, int block)
{
  /* quickly reject invalid block numbers */
  if(block<0) {
#ifdef DEBUG
    printk("efs_bmap: block < 0\n");
#endif
    return 0;
  }
  /* since the kernel wants to read a full page of data, i.e. 8 blocks
     we must check if the block number is not too large */
  if(block>((in->i_size-1)>>EFS_BLOCK_SIZE_BITS)) {
#ifdef DEBUG
    printk("efs_bmap: block %d > max %d == %d\n",
	   block,in->i_size>>EFS_BLOCK_SIZE_BITS,in->i_blocks);
#endif
    return 0;
  }

  return efs_getblk(in,block);
}
