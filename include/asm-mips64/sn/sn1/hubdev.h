/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */

#ifndef _ASM_SN_SN1_HUBDEV_H
#define _ASM_SN_SN1_HUBDEV_H

extern void hubdev_init(void);
extern void hubdev_register(int (*attach_method)(dev_t));
extern int hubdev_unregister(int (*attach_method)(dev_t));
extern int hubdev_docallouts(vertex_hdl_t hub);

extern caddr_t hubdev_prombase_get(dev_t hub);
extern cnodeid_t hubdev_cnodeid_get(dev_t hub);

#endif /* _ASM_SN_SN1_HUBDEV_H */
