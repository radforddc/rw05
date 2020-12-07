#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "minig.h"

int main(void) 
{
  /* used only for testing/debugging this version of minig */
  float x, y;
  int   n, lg, nx, ny;
  char  ans[40];

  set_xwg("standard");
  initg(&nx, &ny);
  erase();
  limg(nx, 0, ny/2, ny/2);
  trax(110.f, 0.f, 110.f, 0.f, 1);
  lg = (nx/80) << 1;
  for (n = 1; n <= 10; ++n) {
    x = (float) (n * 10);
    y = x;
    setcolor(n + 1);
    symbg(n, x, y, lg);
  }
  mspot(0, ny);
  putg("MINIG sample graph.", 20, 3);
  finig();

  /* call cursor */
  printf(" Type any character; X to exit\n");
  ans[0] = 0;
  while (ans[0] != 'X' && ans[0] != 'x') {
    retic(&x, &y, ans);
    printf("%s %f %f\n", ans, x, y);
  }

  /* erase the graphics window */
  erase();
  printf("The graphics screen was just cleared.\n"
	 " Type any character to exit.\n");
  retic(&x, &y, ans);
  save_xwg("standard");
  return 0;
} /* main */

