/*
 * Driver for LynLoong PC extras
 *
 *  Copyright (C) 2009 Lemote Inc.
 *  Author: Wu Zhangjin <wuzj@lemote.com>, Xiang Yu <xiangy@lemote.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>	/* for backlight subdriver */
#include <linux/fb.h>

#include <cs5536/cs5536.h>
#include <cs5536/cs5536_mfgpt.h>

#include <loongson.h>

static u32 gpio_base, mfgpt_base;

static void set_gpio_reg_high(int gpio, int reg)
{
	u32 val;

	val = inl(gpio_base + reg);
	val |= (1 << gpio);
	val &= ~(1 << (16 + gpio));
	outl(val, gpio_base + reg);
	mmiowb();
}

static void set_gpio_reg_low(int gpio, int reg)
{
	u32 val;

	val = inl(gpio_base + reg);
	val |= (1 << (16 + gpio));
	val &= ~(1 << gpio);
	outl(val, gpio_base + reg);
	mmiowb();
}

static void set_gpio_output_low(int gpio)
{
	set_gpio_reg_high(gpio, GPIOL_OUT_EN);
	set_gpio_reg_low(gpio, GPIOL_OUT_VAL);
}

static void set_gpio_output_high(int gpio)
{
	set_gpio_reg_high(gpio, GPIOL_OUT_EN);
	set_gpio_reg_high(gpio, GPIOL_OUT_VAL);
}

/* backlight subdriver */

#define MAX_BRIGHTNESS 100
#define DEFAULT_BRIGHTNESS 50
#define MIN_BRIGHTNESS 0
static unsigned int level;

/* Tune the brightness */
static void setup_mfgpt2(void)
{
	/* Set MFGPT2 comparator 1,2 */
	outw(MAX_BRIGHTNESS-level, MFGPT2_CMP1);
	outw(MAX_BRIGHTNESS, MFGPT2_CMP2);
	/* Clear MFGPT2 UP COUNTER */
	outw(0, MFGPT2_CNT);
	/* Enable counter, compare mode, 32k */
	outw(0x8280, MFGPT2_SETUP);
}

static int lynloong_set_brightness(struct backlight_device *bd)
{
	level = (bd->props.fb_blank == FB_BLANK_UNBLANK &&
		 bd->props.power == FB_BLANK_UNBLANK) ?
	    bd->props.brightness : 0;

	if (level > MAX_BRIGHTNESS)
		level = MAX_BRIGHTNESS;
	else if (level < MIN_BRIGHTNESS)
		level = MIN_BRIGHTNESS;

	setup_mfgpt2();

	return 0;
}

static int lynloong_get_brightness(struct backlight_device *bd)
{
	return level;
}

static struct backlight_ops backlight_ops = {
	.get_brightness = lynloong_get_brightness,
	.update_status = lynloong_set_brightness,
};

static struct backlight_device *lynloong_backlight_dev;

static int lynloong_backlight_init(void)
{
	int ret;
	u32 hi;

	/* Get gpio_base */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), &hi, &gpio_base);
	/* Get mfgpt_base */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_MFGPT), &hi, &mfgpt_base);
	/* Get gpio_base */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), &hi, &gpio_base);

	/* Select for mfgpt */
	set_gpio_reg_high(7, GPIOL_OUT_AUX1_SEL);
	/* Enable brightness controlling */
	set_gpio_output_high(7);

	lynloong_backlight_dev = backlight_device_register("backlight0", NULL,
			NULL, &backlight_ops);

	if (IS_ERR(lynloong_backlight_dev)) {
		ret = PTR_ERR(lynloong_backlight_dev);
		return ret;
	}

	lynloong_backlight_dev->props.max_brightness = MAX_BRIGHTNESS;
	lynloong_backlight_dev->props.brightness = DEFAULT_BRIGHTNESS;
	backlight_update_status(lynloong_backlight_dev);

	return 0;
}

static void lynloong_backlight_exit(void)
{
	if (lynloong_backlight_dev) {
		backlight_device_unregister(lynloong_backlight_dev);
		lynloong_backlight_dev = NULL;
	}
	/* Disable brightness controlling */
	set_gpio_output_low(7);
}

static struct platform_device_id platform_device_ids[] = {
	{
		.name = "lynloong_pc",
	},
	{}
};

MODULE_DEVICE_TABLE(platform, platform_device_ids);

static struct platform_driver platform_driver = {
	.driver = {
		.name = "lynloong_pc",
		.owner = THIS_MODULE,
	},
	.id_table = platform_device_ids,
};

static int __init lynloong_init(void)
{
	int ret;

	pr_info("Load LynLoong Platform Specific Driver.\n");

	/* Register platform stuff */
	ret = platform_driver_register(&platform_driver);
	if (ret) {
		pr_err("Fail to register lynloong platform driver.\n");
		return ret;
	}

	ret = lynloong_backlight_init();
	if (ret) {
		pr_err("Fail to register lynloong backlight driver.\n");
		return ret;
	}

	return 0;
}

static void __exit lynloong_exit(void)
{
	lynloong_backlight_exit();
	platform_driver_unregister(&platform_driver);

	pr_info("Unload LynLoong Platform Specific Driver.\n");
}

module_init(lynloong_init);
module_exit(lynloong_exit);

MODULE_AUTHOR("Wu Zhangjin <wuzj@lemote.com>; Xiang Yu <xiangy@lemote.com>");
MODULE_DESCRIPTION("LynLoong PC driver");
MODULE_LICENSE("GPL");
