#include <cmoc.h>
#include <coco.h>
#include "dsk.h"
#include "dirent.h"
#include "loader.h"
#include "chardef.h"

#ifndef MENU_NAME
#define MENU_NAME "COCOMENU"
#endif

#define MAX_ENTRIES  68
#define NAME_LEN     16   /* power of two: indexing shifts instead of calling MUL16 */
#define RULE_TOP     0
#define FIRST_ROW    1
#define ROWS_VISIBLE 13
#define RULE_BOT     14
#define LAST_ROW     15
#define LABEL_COL    2
#define LABEL_MAX    25   /* stops short of the rule's scroll triangle */

#define SCREEN      ((byte *) 0x0400)
#define SCREEN_COLS 32

/* VDG semigraphics-4: bit 7 set, bits 6-4 color, low nibble the lit quadrants
   (bit3 upper-left, bit2 upper-right, bit1 lower-left, bit0 lower-right). */
#define SG(color, quad) ((byte) (0x80 | ((color) << 4) | (quad)))

#define SG_CYAN 5

#define QUAD_ALL 0x0F
#define QUAD_TOP 0x0C
#define QUAD_BOT 0x03

/* Bit 6 SET is normal video, CLEAR is reversed (fujinet-config src/coco/bar.c).
   The VDG index is the low 6 bits of ASCII, so a normal cell is
   (ascii & $3F) | $40. */
#define NORMAL_VIDEO 0x40
#define BLANK_CELL   (NORMAL_VIDEO | ' ')

#define Q_UL 0x08
#define Q_UR 0x04
#define Q_LL 0x02
#define Q_LR 0x01

static char names[MAX_ENTRIES][NAME_LEN];
static byte types[MAX_ENTRIES];
static byte count;

static byte *row_ptr(byte row)
{
  return SCREEN + ((word) row << 5);
}

static byte say(byte col, byte row, const char *s)
{
  byte *p = row_ptr(row) + col;

  while (*s) {
    *p++ = (byte) ((*s++ & 0x3F) | NORMAL_VIDEO);
    col++;
  }
  return col;
}


static void fill_row(byte row, byte v)
{
  byte *p = row_ptr(row);
  byte c;

  for (c = 0; c < SCREEN_COLS; c++)
    *p++ = v;
}


/* Flips normal/reversed. The row's state is always known, so one flip serves
   both directions. */
static void flip_row(byte row)
{
  byte *p = row_ptr(row);
  byte c;

  for (c = 0; c < SCREEN_COLS; c++, p++)
    *p ^= NORMAL_VIDEO;
}

/* Last three cells of a rule: a triangle pointing where the list continues, or
   plain rule. */
static void rule_end(byte row, byte a, byte b, byte c)
{
  byte *p = row_ptr(row) + SCREEN_COLS - 4;

  *p++ = SG(SG_CYAN, a);
  *p++ = SG(SG_CYAN, b);
  *p = SG(SG_CYAN, c);
}

static byte name_is(const char *name, const char *want)
{
  byte i;

  for (i = 0; want[i]; i++) {
    if (name[i] != want[i])
      return 0;
  }
  return name[i] == '\0' || name[i] == '.';
}

static void scan_disk(void)
{
  byte type;

  count = 0;
  dir_rewind();

  while (count < MAX_ENTRIES && dir_next(names[count], &type)) {
    if (type != DECB_TYPE_ML && type != DECB_TYPE_BASIC)
      continue;
    if (name_is(names[count], MENU_NAME) || name_is(names[count], "AUTOEXEC"))
      continue;

    types[count] = type;
    count++;
  }
}

/* Subtraction, so cmoc's division helpers stay unlinked. */
static byte say_num(byte col, byte row, byte v)
{
  char d[4];
  char *p = d;
  byte h = 0, t = 0;

  while (v >= 100) {
    v -= 100;
    h++;
  }
  while (v >= 10) {
    v -= 10;
    t++;
  }
  if (h)
    *p++ = (char) ('0' + h);
  if (h || t)
    *p++ = (char) ('0' + t);
  *p++ = (char) ('0' + v);
  *p = '\0';
  return say(col, row, d);
}

/* Text set into a rule, with a blank cell either side. */
static void rule_label(byte row, const char *s)
{
  *(row_ptr(row) + LABEL_COL - 1) = BLANK_CELL;
  *(row_ptr(row) + say(LABEL_COL, row, s)) = BLANK_CELL;
}

/* The VDG has no lowercase, and (c & $3F) would turn it into punctuation. */
static void draw_title(void)
{
  char title[LABEL_MAX + 1];
  byte i, c;

  if (!dsk_read(dsk_buf, DIR_TRACK, TITLE_SECTOR) || dsk_buf[0] == 0xFF)
    return;

  for (i = 0; i < LABEL_MAX; i++) {
    c = dsk_buf[i];
    if (c < ' ' || c > 'z')
      break;
    if (c >= 'a')
      c -= 'a' - 'A';
    title[i] = (char) c;
  }
  title[i] = '\0';

  if (i)
    rule_label(RULE_TOP, title);
}

/* Redrawn only when the disk changes. */
static void draw_frame(void)
{
  byte col;

  /* No screen wipe: every row is painted by someone - 0 and 14 here, 1-13 by
     draw_list, 15 just below. */
  fill_row(RULE_TOP, SG(SG_CYAN, QUAD_BOT));
  fill_row(RULE_BOT, SG(SG_CYAN, QUAD_TOP));
  draw_title();

  fill_row(LAST_ROW, BLANK_CELL);
  say(0, LAST_ROW, "UP/DN SHFT=PG ENTER=RUN BRK=EXIT");

  if (dsk_count < 2)
    return;

  rule_label(RULE_BOT, "</>=DISK ");
  col = say_num(LABEL_COL + 9, RULE_BOT, dsk_drive + 1);
  col = say(col, RULE_BOT, " OF ");
  col = say_num(col, RULE_BOT, dsk_count);
  *(row_ptr(RULE_BOT) + col) = BLANK_CELL;
}

/* Only for a window or disk change; moving within a page just moves the
   highlight. */
static void draw_list(byte sel, byte top)
{
  byte i, row;

  for (i = 0; i < ROWS_VISIBLE; i++) {
    row = top + i;
    fill_row(FIRST_ROW + i, BLANK_CELL);
    if (row >= count)
      continue;
    say(1, FIRST_ROW + i, names[row]);
    if (row == sel)
      flip_row(FIRST_ROW + i);
  }

  if (!count)
    say(1, FIRST_ROW, "NO PROGRAMS");

  if (top)
    rule_end(RULE_TOP, Q_LR, QUAD_ALL, Q_LL);
  else
    rule_end(RULE_TOP, QUAD_BOT, QUAD_BOT, QUAD_BOT);

  if (top + ROWS_VISIBLE < count)
    rule_end(RULE_BOT, Q_UR, QUAD_ALL, Q_UL);
  else
    rule_end(RULE_BOT, QUAD_TOP, QUAD_TOP, QUAD_TOP);
}

static byte wait_key(void)
{
  byte k = 0;

  while (!k)
    k = inkey();
  return k;
}

static void launch(byte sel)
{
  char base[NAME_LEN];
  byte i;

  /* LOADM and RUN each supply their own default extension. */
  for (i = 0; i < NAME_LEN - 1 && names[sel][i] && names[sel][i] != '.'; i++)
    base[i] = names[sel][i];
  base[i] = '\0';

  fill_row(LAST_ROW, BLANK_CELL);
  say(0, LAST_ROW, "RUNNING: ");
  say(9, LAST_ROW, base);
  say(9 + i, LAST_ROW, types[sel] == DECB_TYPE_BASIC ? ".BAS" : ".BIN");

  run_file(base, types[sel] == DECB_TYPE_BASIC);   /* does not return */
}

int main(void)
{
  byte sel, top, key, prev_sel, prev_top;

  /* The cold start does not undo width(80), and the menu draws straight to the
     32-column VDG screen, so put the video mode back before drawing. */
  initCoCoSupport();
  if (isCoCo3)
    width(32);

  dsk_init();

  for (;;) {
    scan_disk();
    sel = top = 0;
    draw_frame();
    draw_list(sel, top);

    for (;;) {
      key = wait_key();

      /* $A928 clears the screen and homes BASIC's cursor. AUTOEXEC.BAS ends
         after the EXEC this returns to, leaving BASIC at OK on this disk. */
      if (key == KEY_BREAK) {
        asm
        {
          jsr $A928
        }
        return 0;
      }

      if (key == KEY_RIGHT_ARROW && dsk_drive + 1 < dsk_count) {
        dsk_select(dsk_drive + 1);
        break;
      }
      if (key == KEY_LEFT_ARROW && dsk_drive) {
        dsk_select(dsk_drive - 1);
        break;
      }

      prev_sel = sel;
      prev_top = top;

      if (key == KEY_DOWN_ARROW) {
        if (sel + 1 < count)
          sel++;
      } else if (key == KEY_UP_ARROW) {
        if (sel)
          sel--;
      } else if (key == KEY_SHIFT_DOWN && count) {
        if (sel + ROWS_VISIBLE < count)
          sel += ROWS_VISIBLE;
        else
          sel = count - 1;
      } else if (key == KEY_SHIFT_UP) {
        if (sel > ROWS_VISIBLE)
          sel -= ROWS_VISIBLE;
        else
          sel = 0;
      } else if (key == KEY_ENTER && count) {
        launch(sel);              /* does not return */
      }

      if (sel < top)
        top = sel;
      if (sel >= top + ROWS_VISIBLE)
        top = sel - ROWS_VISIBLE + 1;

      if (top != prev_top)
        draw_list(sel, top);
      else if (sel != prev_sel) {
        flip_row(FIRST_ROW + prev_sel - top);
        flip_row(FIRST_ROW + sel - top);
      }
    }
  }

  return 0;
}
