/* $Id$
 *
 * indy_pbus.h: Routines for manipulation of the HPC PBUS DMA controller.
 *
 * Copyright (C) 1998 Ulf Carlsson
 */

#define MAX_PBUS_CHANNELS	8

int sgipbus_get_list(char *buf);
int sgipbus_request(unsigned int channel, char *device_id);
int sgipbus_setup(unsigned int channel,
		  int fifo_size,		/* doublewords (64 bit) */
		  unsigned long highwater,	/* halfwords (16 bit) */
		  unsigned long flags,
		  unsigned long dmacfg);
void sgipbus_free(unsigned int channel);

/* enable/disable a specific PBUS dma channel */
__inline__ void sgipbus_enable(unsigned int channel, unsigned long desc);
__inline__ void sgipbus_disable(unsigned int channel);
/* true if interrupted */
__inline__ int sgipbus_interrupted(unsigned int channel);
