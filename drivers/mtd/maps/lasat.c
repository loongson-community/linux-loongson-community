/*
 * Flash device on lasat 100 and 200 boards
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/config.h>
#include <asm/lasat/lasat.h>
#include <linux/init.h>

static struct mtd_info *lasat_mtd;

static struct map_info lasat_map = {
	name: "LASAT flash",
	buswidth: 4,
};

static struct mtd_partition partition_info[LASAT_MTD_LAST];
static char *lasat_mtd_partnames[] = {"Bootloader", "Service", "Normal", "Config", "Filesystem"};

static int __init init_sp(void)
{
	int i;
	int nparts = 0;
	/* this does not play well with the old flash code which 
	 * protects and uprotects the flash when necessary */
       	printk(KERN_NOTICE "Unprotecting flash\n");
	*lasat_misc->flash_wp_reg |= 1 << lasat_misc->flash_wp_bit;

	lasat_map.phys = lasat_flash_partition_start(LASAT_MTD_BOOTLOADER);
	lasat_map.virt = (unsigned long)ioremap_nocache(
			lasat_map.phys, lasat_board_info.li_flash_size);
	lasat_map.size = lasat_board_info.li_flash_size;

       	printk(KERN_NOTICE "sp flash device: %lx at %lx\n", 
			lasat_map.size, lasat_map.phys);

	for (i=0; i < LASAT_MTD_LAST; i++)
		partition_info[i].name = lasat_mtd_partnames[i];

	lasat_mtd = do_map_probe("cfi_probe", &lasat_map);
	if (lasat_mtd) {
		u32 size, offset = 0;

		lasat_mtd->owner = THIS_MODULE;

		for (i=0; i < LASAT_MTD_LAST; i++) {
			size = lasat_flash_partition_size(i);
			if (size != 0) {
				nparts++;
				partition_info[i].size = size;
				partition_info[i].offset = offset;
				offset += size;
			}
		}

		add_mtd_partitions( lasat_mtd, partition_info, nparts );
		return 0;
	}

	return -ENXIO;
}

static void __exit cleanup_sp(void)
{
	if (lasat_mtd) {
	  del_mtd_partitions(lasat_mtd);
	  map_destroy(lasat_mtd);
	}
}

module_init(init_sp);
module_exit(cleanup_sp);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brian Murphy <brian@murphy.dk>");
MODULE_DESCRIPTION("Lasat Safepipe/Masquerade MTD map driver");
