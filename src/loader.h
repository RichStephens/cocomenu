#ifndef LOADER_H
#define LOADER_H

#include "dsk.h"

/* Runs the named program. is_basic picks RUN"NAME" over the machine-language
   path. Does not return to the caller either way. */
void run_file(const char *filename, byte is_basic);

#endif /* LOADER_H */
