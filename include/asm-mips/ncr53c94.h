/*
 * Makefile for MIPS Linux main source directory
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */

/*
 * As far as I know there are some MIPS boxes which have their NCRs
 * mapped with pad bytes.  For these NCR_PAD will have to be redefined.
 */
#define NCR_PAD(name)
struct ncr53c94 {
	NCR_PAD(pad0);
	volatile unsigned char count_lo;
	NCR_PAD(pad1);
	volatile unsigned char count_hi;
	NCR_PAD(pad2);
	volatile unsigned char fifo;
	NCR_PAD(pad3);
	volatile unsigned char command;
	NCR_PAD(pad4);
	union {
		volatile unsigned char status;
		volatile unsigned char dest_id;
	} s_d;
	NCR_PAD(pad5);
	union {
		volatile unsigned char interrupts;
		volatile unsigned char timeout;
	} i_t;
	NCR_PAD(pad6);
	union {
		volatile unsigned char seqn_step;
		volatile unsigned char sync_period;
	} s_p;
	NCR_PAD(pad7);
	union {
		volatile unsigned char fifo_flags;
		volatile unsigned char sync_offset;
	} f_o;
	NCR_PAD(pad8);
	volatile unsigned char config1;
	NCR_PAD(pad9);
	volatile unsigned char clk_conv;
	NCR_PAD(pad10);
	volatile unsigned char test;
	NCR_PAD(pad11);
	volatile unsigned char config2;
	NCR_PAD(pad12);
	volatile unsigned char config3;
	NCR_PAD(pad13);
	volatile unsigned char unused;
	NCR_PAD(pad14);
	volatile unsigned char count_xhi;
	NCR_PAD(pad15);
	volatile unsigned char fifo_b;
};

/*
 * Clock conversion factors
 */
#define CCV_10MHZ	0x2		/* 10 Mhz         */
#define CCV_15MHZ	0x3		/* 10.01 - 15 Mhz */
#define CCV_20MHZ	0x4		/* 15.01 - 20 Mhz */
#define CCV_25MHZ	0x5		/* 20.01 - 25 Mhz */
#define CCV_30MHZ	0x6		/* 25.01 - 30 Mhz */
#define CCV_35MHZ	0x7		/* 30.01 - 35 Mhz */
#define CCV_40MHZ	0x0		/* 35.01 - 40 Mhz */

/*
 * Set this additional to enable DMA for a command.
 */
#define NCR_DMA			0x80

/*
 * Miscellaneous commands
 */
#define NCR_NOP			0x00
#define NCR_FLUSH_FIFO		0x01
#define NCR_RESET_NCR		0x02
#define NCR_RESET_SCSI		0x03

/*
 * Disconnected state commands
 */
#define NCR_SELECT_NO_ATN	0x41
#define NCR_SELECT_ATN		0x42
#define NCR_SELECT_ATN_STOP	0x43
#define NCR_ENABLE_RESEL	0x44
#define NCR_DISABLE_RESEL	0x45
#define NCR_SELECT_ATN3		0x46

/*
 * Initiator state commands
 */
#define NCR_TRANSFER		0x10
#define NCR_CMD_CMP		0x11
#define NCR_MSG_OK		0x12
#define NCR_TRANSFER_PAD	0x18
#define NCR_SET_ATN		0x1a
#define NCR_RESET_ATN		0x1b

/*
 * Target state commands
 */
#define NCR_SEND_MSG		0x20
#define NCR_SEND_STATUS		0x21
#define NCR_SEND_DATA		0x22
#define NCR_DISC_SEQN		0x23
#define NCR_TERM_SEQN		0x24
#define NCR_CMD_COMP_SEQN	0x25
#define NCR_DISC		0x27
#define NCR_REC_MSG		0x28
#define NCR_REC_CMD		0x29
#define NCR_REC_DATA		0x2a
#define NCR_REC_CMD_SEQN	0x2b
#define NCR_ABORT_DMA		0x04
