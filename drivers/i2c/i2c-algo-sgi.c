/*
 * i2c-algo-sgi.c: i2c driver algorithms for SGI adapters.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * Copyright (C) 2003 Ladislav Michl <ladis@linux-mips.org>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-sgi.h>


#define SGI_I2C_FORCE_IDLE	(0 << 0)
#define SGI_I2C_NOT_IDLE	(1 << 0)
#define SGI_I2C_WRITE		(0 << 1)
#define SGI_I2C_READ		(1 << 1)
#define SGI_I2C_RELEASE_BUS	(0 << 2)
#define SGI_I2C_HOLD_BUS	(1 << 2)
#define SGI_I2C_XFER_DONE	(0 << 4)
#define SGI_I2C_XFER_BUSY	(1 << 4)
#define SGI_I2C_ACK		(0 << 5)
#define SGI_I2C_NACK		(1 << 5)
#define SGI_I2C_BUS_OK		(0 << 7)
#define SGI_I2C_BUS_ERR		(1 << 7)

#define get_control(adap)	adap->getctrl(adap->data)
#define set_control(adap, val)	adap->setctrl(adap->data, val)
#define read_data(adap)		adap->rdata(adap->data)
#define write_data(adap, val)	adap->wdata(adap->data, val)

/* I2C debugging? */
#if 1
#define DEBUGI2C(x...)     printk(x);
#else
#define DEBUGI2C(x...)
#endif

static int wait_xfer_done(struct i2c_algo_sgi_data *adap)
{
	int i;

	for (i = 0; i < adap->xfer_timeout; i++) {
		if ((get_control(adap) & SGI_I2C_XFER_BUSY) == 0)
			return 0;
		udelay(1);
	}

	DEBUGI2C("sgi_i2c: timeout waiting for xfer done.\n");

	return -ETIMEDOUT;
}

static int wait_ack(struct i2c_algo_sgi_data *adap)
{
	int i;

	if (wait_xfer_done(adap))
		return -ETIMEDOUT;
	for (i = 0; i < adap->ack_timeout; i++) {
		if ((get_control(adap) & SGI_I2C_NACK) == 0)
			return 0;
		udelay(1);
	}

	DEBUGI2C("sgi_i2c: timeout waiting for ack.\n");

	return -ETIMEDOUT;
}

static int force_idle(struct i2c_algo_sgi_data *adap)
{
	int i;

	set_control(adap, SGI_I2C_FORCE_IDLE);
	for (i = 0; i < adap->xfer_timeout; i++) {
		if ((get_control(adap) & SGI_I2C_NOT_IDLE) == 0)
			goto out;
		udelay(1);
	}
	return -ETIMEDOUT;
out:
	if (get_control(adap) & SGI_I2C_BUS_ERR) {
		return -EIO;
	}
	return 0;
}

static int do_address(struct i2c_algo_sgi_data *adap, unsigned int addr,
		      int rd, int release)
{
	DEBUGI2C("preparing to %s %02x.\n", rd ? "read from" : "write to",
		 addr);
	/* Check if bus is idle, eventually force it to do so */
	if ((get_control(adap) & SGI_I2C_NOT_IDLE) == 0)
		if (force_idle(adap))
	                return -EIO;
	/* Write out the i2c chip address and specify operation */
	set_control(adap, (release ? SGI_I2C_RELEASE_BUS : SGI_I2C_HOLD_BUS) |
			  SGI_I2C_WRITE | SGI_I2C_NOT_IDLE);
	write_data(adap, addr | (rd ? 1 : 0));
	if (wait_ack(adap))
		return -EIO;
	return 0;
}

static int i2c_read(struct i2c_algo_sgi_data *adap, unsigned char *buf,
		    unsigned int len, int last)
{
	int i;

	DEBUGI2C("i2c_read\n");
	set_control(adap, (last && len < 2 ?
			  SGI_I2C_RELEASE_BUS : SGI_I2C_HOLD_BUS) |
			  SGI_I2C_READ | SGI_I2C_NOT_IDLE);
	for (i = 0; i < len; i++) {
		if (last && len == i + 1)
			set_control(adap, SGI_I2C_RELEASE_BUS |
					  SGI_I2C_READ |
					  SGI_I2C_NOT_IDLE);
		if (wait_xfer_done(adap))
			return -EIO;
		buf[i] = read_data(adap);
	}
	return 0;

}

static int i2c_write(struct i2c_algo_sgi_data *adap, unsigned char *buf,
		     unsigned int len, int last)
{
	int i;

	DEBUGI2C("i2c_write\n");
	/* We are already in write state */
	for (i = 0; i < len; i++) {
		if (last && len == i + 1)
			set_control(adap, SGI_I2C_RELEASE_BUS |
					  SGI_I2C_WRITE |
					  SGI_I2C_NOT_IDLE);
		DEBUGI2C("writing %02x\n", buf[i]);
		write_data(adap, buf[i]);
		if (wait_ack(adap))
			return -EIO;
	}
	return 0;
}

static int sgi_xfer(struct i2c_adapter *i2c_adap, struct i2c_msg msgs[],
		    int num)
{
	struct i2c_algo_sgi_data *adap = i2c_adap->algo_data;
	struct i2c_msg *p;
	int i, err = 0;
  
	for (i = 0; !err && i < num; i++) {
		p = &msgs[i];
		err = do_address(adap, p->addr, p->flags & I2C_M_RD,
				 p->len == 0);
		if (err || p->len == 0)
			continue;
		if (p->flags & I2C_M_RD)
			err = i2c_read(adap, p->buf, p->len, num == i + 1);
		else
			err = i2c_write(adap, p->buf, p->len, num == i + 1);
	}

	return err;
}

static u32 sgi_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm sgi_algo = {
	.name		= "SGI algorithm",
	.id		= I2C_ALGO_SGI,
	.master_xfer	= sgi_xfer,
	.functionality	= sgi_func,
};

/* 
 * registering functions to load algorithms at runtime 
 */
int i2c_sgi_add_bus(struct i2c_adapter *adap)
{
	adap->id |= sgi_algo.id;
	adap->algo = &sgi_algo;

	return i2c_add_adapter(adap);
}


int i2c_sgi_del_bus(struct i2c_adapter *adap)
{
	return i2c_del_adapter(adap);
}

EXPORT_SYMBOL(i2c_sgi_add_bus);
EXPORT_SYMBOL(i2c_sgi_del_bus);

MODULE_AUTHOR("Ladislav Michl <ladis@linux-mips.org>");
MODULE_DESCRIPTION("I2C-Bus SGI algorithm");
MODULE_LICENSE("GPL");
