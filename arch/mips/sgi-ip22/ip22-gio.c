/* 
 * ip22-gio.c: Support for GIO64 bus (inspired by PCI code)
 * 
 * Copyright (C) 2002 Ladislav Michl
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

#include <asm/sgi/sgimc.h>
#include <asm/sgi/sgigio.h>

#define GIO_PIO_MAP_BASE	0x1f000000L
#define GIO_PIO_MAP_SIZE	(16 * 1024*1024)

#define GIO_ADDR_GFX		0x1f000000L
#define GIO_ADDR_GIO1		0x1f400000L
#define GIO_ADDR_GIO2		0x1f600000L

#define GIO_GFX_MAP_SIZE	(4 * 1024*1024)
#define GIO_GIO1_MAP_SIZE	(2 * 1024*1024)
#define GIO_GIO2_MAP_SIZE	(4 * 1024*1024)

#define GIO_NO_DEVICE		0x80

static struct gio_dev gio_slot[GIO_NUM_SLOTS] = {
	{
		0,
		0,
		0,
		GIO_NO_DEVICE,
		GIO_SLOT_GFX,
		GIO_ADDR_GFX,
		GIO_GFX_MAP_SIZE,
		NULL,
		"GFX"
	},
	{
		0,
		0,
		0,
		GIO_NO_DEVICE,
		GIO_SLOT_GIO1,
		GIO_ADDR_GIO1,
		GIO_GIO1_MAP_SIZE,
		NULL,
		"EXP0"
	},
	{
		0,
		0,
		0,
		GIO_NO_DEVICE,
		GIO_SLOT_GIO2,
		GIO_ADDR_GIO2,
		GIO_GIO2_MAP_SIZE,
		NULL,
		"EXP1"
	}
};

static int gio_read_proc(char *buf, char **start, off_t off,
			 int count, int *eof, void *data)
{
	int i;
	char *p = buf;
	
	p += sprintf(p, "GIO devices found:\n");
	for (i = 0; i < GIO_NUM_SLOTS; i++) {
		if (gio_slot[i].flags & GIO_NO_DEVICE)
			continue;
		p += sprintf(p, "  Slot %s, DeviceId 0x%02x\n",
			     gio_slot[i].slot_name, gio_slot[i].device);
		p += sprintf(p, "    BaseAddr 0x%08lx, MapSize 0x%08x\n",
			     gio_slot[i].base_addr, gio_slot[i].map_size);
	}
	
	return p - buf;
}

void create_gio_proc_entry(void)
{
	create_proc_read_entry("gio", 0, NULL, gio_read_proc, NULL);
}

/**
 * gio_find_device - begin or continue searching for a GIO device by device id
 * @device: GIO device id to match, or %GIO_ANY_ID to match all device ids
 * @from: Previous GIO device found in search, or %NULL for new search.
 *
 * Iterates through the list of known GIO devices. If a GIO device is found
 * with a matching @device, a pointer to its device structure is returned. 
 * Otherwise, %NULL is returned.
 * A new search is initiated by passing %NULL to the @from argument.
 * Otherwise if @from is not %NULL, searches continue from next device.
 */
struct gio_dev *
gio_find_device(unsigned char device, const struct gio_dev *from)
{
	int i;
	
	for (i = (from) ? from->slot_number : 0; i < GIO_NUM_SLOTS; i++)
		if (!(gio_slot[i].flags & GIO_NO_DEVICE) && 
		   (device == GIO_ANY_ID || device == gio_slot[i].device))
			return &gio_slot[i];
	
	return NULL;
}

#define GIO_IDCODE(x)		(x & 0x7f)
#define GIO_ALL_BITS_VALID	0x80
#define GIO_REV(x)		((x >> 8) & 0xff)
#define GIO_GIO_SIZE_64		0x10000
#define GIO_ROM_PRESENT		0x20000
#define GIO_VENDOR_CODE(x)	((x >> 18) & 0x3fff)

extern int ip22_baddr(unsigned int *val, unsigned long addr);

/** 
 * sgigio_init - scan the GIO space and figure out what hardware is actually
 * present.
 */
void __init sgigio_init(void)
{
	unsigned int i, id, found = 0;

	printk("GIO: Scanning for GIO cards...\n");
	for (i = 0; i < GIO_NUM_SLOTS; i++) {
		if (ip22_baddr(&id, KSEG1ADDR(gio_slot[i].base_addr)))
			continue;

		found = 1;
		gio_slot[i].device = GIO_IDCODE(id);
		if (id & GIO_ALL_BITS_VALID) {
			gio_slot[i].revision = GIO_REV(id);
			gio_slot[i].vendor = GIO_VENDOR_CODE(id);
			gio_slot[i].flags =
				(id & GIO_GIO_SIZE_64) ? GIO_IFACE_64 : 0 |
				(id & GIO_ROM_PRESENT) ? GIO_HAS_ROM : 0;
		} else
			gio_slot[i].flags = GIO_VALID_ID_ONLY;

		printk("GIO: Card 0x%02x @ 0x%08lx\n", gio_slot[i].device,
			gio_slot[i].base_addr);
	}
	
	if (!found)
		printk("GIO: No GIO cards present.\n");
}
