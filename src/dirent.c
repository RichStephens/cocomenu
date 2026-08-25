/* RS-DOS directory reader. Derived from fujinet-tests/localcmd/src/coco, with
   its sector reads redirected through dsk_read().
 *
 * Track 17, sectors 3-18, eight 32-byte entries per sector:
 *   0-7   filename, space padded      11    file type (0 BASIC, 2 machine code)
 *   8-10  extension, space padded     12    ASCII flag
 * A first byte of $00 marks an erased entry, $FF the end of the directory. */

#include "dirent.h"

#define FIRST_DIR_SECTOR  3
#define LAST_DIR_SECTOR  18
#define ENTRIES_PER_SEC   8
#define ENTRY_SHIFT       5    /* 32 bytes per entry */
#define EXT_OFFSET        8
#define TYPE_OFFSET      11

#define END_OF_DIR   0xFF
#define ERASED_ENTRY 0x00

static byte sector;
static byte index;

void dir_rewind(void)
{
  sector = FIRST_DIR_SECTOR;
  index = ENTRIES_PER_SEC;   /* forces a read on the first call */
}

/* Turns the padded 8+3 name into "NAME.EXT". */
static void denormalize(char *dest, const char *src)
{
  signed char i, end;

  end = 7;
  while (end >= 0 && src[end] == ' ')
    --end;
  for (i = 0; i <= end; ++i)
    *dest++ = src[i];

  src += EXT_OFFSET;
  end = 2;
  while (end >= 0 && src[end] == ' ')
    --end;

  if (end >= 0) {
    *dest++ = '.';
    for (i = 0; i <= end; ++i)
      *dest++ = src[i];
  }

  *dest = '\0';
}

byte dir_next(char *name, byte *type)
{
  byte *e;

  for (;;) {
    if (index >= ENTRIES_PER_SEC) {
      if (sector > LAST_DIR_SECTOR)
        return 0;
      if (!dsk_read(dsk_buf, DIR_TRACK, sector))
        return 0;
      sector++;
      index = 0;
    }

    e = &dsk_buf[(word) index << ENTRY_SHIFT];
    index++;

    if (e[0] == ERASED_ENTRY)
      continue;
    if (e[0] == END_OF_DIR)
      return 0;

    denormalize(name, (char *) e);
    *type = e[TYPE_OFFSET];
    return 1;
  }
}
