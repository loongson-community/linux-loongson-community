/* jazz_esp.h: Defines and structures for the JAZZ SCSI driver.
 *
 * Copyright (C) 1997 Thomas Bogendoerfer (tsbogend@alpha.franken.de)
 */

#ifndef JAZZ_ESP_H
#define JAZZ_ESP_H

#define EREGS_PAD(n)

#include "esp.h"


extern int jazz_esp_detect(struct SHT *);
extern const char *esp_info(struct Scsi_Host *);
extern int esp_queue(Scsi_Cmnd *, void (*done)(Scsi_Cmnd *));
extern int esp_command(Scsi_Cmnd *);
extern int esp_abort(Scsi_Cmnd *);
extern int esp_reset(Scsi_Cmnd *, unsigned int);
extern int esp_proc_info(char *buffer, char **start, off_t offset, int length,
			 int hostno, int inout);


#define SCSI_JAZZ_ESP {                                                                \
/* struct SHT *next */                                         NULL,                   \
/* long *usage_count */                                        NULL,                   \
/* struct proc_dir_entry *proc_dir */                          &proc_scsi_esp,         \
/* int (*proc_info)(char *, char **, off_t, int, int, int) */  &esp_proc_info,         \
/* const char *name */                                         "JAZZ SCSI",            \
/* int detect(struct SHT *) */                                 jazz_esp_detect,        \
/* int release(struct Scsi_Host *) */                          NULL,                   \
/* const char *info(struct Scsi_Host *) */                     esp_info,               \
/* int command(Scsi_Cmnd *) */                                 esp_command,            \
/* int queuecommand(Scsi_Cmnd *, void (*done)(Scsi_Cmnd *)) */ esp_queue,              \
/* int abort(Scsi_Cmnd *) */                                   esp_abort,              \
/* int reset(Scsi_Cmnd *) */                                   esp_reset,              \
/* int slave_attach(int, int) */                               NULL,                   \
/* int bios_param(Disk *, kdev_t, int[]) */                    NULL,                   \
/* int can_queue */                                            7,                      \
/* int this_id */                                              7,                      \
/* short unsigned int sg_tablesize */                          SG_ALL,                 \
/* short cmd_per_lun */                                        1,                      \
/* unsigned char present */                                    0,                      \
/* unsigned unchecked_isa_dma:1 */                             0,                      \
/* unsigned use_clustering:1 */                                DISABLE_CLUSTERING, }

#endif /* JAZZ_ESP_H */

