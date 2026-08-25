#include <cmoc.h>
#include <coco.h>

int main(void)
{
  byte k = 0;

  width(32);
  cls(1);
  printf("HELLO.BIN IS RUNNING\n\n");
  printf("I WAS LOADED AND ENTERED\n");
  printf("WITH JSR BY THE MENU.\n\n");
  printf("WHEN I RETURN FROM MAIN,\n");
  printf("CMOC DOES LDS INISTK/RTS\n");
  printf("AND YOU LAND BACK IN THE\n");
  printf("MENU. NOTHING IN HERE\n");
  printf("KNOWS ABOUT THE MENU.\n\n");
  printf("PRESS A KEY TO GO BACK.\n");

  while (!k)
    k = inkey();

  return 0;
}
