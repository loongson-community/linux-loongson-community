/* $Id: sgiwd93.h,v 1.4 1996/07/14 06:43:13 dm Exp $
 * sgiwd93.h: SGI WD93 scsi definitions.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#ifndef _SGIWD93_H
#define _SGIWD93_H

#ifndef NULL
#define NULL        0
#endif

#ifndef CMD_PER_LUN
#define CMD_PER_LUN 8
#endif

#ifndef CAN_QUEUE
#define CAN_QUEUE   16
#endif

int sgiwd93_detect(Scsi_Host_Template *);
const char *wd33c93_info(void);
int wd33c93_queuecommand(Scsi_Cmnd *, void (*done)(Scsi_Cmnd *));
int wd33c93_abort(Scsi_Cmnd *);
int wd33c93_reset(Scsi_Cmnd *, unsigned int);

extern struct proc_dir_entry proc_scsi_sgiwd93;

#define SGIWD93_SCSI {/* next */                NULL,                 \
		      /* usage_count */         NULL,	              \
		      /* proc_dir_entry */      &proc_scsi_sgiwd93,   \
		      /* proc_info */           NULL,                 \
		      /* name */                "SGI WD93",           \
		      /* detect */              sgiwd93_detect,       \
		      /* release */             NULL,                 \
		      /* info */                NULL,	              \
		      /* command */             NULL,                 \
		      /* queuecommand */        wd33c93_queuecommand, \
		      /* abort */               wd33c93_abort,        \
		      /* reset */               wd33c93_reset,        \
		      /* slave_attach */        NULL,                 \
		      /* bios_param */          NULL, 	              \
		      /* can_queue */           CAN_QUEUE,            \
		      /* this_id */             7,                    \
		      /* sg_tablesize */        SG_ALL,               \
		      /* cmd_per_lun */	        CMD_PER_LUN,          \
		      /* present */             0,                    \
		      /* unchecked_isa_dma */   0,                    \
		      /* use_clustering */      DISABLE_CLUSTERING    \
}

#endif /* !(_SGIWD93_H) */
