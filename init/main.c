/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#define __KERNEL_SYSCALLS__
#include <stdarg.h>

#include <asm/system.h>
#include <asm/io.h>

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
#include <linux/hdreg.h>
#include <linux/mm.h>

#include <asm/bugs.h>

#ifdef CONFIG_MIPS_JAZZ
#include <asm/jazz.h>
#if 1
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#endif
#endif

extern unsigned long * prof_buffer;
extern unsigned long prof_len;
extern char _stext, _etext;
extern char *linux_banner;

static char printbuf[1024];

extern int console_loglevel;

extern void init(void);
extern void init_IRQ(void);
extern void init_modules(void);
extern long console_init(long, long);
extern long kmalloc_init(long,long);
extern long blk_dev_init(long,long);
extern long chr_dev_init(long,long);
extern void sock_init(void);
extern long rd_init(long mem_start, int length);
unsigned long net_dev_init(unsigned long, unsigned long);
extern long pci_init(long, long);

extern void bmouse_setup(char *str, int *ints);
extern void eth_setup(char *str, int *ints);
extern void xd_setup(char *str, int *ints);
extern void floppy_setup(char *str, int *ints);
extern void mcd_setup(char *str, int *ints);
extern void aztcd_setup(char *str, int *ints);
extern void st_setup(char *str, int *ints);
extern void st0x_setup(char *str, int *ints);
extern void tmc8xx_setup(char *str, int *ints);
extern void t128_setup(char *str, int *ints);
extern void pas16_setup(char *str, int *ints);
extern void generic_NCR5380_setup(char *str, int *intr);
extern void aha152x_setup(char *str, int *ints);
extern void aha1542_setup(char *str, int *ints);
extern void aha274x_setup(char *str, int *ints);
extern void buslogic_setup(char *str, int *ints);
extern void scsi_luns_setup(char *str, int *ints);
extern void sound_setup(char *str, int *ints);
#ifdef CONFIG_SBPCD
extern void sbpcd_setup(char *str, int *ints);
#endif CONFIG_SBPCD
#ifdef CONFIG_CDU31A
extern void cdu31a_setup(char *str, int *ints);
#endif CONFIG_CDU31A
#ifdef CONFIG_CDU535
extern void sonycd535_setup(char *str, int *ints);
#endif CONFIG_CDU535
void ramdisk_setup(char *str, int *ints);

#ifdef CONFIG_SYSVIPC
extern void ipc_init(void);
#endif
#ifdef CONFIG_SCSI
extern unsigned long scsi_dev_init(unsigned long, unsigned long);
#endif
#ifdef CONFIG_MIPS_JAZZ
#include <asm/jazzdma.h>
#endif
#ifdef CONFIG_REMOTE_DEBUG
#include <asm/gdb-stub.h>
#endif

/*
 * Boot command-line arguments
 */
#define MAX_INIT_ARGS 8
#define MAX_INIT_ENVS 8

extern void time_init(void);

static unsigned long memory_start = 0;
static unsigned long memory_end = 0;

static char term[21];
int rows, cols;

int ramdisk_size;
int root_mountflags = 0;

static char * argv_init[MAX_INIT_ARGS+2] = { "init", NULL, };
static char * envp_init[MAX_INIT_ENVS+2] = { "HOME=/", term, NULL, };

static char * argv_rc[] = { "/bin/sh", NULL };
static char * envp_rc[] = { "HOME=/", term, NULL };

static char * argv[] = { "-/bin/sh",NULL };
static char * envp[] = { "HOME=/usr/root", term, NULL };

char *get_options(char *str, int *ints)
{
	char *cur = str;
	int i=1;

	while (cur && isdigit(*cur) && i <= 10) {
		ints[i++] = simple_strtoul(cur,NULL,0);
		if ((cur = strchr(cur,',')) != NULL)
			cur++;
	}
	ints[0] = i-1;
	return(cur);
}

struct {
	char *str;
	void (*setup_func)(char *, int *);
} bootsetups[] = {
	{ "reserve=", reserve_setup },
	{ "ramdisk=", ramdisk_setup },
#ifdef CONFIG_BUGi386
	{ "no-hlt", no_halt },
	{ "no387", no_387 },
#endif
#ifdef CONFIG_INET
	{ "ether=", eth_setup },
#endif
#ifdef CONFIG_SCSI
	{ "max_scsi_luns=", scsi_luns_setup },
#endif
#ifdef CONFIG_BLK_DEV_IDE
	{ "hda=", hda_setup },
	{ "hdb=", hdb_setup },
	{ "hdc=", hdc_setup },
	{ "hdd=", hdd_setup },
	{ "hd=",  ide_setup },
#elif defined(CONFIG_BLK_DEV_HD)
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
#ifdef CONFIG_SCSI_BUSLOGIC
	{ "buslogic=", buslogic_setup},
#endif
#ifdef CONFIG_BLK_DEV_XD
	{ "xd=", xd_setup },
#endif
#ifdef CONFIG_BLK_DEV_FD
	{ "floppy=", floppy_setup },
#endif
#ifdef CONFIG_MCD
	{ "mcd=", mcd_setup },
#endif
#ifdef CONFIG_AZTCD
	{ "aztcd=", aztcd_setup },
#endif
#ifdef CONFIG_CDU535
	{ "sonycd535=", sonycd535_setup },
#endif CONFIG_CDU535
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

static int checksetup(char *line)
{
	int i = 0;
	int ints[11];

	while (bootsetups[i].str) {
		int n = strlen(bootsetups[i].str);
		if (!strncmp(line,bootsetups[i].str,n)) {
			bootsetups[i].setup_func(get_options(line+n,ints), ints);
			return 1;
		}
		i++;
	}
	return 0;
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
			loops_per_sec = muldiv(loops_per_sec, HZ, ticks);
			printk("ok - %lu.%02lu BogoMips\n",
				loops_per_sec/500000,
				(loops_per_sec/5000) % 100);
			return;
		}
	}
	printk("failed\n");
}


/*
 * This is a simple kernel command line parsing function: it parses
 * the command line, and fills in the arguments/environment to init
 * as appropriate. Any cmd-line option is taken to be an environment
 * variable if it contains the character '='.
 *
 *
 * This routine also checks for options meant for the kernel.
 * These options are not given to init - they are for internal kernel use only.
 */
static void parse_options(char *line)
{
	char *next;
	char *devnames[] = { "hda", "hdb", "hdc", "hdd", "sda", "sdb", "sdc", "sdd", "sde", "fd", "xda", "xdb", NULL };
	int devnums[]    = { 0x300, 0x340, 0x1600, 0x1640, 0x800, 0x810, 0x820, 0x830, 0x840, 0x200, 0xD00, 0xD40, 0};
	int args, envs;

	if (!*line)
		return;
	args = 0;
	envs = 1;	/* TERM is set to 'console' by default */
	next = line;
	while ((line = next) != NULL) {
		if ((next = strchr(line,' ')) != NULL)
			*next++ = 0;
		/*
		 * check for kernel options first..
		 */
		if (!strncmp(line,"root=",5)) {
			int n;
			line += 5;
			if (strncmp(line,"/dev/",5)) {
				ROOT_DEV = simple_strtoul(line,NULL,16);
				continue;
			}
			line += 5;
			for (n = 0 ; devnames[n] ; n++) {
				int len = strlen(devnames[n]);
				if (!strncmp(line,devnames[n],len)) {
					ROOT_DEV = devnums[n]+simple_strtoul(line+len,NULL,0);
					break;
				}
			}
			continue;
		}
		if (!strcmp(line,"ro")) {
			root_mountflags |= MS_RDONLY;
			continue;
		}
		if (!strcmp(line,"rw")) {
			root_mountflags &= ~MS_RDONLY;
			continue;
		}
		if (!strcmp(line,"debug")) {
			console_loglevel = 10;
			continue;
		}
		if (checksetup(line))
			continue;
		/*
		 * Then check if it's an environment variable or
		 * an option.
		 */
		if (strchr(line,'=')) {
			if (envs >= MAX_INIT_ENVS)
				break;
			envp_init[++envs] = line;
		} else {
			if (args >= MAX_INIT_ARGS)
				break;
			argv_init[++args] = line;
		}
	}
	argv_init[args+1] = NULL;
	envp_init[envs+1] = NULL;
}

extern void check_bugs(void);
extern void setup_arch(char **, unsigned long *, unsigned long *);

asmlinkage void start_kernel(void)
{
	char * command_line;
/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
	setup_arch(&command_line, &memory_start, &memory_end);
	memory_start = paging_init(memory_start,memory_end);
	trap_init();
	init_IRQ();
	sched_init();
	parse_options(command_line);
	init_modules();
#ifdef CONFIG_PROFILE
	prof_buffer = (unsigned long *) memory_start;
	/* only text is profiled */
	prof_len = (unsigned long) &_etext - (unsigned long) &_stext;
	prof_len >>= CONFIG_PROFILE_SHIFT;
	memory_start += prof_len * sizeof(unsigned long);
#endif
	memory_start = console_init(memory_start,memory_end);
#ifdef CONFIG_REMOTE_DEBUG
	set_debug_traps();
/*	breakpoint();	*/	/* execute a BREAK insn */
#endif	
#ifdef CONFIG_PCI
	memory_start = pci_init(memory_start,memory_end);
#endif
	memory_start = kmalloc_init(memory_start,memory_end);
#ifdef CONFIG_MIPS_JAZZ
	memory_start = vdma_init(memory_start,memory_end);
#endif
	sti();
	calibrate_delay();
	memory_start = chr_dev_init(memory_start,memory_end);
	memory_start = blk_dev_init(memory_start,memory_end);
	sti();
#ifdef CONFIG_SCSI
	memory_start = scsi_dev_init(memory_start,memory_end);
#endif
#ifdef CONFIG_INET
	memory_start = net_dev_init(memory_start,memory_end);
#endif
	memory_start = inode_init(memory_start,memory_end);
	memory_start = file_table_init(memory_start,memory_end);
	memory_start = name_cache_init(memory_start,memory_end);
	mem_init(memory_start,memory_end);
	buffer_init();
#ifndef CONFIG_MIPS_JAZZ
	/*
	 * time_init() works well for the Deskstation but fails
	 * on Acer Pica which uses a completly different design.
	 * Unfortunately no info yet about the clock...
	 */
	time_init();
#endif
	sock_init();
#ifdef CONFIG_SYSVIPC
	ipc_init();
#endif
	sti();
	check_bugs();

	printk(linux_banner);

#if 0				/* HACKS FOR THE SONIC DRIVER TEST */
	{
	  int rc;
	  struct device *dev;
	  
	  printk("opening ETH0\n");
	  dev = dev_get("eth0");
	  printk("get_dev returned %p\n",dev);
	  if (dev)
	    {
	      rc = dev_open(dev);
	      printk("dev_open() returned %d\n",rc);
	    }
	}
#endif
	
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

static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

void init(void)
{
	int pid,i;

	setup();
	sprintf(term, "TERM=con%dx%d", ORIG_VIDEO_COLS, ORIG_VIDEO_LINES);

	#ifdef CONFIG_UMSDOS_FS
	{
		/*
			When mounting a umsdos fs as root, we detect
			the pseudo_root (/linux) and initialise it here.
			pseudo_root is defined in fs/umsdos/inode.c
		*/
		extern struct inode *pseudo_root;
		if (pseudo_root != NULL){
			current->fs->root = pseudo_root;
			current->fs->pwd  = pseudo_root;
		}
	}
	#endif

	(void) open("/dev/tty1",O_RDWR,0);
	(void) dup(0);
	(void) dup(0);

#if 0				/* HACKS FOR THE PARALLEL DRIVER TEST */
	{
	  int fd;
	  int rc;

	  fd = open("/dev/lp0",O_WRONLY,0);
	  printk("open(/dev/lp0,O_WRONLY) returns %d\n",fd);
	  
	  for (;;)
	    rc = write(fd,"Hello World!\n\r",14);
	}
#endif
	
#if 0				/* HACKS FOR THE SERIAL DRIVER TEST */
	{
	  int fd;
	  int rc;
	  char c;
	  
	  fd = open("/dev/ttyS0",O_RDWR | O_NDELAY,0);
	  printk("open(/dev/ttyS0,O_RDWR) returns %d\n",fd);
	  
	  for (;;) {
	    rc = write(fd,"Hello World!\n\r",14);
	    while (sys_read(fd,&c,1) == 1)
	      printk("%c",c);
	  }
	}
#endif	
#if 0				/* HACKS FOR THE FLOPPY DRIVER TEST */
	{
	  int fd;
	  int rc;
	  char c;

	  show_floppy();
	  printk("Trying to open /dev/fd0...\n");
	  fd = open("/dev/fd0",O_RDWR,0);
	  printk("open(/dev/fd0,O_RDWR) returns %d\n",fd);

	  if (fd > 0) {
	    for (;;) {
	      while ((rc = sys_read(fd,&c,1)) == 1)
		printk("%c",c);
	      printk("read failed with rc=%d\n",rc);
	    }
	  } else
	    for (;;);
	}
#endif
	
	execve("/etc/init",argv_init,envp_init);
	execve("/bin/init",argv_init,envp_init);
	execve("/sbin/init",argv_init,envp_init);
	/* if this fails, fall through to original stuff */

	if (!(pid=fork())) {
		close(0);
		if (open("/etc/rc",O_RDONLY,0))
			_exit(1);
		execve("/bin/sh",argv_rc,envp_rc);
		_exit(2);
	}
	if (pid>0)
		while (pid != wait(&i))
			/* nothing */;
	while (1) {
		if ((pid = fork()) < 0) {
			printf("Fork failed in init\n\r");
			continue;
		}
		if (!pid) {
			close(0);close(1);close(2);
			setsid();
			(void) open("/dev/tty1",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			_exit(execve("/bin/sh",argv,envp));
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();
	}
	_exit(0);
}
