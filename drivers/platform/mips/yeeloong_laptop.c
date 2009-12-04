/*
 * Driver for YeeLoong laptop extras
 *
 *  Copyright (C) 2009 Lemote Inc.
 *  Author: Wu Zhangjin <wuzj@lemote.com>, Liu Junliang <liujl@lemote.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>	/* for backlight subdriver */
#include <linux/fb.h>
#include <linux/apm-emulation.h>/* for battery subdriver */
#include <linux/hwmon.h>	/* for hwmon subdriver */
#include <linux/hwmon-sysfs.h>

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

	level = SENSORS_LIMIT(level, 0, MAX_BRIGHTNESS);

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

/* battery subdriver */

static void get_fixed_battery_info(void)
{
	int design_cap, full_charged_cap, design_vol, vendor, cell_count;

	design_cap = (ec_read(REG_BAT_DESIGN_CAP_HIGH) << 8)
	    | ec_read(REG_BAT_DESIGN_CAP_LOW);
	full_charged_cap = (ec_read(REG_BAT_FULLCHG_CAP_HIGH) << 8)
	    | ec_read(REG_BAT_FULLCHG_CAP_LOW);
	design_vol = (ec_read(REG_BAT_DESIGN_VOL_HIGH) << 8)
	    | ec_read(REG_BAT_DESIGN_VOL_LOW);
	vendor = ec_read(REG_BAT_VENDOR);
	cell_count = ec_read(REG_BAT_CELL_COUNT);

	if (vendor != 0) {
		pr_info("battery vendor(%s), cells count(%d), "
		       "with designed capacity(%d),designed voltage(%d),"
		       " full charged capacity(%d)\n",
		       (vendor ==
			FLAG_BAT_VENDOR_SANYO) ? "SANYO" : "SIMPLO",
		       (cell_count == FLAG_BAT_CELL_3S1P) ? 3 : 6,
		       design_cap, design_vol,
		       full_charged_cap);
	}
}

#define APM_CRITICAL		5

static void get_power_status(struct apm_power_info *info)
{
	unsigned char bat_status;

	info->battery_status = APM_BATTERY_STATUS_UNKNOWN;
	info->battery_flag = APM_BATTERY_FLAG_UNKNOWN;
	info->units = APM_UNITS_MINS;

	info->battery_life = (ec_read(REG_BAT_RELATIVE_CAP_HIGH) << 8) |
		(ec_read(REG_BAT_RELATIVE_CAP_LOW));

	info->ac_line_status = (ec_read(REG_BAT_POWER) & BIT_BAT_POWER_ACIN) ?
		APM_AC_ONLINE : APM_AC_OFFLINE;

	bat_status = ec_read(REG_BAT_STATUS);

	if (!(bat_status & BIT_BAT_STATUS_IN)) {
		/* no battery inserted */
		info->battery_status = APM_BATTERY_STATUS_NOT_PRESENT;
		info->battery_flag = APM_BATTERY_FLAG_NOT_PRESENT;
		info->time = 0x00;
		return;
	}

	/* adapter inserted */
	if (info->ac_line_status == APM_AC_ONLINE) {
		if (!(bat_status & BIT_BAT_STATUS_FULL)) {
			/* battery is not fully charged */
			info->battery_status = APM_BATTERY_STATUS_CHARGING;
			info->battery_flag = APM_BATTERY_FLAG_CHARGING;
		} else {
			/* battery is fully charged */
			info->battery_status = APM_BATTERY_STATUS_HIGH;
			info->battery_flag = APM_BATTERY_FLAG_HIGH;
			info->battery_life = 100;
		}
	} else {
		/* battery is too low */
		if (bat_status & BIT_BAT_STATUS_LOW) {
			info->battery_status = APM_BATTERY_STATUS_LOW;
			info->battery_flag = APM_BATTERY_FLAG_LOW;
			if (info->battery_life <= APM_CRITICAL) {
				/* we should power off the system now */
				info->battery_status =
					APM_BATTERY_STATUS_CRITICAL;
				info->battery_flag = APM_BATTERY_FLAG_CRITICAL;
			}
		} else {
			/* assume the battery is high enough. */
			info->battery_status = APM_BATTERY_STATUS_HIGH;
			info->battery_flag = APM_BATTERY_FLAG_HIGH;
		}
	}
	info->time = ((info->battery_life - 3) * 54 + 142) / 60;
}

static int yeeloong_battery_init(void)
{
	get_fixed_battery_info();

	apm_get_power_status = get_power_status;

	return 0;
}

static void yeeloong_battery_exit(void)
{
	if (apm_get_power_status == get_power_status)
		apm_get_power_status = NULL;
}

/* hwmon subdriver */

#define MIN_FAN_SPEED 0
#define MAX_FAN_SPEED 3

static int get_fan_pwm_enable(void)
{
	int level, mode;

	level = ec_read(REG_FAN_SPEED_LEVEL);
	mode = ec_read(REG_FAN_AUTO_MAN_SWITCH);

	if (level == MAX_FAN_SPEED && mode == BIT_FAN_MANUAL)
		mode = 0;
	else if (mode == BIT_FAN_MANUAL)
		mode = 1;
	else
		mode = 2;

	return mode;
}

static void set_fan_pwm_enable(int mode)
{
	switch (mode) {
	case 0:
		/* fullspeed */
		ec_write(REG_FAN_AUTO_MAN_SWITCH, BIT_FAN_MANUAL);
		ec_write(REG_FAN_SPEED_LEVEL, MAX_FAN_SPEED);
		break;
	case 1:
		ec_write(REG_FAN_AUTO_MAN_SWITCH, BIT_FAN_MANUAL);
		break;
	case 2:
		ec_write(REG_FAN_AUTO_MAN_SWITCH, BIT_FAN_AUTO);
		break;
	default:
		break;
	}
}

static int get_fan_pwm(void)
{
	return ec_read(REG_FAN_SPEED_LEVEL);
}

static void set_fan_pwm(int value)
{
	int mode;

	mode = ec_read(REG_FAN_AUTO_MAN_SWITCH);
	if (mode != BIT_FAN_MANUAL)
		return;

	value = SENSORS_LIMIT(value, 0, 3);

	/* We must ensure the fan is on */
	if (value > 0)
		ec_write(REG_FAN_CONTROL, BIT_FAN_CONTROL_ON);

	ec_write(REG_FAN_SPEED_LEVEL, value);
}

static int get_fan_rpm(void)
{
	int value;

	value = FAN_SPEED_DIVIDER /
	    (((ec_read(REG_FAN_SPEED_HIGH) & 0x0f) << 8) |
	     ec_read(REG_FAN_SPEED_LOW));

	return value;
}

static int get_cpu_temp(void)
{
	s8 value;

	value = ec_read(REG_TEMPERATURE_VALUE);

	return value * 1000;
}

static int get_cpu_temp_max(void)
{
	return 60 * 1000;
}

static int get_battery_temp(void)
{
	int value;

	value = (ec_read(REG_BAT_TEMPERATURE_HIGH) << 8) |
		(ec_read(REG_BAT_TEMPERATURE_LOW));

	return value * 1000;
}

static int get_battery_temp_alarm(void)
{
	int status;

	status = (ec_read(REG_BAT_CHARGE_STATUS) &
			BIT_BAT_CHARGE_STATUS_OVERTEMP);

	return !!status;
}

static int get_battery_current(void)
{
	s16 value;

	value = (ec_read(REG_BAT_CURRENT_HIGH) << 8) |
		(ec_read(REG_BAT_CURRENT_LOW));

	return -value;
}

static int get_battery_voltage(void)
{
	int value;

	value = (ec_read(REG_BAT_VOLTAGE_HIGH) << 8) |
		(ec_read(REG_BAT_VOLTAGE_LOW));

	return value;
}

static ssize_t store_sys_hwmon(void (*set) (int), const char *buf, size_t count)
{
	int ret;
	unsigned long value;

	if (!count)
		return 0;

	ret = strict_strtoul(buf, 10, &value);
	if (ret)
		return ret;

	set(value);

	return count;
}

static ssize_t show_sys_hwmon(int (*get) (void), char *buf)
{
	return sprintf(buf, "%d\n", get());
}

#define CREATE_SENSOR_ATTR(_name, _mode, _set, _get)		\
	static ssize_t show_##_name(struct device *dev,			\
				    struct device_attribute *attr,	\
				    char *buf)				\
	{								\
		return show_sys_hwmon(_set, buf);			\
	}								\
	static ssize_t store_##_name(struct device *dev,		\
				     struct device_attribute *attr,	\
				     const char *buf, size_t count)	\
	{								\
		return store_sys_hwmon(_get, buf, count);		\
	}								\
	static SENSOR_DEVICE_ATTR(_name, _mode, show_##_name, store_##_name, 0);

CREATE_SENSOR_ATTR(fan1_input, S_IRUGO, get_fan_rpm, NULL);
CREATE_SENSOR_ATTR(pwm1, S_IRUGO | S_IWUSR, get_fan_pwm, set_fan_pwm);
CREATE_SENSOR_ATTR(pwm1_enable, S_IRUGO | S_IWUSR, get_fan_pwm_enable,
		set_fan_pwm_enable);
CREATE_SENSOR_ATTR(temp1_input, S_IRUGO, get_cpu_temp, NULL);
CREATE_SENSOR_ATTR(temp1_max, S_IRUGO, get_cpu_temp_max, NULL);
CREATE_SENSOR_ATTR(temp2_input, S_IRUGO, get_battery_temp, NULL);
CREATE_SENSOR_ATTR(temp2_max_alarm, S_IRUGO, get_battery_temp_alarm, NULL);
CREATE_SENSOR_ATTR(curr1_input, S_IRUGO, get_battery_current, NULL);
CREATE_SENSOR_ATTR(in1_input, S_IRUGO, get_battery_voltage, NULL);

static ssize_t
show_name(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "yeeloong\n");
}

static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, 0);

static struct attribute *hwmon_attributes[] = {
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm1_enable.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp2_max_alarm.dev_attr.attr,
	&sensor_dev_attr_curr1_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_name.dev_attr.attr,
	NULL
};

static struct attribute_group hwmon_attribute_group = {
	.attrs = hwmon_attributes
};

static struct device *yeeloong_hwmon_dev;

static int yeeloong_hwmon_init(void)
{
	int ret;

	yeeloong_hwmon_dev = hwmon_device_register(NULL);
	if (IS_ERR(yeeloong_hwmon_dev)) {
		pr_err("Fail to register yeeloong hwmon device\n");
		yeeloong_hwmon_dev = NULL;
		return PTR_ERR(yeeloong_hwmon_dev);
	}
	ret = sysfs_create_group(&yeeloong_hwmon_dev->kobj,
				 &hwmon_attribute_group);
	if (ret) {
		hwmon_device_unregister(yeeloong_hwmon_dev);
		yeeloong_hwmon_dev = NULL;
		return ret;
	}
	/* ensure fan is set to auto mode */
	set_fan_pwm_enable(2);

	return 0;
}

static void yeeloong_hwmon_exit(void)
{
	if (yeeloong_hwmon_dev) {
		sysfs_remove_group(&yeeloong_hwmon_dev->kobj,
				   &hwmon_attribute_group);
		hwmon_device_unregister(yeeloong_hwmon_dev);
		yeeloong_hwmon_dev = NULL;
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

	yeeloong_battery_init();

	ret = yeeloong_hwmon_init();
	if (ret) {
		pr_err("Fail to register yeeloong hwmon driver.\n");
		yeeloong_hwmon_exit();
		return ret;
	}

	return 0;
}

static void __exit yeeloong_exit(void)
{
	yeeloong_hwmon_exit();
	yeeloong_battery_exit();
	yeeloong_backlight_exit();
	platform_driver_unregister(&platform_driver);

	pr_info("Unload YeeLoong Platform Specific Driver.\n");
}

module_init(yeeloong_init);
module_exit(yeeloong_exit);

MODULE_AUTHOR("Wu Zhangjin <wuzj@lemote.com>; Liu Junliang <liujl@lemote.com>");
MODULE_DESCRIPTION("YeeLoong laptop driver");
MODULE_LICENSE("GPL");
