/*
 * hid-gdium  --  Gdium laptop function keys
 *
 * Arnaud Patard <apatard@mandriva.com>
 *
 * Based on hid-apple.c
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */


#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/usb.h>

#include "hid-ids.h"

#define GDIUM_FN_ON	1

static int fnmode = GDIUM_FN_ON;
module_param(fnmode, int, 0644);
MODULE_PARM_DESC(fnmode, "Mode of fn key on Gdium (0 = disabled, 1 = Enabled)");

struct gdium_data {
	unsigned int fn_on;
};


struct gdium_key_translation {
	u16 from;
	u16 to;
};

static struct gdium_key_translation gdium_fn_keys[] = {
	{ KEY_F1,	KEY_CAMERA },
	{ KEY_F2,	KEY_CONNECT },
	{ KEY_F3,	KEY_MUTE },
	{ KEY_F4,	KEY_VOLUMEUP},
	{ KEY_F5,	KEY_VOLUMEDOWN },
	{ KEY_F6,	KEY_SWITCHVIDEOMODE },
	{ KEY_F7,	KEY_F19 }, /* F7+12. Have to use existant keycodes */
	{ KEY_F8,	KEY_BRIGHTNESSUP },
	{ KEY_F9,	KEY_BRIGHTNESSDOWN },
	{ KEY_F10,	KEY_SLEEP },
	{ KEY_F11,	KEY_PROG1 },
	{ KEY_F12,	KEY_PROG2 },
	{ KEY_UP,	KEY_PAGEUP },
	{ KEY_DOWN,	KEY_PAGEDOWN },
	{ KEY_INSERT,	KEY_NUMLOCK },
	{ KEY_DELETE,	KEY_SCROLLLOCK },
	{ KEY_T,	KEY_STOPCD },
	{ KEY_F,	KEY_PREVIOUSSONG },
	{ KEY_H,	KEY_NEXTSONG },
	{ KEY_G,        KEY_PLAYPAUSE },
	{ }
};

static struct gdium_key_translation *gdium_find_translation(
		struct gdium_key_translation *table, u16 from)
{
	struct gdium_key_translation *trans;

	/* Look for the translation */
	for (trans = table; trans->from; trans++)
		if (trans->from == from)
			return trans;
	return NULL;
}

static int hidinput_gdium_event(struct hid_device *hid, struct input_dev *input,
		struct hid_usage *usage, __s32 value)
{
	struct gdium_data *data = hid_get_drvdata(hid);
	struct gdium_key_translation *trans;
	int do_translate;

	if (usage->type != EV_KEY)
		return 0;

	if ((usage->code == KEY_FN)) {
		data->fn_on = !!value;
		input_event(input, usage->type, usage->code, value);
		return 1;
	}

	if (fnmode) {
		trans = gdium_find_translation(gdium_fn_keys, usage->code);
		if (trans) {
			do_translate = data->fn_on;
			if (do_translate) {
				input_event(input, usage->type, trans->to, value);
				return 1;
			}
		}
	}

	return 0;
}

static int gdium_input_event(struct hid_device *hdev, struct hid_field *field,
			struct hid_usage *usage, __s32 value)
{
	if (!(hdev->claimed & HID_CLAIMED_INPUT) || !field->hidinput || !usage->type)
		return 0;

	if (hidinput_gdium_event(hdev, field->hidinput->input, usage, value))
		return 1;

	return 0;
}


static void gdium_input_setup(struct input_dev *input)
{
	struct gdium_key_translation *trans;

	set_bit(KEY_NUMLOCK, input->keybit);

	/* Enable all needed keys */
	for (trans = gdium_fn_keys; trans->from; trans++)
		set_bit(trans->to, input->keybit);
}

static int gdium_input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	if (((usage->hid & HID_USAGE_PAGE) == HID_UP_KEYBOARD)
			&& ((usage->hid & HID_USAGE) == 0x82)) {
		hid_map_usage_clear(hi, usage, bit, max, EV_KEY, KEY_FN);
		gdium_input_setup(hi->input);
		return 1;
	}
	return 0;
}

static int gdium_input_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	struct gdium_data *data;
	int ret;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(&hdev->dev, "can't alloc gdium keyboard data\n");
		return -ENOMEM;
	}

	hid_set_drvdata(hdev, data);

	ret = hid_parse(hdev);
	if (ret) {
		dev_err(&hdev->dev, "parse failed\n");
		goto err_free;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		dev_err(&hdev->dev, "hw start failed\n");
		goto err_free;
	}

	return 0;
err_free:
	kfree(data);
	return ret;
}
static void gdium_input_remove(struct hid_device *hdev)
{
	hid_hw_stop(hdev);
	kfree(hid_get_drvdata(hdev));
}

static const struct hid_device_id gdium_input_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_GDIUM, USB_DEVICE_ID_GDIUM) },
	{}
};
MODULE_DEVICE_TABLE(hid, gdium_input_devices);

static struct hid_driver gdium_input_driver = {
	.name = "gdium-fnkeys",
	.id_table = gdium_input_devices,
	.probe = gdium_input_probe,
	.remove = gdium_input_remove,
	.event = gdium_input_event,
	.input_mapping = gdium_input_mapping,
};

static int gdium_input_init(void)
{
	int ret;

	ret = hid_register_driver(&gdium_input_driver);
	if (ret)
		 pr_err("can't register gdium keyboard driver\n");

	return ret;
}
static void gdium_input_exit(void)
{
	hid_unregister_driver(&gdium_input_driver);
}

module_init(gdium_input_init);
module_exit(gdium_input_exit);
MODULE_LICENSE("GPL");

