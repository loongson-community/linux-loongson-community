

/*
 *  linux/arch/mips/dec/reset.c
 *
 *  Reset a DECstation machine.
 *
 *  $Id: reset.c,v 1.3 1998/03/04 08:29:10 ralf Exp $
 */

void dec_machine_restart(char *command)
{
}

void dec_machine_halt(void)
{
}

void dec_machine_power_off(void)
{
    /* DECstations don't have a software power switch */
}
