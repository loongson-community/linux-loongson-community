/*
 *  fs/partitions/ultrix.h
 */

#define PT_MAGIC	0x032957	/* Partition magic number */
#define PT_VALID	1		/* Indicates if struct is valid */

#define	SBLOCK	((unsigned long)((16384 - sizeof(struct ultrix_disklabel)) \
                  /get_ptable_blocksize(dev)))

int ultrix_partition(struct gendisk *hd, kdev_t dev,
		  unsigned long first_sector, int first_part_minor);

