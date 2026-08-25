# cocomenu

A small TRS-80 Color Computer program that reads the directory of the disk it
booted from, lists the other programs on it - machine language and BASIC - and
runs the one you pick. When a machine-language program exits, **you come back to
the menu**, and it needs no changes to do so.

Like BITSY BYE on the Apple II. 2050 bytes, one granule, small enough to drop on
every disk.

## Requirements

**HDB-DOS is what makes the return work.** It runs `AUTOEXEC.BAS` on a cold
start, and that is what brings the menu back:

| program type | getting back to the menu |
|---|---|
| machine language | automatic - the menu cold-starts once the program returns |
| BASIC | type **`DOS`** at the `OK` prompt |

`DOS` works because HDB-DOS's `DOSCOM` reads track 34 sector 1 and, finding no
OS-9 header, falls through to `DOAUTO` and runs `AUTOEXEC.BAS`
(`hdbdos.asm:2525`, `:4041`). BASIC keeps control of the machine when a BASIC
program ends, so there is nothing the menu can do to return by itself - but `DOS`
is one word and needs no cooperation from the program.

On stock RS-DOS neither route works. The menu still runs and still launches
programs, but `AUTOEXEC.BAS` does not autoboot (type `RUN"AUTOEXEC"` to start it),
a returning machine-language program leaves you at `OK`, and `DOS` tries to boot
OS-9 rather than falling back to `AUTOEXEC.BAS`.

Also assumed:

- **Disk Basic 1.1.** `$CFC1` (LOADM) and `$AE75` (RUN) are ROM entry points, taken
  from `hdbdos.asm`. Disk Basic 1.0 may place them elsewhere.
- **A 35-track single-sided RS-DOS disk** - directory on track 17, 68 granules, so
  at most 68 files.
- **The 32-column text screen.** The menu does not call `width(32)`, since it is
  only ever entered from a cold start where that is already the mode. Loading it
  by hand from a CoCo 3 in 40- or 80-column mode will draw wrong.

## How it runs things, and gets back

Every launch goes through Disk Basic's own loader. The menu never loads anything
itself, and never has to survive what it starts. It lists both machine-language
(type 2) and BASIC (type 0) files, and picks the path from the directory entry's
file type.

**BASIC programs** are started with `RUN"NAME"`. Basic's RUN entry takes the
character at CHARAD in A: `'M'` selects RUNM, anything else falls through to the
plain filename form (`hdbdos.asm:1782`). Basic keeps control from there, so the
program ends at the `OK` prompt and `DOS` brings the menu back - see
[Requirements](#requirements).

**Machine-language programs** go through a stub, which gets control back by
itself and needs no cooperation from the program:

`$CFC1` is Disk Basic's LOADM: it reads the filename from CHARAD, stores the
file's entry point at `$9D`, and **returns** - which is exactly how the `RUNM`
patch in `hdbdos.asm` uses it (`jsr >LCFC1   Do a LOADM...`). Being ROM, it can
copy a program straight over the menu without harming itself.

So the menu plants a 20-byte stub in the cassette buffer at `$01F0` - below the
DOS area, out of reach of anything loading at `$0E00` - and jumps to it:

    LDS  #$7F00     give the program a normal high stack
    JSR  $CFC1      Disk Basic LOADM - ROM copies the file in
    CLR  $FF40      drive motor off
    JSR  [$009D]    call the loaded program
    CLR  $0071      clear the warm-start flag
    JMP  $A027      cold start

The program returns to the stub the same way it would return to any caller -
every CMOC program does, because `exit()` and returning from `main` both end in
`LDS INISTK` / `RTS`, which pops the caller's return address. Nothing in the
launched program knows the menu exists. The cold start then makes HDB-DOS re-run
`AUTOEXEC.BAS`, and the menu is back.

Because the menu is disposable, **a program may load anywhere and be any size.**
There is no overlap check, no address arithmetic, and no constraint tying the
menu's address to what it can run.

## The list

Nothing goes through Basic's console. The menu writes screen cells directly at
`$0400`: the VDG character index is the low 6 bits of ASCII and bit 6 set means
normal video, so a cell is `(ascii & $3F) | $40`. That replaced `locate`, `cls`,
`putstr`, `putchar` and `strlen` - 257 bytes of ROM wrappers - with about 50, and
it sidesteps CHROUT's scroll-on-last-cell behavior entirely.

The screen shows 13 names at a time between two thin cyan rules, drawn with VDG
semigraphics-4 blocks (`1 CCC LLLL` - bit 7 set, bits 6-4 color, low nibble the
lit quadrants). The rules use half-height blocks so they sit tight against the
list. The selected name is a full-width reverse-video bar: in alphanumeric mode
bit 6 is the inverse bit, so the row's cells are simply raised by `$40`.

A longer list scrolls with the selection, and a solid orange block appears at the
right end of the top or bottom rule when there are more names off that end.

Keys:

| key | does |
|---|---|
| up / down | move one name |
| SHIFT + up / down | page by 13 |
| ENTER | run the selected program |

`MAX_ENTRIES` is 68, which a disk cannot exceed: a 35-track disk has 68 granules
and every file needs at least one, so 68 files is the physical maximum. The cap
costs nothing on disk - the name table is BSS, which is not in the .bin.

## Trying it

    make disk

Builds `cocomenu.dsk` - the only disk this repo produces - carrying the menu plus:

| program | linked at | what it shows |
|---|---|---|
| `HELLO.BIN` | `$0E00` | prints, waits, returns from `main` -> you land back in the menu |
| `COUNT.BIN` | `$0E00` | counts to 10 and returns; run it twice to prove the menu really survived |
| `TOOBIG.BIN` | `$7000` | linked on top of the menu on purpose, so it goes through Disk Basic's LOADM and the stub, running and then rebooting back to the menu |
| `PROG01`..`PROG20` | `$0E00` | 787-byte filler, so the list runs past the 13 visible rows and scrolls |

That is 23 names against 13 rows, so the list scrolls and the `^` / `V` marks
appear. Each `PROGnn` announces which one it is, so you can confirm the menu ran
the one you actually selected rather than an off-by-one neighbor.

`make testdisk SCROLL_N=40` generates more filler if you want a longer list.

Mount it and start it. Under HDB-DOS the `AUTOEXEC.BAS` runs by itself; under
stock Disk Basic it does not, so type:

    RUN"AUTOEXEC"

Then use the up/down arrows, SHIFT+up/down to page, and ENTER. Nothing in
`HELLO.BIN` or `COUNT.BIN` knows the menu exists - coming back is entirely CMOC's
`LDS INISTK` / `RTS` exit path landing on the return address the menu's `JSR`
pushed.

Picking `TOOBIG.BIN` should stop and warn instead of running straight in. If you
accept, Disk Basic loads it and the stub calls it; pressing a key inside it
returns to the stub, which cold-starts, and `AUTOEXEC.BAS` brings the menu back.
The reboot is visible and takes a moment - that is the intended behavior, not a
crash.

## Build

Needs `cmoc`, `decb` and `writecocofile` on `PATH`.

    make            # same as `make disk`
    make disk       # cocomenu.dsk: the menu and its AUTOEXEC, nothing else
    make testdisk   # the same image, plus the demo programs
    make size       # size report
    make clean

Both disk targets write `cocomenu.dsk`, removing it first so the image is never
a mix of the two. Nothing is copied anywhere; deploying is a separate step.

To put the menu on an existing disk image:

    writecocofile -b yourdisk.dsk cocomenu.bin
    writecocofile -n yourdisk.dsk AUTOEXEC.BAS

## Layout

    src/dsk.c      sector reads via the DSKCON ROM vector at $C004
    src/dirent.c   RS-DOS directory walk (track 17, sectors 3-18)
    src/loader.c   granule-chain file reader, DECB segment loader, launch
    src/main.c     the menu itself

Disk access goes straight at DSKCON rather than through cmoc's `<disk.h>`,
because that header's implementation is a single 1804-byte object that links in
whole. Calling the same ROM vector directly costs about 30 bytes and works over
DriveWire/FujiNet just the same, since that is the path HDB-DOS hooks.
