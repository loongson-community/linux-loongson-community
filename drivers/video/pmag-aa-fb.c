/*
 *	linux/drivers/video/pmag-aa-fb.c
 *	Copyright 2002 Karsten Merker <merker@debian.org>
 *
 *	PMAG-AA TurboChannel framebuffer card support ... derived from
 *	pmag-ba-fb.c, which is Copyright (C) 1999, 2000, 2001 by
 *	Michael Engel <engel@unix-ag.org>, Karsten Merker <merker@debian.org>
 *	and Harald Koerfgen <hkoerfg@web.de>, which itself is derived from
 *	"HP300 Topcat framebuffer support (derived from macfb of all things)
 *	Phil Blundell <philb@gnu.org> 1998"
 *
 *	This file is subject to the terms and conditions of the GNU General
 *	Public License.  See the file COPYING in the main directory of this
 *	archive for more details.
 *
 *	Version 0.01 2002/09/28 first try to get a PMAG-AA running
 * 
 *	2003/02/24  Thiemo Seufer  <seufer@csv.ica.uni-stuttgart.de>
 *		Code cleanup.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/fb.h>

#include <asm/bootinfo.h>
#include <asm/dec/machtype.h>
#include <asm/dec/tc.h>

#include <video/fbcon.h>
#include <video/fbcon-mfb.h>
#include <video/fbcon-cfb8.h>

#include "bt455.h"
#include "bt431.h"

/* Version information */
#define DRIVER_VERSION "v0.02"
#define DRIVER_AUTHOR "Karsten Merker <merker@linuxtag.org>"
#define DRIVER_DESC "PMAG-AA Framebuffer Driver"

/*
 * Bt455 RAM DAC register base offset (rel. to TC slot base address).
 */
#define PMAG_AA_BT455_OFFSET		0x100000

/*
 * Bt431 cursor generator offset (rel. to TC slot base address).
 */
#define PMAG_AA_BT431_OFFSET		0x180000

/*
 * Begin of PMAG-AA framebuffer memory relative to TC slot address,
 * resolution is 1280x1024x1 (8 bits deep, but only LSB is used).
 */
#define PMAG_AA_ONBOARD_FBMEM_OFFSET	0x200000

struct aafb_info {
	struct fb_info info;
	struct display disp;
	struct bt455_regs *bt455;
	struct bt431_regs *bt431;
	unsigned long fb_start;
	unsigned long fb_size;
	unsigned long fb_line_length;
};

/*
 * Max 3 TURBOchannel slots -> max 3 PMAG-AA.
 */
static struct aafb_info my_fb_info[3];

static struct aafb_par {
} current_par;

static int currcon = 0;

static void aafb_get_par(struct aafb_par *par)
{
	*par = current_par;
}

static void aafb_encode_fix(struct fb_fix_screeninfo *fix,
			    struct aafb_par *par, struct aafb_info *info)
{
	memset(fix, 0, sizeof(struct fb_fix_screeninfo));
	strcpy(fix->id, "PMAG-AA");

	fix->smem_start = info->fb_start;
	fix->smem_len = info->fb_size;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->visual = FB_VISUAL_MONO10;
	fix->line_length = info->fb_line_length;
	fix->accel = FB_ACCEL_NONE;
}

static int aafb_get_fix(struct fb_fix_screeninfo *fix, int con,
			struct fb_info *info)
{
	struct aafb_par par;

	aafb_get_par(&par);
	aafb_encode_fix(fix, &par, (struct aafb_info *) info);

	return 0;
}

static void aafb_set_disp(int con, struct fb_info *info)
{
	struct fb_fix_screeninfo fix;
	struct display *disp = (con < 0) ? info->disp : (fb_display + con);

	aafb_get_fix(&fix, con, info);

	disp->screen_base = (char *) fix.smem_start;
	disp->visual = fix.visual;
	disp->type = fix.type;
	disp->type_aux = fix.type_aux;
	disp->ypanstep = fix.ypanstep;
	disp->ywrapstep = fix.ywrapstep;
//	disp->line_length = fix.line_length;
	disp->next_line = fix.line_length;
	disp->can_soft_blank = 1;
	disp->inverse = 0;
//	disp->scrollmode = SCROLL_YREDRAW;
	disp->dispsw = &fbcon_cfb8;
}

static int aafb_get_cmap(struct fb_cmap *cmap, int kspc, int con,
			 struct fb_info *info)
{
	static u16 color[2] = {0x0000, 0x000f};
	static struct fb_cmap aafb_cmap = {0, 2, color, color, color, NULL};

	fb_copy_cmap(&aafb_cmap, cmap, kspc ? 0 : 2);
	return 0;
}

static int aafb_set_cmap(struct fb_cmap *cmap, int kspc, int con,
			 struct fb_info *info)
{
	return -EINVAL;
}

static int aafb_switch(int con, struct fb_info *info)
{
	/* Set the current console. */
	currcon = con;
	aafb_set_disp(con, info);

	return 0;
}

static void aafb_encode_var(struct fb_var_screeninfo *var,
			    struct aafb_par *par)
{
	var->xres = 1280;
	var->yres = 1024;
	var->xres_virtual = 2048;
	var->yres_virtual = 1024;
	var->xoffset = 0;
	var->yoffset = 0;
	var->bits_per_pixel = 8;
	var->grayscale = 1;
	var->red.offset = 0;
	var->red.length = 0;
	var->red.msb_right = 0;
	var->green.offset = 0;
	var->green.length = 1;
	var->green.msb_right = 0;
	var->blue.offset = 0;
	var->blue.length = 0;
	var->blue.msb_right = 0;
	var->transp.offset = 0;
	var->transp.length = 0;
	var->transp.msb_right = 0;
	var->nonstd = 0;
	var->activate &= ~FB_ACTIVATE_MASK & FB_ACTIVATE_NOW;
	var->accel_flags = 0;
	var->sync = FB_SYNC_ON_GREEN;
	var->vmode &= ~FB_VMODE_MASK & FB_VMODE_NONINTERLACED;
}

static int aafb_get_var(struct fb_var_screeninfo *var, int con,
			struct fb_info *info)
{
	if (con < 0) {
		struct aafb_par par;

		memset(var, 0, sizeof(struct fb_var_screeninfo));
		aafb_get_par(&par);
		aafb_encode_var(var, &par);
	} else
		*var = info->var;

	return 0;
}

static int aafb_set_var(struct fb_var_screeninfo *var, int con,
			struct fb_info *info)
{
	struct aafb_par par;

	aafb_get_par(&par);
	aafb_encode_var(var, &par);
	info->var = *var;

	return 0;
}

static int aafb_update_var(int con, struct fb_info *info)
{
	return 0;
}

/* 0 unblanks, any other blanks. */

static void aafb_blank(int blank, struct fb_info *info)
{
	struct aafb_info *ip = (struct aafb_info *)info;
	u8 val = blank ? 0x00 : 0x0f;

	bt455_write_cmap_entry(ip->bt455, 1, val, val, val);
}

static struct fb_ops aafb_ops = {
	owner:THIS_MODULE,
	fb_get_fix:aafb_get_fix,
	fb_get_var:aafb_get_var,
	fb_set_var:aafb_set_var,
	fb_get_cmap:aafb_get_cmap,
	fb_set_cmap:aafb_set_cmap
};

static int __init init_one(int slot)
{
	unsigned long base_addr = get_tc_base_addr(slot);
	struct aafb_info *ip = &my_fb_info[slot];

	memset(ip, 0, sizeof(struct aafb_info));

	/*
	 * Framebuffer display memory base address and friends.
	 */
	ip->bt455 = (struct bt455_regs *) (base_addr + PMAG_AA_BT455_OFFSET);
	ip->bt431 = (struct bt431_regs *) (base_addr + PMAG_AA_BT431_OFFSET);
	ip->fb_start = base_addr + PMAG_AA_ONBOARD_FBMEM_OFFSET;
	ip->fb_size = 2048 * 1024; /* fb_fix_screeninfo.smem_length
				      seems to be physical */
	ip->fb_line_length = 2048;

	/*
	 * Configure the RAM DACs.
	 */
// TODO
//	bt455_erase_cursor(ip->bt455);
//	bt455_set_cursor(ip->bt455);
//	bt431_erase_cursor(ip->bt431);
//	bt431_init_cursor(ip->bt431);
//	bt431_position_cursor(ip->bt431, 16, 16);

	/* Init colormap. */
	bt455_write_cmap_entry(ip->bt455, 0, 0x00, 0x00, 0x00);
	bt455_write_cmap_entry(ip->bt455, 1, 0x0f, 0x0f, 0x0f);

	/*
	 * Let there be consoles..
	 */
	strcpy(ip->info.modename, "PMAG-AA");
	ip->info.node = -1;
	ip->info.flags = FBINFO_FLAG_DEFAULT;
	ip->info.fbops = &aafb_ops;
	ip->info.disp = &ip->disp;
	ip->info.changevar = NULL;
	ip->info.switch_con = &aafb_switch;
	ip->info.updatevar = &aafb_update_var;
	ip->info.blank = &aafb_blank;

	aafb_set_disp(-1, &ip->info);
	aafb_set_var(&ip->disp.var, -1, &ip->info);

	/* Clear the screen. */
	memset ((void *)ip->fb_start, 0, ip->fb_size);

	if (register_framebuffer(&ip->info) < 0)
		return -EINVAL;

	printk(KERN_INFO "fb%d: %s frame buffer device in TC slot %d\n",
	       GET_FB_IDX(ip->info.node), ip->info.modename, slot);

	return 0;
}

static int __exit exit_one(int slot)
{
	struct aafb_info *ip = &my_fb_info[slot];

	if (unregister_framebuffer(&ip->info) < 0)
		return -EINVAL;

	return 0;
}

/* 
 * Initialise the framebuffer.
 */
int __init pmagaafb_init(void)
{
	int sid;
	int found = 0;

	while ((sid = search_tc_card("PMAG-AA")) >= 0) {
		found = 1;
		claim_tc_card(sid);
		init_one(sid);
	}

	return found ? 0 : -ENODEV;
}

static void __exit pmagaafb_exit(void)
{
	int sid;

	while ((sid = search_tc_card("PMAG-AA")) >= 0) {
		exit_one(sid);
		release_tc_card(sid);
	}
}

MODULE_LICENSE("GPL");
