/*
 *  fs/partitions/ultrix.h
 */

static int ultrix_partition(struct gendisk *hd, kdev_t dev,
                            unsigned long first_sector);

#define SGI_LABEL_MAGIC 0x0be5a941

