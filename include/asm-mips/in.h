#ifndef _ASM_I386_IN_H
#define _ASM_I386_IN_H

static __inline__ unsigned long int
__ntohl(unsigned long int x)
{
	return (((x & 0x000000ffU) << 24) |
		((x & 0x0000ff00U) <<  8) |
		((x & 0x00ff0000U) >>  8) |
		((x & 0xff000000U) >> 24));
}

static __inline__ unsigned short int
__ntohs(unsigned short int x)
{
	return (((x & 0x00ff) << 8) |
		((x & 0xff00) >> 8));
}

#define __htonl(x) __ntohl(x)
#define __htons(x) __ntohs(x)

#ifdef  __OPTIMIZE__
#  define ntohl(x) \
(__ntohl((x)))
#  define ntohs(x) \
(__ntohs((x)))
#  define htonl(x) \
(__htonl((x)))
#  define htons(x) \
(__htons((x)))
#endif

#endif	/* _ASM_I386_IN_H */
