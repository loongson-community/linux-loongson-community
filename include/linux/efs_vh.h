/*
 * efs_vh.h
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from IRIX header files (c) 1985 MIPS Computer Systems, Inc.
 */

#ifndef __EFS_VH_H__
#define __EFS_VH_H__

#define VHMAGIC		0xbe5a941	/* volume header magic number */
#define NPARTAB		16		/* 16 unix partitions */
#define NVDIR		15		/* max of 15 directory entries */
#define BFNAMESIZE	16		/* max 16 chars in boot file name */
#define VDNAMESIZE	8

struct volume_directory {
	char	vd_name[VDNAMESIZE];	/* name */
	int	vd_lbn;			/* logical block number */
	int	vd_nbytes;		/* file length in bytes */
};

struct partition_table {	/* one per logical partition */
	int	pt_nblks;	/* # of logical blks in partition */
	int	pt_firstlbn;	/* first lbn of partition */
	int	pt_type;	/* use of partition */
};

struct volume_header {
	int vh_magic;				/* identifies volume header */
	short vh_rootpt;			/* root partition number */
	short vh_swappt;			/* swap partition number */
	char vh_bootfile[BFNAMESIZE];		/* name of file to boot */
	char pad[48];				/* space for device params */
	struct volume_directory vh_vd[NVDIR];	/* other vol hdr contents */
	struct partition_table vh_pt[NPARTAB];	/* device partition layout */
	int vh_csum;				/* volume header checksum */
	int vh_fill;				/* fill out to 512 bytes */
};

#endif /* __EFS_VH_H__ */

