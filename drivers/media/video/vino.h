/*
 * Copyright (C) 1999 Ulf Karlsson <ulfc@bun.falkenberg.se>
 * Copyright (C) 2003 Ladislav Michl <ladis@linux-mips.org>
 */

#ifndef _vino_h_
#define _vino_h_

/* VINO AISIC register definitions */
#define VINO_BASE	0x00080000	/* Vino is in the EISA address space,
					 * but it is not an EISA bus card */

typedef volatile unsigned long	vino_reg;

#ifdef __mips64
struct sgi_vino_channel {
	vino_reg	alpha;
	vino_reg	clip_start;
	vino_reg	clip_end;
	vino_reg	frame_rate;
	vino_reg	field_counter;
	vino_reg	line_size;
	vino_reg	line_count;
	vino_reg	page_index;
	vino_reg	next_4_desc;
	vino_reg	start_desc_tbl;
	vino_reg	desc_0;
	vino_reg	desc_1;
	vino_reg	desc_2;
	vino_reg	desc_3;
	vino_reg	fifo_thres;
	vino_reg	fifo_read;
	vino_reg	fifo_write;
};

struct sgi_vino {
	vino_reg	rev_id;
	vino_reg	control;
	vino_reg	intr_status;
	vino_reg	i2c_control;
	vino_reg	i2c_data;
	struct sgi_vino_channel	a;
	struct sgi_vino_channel	b;
};

#else

struct sgi_vino_channel {
	vino_reg	alpha_;
	vino_reg	alpha;
	vino_reg	clip_start_;
	vino_reg	clip_start;
	vino_reg	clip_end_;
	vino_reg	clip_end;
	vino_reg	frame_rate_;
	vino_reg	frame_rate;
	vino_reg	field_counter_;
	vino_reg	field_counter;
	vino_reg	line_size_;
	vino_reg	line_size;
	vino_reg	line_count_;
	vino_reg	line_count;
	vino_reg	page_index_;
	vino_reg	page_index;
	vino_reg	next_4_desc_;
	vino_reg	next_4_desc;
	vino_reg	start_desc_tbl_;
	vino_reg	start_desc_tbl;
	vino_reg	desc_0_;
	vino_reg	desc_0;
	vino_reg	desc_1_;
	vino_reg	desc_1;
	vino_reg	desc_2_;
	vino_reg	desc_2;
	vino_reg	desc_3_;
	vino_reg	desc_3;
	vino_reg	fifo_thres_;
	vino_reg	fifo_thres;
	vino_reg	fifo_read_;
	vino_reg	fifo_read;
	vino_reg	fifo_write_;
	vino_reg	fifo_write;
};

struct sgi_vino {
	vino_reg	rev_id_;
	vino_reg	rev_id;
	vino_reg	control_;
	vino_reg	control;
	vino_reg	intr_status_;
	vino_reg	intr_status;
	vino_reg	i2c_control_;
	vino_reg	i2c_control;
	vino_reg	i2c_data_;
	vino_reg	i2c_data;
	struct sgi_vino_channel	a;
	struct sgi_vino_channel	b;
};
#endif

/* Bits in the rev_id register */
#define VINO_CHIP_ID		0xb
#define VINO_REV_NUM(x)		((x) & 0x0f)
#define VINO_ID_VALUE(x)	(((x) & 0xf0) >> 4)
	
/* Bits in the control register */
#define VINO_CTRL_LITTLE_ENDIAN		(1<<0)
#define VINO_CTRL_A_FIELD_TRANS_INT	(1<<1)	/* Field transferred int */
#define VINO_CTRL_A_FIFO_OF_INT		(1<<2)	/* FIFO overflow int */
#define VINO_CTRL_A_END_DESC_TBL_INT	(1<<3)	/* End of desc table int */
#define VINO_CTRL_B_FIELD_TRANS_INT	(1<<4)	/* Field transferred int */
#define VINO_CTRL_B_FIFO_OF_INT		(1<<5)	/* FIFO overflow int */
#define VINO_CTRL_B_END_DESC_TBL_INT	(1<<6)	/* End of desc table int */
#define VINO_CTRL_A_DMA_ENBL		(1<<7)
#define VINO_CTRL_A_INTERLEAVE_ENBL	(1<<8)
#define VINO_CTRL_A_SYNC_ENBL		(1<<9)
#define VINO_CTRL_A_SELECT		(1<<10)	/* 1=D1 0=Philips */
#define VINO_CTRL_A_RGB			(1<<11)	/* 1=RGB 0=YUV */
#define VINO_CTRL_A_LUMA_ONLY		(1<<12)
#define VINO_CTRL_A_DEC_ENBL		(1<<13)	/* Decimation */
#define VINO_CTRL_A_DEC_SCALE_MASK	0x1c000	/* bits 14:17 */
#define VINO_CTRL_A_DEC_HOR_ONLY	(1<<17)	/* Horizontal only */
#define VINO_CTRL_A_DITHER		(1<<18)	/* 24 -> 8 bit dither */
#define VINO_CTRL_B_DMA_ENBL		(1<<19)
#define VINO_CTRL_B_INTERLEAVE_ENBL	(1<<20)
#define VINO_CTRL_B_SYNC_ENBL		(1<<21)
#define VINO_CTRL_B_SELECT		(1<<22)	/* 1=D1 0=Philips */
#define VINO_CTRL_B_RGB			(1<<23)	/* 1=RGB 0=YUV */
#define VINO_CTRL_B_LUMA_ONLY		(1<<24)
#define VINO_CTRL_B_DEC_ENBL		(1<<25)	/* Decimation */
#define VINO_CTRL_B_DEC_SCALE_MASK	0x1c000000	/* bits 25:28 */
#define VINO_CTRL_B_DEC_HOR_ONLY	(1<<29)	/* Decimation horizontal only */
#define VINO_CTRL_B_DITHER		(1<<30)	/* ChanB 24 -> 8 bit dither */

/* Bits in the Descriptor data register */
#define VINO_DESC_JUMP			(1<<30)
#define VINO_DESC_STOP			(1<<31)
#define VINO_DESC_VALID			(1<<32)

/* Bits in the Interrupt and Status register */
#define VINO_INTSTAT_A_FIELD_TRANS	(1<<0)	/* Field transferred int */
#define VINO_INTSTAT_A_FIFO_OF		(1<<1)	/* FIFO overflow int */
#define VINO_INTSTAT_A_END_DESC_TBL	(1<<2)	/* End of desc table int */
#define VINO_INTSTAT_B_FIELD_TRANS	(1<<3)	/* Field transferred int */
#define VINO_INTSTAT_B_FIFO_OF		(1<<4)	/* FIFO overflow int */
#define VINO_INTSTAT_B_END_DESC_TBL	(1<<5)	/* End of desc table int */

/* Bits in the Clipping register */
#define VINO_CLIP_START(x)		((x) & 0x3ff)		/* bits 0:9 */
#define VINO_CLIP_ODD(x)		(((x) & 0x1ff) << 10)	/* bits 10:18 */
#define VINO_CLIP_EVEN(x)		(((x) & 0x1ff) << 19)	/* bits 19:27 */

/* Bits in the Frame Rate register */
#define VINO_FRAMERT_PAL		(1<<0)			/* 0=NTSC 1=PAL */
#define VINO_FRAMERT_RT(x)		(((x) & 0x1fff) << 1)	/* bits 1:12 */

#endif
