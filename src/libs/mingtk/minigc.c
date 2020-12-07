/* gtk+ version of minig (X11) subroutines
   version 0.0   D.C. Radford    July 2003 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "minig.h"

extern GdkPixmap *pixmap;
extern GtkWidget *toplevel, *drawing_area;
extern int       InReticMode;       /* used to ignore unwanted button presses etc */
extern Display   *disp_id;
extern GdkGC     *xmgc, *gc_id, *gc_comp_id;
extern int   win_width, win_height, ixp, iyp, ixb, iyb, ibnum;
extern int   hclog;
extern FILE  *hcscrfile;
extern char  cin[40];

int done_report_curs(void);

/* ======================================================================= */
int erase(void)
{
  /* rewind scratch file for storage of data for possible hardcopy */
  if (hclog > 0) {
    fflush(hcscrfile);
    rewind(hcscrfile);
    fwrite("s", 1, 1, hcscrfile);
    fwrite(&win_width, 4, 1, hcscrfile);
    fwrite(&win_height, 4, 1, hcscrfile);
  }

  gdk_draw_rectangle(pixmap, xmgc, TRUE, 0, 0, win_width, win_height);
  gdk_draw_pixmap(drawing_area->window, gc_id, pixmap,
		  0, 0, 0, 0, win_width, win_height);
  return 0;
} /* erase */

/*--------------------------------------------------------*/
void set_up_cursor()
{
  /* draw cross hairs (complement) */
  gdk_draw_line(pixmap, gc_comp_id, ixp, 0, ixp, win_height);
  gdk_draw_line(pixmap, gc_comp_id, 0, iyp, win_width, iyp);
  gdk_draw_pixmap(drawing_area->window, xmgc, pixmap,
		  0, 0, 0, 0, win_width, win_height);
  InReticMode = 1;
} /* set_up_cursor */

/*--------------------------------------------------------*/
void done_cursor()
{
  /* remove cross hairs (complement) */
  gdk_draw_line(pixmap, gc_comp_id, ixp, 0, ixp, win_height);
  gdk_draw_line(pixmap, gc_comp_id, 0, iyp, win_width, iyp);
  gdk_draw_pixmap(drawing_area->window, xmgc, pixmap,
		  0, 0, 0, 0, win_width, win_height);
  InReticMode = 0;
  done_report_curs();
} /* done_cursor */

/* ======================================================================= */
int retic(float *x, float *y, char *cout)
{
  /* call up cursor
     x,y  returns position w.r.to axes when cursor terminated
     cout returns pressed key, or G for mouse button */

  int  ixi, iyi;

  /* ring bell */
  if (strncmp(cout, "NOBELL", 6)) bell();

  set_up_cursor();
  ibnum = 0;
  *cout = cin[0] = 0;

  while (!(*cout)) {
    gtk_main_iteration();
    /* printf("ibnum = %d\n", ibnum);  fflush(stdout); */
    if (ibnum) {
      if (ibnum == 2) {
	strcpy(cout, "X2");
      } else if (ibnum == 3) {
	strcpy(cout, "X2");
      } else {
	strcpy(cout, "G");
      }
      ixi = ixb; iyi = iyb;
    }
    if (cin[0]) {
      strcpy(cout, cin);
      ixi = ixb; iyi = iyb;
    }
  }
  done_cursor();

  /* convert coordinates and exit */
  *x = 0.f;
  *y = 0.f;
  if (ixi >= 0 && ixi <= win_width &&
      iyi >= 0 && iyi <= win_height) {
    iyi = win_height - iyi;
    cvxy(x, y, &ixi, &iyi, 2);
  }
  return 0;
} /* retic */
