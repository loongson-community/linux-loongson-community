/*
 * linux/include/asm-mips/namei.h
 *
 * Included from linux/fs/namei.c
 */
#ifndef __ASM_MIPS_NAMEI_H
#define __ASM_MIPS_NAMEI_H

#include <linux/config.h>

#ifdef CONFIG_BINFMT_IRIX

/* Only one at this time. */
#define IRIX32_EMUL "/usr/gnemul/irix"

#define translate_namei(pathname, base, follow_links, res_inode) ({					\
	if ((current->personality == PER_IRIX32) && !base && *pathname == '/') {			\
		struct inode *emul_ino;									\
		int namelen;										\
		const char *name;									\
													\
		while (*pathname == '/')								\
			pathname++;									\
		current->fs->root->i_count++;								\
		if (dir_namei (IRIX32_EMUL, 								\
				&namelen, &name, current->fs->root, &emul_ino) >= 0 && emul_ino) {	\
			*res_inode = NULL;								\
			if (_namei (pathname, emul_ino, follow_links, res_inode) >= 0 && *res_inode) 	\
				return 0;								\
		}											\
		base = current->fs->root;								\
		base->i_count++;									\
	}												\
})

#define translate_open_namei(pathname, flag, mode, res_inode, base) ({					\
	if ((current->personality == PER_IRIX32) && !base && *pathname == '/') {			\
		struct inode *emul_ino;									\
		int namelen;										\
		const char *name;									\
													\
		while (*pathname == '/')								\
			pathname++;									\
		current->fs->root->i_count++;								\
		if (dir_namei (IRIX32_EMUL, 								\
				&namelen, &name, current->fs->root, &emul_ino) >= 0 && emul_ino) {	\
			*res_inode = NULL;								\
			if (open_namei (pathname, flag, mode, res_inode, emul_ino) >= 0 && *res_inode)	\
				return 0;								\
		}											\
		base = current->fs->root;								\
		base->i_count++;									\
	}												\
})

#else /* !defined(CONFIG_BINFMT_IRIX) */

#define translate_namei(pathname, base, follow_links, res_inode)	\
	do { } while (0)

#define translate_open_namei(pathname, flag, mode, res_inode, base) \
	do { } while (0)

#endif /* !defined(CONFIG_BINFMT_IRIX) */

#endif /* __ASM_MIPS_NAMEI_H */
