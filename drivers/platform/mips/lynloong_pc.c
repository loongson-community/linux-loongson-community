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

	return 0;
}

static void __exit lynloong_exit(void)
{
	platform_driver_unregister(&platform_driver);

	pr_info("Unload LynLoong Platform Specific Driver.\n");
}

module_init(lynloong_init);
module_exit(lynloong_exit);

MODULE_AUTHOR("Wu Zhangjin <wuzj@lemote.com>; Xiang Yu <xiangy@lemote.com>");
MODULE_DESCRIPTION("LynLoong PC driver");
MODULE_LICENSE("GPL");
