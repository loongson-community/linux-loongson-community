/*
 * EC(Embedded Controller) KB3310B header file
 *
 *  Copyright (C) 2008 Lemote Inc. & Insititute of Computing Technology
 *  Author: liujl <liujl@lemote.com>, 2008-03-14
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>

extern unsigned char ec_read(unsigned short addr);
extern void ec_write(unsigned short addr, unsigned char val);
extern int ec_query_seq(unsigned char cmd);
extern int ec_query_event_num(void);
extern int ec_get_event_num(void);

#define SCI_IRQ_NUM 0x0A

/*
 * The following registers are determined by the EC index configuration.
 * 1, fill the PORT_HIGH as EC register high part.
 * 2, fill the PORT_LOW as EC register low part.
 * 3, fill the PORT_DATA as EC register write data or get the data from it.
 */
#define	EC_IO_PORT_HIGH	0x0381
#define	EC_IO_PORT_LOW	0x0382
#define	EC_IO_PORT_DATA	0x0383

/* ec delay time 500us for register and status access */
#define	EC_REG_DELAY	500	/* unit : us */
#define	EC_CMD_TIMEOUT		0x1000

/* EC access port for sci communication */
#define	EC_CMD_PORT		0x66
#define	EC_STS_PORT		0x66
#define	EC_DAT_PORT		0x62
#define	CMD_INIT_IDLE_MODE	0xdd
#define	CMD_EXIT_IDLE_MODE	0xdf
#define	CMD_INIT_RESET_MODE	0xd8
#define	CMD_REBOOT_SYSTEM	0x8c
#define	CMD_GET_EVENT_NUM	0x84
#define	CMD_PROGRAM_PIECE	0xda

/* temperature & fan registers */
#define	REG_TEMPERATURE_VALUE	0xF458	/*  current temperature value */
#define	REG_FAN_AUTO_MAN_SWITCH 0xF459  /*  fan auto/manual control switch */
#define	REG_FAN_CONTROL		0xF4D2	/*  fan control */
#define	BIT_FAN_CONTROL_ON	(1 << 0)
#define	BIT_FAN_CONTROL_OFF	(0 << 0)
#define	REG_FAN_STATUS		0xF4DA	/*  fan status */
#define	BIT_FAN_STATUS_ON	(1 << 0)
#define	BIT_FAN_STATUS_OFF	(0 << 0)
#define	REG_FAN_SPEED_HIGH	0xFE22	/*  fan speed high byte */
#define	REG_FAN_SPEED_LOW	0xFE23	/*  fan speed low byte */
#define	REG_FAN_SPEED_LEVEL	0xF4CC	/*  fan speed level, from 1 to 5 */
/* fan speed divider */
#define	FAN_SPEED_DIVIDER	480000	/* (60*1000*1000/62.5/2)*/

/* battery registers */
#define	REG_BAT_DESIGN_CAP_HIGH		0xF77D	/*  design capacity high byte */
#define	REG_BAT_DESIGN_CAP_LOW		0xF77E	/*  design capacity low byte */
#define	REG_BAT_FULLCHG_CAP_HIGH	0xF780	/*  full charged capacity high byte */
#define	REG_BAT_FULLCHG_CAP_LOW		0xF781	/*  full charged capacity low byte */
#define	REG_BAT_DESIGN_VOL_HIGH		0xF782	/*  design voltage high byte */
#define	REG_BAT_DESIGN_VOL_LOW		0xF783	/*  design voltage low byte */
#define	REG_BAT_CURRENT_HIGH		0xF784	/*  battery in/out current high byte */
#define	REG_BAT_CURRENT_LOW		0xF785	/*  battery in/out current low byte */
#define	REG_BAT_VOLTAGE_HIGH		0xF786	/*  battery current voltage high byte */
#define	REG_BAT_VOLTAGE_LOW		0xF787	/*  battery current voltage low byte */
#define	REG_BAT_TEMPERATURE_HIGH	0xF788	/*  battery current temperature high byte */
#define	REG_BAT_TEMPERATURE_LOW		0xF789	/*  battery current temperature low byte */
#define	REG_BAT_RELATIVE_CAP_HIGH	0xF492	/*  relative capacity high byte */
#define	REG_BAT_RELATIVE_CAP_LOW	0xF493	/*  relative capacity low byte */
#define	REG_BAT_VENDOR			0xF4C4	/*  battery vendor number */
#define	FLAG_BAT_VENDOR_SANYO		0x01
#define	FLAG_BAT_VENDOR_SIMPLO		0x02
#define	REG_BAT_CELL_COUNT		0xF4C6	/*  how many cells in one battery */
#define	FLAG_BAT_CELL_3S1P		0x03
#define	FLAG_BAT_CELL_3S2P		0x06
#define	REG_BAT_CHARGE			0xF4A2	/*  macroscope battery charging */
#define	FLAG_BAT_CHARGE_DISCHARGE	0x01
#define	FLAG_BAT_CHARGE_CHARGE		0x02
#define	FLAG_BAT_CHARGE_ACPOWER		0x00
#define	REG_BAT_STATUS			0xF4B0
#define	BIT_BAT_STATUS_LOW		(1 << 5)
#define	BIT_BAT_STATUS_DESTROY		(1 << 2)
#define	BIT_BAT_STATUS_FULL		(1 << 1)
#define	BIT_BAT_STATUS_IN		(1 << 0)
#define	REG_BAT_CHARGE_STATUS		0xF4B1
#define	BIT_BAT_CHARGE_STATUS_OVERTEMP	(1 << 2)	/*  over temperature */
#define	BIT_BAT_CHARGE_STATUS_PRECHG	(1 << 1)	/*  pre-charge the battery */
#define	REG_BAT_STATE			0xF482
#define	BIT_BAT_STATE_CHARGING		(1 << 1)
#define	BIT_BAT_STATE_DISCHARGING	(1 << 0)
#define	REG_BAT_POWER			0xF440
#define	BIT_BAT_POWER_S3		(1 << 2)	/*  enter s3 standby mode */
#define	BIT_BAT_POWER_ON		(1 << 1)	/*  system is on */
#define	BIT_BAT_POWER_ACIN		(1 << 0)	/*  adapter is inserted */

/* other registers */
#define	REG_AUDIO_MUTE			0xF4E7	/*  audio mute : rd/wr */
#define	BIT_AUDIO_MUTE_ON		(1 << 0)
#define	BIT_AUDIO_MUTE_OFF		(0 << 0)
#define	REG_AUDIO_BEEP			0xF4D0	/*  audio beep and reset : rd/wr */
#define	BIT_AUDIO_BEEP_ON		(1 << 0)
#define	BIT_AUDIO_BEEP_OFF		(0 << 0)
#define	REG_USB0_FLAG			0xF461	/*  usb0 port power or not : rd/wr */
#define	BIT_USB0_FLAG_ON		(1 << 0)
#define	BIT_USB0_FLAG_OFF		(0 << 0)
#define	REG_USB1_FLAG			0xF462	/*  usb1 port power or not : rd/wr */
#define	BIT_USB1_FLAG_ON		(1 << 0)
#define	BIT_USB1_FLAG_OFF		(0 << 0)
#define	REG_USB2_FLAG			0xF463	/*  usb2 port power or not : rd/wr */
#define	BIT_USB2_FLAG_ON		(1 << 0)
#define	BIT_USB2_FLAG_OFF		(0 << 0)
#define	REG_CRT_DETECT			0xF4AD	/*  detected CRT exist or not */
#define	BIT_CRT_DETECT_PLUG		(1 << 0)
#define	BIT_CRT_DETECT_UNPLUG		(0 << 0)
#define	REG_LID_DETECT			0xF4BD	/*  detected LID is on or not */
#define	BIT_LID_DETECT_ON		(1 << 0)
#define	BIT_LID_DETECT_OFF		(0 << 0)
#define	REG_RESET			0xF4EC	/*  reset the machine auto-clear : rd/wr */
#define	BIT_RESET_ON			(1 << 0)
#define	BIT_RESET_OFF			(0 << 0)
#define	REG_LED				0xF4C8	/*  light the led : rd/wr */
#define	BIT_LED_RED_POWER		(1 << 0)
#define	BIT_LED_ORANGE_POWER		(1 << 1)
#define	BIT_LED_GREEN_CHARGE		(1 << 2)
#define	BIT_LED_RED_CHARGE		(1 << 3)
#define	BIT_LED_NUMLOCK			(1 << 4)
#define	REG_LED_TEST			0xF4C2	/*  test led mode, all led on or off */
#define	BIT_LED_TEST_IN			(1 << 0)
#define	BIT_LED_TEST_OUT		(0 << 0)
#define	REG_DISPLAY_BRIGHTNESS		0xF4F5	/* 9 stages LCD backlight brightness adjust */
#define	REG_CAMERA_STATUS		0xF46A	/* camera is in ON/OFF status. */
#define	BIT_CAMERA_STATUS_ON		(1 << 0)
#define	BIT_CAMERA_STATUS_OFF		(0 << 0)
#define	REG_CAMERA_CONTROL		0xF7B7	/* control camera to ON/OFF. */
#define	BIT_CAMERA_CONTROL_OFF		(1 << 1)
#define	BIT_CAMERA_CONTROL_ON		(1 << 1)
#define	REG_AUDIO_VOLUME		0xF46C	/* The register to show volume level */
#define	REG_WLAN			0xF4FA	/* Wlan Status */
#define	BIT_WLAN_ON			(1 << 0)
#define	BIT_WLAN_OFF			(0 << 0)
#define	REG_DISPLAY_LCD			0xF79F	/* Black screen Status */
#define	BIT_DISPLAY_LCD_ON		(1 << 0)
#define	BIT_DISPLAY_LCD_OFF		(0 << 0)
#define	REG_BACKLIGHT_CTRL		0xF7BD	/* LCD backlight control: off/restore */
#define	BIT_BACKLIGHT_ON		(1 << 0)
#define	BIT_BACKLIGHT_OFF		(0 << 0)

/* SCI Event Number from EC */
enum {
	EVENT_LID = 0x23,	/*  press the lid or not */
	EVENT_DISPLAY_TOGGLE,	/*  Fn+F3 for display switch */
	EVENT_SLEEP,		/*  Fn+F1 for entering sleep mode */
	EVENT_OVERTEMP,		/*  Over-temperature happened */
	EVENT_CRT_DETECT,	/*  CRT is connected */
	EVENT_CAMERA,		/*  Camera is on or off */
	EVENT_USB_OC2,		/*  USB2 Over Current occurred */
	EVENT_USB_OC0,		/*  USB0 Over Current occurred */
	EVENT_BLACK_SCREEN,	/*  Black screen is on or off */
	EVENT_AUDIO_MUTE,	/*  Mute is on or off */
	EVENT_DISPLAY_BRIGHTNESS,/*  LCD backlight brightness adjust */
	EVENT_AC_BAT,		/*  ac & battery relative issue */
	EVENT_AUDIO_VOLUME,	/*  Volume adjust */
	EVENT_WLAN,		/*  Wlan is on or off */
	EVENT_END
};

enum {
	BIT_AC_BAT_BAT_IN = 0,
	BIT_AC_BAT_AC_IN,
	BIT_AC_BAT_INIT_CAP,
	BIT_AC_BAT_CHARGE_MODE,
	BIT_AC_BAT_STOP_CHARGE,
	BIT_AC_BAT_BAT_LOW,
	BIT_AC_BAT_BAT_FULL
};

typedef int (*sci_event_handler) (int status);
extern int ec_query_seq(unsigned char cmd);
extern int sci_get_event_num(void);
extern int yeeloong_install_sci_event_handler(int event, sci_event_handler handler);
extern sci_event_handler yeeloong_report_lid_status;
