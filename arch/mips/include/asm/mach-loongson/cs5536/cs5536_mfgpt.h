/*
 * cs5536 mfgpt header file
 */

#ifndef _CS5536_MFGPT_H
#define _CS5536_MFGPT_H

#include <cs5536/cs5536.h>

extern u32 mfgpt_base;
extern void setup_mfgpt_timer(void);

#if 1
#define MFGPT_TICK_RATE 14318000
#else
#define MFGPT_TICK_RATE (14318180 / 8)
#endif
#define COMPARE  ((MFGPT_TICK_RATE + HZ/2) / HZ)

#define	CS5536_MFGPT_INTR	5

#define MFGPT_BASE	mfgpt_base
#define MFGPT0_CMP2	(MFGPT_BASE + 2)
#define MFGPT0_CNT	(MFGPT_BASE + 4)
#define MFGPT0_SETUP	(MFGPT_BASE + 6)

#endif /*!_CS5536_MFGPT_H */
