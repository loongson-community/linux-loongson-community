/*
 *	linux/drivers/video/bt431.h
 *
 *	Copyright 2003  Thiemo Seufer <seufer@csv.ica.uni-stuttgart.de>
 *
 *	This file is subject to the terms and conditions of the GNU General
 *	Public License. See the file COPYING in the main directory of this
 *	archive for more details.
 */
#include <linux/types.h>
#include <asm/system.h>

/*
 * Bt431 cursor generator registers, 32-bit aligned.
 * Two twin Bt431 are used on the DECstation's PMAG-AA.
 */
struct bt431_regs {
	volatile u16 addr_lo;
	u16 pad0;
	volatile u16 addr_hi;
	u16 pad1;
	volatile u16 addr_cmap;
	u16 pad2;
	volatile u16 addr_reg;
	u16 pad3;
};

static inline u16 bt431_set_value(u8 val)
{
	return ((val << 8) | (val & 0xff)) & 0xffff;
}

static inline u8 bt431_get_value(u16 val)
{
	return val & 0xff;
}

/*
 * Additional registers addressed indirectly.
 */
#define BT431_REG_CMD		0x0000
#define BT431_REG_CXLO		0x0001
#define BT431_REG_CXHI		0x0002
#define BT431_REG_CYLO		0x0003
#define BT431_REG_CYHI		0x0004
#define BT431_REG_WXLO		0x0005
#define BT431_REG_WXHI		0x0006
#define BT431_REG_WYLO		0x0007
#define BT431_REG_WYHI		0x0008
#define BT431_REG_WWLO		0x0009
#define BT431_REG_WWHI		0x000a
#define BT431_REG_WHLO		0x000b
#define BT431_REG_WHHI		0x000c

#define BT431_REG_CRAM_BASE	0x0000
#define BT431_REG_CRAM_END	0x01ff

/*
 * Command register.
 */
#define BT431_CMD_CURS_ENABLE	0x40
#define BT431_CMD_XHAIR_ENABLE	0x20
#define BT431_CMD_OR_CURSORS	0x10
#define BT431_CMD_AND_CURSORS	0x00
#define BT431_CMD_1_1_MUX	0x00
#define BT431_CMD_4_1_MUX	0x04
#define BT431_CMD_5_1_MUX	0x08
#define BT431_CMD_xxx_MUX	0x0c
#define BT431_CMD_THICK_1	0x00
#define BT431_CMD_THICK_3	0x01
#define BT431_CMD_THICK_5	0x02
#define BT431_CMD_THICK_7	0x03

static inline void bt431_select_reg(struct bt431_regs *regs, int ir)
{
	mb();
	regs->addr_lo = bt431_set_value(ir & 0xff);
	regs->addr_hi = bt431_set_value((ir >> 8) & 0xff);
}

/* Autoincrement read/write. */
static inline u8 bt431_read_reg_inc(struct bt431_regs *regs)
{
	mb();
	return bt431_get_value(regs->addr_reg);
}

static inline void bt431_write_reg_inc(struct bt431_regs *regs, u8 value)
{
	mb();
	regs->addr_reg = bt431_set_value(value);
}

static inline u8 bt431_read_reg(struct bt431_regs *regs, int ir)
{
	bt431_select_reg(regs, ir);
	return bt431_read_reg_inc(regs);
}

static inline void bt431_write_reg(struct bt431_regs *regs, int ir, u16 value)
{
	bt431_select_reg(regs, ir);
	bt431_write_reg_inc(regs, value);
}

/* Autoincremented read/write for the cursor map */
static inline u16 bt431_read_cmap_inc(struct bt431_regs *regs)
{
	mb();
	return regs->addr_cmap;
}

static inline void bt431_write_cmap_inc(struct bt431_regs *regs, u16 value)
{
	mb();
	regs->addr_cmap = value;
}

static inline u16 bt431_read_cmap(struct bt431_regs *regs, int cr)
{
	bt431_select_reg(regs, cr);
	return bt431_read_cmap_inc(regs);
}

static inline void bt431_write_cmap(struct bt431_regs *regs, int cr, u16 value)
{
	bt431_select_reg(regs, cr);
	bt431_write_cmap_inc(regs, value);
}

static inline void bt431_enable_cursor(struct bt431_regs *regs)
{
/*	bt431_write_reg(regs, BT431_REG_CMD,
			BT431_CMD_CURS_ENABLE | BT431_CMD_OR_CURSORS
			| BT431_CMD_4_1_MUX | BT431_CMD_THICK_1);
*/	bt431_write_reg(regs, BT431_REG_CMD, BT431_CMD_CURS_ENABLE);
}

static inline void bt431_erase_cursor(struct bt431_regs *regs)
{
	bt431_write_reg(regs, BT431_REG_CMD, BT431_CMD_4_1_MUX);
}

static inline void bt431_position_cursor(struct bt431_regs *regs, u16 x, u16 y)
{
	/*
	 * Magic from the MACH sources.
	 * 
	 * Cx = x + D + H - P
	 *  P = 37 if 1:1, 52 if 4:1, 57 if 5:1
	 *  D = pixel skew between outdata and external data
	 *  H = pixels between HSYNCH falling and active video
	 *
	 * Cy = y + V - 32
	 *  V = scanlines between HSYNCH falling, two or more
	 *      clocks after VSYNCH falling, and active video
	 */
	x += 412 - 52;
	y += 68 - 32;

	/* Use autoincrement. */
	bt431_select_reg(regs, BT431_REG_CXLO);
	bt431_write_reg_inc(regs, x & 0xff); /* BT431_REG_CXLO */
	bt431_write_reg_inc(regs, (x >> 8) & 0x0f); /* BT431_REG_CXHI */
	bt431_write_reg_inc(regs, y & 0xff); /* BT431_REG_CYLO */
	bt431_write_reg_inc(regs, (y >> 8) & 0x0f); /* BT431_REG_CYHI */
}

/*u16 _bt431_default_cursor[64 * 8] = {
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0xffff, 0, 0, 0, 0, 0, 0, 0,
	0,
};
*/
static inline void bt431_load_cursor_sprite(struct bt431_regs *regs)
{
	int i;

	bt431_select_reg(regs, BT431_REG_CRAM_BASE);
	for (i = 0; i < 64 * 8; i++)
		bt431_write_cmap_inc(regs, ((i < 16 * 8) && (i % 8)) ? 0xffff : 0);
}

static inline void bt431_init_cursor(struct bt431_regs *regs)
{
	bt431_write_reg(regs, BT431_REG_CMD,
			BT431_CMD_CURS_ENABLE | BT431_CMD_OR_CURSORS
			| BT431_CMD_4_1_MUX | BT431_CMD_THICK_1);

	/* home cursor */
#if 0
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_CXLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_CXHI */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_CYLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_CYHI */
#endif
	bt431_write_reg_inc(regs, 0x80); /* BT431_REG_CXLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_CXHI */
	bt431_write_reg_inc(regs, 0x80); /* BT431_REG_CYLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_CYHI */

	/* no crosshair window */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WXLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WXHI */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WYLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WYHI */
//	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WWLO */
	bt431_write_reg_inc(regs, 0x01); /* BT431_REG_WWLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WWHI */
//	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WHLO */
	bt431_write_reg_inc(regs, 0x01); /* BT431_REG_WHLO */
	bt431_write_reg_inc(regs, 0x00); /* BT431_REG_WHHI */

	bt431_load_cursor_sprite(regs);
}
