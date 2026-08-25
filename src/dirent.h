#ifndef DIRENT_H
#define DIRENT_H

#include "dsk.h"

#define DECB_TYPE_BASIC 0
#define DECB_TYPE_ML    2

/* Walks the RS-DOS directory. dir_next() writes at most 13 bytes ("NAME.EXT"
   plus NUL) into name, and returns 0 at the end of the directory. */
void dir_rewind(void);
byte dir_next(char *name, byte *type);

#endif /* DIRENT_H */
