/*
 * efs_super.h
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from IRIX header files (c) 1988 Silicon Graphics
 */

#ifndef __EFS_SUPER_H__
#define __EFS_SUPER_H__

/* statfs() magic number for EFS */
#define EFS_SUPER_MAGIC	0x414A83

/* EFS superblock magic numbers */
#define EFS_MAGIC	0x072959
#define EFS_NEWMAGIC	0x07295a

#define IS_EFS_MAGIC(x)	((x == EFS_MAGIC) || (x == EFS_NEWMAGIC))

#define EFS_SUPER		1
#define EFS_ROOTINODE		2
#define EFS_BLOCKSIZE		512
#define EFS_BLOCKSIZE_BITS	9

struct efs {
	int32_t		fs_size;        /* size of filesystem, in sectors */
	int32_t		fs_firstcg;     /* bb offset to first cg */
	int32_t		fs_cgfsize;     /* size of cylinder group in bb's */
	short		fs_cgisize;     /* bb's of inodes per cylinder group */
	short		fs_sectors;     /* sectors per track */
	short		fs_heads;       /* heads per cylinder */
	short		fs_ncg;         /* # of cylinder groups in filesystem */
	short		fs_dirty;       /* fs needs to be fsck'd */
	int32_t		fs_time;        /* last super-block update */
	int32_t		fs_magic;       /* magic number */
	char		fs_fname[6];    /* file system name */
	char		fs_fpack[6];    /* file system pack name */
	int32_t		fs_bmsize;      /* size of bitmap in bytes */
	int32_t		fs_tfree;       /* total free data blocks */
	int32_t		fs_tinode;      /* total free inodes */
	int32_t		fs_bmblock;     /* bitmap location. */
	int32_t		fs_replsb;      /* Location of replicated superblock. */
	int32_t		fs_lastialloc;  /* last allocated inode */
	char		fs_spare[20];   /* space for expansion - MUST BE ZERO */
	int32_t		fs_checksum;    /* checksum of volume portion of fs */
};

/* efs superblock information */
struct efs_spb {
	int32_t	fs_magic;	/* first block of filesystem */
	int32_t	fs_start;	/* first block of filesystem */
	int32_t	total_blocks;	/* total number of blocks in filesystem */
	int32_t	first_block;	/* first data block in filesystem */
	int32_t	group_size;	/* number of blocks a group consists of */ 
	short	inode_blocks;	/* number of blocks used for inodes in every grp */
	short	total_groups;	/* number of groups */
};

#endif /* __EFS_SUPER_H__ */

