/*
 * linux/fs/efs/inode.c
 *
 * Copyright (C) 1998  Mike Shaver
 *
 * Portions derived from work (C) 1995,1996 Christian Vogelgsang.
 * ``Inspired by'' fs/minix/inode.c.
 */

#include <linux/module.h>	/* module apparatus */
#include <linux/init.h>		/* __initfunc */
#include <linux/efs_fs.h>
#include <linux/locks.h>
#include <asm/uaccess.h>

#define COPY_EXTENT(from, to)						      \
{									      \
    to.ex_bytes[0] = efs_swab32(from.ex_bytes[0]);			      \
    to.ex_bytes[1] = efs_swab32(from.ex_bytes[1]);			      \
}

void
efs_put_super(struct super_block *sb)
{
    MOD_DEC_USE_COUNT;
}

static struct super_operations efs_sops = {
    efs_read_inode,
    NULL,			/* write_inode */
    NULL,			/* put_inode */
    NULL,			/* delete_inode */
    NULL,			/* notify_change */
    efs_put_super,
    NULL,			/* write_super */
    efs_statfs,
    NULL,			/* remount */
};

static const char *
efs_checkroot(struct super_block *sb, struct inode *dir)
{
    struct buffer_head *bh;

    if (!S_ISDIR(dir->i_mode))
	return "root directory is not a directory";

    bh = bread(dir->i_dev, efs_bmap(dir, 0), EFS_BLOCK_SIZE);	
    if (!bh)
	return "unable to read root directory";

    /* XXX check sanity of root directory */
    
    brelse(bh);
    return NULL;
}

struct super_block *
efs_read_super(struct super_block *s, void *data, int silent)
{
    struct buffer_head *bh;
    struct efs_disk_sb *efs_sb;
    struct efs_sb_info *sbi;
    kdev_t dev = s->s_dev;
    const char *errmsg = "default error message";
    struct inode *root;
    __u32 magic;

    DB(("read_super on dev %s\n", kdevname(dev)));
    MOD_INC_USE_COUNT;

    lock_super(s);
    set_blocksize(dev, EFS_BLOCK_SIZE);
    
    /*
     * XXXshaver
     * If this is a CDROM, then there's a volume descriptor at block
     * EFS_BLK_VOLDESC.  What's there if it's just a disk partition?
     */
       
    bh = bread(dev, EFS_BLK_SUPER, EFS_BLOCK_SIZE);
    if (!bh)
	goto out_bad_sb;
    
    efs_sb = (struct efs_disk_sb *)bh->b_data;
    sbi = &s->u.efs_sb;
    sbi->total_blocks = efs_sb->s_size;
    sbi->first_block = efs_sb->s_firstcg;
    sbi->group_size = efs_sb->s_cgfsize;
    sbi->inode_blocks = efs_sb->s_cgisize;
    sbi->total_groups = efs_sb->s_ncg;
    magic = efs_sb->s_magic;
    brelse(bh);

    if (magic == EFS_MAGIC1 || magic == EFS_MAGIC2) {
	DB(("EFS: valid superblock magic\n"));
    } else {
	goto out_no_fs;
    }

    if (efs_sb->s_dirty != EFS_CLEAN) {
	switch(efs_sb->s_dirty) {
	  case EFS_ACTIVE:
	    errmsg = "Partition was not unmounted properly, but is clean";
	    break;
	  case EFS_ACTIVEDIRT:
	    errmsg = "Partition was mounted dirty and not cleanly unmounted";
	    break;
	  case EFS_DIRTY:
	    errmsg = "Partition was not umounted properly, and is dirty";
	    break;
	  default:
	    errmsg = "unknown!\n";
	    break;
	}
	if (!silent)
	    printk("EFS: ERROR: cleanliness is %#04x: %s\n", efs_sb->s_dirty,
		   errmsg);
	goto out_unlock;
    }
    
    s->s_blocksize = EFS_BLOCK_SIZE;
    s->s_blocksize_bits = EFS_BLOCK_SIZE_BITS;
    s->s_magic = EFS_SUPER_MAGIC;
    s->s_op = &efs_sops;
    DB(("getting root inode (%d)\n", EFS_ROOT_INODE));
    root = iget(s, EFS_ROOT_INODE);
    
    if (!root->i_size)
	goto out_bad_root;
    DB(("checking root inode\n"));
    errmsg = efs_checkroot(s, root);
    if (errmsg)
	goto out_bad_root;

    DB(("root inode OK\n"));
    
    s->s_root = d_alloc_root(root, NULL);
    if (!s->s_root)
	goto out_iput;
    
    /* we only do RO right now */
    if (!(s->s_flags & MS_RDONLY)) {
	if (!silent)
	    printk("EFS: forcing read-only: RW access not supported\n");
	s->s_flags |= MS_RDONLY;
    }

    unlock_super(s);
    return s;

    /* error-handling exit paths */    
 out_bad_root:
    if (!silent && errmsg)
	printk("EFS: bad_root ERROR: %s\n", errmsg);

 out_iput:
    iput(root);
    brelse(bh);
    goto out_unlock;

 out_no_fs:
    printk("EFS: ERROR: bad magic\n");
    goto out_unlock;

 out_bad_sb:
    printk("EFS: unable to read superblock\n");

 out_unlock:
    s->s_dev = 0;
    unlock_super(s);
    MOD_DEC_USE_COUNT;
    return NULL;
}

int
efs_statfs(struct super_block *sb, struct statfs *buf, int bufsiz)
{
    struct statfs tmp;

    DB(("statfs\n"));
    tmp.f_type = sb->s_magic;
    tmp.f_bsize = sb->s_blocksize;
    tmp.f_blocks = sb->u.efs_sb.total_blocks;
    tmp.f_bfree = 0;
    tmp.f_bavail = 0;
    tmp.f_files = 0;		/* XXX? */
    tmp.f_ffree = 0;
    tmp.f_namelen = NAME_MAX;
    return copy_to_user(buf, &tmp, bufsiz) ? -EFAULT : 0;
}

void
efs_read_inode(struct inode *in)
{
    struct efs_sb_info *efs_sb = &in->i_sb->u.efs_sb;
    struct buffer_head *bh;
    struct efs_disk_inode *di;
    struct efs_inode_info *ini = &in->u.efs_i;
    int block, ino = in->i_ino, offset;
    __u16 numext;
    __u32 rdev;

    DB(("read_inode\n"));

    /*
     * Calculate the disk block and offset for the inode.
     * There are 4 inodes per block.
     */
    block = ino / EFS_INODES_PER_BLOCK;

    /*
     * Inodes are stored at the beginning of every cylinder group.
     * 
     * We find the block containing the inode like so:
     * - block is set above to the ``logical'' block number
     * - first_block is the start of the FS
     * - (block / inode_blocks) is the cylinder group that the inode is in.
     * - (block % inode_blocks) is the block offset within the cg
     *
     */
    block = efs_sb->first_block +
	(efs_sb->group_size * (block / efs_sb->inode_blocks)) +
	(block % efs_sb->inode_blocks);
    
    /* find the offset */
    offset = (ino % EFS_INODES_PER_BLOCK) << 7;

    DB(("EFS: looking for inode #%xl in blk %d offset %d\n",
	ino, block, offset));

    bh = bread(in->i_dev, block, EFS_BLOCK_SIZE);

    if (!bh) {
	printk("EFS: failed to bread blk #%xl for inode %#xl\n", block, ino);
	goto error;
    }

    di = (struct efs_disk_inode *)(bh->b_data + offset);

    /* standard inode info */
    in->i_mtime = efs_swab32(di->di_mtime);
    in->i_ctime = efs_swab32(di->di_ctime);
    in->i_atime = efs_swab32(di->di_atime);
    in->i_size  = efs_swab32(di->di_size);
    in->i_nlink = efs_swab16(di->di_nlink);
    in->i_uid   = efs_swab16(di->di_uid);
    in->i_gid   = efs_swab16(di->di_gid);
    in->i_mode  = efs_swab16(di->di_mode);
    
    DB(("INODE %ld: mt %ld ct %ld at %ld sz %ld nl %ld uid %ld gid %ld mode %lo\n",
	in->i_ino,
	in->i_mtime, in->i_ctime, in->i_atime, in->i_size, in->i_nlink,
	in->i_uid, in->i_gid, in->i_mode));

    rdev = efs_swab32(*(__u32 *) &di->di_u.di_dev);
    numext = efs_swab16(di->di_numextents);

    if (numext > EFS_MAX_EXTENTS) {
	DB(("EFS: inode %#0x is indirect (%d)\n", ino, numext));

	/*
	 * OPT: copy the first 10 extents in here?
	 */
    } else {
	int i;

	DB(("EFS: inode %#lx is direct (%d).  Happy day!\n", in->i_ino,
	    numext));
	ini->extblk = block;

	/* copy extents into inode_info */
	for (i = 0; i < numext; i++) {
	    COPY_EXTENT(di->di_u.di_extents[i], ini->extents[i]);
	}

    }
    ini->tot = numext;
    ini->cur = 0;
    brelse(bh);
    
    if (S_ISDIR(in->i_mode))
	in->i_op = &efs_dir_inode_operations;
    else if (S_ISREG(in->i_mode))
	in->i_op = &efs_file_inode_operations;
    else if (S_ISLNK(in->i_mode))
	in->i_op = &efs_symlink_inode_operations;
    else if (S_ISCHR(in->i_mode)) {
	in->i_rdev = rdev;
	in->i_op = &chrdev_inode_operations;
    } else if (S_ISBLK(in->i_mode)) {
	in->i_rdev = rdev;
	in->i_op = &blkdev_inode_operations;
    } else if (S_ISFIFO(in->i_mode))
	init_fifo(in);
    else {
	printk("EFS: ERROR: unsupported inode mode %#lo (dir is %#lo) =? %d\n",
	       (in->i_mode & S_IFMT), (long)S_IFDIR,
	       in->i_mode & S_IFMT == S_IFDIR);
	goto error;
    }

    return;

 error:
    DB(("ERROR: INODE %ld: mt %ld ct %ld at %ld sz %ld nl %ld uid %ld "
	"gid %ld mode %lo\n", in->i_ino,
	in->i_mtime, in->i_ctime, in->i_atime, in->i_size, in->i_nlink,
	in->i_uid, in->i_gid, in->i_mode));
    in->i_mtime = in->i_atime = in->i_ctime = 0;
    in->i_size = 0;
    in->i_nlink = 1;
    in->i_uid = in->i_gid = 0;
    in->i_mode = S_IFREG;
    in->i_op = NULL;
}

static struct file_system_type efs_fs_type = {
    "efs",
    FS_REQUIRES_DEV,
    efs_read_super,
    NULL
};

__initfunc(int
init_efs_fs(void))
{
    return register_filesystem(&efs_fs_type);
}

#ifdef MODULE
EXPORT_NO_SYMBOLS;

int
init_module(void)
{
    DB(("loading EFS module\n"));
    return init_efs_fs();
}

void
cleanup_module(void)
{
    DB(("removing EFS module\n"));
    unregister_filesystem(&efs_fs_type);
}

#endif
