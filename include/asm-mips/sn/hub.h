#ifndef __ASM_SN_HUB_H
#define __ASM_SN_HUB_H

#include <linux/types.h>
#include <asm/sn/types.h>
#include <asm/sn/io.h>
#include <asm/sn/klkernvars.h>
#include <asm/xtalk/xtalk.h>

struct hub_data {
	kern_vars_t	kern_vars;
	DECLARE_BITMAP  (h_bigwin_used, HUB_NUM_BIG_WINDOW);
};

extern struct hub_data *hub_data[];
#define HUB_DATA(n)		(hub_data[(n)])

/* ip27-hubio.c */
extern unsigned long hub_pio_map(cnodeid_t cnode, xwidgetnum_t widget,
			  unsigned long xtalk_addr, size_t size);
extern void hub_pio_init(cnodeid_t cnode);
#endif /* __ASM_SN_HUB_H */
