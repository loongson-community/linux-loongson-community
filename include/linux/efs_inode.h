/*
 * efs_ino.h
 *
 * Copyright (c) 1999 Al Smith
 *
 * Portions derived from IRIX header files (c) 1988 Silicon Graphics
 */

#ifndef	__EFS_INO_
#define	__EFS_INO_

typedef	int32_t		efs_block_t;
typedef uint32_t	efs_ino_t;

/* this is very icky */
union extent1 {
#ifdef __LITTLE_ENDIAN
	struct s1 {
		unsigned int	ex_bn:24;	/* basic block # */
		unsigned int	ex_magic:8;	/* magic # (zero) */
	} s;
#else
#ifdef __BIG_ENDIAN
	struct s1 {
		unsigned int	ex_magic:8;	/* magic # (zero) */
		unsigned int	ex_bn:24;	/* basic block # */
	} s;
#else
#error system endianness is undefined
#endif
#endif
	unsigned long l;
} extent1;

union extent2 {
#ifdef __LITTLE_ENDIAN
	struct s2 {
		unsigned int	ex_offset:24; /* logical bb offset into file */
		unsigned int	ex_length:8;  /* numblocks in this extent */
	} s;
#else
#ifdef __BIG_ENDIAN
	struct s2 {
		unsigned int	ex_length:8;  /* numblocks in this extent */
		unsigned int	ex_offset:24; /* logical bb offset into file */
	} s;
#else
#error system endianness is undefined
#endif
#endif
	unsigned long l;
} extent2;

/*
 * layout of an extent, in memory and on disk. 8 bytes exactly
 */
typedef struct	extent {
	union extent1 u1;
	union extent2 u2;
} efs_extent;

#define	EFS_DIRECTEXTENTS	12

/*
 * extent based filesystem inode as it appears on disk.  The efs inode
 * is exactly 128 bytes long.
 */
struct	efs_dinode {
	u_short		di_mode;	/* mode and type of file */
	short		di_nlink;	/* number of links to file */
	u_short		di_uid;		/* owner's user id */
	u_short		di_gid;		/* owner's group id */
	int32_t		di_size;	/* number of bytes in file */
	int32_t		di_atime;	/* time last accessed */
	int32_t		di_mtime;	/* time last modified */
	int32_t		di_ctime;	/* time created */
	uint32_t	di_gen;		/* generation number */
	short		di_numextents;	/* # of extents */
	u_char		di_version;	/* version of inode */
	u_char		di_spare;	/* spare - used by AFS */
	union di_addr {
		efs_extent di_extents[EFS_DIRECTEXTENTS];
		dev_t	di_dev;		/* device for IFCHR/IFBLK */
	} di_u;
};

#endif	/* __EFS_INO_ */

