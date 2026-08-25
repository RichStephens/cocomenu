#include <cmoc.h>
#include <coco.h>

int main(void)
{
  byte k = 0;
  byte i;

  width(32);
  cls(1);
  printf("COUNT.BIN IS RUNNING\n\n");

  for (i = 1; i <= 10; i++) {
    printf("%d ", i);
    delay(6);
  }

  printf("\n\nRUN ME TWICE TO PROVE THE\n");
  printf("MENU REALLY SURVIVES.\n\n");
  printf("PRESS A KEY TO GO BACK.\n");

  while (!k)
    k = inkey();

  return 0;
}
