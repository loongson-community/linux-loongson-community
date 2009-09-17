/*
 *  Driver for YeeLoong laptop extras
 *
 *  Copyright (C) 2009 Lemote Inc.
 *  Author: Wu Zhangjin <wuzj@lemote.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/video_output.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>

#include <loongson.h>

#include <ec/kb3310b.h>
#include <cs5536/cs5536.h>

/********************** backlight sub driver *****************/
#define MAX_BRIGHTNESS 8
#define DEFAULT_BRIGHTNESS (MAX_BRIGHTNESS - 1)

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

	/* avoid tune the brightness when the EC is tuning it */
	current_level = ec_read(REG_DISPLAY_BRIGHTNESS);
	if ((old_level == current_level) && (old_level != level))
		ec_write(REG_DISPLAY_BRIGHTNESS, level);
	old_level = level;

	return 0;
}

static int yeeloong_get_brightness(struct backlight_device *bd)
{
	return (int)ec_read(REG_DISPLAY_BRIGHTNESS);
}

static struct backlight_ops backlight_ops = {
	.get_brightness = yeeloong_get_brightness,
	.update_status = yeeloong_set_brightness,
};

static struct backlight_device *yeeloong_backlight_dev;

static int yeeloong_backlight_init(struct device *dev)
{
	int ret;

	yeeloong_backlight_dev = backlight_device_register("backlight0",
							   dev, NULL,
							   &backlight_ops);

	if (IS_ERR(yeeloong_backlight_dev)) {
		ret = PTR_ERR(yeeloong_backlight_dev);
		yeeloong_backlight_dev = NULL;
		return ret;
	}

	yeeloong_backlight_dev->props.max_brightness = MAX_BRIGHTNESS;
	yeeloong_backlight_dev->props.brightness = DEFAULT_BRIGHTNESS;
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

/********************* hwmon sub driver **************************/

/* pwm(auto/manual) enable or not */
static int yeeloong_get_fan_pwm_enable(void)
{
	/* This get the fan control method: auto or manual */
	return ec_read(REG_FAN_AUTO_MAN_SWITCH);
}

static void yeeloong_set_fan_pwm_enable(int manual)
{
	ec_write(REG_FAN_AUTO_MAN_SWITCH, !!manual);
}

static int yeeloong_get_fan_pwm(void)
{
	/* fan speed level */
	return ec_read(REG_FAN_SPEED_LEVEL);
}

static void yeeloong_set_fan_pwm(int value)
{
	int status;

	value = SENSORS_LIMIT(value, 0, 3);

	/* if value is not ZERO, we should ensure it is on */
	if (value != 0) {
		status = ec_read(REG_FAN_STATUS);
		if (status == 0)
			ec_write(REG_FAN_CONTROL, BIT_FAN_CONTROL_ON);
	}
	/* 0xf4cc is for writing */
	ec_write(REG_FAN_SPEED_LEVEL, value);
}

static int yeeloong_get_fan_rpm(void)
{
	int value = 0;

	value = FAN_SPEED_DIVIDER /
	    (((ec_read(REG_FAN_SPEED_HIGH) & 0x0f) << 8) |
	     ec_read(REG_FAN_SPEED_LOW));

	return value;
}

static int yeeloong_get_cpu_temp(void)
{
	int value;

	value = ec_read(REG_TEMPERATURE_VALUE);

	if (value & (1 << 7))
		value = (value & 0x7f) - 128;
	else
		value = value & 0xff;

	return value * 1000;
}

static int parse_arg(const char *buf, unsigned long count, int *val)
{
	if (!count)
		return 0;
	if (sscanf(buf, "%i", val) != 1)
		return -EINVAL;
	return count;
}

static ssize_t store_sys_hwmon(void (*set) (int), const char *buf, size_t count)
{
	int rv, value;

	rv = parse_arg(buf, count, &value);
	if (rv > 0)
		set(value);
	return rv;
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

CREATE_SENSOR_ATTR(fan1_input, S_IRUGO, yeeloong_get_fan_rpm, NULL);
CREATE_SENSOR_ATTR(pwm1, S_IRUGO | S_IWUSR,
		   yeeloong_get_fan_pwm, yeeloong_set_fan_pwm);
CREATE_SENSOR_ATTR(pwm1_enable, S_IRUGO | S_IWUSR,
		   yeeloong_get_fan_pwm_enable, yeeloong_set_fan_pwm_enable);
CREATE_SENSOR_ATTR(temp1_input, S_IRUGO, yeeloong_get_cpu_temp, NULL);

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
	&sensor_dev_attr_name.dev_attr.attr,
	NULL
};

static struct attribute_group hwmon_attribute_group = {
	.attrs = hwmon_attributes
};

struct device *yeeloong_hwmon_dev;

static int yeeloong_hwmon_init(struct device *dev)
{
	int ret;

	yeeloong_hwmon_dev = hwmon_device_register(dev);
	if (IS_ERR(yeeloong_hwmon_dev)) {
		printk(KERN_INFO "Could not register yeeloong hwmon device\n");
		yeeloong_hwmon_dev = NULL;
		return PTR_ERR(yeeloong_hwmon_dev);
	}
	ret = sysfs_create_group(&yeeloong_hwmon_dev->kobj,
				 &hwmon_attribute_group);
	if (ret) {
		sysfs_remove_group(&yeeloong_hwmon_dev->kobj,
				   &hwmon_attribute_group);
		hwmon_device_unregister(yeeloong_hwmon_dev);
		yeeloong_hwmon_dev = NULL;
	}
	/* ensure fan is set to auto mode */
	yeeloong_set_fan_pwm_enable(0);

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

/********************* video output sub driver ****************************/

static int lcd_video_output_get(struct output_device *od)
{
	return ec_read(REG_DISPLAY_LCD);
}

static int lcd_video_output_set(struct output_device *od)
{
	unsigned long status = od->request_state;
	int value;

	if (status) {
		/* open LCD */
		outb(0x31, 0x3c4);
		value = inb(0x3c5);
		value = (value & 0xf8) | 0x03;
		outb(0x31, 0x3c4);
		outb(value, 0x3c5);
		/* ensure the brightness is suitable */
		ec_write(REG_DISPLAY_BRIGHTNESS, DEFAULT_BRIGHTNESS);
		/* open backlight */
		ec_write(REG_BACKLIGHT_CTRL, BIT_BACKLIGHT_ON);
	} else {
		/* close backlight */
		ec_write(REG_BACKLIGHT_CTRL, BIT_BACKLIGHT_OFF);
		/* close LCD */
		outb(0x31, 0x3c4);
		value = inb(0x3c5);
		value = (value & 0xf8) | 0x02;
		outb(0x31, 0x3c4);
		outb(value, 0x3c5);
	}

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
	unsigned long status = od->request_state;
	int value;

	if (status) {
		if (ec_read(REG_CRT_DETECT)) {
			/* open CRT */
			outb(0x21, 0x3c4);
			value = inb(0x3c5);
			value &= ~(1 << 7);
			outb(0x21, 0x3c4);
			outb(value, 0x3c5);
		}
	} else {
		/* close CRT */
		outb(0x21, 0x3c4);
		value = inb(0x3c5);
		value |= (1 << 7);
		outb(0x21, 0x3c4);
		outb(value, 0x3c5);
	}

	return 0;
}

static struct output_properties crt_output_properties = {
	.set_state = crt_video_output_set,
	.get_status = crt_video_output_get,
};

struct output_device *lcd_output_dev, *crt_output_dev;

static void lcd_vo_set(int status)
{
	lcd_output_dev->request_state = status;
	lcd_video_output_set(lcd_output_dev);
}

static void crt_vo_set(int status)
{
	crt_output_dev->request_state = status;
	crt_video_output_set(crt_output_dev);
}

static int crt_detect_handler(int status)
{
	if (status) {
		crt_vo_set(1);
		lcd_vo_set(0);
	} else {
		lcd_vo_set(1);
		crt_vo_set(0);
	}
	return status;
}

static int black_screen_handler(int status)
{
	lcd_vo_set(status);

	return status;
}

static int display_toggle_handler(int status)
{
	static int video_output_status;

	/* only enable switch video output button
	 * when CRT is connected */
	if (!ec_read(REG_CRT_DETECT))
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
		lcd_vo_set(1);
		crt_vo_set(1);
		break;
	case 2:
		lcd_vo_set(0);
		crt_vo_set(1);
		break;
	case 3:
		lcd_vo_set(0);
		crt_vo_set(0);
		break;
	case 4:
		lcd_vo_set(1);
		crt_vo_set(0);
		break;
	default:
		/* ensure LCD is on */
		lcd_vo_set(1);
		break;
	}
	return video_output_status;
}

static int yeeloong_vo_init(struct device *dev)
{
	int ret;

	/* register video output device: lcd, crt */
	lcd_output_dev = video_output_register("LCD",
					       dev, NULL,
					       &lcd_output_properties);
	if (IS_ERR(lcd_output_dev)) {
		ret = PTR_ERR(lcd_output_dev);
		lcd_output_dev = NULL;
		return ret;
	}
	/* ensure LCD is on by default */
	lcd_vo_set(1);

	crt_output_dev = video_output_register("CRT",
					       dev, NULL,
					       &crt_output_properties);
	if (IS_ERR(crt_output_dev)) {
		ret = PTR_ERR(crt_output_dev);
		crt_output_dev = NULL;
		return ret;
	}
	/* close CRT by default, and will be enabled
	 * when the CRT connectting event reported by SCI */
	crt_vo_set(0);

	/* install event handlers */
	yeeloong_install_sci_event_handler(EVENT_CRT_DETECT,
					   &crt_detect_handler);
	yeeloong_install_sci_event_handler(EVENT_BLACK_SCREEN,
					   &black_screen_handler);
	yeeloong_install_sci_event_handler(EVENT_DISPLAY_TOGGLE,
					   &display_toggle_handler);

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

/********************* Thermal cooling devices sub driver ****************************/

static int video_get_max_state(struct thermal_cooling_device *cdev, unsigned
			       long *state)
{
	*state = MAX_BRIGHTNESS;
	return 0;
}

static int video_get_cur_state(struct thermal_cooling_device *cdev, unsigned
			       long *state)
{
	static struct backlight_device *bd;

	bd = (struct backlight_device *)cdev->devdata;

	*state = yeeloong_get_brightness(bd);

	return 0;
}

static int video_set_cur_state(struct thermal_cooling_device *cdev, unsigned
			       long state)
{
	static struct backlight_device *bd;

	bd = (struct backlight_device *)cdev->devdata;

	yeeloong_backlight_dev->props.brightness = state;
	backlight_update_status(bd);

	return 0;
}

static struct thermal_cooling_device_ops video_cooling_ops = {
	.get_max_state = video_get_max_state,
	.get_cur_state = video_get_cur_state,
	.set_cur_state = video_set_cur_state,
};

static struct thermal_cooling_device *yeeloong_thermal_cdev;

/* TODO: register fan, cpu as the cooling devices */
static int yeeloong_thermal_init(struct device *dev)
{
	int ret;

	if (!dev)
		return -1;

	yeeloong_thermal_cdev = thermal_cooling_device_register("LCD",
								dev,
								&video_cooling_ops);
	if (IS_ERR(yeeloong_thermal_cdev)) {
		ret = PTR_ERR(yeeloong_thermal_cdev);
		return ret;
	}

	ret = sysfs_create_link(&dev->kobj,
				&yeeloong_thermal_cdev->device.kobj,
				"thermal_cooling");
	if (ret) {
		printk(KERN_ERR "Create sysfs link\n");
		return ret;
	}
	ret = sysfs_create_link(&yeeloong_thermal_cdev->device.kobj,
				&dev->kobj, "device");
	if (ret) {
		printk(KERN_ERR "Create sysfs link\n");
		return ret;
	}

	return 0;
}

static void yeeloong_thermal_exit(struct device *dev)
{
	if (yeeloong_thermal_cdev) {
		if (dev)
			sysfs_remove_link(&dev->kobj, "thermal_cooling");
		sysfs_remove_link(&yeeloong_thermal_cdev->device.kobj,
				  "device");
		thermal_cooling_device_unregister(yeeloong_thermal_cdev);
		yeeloong_thermal_cdev = NULL;
	}
}

/********************* hotkey input sub driver ****************************/

static struct input_dev *yeeloong_hotkey_dev;
static int event, status;

struct key_entry {
	char type;		/* See KE_* below */
	int event;		/* event from SCI */
	u16 keycode;		/* KEY_* or SW_* */
};

enum { KE_KEY, KE_SW, KE_END };

static struct key_entry yeeloong_keymap[] = {
	{KE_SW, EVENT_LID, SW_LID},
	/* CRT_DETECT should be SW_VIDEOOUT_INSERT, not included in hald-addon-input! */
	{KE_KEY, EVENT_CRT_DETECT, KEY_PROG1},
	/* no specific KEY_ found for overtemp! this should be reported in the batter subdriver */
	{KE_KEY, EVENT_OVERTEMP, KEY_PROG2},
	/*{KE_KEY, EVENT_AC_BAT, KEY_BATTERY},*/
	{KE_KEY, EVENT_CAMERA, KEY_CAMERA},	/* Fn + ESC */
	{KE_KEY, EVENT_SLEEP, KEY_SLEEP},	/* Fn + F1 */
	/* BLACK_SCREEN should be KEY_DISPLAYTOGGLE, but not included in hald-addon-input yet!! */
	{KE_KEY, EVENT_BLACK_SCREEN, KEY_PROG3},	/* Fn + F2 */
	{KE_KEY, EVENT_DISPLAY_TOGGLE, KEY_SWITCHVIDEOMODE},	/* Fn + F3 */
	{KE_KEY, EVENT_AUDIO_MUTE, KEY_MUTE},	/* Fn + F4 */
	{KE_KEY, EVENT_WLAN, KEY_WLAN},	/* Fn + F5 */
	{KE_KEY, EVENT_DISPLAY_BRIGHTNESS, KEY_BRIGHTNESSUP},	/* Fn + up */
	{KE_KEY, EVENT_DISPLAY_BRIGHTNESS, KEY_BRIGHTNESSDOWN},	/* Fn + down */
	{KE_KEY, EVENT_AUDIO_VOLUME, KEY_VOLUMEUP},	/* Fn + right */
	{KE_KEY, EVENT_AUDIO_VOLUME, KEY_VOLUMEDOWN},	/* Fn + left */
	{KE_END, 0}
};

static int yeeloong_lid_update_status(int status)
{
	input_report_switch(yeeloong_hotkey_dev, SW_LID, !status);
	input_sync(yeeloong_hotkey_dev);

	return status;
}

static void yeeloong_hotkey_update_status(int key)
{
	input_report_key(yeeloong_hotkey_dev, key, 1);
	input_sync(yeeloong_hotkey_dev);
	input_report_key(yeeloong_hotkey_dev, key, 0);
	input_sync(yeeloong_hotkey_dev);
}

static int get_event_keycode(void)
{
	struct key_entry *key;

	for (key = yeeloong_keymap; key->type != KE_END; key++) {
		if (key->event != event)
			continue;
		else {
			if (EVENT_DISPLAY_BRIGHTNESS == event) {
				static int old_brightness_status = -1;
				/* current status is higher than the old one, means up */
				if ((status < old_brightness_status)
				    || (0 == status))
					key++;
				old_brightness_status = status;
			} else if (EVENT_AUDIO_VOLUME == event) {
				static int old_volume_status = -1;
				if ((status < old_volume_status)
				    || (0 == status))
					key++;
				old_volume_status = status;
			}
			break;
		}
	}
	return key->keycode;
}

void yeeloong_report_key(void)
{
	int keycode;

	keycode = get_event_keycode();

	if (keycode == SW_LID)
		yeeloong_lid_update_status(status);
	else
		yeeloong_hotkey_update_status(keycode);
}

enum { NO_REG, MUL_REG, REG_END };

int event_reg[15] = {
	REG_LID_DETECT,		/*  press the lid or not */
	NO_REG,			/*  Fn+F3 for display switch */
	NO_REG,			/*  Fn+F1 for entering sleep mode */
	MUL_REG,		/*  Over-temperature happened */
	REG_CRT_DETECT,		/*  CRT is connected */
	REG_CAMERA_STATUS,	/*  Camera is on or off */
	REG_USB2_FLAG,		/*  USB2 Over Current occurred */
	REG_USB0_FLAG,		/*  USB0 Over Current occurred */
	REG_DISPLAY_LCD,	/*  Black screen is on or off */
	REG_AUDIO_MUTE,		/*  Mute is on or off */
	REG_DISPLAY_BRIGHTNESS,	/*  LCD backlight brightness adjust */
	NO_REG,			/*  ac & battery relative issue */
	REG_AUDIO_VOLUME,	/*  Volume adjust */
	REG_WLAN,		/*  Wlan is on or off */
	REG_END
};

static int ec_get_event_status(void)
{
	int reg;

	reg = event_reg[event - EVENT_LID];

	if (reg == NO_REG)
		return 1;
	else if (reg == MUL_REG) {
		if (event == EVENT_OVERTEMP) {
			return (ec_read(REG_BAT_CHARGE_STATUS) &
				BIT_BAT_CHARGE_STATUS_OVERTEMP) >> 2;
		}
	} else if (reg != REG_END)
		return ec_read(reg);

	return -1;
}

static sci_event_handler event_handler[15];

int yeeloong_install_sci_event_handler(int event, sci_event_handler handler)
{
	if (event_handler[event - EVENT_LID] != NULL) {
		printk(KERN_INFO "There is a handler installed for event: %d\n",
		       event);
		return -1;
	}
	event_handler[event - EVENT_LID] = handler;

	return 0;
}
EXPORT_SYMBOL(yeeloong_install_sci_event_handler);

static void yeeloong_event_action(void)
{
	sci_event_handler handler;

	handler = event_handler[event - EVENT_LID];

	if (handler == NULL)
		return;

	if (event == EVENT_WLAN)
		/* notify the wifi driver to update it's status FIXME: we user
		 * set the rfkill status from user-space, we need to sync the
		 * status to the EC register, but this is not suitable to do in
		 * the rfkill subdriver. to avoid this problem, we not use the
		 * EC wifi register, just use the event as a switch. the
		 * argument 2 means we just swith the status. the return value
		 * is the real status we set.
		 */
		status = handler(2);
	else if (event == EVENT_CAMERA)
		status = handler(3);
	else
		status = handler(status);
}

/*
 * sci main interrupt routine
 *
 * we will do the query and get event number together so the interrupt routine
 * should be longer than 120us now at least 3ms elpase for it.
 */
static irqreturn_t sci_irq_handler(int irq, void *dev_id)
{
	int ret;

	if (SCI_IRQ_NUM != irq) {
		printk(KERN_ERR "%s: spurious irq.\n", __func__);
		return IRQ_NONE;
	}

	/* query the event number */
	ret = ec_query_event_num();
	if (ret < 0) {
		printk(KERN_ERR "%s: return: %d\n", __func__, ret);
		return IRQ_NONE;
	}

	event = ec_get_event_num();
	if (event < 0) {
		printk(KERN_ERR "%s: return: %d\n", __func__, event);
		return IRQ_NONE;
	}

	printk(KERN_INFO "sci event number: 0x%x\n", event);

	/* parse the event number and wake the queue */
	if ((event != 0x00) && (event != 0xff)) {
		/* get status of current event */
		status = ec_get_event_status();
		printk(KERN_INFO "%s: status: %d\n", __func__, status);
		if (status == -1)
			return IRQ_NONE;
		/* execute relative actions */
		yeeloong_event_action();
		/* report current key */
		yeeloong_report_key();
	}
	return IRQ_HANDLED;
}

/*
 * config and init some msr and gpio register properly.
 */
static int sci_irq_init(void)
{
	u32 hi, lo;
	u32 gpio_base;
	int ret = 0;
	unsigned long flags;

	/* get gpio base */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), &hi, &lo);
	gpio_base = lo & 0xff00;

	/* filter the former kb3310 interrupt for security */
	ret = ec_query_event_num();
	if (ret) {
		printk(KERN_ERR "%s: failed.\n", __func__);
		return ret;
	}

	/* for filtering next number interrupt */
	udelay(10000);

	/* set gpio native registers and msrs for GPIO27 SCI EVENT PIN
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

	/* set gpio27 as sci interrupt
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

struct irqaction sci_irqaction = {
	.handler = sci_irq_handler,
	.name = "sci",
	.flags = IRQF_SHARED,
};

static int setup_sci(void)
{
	sci_irq_init();

	setup_irq(SCI_IRQ_NUM, &sci_irqaction);

	return 0;
}

static ssize_t
ignore_store(struct device *dev,
	     struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t
show_hotkeystate(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d\n", event, status);
}

static DEVICE_ATTR(state, 0444, show_hotkeystate, ignore_store);

static struct attribute *hotkey_attributes[] = {
	&dev_attr_state.attr,
	NULL
};

static struct attribute_group hotkey_attribute_group = {
	.attrs = hotkey_attributes
};

static int camera_set(int status)
{
	int value;
	static int camera_status;

	if (status == 2)
		/* resume the old camera status */
		camera_set(camera_status);
	else if (status == 3) {
		/* revert the camera status */
		value = ec_read(REG_CAMERA_CONTROL);
		ec_write(REG_CAMERA_CONTROL, value | (1 << 1));
	} else {		/* status == 0 or status == 1 */
		status = !!status;
		camera_status = ec_read(REG_CAMERA_STATUS);
		if (status != camera_status)
			camera_set(3);
	}
	return ec_read(REG_CAMERA_STATUS);
}

static int yeeloong_hotkey_init(struct device *dev)
{
	int ret;
	struct key_entry *key;

	/* setup the system control interface */
	setup_sci();

	yeeloong_hotkey_dev = input_allocate_device();

	if (!yeeloong_hotkey_dev)
		return -ENOMEM;

	yeeloong_hotkey_dev->name = "HotKeys";
	yeeloong_hotkey_dev->phys = "button/input0";
	yeeloong_hotkey_dev->id.bustype = BUS_HOST;
	yeeloong_hotkey_dev->dev.parent = dev;

	for (key = yeeloong_keymap; key->type != KE_END; key++) {
		switch (key->type) {
		case KE_KEY:
			set_bit(EV_KEY, yeeloong_hotkey_dev->evbit);
			set_bit(key->keycode, yeeloong_hotkey_dev->keybit);
			break;
		case KE_SW:
			set_bit(EV_SW, yeeloong_hotkey_dev->evbit);
			set_bit(key->keycode, yeeloong_hotkey_dev->swbit);
			break;
		}
	}

	ret = input_register_device(yeeloong_hotkey_dev);
	if (ret) {
		input_free_device(yeeloong_hotkey_dev);
		return ret;
	}

	ret = sysfs_create_group(&yeeloong_hotkey_dev->dev.kobj,
				 &hotkey_attribute_group);
	if (ret) {
		sysfs_remove_group(&yeeloong_hotkey_dev->dev.kobj,
				   &hotkey_attribute_group);
		input_unregister_device(yeeloong_hotkey_dev);
		yeeloong_hotkey_dev = NULL;
	}
	/* update the current status of lid */
	yeeloong_lid_update_status(BIT_LID_DETECT_ON);

	/* install the real yeeloong_report_lid_status for pm.c */
	yeeloong_report_lid_status = yeeloong_lid_update_status;

	/* install event handler */
	yeeloong_install_sci_event_handler(EVENT_CAMERA, camera_set);

	return 0;
}

static void yeeloong_hotkey_exit(void)
{
	if (yeeloong_hotkey_dev) {
		sysfs_remove_group(&yeeloong_hotkey_dev->dev.kobj,
				   &hotkey_attribute_group);
		input_unregister_device(yeeloong_hotkey_dev);
		yeeloong_hotkey_dev = NULL;
	}
}

/************************* platform sub driver ****************************/
static struct platform_device *yeeloong_pdev;

#ifdef CONFIG_SUSPEND

static void usb_ports_set(int status)
{
	status = !!status;

	ec_write(REG_USB0_FLAG, status);
	ec_write(REG_USB1_FLAG, status);
	ec_write(REG_USB2_FLAG, status);
}

static int yeeloong_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_INFO "yeeloong specific suspend\n");

	/* close LCD */
	lcd_vo_set(0);
	/* close CRT */
	crt_vo_set(0);
	/* power off camera */
	camera_set(0);
	/* poweroff three usb ports */
	usb_ports_set(0);
	/* minimize the speed of FAN */
	yeeloong_set_fan_pwm_enable(1);
	yeeloong_set_fan_pwm(1);

	return 0;
}

static int yeeloong_resume(struct platform_device *pdev)
{
	printk(KERN_INFO "yeeloong specific resume\n");

	/* resume the status of lcd & crt */
	lcd_vo_set(1);
	crt_vo_set(1);

	/* power on three usb ports */
	usb_ports_set(1);

	/* resume the camera status */
	camera_set(2);

	/* resume fan to auto mode */
	yeeloong_set_fan_pwm_enable(0);

	return 0;
}
#else
static int yeeloong_suspend(struct platform_device *pdev, pm_message_t state)
{
}

static int yeeloong_resume(struct platform_device *pdev)
{
}
#endif

static struct platform_driver platform_driver = {
	.driver = {
		   .name = "yeeloong-laptop",
		   .owner = THIS_MODULE,
		   },
#ifdef CONFIG_PM
	.suspend = yeeloong_suspend,
	.resume = yeeloong_resume,
#endif
};

static ssize_t yeeloong_pdev_name_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "yeeloong laptop\n");
}

static struct device_attribute dev_attr_yeeloong_pdev_name =
__ATTR(name, S_IRUGO, yeeloong_pdev_name_show, NULL);

static int yeeloong_pdev_init(void)
{
	int ret;

	/* Register platform stuff */
	ret = platform_driver_register(&platform_driver);
	if (ret)
		return ret;

	yeeloong_pdev = platform_device_alloc("yeeloong-laptop", -1);
	if (!yeeloong_pdev) {
		ret = -ENOMEM;
		platform_driver_unregister(&platform_driver);
		return ret;
	}

	ret = platform_device_add(yeeloong_pdev);
	if (ret) {
		platform_device_put(yeeloong_pdev);
		return ret;
	}

	if (IS_ERR(yeeloong_pdev)) {
		ret = PTR_ERR(yeeloong_pdev);
		yeeloong_pdev = NULL;
		printk(KERN_INFO "unable to register platform device\n");
		return ret;
	}

	ret = device_create_file(&yeeloong_pdev->dev,
				 &dev_attr_yeeloong_pdev_name);
	if (ret) {
		printk(KERN_INFO "unable to create sysfs device attributes\n");
		return ret;
	}

	return 0;
}

static void yeeloong_pdev_exit(void)
{
	if (yeeloong_pdev) {
		platform_device_unregister(yeeloong_pdev);
		yeeloong_pdev = NULL;
		platform_driver_unregister(&platform_driver);
	}
}

static int __init yeeloong_init(void)
{
	int ret;

	ret = yeeloong_pdev_init();
	if (ret) {
		yeeloong_pdev_exit();
		printk(KERN_INFO "init yeeloong platform driver failure\n");
		return ret;
	}

	ret = yeeloong_backlight_init(&yeeloong_pdev->dev);
	if (ret) {
		yeeloong_backlight_exit();
		printk(KERN_INFO "init yeeloong backlight driver failure\n");
		return ret;
	}
	ret = yeeloong_thermal_init(&yeeloong_backlight_dev->dev);
	if (ret) {
		yeeloong_thermal_exit(&yeeloong_backlight_dev->dev);
		printk(KERN_INFO
		       "init yeeloong thermal cooling device(LCD backlight) failure\n");
		return ret;
	}

	ret = yeeloong_hwmon_init(&yeeloong_pdev->dev);
	if (ret) {
		yeeloong_hwmon_exit();
		printk(KERN_INFO "init yeeloong hwmon driver failure\n");
		return ret;
	}

	ret = yeeloong_vo_init(&yeeloong_pdev->dev);
	if (ret) {
		yeeloong_vo_exit();
		printk(KERN_INFO "init yeeloong video output driver failure\n");
		return ret;
	}

	ret = yeeloong_hotkey_init(&yeeloong_pdev->dev);
	if (ret) {
		yeeloong_hotkey_exit();
		printk(KERN_INFO "init yeeloong hotkey driver failure\n");
		return ret;
	}

	return 0;
}

static void __exit yeeloong_exit(void)
{
	yeeloong_pdev_exit();
	yeeloong_thermal_exit(&yeeloong_backlight_dev->dev);
	yeeloong_backlight_exit();
	yeeloong_hwmon_exit();
	yeeloong_vo_exit();
	yeeloong_hotkey_exit();
}

module_init(yeeloong_init);
module_exit(yeeloong_exit);

MODULE_AUTHOR("Wu Zhangjin <wuzj@lemote.com>");
MODULE_DESCRIPTION("YeeLoong laptop driver");
MODULE_LICENSE("GPL");
