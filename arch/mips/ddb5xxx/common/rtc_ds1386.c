/***********************************************************************
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: jsun@mvista.com or jsun@junsun.net
 *
 * arch/mips/ddb5xxx/common/rtc_ds1386.c
 *     low-level RTC hookups for s for Dallas 1396 chip.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 ***********************************************************************
 */


/*
 * This file exports a function, rtc_ds1386_init(), which expects an
 * uncached base address as the argument.  It will set the two function
 * pointers expected by the MIPS generic timer code.
 */

#include <linux/types.h>
#include <linux/time.h>
#include <linux/rtc.h>

#include <asm/time.h>
#include <asm/addrspace.h>

#include <asm/ddb5xxx/debug.h>

#define	EPOCH		2000

#undef BCD_TO_BIN
#define BCD_TO_BIN(val) (((val)&15) + ((val)>>4)*10)

#undef BIN_TO_BCD
#define BIN_TO_BCD(val) ((((val)/10)<<4) + (val)%10)

#define	READ_RTC(x)	*(volatile unsigned char*)(rtc_base+x)
#define	WRITE_RTC(x, y)	*(volatile unsigned char*)(rtc_base+x) = y

static unsigned long rtc_base;

static unsigned long
rtc_ds1386_get_time(void)
{	
	u8 byte;
	u8 temp;
	unsigned int year, month, day, hour, minute, second;

	/* let us freeze external registers */
	byte = READ_RTC(0xB);
	byte &= 0x3f;
	WRITE_RTC(0xB, byte);

	/* read time data */
	year = BCD_TO_BIN(READ_RTC(0xA)) + EPOCH;
	month = BCD_TO_BIN(READ_RTC(0x9) & 0x1f);
	day = BCD_TO_BIN(READ_RTC(0x8));
	minute = BCD_TO_BIN(READ_RTC(0x2));
	second = BCD_TO_BIN(READ_RTC(0x1));

	/* hour is special - deal with it later */
	temp = READ_RTC(0x4);

	/* enable time transfer */
	byte |= 0x80;
	WRITE_RTC(0xB, byte);

	/* calc hour */
	if (temp & 0x40) {
		/* 12 hour format */
		hour = BCD_TO_BIN(temp & 0x1f);
		if (temp & 0x20) hour += 12; 		/* PM */
	} else {
		/* 24 hour format */
		hour = BCD_TO_BIN(temp & 0x3f);
	}

	return mktime(year, month, day, hour, minute, second);
}

void to_tm(unsigned long tim, struct rtc_time * tm);
static int 
rtc_ds1386_set_time(unsigned long t)
{
	struct rtc_time tm;
	u8 byte;
	u8 temp;
	u8 year, month, day, hour, minute, second;

	/* let us freeze external registers */
	byte = READ_RTC(0xB);
	byte &= 0x3f;
	WRITE_RTC(0xB, byte);

	/* convert */
	to_tm(t, &tm);

	/* check each field one by one */
	year = BIN_TO_BCD(tm.tm_year - EPOCH);
	if (year != READ_RTC(0xA)) {
		WRITE_RTC(0xA, year);
	}

	temp = READ_RTC(0x9);
	month = BIN_TO_BCD(tm.tm_mon);
	if (month != (temp & 0x1f)) {
		WRITE_RTC( 0x9,
			   (month & 0x1f) | (temp & ~0x1f) );
	}

	day = BIN_TO_BCD(tm.tm_mday);
	if (day != READ_RTC(0x8)) {
		WRITE_RTC(0x8, day);
	}

	temp = READ_RTC(0x4);
	if (temp & 0x40) {
		/* 12 hour format */
		hour = 0x40;
		if (tm.tm_hour > 12) {
			hour |= 0x20 | (BIN_TO_BCD(hour-12) & 0x1f);
		} else {
			hour |= BIN_TO_BCD(tm.tm_hour);
		}
	} else {
		/* 24 hour format */
		hour = BIN_TO_BCD(tm.tm_hour) & 0x3f;
	}
	if (hour != temp) WRITE_RTC(0x4, hour);

	minute = BIN_TO_BCD(tm.tm_min);
	if (minute != READ_RTC(0x2)) {
		WRITE_RTC(0x2, minute);
	}

	second = BIN_TO_BCD(tm.tm_sec);
	if (second != READ_RTC(0x1)) {
		WRITE_RTC(0x1, second);
	}
	
	return 0;
}

void
rtc_ds1386_init(unsigned long base)
{
	unsigned char byte;
	
	/* remember the base */
	rtc_base = base;
	MIPS_ASSERT((rtc_base & 0xe0000000) == KSEG1);

	/* turn on RTC if it is not on */
	byte = READ_RTC(0x9);
	if (byte & 0x80) {
		byte &= 0x7f;
		WRITE_RTC(0x9, byte);
	}

	/* enable time transfer */
	byte = READ_RTC(0xB);
	byte |= 0x80;
	WRITE_RTC(0xB, byte);

	/* set the function pointers */
	rtc_get_time = rtc_ds1386_get_time;
	rtc_set_time = rtc_ds1386_set_time;
}


/* ================================================== */
#define TICK_SIZE tick
#define FEBRUARY        2
#define STARTOFTIME     1970
#define SECDAY          86400L
#define SECYR           (SECDAY * 365)
#define leapyear(year)          ((year) % 4 == 0)
#define days_in_year(a)         (leapyear(a) ? 366 : 365)
#define days_in_month(a)        (month_days[(a) - 1])

static int month_days[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 * This only works for the Gregorian calendar - i.e. after 1752 (in the UK)
 */
static void 
GregorianDay(struct rtc_time * tm)
{
        int leapsToDate;
        int lastYear;
        int day;
        int MonthOffset[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

        lastYear=tm->tm_year-1;

        /*
         * Number of leap corrections to apply up to end of last year
         */
        leapsToDate = lastYear/4 - lastYear/100 + lastYear/400;

        /*
         * This year is a leap year if it is divisible by 4 except when it is
         * divisible by 100 unless it is divisible by 400
         *
         * e.g. 1904 was a leap year, 1900 was not, 1996 is, and 2000 will be
         */
        if((tm->tm_year%4==0) &&
           ((tm->tm_year%100!=0) || (tm->tm_year%400==0)) &&
           (tm->tm_mon>2))
        {
                /*
                 * We are past Feb. 29 in a leap year
                 */
                day=1;
        }
        else
        {
                day=0;
        }

        day += lastYear*365 + leapsToDate + MonthOffset[tm->tm_mon-1] +
                   tm->tm_mday;

        tm->tm_wday=day%7;
}


void to_tm(unsigned long tim, struct rtc_time * tm)
{
        register int    i;
        register long   hms, day;

        day = tim / SECDAY;
        hms = tim % SECDAY;

        /* Hours, minutes, seconds are easy */
        tm->tm_hour = hms / 3600;
        tm->tm_min = (hms % 3600) / 60;
        tm->tm_sec = (hms % 3600) % 60;

        /* Number of years in days */
        for (i = STARTOFTIME; day >= days_in_year(i); i++)
                day -= days_in_year(i);
        tm->tm_year = i;

        /* Number of months in days left */
        if (leapyear(tm->tm_year))
                days_in_month(FEBRUARY) = 29;
        for (i = 1; day >= days_in_month(i); i++)
                day -= days_in_month(i);
        days_in_month(FEBRUARY) = 28;
        tm->tm_mon = i;

        /* Days are what is left over (+1) from all that. */
        tm->tm_mday = day + 1;

        /*
         * Determine the day of week
         */
        GregorianDay(tm);
}
