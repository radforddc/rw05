/* C version of minig_x fortran subroutines
   version 0.0   D.C. Radford    July 1999 */
#include <stdio.h>
#include <X11/Xlib.h>

#include "minig.h"

extern Display   *disp_id;
extern Window    win_id;
extern GC        gc_comp_id;
extern XEvent    event;
extern int   win_width, win_height;
extern int   ixp, iyp;

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
