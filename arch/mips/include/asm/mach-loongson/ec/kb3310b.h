/*
 * EC(Embedded Controller) KB3310B header file
 *
 *  Copyright (C) 2008 Lemote Inc. & Insititute of Computing Technology
 *  Author: liujl <liujl@lemote.com>, 2008-03-14
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>

extern unsigned char ec_read(unsigned short addr);
extern void ec_write(unsigned short addr, unsigned char val);
extern int ec_query_seq(unsigned char cmd);
extern int ec_query_event_num(void);
extern int ec_get_event_num(void);

/*
 * The following registers are determined by the EC index configuration.
 * 1, fill the PORT_HIGH as EC register high part.
 * 2, fill the PORT_LOW as EC register low part.
 * 3, fill the PORT_DATA as EC register write data or get the data from it.
 */
#define	EC_IO_PORT_HIGH	0x0381
#define	EC_IO_PORT_LOW	0x0382
#define	EC_IO_PORT_DATA	0x0383

/* ec delay time 500us for register and status access */
#define	EC_REG_DELAY	500	/* unit : us */
#define	EC_CMD_TIMEOUT		0x1000

/* EC access port for sci communication */
#define	EC_CMD_PORT		0x66
#define	EC_STS_PORT		0x66
#define	EC_DAT_PORT		0x62
#define	CMD_INIT_IDLE_MODE	0xdd
#define	CMD_EXIT_IDLE_MODE	0xdf
#define	CMD_INIT_RESET_MODE	0xd8
#define	CMD_REBOOT_SYSTEM	0x8c
#define	CMD_GET_EVENT_NUM	0x84
#define	CMD_PROGRAM_PIECE	0xda
