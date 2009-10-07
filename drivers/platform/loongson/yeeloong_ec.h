/*
 * Driver for accessing ROM of EC on YeeLoong laptop(head file)
 *
 * Copyright (C) 2009 Lemote Inc.
 * Author: liujl <liujl@lemote.com>
 *
 * NOTE:
 * 	The application layer for reading, writing ec registers and code
 * 	program are supported.
 */

/* ec registers range */
#define	EC_MAX_REGADDR	0xFFFF
#define	EC_MIN_REGADDR	0xF000
#define	EC_RAM_ADDR	0xF800

/* version burned address */
#define	VER_ADDR	0xf7a1
#define	VER_MAX_SIZE	7
#define	EC_ROM_MAX_SIZE	0x10000

/* ec internal register */
#define	REG_POWER_MODE		0xF710
#define	FLAG_NORMAL_MODE	0x00
#define	FLAG_IDLE_MODE		0x01
#define	FLAG_RESET_MODE		0x02

/* ec update program flag */
#define	PROGRAM_FLAG_NONE	0x00
#define	PROGRAM_FLAG_IE		0x01
#define	PROGRAM_FLAG_ROM	0x02

/* XBI relative registers */
#define REG_XBISEG0     0xFEA0
#define REG_XBISEG1     0xFEA1
#define REG_XBIRSV2     0xFEA2
#define REG_XBIRSV3     0xFEA3
#define REG_XBIRSV4     0xFEA4
#define REG_XBICFG      0xFEA5
#define REG_XBICS       0xFEA6
#define REG_XBIWE       0xFEA7
#define REG_XBISPIA0    0xFEA8
#define REG_XBISPIA1    0xFEA9
#define REG_XBISPIA2    0xFEAA
#define REG_XBISPIDAT   0xFEAB
#define REG_XBISPICMD   0xFEAC
#define REG_XBISPICFG   0xFEAD
#define REG_XBISPIDATR  0xFEAE
#define REG_XBISPICFG2  0xFEAF

/* commands definition for REG_XBISPICMD */
#define	SPICMD_WRITE_STATUS		0x01
#define	SPICMD_BYTE_PROGRAM		0x02
#define	SPICMD_READ_BYTE		0x03
#define	SPICMD_WRITE_DISABLE	0x04
#define	SPICMD_READ_STATUS		0x05
#define	SPICMD_WRITE_ENABLE		0x06
#define	SPICMD_HIGH_SPEED_READ	0x0B
#define	SPICMD_POWER_DOWN		0xB9
#define	SPICMD_SST_EWSR			0x50
#define	SPICMD_SST_SEC_ERASE	0x20
#define	SPICMD_SST_BLK_ERASE	0x52
#define	SPICMD_SST_CHIP_ERASE	0x60
#define	SPICMD_FRDO				0x3B
#define	SPICMD_SEC_ERASE		0xD7
#define	SPICMD_BLK_ERASE		0xD8
#define SPICMD_CHIP_ERASE		0xC7

/* bits definition for REG_XBISPICFG */
#define	SPICFG_AUTO_CHECK		0x01
#define	SPICFG_SPI_BUSY			0x02
#define	SPICFG_DUMMY_READ		0x04
#define	SPICFG_EN_SPICMD		0x08
#define	SPICFG_LOW_SPICS		0x10
#define	SPICFG_EN_SHORT_READ	0x20
#define	SPICFG_EN_OFFSET_READ	0x40
#define	SPICFG_EN_FAST_READ		0x80

/* SMBUS relative register block according to the EC datasheet. */
#define	REG_SMBTCRC				0xff92
#define	REG_SMBPIN				0xff93
#define	REG_SMBCFG				0xff94
#define	REG_SMBEN				0xff95
#define	REG_SMBPF				0xff96
#define	REG_SMBRCRC				0xff97
#define	REG_SMBPRTCL			0xff98
#define	REG_SMBSTS				0xff99
#define	REG_SMBADR				0xff9a
#define	REG_SMBCMD				0xff9b
#define	REG_SMBDAT_START		0xff9c
#define	REG_SMBDAT_END			0xffa3
#define	SMBDAT_SIZE				8
#define	REG_SMBRSA				0xffa4
#define	REG_SMBCNT				0xffbc
#define	REG_SMBAADR				0xffbd
#define	REG_SMBADAT0			0xffbe
#define	REG_SMBADAT1			0xffbf

/* watchdog timer registers */
#define	REG_WDTCFG				0xfe80
#define	REG_WDTPF				0xfe81
#define REG_WDT					0xfe82

/* lpc configure register */
#define	REG_LPCCFG				0xfe95

/* 8051 reg */
#define	REG_PXCFG				0xff14

/* Fan register in KB3310 */
#define	REG_ECFAN_SPEED_LEVEL	0xf4e4
#define	REG_ECFAN_SWITCH		0xf4d2

/* the ec flash rom id number */
#define	EC_ROM_PRODUCT_ID_SPANSION	0x01
#define	EC_ROM_PRODUCT_ID_MXIC		0xC2
#define	EC_ROM_PRODUCT_ID_AMIC		0x37
#define	EC_ROM_PRODUCT_ID_EONIC		0x1C

/* Ec misc device name */
#define	EC_MISC_DEV		"ec_misc"

/* Ec misc device minor number */
#define	ECMISC_MINOR_DEV	MISC_DYNAMIC_MINOR

#define	EC_IOC_MAGIC		'E'
/* misc ioctl operations */
#define	IOCTL_RDREG		_IOR(EC_IOC_MAGIC, 1, int)
#define	IOCTL_WRREG		_IOW(EC_IOC_MAGIC, 2, int)
#define	IOCTL_READ_EC		_IOR(EC_IOC_MAGIC, 3, int)
#define	IOCTL_PROGRAM_IE	_IOW(EC_IOC_MAGIC, 4, int)
#define	IOCTL_PROGRAM_EC	_IOW(EC_IOC_MAGIC, 5, int)

/* start address for programming of EC content or IE */
/*  ec running code start address */
#define	EC_START_ADDR	0x00000000
/*  ec information element storing address */
#define	IE_START_ADDR	0x00020000

/* EC state */
#define	EC_STATE_IDLE	0x00	/*  ec in idle state */
#define	EC_STATE_BUSY	0x01	/*  ec in busy state */

/* timeout value for programming */
#define	EC_FLASH_TIMEOUT	0x1000	/*  ec program timeout */
/* command checkout timeout including cmd to port or state flag check */
#define	EC_CMD_TIMEOUT		0x1000
#define	EC_SPICMD_STANDARD_TIMEOUT	(4 * 1000)	/*  unit : us */
#define	EC_MAX_DELAY_UNIT	(10)	/*  every time for polling */
#define	SPI_FINISH_WAIT_TIME	10
/* EC content max size */
#define	EC_CONTENT_MAX_SIZE	(64 * 1024)
#define	IE_CONTENT_MAX_SIZE	(0x100000 - IE_START_ADDR)

/*
 * piece structure :
 *	------------------------------
 *	| 1byte | 3 bytes | 28 bytes |
 *	| flag  | addr	  | data	 |
 *	------------------------------
 *	flag :
 *		bit0 : '1' for first piece, '0' for other
 *	addr :
 *		address for EC to burn the data to(rom address)
 *	data :
 *		data which we should burn to the ec rom
 *
 * NOTE:
 *     so far max size should be 256B, or we should change the address-1 value.
 * 		piece is used for IE program
 */
#define	PIECE_SIZE		(32 - 1 - 3)
#define	FIRST_PIECE_YES	1
#define	FIRST_PIECE_NO	0
/* piece program status reg from ec firmware */
#define	PIECE_STATUS_REG	0xF77C
/* piece program status reg done flag */
#define	PIECE_STATUS_PROGRAM_DONE	0x80
/* piece program status reg error flag */
#define	PIECE_STATUS_PROGRAM_ERROR	0x40
/* 32bytes should be stored here */
#define	PIECE_START_ADDR	0xF800
