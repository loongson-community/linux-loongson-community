/*
 * board-specific serial port
 */

#include <asm/addrspace.h>

#if defined(CONFIG_LEMOTE_FULONG) || defined(CONFIG_MIPS_MALTA)
#define UART_BASE 0x1fd003f8
#define PORT(offset) (CKSEG1ADDR(UART_BASE) + (offset))
#endif

#ifdef CONFIG_AR7
#include <ar7.h>
#define PORT(offset) (CKSEG1ADDR(AR7_REGS_UART0) + (4 * offset))
#endif

#ifndef PORT
#error please define the serial port address for your own machine
#endif
