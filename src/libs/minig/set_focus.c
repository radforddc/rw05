/* C version of minig_x fortran subroutines
   version 0.0   D.C. Radford    July 1999
 */
#include <stdio.h>
#include <X11/Xlib.h>
#include "minig.h"

extern Display   *disp_id;
extern Screen    *screen_id;
extern Window    win_id;
extern int       menu_mode;

/* ======================================================================= */
int set_focus(int focus_id)
{
  int    revert_to;
  Window current_focus;

  /* set input focus */
  XGetInputFocus(disp_id, &current_focus, &revert_to);
  if (current_focus != focus_id) {
      // if (current_focus == win_id ||
      // (menu_mode && current_focus != focus_id)) {
    // printf("Setting focus from %lu to %d\n", current_focus, focus_id);
    XFlush(disp_id);
    //XSync(disp_id, 0);
    XSetInputFocus(disp_id, (Window) focus_id, revert_to, CurrentTime);
    XFlush(disp_id);
    //XSync(disp_id, 0);
  }
  return 0;
} /* set_focus */
