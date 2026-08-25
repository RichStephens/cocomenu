/* Sector reads through Disk Basic's DSKCON vector - the path HDB-DOS/DriveWire
   hooks. Calling it directly keeps cmoc's monolithic 1804-byte disk.o unlinked. */

#include "dsk.h"

#define DCOPC (*(byte *) 0x00EA)   /* operation: 2 = read */
#define DCDRV (*(byte *) 0x00EB)
#define DCTRK (*(byte *) 0x00EC)
#define DCSEC (*(byte *) 0x00ED)
#define DCBPT (*(byte **) 0x00EE)
#define DCSTA (*(byte *) 0x00F0)

#define DSKCON_READ 2

byte dsk_drive;
byte dsk_buf[SECTOR_SIZE];

void dsk_init(void)
{
  dsk_drive = DCDRV;
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
