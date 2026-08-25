#include <cmoc.h>
#include "loader.h"

/* $CFC1 is Disk Basic's LOADM: filename from CHARAD, entry point stored at $9D,
   and it RETURNS (hdbdos.asm:5094 calls it with JSR). Being ROM it can copy a
   program over the menu unharmed, so the stub below - parked in the cassette
   buffer, clear of anything loading at $0E00 - regains control afterwards. */

#define CB_ANCHOR ((byte *) 0x01DA)   /* CHARAD sits here; GETNCH skips it */
#define CB_QUOTE  ((byte *) 0x01DB)
#define CB_NAME   ((char *) 0x01DC)
#define CB_STUB   ((byte *) 0x01F0)
#define CHARAD    (*((word *) 0x00A6))

/* CLEAR leaves Basic's stack too low for a big program; use a cold-boot one. */
#define BIG_STACK 0x7F00

static const byte stub[] = {
  0x10, 0xCE, (byte) (BIG_STACK >> 8), (byte) BIG_STACK,
                            /* LDS  #BIG_STACK                        */
  0xBD, 0xCF, 0xC1,         /* JSR  $CFC1    Disk Basic LOADM         */
  0x7F, 0xFF, 0x40,         /* CLR  $FF40    drive motor off          */
  0xAD, 0x9F, 0x00, 0x9D,   /* JSR  [$009D]  call the loaded program  */
  0x7F, 0x00, 0x71,         /* CLR  $0071    clear warm-start flag    */
  0x7E, 0xA0, 0x27          /* JMP  $A027    cold start               */
};

void run_file(const char *filename, byte is_basic)
{
  byte i;

  if (is_basic) {
    /* Basic's RUN entry takes the CHARAD character in A: 'M' selects RUNM,
       anything else reaches DVEC18, which matches '"' (hdbdos.asm:1782).
       Basic keeps control from here. */
    *((byte *) 0x02DD) = '"';
    strcpy((char *) 0x02DE, filename);
    CHARAD = 0x02DD;

    asm
    {
      ldd #$221C
      jmp $AE75
    }
  }

  /* Machine language: plant the stub, then hand it the command line LOADM
     parses - any byte, a quote, the name, then NUL as end of line. The stub
     calls LOADM, calls the program, and resets when it returns. Extended addressing all through the stub, since a
     program that has just returned leaves DP set to anything it likes. */
  for (i = 0; i < sizeof(stub); i++)
    CB_STUB[i] = stub[i];

  *CB_ANCHOR = ' ';
  *CB_QUOTE = '"';
  strcpy(CB_NAME, filename);

  CHARAD = (word) CB_ANCHOR;

  asm
  {
    jmp $01F0
  }
}
