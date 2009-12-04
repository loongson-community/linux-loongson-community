/*
 * Driver for YeeLoong laptop extras
 *
 *  Copyright (C) 2009 Lemote Inc.
 *  Author: Wu Zhangjin <wuzj@lemote.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>	/* for backlight subdriver */
#include <linux/fb.h>

#include <ec_kb3310b.h>

/* backlight subdriver */
#define MAX_BRIGHTNESS	8

static int yeeloong_set_brightness(struct backlight_device *bd)
{
	unsigned int level, current_level;
	static unsigned int old_level;

	level = (bd->props.fb_blank == FB_BLANK_UNBLANK &&
		 bd->props.power == FB_BLANK_UNBLANK) ?
	    bd->props.brightness : 0;

	if (level > MAX_BRIGHTNESS)
		level = MAX_BRIGHTNESS;
	else if (level < 0)
		level = 0;

	/* Avoid to modify the brightness when EC is tuning it */
	if (old_level != level) {
		current_level = ec_read(REG_DISPLAY_BRIGHTNESS);
		if (old_level == current_level)
			ec_write(REG_DISPLAY_BRIGHTNESS, level);
		old_level = level;
	}

	return 0;
}

static int yeeloong_get_brightness(struct backlight_device *bd)
{
	return ec_read(REG_DISPLAY_BRIGHTNESS);
}

static struct backlight_ops backlight_ops = {
	.get_brightness = yeeloong_get_brightness,
	.update_status = yeeloong_set_brightness,
};

static struct backlight_device *yeeloong_backlight_dev;

static int yeeloong_backlight_init(void)
{
	int ret;

	yeeloong_backlight_dev = backlight_device_register("backlight0", NULL,
			NULL, &backlight_ops);

	if (IS_ERR(yeeloong_backlight_dev)) {
		ret = PTR_ERR(yeeloong_backlight_dev);
		yeeloong_backlight_dev = NULL;
		return ret;
	}

	yeeloong_backlight_dev->props.max_brightness = MAX_BRIGHTNESS;
	yeeloong_backlight_dev->props.brightness =
		yeeloong_get_brightness(yeeloong_backlight_dev);
	backlight_update_status(yeeloong_backlight_dev);

	return 0;
}

static void yeeloong_backlight_exit(void)
{
	if (yeeloong_backlight_dev) {
		backlight_device_unregister(yeeloong_backlight_dev);
		yeeloong_backlight_dev = NULL;
	}
}

static struct platform_device_id platform_device_ids[] = {
	{
		.name = "yeeloong_laptop",
	},
	{}
};

MODULE_DEVICE_TABLE(platform, platform_device_ids);

static struct platform_driver platform_driver = {
	.driver = {
		   .name = "yeeloong_laptop",
		   .owner = THIS_MODULE,
		   },
	.id_table = platform_device_ids,
};

static int __init yeeloong_init(void)
{
	int ret;

	pr_info("Load YeeLoong Laptop Platform Specific Driver.\n");

	/* Register platform stuff */
	ret = platform_driver_register(&platform_driver);
	if (ret) {
		pr_err("Fail to register yeeloong platform driver.\n");
		return ret;
	}

	ret = yeeloong_backlight_init();
	if (ret) {
		pr_err("Fail to register yeeloong backlight driver.\n");
		yeeloong_backlight_exit();
		return ret;
	}

	return 0;
}

static void __exit yeeloong_exit(void)
{
	yeeloong_backlight_exit();
	platform_driver_unregister(&platform_driver);

	pr_info("Unload YeeLoong Platform Specific Driver.\n");
}

module_init(yeeloong_init);
module_exit(yeeloong_exit);

MODULE_AUTHOR("Wu Zhangjin <wuzj@lemote.com>");
MODULE_DESCRIPTION("YeeLoong laptop driver");
MODULE_LICENSE("GPL");
