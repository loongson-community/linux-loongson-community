/*
 * super.c
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang.
 */

#include <linux/module.h>
#include <linux/efs.h>

void			efs_read_inode(struct inode *);
void			efs_put_super(struct super_block *);
int			efs_statfs(struct super_block *, struct statfs *, int);
struct super_block *	efs_read_super(struct super_block *, void *, int);

static struct file_system_type efs_fs_type = {
	"efs",			/* filesystem name */
	FS_REQUIRES_DEV,	/* fs_flags */
	efs_read_super,		/* entry function pointer */
	NULL 			/* next */
};

static struct super_operations efs_superblock_operations = {
	efs_read_inode,	/* read_inode */
	NULL,		/* write_inode */
	NULL,		/* put_inode */
	NULL,		/* delete_inode */
	NULL,		/* notify_change */
	efs_put_super,	/* put_super */
	NULL,		/* write_super */
	efs_statfs,	/* statfs */
	NULL		/* remount */
};

int init_module(void) {
	return register_filesystem(&efs_fs_type);
}

void cleanup_module(void) {
	unregister_filesystem(&efs_fs_type);
}

static long efs_validate_vh(struct volume_header *vh, int slice) {
	int	i, j;
	int32_t	sblock = 0;
	int	type;
	char	name[VDNAMESIZE+1];

	if (be32_to_cpu(vh->vh_magic) != VHMAGIC) {
		printk("EFS: invalid volume descriptor\n");
		return 0;
	}

#ifdef DEBUG
	printk("EFS: bf: %16s\n", vh->vh_bootfile);
#endif

	for(i = 0; i < NVDIR; i++) {
		for(j = 0; j < VDNAMESIZE; j++) {
			name[j] = vh->vh_vd[i].vd_name[j];
		}
		name[j] = (char) 0;

#ifdef DEBUG
		if (name[0]) {
			printk("EFS: vh: 0x%8s block: 0x%08x size: 0x%08x\n",
				name,
				(int) be32_to_cpu(vh->vh_vd[i].vd_lbn),
				(int) be32_to_cpu(vh->vh_vd[i].vd_nbytes));
		}
#endif
	}

	for(i = 0; i < NPARTAB; i++) {
		type = (int) be32_to_cpu(vh->vh_pt[i].pt_type);
#ifdef DEBUG
		printk("EFS: pt: start: %08d size: %08d type: %08d\n",
			(int) be32_to_cpu(vh->vh_pt[i].pt_firstlbn),
			(int) be32_to_cpu(vh->vh_pt[i].pt_nblks),
			type);
#endif
		if (slice == i && (type == 5 || type == 7)) {
			sblock = be32_to_cpu(vh->vh_pt[i].pt_firstlbn);
		} else if (slice == -1 && sblock == 0) {
			if (type == 5 || type == 7) {
				sblock = be32_to_cpu(vh->vh_pt[i].pt_firstlbn);
				slice = i;
			}
		}
	}

	if (sblock) {
		printk("EFS: using slice %d (offset 0x%x)\n", slice, sblock);
	} else if (slice != -1) {
		printk("EFS: no efs filesystem found on slice %d\n", slice);
	}
	return(sblock);
}

static int efs_validate_super(struct efs_spb *sb, struct efs *super) {

	if (!IS_EFS_MAGIC(be32_to_cpu(super->fs_magic))) return -1;

	sb->fs_magic     = be32_to_cpu(super->fs_magic);
	sb->total_blocks = be32_to_cpu(super->fs_size);
	sb->first_block  = be32_to_cpu(super->fs_firstcg);
	sb->group_size   = be32_to_cpu(super->fs_cgfsize);
	sb->inode_blocks = be16_to_cpu(super->fs_cgisize);
	sb->total_groups = be16_to_cpu(super->fs_ncg);
    
	return 0;    
}

struct super_block *efs_read_super(struct super_block *s, void *d, int verbose) {
	int slice = -1;
	kdev_t dev = s->s_dev;
	struct inode *root_inode;
	struct efs_spb *spb;
	struct buffer_head *bh;

	if (d) {
		/* parse mount option */
		if (!strncmp(d, "slice=", 6)) {
			slice = simple_strtoul(d+6, NULL, 10);
		} else if (strcmp(d, "")) {
			printk("EFS: invalid mount option\n");
			return(NULL);
		}
	}

	MOD_INC_USE_COUNT;
	lock_super(s);
  
	/* approx 230 bytes available in this union */
 	spb = (struct efs_spb *) &(s->u.generic_sbp);

	set_blocksize(dev, EFS_BLOCKSIZE);
  
	/* read the vh (volume header) block */
	bh = bread(dev, 0, EFS_BLOCKSIZE);

	if (!bh) {
		printk("EFS: cannot read volume header\n");
		goto out_no_fs_ul;
	}

	spb->fs_start = efs_validate_vh((struct volume_header *) bh->b_data, slice);
	brelse(bh);

	if(!spb->fs_start) {
		printk("EFS: invalid volume descriptor\n");
		goto out_no_fs_ul;
	}

	bh = bread(dev, spb->fs_start + EFS_SUPER, EFS_BLOCKSIZE);
	if (!bh) {
		printk("EFS: unable to read superblock\n");
		goto out_no_fs_ul;
	}
		
	if (efs_validate_super(spb, (struct efs *) bh->b_data)) {
		printk("EFS: invalid superblock\n");
		brelse(bh);
		goto out_no_fs_ul;
	}
  
	s->s_magic		= EFS_SUPER_MAGIC;
	s->s_blocksize		= EFS_BLOCKSIZE;
	s->s_blocksize_bits	= EFS_BLOCKSIZE_BITS;
	if (!(s->s_flags & MS_RDONLY)) {
#ifdef DEBUG
		printk("EFS: forcing read-only: RW access not supported\n");
#endif
		s->s_flags |= MS_RDONLY;
	}
	s->s_op			= &efs_superblock_operations;
	s->s_dev		= dev;
	s->s_root		= NULL;

	root_inode = iget(s, EFS_ROOTINODE);
	if (root_inode) {
		s->s_root = d_alloc_root(root_inode, NULL);
		if (!(s->s_root)) {
			iput(root_inode);
		}
	}
	unlock_super(s);
  
	if(!(s->s_root)) {
		printk("EFS: not mounted\n");
		goto out_no_fs;
	}

	if(check_disk_change(s->s_dev)) {
		printk("EFS: device changed\n");
		goto out_no_fs;
	}

	return(s);

out_no_fs_ul:
	unlock_super(s);
out_no_fs:
	s->s_dev = 0;
	MOD_DEC_USE_COUNT;
	return(NULL);
}

void efs_put_super(struct super_block *s) {
	s->s_dev = 0;
	MOD_DEC_USE_COUNT;
}

int efs_statfs(struct super_block *s, struct statfs *buf, int bufsiz) {
	struct statfs tmp;
	struct efs_spb *sbp = (struct efs_spb *)&s->u.generic_sbp;

	tmp.f_type    = EFS_SUPER_MAGIC;
	tmp.f_bsize   = EFS_BLOCKSIZE;
	tmp.f_blocks  = sbp->total_blocks;
	tmp.f_bfree   = 0;
	tmp.f_bavail  = 0;
	tmp.f_files   = 0; /* ? */
	tmp.f_ffree   = 0;
	tmp.f_fsid.val[0] = (sbp->fs_magic >> 16) & 0xffff;
	tmp.f_fsid.val[1] =  sbp->fs_magic        & 0xffff;
	tmp.f_namelen = EFS_MAXNAMELEN;

	return copy_to_user(buf, &tmp, bufsiz) ? -EFAULT : 0;
}

