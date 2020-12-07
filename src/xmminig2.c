#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <Xm/MainW.h>

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
extern Display   *disp_id;
extern Screen    *screen_id;
extern Window    win_id;
extern GC        gc_id, gc_comp_id;
extern int   win_width, win_height;
extern int   nstored;
extern short points[512][2];
extern int   color[20], ixp, iyp;

extern int bell(void);
extern int define_color(char *);

/* These are defined in xm234.c */
extern XtAppContext app_context;
extern Widget       toplevel, mainWindow1, mainWindow2;
extern Window       window1, window2, root_id;
extern Cursor       watchCursor;
extern Dimension    width, height;
extern Pixmap       pixmap;
extern GC           xmgc;
extern int          igw_id, revert_to, scr_width, scr_height;
extern int          zoom;
extern float        xin, yin, xmg[8];
extern char         cin[40];
extern FILE  *hcscrfile;
extern int   hclog;

extern void drawcrosshairs(void), removecrosshairs(void);
extern void copy_area(void);

int select_gw(int);
#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"

/* ======================================================================= */
int set_xwg(char *in)
{
  FILE *fp;
  char filename[120], line[120];

  xmg[2] = 0;
  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xmg");
#else
  strcat (filename, "/.radware.xmg");
#endif
  if (!(fp = fopen(filename, "r+"))) return 1;
  while (fgets(line, 120, fp)) {
    if (!strncmp(line, in, 8)) {
      sscanf((char *) strchr(line,' '), " %f%f%f%f%f%f%f%f",
	     &xmg[0], &xmg[1], &xmg[2], &xmg[3],
	     &xmg[4], &xmg[5], &xmg[6], &xmg[7]);
    }
  }
  fclose(fp);
  return 0;
} /* set_xwg */

/* ======================================================================= */
int save_xwg(char *in)
{
  FILE         *fp;
  char         filename[80], line[20][120];
  int          lineNum = 0, done = 0, i;
  int          ix, iy;
  unsigned int w, h, junk_border, junk_depth;
  Window       junk;

  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xmg");
#else
  strcat (filename, "/.radware.xmg");
#endif
  if (!(fp = fopen(filename, "a+"))) {
    printf("Cannot open file %s\n", filename);
    return 1;
  }
  rewind(fp);

  XGetGeometry(disp_id,XtWindow(toplevel),&junk,&ix,&iy,
	       &w,&h,&junk_border,&junk_depth);
  XTranslateCoordinates(disp_id,XtWindow(toplevel),root_id,
			0,0,&ix,&iy,&junk);
  xmg[0] = (float)ix/(float)scr_width;
  xmg[1] = (float)iy/(float)scr_height;
  xmg[2] = (float)w/(float)scr_width;
  xmg[3] = (float)h/(float)scr_height;

  XGetGeometry(disp_id,XtWindow(mainWindow1),&junk,&ix,&iy,
	       &w,&h,&junk_border,&junk_depth);
  XTranslateCoordinates(disp_id,XtWindow(mainWindow1),root_id,
			0,0,&ix,&iy,&junk);
  xmg[4] = (float)ix/(float)scr_width;
  xmg[5] = (float)iy/(float)scr_height;
  xmg[6] = (float)w/(float)scr_width;
  xmg[7] = (float)h/(float)scr_height;

  while (fgets(line[lineNum], 120, fp) && lineNum < 20) {
    lineNum++;
    for (i=0; i<lineNum-1; i++) {
      if (!strncmp(line[i],line[lineNum-1],8)) {
	lineNum--;
	break;
      }
    }
  }

  fclose(fp);
  if (!(fp = fopen(filename, "w+"))) {
    printf("Cannot open file %s\n", filename);
    return 1;
  }
  for (i=0; i<lineNum; i++) {
    if (!strncmp(line[i], in, 8)) {
    done = 1;
    if (line[i][8] == ' ')
      sprintf(&line[i][8],
      	" %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f\n",
      	xmg[0],xmg[1],xmg[2],xmg[3],xmg[4],xmg[5],xmg[6],xmg[7]);
    }
    fprintf(fp, "%s", line[i]);
  }
  if (!done) {
    strncpy(line[0], in, 8);
    line[0][8] = '\0';
    fprintf(fp, "%s %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f\n",
	    line[0], xmg[0],xmg[1],xmg[2],xmg[3],xmg[4],xmg[5],xmg[6],xmg[7]);
  }
  fclose(fp);
  return 0;
}

/* ======================================================================= */
int initg0(int n, int *nx, int *ny)
{
  XGCValues    xgcvl;
  static int   first = 1, nsave;
  Window       junk_id;
  int          ix, iy;
  unsigned int w, h, junk_border, junk_depth;
  int          font, black, white, igw;

  nsave = n;
  select_gw(nsave);

  if (first) {
    first = 0;
    /* open scratch file for storage of data for possible hardcopy */
    if (!(hcscrfile = tmpfile())) {
      printf("ERROR - cannot open temp file!\n");
      hclog = -1;
    }
    /* Next instruction for debugging only */
    /* XSynchronize(disp_id, 1); */

    screen_id =  XDefaultScreenOfDisplay(disp_id);
    black =      BlackPixelOfScreen(screen_id);
    white =      WhitePixelOfScreen(screen_id);
    scr_width =  XWidthOfScreen(screen_id);
    scr_height = XHeightOfScreen(screen_id);

    /* Get named color values */
    color[1]  = white;
    color[2]  = black;
    color[3]  = define_color("RED");
    color[4]  = define_color("BLUE");
    color[5]  = define_color("MAGENTA");
    color[6]  = define_color("SLATE BLUE");
    color[7]  = define_color("ORANGE RED");
    color[8]  = define_color("FOREST GREEN");
    color[9]  = define_color("GREEN YELLOW");
    color[10] = define_color("MAROON");
    color[11] = define_color("CORAL");
    color[12] = define_color("CADET BLUE");
    color[13] = define_color("ORANGE");
    color[14] = define_color("PALE GREEN");
    color[15] = define_color("BROWN");
    color[16] = define_color("GREY30");

    /* Load the font for text writing */
    font = XLoadFont(disp_id, FONTNAME);

    /* Initialize the two windows. */
    for (igw = 1; igw <= 2; igw++) {
      select_gw(igw);

      /* Create graphics context */
      xgcvl.background = color[1];
      xgcvl.foreground = color[2];
      gc_id = XCreateGC(disp_id, win_id, GCForeground | GCBackground, &xgcvl);
      xgcvl.function = GXinvert;
      gc_comp_id = XCreateGC(disp_id, win_id, GCFunction, &xgcvl);
      XSetFont(disp_id, gc_id, font);
    }
    select_gw(nsave);
  }
  if (nsave == 1) {
    (void) XGetGeometry(disp_id, win_id, &junk_id, &ix, &iy,
                        &w, &h, &junk_border, &junk_depth);
    win_width  = (int) w;
    win_height = (int) h;
  }
  *nx = win_width - 5;
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
    XDrawLines(disp_id, win_id, gc_id, (XPoint *)points[0], nstored,
	       CoordModeOrigin);
    nstored = 0;
  }
  XFlush(disp_id);
  return 0;
} /* dbuf */

/* ======================================================================= */
int finig(void)
{
  /* empty buffer of stored x,y values */
  if (nstored >= 2) {
    XDrawLines(disp_id, win_id, gc_id, (XPoint *)points[0], nstored,
	       CoordModeOrigin);
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
  if (win_id == window1) {
    /* rewind scratch file for storage of data for possible hardcopy */
    if (hclog > 0) {
      fflush(hcscrfile);
      rewind(hcscrfile);
    }
    XClearWindow(disp_id, window1);
  } else {
    if (zoom < 8) {
      XFillRectangle(disp_id, pixmap, xmgc, 0, 0, width, height);
      copy_area();
    } else {
      XClearWindow(disp_id, window2);
    }
  }
  return 0;
} /* erase */

/* ======================================================================= */
int select_gw(int n)
{
  static float  nfdx[2], nfx0[2], nfdy[2], nfy0[2];
  static int    nidx[2], nix0[2], nidy[2], niy0[2], nyflag[2];
  static int    nwin_width[2], nwin_height[2], nixp[2], niyp[2];
  static Window nwin_id[2];
  static GC     ngc_id[2], ngc_comp_id[2];
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
  GC tmp;

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
  XSetForeground(disp_id, gc_id, color[icol + 1]);

  /* log info to temp file for possible later hardcopy */
  if (hclog > 0) {
    fwrite("c", 1, 1, hcscrfile);
    fwrite(&icol, 4, 1, hcscrfile);
  }
  return 0;
} /* setcolor */

/* ======================================================================= */
int set_size(int winpix, int width, int height)
{
  extern void display_gls(int);

  select_gw(2);
  win_id = (Window) winpix;
  win_width = width;
  win_height = height;
  select_gw(1);
  display_gls(-1);
  return 0;
} /* set_size */

/* ======================================================================= */
void retic(float *xout, float *yout, char *cout)
{
  static int   callCount = 0;
  static float *ixp[40], *iyp[40];
  static char  *icp[40];
  Time         time;

  /* save incoming addresses in case retic is called again
     with different variables */
  ixp[callCount] = xout;
  iyp[callCount] = yout;
  icp[callCount] = cout;
  callCount++;

  /* remove watch cusor to indicate open for business */
  XUndefineCursor(disp_id, XtWindow(mainWindow2));

  /* set input focus to required window */
  time = CurrentTime;
  XSetInputFocus(disp_id, window1, revert_to, time);

  /* process events until we get a valid input event in the drawing area */
  bell();
  igw_id = 0;
  drawcrosshairs();
  while (igw_id != 1) XtAppProcessEvent(app_context, XtIMAll);

  /* set cusor to watch to indicate busy */
  XDefineCursor(disp_id, XtWindow(mainWindow2), watchCursor);

  /* restore incoming addresses and return values */
  callCount--;
  xout = ixp[callCount];
  yout = iyp[callCount];
  cout = icp[callCount];
  strcpy(cout, cin);
  *xout = xin;
  *yout = yin;
  igw_id = 0;
  removecrosshairs();
} /* retic */

/* ======================================================================= */
void retic2(float *xout, float *yout, char *cout)
{
  static int   callCount = 0;
  static float *ixp[40], *iyp[40];
  static char  *icp[40];
  Time         time;

  /* save incoming addresses in case retic2 is called again
     with different variables */
  ixp[callCount] = xout;
  iyp[callCount] = yout;
  icp[callCount] = cout;
  callCount++;

  /* remove watch cusor to indicate open for business */
  XUndefineCursor(disp_id, XtWindow(mainWindow2));

  /* set input focus to required window */
  time = CurrentTime;
  XSetInputFocus(disp_id, window2, revert_to, time);

  /* process events until we get a valid input event in the drawing area */
  bell();
  igw_id = 0;
  while (igw_id != 2) XtAppProcessEvent(app_context, XtIMAll);

  /* set cusor to watch to indicate busy */
  XDefineCursor(disp_id, XtWindow(mainWindow2), watchCursor);

  /* restore incoming addresses and return values */
  callCount--;
  xout = ixp[callCount];
  yout = iyp[callCount];
  cout = icp[callCount];
  strcpy(cout, cin);
  *xout = xin;
  *yout = yin;
  igw_id = 0;
  removecrosshairs();
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

  /* remove watch cusor to indicate open for business */
  XUndefineCursor(disp_id, XtWindow(mainWindow2));

  /* process events until we get a valid input event in the drawing area */
  bell();
  igw_id = 0;
  drawcrosshairs();
  while (igw_id == 0) XtAppProcessEvent(app_context, XtIMAll);

  /* set cusor to watch to indicate busy */
  XDefineCursor(disp_id, XtWindow(mainWindow2), watchCursor);

  /* restore incoming addresses and return values */
  callCount--;
  xout = ixp[callCount];
  yout = iyp[callCount];
  cout = icp[callCount];
  wout = iwp[callCount];
  strcpy(cout, cin);
  *xout = xin;
  *yout = yin;
  *wout = igw_id;
  igw_id = 0;
  removecrosshairs();
} /* retic3 */
