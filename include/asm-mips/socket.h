#ifndef __ASM_MIPS_SOCKET_H
#define __ASM_MIPS_SOCKET_H

#include <linux/types.h>
#include <asm/sockios.h>

/*
 * For setsockoptions(2)
 *
 * This defines are ABI conformant as far as Linux supports these ...
 */
#define SOL_SOCKET	0xffff

enum
{
    SO_DEBUG = 0x0001,		/* Record debugging information.  */
    SO_REUSEADDR = 0x0004,	/* Allow reuse of local addresses.  */
    SO_KEEPALIVE = 0x0008,	/* Keep connections alive and send
				   SIGPIPE when they die.  */
    SO_DONTROUTE = 0x0010,	/* Don't do local routing.  */
    SO_BROADCAST = 0x0020,	/* Allow transmission of
				   broadcast messages.  */
    SO_LINGER = 0x0080,		/* Block on close of a reliable
				   socket to transmit pending data.  */
    SO_OOBINLINE = 0x0100,	/* Receive out-of-band data in-band.  */
#if 0
To add: SO_REUSEPORT = 0x0200,	/* Allow local address and port reuse.  */
#endif

    SO_TYPE = 0x1008,		/* Compatible name for SO_STYLE.  */
    SO_STYLE = SO_TYPE,		/* Synonym */
    SO_ERROR = 0x1007,		/* get error status and clear */
    SO_SNDBUF = 0x1001,		/* Send buffer size. */
    SO_RCVBUF = 0x1002,		/* Receive buffer. */
    SO_SNDLOWAT = 0x1003,	/* send low-water mark */
    SO_RCVLOWAT = 0x1004,	/* receive low-water mark */
    SO_SNDTIMEO = 0x1005,	/* send timeout */
    SO_RCVTIMEO = 0x1006,	/* receive timeout */

/* linux-specific, might as well be the same as on i386 */
    SO_NO_CHECK = 11,
    SO_PRIORITY = 12,
    SO_BSDCOMPAT = 14,
};

/*
 * Weird.  The existing SVr4 port to MIPS use two different definitions for
 * SOCK_STREAM and SOCK_DGRAM with swapped values.  I choose to be compatible
 * with the newer definitions though that looks wrong ...
 */

/* Types of sockets.  */
enum __socket_type
{
  SOCK_DGRAM = 1,               /* Connectionless, unreliable datagrams
                                   of fixed maximum length.  */
  SOCK_STREAM = 2,              /* Sequenced, reliable, connection-based
                                   byte streams.  */
  SOCK_RAW = 3,                 /* Raw protocol interface.  */
  SOCK_RDM = 4,                 /* Reliably-delivered messages.  */
  SOCK_SEQPACKET = 5,           /* Sequenced, reliable, connection-based,
                                   datagrams of fixed maximum length.  */
  SOCK_PACKET = 10,		/* linux specific way of getting packets at
				   the dev level.  For writing rarp and
				   other similar things on the user level.  */
};

#endif /* __ASM_MIPS_SOCKET_H */
