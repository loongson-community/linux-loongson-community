/*
 *  Registration of Cobalt RTC platform device.
 *
 *  Copyright (C) 2007  Yoichi Yuasa <yoichi_yuasa@tripeaks.co.jp>
 *  Copyright (C) 2009  Wu Zhangjin <wuzj@lemote.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/mc146818rtc.h>
#include <linux/platform_device.h>

static struct resource rtc_cmos_resource[] = {
	{
		.start	= RTC_PORT(0),
		.end	= RTC_PORT(1),
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= RTC_IRQ,
		.end	= RTC_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device rtc_cmos_device = {
	.name		= "rtc_cmos",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(rtc_cmos_resource),
	.resource	= rtc_cmos_resource
};

static __init int rtc_cmos_init(void)
{
	platform_device_register(&rtc_cmos_device);

	return 0;
}

device_initcall(rtc_cmos_init);
