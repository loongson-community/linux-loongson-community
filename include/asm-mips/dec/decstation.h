/*
 * General info common to all DECstation systems
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995,1996 by Paul M. Antoine, some code and definitions 
 * are by curteousy of Chris Fraser.
 *
 * This file is under construction - you were warned!
 */

#ifndef __ASM_DEC_DECSTATION_H 
#define __ASM_DEC_DECSTATION_H 

/*
 * REX functions -- these are for the new TURBOchannel style ROMs
 */
#define REX_PROM_MAGIC  0x30464354			/* passed in a2 */

#define REX_GETBITMAP		0x84			/* get mem bitmap */
#define REX_GETCHAR		0x24			/* getch() */
#define REX_PUTCHAR		0x13			/* putch() */
#define REX_HALT		0x9c			/* halt the system */
#define REX_PRINTF		0x30			/* printf() */
#define REX_PUTS		0x2c			/* puts() */
#define REX_SLOTADDR		0x6c			/* slotaddr */
#define REX_GETENV		0x64			/* get env. variable */
#define REX_GETSYSID		0x80			/* get system id */
#define REX_OPEN		0x54			/* open() */
#define REX_READ		0x58			/* read() */



#ifndef __LANGUAGE_ASSEMBLY__

/*
 * A structure to allow calling of the various DEC boot prom routines.
 * FIXME: Don't know how well different DECStation boot prom revisions
 *        are accomodated.
 */
struct dec_prom {
    void (*dec_prom_printf)(char *format, ...);
    char *(*dec_prom_getenv)(char *name);
    unsigned long (*dec_prom_getbitmap)(void);
    unsigned long (*dec_prom_getsysid)(void);
    char *(*dec_prom_gets)(char *s);
    void (*dec_prom_halt)(const unsigned int);
};

#endif

#endif /* __ASM_DEC_DECSTATION_H */
