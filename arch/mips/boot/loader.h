/*
 * Defines for Linux/MIPS executable loaders
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 by Ralf Baechle
 */
#ifndef __STAND_LOADER
#define __STAND_LOADER

struct loader {
    struct nlist *(*ld_get_nlist)(char *symbol);
    char *(*ld_isymbol)(char *);
    u_long (*ld_get_kbase)(void);
    u_long (*ld_get_ksize)(void);
    int (*ld_load_kernel)(void *mem);
    void (*ld_print_stats)(void);
    int (*ld_open_kernel)(char *kernel);
    void (*ld_close_kernel)(void);
};

#endif /* __STAND_LOADER */
