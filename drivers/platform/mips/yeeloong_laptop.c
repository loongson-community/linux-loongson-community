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
#include <linux/video_output.h>	/* for video output subdriver */
#include <linux/input.h>	/* for hotkey subdriver */
#include <linux/input/sparse-keymap.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <cs5536/cs5536.h>

#include <loongson.h>		/* for loongson_cmdline */
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

/* video output subdriver */

static int lcd_video_output_get(struct output_device *od)
{
	return ec_read(REG_DISPLAY_LCD);
}

#define LCD	0
#define CRT	1

static void display_vo_set(int display, int on)
{
	int addr;
	unsigned long value;

	addr = (display == LCD) ? 0x31 : 0x21;

	outb(addr, 0x3c4);
	value = inb(0x3c5);

	if (display == LCD)
		value |= (on ? 0x03 : 0x02);
	else {
		if (on)
			clear_bit(7, &value);
		else
			set_bit(7, &value);
	}

	outb(addr, 0x3c4);
	outb(value, 0x3c5);
}

static int lcd_video_output_set(struct output_device *od)
{
	unsigned long status;

	status = !!od->request_state;

	display_vo_set(LCD, status);
	ec_write(REG_BACKLIGHT_CTRL, status);

	return 0;
}

static struct output_properties lcd_output_properties = {
	.set_state = lcd_video_output_set,
	.get_status = lcd_video_output_get,
};

static int crt_video_output_get(struct output_device *od)
{
	return ec_read(REG_CRT_DETECT);
}

static int crt_video_output_set(struct output_device *od)
{
	unsigned long status;

	status = !!od->request_state;

	if (ec_read(REG_CRT_DETECT) == BIT_CRT_DETECT_PLUG)
		display_vo_set(CRT, status);

	return 0;
}

static struct output_properties crt_output_properties = {
	.set_state = crt_video_output_set,
	.get_status = crt_video_output_get,
};

static struct output_device *lcd_output_dev, *crt_output_dev;

static void yeeloong_lcd_vo_set(int status)
{
	lcd_output_dev->request_state = status;
	lcd_video_output_set(lcd_output_dev);
}

static void yeeloong_crt_vo_set(int status)
{
	crt_output_dev->request_state = status;
	crt_video_output_set(crt_output_dev);
}

static int yeeloong_vo_init(void)
{
	int ret;

	/* Register video output device: lcd, crt */
	lcd_output_dev = video_output_register("LCD", NULL, NULL,
			&lcd_output_properties);

	if (IS_ERR(lcd_output_dev)) {
		ret = PTR_ERR(lcd_output_dev);
		lcd_output_dev = NULL;
		return ret;
	}
	/* Ensure LCD is on by default */
	yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_ON);

	crt_output_dev = video_output_register("CRT", NULL, NULL,
			&crt_output_properties);

	if (IS_ERR(crt_output_dev)) {
		ret = PTR_ERR(crt_output_dev);
		crt_output_dev = NULL;
		return ret;
	}

	/* Turn off CRT by default, and will be enabled when the CRT
	 * connectting event reported by SCI */
	yeeloong_crt_vo_set(BIT_CRT_DETECT_UNPLUG);

	return 0;
}

static void yeeloong_vo_exit(void)
{
	if (lcd_output_dev) {
		video_output_unregister(lcd_output_dev);
		lcd_output_dev = NULL;
	}
	if (crt_output_dev) {
		video_output_unregister(crt_output_dev);
		crt_output_dev = NULL;
	}
}

/* hotkey subdriver */

static struct input_dev *yeeloong_hotkey_dev;

static const struct key_entry yeeloong_keymap[] = {
	{KE_SW, EVENT_LID, { SW_LID } },
	{KE_KEY, EVENT_CAMERA, { KEY_CAMERA } }, /* Fn + ESC */
	{KE_KEY, EVENT_SLEEP, { KEY_SLEEP } }, /* Fn + F1 */
	{KE_KEY, EVENT_DISPLAY_TOGGLE, { KEY_SWITCHVIDEOMODE } }, /* Fn + F3 */
	{KE_KEY, EVENT_AUDIO_MUTE, { KEY_MUTE } }, /* Fn + F4 */
	{KE_KEY, EVENT_WLAN, { KEY_WLAN } }, /* Fn + F5 */
	{KE_KEY, EVENT_DISPLAY_BRIGHTNESS, { KEY_BRIGHTNESSUP } }, /* Fn + up */
	{KE_KEY, EVENT_DISPLAY_BRIGHTNESS, { KEY_BRIGHTNESSDOWN } }, /* Fn + down */
	{KE_KEY, EVENT_AUDIO_VOLUME, { KEY_VOLUMEUP } }, /* Fn + right */
	{KE_KEY, EVENT_AUDIO_VOLUME, { KEY_VOLUMEDOWN } }, /* Fn + left */
	{KE_END, 0}
};

static struct key_entry *get_event_key_entry(int event, int status)
{
	struct key_entry *ke;
	static int old_brightness_status = -1;
	static int old_volume_status = -1;

	ke = sparse_keymap_entry_from_scancode(yeeloong_hotkey_dev, event);
	if (!ke)
		return NULL;

	switch (event) {
	case EVENT_DISPLAY_BRIGHTNESS:
		/* current status > old one, means up */
		if ((status < old_brightness_status) || (0 == status))
			ke++;
		old_brightness_status = status;
		break;
	case EVENT_AUDIO_VOLUME:
		if ((status < old_volume_status) || (0 == status))
			ke++;
		old_volume_status = status;
		break;
	default:
		break;
	}

	return ke;
}

static int report_lid_switch(int status)
{
	input_report_switch(yeeloong_hotkey_dev, SW_LID, !status);
	input_sync(yeeloong_hotkey_dev);

	return status;
}

static int crt_detect_handler(int status)
{
	if (status) {
		yeeloong_crt_vo_set(BIT_CRT_DETECT_PLUG);
		yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_OFF);
	} else {
		yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_ON);
		yeeloong_crt_vo_set(BIT_CRT_DETECT_UNPLUG);
	}
	return status;
}

#define EC_VER_LEN 64

static int black_screen_handler(int status)
{
	char *p, ec_ver[EC_VER_LEN];

	p = strstr(loongson_cmdline, "EC_VER=");
	if (!p)
		memset(ec_ver, 0, EC_VER_LEN);
	else {
		strncpy(ec_ver, p, EC_VER_LEN);
		p = strstr(ec_ver, " ");
		if (p)
			*p = '\0';
	}

	/* Seems EC(>=PQ1D26) does this job for us, we can not do it again,
	 * otherwise, the brightness will not resume to the normal level! */
	if (strncasecmp(ec_ver, "EC_VER=PQ1D26", 64) < 0)
		yeeloong_lcd_vo_set(status);

	return status;
}

static int display_toggle_handler(int status)
{
	static int video_output_status;

	/* Only enable switch video output button
	 * when CRT is connected */
	if (ec_read(REG_CRT_DETECT) == BIT_CRT_DETECT_UNPLUG)
		return 0;
	/* 0. no CRT connected: LCD on, CRT off
	 * 1. BOTH on
	 * 2. LCD off, CRT on
	 * 3. BOTH off
	 * 4. LCD on, CRT off
	 */
	video_output_status++;
	if (video_output_status > 4)
		video_output_status = 1;

	switch (video_output_status) {
	case 1:
		yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_ON);
		yeeloong_crt_vo_set(BIT_CRT_DETECT_PLUG);
		break;
	case 2:
		yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_OFF);
		yeeloong_crt_vo_set(BIT_CRT_DETECT_PLUG);
		break;
	case 3:
		yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_OFF);
		yeeloong_crt_vo_set(BIT_CRT_DETECT_UNPLUG);
		break;
	case 4:
		yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_ON);
		yeeloong_crt_vo_set(BIT_CRT_DETECT_UNPLUG);
		break;
	default:
		/* Ensure LCD is on */
		yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_ON);
		break;
	}
	return video_output_status;
}

static int camera_handler(int status)
{
	int value;

	value = ec_read(REG_CAMERA_CONTROL);
	ec_write(REG_CAMERA_CONTROL, value | (1 << 1));

	return status;
}

static int usb2_handler(int status)
{
	pr_emerg("USB2 Over Current occurred\n");

	return status;
}

static int usb0_handler(int status)
{
	pr_emerg("USB0 Over Current occurred\n");

	return status;
}

static void do_event_action(int event)
{
	sci_handler handler;
	int reg, status;
	struct key_entry *ke;

	reg = 0;
	handler = NULL;

	switch (event) {
	case EVENT_LID:
		reg = REG_LID_DETECT;
		break;
	case EVENT_DISPLAY_TOGGLE:
		handler = display_toggle_handler;
		break;
	case EVENT_CRT_DETECT:
		reg = REG_CRT_DETECT;
		handler = crt_detect_handler;
		break;
	case EVENT_CAMERA:
		reg = REG_CAMERA_STATUS;
		handler = camera_handler;
		break;
	case EVENT_USB_OC2:
		reg = REG_USB2_FLAG;
		handler = usb2_handler;
		break;
	case EVENT_USB_OC0:
		reg = REG_USB0_FLAG;
		handler = usb0_handler;
		break;
	case EVENT_BLACK_SCREEN:
		reg = REG_DISPLAY_LCD;
		handler = black_screen_handler;
		break;
	case EVENT_AUDIO_MUTE:
		reg = REG_AUDIO_MUTE;
		break;
	case EVENT_DISPLAY_BRIGHTNESS:
		reg = REG_DISPLAY_BRIGHTNESS;
		break;
	case EVENT_AUDIO_VOLUME:
		reg = REG_AUDIO_VOLUME;
		break;
	default:
		break;
	}

	if (reg != 0)
		status = ec_read(reg);

	if (handler != NULL)
		status = handler(status);

	pr_info("%s: event: %d status: %d\n", __func__, event, status);

	/* Report current key to user-space */
	ke = get_event_key_entry(event, status);
	if (ke) {
		if (ke->keycode == SW_LID)
			report_lid_switch(status);
		else
			sparse_keymap_report_entry(yeeloong_hotkey_dev, ke, 1,
					true);
	}
}

/*
 * SCI(system control interrupt) main interrupt routine
 *
 * We will do the query and get event number together so the interrupt routine
 * should be longer than 120us now at least 3ms elpase for it.
 */
static irqreturn_t sci_irq_handler(int irq, void *dev_id)
{
	int ret, event;

	if (SCI_IRQ_NUM != irq)
		return IRQ_NONE;

	/* Query the event number */
	ret = ec_query_event_num();
	if (ret < 0)
		return IRQ_NONE;

	event = ec_get_event_num();
	if (event < EVENT_START || event > EVENT_END)
		return IRQ_NONE;

	/* Execute corresponding actions */
	do_event_action(event);

	return IRQ_HANDLED;
}

/*
 * Config and init some msr and gpio register properly.
 */
static int sci_irq_init(void)
{
	u32 hi, lo;
	u32 gpio_base;
	unsigned long flags;
	int ret;

	/* Get gpio base */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), &hi, &lo);
	gpio_base = lo & 0xff00;

	/* Filter the former kb3310 interrupt for security */
	ret = ec_query_event_num();
	if (ret)
		return ret;

	/* For filtering next number interrupt */
	udelay(10000);

	/* Set gpio native registers and msrs for GPIO27 SCI EVENT PIN
	 * gpio :
	 *      input, pull-up, no-invert, event-count and value 0,
	 *      no-filter, no edge mode
	 *      gpio27 map to Virtual gpio0
	 * msr :
	 *      no primary and lpc
	 *      Unrestricted Z input to IG10 from Virtual gpio 0.
	 */
	local_irq_save(flags);
	_rdmsr(0x80000024, &hi, &lo);
	lo &= ~(1 << 10);
	_wrmsr(0x80000024, hi, lo);
	_rdmsr(0x80000025, &hi, &lo);
	lo &= ~(1 << 10);
	_wrmsr(0x80000025, hi, lo);
	_rdmsr(0x80000023, &hi, &lo);
	lo |= (0x0a << 0);
	_wrmsr(0x80000023, hi, lo);
	local_irq_restore(flags);

	/* Set gpio27 as sci interrupt
	 *
	 * input, pull-up, no-fliter, no-negedge, invert
	 * the sci event is just about 120us
	 */
	asm(".set noreorder\n");
	/*  input enable */
	outl(0x00000800, (gpio_base | 0xA0));
	/*  revert the input */
	outl(0x00000800, (gpio_base | 0xA4));
	/*  event-int enable */
	outl(0x00000800, (gpio_base | 0xB8));
	asm(".set reorder\n");

	return 0;
}

static struct irqaction sci_irqaction = {
	.handler = sci_irq_handler,
	.name = "sci",
	.flags = IRQF_SHARED,
};

static int yeeloong_hotkey_init(void)
{
	int ret;

	ret = sci_irq_init();
	if (ret)
		return -EFAULT;

	ret = setup_irq(SCI_IRQ_NUM, &sci_irqaction);
	if (ret)
		return -EFAULT;

	yeeloong_hotkey_dev = input_allocate_device();

	if (!yeeloong_hotkey_dev) {
		remove_irq(SCI_IRQ_NUM, &sci_irqaction);
		return -ENOMEM;
	}

	yeeloong_hotkey_dev->name = "HotKeys";
	yeeloong_hotkey_dev->phys = "button/input0";
	yeeloong_hotkey_dev->id.bustype = BUS_HOST;
	yeeloong_hotkey_dev->dev.parent = NULL;

	ret = sparse_keymap_setup(yeeloong_hotkey_dev, yeeloong_keymap, NULL);
	if (ret) {
		pr_err("Fail to setup input device keymap\n");
		input_free_device(yeeloong_hotkey_dev);
		return ret;
	}

	ret = input_register_device(yeeloong_hotkey_dev);
	if (ret) {
		sparse_keymap_free(yeeloong_hotkey_dev);
		input_free_device(yeeloong_hotkey_dev);
		return ret;
	}

	/* Update the current status of LID */
	report_lid_switch(BIT_LID_DETECT_ON);

#ifdef CONFIG_LOONGSON_SUSPEND
	/* Install the real yeeloong_report_lid_status for pm.c */
	yeeloong_report_lid_status = report_lid_switch;
#endif

	return 0;
}

static void yeeloong_hotkey_exit(void)
{
	/* Free irq */
	remove_irq(SCI_IRQ_NUM, &sci_irqaction);

#ifdef CONFIG_LOONGSON_SUSPEND
	/* Uninstall yeeloong_report_lid_status for pm.c */
	if (yeeloong_report_lid_status == report_lid_switch)
		yeeloong_report_lid_status = NULL;
#endif

	if (yeeloong_hotkey_dev) {
		sparse_keymap_free(yeeloong_hotkey_dev);
		input_unregister_device(yeeloong_hotkey_dev);
		yeeloong_hotkey_dev = NULL;
	}
}

#ifdef CONFIG_PM
static void usb_ports_set(int status)
{
	status = !!status;

	ec_write(REG_USB0_FLAG, status);
	ec_write(REG_USB1_FLAG, status);
	ec_write(REG_USB2_FLAG, status);
}

static int yeeloong_suspend(struct device *dev)

{
	yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_OFF);
	yeeloong_crt_vo_set(BIT_CRT_DETECT_UNPLUG);
	usb_ports_set(BIT_USB_FLAG_OFF);

	return 0;
}

static int yeeloong_resume(struct device *dev)
{
	yeeloong_lcd_vo_set(BIT_DISPLAY_LCD_ON);
	yeeloong_crt_vo_set(BIT_CRT_DETECT_PLUG);
	usb_ports_set(BIT_USB_FLAG_ON);

	return 0;
}

static const SIMPLE_DEV_PM_OPS(yeeloong_pm_ops, yeeloong_suspend,
	yeeloong_resume);
#endif

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
#ifdef CONFIG_PM
		.pm = &yeeloong_pm_ops,
#endif
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

	ret = yeeloong_vo_init();
	if (ret) {
		pr_err("Fail to register yeeloong video output driver.\n");
		yeeloong_vo_exit();
		return ret;
	}

	ret = yeeloong_hotkey_init();
	if (ret) {
		pr_err("Fail to register yeeloong hotkey driver.\n");
		yeeloong_hotkey_exit();
		return ret;
	}

	return 0;
}

static void __exit yeeloong_exit(void)
{
	yeeloong_hotkey_exit();
	yeeloong_vo_exit();
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
