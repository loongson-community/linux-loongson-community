/*
 *  EVB96100  -Galileo reset subroutines
 *
 */
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/reboot.h>
#include <asm/system.h>

/********************************************************************
 *galileo_machine_restart -
 *
 *Restart the machine
 *
 *
 *Inputs :
 *command - not used
 *
 *Outpus :
 *
 *********************************************************************/
void galileo_machine_restart(char *command)
{
	*(volatile char *) 0xbc000000 = 0x0f;
	/*
	 * Ouch, we're still alive ... This time we take the silver bullet ...
	 * ... and find that we leave the hardware in a state in which the
	 * kernel in the flush locks up somewhen during of after the PCI
	 * detection stuff.
	 */
	set_cp0_status((ST0_BEV | ST0_ERL), (ST0_BEV | ST0_ERL));
	set_cp0_config(CONF_CM_CMASK, CONF_CM_UNCACHED);
	flush_cache_all();
	write_32bit_cp0_register(CP0_WIRED, 0);
	__asm__ __volatile__("jr\t%0"::"r"(0xbfc00000));
}

/********************************************************************
 *galileo_machine_halt -
 *
 *Halt the machine
 *
 *
 *Inputs :
 *
 *Outpus :
 *
 *********************************************************************/
void galileo_machine_halt(void)
{
	printk("\n** You can safely turn off the power\n");
	while (1) {
	}
}

/********************************************************************
 *galileo_machine_power_off -
 *
 *Halt the machine
 *
 *
 *Inputs :
 *
 *Outpus :
 *
 *********************************************************************/
void galileo_machine_power_off(void)
{
	galileo_machine_halt();
}
