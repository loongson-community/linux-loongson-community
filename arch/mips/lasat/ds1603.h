/* 
 * Dallas Semiconductors 1603 RTC driver 
 *
 * Brian Murphy <brian@murphy.dk> 
 *
 */
#ifndef __DS1603_H
#define __DS1603_H

unsigned long ds1603_read(void);
void ds1603_set(unsigned long);
void ds1603_set_trimmer(unsigned int);
void ds1603_enable(void);
void ds1603_disable(void);

#define TRIMMER_DEFAULT	3
#define TRIMMER_DISABLE_RTC 0

#endif
