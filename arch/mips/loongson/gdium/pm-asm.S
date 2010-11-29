/*
 * Suspend support specific Gdium
 *
 * Copyright (c) 2009 Arnaud Patard <apatard@mandriva.com>
 */
#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/asm-offsets.h>

	.set noreorder

#define DELAY_VALUE	5

#define DELAY(T)\
	li		t0,0x10;	\
	li		t1,T;		\
	sll		t0,t0,t1;	\
10:	nop;				\
	subu		t0,t0,1;	\
	bnez		t0,10b;		\
	nop;

#define GET_SDA				\
	lw		v0,0(a0);	\
	srl		v0,v0,13;	\
	andi		v0,v0,1;

#define GET_SCL				\
	lw		v0,0(a0);	\
	srl		v0,v0,6;	\
	andi		v0,v0,1;

/* set sda input = set sda to 1 */
#define SET_SDA 			\
	lw		t0,0(a1);	\
	lui		t1,0xffff;	\
	ori		t1,t1,0xdfff;	\
	and		t0,t0,t1;	\
	sw		t0,0(a1);

	/* set sda output = set sda to 0 */
#define CLEAR_SDA 			\
	lw		t0,0(a0);	\
	lui		t1,0xffff;	\
	ori		t1,t1,0xdfff;	\
	and		t0,t0,t1;	\
	sw		t0,0(a0);	\
	lw		t0,0(a1);	\
	ori		t0,t0,0x2000;	\
	sw		t0,0(a1);

#define SDA_LOW				\
	CLEAR_SDA			\
	DELAY((DELAY_VALUE+1)/2)

#define SDA_HI				\
	SET_SDA				\
	DELAY((DELAY_VALUE+1)/2)

	/* set scl output = set scl to 0 */
#define CLEAR_SCL			\
	lw		t0,0(a0);	\
	lui		t1,0xffff;	\
	ori		t1,t1,0xffbf;	\
	and		t0,t0,t1;	\
	sw		t0,0(a0);	\
	lw		t0,0(a1);	\
	ori		t0,t0,0x40;	\
	sw		t0,0(a1);

	/* set scl input = set scl to 1 */
#define SCL_HI				\
	lw		t0,0(a1);	\
	lui		t1,0xffff;	\
	ori		t1,t1,0xffbf;	\
	and		t0,t0,t1;	\
	sw		t0,0(a1);	\
9:					\
	GET_SCL				\
	beqz		v0,9b;		\
	nop;				\
	DELAY(DELAY_VALUE)


#define SCL_LOW				\
	CLEAR_SCL			\
	DELAY(DELAY_VALUE/2)

/* inspired from i2c-algo-bit.c */
#define I2C_START 			\
	CLEAR_SDA			\
	DELAY(DELAY_VALUE)		\
	SCL_LOW

#define I2C_STOP 			\
	SDA_LOW				\
	SCL_HI	 			\
	SET_SDA 			\
	DELAY(DELAY_VALUE)

/* reg != than t4,t0,t1,t2 */
/* return ack value in v0 */
#define I2C_OUTB(reg)			\
	li		s0,7;		\
1:					\
	bltz		s0,4f;		\
	srlv		t2,reg,s0;	\
	andi		t0,t2,1;	\
	bnez		t0,2f;		\
	nop;				\
	CLEAR_SDA			\
	b		3f;		\
	nop;				\
2:	SET_SDA				\
3:	DELAY((DELAY_VALUE+1)/2)	\
	SCL_HI				\
	SCL_LOW				\
	addiu		s0,s0,-1;	\
	b		1b;		\
	nop;				\
4:	SDA_HI				\
	SCL_HI				\
5:					\
	GET_SDA				\
	li		t0,1;		\
	subu		v0,t0,v0;	\
	beqz		v0, 5b;		\
	nop;				\
	SCL_LOW

/* t0 contains 1 for read */
/* 0 otherwise */
#define DOADDRESS(addr)			\
	sll		t3,addr,1;	\
	or		t3,t3,t0;	\
	I2C_OUTB(t3)

LEAF(ls2f_board_standby)
	/* a1 : dir low */
	/* a0 : data low */
	ld              a0, sm50x_base_reg
	li		t0,0x010000
	LONG_ADDU	a0,a0,t0
	LONG_ADDIU	a1,a0,8
	LONG_ADDIU	a0,a0,0
	/*
	 * FIXME : configure sm501 sda/scl pin as gpio
	 * with right level to avoid troubles with
	 * booting say  with init=/bin/bash
	 */

	I2C_START
	/* write 0x5a in the st7 "sign" register */
	li		v1,0x40
	li		t0,0
	DOADDRESS(v1)
	li		v1,5
	I2C_OUTB(v1)
	li		v1,0x5a
	I2C_OUTB(v1)
	I2C_STOP

#if 0
        lui		t0, 0xbfc0
	ori		t0,t0,0x073c
	jr		t0
	nop
#else
	.long		0x3c0cbfc0
	.long		0x358c073c
	.long		0x01800008
	.long		0x00000000
#endif
END(ls2f_board_standby)

