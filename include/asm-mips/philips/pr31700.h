/*
 *  linux/include/asm-mips/philips/pr31700.h
 *
 *  Copyright (C) 2000 Pavel Machek (pavel@suse.cz)
 *  Copyright (C) 2001 Steven J. Hill (sjhill@realitydiluted.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Defines for the PR31700 processor used in the Philips Nino and Velo.
 */
#ifndef __PR31700_H__
#define __PR31700_H__

#include <asm/addrspace.h>
 
/******************************************************************************
*
* 	01  General macro definitions
*
******************************************************************************/

#define REGISTER_BASE   0xb0c00000

#ifndef _LANGUAGE_ASSEMBLY

	#define REG_AT(x)	(*((volatile unsigned long *)(REGISTER_BASE + x)))

#else

	#define REG_AT(x)   (REGISTER_BASE + x)

#endif

#define BIT(x)	(1 << x)

/******************************************************************************
*
* 	02  Bus Interface Unit
*
******************************************************************************/

#define MemConfig0    REG_AT(0x000)
#define MemConfig1    REG_AT(0x004)
#define MemConfig2    REG_AT(0x008)
#define MemConfig3    REG_AT(0x00c)
#define MemConfig4    REG_AT(0x010)
#define MemConfig5    REG_AT(0x014)
#define MemConfig6    REG_AT(0x018)
#define MemConfig7    REG_AT(0x01c)
#define MemConfig8    REG_AT(0x020)

/* Memory config register 1 */
#define MEM1_ENCS1USER	BIT(21)

/* Memory config register 3 */
#define MEM3_CARD1ACCVAL_MASK	(BIT(24) | BIT(25) | BIT(26) | BIT(27))
#define MEM3_CARD1IOEN		BIT(4)

/* Memory config register 4 */
#define MEM4_ARBITRATIONEN	BIT(29)
#define MEM4_MEMPOWERDOWN	BIT(16)
#define MEM4_ENREFRESH1		BIT(15)
#define MEM4_ENREFRESH0		BIT(14)
#define MEM4_ENWATCH            BIT(24)
#define MEM4_WATCHTIMEVAL_MASK  (0xf)
#define MEM4_WATCHTIMEVAL_SHIFT (20)
#define MEM4_WATCHTIME_VALUE    (0xf)

/******************************************************************************
*
*	06  Clock module
*
******************************************************************************/

#define ClockControl  	REG_AT(0x1C0)

#define CLK_CHICLKDIV_MASK    0xff000000
#define CLK_CHICLKDIV_SHIFT   24
#define CLK_ENCLKTEST         BIT(23)
#define CLK_CLKTESTSELSIB     BIT(22)
#define CLK_CHIMCLKSEL        BIT(21)
#define CLK_CHICLKDIR	      BIT(20)
#define CLK_ENCHIMCLK	      BIT(19)
#define CLK_ENVIDCLK	      BIT(18)
#define CLK_ENMBUSCLK         BIT(17)
#define CLK_ENSPICLK	      BIT(16)
#define CLK_ENTIMERCLK	      BIT(15)
#define CLK_ENFASTTIMERCLK    BIT(14)
#define CLK_SIBMCLKDIR	      BIT(13)
#define CLK_ENSIBMCLK	      BIT(11)
#define CLK_SIBMCLKDIV_MASK   (BIT(10) | BIT(9) | BIT(8))
#define CLK_SIBMCLKDIV_SHIFT  8
#define CLK_CSERSEL	      BIT(7)
#define CLK_CSERDIV_MASK      (BIT(6) | BIT(5) | BIT(4))
#define CLK_CSERDIV_SHIFT     4
#define CLK_ENCSERCLK	      BIT(3)
#define CLK_ENIRCLK           BIT(2)
#define CLK_EN_UART_A	      BIT(1)
#define CLK_EN_UART_B	      BIT(0)

/******************************************************************************
*
* 	07  CHI module
*
******************************************************************************/

#define CHIControl		REG_AT(0x1D8)
#define CHIPointerEnable	REG_AT(0x1DC)
#define CHIReceivePtrA		REG_AT(0x1E0)
#define CHIReceivePtrB		REG_AT(0x1E4)
#define CHITransmitPtrA		REG_AT(0x1E8)
#define CHITransmitPtrB		REG_AT(0x1EC)
#define CHISize			REG_AT(0x1F0)
#define CHIReceiveStart		REG_AT(0x1F4)
#define CHITransmitStart	REG_AT(0x1F8)
#define CHIHoldingReg		REG_AT(0x1FC)

/* CHI Control Register */
/* <incomplete!> */
#define	CHI_RXEN		BIT(2)
#define	CHI_TXEN		BIT(1)
#define	CHI_ENCHI		BIT(0)

/******************************************************************************
*
*	08  Interrupt module
*
******************************************************************************/

/* Register locations */

#define IntStatus1    REG_AT(0x100)
#define IntStatus2    REG_AT(0x104)
#define IntStatus3    REG_AT(0x108)
#define IntStatus4    REG_AT(0x10c)
#define IntStatus5    REG_AT(0x110)
#define IntStatus6    REG_AT(0x114)

#define IntClear1     REG_AT(0x100)
#define IntClear2     REG_AT(0x104)
#define IntClear3     REG_AT(0x108)
#define IntClear4     REG_AT(0x10c)
#define IntClear5     REG_AT(0x110)
#define IntClear6     REG_AT(0x114)

#define IntEnable1    REG_AT(0x118)
#define IntEnable2    REG_AT(0x11c)
#define IntEnable3    REG_AT(0x120)
#define IntEnable4    REG_AT(0x124)
#define IntEnable5    REG_AT(0x128)
#define IntEnable6    REG_AT(0x12c)

/* Interrupt Status Register 1 at offset 100 */
#define INT1_LCDINT		BIT(31)
#define INT1_DFINT              BIT(30)
#define INT1_CHIDMAHALF		BIT(29)
#define INT1_CHIDMAFULL		BIT(28)
#define INT1_CHIDMACNTINT       BIT(27)
#define INT1_CHIRXAINT		BIT(26)
#define INT1_CHIRXBINT		BIT(25)
#define INT1_CHIACTINT          BIT(24)
#define INT1_CHIERRINT          BIT(23)
#define INT1_SND0_5INT          BIT(22)
#define INT1_SND1_0INT		BIT(21)
#define INT1_TEL0_5INT          BIT(20)
#define INT1_TEL1_0INT          BIT(19)
#define INT1_SNDDMACNTINT       BIT(18)
#define INT1_TELDMACNTINT       BIT(17)
#define INT1_LSNDCLIPINT        BIT(16)
#define INT1_RSNDCLIPINT        BIT(15)
#define INT1_VALSNDPOSINT       BIT(14)
#define INT1_VALSNDNEGINT       BIT(13)
#define INT1_VALTELPOSINT       BIT(12)
#define INT1_VALTELNEGINT       BIT(11)
#define INT1_SNDININT           BIT(10)
#define INT1_TELININT           BIT(9)
#define INT1_SIBSF0INT          BIT(8)
#define INT1_SIBSF1INT          BIT(7)
#define INT1_SIBIRQPOSINT       BIT(6)
#define INT1_SIBIRQNEGINT       BIT(5)

/* Interrupt Status Register 2 at offset 104 */
#define INT2_UARTARXINT		BIT(31)
#define INT2_UARTARXOVERRUN	BIT(30)
#define INT2_UARTAFRAMEINT	BIT(29)
#define INT2_UARTABREAKINT	BIT(28)
#define INT2_UARTATXINT		BIT(26)
#define INT2_UARTATXOVERRUN	BIT(25)
#define INT2_UARTAEMPTY		BIT(24)

#define INT2_UARTBRXINT		BIT(21)
#define INT2_UARTBRXOVERRUN	BIT(20)
#define INT2_UARTBFRAMEINT	BIT(29)
#define INT2_UARTBBREAKINT	BIT(18)
#define INT2_UARTBTXINT		BIT(16)
#define INT2_UARTBTXOVERRUN	BIT(15)
#define INT2_UARTBEMPTY		BIT(14)

#define INT2_UARTA_RX		(BIT(31) | BIT(30) | BIT(29) | BIT(28) | BIT(27))
#define INT2_UARTA_TX		(BIT(26) | BIT(25) | BIT(24))
#define INT2_UARTA_DMA		(BIT(23) | BIT(22))

#define INT2_UARTB_RX		(BIT(21) | BIT(20) | BIT(19) | BIT(18) | BIT(17))
#define INT2_UARTB_TX		(BIT(16) | BIT(15) | BIT(14))
#define INT2_UARTB_DMA		(BIT(13) | BIT(12))

/* Interrupt Status Register 5 */
#define INT5_RTCINT	 BIT(31)
#define INT5_ALARMINT	 BIT(30)
#define INT5_PERIODICINT BIT(29)
#define INT5_POSPWRINT 	 BIT(27)
#define INT5_NEGPWRINT	 BIT(26)
#define INT5_POSPWROKINT BIT(25)
#define INT5_NEGPWROKINT BIT(24)
#define INT5_POSONBUTINT BIT(23)
#define INT5_NEGONBUTINT BIT(22)
#define INT5_SPIAVAILINT BIT(21)        /* 0x0020 0000 */
#define INT5_SPIERRINT   BIT(20)        /* 0x0010 0000 */
#define INT5_SPIRCVINT	 BIT(19)	/* 0x0008 0000 */
#define INT5_SPIEMPTYINT BIT(18)	/* 0x0004 0000 */
#define INT5_IOPOSINT6	 BIT(13)
#define INT5_IOPOSINT5	 BIT(12)
#define INT5_IOPOSINT4	 BIT(11)
#define INT5_IOPOSINT3	 BIT(10)
#define INT5_IOPOSINT2	 BIT(9)
#define INT5_IOPOSINT1	 BIT(8)
#define INT5_IOPOSINT0	 BIT(7)
#define INT5_IONEGINT6	 BIT(6)
#define INT5_IONEGINT5	 BIT(5)
#define INT5_IONEGINT4	 BIT(4)
#define INT5_IONEGINT3	 BIT(3)
#define INT5_IONEGINT2	 BIT(2)
#define INT5_IONEGINT1	 BIT(1)
#define INT5_IONEGINT0	 BIT(0)

#define INT5_IONEGINT_SHIFT 0
#define	INT5_IONEGINT_MASK  (0x7F<<INT5_IONEGINT_SHIFT)
#define INT5_IOPOSINT_SHIFT 7
#define	INT5_IOPOSINT_MASK  (0x7F<<INT5_IOPOSINT_SHIFT)

/* Interrupt Status Register 6 */
#define INT6_IRQHIGH	BIT(31)
#define INT6_IRQLOW	BIT(30)
#define INT6_INTVECT	(BIT(5) | BIT(4) | BIT(3) | BIT(2))


/* Interrupt Enable Register 6 */
#define INT6_GLOBALEN		BIT(18)
#define INT6_PWROKINT		BIT(15)
#define	INT6_ALARMINT		BIT(14)
#define INT6_PERIODICINT 	BIT(13)
#define	INT6_MBUSINT		BIT(12)
#define INT6_UARTARXINT		BIT(11)
#define INT6_UARTBRXINT		BIT(10)
#define	INT6_MFIOPOSINT1619	BIT(9)
#define INT6_IOPOSINT56         BIT(8)
#define	INT6_MFIONEGINT1619	BIT(7)
#define INT6_IONEGINT56         BIT(6)
#define	INT6_MBUSDMAFULLINT	BIT(5)
#define	INT6_SNDDMACNTINT	BIT(4)
#define	INT6_TELDMACNTINT	BIT(3)
#define	INT6_CHIDMACNTINT	BIT(2)
#define INT6_IOPOSNEGINT0       BIT(1)

/******************************************************************************
*
*	09  GPIO and MFIO modules
*
******************************************************************************/

#define IOControl   	REG_AT(0x180)
#define MFIOOutput   	REG_AT(0x184)
#define MFIODirection  	REG_AT(0x188)
#define MFIOInput  	REG_AT(0x18c)
#define MFIOSelect   	REG_AT(0x190)
#define IOPowerDown   	REG_AT(0x194)
#define MFIOPowerDown  	REG_AT(0x198)

#define IODIN_MASK      0x0000007f
#define IODIN_SHIFT     0
#define IODOUT_MASK     0x00007f00
#define IODOUT_SHIFT    8
#define IODIREC_MASK    0x007f0000
#define IODIREC_SHIFT   16
#define IODEBSEL_MASK   0x7f000000
#define IODEBSEL_SHIFT  24

/******************************************************************************
*
*	10  IR module
*
******************************************************************************/

#define IRControl1                  REG_AT(0x0a0)
#define IRControl2                  REG_AT(0x0a4)

/* IR Control 1 Register */
#define IR_CARDRET                  BIT(24)
#define IR_BAUDVAL_MASK             0x00ff0000
#define IR_BAUDVAL_SHIFT            16
#define IR_TESTIR                   BIT(4)
#define IR_DTINVERT                 BIT(3)
#define IR_RXPWR                    BIT(2)
#define IR_ENSTATE                  BIT(1)
#define IR_ENCONSM                  BIT(0)

/* IR Control 2 Register */
#define IR_PER_MASK                 0xff000000
#define IR_PER_SHIFT                24
#define IR_ONTIME_MASK              0x00ff0000
#define IR_ONTIME_SHIFT             16
#define IR_DELAYVAL_MASK            0x0000ff00
#define IR_DELAYVAL_SHIFT           8
#define IR_WAITVAL_MASK             0x000000ff
#define IR_WAITVAL_SHIFT            0

/******************************************************************************
*
*	11  Magicbus Module
*
******************************************************************************/

#define MbusCntrl1		REG_AT(0x0e0)
#define MbusCntrl2		REG_AT(0x0e4)
#define MbusDMACntrl1		REG_AT(0x0e8)
#define MbusDMACntrl2		REG_AT(0x0ec)
#define MbusDMACount		REG_AT(0x0f0)
#define MbusTxReg		REG_AT(0x0f4)
#define MbusRxReg		REG_AT(0x0f8)

#define	MBUS_CLKPOL		BIT(4)
#define	MBUS_SLAVE		BIT(3)
#define	MBUS_FSLAVE		BIT(2)
#define	MBUS_LONG		BIT(1)
#define	MBUS_ENMBUS		BIT(0)

/******************************************************************************
*
*	12  Power module
*
******************************************************************************/

#define PowerControl   	            REG_AT(0x1C4)

#define PWR_ONBUTN                  BIT(31)
#define PWR_PWRINT                  BIT(30)
#define PWR_PWROK                   BIT(29)
#define PWR_VIDRF_MASK              (BIT(28) | BIT(27))
#define PWR_VIDRF_SHIFT             27
#define PWR_SLOWBUS                 BIT(26)
#define PWR_DIVMOD                  BIT(25)
#define PWR_STPTIMERVAL_MASK        (BIT(15) | BIT(14) | BIT(13) | BIT(12))
#define PWR_STPTIMERVAL_SHIFT       12
#define PWR_ENSTPTIMER              BIT(11)
#define PWR_ENFORCESHUTDWN          BIT(10)
#define PWR_FORCESHUTDWN            BIT(9)
#define PWR_FORCESHUTDWNOCC         BIT(8)
#define PWR_SELC2MS                 BIT(7)
#define PWR_BPDBVCC3                BIT(5)
#define PWR_STOPCPU                 BIT(4)
#define PWR_DBNCONBUTN              BIT(3)
#define PWR_COLDSTART               BIT(2)
#define PWR_PWRCS                   BIT(1)
#define PWR_VCCON                   BIT(0)

/******************************************************************************
*
*	13  SIB (Serial Interconnect Bus) Module
*
******************************************************************************/

/* Register locations */
#define SIBSize	      	        REG_AT(0x060)
#define SIBSoundRXStart	      	REG_AT(0x064)
#define SIBSoundTXStart         REG_AT(0x068)
#define SIBTelecomRXStart       REG_AT(0x06C)
#define SIBTelecomTXStart       REG_AT(0x070)
#define SIBControl              REG_AT(0x074)
#define SIBSoundTXRXHolding     REG_AT(0x078)
#define SIBTelecomTXRXHolding   REG_AT(0x07C)
#define SIBSubFrame0Control     REG_AT(0x080)
#define SIBSubFrame1Control     REG_AT(0x084)
#define SIBSubFrame0Status      REG_AT(0x088)
#define SIBSubFrame1Status      REG_AT(0x08C)
#define SIBDMAControl           REG_AT(0x090)

/* SIB Size Register */
#define SIB_SNDSIZE_MASK        0x3ffc0000
#define SIB_SNDSIZE_SHIFT       18
#define SIB_TELSIZE_MASK        0x00003ffc
#define SIB_TELSIZE_SHIFT       2

/* SIB Control Register */
#define SIB_SIBIRQ              BIT(31)
#define SIB_ENCNTTEST           BIT(30)
#define SIB_ENDMATEST           BIT(29)
#define SIB_SNDMONO             BIT(28)
#define SIB_RMONOSNDIN          BIT(27)
#define SIB_SIBSCLKDIV_MASK     (BIT(26) | BIT(25) | BIT(24))
#define SIB_SIBSCLKDIV_SHIFT    24
#define SIB_TEL16               BIT(23)
#define SIB_TELFSDIV_MASK       0x007f0000
#define SIB_TELFSDIV_SHIFT      16
#define SIB_SND16               BIT(15)
#define SIB_SNDFSDIV_MASK       0x00007f00
#define SIB_SNDFSDIV_SHIFT      8
#define SIB_SELTELSF1           BIT(7)
#define SIB_SELSNDSF1           BIT(6)
#define SIB_ENTEL               BIT(5)
#define SIB_ENSND               BIT(4)
#define SIB_SIBLOOP             BIT(3)
#define SIB_ENSF1               BIT(2)
#define SIB_ENSF0               BIT(1)
#define SIB_ENSIB               BIT(0)

/* SIB Frame Format (SIBSubFrame0Status and SIBSubFrame1Status) */
#define SIB_REGISTER_EXT        BIT(31)  /* Must be zero */
#define SIB_ADDRESS_MASK        0x78000000
#define SIB_ADDRESS_SHIFT       27
#define SIB_WRITE               BIT(26)
#define SIB_AUD_VALID           BIT(17)
#define SIB_TEL_VALID           BIT(16)
#define SIB_DATA_MASK           0x00ff
#define SIB_DATA_SHIFT          0

/* SIB DMA Control Register */
#define SIB_SNDBUFF1TIME        BIT(31)
#define SIB_SNDDMALOOP          BIT(30)
#define SIB_SNDDMAPTR_MASK      0x3ffc0000
#define SIB_SNDDMAPTR_SHIFT     18
#define SIB_ENDMARXSND          BIT(17)
#define SIB_ENDMATXSND          BIT(16)
#define SIB_TELBUFF1TIME        BIT(15)
#define SIB_TELDMALOOP          BIT(14)
#define SIB_TELDMAPTR_MASK      0x00003ffc
#define SIB_TELDMAPTR_SHIFT     2
#define SIB_ENDMARXTEL          BIT(1)
#define SIB_ENDMATXTEL          BIT(0)

/******************************************************************************
*
* 	14  SPI module
*
******************************************************************************/

#define SPIControl		REG_AT(0x160)
#define SPITransmit		REG_AT(0x164)
#define SPIReceive		REG_AT(0x164)

#define SPI_SPION		BIT(17)
#define SPI_EMPTY		BIT(16)
#define SPI_DELAYVAL_MASK	(BIT(12) | BIT(13) | BIT(14) | BIT(15))
#define SPI_DELAYVAL_SHIFT	12
#define	SPI_BAUDRATE_MASK	(BIT(8) | BIT(9) | BIT(10) | BIT(11))
#define	SPI_BAUDRATE_SHIFT	8
#define	SPI_PHAPOL		BIT(5)
#define	SPI_CLKPOL		BIT(4)
#define	SPI_WORD		BIT(2)
#define	SPI_LSB			BIT(1)
#define	SPI_ENSPI		BIT(0)

/******************************************************************************
*
*	15  Timer module
*
******************************************************************************/

#define RTChigh	      	REG_AT(0x140)
#define RTClow	        REG_AT(0x144)
#define RTCalarmHigh    REG_AT(0x148)
#define RTCalarmLow     REG_AT(0x14c)
#define RTCtimerControl REG_AT(0x150)
#define RTCperiodTimer  REG_AT(0x154)

/* RTC Timer Control */
#define TIM_FREEZEPRE	BIT(7)
#define TIM_FREEZERTC	BIT(6)
#define TIM_FREEZETIMER	BIT(5)
#define TIM_ENPERTIMER	BIT(4)
#define TIM_RTCCLEAR	BIT(3)

#define	RTC_HIGHMASK	(0xFF)

/* RTC Periodic Timer */
#define	TIM_PERCNT	0xFFFF0000
#define	TIM_PERVAL	0x0000FFFF

/* For a system clock frequency of 36.864MHz, the timer counts at one tick
   every 868nS (ie CLK/32). Therefore 11520 counts gives a 10mS interval
 */
#define PER_TIMER_COUNT (1152000/HZ)

/******************************************************************************
*
*	16  UART module
*
******************************************************************************/

#define UartA_Ctrl1   REG_AT(0x0b0)
#define UartA_Ctrl2   REG_AT(0x0b4)
#define UartA_DMActl1 REG_AT(0x0b8)
#define UartA_DMActl2 REG_AT(0x0bc)
#define UartA_DMAcnt  REG_AT(0x0c0)
#define UartA_Data    REG_AT(0x0c4)
#define UartB_Ctrl1   REG_AT(0x0c8)
#define UartB_Ctrl2   REG_AT(0x0cc)
#define UartB_DMActl1 REG_AT(0x0d0)
#define UartB_DMActl2 REG_AT(0x0d4)
#define UartB_DMAcnt  REG_AT(0x0d8)
#define UartB_Data    REG_AT(0x0dc)

/* bit allocations within UART control register 1 */
#define UART_ON			BIT(31)	/* indicates status of UART */
#define UART_TX_EMPTY		BIT(30)	/* tx holding and shift registers empty */
#define UART_PRX_HOLD_FULL	BIT(29)	/* pre-rx holding register full */
#define UART_RX_HOLD_FULL	BIT(28)	/* rx holding register is full */
#define UART_EN_DMA_RX 		BIT(15)	/* enable rx DMA */
#define UART_EN_DMA_TX 		BIT(14)	/* enable tx DMA */
#define UART_BREAK_HALT 	BIT(12)	/* enable halt after receiving break */
#define UART_DMA_LOOP		BIT(10)	/* enable DMA loop-roud */
#define UART_PULSE_THREE	BIT(9)	/* tx data is 3 low pulses */
#define UART_PULSE_SIX		BIT(8)	/* tx data is 6 low pulses */
#define UART_DT_INVERT		BIT(7)	/* invert txd and rxd */
#define UART_DIS_TXD		BIT(6)	/* set txd low */
#define UART_LOOPBACK		BIT(4)	/* enable loopback mode */
#define UART_ENABLE		BIT(0)	/* enable UART */
		 
#define SER_SEVEN_BIT		BIT(3)	/* use 7-bit data */
#define SER_EIGHT_BIT		    0	/* use 8-bit data */
#define SER_EVEN_PARITY 	(BIT(2) | BIT(1))	/* use even parity */
#define SER_ODD_PARITY  	BIT(1)	/* use odd parity */
#define SER_NO_PARITY		    0	/* enable parity checking */
#define SER_TWO_STOP		BIT(5)	/* transmit 2 stop bits */
#define SER_ONE_STOP		    0	/* transmit 1 stop bits */

/* Baud rate definitions for UART control register 2 */
#define SER_BAUD_230400	   ( 0)
#define SER_BAUD_115200	   ( 1)
#define SER_BAUD_76800	   ( 2)
#define SER_BAUD_57600	   ( 3)
#define SER_BAUD_38400     ( 5)		/* divisors are  3.6864MHz      */
#define SER_BAUD_19200     (11)		/*		----------- - 1 */
#ifdef CONFIG_VTECH_HELIO_EMULATOR
#define SER_BAUD_9600      (23)
#else  
#define SER_BAUD_9600      (22)		/*		(baud * 16)     */
#endif
#define SER_BAUD_4800      (47)
#define SER_BAUD_2400      (95)
#define SER_BAUD_1200      (191)
#define SER_BAUD_600       (383)
#define SER_BAUD_300	   (767)

/******************************************************************************
*
*	17  Video module
*
******************************************************************************/

#define VidCtrl1	REG_AT(0x028)
#define VidCtrl2	REG_AT(0x02C)
#define VidCtrl3	REG_AT(0x030)
#define VidCtrl4	REG_AT(0x034)
#define VidCtrl5	REG_AT(0x038)
#define VidCtrl6	REG_AT(0x03C)
#define VidCtrl7	REG_AT(0x040)
#define VidCtrl8	REG_AT(0x044)
#define VidCtrl9	REG_AT(0x048)
#define VidCtrl10	REG_AT(0x04C)
#define VidCtrl11	REG_AT(0x050)
#define VidCtrl12	REG_AT(0x054)
#define VidCtrl13	REG_AT(0x058)
#define VidCtrl14	REG_AT(0x05C)

/* VidCtrl1 */
#define LINECNT         0xffc00000
#define LINECNT_SHIFT   22
#define LOADDLY         BIT(21)
#define BAUDVAL         (BIT(20) | BIT(19) | BIT(18) | BIT(17) | BIT(16))
#define BAUDVAL_SHIFT   16
#define VIDDONEVAL      (BIT(15) | BIT(14) | BIT(13) | BIT(12) | BIT(11) | BIT(10) | BIT(9))
#define VIDDONEVAL_SHIFT  9
#define ENFREEZEFRAME 	BIT(8)
#define BITSEL_MASK	0xc0
#define BITSEL_SHIFT	6
#define DISPSPLIT       BIT(5)
#define DISP8           BIT(4)
#define DFMODE          BIT(3)
#define INVVID          BIT(2)
#define DISPON		BIT(1)
#define ENVID		BIT(0)

/* VidCtrl2 */
#define VIDRATE_MASK    0xffc00000
#define VIDRATE_SHIFT   22
#define HORZVAL_MASK	0x001ff000
#define HORZVAL_SHIFT	12
#define LINEVAL_MASK	0x000001ff

/* VidCtrl3 */
#define VIDBANK_MASK	0xfff00000
#define VIDBASEHI_MASK	0x000ffff0

/* VidCtrl4 */
#define VIDBASELO_MASK	0x000ffff0

#endif	/* __PR31700_H__ */
