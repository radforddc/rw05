#include <stdio.h>
#include "minig.h"

/* ======================================================================= */
int report_curs(int ix, int iy)
{
  float  x, y;

  cvxy(&x, &y, &ix, &iy, 2);
  printf(" x, y: %.1f, %.1f         \r", x, y);
  fflush(stdout);
  return 0;
}
/* ======================================================================= */
int done_report_curs(void)
{
  printf("                                  \r");
  fflush(stdout);
  return 0;
}
