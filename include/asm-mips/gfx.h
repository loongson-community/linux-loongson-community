/*
 * SGI GFX interface
 */

/* Applications depend on these defines/ioctls */

#define GFX_BASE             100
#define GFX_GETNUM_BOARDS    (GFX_BASE + 1)
#define GFX_GETBOARD_INFO    (GFX_BASE + 2)

#define GFX_INFO_NAME_SIZE  16
#define GFX_INFO_LABEL_SIZE 16

struct gfx_info {
	char name  [GFX_INFO_NAME_SIZE];  /* board name */
	char label [GFX_INFO_LABEL_SIZE]; /* label name */
	unsigned short int xpmax, ypmax;  /* screen resolution */
	unsigned int lenght;	          /* size of a complete gfx_info for this board */
};

struct gfx_getboardinfo_args {
	unsigned int board;     /* board number.  starting from zero */
	void *buf;              /* pointer to gfx_info */
	unsigned int len;       /* buffer size of buf */
};

