#ifndef DSK_H
#define DSK_H

#include <coco.h>

#define DIR_TRACK    17
#define TITLE_SECTOR 17   /* HDB-DOS disk title, NUL terminated */
#define SECTOR_SIZE  256

/* Disk Basic's random file buffer. A cold start leaves it alone, so the
   launch stub parks the current disk here as (disk, ~disk). */
#define SAVED_DISK_ADDR 0x0989
#define SAVED_DISK      ((byte *) SAVED_DISK_ADDR)

/* Drive DSKCON uses, and under HDB-DOS the virtual disk within the image. */
extern byte dsk_drive;

/* The arrows reach disks 0 to dsk_count - 1. */
extern byte dsk_count;

extern byte dsk_buf[SECTOR_SIZE];

void dsk_init(void);
void dsk_select(byte drive);
byte dsk_read(byte *dst, byte track, byte sector);

#endif /* DSK_H */
