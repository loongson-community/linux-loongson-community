
/*
 * Low level serial I/O routines.
 */
#include <asm/asm.h>
#include <linux/errno.h>
#include <asm/current.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/isadep.h>
#include <asm/unistd.h>

#define SERIAL_BASE             0xbd000020
#define SERB_DLABLO             0
#define SERB_DLABHI             1
#define SERB_FIFO               2
#define SERB_DATA               0
#define SERB_LCR                3
#define SERB_MCR                4
#define SERB_CMD                5

#define SERA_DLABLO             0x08
#define SERA_DLABHI             0x09
#define SERA_DATA               0x08
#define SERA_LCR                0x0B
#define SERA_MCR                0x0C
#define SERA_CMD                0x0D

#define TX_BUSY			0x20

/*
 * To calculate divisor for different baud rates:
 * divisor = 3686400/(baud rate x 16), where 3686400 is our
 * 3.6864MHz clock input.
 */

/*
 * routine to initialize the serial DUART.  Channel A only
 */
	.set	noreorder
 	.global	init_serial
	.ent	init_serial
init_serial:
        li      a0,SERIAL_BASE

#if 0
	# init channel A
        li      t0,0x0083
        sb      t0,SERA_LCR(a0)           # set DLAB

#        li      t0,24                    # 9600 baud
		li      t0,2                      # 115200 baud
        sb      t0,SERA_DLABLO(a0)        # set divisor

        li      t0,0
        sb      t0,SERA_DLABHI(a0)

        li      t0,0x0003                 # 8 data bits
        sb      t0,SERA_LCR(a0)
#endif

#if 1
	# init channel B
        li      t0,0x0083
        sb      t0,SERB_LCR(a0)           # set DLAB

        li      t0,24
        sb      t0,SERB_DLABLO(a0)        # set divisor for 115200 baud

        li      t0,0
        sb      t0,SERB_DLABHI(a0)

        li      t0,0x0003                 # 8 data bits
        sb      t0,SERB_LCR(a0)

        li      t0,0                      # no fifo
        sb      t0,SERB_FIFO(a0)
#endif

        li a0, 0xB4000000
        li t0, 0x002fffff
        sw t0, 0x464(a0)
        

        jr      ra
	nop

  	.size	init_serial, . - init_serial;
  	.end	init_serial

	.set	reorder
