/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */
#ifndef _ASM_SN_SN1_BDRKHSPECREGS_H
#define _ASM_SN_SN1_BDRKHSPECREGS_H


/************************************************************************
 *                                                                      *
 *      WARNING!!!  WARNING!!!  WARNING!!!  WARNING!!!  WARNING!!!      *
 *                                                                      *
 * This file is created by an automated script. Any (minimal) changes   *
 * made manually to this  file should be made with care.                *
 *                                                                      *
 *               MAKE ALL ADDITIONS TO THE END OF THIS FILE             *
 *                                                                      *
 ************************************************************************/


#define    HSPEC_MEM_DIMM_INIT_0     0x00000000    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_MEM_DIMM_INIT_1     0x00000008    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_MEM_DIMM_INIT_2     0x00000010    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_MEM_DIMM_INIT_3     0x00000018    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_MEM_DIMM_INIT_4     0x00000020    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_MEM_DIMM_INIT_5     0x00000028    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_MEM_DIMM_INIT_6     0x00000030    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_MEM_DIMM_INIT_7     0x00000038    /*
                                                    * Memory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_0     0x00000040    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_1     0x00000048    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_2     0x00000050    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_3     0x00000058    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_4     0x00000060    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_5     0x00000068    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_6     0x00000070    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_DIR_DIMM_INIT_7     0x00000078    /*
                                                    * Directory DIMM mode
                                                    * initialization
                                                    */



#define    HSPEC_UART_0              0x00000080    /* UART Registers         */



#define    HSPEC_UART_1              0x00000088    /* UART Registers         */



#define    HSPEC_UART_2              0x00000090    /* UART Registers         */



#define    HSPEC_UART_3              0x00000098    /* UART Registers         */



#define    HSPEC_UART_4              0x000000A0    /* UART Registers         */



#define    HSPEC_UART_5              0x000000A8    /* UART Registers         */



#define    HSPEC_UART_6              0x000000B0    /* UART Registers         */



#define    HSPEC_UART_7              0x000000B8    /* UART Registers         */



#define    HSPEC_LED0                0x000000C0    /* Write 8-bit LED        */



#define    HSPEC_LED1                0x000000C8    /* Write 8-bit LED        */



#define    HSPEC_LED2                0x000000D0    /* Write 8-bit LED        */



#define    HSPEC_LED3                0x000000D8    /* Write 8-bit LED        */



#define    HSPEC_SYNERGY0_0          0x04000000    /* Synergy0 Registers     */



#define    HSPEC_SYNERGY0_2097151    0x04FFFFF8    /* Synergy0 Registers     */



#define    HSPEC_SYNERGY1_0          0x05000000    /* Synergy1 Registers     */



#define    HSPEC_SYNERGY1_2097151    0x05FFFFF8    /* Synergy1 Registers     */





#ifdef _LANGUAGE_C

/************************************************************************
 *                                                                      *
 *  Writes to these 8 consecutive registers will perform mode           *
 * register writes to the memory SDRAMs on the DIMMs. There is one      *
 * initialization register for each of the 8 physical banks of          *
 * memory. The value in the DIMM_MODE field is written to the           *
 * designated DIMM after the hardware does a precharge operation and    *
 * enforces the minimum SDRAM timing..                                  *
 *                                                                      *
 ************************************************************************/




typedef union hspec_mem_dimm_init_0_u {
	bdrkreg_t	hspec_mem_dimm_init_0_regval;
	struct  {
		bdrkreg_t	mdi0_reserved             :	52;
		bdrkreg_t	mdi0_dimm_mode            :	12;
	} hspec_mem_dimm_init_0_fld_s;
} hspec_mem_dimm_init_0_u_t;




/************************************************************************
 *                                                                      *
 *  Writes to these 8 consecutive registers will perform mode           *
 * register writes to the memory SDRAMs on the DIMMs. There is one      *
 * initialization register for each of the 8 physical banks of          *
 * memory. The value in the DIMM_MODE field is written to the           *
 * designated DIMM after the hardware does a precharge operation and    *
 * enforces the minimum SDRAM timing..                                  *
 *                                                                      *
 ************************************************************************/




typedef union hspec_mem_dimm_init_7_u {
	bdrkreg_t	hspec_mem_dimm_init_7_regval;
	struct  {
		bdrkreg_t	mdi7_reserved             :	52;
		bdrkreg_t	mdi7_dimm_mode            :	12;
	} hspec_mem_dimm_init_7_fld_s;
} hspec_mem_dimm_init_7_u_t;




/************************************************************************
 *                                                                      *
 *  Writes to these 8 consecutive registers will perform mode           *
 * register writes to the directory SDRAMs on the DIMMs. There is one   *
 * initialization register for each of the 8 physical banks of          *
 * memory. The value in the DIMM_MODE field is written to the           *
 * designated DIMM after the hardware does a precharge operation and    *
 * enforces the minimum SDRAM timing.                                   *
 *                                                                      *
 ************************************************************************/




typedef union hspec_dir_dimm_init_0_u {
	bdrkreg_t	hspec_dir_dimm_init_0_regval;
	struct  {
		bdrkreg_t	ddi0_reserved             :	52;
		bdrkreg_t	ddi0_dimm_mode            :	12;
	} hspec_dir_dimm_init_0_fld_s;
} hspec_dir_dimm_init_0_u_t;




/************************************************************************
 *                                                                      *
 *  Writes to these 8 consecutive registers will perform mode           *
 * register writes to the directory SDRAMs on the DIMMs. There is one   *
 * initialization register for each of the 8 physical banks of          *
 * memory. The value in the DIMM_MODE field is written to the           *
 * designated DIMM after the hardware does a precharge operation and    *
 * enforces the minimum SDRAM timing.                                   *
 *                                                                      *
 ************************************************************************/




typedef union hspec_dir_dimm_init_7_u {
	bdrkreg_t	hspec_dir_dimm_init_7_regval;
	struct  {
		bdrkreg_t	ddi7_reserved             :	52;
		bdrkreg_t	ddi7_dimm_mode            :	12;
	} hspec_dir_dimm_init_7_fld_s;
} hspec_dir_dimm_init_7_u_t;




/************************************************************************
 *                                                                      *
 *  These addresses provide read/write access to up to eight UART       *
 * registers. The datapath to the UART is eight bits wide.              *
 *                                                                      *
 ************************************************************************/




typedef union hspec_uart_0_u {
	bdrkreg_t	hspec_uart_0_regval;
	struct  {
		bdrkreg_t	u0_reserved               :	55;
		bdrkreg_t	u0_unused                 :	 1;
		bdrkreg_t	u0_data                   :	 8;
	} hspec_uart_0_fld_s;
} hspec_uart_0_u_t;




/************************************************************************
 *                                                                      *
 *  These addresses provide read/write access to up to eight UART       *
 * registers. The datapath to the UART is eight bits wide.              *
 *                                                                      *
 ************************************************************************/




typedef union hspec_uart_7_u {
	bdrkreg_t	hspec_uart_7_regval;
	struct  {
		bdrkreg_t	u7_reserved               :	55;
		bdrkreg_t	u7_unused                 :	 1;
		bdrkreg_t	u7_data                   :	 8;
	} hspec_uart_7_fld_s;
} hspec_uart_7_u_t;




/************************************************************************
 *                                                                      *
 *  This register sets the value on the first 8-bit bank of LEDs.       *
 *                                                                      *
 ************************************************************************/




typedef union hspec_led0_u {
	bdrkreg_t	hspec_led0_regval;
	struct  {
		bdrkreg_t	l_reserved                :	55;
		bdrkreg_t	l_unused                  :	 1;
		bdrkreg_t	l_data                    :	 8;
	} hspec_led0_fld_s;
} hspec_led0_u_t;




/************************************************************************
 *                                                                      *
 *  This register sets the value on the second 8-bit bank of LEDs.      *
 *                                                                      *
 ************************************************************************/




typedef union hspec_led1_u {
	bdrkreg_t	hspec_led1_regval;
	struct  {
		bdrkreg_t	l_reserved                :	55;
		bdrkreg_t	l_unused                  :	 1;
		bdrkreg_t	l_data                    :	 8;
	} hspec_led1_fld_s;
} hspec_led1_u_t;




/************************************************************************
 *                                                                      *
 *  This register sets the value on the third 8-bit bank of LEDs.       *
 *                                                                      *
 ************************************************************************/




typedef union hspec_led2_u {
	bdrkreg_t	hspec_led2_regval;
	struct  {
		bdrkreg_t	l_reserved                :	55;
		bdrkreg_t	l_unused                  :	 1;
		bdrkreg_t	l_data                    :	 8;
	} hspec_led2_fld_s;
} hspec_led2_u_t;




/************************************************************************
 *                                                                      *
 *  This register sets the value on the fourth 8-bit bank of LEDs.      *
 *                                                                      *
 ************************************************************************/




typedef union hspec_led3_u {
	bdrkreg_t	hspec_led3_regval;
	struct  {
		bdrkreg_t	l_reserved                :	55;
		bdrkreg_t	l_unused                  :	 1;
		bdrkreg_t	l_data                    :	 8;
	} hspec_led3_fld_s;
} hspec_led3_u_t;




/************************************************************************
 *                                                                      *
 *  This range allows access to the first Synergy chip.                 *
 *                                                                      *
 ************************************************************************/




typedef union hspec_synergy0_0_u {
	bdrkreg_t	hspec_synergy0_0_regval;
	struct  {
		bdrkreg_t	s0_data                   :	64;
	} hspec_synergy0_0_fld_s;
} hspec_synergy0_0_u_t;




/************************************************************************
 *                                                                      *
 *  This range allows access to the first Synergy chip.                 *
 *                                                                      *
 ************************************************************************/




typedef union hspec_synergy0_2097151_u {
	bdrkreg_t	hspec_synergy0_2097151_regval;
	struct  {
		bdrkreg_t	s2_data                   :	64;
	} hspec_synergy0_2097151_fld_s;
} hspec_synergy0_2097151_u_t;




/************************************************************************
 *                                                                      *
 *  This range allows access to the second Synergy chip.                *
 *                                                                      *
 ************************************************************************/




typedef union hspec_synergy1_0_u {
	bdrkreg_t	hspec_synergy1_0_regval;
	struct  {
		bdrkreg_t	s0_data                   :	64;
	} hspec_synergy1_0_fld_s;
} hspec_synergy1_0_u_t;




/************************************************************************
 *                                                                      *
 *  This range allows access to the second Synergy chip.                *
 *                                                                      *
 ************************************************************************/




typedef union hspec_synergy1_2097151_u {
	bdrkreg_t	hspec_synergy1_2097151_regval;
	struct  {
		bdrkreg_t	s2_data                   :	64;
	} hspec_synergy1_2097151_fld_s;
} hspec_synergy1_2097151_u_t;






#endif /* _LANGUAGE_C */

/************************************************************************
 *                                                                      *
 * The following defines which were not formed into structures are      *
 * probably indentical to another register, and the name of the         *
 * register is provided against each of these registers. This           *
 * information needs to be checked carefully                            *
 *                                                                      *
 *           HSPEC_MEM_DIMM_INIT_1     HSPEC_MEM_DIMM_INIT_0            *
 *           HSPEC_MEM_DIMM_INIT_2     HSPEC_MEM_DIMM_INIT_0            *
 *           HSPEC_MEM_DIMM_INIT_3     HSPEC_MEM_DIMM_INIT_0            *
 *           HSPEC_MEM_DIMM_INIT_4     HSPEC_MEM_DIMM_INIT_0            *
 *           HSPEC_MEM_DIMM_INIT_5     HSPEC_MEM_DIMM_INIT_0            *
 *           HSPEC_MEM_DIMM_INIT_6     HSPEC_MEM_DIMM_INIT_0            *
 *           HSPEC_DIR_DIMM_INIT_1     HSPEC_DIR_DIMM_INIT_0            *
 *           HSPEC_DIR_DIMM_INIT_2     HSPEC_DIR_DIMM_INIT_0            *
 *           HSPEC_DIR_DIMM_INIT_3     HSPEC_DIR_DIMM_INIT_0            *
 *           HSPEC_DIR_DIMM_INIT_4     HSPEC_DIR_DIMM_INIT_0            *
 *           HSPEC_DIR_DIMM_INIT_5     HSPEC_DIR_DIMM_INIT_0            *
 *           HSPEC_DIR_DIMM_INIT_6     HSPEC_DIR_DIMM_INIT_0            *
 *           HSPEC_UART_1              HSPEC_UART_0                     *
 *           HSPEC_UART_2              HSPEC_UART_0                     *
 *           HSPEC_UART_3              HSPEC_UART_0                     *
 *           HSPEC_UART_4              HSPEC_UART_0                     *
 *           HSPEC_UART_5              HSPEC_UART_0                     *
 *           HSPEC_UART_6              HSPEC_UART_0                     *
 *                                                                      *
 ************************************************************************/


/************************************************************************
 *                                                                      *
 *               MAKE ALL ADDITIONS AFTER THIS LINE                     *
 *                                                                      *
 ************************************************************************/





#endif /* _ASM_SN_SN1_BDRKHSPECREGS_H */
