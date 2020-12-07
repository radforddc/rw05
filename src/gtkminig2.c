#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

/* global data */
/* These are defined in miniga.c */
extern float fdx, fx0, fdy, fy0;
extern int   idx, ix0, idy, iy0, yflag;
/* fdx = range of channels on x-axis
   fx0 = starting channel on x-axis
   fdy = range of counts on y-axis
   fy0 = starting counts on y-axis
   idx = no. of x-pixels in display region
   ix0 = low x-pixel in display region
   idy = no. of y-pixels in display region
   iy0 = low y-pixel in display region
   yflag = 1 for linear y-axis
           2 for sqrt y-axis
           3 for log y-axis */

extern Display     *disp_id;
extern Screen      *screen_id;
extern GdkWindow   *win_id;
extern GdkGC       *xmgc, *gc_id, *gc_comp_id;
extern GdkColormap *colormap;
extern GdkPoint    points[512];
extern GdkColor    color_id[20];
extern int         win_width, win_height, nstored, ixp, iyp;

extern int bell(void);
extern int define_color(char *);

/* These are defined in xm234.c */
extern GtkWidget    *drawing_area_gls, *drawing_area_sp;
extern GdkPixmap    *pixmap;
extern int          igw_id;
extern int          zoom;
extern float        xin, yin, xmg[8];
extern char         cin[40];
extern FILE         *hcscrfile;
extern int          hclog;

extern void drawcrosshairs(void), removecrosshairs(void);
extern void copy_area(void);

int select_gw(int);

/* ======================================================================= */
int initg0(int n, int *nx, int *ny)
{
  static int   first = 1;
  int          igw;


  if (first) {
    first = 0;
    /* open scratch file for storage of data for possible hardcopy */
    if (!(hcscrfile = tmpfile())) {
      printf("ERROR - cannot open temp file!\n");
      hclog = -1;
    }

    /* Initialize the two windows. */
    for (igw = 1; igw <= 2; igw++) {
      select_gw(igw);
      if (igw == 1) {
	win_id = drawing_area_sp->window;
      } else {
	win_id = pixmap;
      }
      gc_id      = gdk_gc_new(win_id);
      gc_comp_id = gdk_gc_new(win_id);
      gdk_gc_copy(gc_id, xmgc);
      gdk_gc_copy(gc_comp_id, xmgc);
      gdk_gc_set_foreground(gc_id, &color_id[2]);
      gdk_gc_set_background(gc_id, &color_id[1]);
      gdk_gc_set_foreground(gc_comp_id, &color_id[2]);
      /* set GC for complement drawing */
      gdk_gc_set_function  (gc_comp_id, GDK_INVERT);
    }
  }
  select_gw(n);
  if (n == 1) {
    win_width  = drawing_area_sp->allocation.width;
    win_height = drawing_area_sp->allocation.height;
  }
  *nx = win_width  - 5;
  *ny = win_height - 25;
  return 0;
}

int initg(int *nx, int *ny)
{
  return initg0(1, nx, ny);
}

int initg2(int *nx, int *ny)
{
  return initg0(2, nx, ny);
}

/* ======================================================================= */
int dbuf(void)
{
  /* empty buffer of stored x,y values */
  if (nstored >= 2) {
    gdk_draw_lines(win_id, gc_id, points, nstored);
    nstored = 0;
  }
  XFlush(disp_id);
//   gdk_draw_pixmap(win_id, xmgc, pixmap,
// 		  0, 0, 0, 0, win_width, win_height);
  return 0;
} /* dbuf */

/* ======================================================================= */
int finig(void)
{
  /* empty buffer of stored x,y values */
  if (nstored >= 2) {
    gdk_draw_lines(win_id, gc_id, points, nstored);
    nstored = 0;
  }
  XFlush(disp_id);
  copy_area();
  select_gw(1);
  return 0;
} /* finig */

/* ======================================================================= */
int erase(void)
{
  if (win_id == drawing_area_sp->window) {
    /* rewind scratch file for storage of data for possible hardcopy */
    if (hclog > 0) {
      fflush(hcscrfile);
      rewind(hcscrfile);
      fwrite("s", 1, 1, hcscrfile);
      fwrite(&win_width, 4, 1, hcscrfile);
      fwrite(&win_height, 4, 1, hcscrfile);
    }
    gdk_draw_rectangle(win_id, xmgc, TRUE, 0, 0, win_width, win_height);
  } else {
    gdk_draw_rectangle(win_id, xmgc, TRUE, 0, 0, win_width, win_height);
    if (zoom < 8) copy_area();
  }
  return 0;
} /* erase */

/* ======================================================================= */
int select_gw(int n)
{
  static float  nfdx[2], nfx0[2], nfdy[2], nfy0[2];
  static int    nidx[2], nix0[2], nidy[2], niy0[2], nyflag[2];
  static int    nwin_width[2], nwin_height[2], nixp[2], niyp[2];
  static GdkWindow *nwin_id[2];
  static GdkGC     *ngc_id[2], *ngc_comp_id[2];
  static int    nsave = 0;

  if (nsave == n || n == 0) return 0;
  if (nsave) {
    nfdx[nsave - 1] = fdx;
    nfx0[nsave - 1] = fx0;
    nfdy[nsave - 1] = fdy;
    nfy0[nsave - 1] = fy0;
    nidx[nsave - 1] = idx;
    nix0[nsave - 1] = ix0;
    nidy[nsave - 1] = idy;
    niy0[nsave - 1] = iy0;
    nixp[nsave - 1] = ixp;
    niyp[nsave - 1] = iyp;
    nyflag[nsave - 1] = yflag;
    nwin_id    [nsave - 1] = win_id;
    nwin_width [nsave - 1] = win_width;
    nwin_height[nsave - 1] = win_height;
    ngc_id     [nsave - 1] = gc_id;
    ngc_comp_id[nsave - 1] = gc_comp_id;
  }

  nsave = n;
  fdx = nfdx[nsave - 1];
  fx0 = nfx0[nsave - 1];
  fdy = nfdy[nsave - 1];
  fy0 = nfy0[nsave - 1];
  idx = nidx[nsave - 1];
  ix0 = nix0[nsave - 1];
  idy = nidy[nsave - 1];
  iy0 = niy0[nsave - 1];
  ixp = nixp[nsave - 1];
  iyp = niyp[nsave - 1];
  yflag = nyflag[nsave - 1];
  win_id     = nwin_id[nsave - 1];
  win_width  = nwin_width[nsave - 1];
  win_height = nwin_height[nsave - 1];
  gc_id      = ngc_id[nsave - 1];
  gc_comp_id = ngc_comp_id[nsave - 1];

  if (hclog >= 0) hclog = 2 - n;

  return 0;
} /* select_gw */

/* ======================================================================= */
int setcolor(int icol)
{
  GdkGC *tmp;

  /* call dbuf to dump stored plot array */
  /* before changing attribute block */
  dbuf();
  if (icol == -1) {
    /* interchange the graphics contexts to use do hiliting */
    tmp = gc_id;
    gc_id = gc_comp_id;
    gc_comp_id = tmp;
   return 0;
  }
  if (icol < 0 || icol > 15) return 1;
  gdk_gc_set_foreground(gc_id, &color_id[icol + 1]);

  /* log info to temp file for possible later hardcopy */
  if (hclog > 0) {
    fwrite("c", 1, 1, hcscrfile);
    fwrite(&icol, 4, 1, hcscrfile);
  }
  return 0;
} /* setcolor */

/* ======================================================================= */
void retic(float *xout, float *yout, char *cout)
{
  static int   callCount = 0;
  static float *ixp[40], *iyp[40];
  static char  *icp[40];

  /* save incoming addresses in case retic is called again
     with different variables */
  ixp[callCount] = xout;
  iyp[callCount] = yout;
  icp[callCount] = cout;
  callCount++;

  /* process events until we get a valid input event in the drawing area */
  bell();
  igw_id = 0;
  cin[0] = 0;
  drawcrosshairs();
  while (igw_id != 1) gtk_main_iteration();

  /* restore incoming addresses and return values */
  callCount--;
  xout = ixp[callCount];
  yout = iyp[callCount];
  cout = icp[callCount];
  strncpy(cout, cin, 4);
  *xout = xin;
  *yout = yin;
  igw_id = 0;
  cin[0] = 0;
  removecrosshairs();
} /* retic */

/* ======================================================================= */
void retic2(float *xout, float *yout, char *cout)
{
  static int   callCount = 0;
  static float *ixp[40], *iyp[40];
  static char  *icp[40];

  /* save incoming addresses in case retic2 is called again
     with different variables */
  ixp[callCount] = xout;
  iyp[callCount] = yout;
  icp[callCount] = cout;
  callCount++;

  /* process events until we get a valid input event in the drawing area */
  bell();
  igw_id = 0;
  cin[0] = 0;
  while (igw_id != 2) gtk_main_iteration();

  /* restore incoming addresses and return values */
  callCount--;
  xout = ixp[callCount];
  yout = iyp[callCount];
  cout = icp[callCount];
  strncpy(cout, cin, 4);
  *xout = xin;
  *yout = yin;
  igw_id = 0;
  cin[0] = 0;
} /* retic2 */

/* ======================================================================= */
void retic3(float *xout, float *yout, char *cout, int *wout)
{
  static int   callCount = 0;
  static float *ixp[40], *iyp[40];
  static char  *icp[40];
  static int   *iwp[40];

  /* save incoming addresses in case retic3 is called again
     with different variables */
  ixp[callCount] = xout;
  iyp[callCount] = yout;
  icp[callCount] = cout;
  iwp[callCount] = wout;
  callCount++;

  /* process events until we get a valid input event in the drawing area */
  bell();
  igw_id = 0;
  cin[0] = 0;
  drawcrosshairs();
  while (igw_id == 0) gtk_main_iteration();

  /* restore incoming addresses and return values */
  callCount--;
  xout = ixp[callCount];
  yout = iyp[callCount];
  cout = icp[callCount];
  wout = iwp[callCount];
  strncpy(cout, cin, 4);
  *xout = xin;
  *yout = yin;
  *wout = igw_id;
  igw_id = 0;
  cin[0] = 0;
  removecrosshairs();
} /* retic3 */
