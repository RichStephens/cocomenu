/* Linked at the menu's own address to show that it does not matter: Disk
   Basic's LOADM does every load, so a program is free to land anywhere. */

#include <cmoc.h>
#include <coco.h>

int main(void)
{
  byte k = 0;

  width(32);
  cls(255);
  printf("TOOBIG.BIN IS RUNNING\n\n");
  printf("I WAS LINKED AT $2800,\n");
  printf("RIGHT ON TOP OF THE MENU.\n\n");
  printf("THE MENU DOES NOT CARE. IT\n");
  printf("NEVER LOADS ANYTHING; DISK\n");
  printf("BASIC DOES, AND A 26 BYTE\n");
  printf("STUB CALLED ME.\n\n");
  printf("A KEY RETURNS ME TO IT, IT\n");
  printf("REBOOTS, AND AUTOEXEC\n");
  printf("BRINGS THE MENU BACK.\n");

  while (!k)
    k = inkey();

  return 0;
}
