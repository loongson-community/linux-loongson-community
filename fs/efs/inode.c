/* inode.c
 * 
 * Inode and Superblock handling for the EFS filesystem
 *
 * (C) 1995,96 Christian Vogelgsang
 */
 
#include <linux/module.h>
#include <linux/init.h>		/* __initfunc */
#include <linux/fs.h>
#include <linux/efs_fs.h>
#include <linux/efs_fs_i.h>
#include <linux/efs_fs_sb.h>
#include <linux/locks.h>
#include <linux/malloc.h>

#include <asm/uaccess.h>

/* ----- Define the operations for the Superblock ----- */
static void efs_read_inode(struct inode *);
static void efs_put_inode(struct inode *);
static void efs_put_super(struct super_block *);
static int efs_statfs(struct super_block *, struct statfs *,int );

static struct super_operations efs_sops = {
	efs_read_inode, 
	NULL, /* write_inode */
	efs_put_inode, /* put_inode */
	NULL, /* delete_inode */
	NULL, /* notify_change */
	efs_put_super, 
	NULL, /* write_super */
	efs_statfs, 
	NULL
};

static void efs_put_inode(struct inode *i) {
  printk("efs_put_inode: iput()ting inode %#0lx\n", i->i_ino);
}


/* ----- Conversion utilities ----- 
   Get 16- and 32-bit unsigned values in big-endian format from a byte buffer
*/
__u32 ConvertLong(__u8 *buf, int offset)
{
	return *((__u32 *)(buf + offset));
	/*  return  (__u32)buf[offset+3] |
	    ((__u32)buf[offset+2])<<8 |
	    ((__u32)buf[offset+1])<<16 |
	    ((__u32)buf[offset])<<24;
	    */
}

__u16 ConvertShort(__u8 *buf, int offset)
{
	return *((__u16 *)(buf + offset));
	/*
	  return (__u16)buf[offset+1] |
	  ((__u16)buf[offset])<<8;
	  */
}


/* ----- Install/Remove Module ----- 
 These procedures are used to install/remove our filesystem
 module in the linux kernel
*/

/* describe filesystem */
struct super_block *efs_read_super(struct super_block *s, void *d, int sil);

static struct file_system_type efs_fs_type = {
	"efs",
	FS_REQUIRES_DEV,
	efs_read_super,
	NULL
};

__initfunc(int init_efs_fs(void)) {
	return register_filesystem(&efs_fs_type);
}

#ifdef MODULE
EXPORT_NO_SYMBOLS;

/* install module */
int init_module(void)
{
	return init_efs_fs();
}

/* remove module */
void cleanup_module(void)
{
	unregister_filesystem(&efs_fs_type);
}
#endif /* MODULE */

#ifdef DEBUG_EFS
void efs_dump_super(struct efs_super_block *sb) {
	printk("efs_super_block @ 0x%p:\n", sb);
	printk("size:     %0#8lX   firstcg:  %0#8lX\n", sb->s_size, sb->s_firstcg);
	printk("cgfsize:  %0#8lX   cgisize:  %0#8hX\n", sb->s_cgfsize,sb->s_cgisize);
	printk("sectors:  %0#8hX   heads:    %0#8hX\n", sb->s_sectors, sb->s_heads);
	printk("ncg:      %0#8hX   dirty:    %0#8hX\n", sb->s_ncg, sb->s_dirty);
	printk("time:     %0#8lX   magic:    %0#8lX\n", sb->s_time, sb->s_magic);
	printk("fname:    %.6s   fpack:    %.6s\n", sb->s_fname, sb->s_fpack);
	printk("bmsize:   %0#8lX   tfree:    %0#8lX\n", sb->s_bmsize, sb->s_tfree);
	printk("tinode:   %0#8lX   bmblock:  %0#8lX\n", sb->s_tinode, sb->s_bmblock);
	printk("replsb:   %0#8lX   lastiall: %0#8lX\n", sb->s_replsb,
	       sb->s_lastialloc);
	printk("checksum: %0#8lX\n", sb->s_checksum);
} 

void efs_dump_inode(struct efs_disk_inode *di) {
	printk("efs_disk_inode @ 0x%p: ", di);
	printk("[%o %hd %hd %u %u %u %u %u #ext=%hd %u %u]\n",
	       di->di_mode, di->di_nlink, di->di_uid, di->di_gid, di->di_size,
	       di->di_atime, di->di_mtime, di->di_ctime, di->di_gen, 
	       di->di_numextents, di->di_version);
}
#endif

/* ----- efs_checkVolDesc -----
   Analyse the first block of a CD and check 
   if it's a valid efs volume descriptor

   blk    - buffer with the data of first block
   silent - 0 -> verbose

   return : 0 - error ; >0 - start block of filesystem
*/
#if 0
static __u32 efs_checkVolDesc(__u8 *blk,int silent)
{
	__u32 magic;
	__u8 *ptr;
	__u8 name[10];
	__u32 pos,len;
	int i;

	/* is the magic cookie here? */
	magic = ConvertLong(blk,0);
	if(magic!=0x0be5a941) {
		printk("EFS: no magic on first block\n");
		return 0;
	}
  
	/* Walk through the entries of the VD */
	/* Quite useless, but gives nice output ;-) */
	ptr = blk + EFS_VD_ENTRYFIRST;
	name[8] = 0;
	while(*ptr) {
		for(i=0;i<8;i++)
			name[i] = ptr[i];

		/* start and length of entry */
		pos = ConvertLong(ptr,EFS_VD_ENTRYPOS);
		len = ConvertLong(ptr,EFS_VD_ENTRYLEN);

		if(!silent) 
			printk("EFS: VolDesc: %8s blk: %08lx len: %08lx\n",
			       name,pos,len);
    
		ptr+=EFS_VD_ENTRYSIZE;
	}

	pos = ConvertLong(blk,EFS_VD_FS_START);
	printk("EFS: FS start: %08lx\n",pos);
	return pos;
}
#endif

/* ----- efs_checkSuper ----- 
 Check if the given block is a valid EFS-superblock
 
 sbi    - my EFS superblock info
 block  - block that must be examined

 return - 0 ok, -1 error
*/
static int efs_verify_super(struct efs_super_block *sb, 
			    struct efs_sb_info *sbi, 
			    int silent)
{
	__u32    magic;

	magic = sb->s_magic;
	/* check if the magic cookie is here */
	if((magic!=EFS_MAGIC1)&&(magic!=EFS_MAGIC2)) {
		printk("EFS: magic %#X doesn't match %#X or %#X!\n",
		       magic, EFS_MAGIC1, EFS_MAGIC2);
		return -1;
	}

	/* XXX should check csum */
  
	sbi->total_blocks = sb->s_size;
	sbi->first_block  = sb->s_firstcg;
	sbi->group_size   = sb->s_cgfsize;
	sbi->inode_blocks = sb->s_cgisize;
	sbi->total_groups = sb->s_ncg;
    
	return 0;    
}


/* ----- efs_read_super ----- 
   read_super: if the fs gets mounted this procedure is called to
   check if the filesystem is valid and to fill the superblock struct
   
   s     - superblock struct
   d     - options for fs (unused)
   sil   - flag to be silent
   
   return - filled s struct or NULL on error
 */

struct super_block *efs_read_super(struct super_block *s, void *d, int silent)
{
	struct buffer_head *bh;
	struct efs_sb_info *sb_info = (struct efs_sb_info *)&s->u.efs_sb;
	struct efs_super_block *es;
	int error = 0;
	int dev = s->s_dev;
	struct inode *root_inode = NULL;

	MOD_INC_USE_COUNT;
  
	/* say hello to my log file! */
#ifdef EFS_DEBUG
	if(!silent)
		printk("EFS: --- Filesystem ---\n");
#endif
	/* set blocksize to 512 */
	set_blocksize(dev, EFS_BLOCK_SIZE);
  
	lock_super(s);
  
#if 0  
	/* Read first block of CD: the Volume Descriptor */
	bh = bread(dev, 0, EFS_BLOCK_SIZE);
	if(bh) {
		sb_info->fs_start = efs_checkVolDesc((__u8 *)bh->b_data,silent);
		if(sb_info->fs_start==0) {
			printk("EFS: failed checking Volume Descriptor\n");
			/* error++; */
		}
		brelse(bh);
	} else {
		printk("EFS: cannot read the first block\n");
		error++;
	}
#endif 0

	/* Read the Superblock */
	if(!error) {	
#ifdef DEBUG_EFS_SB
		if (!silent)
			printk("EFS: reading superblock.\n");
#endif
		bh = bread(dev, EFS_BLK_SUPER,  EFS_BLOCK_SIZE );
		if(bh) {
			es = (struct efs_super_block *)(bh->b_data);
#ifdef DEBUG_EFS_SB
			if(!silent)
				efs_dump_super(es);
#endif
			if(efs_verify_super(es, sb_info, silent)) {
				printk("EFS: failed checking Superblock\n");
				error++;
			}
			brelse(bh);
		} else {
			printk("EFS: cannot read the superblock\n");
			error++;
		}
	} 
  
	if(!error) {
		s->s_blocksize = EFS_BLOCK_SIZE;
		s->s_blocksize_bits = EFS_BLOCK_SIZE_BITS;
    
		s->s_magic	= EFS_SUPER_MAGIC;
		s->s_flags	= MS_RDONLY;
		s->s_op	= &efs_sops;
		s->s_dev	= dev;
		root_inode = iget(s, EFS_ROOT_INODE);
		s->s_root = d_alloc_root(root_inode, NULL);
		if (!s->s_root) {
			error++;
			printk("EFS: couldn't allocate root inode!\n");
		}
	}

	unlock_super(s);
  
	if(check_disk_change(s->s_dev)) {
		printk("EFS: Device changed!\n");
		error++;
	}

	/* We found errors -> say goodbye! */
	if(error) {
		s->s_dev = 0;
		d_delete(s->s_root);	/* XXX is this enough? */
		printk("EFS: init failed with %d errors\n", error);
		brelse(bh);
		MOD_DEC_USE_COUNT;
		return NULL;
	} 

	return s;
}


/* ----- efs_put_super ----- 
   put_super: remove the filesystem and the module use count

   s - superblock
*/
static void efs_put_super(struct super_block *s)
{
	lock_super(s);
	s->s_dev = 0;
	unlock_super(s);
	MOD_DEC_USE_COUNT;
}


/* ----- efs_statfs ----- 
   statfs: get informatio on the filesystem
   
   s   - superblock of fs
   buf - statfs struct that has to be filled
*/
static int efs_statfs(struct super_block *s, struct statfs *buf,int bufsize)
{
	struct efs_sb_info *sbi = (struct efs_sb_info *)&s->u.generic_sbp;
	struct statfs tmp;

	tmp.f_type = EFS_SUPER_MAGIC;
	tmp.f_bsize = EFS_BLOCK_SIZE;
	tmp.f_blocks = sbi->total_blocks;
	tmp.f_bfree = 0;
	tmp.f_bavail = 0;
	tmp.f_files = 100; /* don't know how to calculate the correct value */
	tmp.f_ffree = 0;
	tmp.f_namelen = NAME_MAX;

	return copy_to_user(buf,&tmp,bufsize) ? -EFAULT : 0;
}


/* ----- efs_read_inode ----- 
   read an inode specified by in->i_ino from disk, fill the inode
   structure and install the correct handler for the file type
   
   in   - inode struct
*/
void efs_read_inode(struct inode *in)
{
    struct buffer_head *bh;
    struct efs_sb_info *sbi = (struct efs_sb_info *)&in->i_sb->u.generic_sbp;
    struct efs_disk_inode *di;
    struct efs_inode_info *ini;
    __u32 blk, off, rdev;
    __u16 numext;
    int i;
    int error = 0;
  
    /*
     * Find the block and offset for desired inode (4 inodes fit in one block)
     */
    blk = in->i_ino >> 2; 
    blk = sbi->fs_start + sbi->first_block + 
	(sbi->group_size * (blk / sbi->inode_blocks)) +
	(blk % sbi->inode_blocks);
    off = (in->i_ino&3)<<7;
  
    /* Read the block with the inode from disk */
#ifdef DEBUG_EFS_INODE
    printk("EFS: looking for inode %#lx at %d/%d\n", in->i_ino, blk, off);
#endif
    bh = bread(in->i_dev,blk,EFS_BLOCK_SIZE);
    if (!bh) {
	printk("EFS: can't read physical block %d for inode %#lx\n",
	       blk, in->i_ino);
	return;
    }
    di = (struct efs_disk_inode *)(bh->b_data + off);
    ini = &in->u.efs_i;
	    
    /* fill in standard inode infos */
    in->i_mtime = efs_swab32(di->di_mtime);
    in->i_ctime = efs_swab32(di->di_ctime);
    in->i_atime = efs_swab32(di->di_atime);
    in->i_size  = efs_swab32(di->di_size);
    in->i_nlink = efs_swab16(di->di_nlink);
    in->i_uid   = efs_swab16(di->di_uid);
    in->i_gid   = efs_swab16(di->di_gid);
    in->i_mode  = efs_swab16(di->di_mode);
	    
    /* Special files store their rdev value where the extends of
       a regular file are found */
    /*    rdev = ConvertLong(rawnode,EFS_IN_EXTENTS);*/
    /* XXX this isn't right */
    rdev = efs_swab32(*(__u32 *)&di->di_u.di_dev); 
    
    /* get the number of extends the inode posseses */
    numext = efs_swab16(di->di_numextents);
    ini->efs_extents = kmalloc(sizeof(union efs_extent) * numext, GFP_KERNEL);
    if (!ini->efs_extents) {
	printk("EFS: not enough memory\n");
	brelse(bh);
	return;
    }

#define COPY_EXTENT(from, to) \
    (to).ex_bytes[0] = efs_swab32((from).ex_bytes[0]); \
    (to).ex_bytes[1] = efs_swab32((from).ex_bytes[1]);

#define EXTENTS_PER_BLOCK	(EFS_BLOCK_SIZE / sizeof (union efs_extent))

    /* if this inode has more than EFS_MAX_EXTENDS then the extends are
       stored not directly in the inode but indirect on an extra block.
       The address of the extends-block is stored in the inode */
    if(numext>EFS_MAX_EXTENTS) { 
	__u32 numindirect, count;
	union efs_extent indirect;

	numindirect = di->di_u.di_extents[0].ex_ex.ex_offset;
#if defined(DEBUG_EFS_INODE) || defined(DEBUG_EFS_EXTENTS)
	printk("EFS: indirect inode #%lx has %d extents (%d indirect)\n",
	       in->i_ino, numext, numindirect);
#endif

	count = 0;
	for (i = 0; i < numindirect; i++) {
	    union efs_extent *ptr;
	    struct buffer_head *bh2;
	    __u32 extbytes, copybytes;

	    COPY_EXTENT(di->di_u.di_extents[i], indirect);
	    if (numext - count > EXTENTS_PER_BLOCK) {
		extbytes = indirect.ex_ex.ex_length * EFS_BLOCK_SIZE;
		copybytes = extbytes;
	    } else {
		extbytes = (numext - count) * sizeof (union efs_extent);
		copybytes = (extbytes & ~511) + 512;
	    }
	    bh2 = bread(in->i_dev, indirect.ex_ex.ex_bn, copybytes);
	    if (!bh2) {
		printk("EFS: failed reading indirect extents\n");
		brelse(bh);
		return;
	    }

	    ptr = (union efs_extent *)bh2->b_data;

#ifdef __BIG_ENDIAN
	    /* we're the same endianness as EFS, so we can just copy them
	       into place */
	    {
		void * copyto = ini->efs_extents + count;
#ifdef DEBUG_EFS_EXTENTS_VERBOSE
		union efs_extent *iter = copyto;
		printk("EFS: copying %lx extbytes (%d exts) from %p to %p "
		       "(ini %p, ->efs_extents %p)\n",
		       extbytes, extbytes / sizeof (union efs_extent),
		       ptr, copyto, ini, ini->efs_extents);
#endif
		bcopy((void *)ptr, copyto, extbytes);
		count += extbytes / sizeof (union efs_extent);
#ifdef DEBUG_EFS_EXTENTS_VERBOSE
		for(; iter < (char *)copyto + extbytes; iter++) {
		    printk("EFS: ext: [%08x%08x]: magic: %x bn: %0#6x "
			   "length: %0#2x offset: %0#6x\n",
			   iter->ex_bytes[0], iter->ex_bytes[1],
			   iter->ex_ex.ex_magic, iter->ex_ex.ex_bn,
			   iter->ex_ex.ex_length, iter->ex_ex.ex_offset);
		}
#endif
	    }
#else
#error "EFS not supported on __LITTLE_ENDIAN systems yet"
#if 0
	    
	    for (j = 0; j < EXTENTS_PER_BLOCK * indirect.ex_ex.ex_length; j++){
		union efs_extent *_ext = &ini->efs_extents[count];
		COPY_EXTENT(ptr[k], *_ext);

#ifdef DEBUG_EFS_EXTENTS_VERBOSE
		printk("EFS: %d/%d: extent %d is %8d-%8d (phys %d)\n",
		       i, j, count, _ext->ex_ex.ex_offset,
		       _ext->ex_ex.ex_offset + _ext->ex_ex.ex_length,
		       _ext->ex_ex.ex_bn);
#endif

		if (++count == numext)
		    break;
	    }
#endif /* 0 */
#endif /* ENDIAN */
	    brelse(bh2);
	    if (count == numext)
		break;
	}
#ifdef DEBUG_EFS_EXTENTS
	printk("EFS: loaded %d of %d extents from indirect inode %#0lx\n",
	       count, numext, in->i_ino);
#endif
    } else {
#if defined(DEBUG_EFS_INODE) || defined(DEBUG_EFS_EXTENTS)
	printk("EFS: inode %#lx is direct with %d extents\n",
	       in->i_ino, numext);
#endif
	/* copy extends directly from rawinode */
	for(i=0; i<numext; i++) {
	    COPY_EXTENT(di->di_u.di_extents[i], ini->efs_extents[i]);
	}
    }
    ini->efs_total = numext;
    ini->efs_current = 0;
    
    brelse(bh);
    
#ifdef DEBUG_EFS_INODE
    printk("%lx inode: blk %lx numext %x\n",in->i_ino,ini->extblk,numext);
    efs_dump_inode(di);
#endif

    /* Install the filetype Handler */
    switch(in->i_mode & S_IFMT) {
    case S_IFDIR: 
	in->i_op = &efs_dir_in_ops; 
	break;
    case S_IFREG:
	in->i_op = &efs_file_in_ops;
	break;
    case S_IFLNK:
	in->i_op = &efs_symlink_in_ops;
	break;
    case S_IFCHR:
	in->i_rdev = rdev;
	in->i_op = &chrdev_inode_operations; 
	break;
    case S_IFBLK:
	in->i_rdev = rdev;
	in->i_op = &blkdev_inode_operations; 
	break;
    case S_IFIFO:
	init_fifo(in);
	break;    
    default:
	printk("EFS: Unsupported inode mode %o\n",(unsigned int)(in->i_mode));
	error++;
	break;
    }
        
}
