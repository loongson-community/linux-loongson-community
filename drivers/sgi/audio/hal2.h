/*
 * drivers/sgi/audio/hal2.h
 *
 * Copyright (C) 1998 Ulf Carlsson (ulfc@bun.falkenberg.se)
 * 
 */

#define H2_HAL2_BASE		(HPC3_CHIP0_PBASE + 0x58000)
#define H2_CTRL_PIO		(H2_HAL2_BASE + 0 * 0x200)
#define H2_AES_PIO		(H2_HAL2_BASE + 1 * 0x200)
#define H2_VOLUME_PIO		(H2_HAL2_BASE + 2 * 0x200)
#define H2_SYNTH_PIO		(H2_HAL2_BASE + 3 * 0x200)

typedef volatile unsigned short hal_reg;

struct hal2_ctrl_regs {
	hal_reg _unused0[8];
	hal_reg isr;		/* 0x10 Status Register */
	hal_reg _unused1[7];
	hal_reg rev;		/* 0x20 Revision Register */
	hal_reg _unused2[7];
	hal_reg iar;		/* 0x30 Indirect Address Register */
	hal_reg _unused3[7];
	hal_reg idr0;		/* 0x40 Indirect Data Register 0 */
	hal_reg _unused4[7];
	hal_reg idr1;		/* 0x50 Indirect Data Register 1 */
	hal_reg _unused5[7];
	hal_reg idr2;		/* 0x60 Indirect Data Register 2 */
	hal_reg _unused6[7];
	hal_reg idr3;		/* 0x70 Indirect Data Register 3 */
} *h2_ctrl = (struct hal2_ctrl_regs *) KSEG1ADDR(H2_CTRL_PIO);

struct hal2_vol_regs {
	hal_reg right;		/* 0x00 Right volume */
	hal_reg _unused0[2];
	hal_reg left;		/* 0x04 Left volume */
} *h2_vol = (struct hal2_vol_regs *) KSEG1ADDR(H2_VOLUME_PIO);

/* AES and synth regs should here if we ever support them */

/* Indirect status register */

#define H2_ISR_TSTATUS		0x01	/* RO: transaction status 1=busy */
#define H2_ISR_USTATUS		0x02	/* RO: utime status bit 1=armed */
#define H2_ISR_CODEC_MODE	0x04	/* codec mode 0=indigo 1=quad */
#define H2_ISR_GLOBAL_RESET_N	0x08	/* chip global reset 0=reset */
#define H2_ISR_CODEC_RESET_N	0x10	/* codec/synth reset 0=reset  */

	/* Revision register */

#define H2_REV_AUDIO_PRESENT	0x8000	/* RO: audio present 0=present */
#define H2_REV_BOARD_M		0x7000	/* RO: bits 14:12, board revision */
#define H2_REV_MAJOR_CHIP_M	0x00F0	/* RO: bits 7:4, major chip revision */
#define H2_REV_MINOR_CHIP_M	0x000F	/* RO: bits 3:0, minor chip revision */

/* Indirect address register */

/*
 * Address of indirect internal register to be accessed. A write to this
 * register initiates read or write access to the indirect registers in the
 * HAL2. Note that there af four indirect data registers for write access to
 * registers larger than 16 byte.
 */

#define H2_IAR_TYPE_M		0xF000	/* bits 15:12, type of functional */
					/* block the register resides in */
					/* 1=DMA Port */
					/* 9=Global DMA Control */
					/* 2=Bresenham */
					/* 3=Unix Timer */
#define H2_IAR_NUM_M		0x0F00	/* bits 11:8 instance of the */
					/* blockin which the indirect */
					/* register resides */
					/* If IAR_TYPE_M=DMA Port: */
					/* 1=Synth In */
					/* 2=AES In */
					/* 3=AES Out */
					/* 4=DAC Out */
					/* 5=ADC Out */
					/* 6=Synth Control */
					/* If IAR_TYPE_M=Global DMA Control: */
					/* 1=Control */
					/* If IAR_TYPE_M=Bresenham: */
					/* 1=Bresenham Clock Gen 1 */
					/* 2=Bresenham Clock Gen 2 */
					/* 3=Bresenham Clock Gen 3 */
					/* If IAR_TYPE_M=Unix Timer: */
					/* 1=Unix Timer */
#define H2_IAR_ACCESS_SELECT	0x0080	/* 1=read 0=write */
#define H2_IAR_PARAM		0x000C	/* Parameter Select */
#define H2_IAR_RB_INDEX_M	0x0003	/* Read Back Index */
					/* 00:word0 */
					/* 01:word1 */
					/* 10:word2 */
					/* 11:word3 */
/*
 * HAL2 internal addressing
 *
 * The HAL2 has "indirect registers" (idr) which are accessed by writing to the
 * Indirect Data registers. Write the address to the Indirect Address register
 * to transfer the data.
 *
 * We define the H2IR_* to the read address and H2IW_* to the write address and
 * H2I_* to be fields in whatever register is referred to.
 *
 * When we write to indirect registers which are larger than one word (16 bit)
 * we have to fill more than one indirect register before writing. When we read
 * back however we have to read several times, each time with different Read
 * Back Indexes (there are defs for doing this easily).
 */

/*
 * Relay Control
 */
#define H2IW_RELAY_C		0x9100 	/* state of RELAY pin signal */
#define H2IR_RELAY_C		0x9180  /* state of RELAY pin signal */
#define H2I_RELAY_C_STATE	0x01	/* state of RELAY pin signal */

#define H2IW_DMA_PORT_EN	0x09104	/* dma port enable */
#define H2IR_DMA_PORT_EN	0x9184  /* dma port enable */
#define H2I_DMA_PORT_EN_SY_IN	0x01	/*    synth_in dma port */
#define H2I_DMA_PORT_EN_AESRX	0x02	/*    aes receiver dma port */
#define H2I_DMA_PORT_EN_AESTX	0x04	/*    aes transmitter dma port */
#define H2I_DMA_PORT_EN_CODECTX	0x08	/*    codec transmit dma port */
#define H2I_DMA_PORT_EN_CODECR	0x10	/*    codec receive dma port */
					/* 0=disable 1=enable */

#define H2IW_DMA_END		0x9108 	/* global dma endian select */
#define H2IR_DMA_END		0x9188 	/* global dma endian select */
#define H2I_DMA_END_SY_IN	0x01	/*    synth_in dma port */
#define H2I_DMA_END_AESRX	0x02	/*    aes receiver dma port */
#define H2I_DMA_END_AESTX	0x04	/*    aes transmitter dma port */
#define H2I_DMA_END_CODECTX	0x08	/*    codec transmit dma port */
#define H2I_DMA_END_CODECR	0x10	/*    codec receive dma port */
					/* 0=big endian 1=little endian */

#define H2IW_DMA_DRV		0x910C  /* global dma bus drive enable */
#define H2IR_DMA_DRV		0x918C  /* global dma bus drive enable */

#define H2IW_SYNTH_C		0x1104 	/* synth dma control write */
#define H2IR_SYNTH_C		0x1184  /* synth dma control read */

#define H2IW_AESRX_C		0x1204 	/* aes rx dma control write */
#define H2IR_AESRX_C		0x1284  /* aes rx dma control read */
#define H2I_AESRX_C_TS_EN	0x20	/* timestamp enable 0=no 1=yes */
#define H2I_AESRX_C_TS_FMT	0x40	/* timestamp format */
#define H2I_AESRX_C_NAUDIO	0x80	/* pbus dma data format */
					/* 0=sign_ext 1=pass_non_audio */

#define H2IW_AESTX_C		0x1304 	/* aes tx dma control write */
#define H2IR_AESTX_C		0x1384 	/* aes tx dma control read */
#define H2I_AESTX_C_CLKID_M	0x18	/* bits 4:3, clockid */
					/* 1=Bresenham Clock Gen 1 */
					/* 2=Bresenham Clock Gen 2 */
					/* 3=Bresenham Clock Gen 3 */
#define H2I_AESTX_C_DTYPE	0x300	/* bits 9:8, datatype */
					/* 1=mono 2=stereo 3=quad */

/* DAC */

#define H2IW_DAC_C1		0x1404 	/* dac tx dma control 1 write */
#define H2IR_DAC_C1		0x1484 	/* dac tx dma control 1 read */
#define H2I_DAC_C1_CLKID	0x18	/* bits 4:3, clockid */
					/* 1=Bresenham Clock Gen 1 */
					/* 2=Bresenham Clock Gen 2 */
					/* 3=Bresenham Clock Gen 3 */
#define H2I_DAC_C1_DTYPE	0x300	/* bits 9:8 datatype */
					/* 1=mono 2=stereo 3=quad */

#define H2IW_DAC_C2		0x1408 	/* dac control 2 write word 0 */
#define H2IR_DAC_C2_0		0x1488  /* dac control 2 read word 0 */
#define H2IR_DAC_C2_1		0x1489 	/* dac control 2 read word 1 */
					/* XXX: The spec says 0x1488 */
#define H2I_DAC_C2_0_R_GAIN	0x0f	/* right a/d input gain bit 0-3 */
#define H2I_DAC_C2_0_L_GAIN	0xf0	/* left a/d input gain bit 0-3 */
#define H2I_DAC_C2_0_R_SEL	0x100	/* right a/d input select */
#define H2I_DAC_C2_0_L_SEL	0x200	/* left a/d input select */
#define H2I_DAC_C2_0_MUTE	0x400	/* 1=mute */
#define H2I_DAC_C2_1_DO1	0x01	/* digital output port bit 1 */
#define H2I_DAC_C2_1_DO2	0x02	/* digital output port bit 0 */
#define H2I_DAC_C2_1_R_ATTEN	0x7c	/* bits 6:2 right a/d output */
					/* attenuation, bit 0-4 (4=msb) */
#define H2I_DAC_C2_1_L_ATTEN	0xf80	/* bits 11:7 left a/d output */
					/* attenuation, bit 0-4 (4=msb) */
/* ADC */

#define H2IW_ADC_C1		0x1504 	/* adc tx dma control 1 write */
#define H2IR_ADC_C1		0x1584 	/* adc tx dma control 1 read */
#define H2I_ADC_C1_CLKID	0x18	/* bits 4:3, clockid */
					/* 1=Bresenham Clock Gen 1 */
					/* 2=Bresenham Clock Gen 2 */
					/* 3=Bresenham Clock Gen 3 */
#define H2I_ADC_C1_DTYPE	0x300	/* bits 9:8, datatype */
					/* 1=mono 2=stereo 3=quad */

#define H2IW_ADC_C2		0x1508 	/* adc control 2 write word 0-1 */
					/* both words have to be written at */
					/* the same time, fill idr[0-1] */
#define H2IR_ADC_C2_0		0x1588  /* adc control 2 read word 0 */
#define H2IR_ADC_C2_1		0x1589 	/* adc control 2 read word 1 */
					/* XXX: The spec says 0x1588 */
#define H2I_ADC_C2_0_R_GAIN	0x0f	/* right a/d input gain bit 0-3 */
#define H2I_ADC_C2_0_L_GAIN	0xf0	/* left a/d input gain bit 0-3 */
#define H2I_ADC_C2_0_R_SEL	0x100	/* right a/d input select */
#define H2I_ADC_C2_0_L_SEL	0x200	/* left a/d input select */
#define H2I_ADC_C2_0_MUTE	0x400	/* 1=mute */
#define H2I_ADC_C2_1_DO1	0x01	/* digital output port bit 1 */
#define H2I_ADC_C2_1_DO2	0x02	/* digital output port bit 0 */
#define H2I_ADC_C2_1_R_ATTEN	0x7c	/* bits 6:2, right a/d output */
					/* attenuation, bit 0-4 (4=msb) */
#define H2I_ADC_C2_1_L_ATTEN	0xf80	/* bits 11:7, left a/d output */
					/* attenuation, bit 0-4 (4=msb) */

#define H2IW_SYNTH_MAP_C	0x1104  /* synth dma handshake ctrl write */
#define H2IR_SYNTH_MAP_C	0x1184 	/* synth dma handshake ctrl read */

#define H2IW_BRES1_C1		0x2104 	/* clock gen 1 ctrl 1 write */
#define H2IR_BRES1_C1		0x2184 	/* clock gen 1 ctrl 1 read */
#define H2I_BRES1_C1_FSRSEL	0x03	/* master clock source */
					/* 0=48.0 1=44.1 2=aes_rx */

#define H2IW_BRES1_C2		0x2108  /* clock gen 1 ctrl 2 write word 0-1 */
#define H2IR_BRES1_C2_0		0x2188  /* clock gen 1 ctrl 2 read word 0 */
#define H2IR_BRES1_C2_1		0x2189  /* clock gen 1 ctrl 2 read word 1 */
					/* XXX: The spec says 0x2188 */
#define H2I_BRES1_C2_0_INC	0xffff	/* increment value, inc <= mod */
#define H2I_BRES1_C2_1_INC	0xffff	/* modcontrol value, */
					/* modctrl=0x00ffff & (modinc-1) */

#define H2IW_BRES2_C1		0x2204 	/* clock gen 2 ctrl 1 write */
#define H2IR_BRES2_C1		0x2284 	/* clock gen 2 ctrl 1 read */
#define H2I_BRES2_C1_FSRSEL	0x03	/* master clock source */
					/* 0=48.0 1=44.1 2=aes_rx */

#define H2IW_BRES2_C2		0x2208  /* clock gen 2 ctrl 2 write word 0-1 */
#define H2IR_BRES2_C2_0		0x2288  /* clock gen 2 ctrl 2 read word 0 */
#define H2IR_BRES2_C2_1		0x2289  /* clock gen 2 ctrl 2 read word 1 */
#define H2I_BRES2_C2_0_INC	0xffff	/* increment value, inc <= mod */
#define H2I_BRES2_C2_1_INC	0xffff	/* modcontrol value, */
					/* modctrl=0x00ffff & (modinc-1) */

#define H2IW_BRES3_C1		0x2304 	/* clock gen 3 ctrl 1 write */
#define H2IR_BRES3_C1		0x2384 	/* clock gen 3 ctrl 1 read */
#define H2I_BRES3_C1_FSRSEL	0x03	/* master clock source */
					/* 0=48.0 1=44.1 2=aes_rx */

#define H2IW_BRES3_C2		0x2308  /* clock gen 3 ctrl 2 write word 0-1 */
#define H2IR_BRES3_C2_0		0x2388  /* clock gen 3 ctrl 2 read word 0 */
#define H2IR_BRES3_C2_1		0x2389  /* clock gen 3 ctrl 2 read word 1 */
#define H2I_BRES3_C2_0_INC	0xffff	/* increment value, inc <= mod */
#define H2I_BRES3_C2_1_INC	0xffff	/* modcontrol value, */
					/* modctrl=0x00ffff & (modinc-1) */

#define H2IW_UTIME		0x3104 	/* unix timer write (preload time) */
#define H2IR_UTIME		0x3184  /* unix timer read */
#define H2I_UTIME_0_LD		0xffff	/* microseconds, LSB's */
#define H2I_UTIME_1_LD0		0x0f	/* microseconds, MSB's */
#define H2I_UTIME_1_LD1		0xf0	/* tenths of microseconds */
#define H2I_UTIME_2_LD		0xffff	/* seconds, LSB's */
#define H2I_UTIME_3_LD		0xffff	/* seconds, MSB's */
