/*
 * arch/mips/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  MIPSified by Ralf Baechle
 */

#include <stdarg.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/bootinfo.h>

#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/head.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/utsname.h>
#include <linux/ioport.h>

extern unsigned long * prof_buffer;
extern unsigned long prof_len;
extern char edata, end;
extern char *linux_banner;

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
#define __NR__exit __NR_exit
static inline _syscall0(int,idle)
static inline _syscall0(int,fork)

extern int console_loglevel;

extern char empty_zero_page[PAGE_SIZE];
extern void init(void);
extern void init_IRQ(void);
extern void init_modules(void);
extern long console_init(long, long);
extern long kmalloc_init(long,long);
extern long blk_dev_init(long,long);
extern long chr_dev_init(long,long);
extern void floppy_init(void);
extern void sock_init(void);
extern long rd_init(long mem_start, int length);
unsigned long net_dev_init(unsigned long, unsigned long);
#if 0
extern long bios32_init(long, long);
#endif

extern void hd_setup(char *str, int *ints);
extern void bmouse_setup(char *str, int *ints);
extern void eth_setup(char *str, int *ints);
extern void xd_setup(char *str, int *ints);
extern void mcd_setup(char *str, int *ints);
extern void st_setup(char *str, int *ints);
extern void st0x_setup(char *str, int *ints);
extern void tmc8xx_setup(char *str, int *ints);
extern void t128_setup(char *str, int *ints);
extern void pas16_setup(char *str, int *ints);
extern void generic_NCR5380_setup(char *str, int *intr);
extern void aha152x_setup(char *str, int *ints);
extern void aha1542_setup(char *str, int *ints);
extern void aha274x_setup(char *str, int *ints);
extern void scsi_luns_setup(char *str, int *ints);
extern void sound_setup(char *str, int *ints);
#ifdef CONFIG_SBPCD
extern void sbpcd_setup(char *str, int *ints);
#endif CONFIG_SBPCD
#ifdef CONFIG_CDU31A
extern void cdu31a_setup(char *str, int *ints);
#endif CONFIG_CDU31A
void ramdisk_setup(char *str, int *ints);

#ifdef CONFIG_SYSVIPC
extern void ipc_init(void);
#endif
#ifdef CONFIG_SCSI
extern unsigned long scsi_dev_init(unsigned long, unsigned long);
#endif

/*
 * This is set up by the setup-routine at boot-time
 */
#define PARAM	empty_zero_page
#define EXT_MEM_K (*(unsigned short *) (PARAM+2))
#define DRIVE_INFO (*(struct drive_info_struct *) (PARAM+0x80))
#define MOUNT_ROOT_RDONLY (*(unsigned short *) (PARAM+0x1F2))
#define RAMDISK_SIZE (*(unsigned short *) (PARAM+0x1F8))
#define ORIG_ROOT_DEV (*(unsigned short *) (PARAM+0x1FC))
#define AUX_DEVICE_INFO (*(unsigned char *) (PARAM+0x1FF))

/*
 * Defaults, may be overwritten by milo
 */
#define SCREEN_INFO {0,0,{0,0},52,3,80,4626,3,9,50}

/*
 * Information passed by milo
 */
struct bootinfo boot_info;
struct screen_info screen_info = SCREEN_INFO;

/*
 * Boot command-line arguments
 */
extern void copy_options(char * to, char * from);
void parse_options(char *line);
#define MAX_INIT_ARGS 8
#define MAX_INIT_ENVS 8
#define COMMAND_LINE ((char *) (PARAM+2048))
#define COMMAND_LINE_SIZE 256

extern void time_init(void);

static unsigned long memory_start = 0;	/* After mem_init, stores the */
					/* amount of free user memory */
/* static */ unsigned long memory_end = 0;
/* static unsigned long low_memory_start = 0; */

int rows, cols;

struct drive_info_struct { char dummy[32]; } drive_info;

unsigned char aux_device_present;
int ramdisk_size;
int root_mountflags;

static char command_line[COMMAND_LINE_SIZE] = { 0, };

struct {
	char *str;
	void (*setup_func)(char *, int *);
} bootsetups[] = {
	{ "ramdisk=", ramdisk_setup },
#ifdef CONFIG_INET
	{ "ether=", eth_setup },
#endif
#ifdef CONFIG_SCSI
	{ "max_scsi_luns=", scsi_luns_setup },
#endif
#ifdef CONFIG_BLK_DEV_HD
	{ "hd=", hd_setup },
#endif
#ifdef CONFIG_CHR_DEV_ST
	{ "st=", st_setup },
#endif
#ifdef CONFIG_BUSMOUSE
	{ "bmouse=", bmouse_setup },
#endif
#ifdef CONFIG_SCSI_SEAGATE
	{ "st0x=", st0x_setup },
	{ "tmc8xx=", tmc8xx_setup },
#endif
#ifdef CONFIG_SCSI_T128
	{ "t128=", t128_setup },
#endif
#ifdef CONFIG_SCSI_PAS16
	{ "pas16=", pas16_setup },
#endif
#ifdef CONFIG_SCSI_GENERIC_NCR5380
	{ "ncr5380=", generic_NCR5380_setup },
#endif
#ifdef CONFIG_SCSI_AHA152X
        { "aha152x=", aha152x_setup},
#endif
#ifdef CONFIG_SCSI_AHA1542
        { "aha1542=", aha1542_setup},
#endif
#ifdef CONFIG_SCSI_AHA274X
        { "aha274x=", aha274x_setup},
#endif
#ifdef CONFIG_BLK_DEV_XD
	{ "xd=", xd_setup },
#endif
#ifdef CONFIG_MCD
	{ "mcd=", mcd_setup },
#endif
#ifdef CONFIG_SOUND
	{ "sound=", sound_setup },
#endif
#ifdef CONFIG_SBPCD
	{ "sbpcd=", sbpcd_setup },
#endif CONFIG_SBPCD
#ifdef CONFIG_CDU31A
	{ "cdu31a=", cdu31a_setup },
#endif CONFIG_CDU31A
	{ 0, 0 }
};

void ramdisk_setup(char *str, int *ints)
{
   if (ints[0] > 0 && ints[1] >= 0)
      ramdisk_size = ints[1];
}

unsigned long loops_per_sec = 1;

static void calibrate_delay(void)
{
	int ticks;

	printk("Calibrating delay loop.. ");
	while (loops_per_sec <<= 1) {
		/* wait for "start of" clock tick */
		ticks = jiffies;
		while (ticks == jiffies)
			/* nothing */;
		/* Go .. */
		ticks = jiffies;
		__delay(loops_per_sec);
		ticks = jiffies - ticks;
		if (ticks >= HZ) {
			/*
			 * No assembler - should be ok
			 */
			loops_per_sec = (loops_per_sec * HZ) / ticks;
			printk("ok - %lu.%02lu BogoMips\n",
				loops_per_sec/500000,
				(loops_per_sec/5000) % 100);
			return;
		}
	}
	printk("failed\n");
}

int parse_machine_options(char *line)
{
	/*
	 * No special MIPS options yet
	 */
	return 0;
}

asmlinkage void start_kernel(void)
{
	/*
	 * Interrupts are still disabled. Do necessary setups, then
	 * enable them
	 */
 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;
	aux_device_present = AUX_DEVICE_INFO;
#if 0
	memory_end = (1<<20) + (EXT_MEM_K<<10);
#else
	memory_end = 0x80800000;
#endif
	memory_end &= PAGE_MASK;
	ramdisk_size = RAMDISK_SIZE;
	copy_options(command_line,COMMAND_LINE);

	if (MOUNT_ROOT_RDONLY)
		root_mountflags |= MS_RDONLY;
	memory_start = 0x7fffffff & (unsigned long) &end;

	memory_start = paging_init(memory_start,memory_end);
	trap_init();
	init_IRQ();
	sched_init();
	parse_options(command_line);
	init_modules();
#ifdef CONFIG_PROFILE
	prof_buffer = (unsigned long *) memory_start;
	prof_len = (unsigned long) &end;
	prof_len >>= 2;
	memory_start += prof_len * sizeof(unsigned long);
#endif
	memory_start = console_init(memory_start,memory_end);
	memory_start = kmalloc_init(memory_start,memory_end);
	memory_start = chr_dev_init(memory_start,memory_end);
	memory_start = blk_dev_init(memory_start,memory_end);
	sti();
	calibrate_delay();
#ifdef CONFIG_SCSI
	memory_start = scsi_dev_init(memory_start,memory_end);
#endif
#ifdef CONFIG_INET
	memory_start = net_dev_init(memory_start,memory_end);
#endif
while(1);
	memory_start = inode_init(memory_start,memory_end);
	memory_start = file_table_init(memory_start,memory_end);
	memory_start = name_cache_init(memory_start,memory_end);
	mem_init(memory_start,memory_end);
	buffer_init();
	time_init();
	floppy_init();
	sock_init();
#ifdef CONFIG_SYSVIPC
	ipc_init();
#endif
	sti();
	
	/*
	 * Get CPU type
	 * FIXME: Not implemented yet
	 */

	printk(linux_banner);

	move_to_user_mode();
	if (!fork())		/* we count on this going ok */
		init();
/*
 * task[0] is meant to be used as an "idle" task: it may not sleep, but
 * it might do some general things like count free pages or it could be
 * used to implement a reasonable LRU algorithm for the paging routines:
 * anything that can be useful, but shouldn't take time from the real
 * processes.
 *
 * Right now task[0] just does a infinite idle loop.
 */
	for(;;)
		idle();
}
