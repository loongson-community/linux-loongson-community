/*
 * Load and launch Linux/MIPS kernel.
 *
 * Copyright (C) 1994, 1995 by Waldorf Electronics,
 * written by Ralf Baechle and Andreas Busse
 *
 * Loosly based on bootstrap.c for Linux/68k,
 * Copyright (C) 1993 by Hamish Macdonald, Greg Harp
 *
 * This file is subject to the terms and conditions of the
 * GNU General Public License. See the file COPYING in the
 * main directory of this archive for more details.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/types.h>

#include <linux/string.h>
#include <linux/tty.h>
#include <linux/a.out.h>

/*
 * Prototypes
 */
void usage(void);

/*
 * Defaults, may be overiden by option or environment.
 */
static char *kernel_name = KERNEL_NAME;
static char *ramdisk_name = NULL;
int option_debuglevel = 0;
int option_verbose = 1;
int behaviour = BEHAVE_MILO;

extern char *optarg;

extern volatile void launch(char *kptr, char *rdptr,
			    u_long kernel_size, u_long rd_size);

static char *kptr;		    /* kernel will be loaded there */
static char *rdptr;		    /* ramdisk will be loaded there */

u_long loadaddr    = LOADADDR;	    /* mallocinit() needs that */
u_long kernel_base = KERNELBASE;    /* whereever that is... */
u_long start_mem;                   /* always true on an ARC system */
u_long mem_size;
u_long rd_size;
u_long kernel_entry;
u_long kernel_size;
u_long kernel_bss_end;

struct bootinfo bi;		    /* Linux boot info */
struct screen_info si;              /* Linux screen info */
struct DisplayInfo *di;             /* ARC display info */


/*
 * For now we just use the aout loader
 */
extern struct loader loader_aout;
struct loader *kld = &loader_aout;

/*
 * main()
 */
int main(int argc, char *argv[])
{
    int ch;
  
    /*
     * Print the greet message
     */
    printf("Linux/MIPS ARC Standalone Shell ");
    printf("V" STR(VERSION) "\r\n");
    printf("Copyright (C) Waldorf Electronics and others 1994, 1995\r\n\r\n");

    /*
     * Analyse arguments
     */
    if(argc > 1)
    {
        while ((ch = getopt(argc, argv, "dik:r:v")) != EOF)
            switch (ch)
            {
            case 'd':
                option_debuglevel++;
                option_verbose = 1;
                break;
            case 'i':
                interactive = 1;
                break;
            case 'k':
                kernel_name = optarg;
                break;
            case 'r':
                ramdisk_name = optarg;
                break;
            case 'v':
                option_verbose = 1;
                break;
            case '?':
            default:
                usage();
            }
    }
}

/* 
 * Do the actual boot
 */
int do_boot(char *kernel_name, char *ramdisk_name)
{
    int kfd, rfd = -1, i;
    u_long memreq;
    struct nlist *nl;
    u_long kbi_offset, ksi_offset;

    /*
     * Verify that there is enough RAM
     */
    mem_size = bi.memupper - bi.memlower;
    if (mem_size < 0x800000)
    {
        fprintf(stderr,
		"Insufficient Memory to load Linux/MIPS, aborting\n\r");
        return(5);
    }

    if (behaviour == BEHAVE_ARCDB)
    {
        printf("%s: kernel file is `%s'\r\n", NAMEOF_ARCDB, kernel_name);
        if (ramdisk_name)
            printf("%s: ramdisk file is `%s'\r\n", NAMEOF_ARCDB, ramdisk_name);
    }
	  
    /*
     * Open kernel and gather some data from the executable
     */
    if (kld->ld_open_kernel(kernel_name) < 0)
    {
	fprintf(stderr, "Error opening kernel file %s.\n\r", kernel_name);
	return 5;
    }
    kernel_base = kld->ld_get_kbase();
    kernel_size = kld->ld_get_ksize();
  
    bi.ramdisk_size = 0;		/* default: no ramdisk */
    if (ramdisk_name)
    {
        if ((rfd = open (ramdisk_name, O_RDONLY)) == -1)
        {
            fprintf (stderr,
                     "Unable to open ramdisk file %s\n\r", ramdisk_name);
	}
	else
	{
	    /*
	     * record ramdisk size
	     */
	    bi.ramdisk_size = (lseek (rfd, 0, L_XTND) + 1023) >> 10;

	    rd_size = lseek (rfd, 0, L_XTND);
	    if (rd_size & ((1<<10)-1))
	    {
		/*
		 * Most probably the file is no image at all or has been
		 * corrupted, so print a warning message.
		 */
		printf("Warning: Ramdisk size is not multiple of 1024 bytes.\r\n");
	    }
	    bi.ramdisk_size = rd_size >> 10;
	}
    }
    bi.ramdisk_base = (u_long)start_mem + mem_size - rd_size;
  
    /*
     * find offset to boot_info structure
     */
    if (!(nl = kld->ld_get_nlist (kld->ld_isymbol("boot_info"))))
    {
        perror("get_nlist1");
        return 1;
    }
    else
    {
        kbi_offset = nl->n_value - kernel_base;
        free(nl);
    }
  
    /*
     * Find offset to screen_info structure
     */
    if (!(nl = kld->ld_get_nlist (kld->ld_isymbol("screen_info"))))
    {
        perror("get_nlist2");
        return 1;
    }
    else
    {
        ksi_offset = nl->n_value - kernel_base;
        free(nl);
    }

    /*
     * Find kernel entry point
     */
    if (!(nl = kld->ld_get_nlist (kld->ld_isymbol("kernel_entry"))))
    {
        perror("get_nlist3");
        return 1;
    }
    else
    {
        kernel_entry = nl->n_value;
        free(nl);
    }

    /*
     * End of bss segment - ramdisk will be placed there
     */
    if (!(nl = kld->ld_get_nlist (kld->ld_isymbol("_end"))))
    {
        perror("get_nlist3");
        return 1;
    }
    else
    {
        kernel_bss_end = nl->n_value;
        free(nl);
    }

    /*
     * allocate buffers for kernel and ramdisk
     */
    if (!(kptr = (char *) malloc(kernel_size)))
    {
        fprintf (stderr, "Unable to allocate %d bytes of memory\n\r", memreq);
        return 1;
    }
    memset (kptr, 0, kernel_size);

    if (rd_size)
    {
	if (!(rdptr = (char *) malloc(rd_size)))
	{
	    fprintf (stderr,
                     "Unable to allocate %d bytes of memory\n\r", memreq);
	    return 1;
	}
	memset (rdptr, 0, rd_size);
    }
	      
    /*
     * Should check whether kernel & RAMdisk moving doesn't overwrite
     * the bootstraper during startup.
     */
    if (option_verbose)
    {
	/*
	 * The following text should be printed by the loader module
	 */
        printf ("\r\n");
	kld->ld_print_stats();
        printf ("Kernel entry at 0x%08lx\n\r", kernel_entry);
    }

    /*
     * Now do the real loading
     */
    if (kld->ld_load_kernel(kptr) < 0)
    {
        fprintf (stderr, "Failed to load kernel executable\r\n");
        free(kptr);
        exit (EXIT_FAILURE);
    }
    kld->ld_close_kernel();

    /*
     * copy the boot and screen info structures to the kernel image,
     * then execute the copy-and-go code
     */
    memcpy ((void *)(kptr + kbi_offset), &bi, sizeof(bi));
    memcpy ((void *)(kptr + ksi_offset), &si, sizeof(si));
  
    /*
     * Write the manipulated kernel back
     */

    /*
     * Success...
     */
    return 0;
}

/*
 * usage()
 *
 * Options:
 * -d - enable debugging (default is no debugging)
 * -i - interactive (default is noninteractive)
 * -k - kernel executable (default multi(0)disk(0)fdisk(0)\VMLINUX)
 * -r - ramdisk image (default is no ramdisk)
 * -v - verbose output
 */
void usage(void)
{
    fprintf (stderr, "Usage:\r\n"
           "\tmilo [-d] [-i] [-k kernel_executable] [-r ramdisk_file] [-v]"
           " [option...]\r\n");
    exit (EXIT_FAILURE);
}
