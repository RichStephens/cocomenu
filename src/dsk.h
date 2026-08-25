#ifndef DSK_H
#define DSK_H

#include <coco.h>

#define DIR_TRACK   17
#define SECTOR_SIZE 256

/* Drive DSKCON last used: the one we were loaded from. */
extern byte dsk_drive;

extern byte dsk_buf[SECTOR_SIZE];

void dsk_init(void);
byte dsk_read(byte *dst, byte track, byte sector);

#endif /* DSK_H */
