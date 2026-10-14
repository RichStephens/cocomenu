/* Sector reads through Disk Basic's DSKCON vector - the path HDB-DOS/DriveWire
   hooks. Calling it directly keeps cmoc's monolithic 1804-byte disk.o unlinked. */

#include "dsk.h"

#define DCOPC (*(byte *) 0x00EA)   /* operation: 2 = read */
#define DCDRV (*(byte *) 0x00EB)
#define DCTRK (*(byte *) 0x00EC)
#define DCSEC (*(byte *) 0x00ED)
#define DCBPT (*(byte **) 0x00EE)
#define DCSTA (*(byte *) 0x00F0)

#define DEFDRV (*(byte *) 0x095A)
#define MAXDRV (*(byte *) 0x0150)   /* HDB-DOS only */

#define DSKCON_VEC (*(byte **) 0xC004)
#define JMP_OP     0x7E

#define DSKCON_READ 2

byte dsk_drive;
byte dsk_count;
byte dsk_buf[SECTOR_SIZE];

/* HDB-DOS's DRIVE n: LOADM and RUN take their drive from DEFDRV. */
void dsk_select(byte drive)
{
  dsk_drive = drive;
  DEFDRV = drive;
}

void dsk_init(void)
{
  byte start, saved;

  start = DCDRV;
  dsk_drive = start;
  dsk_count = start + 1;

  /* HDB-DOS reroutes DSKCON through a JMP; stock Disk Basic would probe real
     floppy drives. The first disk whose title sector fails ends the image. */
  if (*DSKCON_VEC == JMP_OP) {
    while (dsk_count <= MAXDRV && dsk_count != 255) {
      dsk_drive = dsk_count;
      if (!dsk_read(dsk_buf, DIR_TRACK, TITLE_SECTOR))
        break;
      dsk_count++;
    }
  }

  saved = SAVED_DISK[0];
  if (SAVED_DISK[1] == (byte) ~saved && saved < dsk_count)
    start = saved;
  SAVED_DISK[0] = SAVED_DISK[1] = 0;

  dsk_select(start);
}

byte dsk_read(byte *dst, byte track, byte sector)
{
  DCOPC = DSKCON_READ;
  DCDRV = dsk_drive;
  DCTRK = track;
  DCSEC = sector;
  DCBPT = dst;

  /* DSKCON is ROM, not cmoc code, so it may clobber the frame pointer. */
  asm
  {
    pshs u,y
    jsr  [$C004]
    puls y,u
  }

  return DCSTA == 0;
}
