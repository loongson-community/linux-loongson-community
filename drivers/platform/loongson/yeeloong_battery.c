/*
 * EC(Embedded Controller) KB3310B battery management driver on Linux
 *
 * Copyright (C) 2008 Lemote Inc.
 * Author: liujl <liujl@lemote.com>
 *
 * NOTE: The SDA0/SCL0 in KB3310B are used to communicate with the battery
 * IC, Here we use DS2786 IC to handle the battery management.  All the
 * resources for handle battery management in KB3310B are:
 *  1, one SMBus interface with port 0
 *  2, gpio40 output for charge enable
 */

#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/capability.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include <asm/delay.h>

#include <ec/kb3310b.h>

#ifndef	APM_32_BIT_SUPPORT
#define	APM_32_BIT_SUPPORT	0x0002
#endif

/* The EC Battery device, here is his minor number. */
#define	ECBAT_MINOR_DEV		MISC_DYNAMIC_MINOR

/*
 * apm threshold : percent unit.
 */
#define	BAT_MAX_THRESHOLD		99
#define	BAT_MIN_THRESHOLD		5

/*
 * This structure gets filled in by the machine specific 'get_power_status'
 * implementation.  Any fields which are not set default to a safe value.
 */
struct apm_pwr_info {
	unsigned char ac_line_status;
#define APM_AC_OFFLINE          0
#define APM_AC_ONLINE           1
#define APM_AC_BACKUP           2
#define APM_AC_UNKNOWN          0xff

	unsigned char battery_status;
#define APM_BATTERY_STATUS_HIGH     0
#define APM_BATTERY_STATUS_LOW      1
#define APM_BATTERY_STATUS_CRITICAL 2
#define APM_BATTERY_STATUS_CHARGING 3
#define APM_BATTERY_STATUS_NOT_PRESENT  4
#define APM_BATTERY_STATUS_UNKNOWN  0xff

	unsigned char battery_flag;
#define APM_BATTERY_FLAG_HIGH       (1 << 0)
#define APM_BATTERY_FLAG_LOW        (1 << 1)
#define APM_BATTERY_FLAG_CRITICAL   (1 << 2)
#define APM_BATTERY_FLAG_CHARGING   (1 << 3)
#define APM_BATTERY_FLAG_NOT_PRESENT    (1 << 7)
#define APM_BATTERY_FLAG_UNKNOWN    0xff

	int battery_life;
	int time;
	int units;
#define APM_UNITS_MINS          0
#define APM_UNITS_SECS          1
#define APM_UNITS_UNKNOWN       -1

	/* battery designed capacity */
	unsigned int bat_design_cap;
	/* battery designed voltage */
	unsigned int bat_design_vol;
	/* battery capacity after full charged */
	unsigned int bat_full_charged_cap;
	/* battery vendor number */
	unsigned char bat_vendor;
	/* battery cell count */
	unsigned char bat_cell_count;

	/* battery dynamic charge/discharge voltage */
	unsigned int bat_voltage;
	/* battery dynamic charge/discharge  current */
	int bat_current;
	/* battery current temperature */
	unsigned int bat_temperature;
};

static DEFINE_MUTEX(bat_info_lock);
struct bat_info {
	unsigned int ac_in;
	unsigned int bat_in;
	unsigned int bat_flag;
	/* we use capacity for caculating the life and time */
	unsigned int curr_bat_cap;

	/* battery designed capacity */
	unsigned int bat_design_cap;
	/* battery designed voltage */
	unsigned int bat_design_vol;
	/* battery capacity after full charged */
	unsigned int bat_full_charged_cap;
	/* battery vendor number */
	unsigned char bat_vendor;
	/* battery cell count */
	unsigned char bat_cell_count;

	/* battery dynamic charge/discharge voltage */
	unsigned int bat_voltage;
	/* battery dynamic charge/discharge  current */
	int bat_current;
	/* battery current temperature */
	unsigned int bat_temperature;
} bat_info = {
	.ac_in = APM_AC_UNKNOWN,
	.bat_in = APM_BATTERY_STATUS_UNKNOWN,
	.curr_bat_cap = 0,
	/* fixed value */
	.bat_design_cap = 0,
	.bat_full_charged_cap = 0,
	.bat_design_vol = 0,
	.bat_vendor = 0,
	.bat_cell_count = 0,
	/* rest variable */
	.bat_voltage = 0,
	.bat_current = 0,
	.bat_temperature = 0
};

/*
 * 1.4 :
 * 	1, 89inch basic work version.
 *	2, you should set xorg.conf ServerFlag of "NoPM" to true
 * 1.5 :
 *	1, bat_flag is added to struct bat_info.
 *	2, BAT_MIN_THRESHOLD is changed to 5%.
 */
static const char driver_version[] = "1.38";	/* no spaces */

static struct miscdevice apm_device = {
	.minor = ECBAT_MINOR_DEV,
	.name = "apm_bios",
	.fops = NULL
};

static void get_battery_fixed_info(void)
{
	bat_info.bat_design_cap = (ec_read(REG_BAT_DESIGN_CAP_HIGH) << 8)
	    | ec_read(REG_BAT_DESIGN_CAP_LOW);
	bat_info.bat_full_charged_cap = (ec_read(REG_BAT_FULLCHG_CAP_HIGH) << 8)
	    | ec_read(REG_BAT_FULLCHG_CAP_LOW);
	bat_info.bat_design_vol = (ec_read(REG_BAT_DESIGN_VOL_HIGH) << 8)
	    | ec_read(REG_BAT_DESIGN_VOL_LOW);
	bat_info.bat_vendor = ec_read(REG_BAT_VENDOR);
	bat_info.bat_cell_count = ec_read(REG_BAT_CELL_COUNT);
	if (bat_info.bat_vendor != 0) {
		printk(KERN_INFO
		       "battery vendor(%s), cells count(%d), "
		       "with designed capacity(%d),designed voltage(%d),"
		       " full charged capacity(%d)\n",
		       (bat_info.bat_vendor ==
			FLAG_BAT_VENDOR_SANYO) ? "SANYO" : "SIMPLO",
		       (bat_info.bat_cell_count == FLAG_BAT_CELL_3S1P) ? 3 : 6,
		       bat_info.bat_design_cap, bat_info.bat_design_vol,
		       bat_info.bat_full_charged_cap);
	}
}

static void get_battery_variable_info(void)
{
	unsigned char bat_charge;
	unsigned char power_flag;
	unsigned char bat_status;
	unsigned char charge_status;

	mutex_lock(&bat_info_lock);

	bat_charge = ec_read(REG_BAT_CHARGE);
	power_flag = ec_read(REG_BAT_POWER);
	bat_status = ec_read(REG_BAT_STATUS);
	charge_status = ec_read(REG_BAT_CHARGE_STATUS);
	bat_info.bat_voltage =
	    (ec_read(REG_BAT_VOLTAGE_HIGH) << 8) |
	    (ec_read(REG_BAT_VOLTAGE_LOW));
	bat_info.bat_current =
	    (ec_read(REG_BAT_CURRENT_HIGH) << 8) |
	    (ec_read(REG_BAT_CURRENT_LOW));
	bat_info.bat_temperature =
	    (ec_read(REG_BAT_TEMPERATURE_HIGH) << 8) |
	    (ec_read(REG_BAT_TEMPERATURE_LOW));
	bat_info.curr_bat_cap =
	    (ec_read(REG_BAT_RELATIVE_CAP_HIGH) << 8) |
	    (ec_read(REG_BAT_RELATIVE_CAP_LOW));

	bat_info.ac_in =
	    (power_flag & BIT_BAT_POWER_ACIN) ? APM_AC_ONLINE :
	    APM_AC_OFFLINE;
	if (!(bat_status & BIT_BAT_STATUS_IN)) {
		/* there is no battery inserted */
		bat_info.bat_in = APM_BATTERY_STATUS_NOT_PRESENT;
		bat_info.bat_flag = APM_BATTERY_FLAG_NOT_PRESENT;
	} else {
		/* there is adapter inserted */
		if (bat_info.ac_in == APM_AC_ONLINE) {
			/* if the battery is not fully charged */
			if (!(bat_status & BIT_BAT_STATUS_FULL)) {
				bat_info.bat_in =
				    APM_BATTERY_STATUS_CHARGING;
				bat_info.bat_flag =
				    APM_BATTERY_FLAG_CHARGING;
			} else {
				/* if the battery is fully charged */
				bat_info.bat_in =
				    APM_BATTERY_STATUS_HIGH;
				bat_info.bat_flag =
				    APM_BATTERY_FLAG_HIGH;
				bat_info.curr_bat_cap = 100;
			}
		} else {
			/* if the battery is too low */
			if (bat_status & BIT_BAT_STATUS_LOW) {
				bat_info.bat_in =
				    APM_BATTERY_STATUS_LOW;
				bat_info.bat_flag =
				    APM_BATTERY_FLAG_LOW;
				if (bat_info.curr_bat_cap <=
				    BAT_MIN_THRESHOLD) {
					bat_info.bat_in =
					    APM_BATTERY_STATUS_CRITICAL;
					bat_info.bat_flag =
					    APM_BATTERY_FLAG_CRITICAL;
				}
				/* we should power off the system now */
			} else {
				/* assume the battery is high enough. */
				bat_info.bat_in =
				    APM_BATTERY_STATUS_HIGH;
				bat_info.bat_flag =
				    APM_BATTERY_FLAG_HIGH;
			}
		}
	}

	mutex_unlock(&bat_info_lock);
}

#ifdef	CONFIG_PROC_FS
static int bat_proc_read(char *page, char **start, off_t off, int count,
			 int *eof, void *data);
static struct proc_dir_entry *bat_proc_entry;

static int bat_proc_read(char *page, char **start, off_t off, int count,
			 int *eof, void *data)
{
	struct apm_pwr_info info;
	char *units;
	int ret;

	/* get variable battery infomation */
	get_battery_variable_info();

	mutex_lock(&bat_info_lock);
	info.battery_life = bat_info.curr_bat_cap;
	info.ac_line_status = bat_info.ac_in;
	info.battery_status = bat_info.bat_in;
	info.battery_flag = bat_info.bat_flag;
	info.bat_voltage = bat_info.bat_voltage;
	if (bat_info.bat_current & 0x8000)
		info.bat_current = 0xffff - bat_info.bat_current;
	else
		info.bat_current = bat_info.bat_current;
	info.bat_temperature = bat_info.bat_temperature;

	/* this should be charged according to the capacity-time flow. */
	if (info.battery_status != APM_BATTERY_STATUS_NOT_PRESENT)
		info.time = ((bat_info.curr_bat_cap - 3) * 54 + 142) / 60;
	else
		info.time = 0x00;
	info.units = APM_UNITS_MINS;

	mutex_unlock(&bat_info_lock);
	switch (info.units) {
	default:
		units = "?";
		break;
	case 0:
		units = "min";
		break;
	case 1:
		units = "sec";
		break;
	}

	ret =
	    sprintf(page,
		    "%s 1.2 0x%02x 0x%02x 0x%02x 0x%02x %d%% %d %s %dmV %dmA %d\n",
		    driver_version, APM_32_BIT_SUPPORT, info.ac_line_status,
		    info.battery_status, info.battery_flag, info.battery_life,
		    info.time, units, info.bat_voltage, info.bat_current,
		    info.bat_temperature);

	ret -= off;
	if (ret < off + count)
		*eof = 1;
	*start = page + off;
	if (ret > count)
		ret = count;
	if (ret < 0)
		ret = 0;

	return ret;
}
#endif

static int ac_bat_action(int status)
{
	get_battery_variable_info();

	return status;
}

static int __init apm_init(void)
{
	int ret;

	printk(KERN_INFO
	       "APM of battery on KB3310B Embedded Controller init.\n");

	/* get fixed battery infomation */
	get_battery_fixed_info();
	/* install battery event handler */
	yeeloong_install_sci_event_handler(EVENT_AC_BAT, ac_bat_action);

#ifdef CONFIG_PROC_FS
	bat_proc_entry = NULL;
	bat_proc_entry = create_proc_entry("apm", S_IWUSR | S_IRUGO, NULL);
	if (bat_proc_entry == NULL) {
		printk(KERN_ERR "EC BAT : register /proc/apm failed.\n");
		return -EINVAL;
	}
	bat_proc_entry->read_proc = bat_proc_read;
	bat_proc_entry->write_proc = NULL;
	bat_proc_entry->data = NULL;
#endif

	ret = misc_register(&apm_device);
	if (ret != 0) {
		remove_proc_entry("apm", NULL);
		printk(KERN_ERR "ecbat : misc register error.\n");
	}
	return ret;
}

static void __exit apm_exit(void)
{
	/* uninstall battery event handler */
	yeeloong_uninstall_sci_event_handler(EVENT_AC_BAT, ac_bat_action);

	misc_deregister(&apm_device);
#ifdef	CONFIG_PROC_FS
	remove_proc_entry("apm", NULL);
#endif
}

module_init(apm_init);
module_exit(apm_exit);

MODULE_AUTHOR("liujl <liujl@lemote.com>");
MODULE_DESCRIPTION("Advanced Power Management For Kb3310");
MODULE_LICENSE("GPL");
