# 1 "/root/root4/orion/os/regaeos/sys/libmyc/char.c"
 



























# 1 "/root/root4/orion/os/regaeos/sys/include/char.h" 1
 






















 
typedef struct char_driver_struct char_driver_t;
typedef struct char_port_struct char_port_t;

extern int char_driver_count;
extern int char_port_count;
 
extern char_driver_t char_drivers[];
extern char_port_t char_ports[];
 

 
struct char_driver_struct {
        int (*write)(char_port_t *port,char *buf, int len);
        int (*read)(char_port_t *port,char *buf, int size);
        int (*put_char)(char_port_t *port, char ch);
	int (*get_char)(char_port_t *port);
        int (*chars_in_buf)(char_port_t *port);

        void (*flush_chars)(char_port_t *port);
        int (*write_room)(char_port_t *port);
};
 
struct char_port_struct {
        char port_name[10];
        char_driver_t *driver;
};
 
int char_register_driver(char_driver_t *drv);


 

int write(int fd, char *buf, int len);
int read(int fd, char *buf, int len);
int putchar(char ch);
int getchar();
int chars_in_buf();

# 29 "/root/root4/orion/os/regaeos/sys/libmyc/char.c" 2

# 1 "/root/root4/orion/os/regaeos/include/rstdio.h" 1
 
 
 
 
 
 
 
 
 



# 1 "/root/root4/orion/os/regaeos/include/std_type.h" 1
 
 
 
 
 
 
 




# 1 "/root/root4/orion/os/regaeos/include/../sys/include/basics.h" 1
 
 
 
 
 
 
 
 
 





typedef char		S8;
typedef unsigned char	U8;
typedef short		S16;
typedef unsigned short	U16;
typedef int		S32;
typedef unsigned int	U32;
typedef int		BOOL;

 








 
typedef U32 (*U32FUNC)();
typedef S32 (*S32FUNC)();
typedef void (*VFUNC)();

 






# 12 "/root/root4/orion/os/regaeos/include/std_type.h" 2


typedef	float	F32;
typedef	double	F64;

 
typedef	F32	Matrix4[4][4];
typedef F32	Vector4[4];
typedef F64	Vector4d[4];

 




 



 




# 13 "/root/root4/orion/os/regaeos/include/rstdio.h" 2

# 1 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/stdarg.h" 1 3
 

























# 1 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/va-mips.h" 1 3
 
 
 
 
 
 
 


 

 



# 27 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/va-mips.h" 3


typedef char * __gnuc_va_list;




 





enum {
  __no_type_class = -1,
  __void_type_class,
  __integer_type_class,
  __char_type_class,
  __enumeral_type_class,
  __boolean_type_class,
  __pointer_type_class,
  __reference_type_class,
  __offset_type_class,
  __real_type_class,
  __complex_type_class,
  __function_type_class,
  __method_type_class,
  __record_type_class,
  __union_type_class,
  __array_type_class,
  __string_type_class,
  __set_type_class,
  __file_type_class,
  __lang_type_class
};


 






















 

# 1 "/usr/local/lib/gcc-lib/mips-linux/2.95/../../../../mips-linux/include/sgidefs.h" 1 3
 






















 

































 






# 76 "/usr/local/lib/gcc-lib/mips-linux/2.95/../../../../mips-linux/include/sgidefs.h" 3














 








 












# 89 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/va-mips.h" 2 3




# 119 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/va-mips.h" 3




# 165 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/va-mips.h" 3



void va_end (__gnuc_va_list);		 



# 232 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/va-mips.h" 3


 

 



# 253 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/va-mips.h" 3



 

















 



# 27 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/stdarg.h" 2 3

# 136 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/stdarg.h" 3







 
 













# 175 "/usr/local/lib/gcc-lib/mips-linux/2.95/include/stdarg.h" 3


 




 

 

 

typedef __gnuc_va_list va_list;
























# 14 "/root/root4/orion/os/regaeos/include/rstdio.h" 2






































typedef struct	 
{
	U8	*_ptr;	 
	S32	_cnt;	 
	U8	*_buf;	 
	U32	_flags;	 
	U32	_file;	 
} FILE;

extern FILE		_iob[10 ];


extern int	fclose(FILE *);
extern int	fflush(FILE *);
extern FILE	*fopen(const char *, const char *);
extern int	fprintf(FILE *, const char *, ...);

extern int	fscanf(FILE *, const char *, ...);

extern int	printf(const char *, ...);
extern int	printf2(const char *, ...);
extern int	scanf(const char *, ...);

extern int	sprintf(char *, const char *, ...);
extern int	sscanf(const char *, const char *, ...);

extern int	vfprintf(FILE *, const char *, va_list);
extern int	vprintf(const char *, va_list);
extern int	vsprintf(char *, const char *, va_list);

extern int	fgetc(FILE *);
extern char	*fgets(char *, int, FILE *);
extern int	fputc(int, FILE *);
extern int	fputs(const char *, FILE *);
extern int	getc(FILE *);
extern int	getchar(void);
extern char	*gets(char *);
extern int	putc(int, FILE *);
extern int	putchar(char);
extern int	puts(const char *);
extern int	ungetc(int, FILE *);
extern U32	fread(void *, U32, U32, FILE *);
extern U32	fwrite(const void *, U32, U32, FILE *);
extern int	fgetpos(FILE *, U32 *);
extern int	fseek(FILE *, long, int);
extern int	fsetpos(FILE *, const U32 *);
extern long	ftell(FILE *);
extern void	rewind(FILE *);
extern void	clearerr(FILE *);
extern int	feof(FILE *);
extern int	ferror(FILE *);
extern void	perror(const char *);

extern int	__filbuf(FILE *);
extern int	__flsbuf(int, FILE *);

























# 30 "/root/root4/orion/os/regaeos/sys/libmyc/char.c" 2









int char_driver_count = 0;
int char_port_count = 0;

char_driver_t char_drivers[2 ];
char_port_t char_ports[2 ];

int char_generic_read(char_port_t *pt, char *buf, int size) {
        int numread = 0;
        int char_read;

        while ((numread < size) && 
			((char_read = pt->driver->get_char(pt)) >= 0)) {
                *buf = char_read;
                numread++;
        }
 
        return (numread);
}
 
int char_generic_write(char_port_t *pt, char *buf, int len) {
        while (len) {
                pt->driver->put_char(pt,*buf);
 
                buf++; len--;
        }
}

 




int char_register_driver(char_driver_t *drv) {
	int index = -1;

	if (drv->read == 0 ) {
		drv->read = char_generic_read;
	}
	if (drv->write == 0 ) {
		drv->write = char_generic_write;
	}

	if (char_driver_count < 2 ) {
		index = char_driver_count;
		char_drivers[char_driver_count] = *drv;
		char_driver_count++;
	}

        if (char_port_count < 2 ) {
		char_ports[char_port_count].driver = 
			&char_drivers[index];
		sprintf(char_ports[char_port_count].port_name,
			":COM%d",char_port_count);
		char_port_count++;
	}
		
	return (index);
}


int null_func() {
	return -1;
}

 

# 1 "/root/root4/orion/os/regaeos/include/serial.h" 1
 
 
 
 
 
 
 












extern void  SerialSetup(U32 baud, U32 console, U32 host, U32 intr_desc);
extern void  SerialPollOn(void);
extern void  SerialPollOff(void);
extern void  SerialPollInit(void);
extern U32 SerialPollConsts(U32 typecode);
extern U8 SerialPollConin(void);
extern void  SerialPollConout(U8 c,int hexify);
extern U32 SerialPollHststs(U32 typecode);
extern U8 SerialPollHstin(void);
void  SerialPollHstout(U8 c);
void *SerialIntInit(U32 channel,
void (*rx_isr)(U32 channel),
void (*tx_char)(U32 unit, U32 isr_flag));
U32 SerialIntBaud(U32 channel, void *hdp, U32 baudrate);
void  SerialIntRxioff(U32 channel, void *hdp);
void  SerialIntRxion(U32 channel, void *hdp);
void  SerialIntTxioff(U32 channel, void *hdp);
void  SerialIntTxion(U32 channel, void *hdp);
U32 SerialIntRead(U32 channel, void *hdp, U8 *ch_ptr);
U32 SerialIntWrite(U32 channel, void *hdp, U8 ch);


# 105 "/root/root4/orion/os/regaeos/sys/libmyc/char.c" 2


int ch_typ_rs_chars_in_buf(char_port_t *pt) {
    int c;
    SerialPollConsts(0);
    if ((c = SerialPollConsts(0 )) == 0 ) {
        return 0;
    }
    
	return 1;
}

int ch_typ_rs_putchar(char_port_t *pt,char ch) {
        	SerialPollConout(ch,1);
	return 0;
}

int ch_typ_rs_getchar(char_port_t *pt) {
    int c;

    if ((c = SerialPollConsts(0 )) == 0 ) {
        return (-1) ;
    }
    else if (c == 2 ) {
        return 2 ;
    }
    else {
        return SerialPollConin();
    }
}


# 154 "/root/root4/orion/os/regaeos/sys/libmyc/char.c"




void init_char_drivers() {
	char_driver_t a_drv;


	 
	a_drv.read = 0 ;
	a_drv.write = 0 ;
	a_drv.chars_in_buf = ch_typ_rs_chars_in_buf;
	a_drv.put_char = ch_typ_rs_putchar;
	a_drv.get_char = ch_typ_rs_getchar;

	char_register_driver(&a_drv);


# 181 "/root/root4/orion/os/regaeos/sys/libmyc/char.c"


}


int chwrite(int fd,char *buf, int len) {
	char_port_t *pt;

	if (fd < char_port_count) {
		pt = &char_ports[fd];
		return (pt->driver->write(pt,buf,len));	
	}
	return -1;
}

int chread(int fd,char *buf, int len) {
	char_port_t *pt;

	if (fd < char_port_count) {
		pt = &char_ports[fd];
		return (pt->driver->read(pt,buf,len));	
	}
	return -1;
}

int putchar(char ch) {
	char_port_t *pt;
	int fd = 0;  

	if (fd < char_port_count) {
		pt = &char_ports[fd];
		return (pt->driver->put_char(pt,ch));	
	}
	return -1;
}

# 229 "/root/root4/orion/os/regaeos/sys/libmyc/char.c"


