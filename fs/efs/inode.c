/*
 * inode.c
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang,
 *              and from work (c) 1998 Mike Shaver.
 */

#include <linux/efs.h>

void efs_read_inode(struct inode *in) {
	int i, extents, inode_index;
	dev_t device;
	struct buffer_head *bh;
	struct efs_spb *sbp = (struct efs_spb *)&in->i_sb->u.generic_sbp;
	struct efs_in_info *ini = (struct efs_in_info *)&in->u.generic_ip;
	efs_block_t block, offset;
	struct efs_dinode *efs_inode;
  
	/*
	** EFS layout:
	**
	** |   cylinder group    |   cylinder group    |   cylinder group ..etc
	** |inodes|data          |inodes|data          |inodes|data       ..etc
	**
	** work out the inode block index, (considering initially that the
	** inodes are stored as consecutive blocks). then work out the block
	** number of that inode given the above layout, and finally the
	** offset of the inode within that block.
	*/

	/* four inodes are stored in one block */
	inode_index = in->i_ino / (EFS_BLOCKSIZE / sizeof(struct efs_dinode));

	block = sbp->fs_start + sbp->first_block + 
		(sbp->group_size * (inode_index / sbp->inode_blocks)) +
		(inode_index % sbp->inode_blocks);

	offset = (in->i_ino % (EFS_BLOCKSIZE / sizeof(struct efs_dinode))) << 7;

	bh = bread(in->i_dev, block, EFS_BLOCKSIZE);
	if (!bh) {
		printk("EFS: bread() failed at block %d\n", block);
		goto read_inode_error;
	}

	efs_inode = (struct efs_dinode *) (bh->b_data + offset);
    
	/* fill in standard inode infos */
	in->i_mode  = be16_to_cpu(efs_inode->di_mode);
	in->i_nlink = be16_to_cpu(efs_inode->di_nlink);
	in->i_uid   = be16_to_cpu(efs_inode->di_uid);
	in->i_gid   = be16_to_cpu(efs_inode->di_gid);
	in->i_size  = be32_to_cpu(efs_inode->di_size);
	in->i_atime = be32_to_cpu(efs_inode->di_atime);
	in->i_mtime = be32_to_cpu(efs_inode->di_mtime);
	in->i_ctime = be32_to_cpu(efs_inode->di_ctime);

	/* this is last valid block in the file */
	in->i_blocks = ((in->i_size - 1) >> EFS_BLOCKSIZE_BITS);

    	device = be32_to_cpu(efs_inode->di_u.di_dev);

	/* The following values are stored in my private part of the Inode.
	   They are necessary for further operations with the file */

	/* get the number of extents for this object */
	extents = be16_to_cpu(efs_inode->di_numextents);

	/* copy the first 12 extents directly from the inode */
	for(i = 0; i < EFS_DIRECTEXTENTS; i++) {
		/* ick. horrible (ab)use of union */
		ini->extents[i].u1.l = be32_to_cpu(efs_inode->di_u.di_extents[i].u1.l);
		ini->extents[i].u2.l = be32_to_cpu(efs_inode->di_u.di_extents[i].u2.l);
		if (i < extents && ini->extents[i].u1.s.ex_magic != 0) {
			printk("EFS: extent %d has bad magic number in inode %lu\n", i, in->i_ino);
			brelse(bh);
			goto read_inode_error;
		}
	}
	ini->numextents = extents;
	ini->lastextent = 0;

	brelse(bh);
   
#ifdef DEBUG
	printk("EFS: efs_read_inode(): inode %lu, extents %d\n",
		in->i_ino, extents);
#endif

	/* Install the filetype Handler */
	switch (in->i_mode & S_IFMT) {
		case S_IFDIR: 
			in->i_op = &efs_dir_inode_operations; 
			break;
		case S_IFREG:
			in->i_op = &efs_file_inode_operations;
			break;
		case S_IFLNK:
			in->i_op = &efs_symlink_inode_operations;
			break;
		case S_IFCHR:
			in->i_rdev = device;
			in->i_op = &chrdev_inode_operations; 
			break;
		case S_IFBLK:
			in->i_rdev = device;
			in->i_op = &blkdev_inode_operations; 
			break;
		case S_IFIFO:
			init_fifo(in);
			break;    
		default:
			printk("EFS: unsupported inode mode %o\n",in->i_mode);
			goto read_inode_error;
			break;
	}

	return;
        
read_inode_error:
	printk("EFS: failed to read inode %lu\n", in->i_ino);
	in->i_mode = S_IFREG;
	in->i_atime = 0;
	in->i_ctime = 0;
	in->i_mtime = 0;
	in->i_nlink = 1;
	in->i_size = 0;
	in->i_blocks = 0;
	in->i_uid = 0;
	in->i_gid = 0;
	in->i_op = NULL;

	return;
}

static inline efs_block_t
efs_extent_check(efs_extent *ptr, efs_block_t block, struct efs_spb *sbi, struct efs_in_info *ini) {
	efs_block_t start;
	efs_block_t length;
	efs_block_t offset;

	/*
	** given an extent and a logical block within a file,
	** can this block be found within this extent ?
	*/
	start  = ptr->u1.s.ex_bn;
	length = ptr->u2.s.ex_length;
	offset = ptr->u2.s.ex_offset;

	if ((block >= offset) && (block < offset+length)) {
		return(sbi->fs_start + start + block - offset);
	} else {
		return 0;
	}
}

/* find the disk block number for a given logical file block number */

efs_block_t efs_read_block(struct inode *inode, efs_block_t block) {
	struct efs_spb *sbi = (struct efs_spb *) &inode->i_sb->u.generic_sbp;
	struct efs_in_info *ini = (struct efs_in_info *) &inode->u.generic_ip;
	struct buffer_head *bh;

	efs_block_t result = 0;
	int indirexts, indirext, imagic;
	efs_block_t istart, iblock, ilen;
	int i, last, total, checked;
	efs_extent *exts;
	efs_extent tmp;

	last  = ini->lastextent;
	total = ini->numextents;

	if (total <= EFS_DIRECTEXTENTS) {
		/* first check the last extent we returned */
		if ((result = efs_extent_check(&ini->extents[last], block, sbi, ini)))
			return result;
    
		/* if we only have one extent then nothing can be found */
		if (total == 1) {
			printk("EFS: read_block() failed to map (1 extent)\n");
			return 0;
		}

		/* check the stored extents in the inode */
		/* start with next extent and check forwards */
		for(i = 0; i < total - 1; i++) {
			if ((result = efs_extent_check(&ini->extents[(last + i) % total], block, sbi, ini))) {
				ini->lastextent = i;
				return result;
			}
		}

		printk("EFS: read_block() failed to map for direct extents\n");
		return 0;
	}

#ifdef DEBUG
	printk("EFS: indirect search for logical block %u\n", block);
#endif
	indirexts = ini->extents[0].u2.s.ex_offset;
	checked = 0;

	for(indirext = 0; indirext < indirexts; indirext++) {
		imagic = ini->extents[indirext].u1.s.ex_magic;
		istart = ini->extents[indirext].u1.s.ex_bn + sbi->fs_start;
		ilen   = ini->extents[indirext].u2.s.ex_length;

		for(iblock = istart; iblock < istart + ilen; iblock++) {
			bh = bread(inode->i_dev, iblock, EFS_BLOCKSIZE);
			if (!bh) {
				printk("EFS: bread() failed at block %d\n", block);
				return 0;
			}

			exts = (struct extent *) bh->b_data;
			for(i = 0; i < EFS_BLOCKSIZE / sizeof(efs_extent) && checked < total; i++, checked++) {
				tmp.u1.l = be32_to_cpu(exts[i].u1.l);
				tmp.u2.l = be32_to_cpu(exts[i].u2.l);

				if (tmp.u1.s.ex_magic != 0) {
					printk("EFS: extent %d has bad magic number in block %d\n", i, iblock);
					brelse(bh);
					return 0;
				}

				if ((result = efs_extent_check(&tmp, block, sbi, ini))) {
					brelse(bh);
					return result;
				}
			}
			brelse(bh);
			/* shouldn't need this if the FS is consistent */
			if (checked == total) {
				printk("EFS: unable to map (checked all extents)\n");
				return 0;
			}
		}
	}
	printk("EFS: unable to map (fell out of loop)\n");
	return 0;
}  

