/*
 * arch/mips/boot/milo.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Ralf Baechle
 * Copyright (C) 1993 by Hamish Macdonald
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>

#include "loader.h"

static int kfd;
static struct exec kexec;
static off_t filesize;
static struct nlist *syms;
static char *strs;
static long strsize, numsyms;

/*
 * Read a symbol from the kernel executable
 */
static struct nlist *aout_get_nlist(char *symbol)
{
    struct nlist *nl, *p = NULL;

    for (nl = syms; nl < syms + numsyms; nl++) {
	/*
	 * We accept only extern visible .text, .data and .bss symbols.
	 */
	if (strcmp (symbol, strs + nl->n_un.n_strx) == 0
	    && ((nl->n_type == N_TEXT | N_EXT) ||
	        (nl->n_type == N_DATA | N_EXT) ||
	        (nl->n_type == N_BSS  | N_EXT))) {
	    p = (struct nlist *)malloc (sizeof (struct nlist)
					+ strlen(strs + nl->n_un.n_strx) + 1);
	    if (!p)
		break;
	    *p = *nl;
	    p->n_un.n_name = (char *)(p+1);
	    strcpy (p->n_un.n_name, strs + nl->n_un.n_strx);
	}
    }
    return p;
}

/*
 * Return a pointer to the internal symbol
 */
static char *aout_ld_isymbol(char *symbol)
{
    static char isymbol[64];

    strcpy(isymbol, STR(C_LABEL_PREFIX));
    strcat(isymbol, symbol);

    return isymbol;
}

/*
 * Return base address for the loaded kernel
 */
static u_long aout_get_kbase(void)
{
    return (u_long) kexec.a_entry;
}

/*
 * Return size of kernel code + data
 */
static u_long aout_get_ksize(void)
{
    return (u_long) (kexec.a_text + kexec.a_data);
}

/*
 * Load a.out kernel into memory
 */
static int aout_load_kernel(void *mem)
{
    if (lseek (kfd, N_TXTOFF(kexec), L_SET) == -1)
    {
	fprintf (stderr, "Failed to seek to text\n\r");
	exit (EXIT_FAILURE);
    }
    if (read (kfd, mem, kexec.a_text) != kexec.a_text)
    {
        fprintf (stderr, "Failed to read text\n\r");
        exit (EXIT_FAILURE);
    }
    if (lseek (kfd, N_DATOFF(kexec), L_SET) == -1)
    {
        fprintf (stderr, "Failed to seek to data\n\r");
        exit (EXIT_FAILURE);
    }
    if (read (kfd, mem + kexec.a_text, kexec.a_data) != kexec.a_data)
    {
        fprintf (stderr, "Failed to read data\n\r");
        exit (EXIT_FAILURE);
    }
    close (kfd);

    return 0;
}

/*
 * Print some statistics about the kernel
 */
static void aout_print_stats(void)
{
    printf("Kernel text  at 0x%08lx, size %d bytes\n\r",
	   kexec.a_entry + N_TXTADDR(kexec), kexec.a_text);
    printf("Kernel data  at 0x%08lx, size %d bytes\n\r",
	   kexec.a_entry + N_DATADDR(kexec), kexec.a_data );
    printf("Kernel bss   at 0x%08lx, size %d bytes\n\r",
	   kexec.a_entry + N_BSSADDR(kexec), kexec.a_bss );
}

static int aout_open_kernel(char *kernel)
{
    int sfd;

    /*
     * open kernel executable and read exec header
     */
    if (debuglevel >= 2)
    {
        printf("aout_open_kernel(): Open kernel file\r\n");
        fflush(stdout);
    }  
    if ((kfd = open (kernel, O_RDONLY)) == -1)
    {
        printf ("Unable to open kernel file %s\r\n", kernel);
        exit (EXIT_FAILURE);
    }
    if (debuglevel >= 2)
    {
	printf("aout_open_kernel(): Reading exec header\r\n");
	fflush(stdout);
    }
    if (read (kfd, (void *)&kexec, sizeof(kexec)) != sizeof(kexec))
    {
        printf ("Unable to read exec header from %s\n\r", kernel);
        exit (EXIT_FAILURE);
    }

    /*
     * Is this really a kernel???
     * (My Mips docs mention SMAGIC, too, but don't tell about what
     * a SMAGIC file exactly is. If someone knows, please tell me -Ralf)
     *
     * FIXME: QMAGIC doesn't work yet.
     */
    if(N_MAGIC(kexec) != OMAGIC &&
       N_MAGIC(kexec) != NMAGIC &&
       N_MAGIC(kexec) != ZMAGIC &&
       N_MAGIC(kexec) != QMAGIC)
    {
        fprintf(stderr, "Kernel %s is no MIPS binary.\r\n", kernel);
        exit (EXIT_FAILURE);
    }
    if(N_MACHTYPE(kexec) != M_MIPS1 &&
       N_MACHTYPE(kexec) != M_MIPS2)
    {
        fprintf(stderr, "Kernel %s is no MIPS binary.\r\n", kernel);
        exit (EXIT_FAILURE);
    }

    /*
     * Read file's symbol table
     */
    /*
     * Open kernel executable and read exec header - we need to
     * use a second open file since the ARC standard doesn't
     * support reverse seeking on files which might run us in
     * trouble later on.
     */
    if (debuglevel >= 2)
    {
	printf("aout_open_kernel(): Open kernel file\r\n");
	fflush(stdout);
    }
    if ((sfd = open (kernel, O_RDONLY)) == -1)
    {
        printf ("Unable to open kernel %s for reading symbols.\r\n", kernel);
        exit (EXIT_FAILURE);
    }
    syms = (struct nlist *)malloc (kexec.a_syms);
    if (!syms)
    {
	return 0;
    }

    if (debuglevel >= 2)
    {
	printf("aout_open_kernel(): Seeking to symbol table\r\n");
	fflush(stdout);
    }
    lseek (sfd, N_SYMOFF(kexec), L_SET);
    if (debuglevel >= 2)
    {
	printf("aout_open_kernel(): Reading symbol table\r\n");
	fflush(stdout);
    }
    read (sfd, syms, kexec.a_syms);
    numsyms = kexec.a_syms / sizeof (struct nlist);
    filesize = lseek (sfd, 0L, L_XTND);
    strsize = filesize - N_STROFF(kexec);
    strs = (char *)malloc (strsize);

    if (!strs)
    {
	free (syms);
        syms = NULL;
	return 0;
    }

    lseek (sfd, N_STROFF(kexec), L_SET);
    read (sfd, strs, strsize);
    close(sfd);

    return 0;
}

static void aout_close_kernel(void)
{
    if (strs)
    {
        free (strs);
        strs = NULL;
    }
    if (syms)
    {
        free (syms);
        syms = NULL;
    }
    close(kfd);
}

struct loader loader_aout = {
    aout_get_nlist,
    aout_ld_isymbol,
    aout_get_kbase,
    aout_get_ksize,
    aout_load_kernel,
    aout_print_stats,
    aout_open_kernel,
    aout_close_kernel
};
