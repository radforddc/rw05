/* extension of minig library to create and use two
   graphics windows rather than just one.
   The first will generally be used to display a level scheme,
   and the second will generally be used to display spectra.

   D.C. Radford    July 1999 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

/* global data */
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
extern Window    root_win_id, win_id;
extern GC        gc_id, gc_comp_id;
extern XEvent    event;
extern int   win_width, win_height;
extern int   menu_mode;
extern int   iMoveFactor;       /* Cursor keys move factor. */
extern int   nstored;
extern short points[512][2];
extern int   color[20], ixp, iyp;
extern float xwg[8];
extern FILE  *hcscrfile;
extern int   hclog;

Window giWinId[2];       /* Global window IDs. */

#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"
#define D_LEVEL_WIN_NAME "Radware: Level Scheme  Window"
#define D_SPECTRA_WIN_NAME "Radware: Histogram Window"

extern int  bell(void);
extern int  cvxy(float *, float *, int *, int *, int);
extern int  retic(float *, float *, char *);
extern int  define_color(char *);
extern void setup_read(int);
extern void set_up_cursor(void);
extern void done_cursor(void);
extern void move_gw_pointer(int *, int *);
extern int  pan_gls(float, float, int);
int doTerminal_io(int *, int *, char *);
int doXWindow_io(int *, int *, char *);
int select_gw(int);
int report_curs3(int ix, int iy, int iwin);

/* ======================================================================= */
int initg0(int n, int *nx, int *ny)
{
  XSetWindowAttributes setwinattr;
  XGCValues xgcvl;
  static int    first = 1, nsave;
  Window        focus, junk_id;
  int           revert_to, ix, iy;
  unsigned int  w, h, junk_border, junk_depth;
  unsigned long valuemask;
  Status        istat;
  Time          time;

  int                 font, black, white, ix1, iy1, scr_width, scr_height;
  int                 igw;
  static int          depth;        /* number of planes */
  static Visual       *visual;      /* visual type */
  static XSizeHints   win_position; /* position and size for window manager */

  nsave = n;
  if (first) {
    first = 0;
    /* open scratch file for storage of data for possible hardcopy */
    if (!(hcscrfile = tmpfile())) {
      printf("ERROR - cannot open temp file!\n");
      hclog = -1;
    }
    /* create new graphics windows on first entry
       initialize display id and screen id */
    disp_id = XOpenDisplay(0);
    if (!disp_id) {
      printf("Display not opened!\n");
      exit(-1);
    }
    /* Next instruction for debugging only */
    /* XSynchronize(disp_id, 1); */

    /* Initialize some global variables */
    screen_id   = XDefaultScreenOfDisplay(disp_id);
    root_win_id = XRootWindowOfScreen(screen_id);
    black       = BlackPixelOfScreen(screen_id);
    white       = WhitePixelOfScreen(screen_id);
    scr_width   = XWidthOfScreen(screen_id);
    scr_height  = XHeightOfScreen(screen_id);
    depth       = XDefaultDepthOfScreen(screen_id);
    visual      = XDefaultVisualOfScreen(screen_id);

    /* Set up backing store */
    valuemask = CWBitGravity | CWBackingStore | CWBackPixel;
    setwinattr.bit_gravity = SouthWestGravity;
    setwinattr.backing_store = Always;
    setwinattr.background_pixel = white;

    if (xwg[2] == 0.0) {
      xwg[0] = 0.03;
      xwg[1] = 0.57;
      xwg[2] = 0.95;
      xwg[3] = 0.40;
      xwg[4] = 0.01;
      xwg[5] = 0.03;
      xwg[6] = 0.50;
      xwg[7] = 0.55;
    }

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

    /* Save ID of window that has input focus */
    XGetInputFocus(disp_id, &focus, &revert_to);

    /* Create two windows. */
    for (igw = 1; igw <= 2; igw++) {
      select_gw(igw);
      ix = xwg[4*(igw-1)+0]*(float)scr_width + 0.5f;
      iy = xwg[4*(igw-1)+1]*(float)scr_height + 0.5f;
      w  = xwg[4*(igw-1)+2]*(float)scr_width + 0.5f;
      h  = xwg[4*(igw-1)+3]*(float)scr_height + 0.5f;

      /* Create the window....
	 Add a pixel to x, y so that later,
	 when we move it, we always get a notify event */
      win_id = XCreateWindow(disp_id, root_win_id, ix+1, iy-1, w, h,
			     0, depth, InputOutput, visual, valuemask, &setwinattr);

      /* Window Manager Hints */
      win_position.x = ix + 1;
      win_position.y = iy - 1;
      win_position.width  = w;
      win_position.height = h;
      win_position.flags  = USPosition|USSize;
      XSetWMNormalHints(disp_id, win_id, &win_position); 
      if (igw == 1) {
	XStoreName(disp_id, win_id, D_SPECTRA_WIN_NAME);
      } else {
	XStoreName(disp_id, win_id, D_LEVEL_WIN_NAME);
      }

      /* Store window Ids in global vars */
      giWinId[igw-1] = win_id;

      /* Create graphics context */
      xgcvl.background = color[1];
      xgcvl.foreground = color[2];
      gc_id = XCreateGC(disp_id, win_id, GCForeground | GCBackground, &xgcvl);
      xgcvl.function = GXinvert;
      gc_comp_id = XCreateGC(disp_id, win_id, GCFunction, &xgcvl);

      /* Load the font for text writing */
      font = XLoadFont(disp_id, FONTNAME);
      XSetFont(disp_id, gc_id, font);

      /* Map the window */
      XSelectInput(disp_id,win_id,ExposureMask|VisibilityChangeMask);
      XMapRaised(disp_id,win_id);
      /* Wait for the window to be raised. Some X servers do not generate
       * an initial expose event, so also check the visibility event. */
      XMaskEvent(disp_id,ExposureMask|VisibilityChangeMask,&event);

      /* move window to where I think it should be and wait for notify event */
      XSelectInput(disp_id,win_id,StructureNotifyMask);
      XMoveWindow(disp_id,win_id,ix,iy);
      XMaskEvent(disp_id,StructureNotifyMask,&event);
      XSelectInput(disp_id,win_id,0);

      /* check position of window, if it's wrong move it again */
      XTranslateCoordinates(disp_id, win_id, root_win_id,
			    0, 0, &ix1, &iy1, &junk_id);
      if (ix1 != ix || iy1 != iy) XMoveWindow(disp_id, win_id, 2*ix-ix1, 2*iy-iy1);

      XSync(disp_id,1);
    }
    /* Restore input focus to original window */
    time = CurrentTime;
    XSetInputFocus(disp_id, focus, revert_to, time);
  }
  select_gw(nsave);
  istat = XGetGeometry(disp_id, win_id, &junk_id, &ix, &iy,
		       &w, &h, &junk_border, &junk_depth);
  win_width  = (int) w;
  win_height = (int) h;
  *nx = win_width - 5;
  *ny = win_height - 25;
  return 0;
} /* initg0 */

int initg(int *nx, int *ny)
{
  return initg0(1, nx, ny);
} /* initg */

int initg2(int *nx, int *ny)
{
  return initg0(2, nx, ny);
} /* initg2 */

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
  select_gw(1);
  return 0;
} /* finig */

/* ======================================================================= */
int retic3(float *x, float *y, char *cout, int *win_out)
{
  /* call up cursor
     x,y  returns position w.r.to axes when cursor terminated
     cout returns pressed key, or G for mouse button
     win_out returns 1 or 2 for graphics window number */

  int           i, ixi, iyi, ilx, ily, validRtnString = 0;
  static int    iwin, iflag;
  static int    iFdDisplay = 0, first = 1;
  char          lstring[40];
  static fd_set ReadFds;
  fd_set        chkReadFds;
#ifdef VMS
  struct timeval tv;
#endif

  /* ring bell */
  if (strncmp(cout, "NOBELL", 6)) bell();

 START:
  for (iwin = 1; iwin <= 2; ++iwin) {
    select_gw(iwin);
    set_up_cursor();
  }
  /* wait for mouse button or text window keyboard button to be pressed
     check to see if mouse button has been pressed */
  if (first) {
    first = 0;
    /* Set-up bit mask for i/o selecting. */
    FD_ZERO(&ReadFds);
    iFdDisplay = ConnectionNumber(disp_id);
    FD_SET(iFdDisplay, &ReadFds);
    FD_SET(STDIN_FILENO, &ReadFds);
  }

#ifdef VMS
  while (1) {
    chkReadFds = ReadFds;
    /* set timeout for select to 0.01 second */
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    /* Wait for data on xterminal or graphics window. */
    if (select(iFdDisplay + 1, &chkReadFds, NULL, NULL, &tv) >= 0) {
      /* Determine which channel was triggered and process event. */
      if (doTerminal_io(&ilx, &ily, lstring) ||
	  doXWindow_io(&ilx, &ily, lstring)) break;
    }
  }
#else
  while (!validRtnString) {
    chkReadFds = ReadFds;
    /* Wait for data on xterminal or graphics window. */
    if (select(iFdDisplay + 1, &chkReadFds, NULL, NULL, NULL) > 0) {
      /* Determine which channel was triggered and process event. */
      if (FD_ISSET(STDIN_FILENO, &chkReadFds))
	validRtnString = doTerminal_io(&ilx, &ily, lstring);
      if (FD_ISSET(iFdDisplay, &chkReadFds))
	validRtnString = doXWindow_io(&ilx, &ily, lstring);
    }
  }
#endif

  *win_out = 1;
  if (win_id == giWinId[1]) *win_out = 2;
  ixi = ilx;
  iyi = ily;
  strcpy(cout, lstring);
  /* remove mouse input events */
  for (iwin = 1; iwin <= 2; ++iwin) {
    select_gw(iwin);
    done_cursor();
  }
  select_gw(*win_out);
  /* convert coordinates and exit */
  *x = 0.f;
  *y = 0.f;
  if (ixi >= 0 && ixi <= win_width &&
      iyi >= 0 && iyi <= win_height) {
    i = win_height - iyi;
    cvxy(x, y, &ixi, &i, 2);
  }
  select_gw(1);
  if (!strncmp(cout + 2, "-S", 2) && *win_out == 2) {
    /* shift-button pressed; pan/zoom gls display */
    iflag = 1;
    if (!strncmp(cout, "X2", 2)) iflag = 2;
    if (!strncmp(cout, "X3", 2)) iflag = 3;
    pan_gls(*x, *y, iflag);
    validRtnString = 0;
    goto START;
  }
  return 0;
} /* retic3 */

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
  int i;
  GC  j;

  /* call dbuf to dump stored plot array
     before changing attribute block */
  dbuf();

  if (icol == -1) {
    j = gc_id;
    gc_id = gc_comp_id;
    gc_comp_id = j;
    return 0;
  }

  i = icol + 1;
  if (i < 1 || i > 16) return 1;
  XSetForeground(disp_id, gc_id, color[i]);

  /* log info to temp file for possible later hardcopy */
  if (hclog > 0) {
    fwrite("c", 1, 1, hcscrfile);
    fwrite(&icol, 4, 1, hcscrfile);
  }
  return 0;
} /* setcolor */

/* ======================================================================= */
int retic2(float *x, float *y, char *cout)
{
  int iflag;

 START:
  select_gw(2);
  retic(x, y, cout);
  select_gw(1);
  if (!strncmp(cout + 2, "-S", 2)) {
    /* shift-button pressed; pan/zoom gls display */
    iflag = 1;
    if (!strncmp(cout, "X2", 2)) iflag = 2;
    if (!strncmp(cout, "X3", 2)) iflag = 3;
    pan_gls(*x, *y, iflag);
    strncpy(cout, "NOBELL", 6);
    goto START;
  }
  return 0;
} /* retic2 */

/* ======================================================================= */
int set_focus(int focus_id)
{
  int    revert_to;
  Window current_focus;

  /* set input focus */
  XGetInputFocus(disp_id, &current_focus, &revert_to);
  if (current_focus == giWinId[0] || current_focus == giWinId[1] ||
      (menu_mode && current_focus != focus_id)) {
    XFlush(disp_id);
    XSetInputFocus(disp_id, (Window) focus_id, revert_to, CurrentTime);
    XFlush(disp_id);
  }
  return 0;
} /* set_focus */

/* ======================================================================= */
int set_xwg(char *in)
{
  /* unchanged from libs/minig/minig.c? */
  FILE   *fp;
  char   filename[80], line[120];

  xwg[2] = 0;
  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xwg");
#else
  strcat (filename, "/.radware.xwg");
#endif
  if (!(fp = fopen(filename, "r+"))) return 1;

  while (fgets(line, 120, fp)) {
    if (!strncmp(line,in,8)) {
      sscanf((char *) strchr(line,' '), " %f%f%f%f%f%f%f%f",
	     &xwg[0], &xwg[1], &xwg[2], &xwg[3],
	     &xwg[4], &xwg[5], &xwg[6], &xwg[7]);
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
  int          ix, iy, scr_width, scr_height;
  unsigned int w, h, junk_border, junk_depth;
  Window       junk_id;

  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xwg");
#else
  strcat (filename, "/.radware.xwg");
#endif
 if (!(fp = fopen(filename, "a+"))) {
    printf("Cannot open file %s\n", filename);
    return 1;
  }
  rewind(fp);

  scr_width =   XWidthOfScreen(screen_id);
  scr_height =  XHeightOfScreen(screen_id);
  XGetGeometry(disp_id, giWinId[0], &junk_id, &ix, &iy,
	       &w, &h, &junk_border, &junk_depth);
  XTranslateCoordinates(disp_id, giWinId[0], root_win_id,
			0, 0, &ix, &iy, &junk_id);
  xwg[0] = (float)ix/(float)scr_width;
  xwg[1] = (float)iy/(float)scr_height;
  xwg[2] = (float)w/(float)scr_width;
  xwg[3] = (float)h/(float)scr_height;

  XGetGeometry(disp_id,giWinId[1],&junk_id,&ix,&iy,
	       &w, &h, &junk_border, &junk_depth);
  XTranslateCoordinates(disp_id, giWinId[1], root_win_id,
			0, 0, &ix, &iy, &junk_id);
  xwg[4] = (float)ix/(float)scr_width;
  xwg[5] = (float)iy/(float)scr_height;
  xwg[6] = (float)w/(float)scr_width;
  xwg[7] = (float)h/(float)scr_height;

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
    if (!strncmp(line[i],in,8)) {
      done = 1;
      if (line[i][8] == ' ')
	sprintf(&line[i][8],
		" %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f\n",
		xwg[0],xwg[1],xwg[2],xwg[3],xwg[4],xwg[5],xwg[6],xwg[7]);
    }
    fprintf(fp, "%s", line[i]);
  }
  if (done == 0) {
    strncpy(line[0], in, 8);
    line[0][8] = '\0';
    fprintf(fp, "%s %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f %7.5f\n",
            line[0], xwg[0],xwg[1],xwg[2],xwg[3],xwg[4],xwg[5],xwg[6],xwg[7]);
  }
  fclose(fp);
  return 0;
} /* save_xwg */

/*--------------------------------------------------------*/
int doXWindow_io(int *ix, int *iy, char *outstr)
{
  KeySym xtKeysym;
  int    i, ixMove, iyMove, igw, goodString = 0;
  char   *cbuf;

  while (!goodString &&
	 (XCheckWindowEvent(disp_id, giWinId[0],
			    PointerMotionMask | KeyPressMask | ButtonPressMask,
			    &event) ||
	  XCheckWindowEvent(disp_id, giWinId[1],
			    PointerMotionMask | KeyPressMask | ButtonPressMask,
			    &event))) {

    if (event.type == MotionNotify) {
      igw = 1;
      if (win_id == giWinId[0]) igw = 2;
      if (event.xmotion.window != win_id) select_gw(igw);

      /* Remove cross hairs (complement). */
      XDrawLine(disp_id,event.xmotion.window,gc_comp_id,
		ixp,0,ixp,win_height);
      XDrawLine(disp_id,event.xmotion.window,gc_comp_id,
		0,iyp,win_width,iyp);

      /* Draw cross hairs (complement). */
      ixp = event.xmotion.x;
      iyp = event.xmotion.y;
      XDrawLine(disp_id,event.xmotion.window,gc_comp_id,
		ixp,0,ixp,win_height);
      XDrawLine(disp_id,event.xmotion.window,gc_comp_id,
		0,iyp,win_width,iyp);
      i = win_height - iyp;
      report_curs3(ixp, i, igw);

    } else  if (event.type == KeyPress) {
      if (event.xkey.window != win_id) {
	igw = 1;
	if (win_id == giWinId[0]) igw = 2;
	select_gw(igw);
      }
      xtKeysym = XLookupKeysym((XKeyEvent*) &event, 0);
      /* Check arrow keys... */
      if (xtKeysym == XK_Up   ||
	  xtKeysym == XK_Down ||
	  xtKeysym == XK_Left ||
	  xtKeysym == XK_Right ) {
	ixMove = 0; iyMove = 0;
	if (xtKeysym == XK_Up) {
	  iyMove = -1;
	} else if (xtKeysym == XK_Down) {
	  iyMove =  1;
	} else if (xtKeysym == XK_Right) {
	  ixMove =  1;
	} else if (xtKeysym == XK_Left) {
	  ixMove = -1;
	}
	ixMove *= iMoveFactor;
	iyMove *= iMoveFactor;
	if (event.xkey.window != win_id) {
	  igw = 1;
	  if (event.xkey.window == giWinId[1]) igw = 2;
	  select_gw(igw);
	}
	move_gw_pointer(&ixMove, &iyMove);
      } else if (xtKeysym == XK_Tab) {
	if (iMoveFactor == 1) {
	  iMoveFactor = 10;
	} else  {
	  iMoveFactor = 1;
	}
      } else if (xtKeysym != XK_Shift_L &&
		 xtKeysym != XK_Shift_R &&
		 xtKeysym != XK_Meta_L &&
		 xtKeysym != XK_Meta_R &&
		 xtKeysym != XK_Control_L &&
		 xtKeysym != XK_Control_R &&
		 xtKeysym != XK_Alt_L &&
		 xtKeysym != XK_Alt_R &&
		 xtKeysym != XK_Super_L &&
		 xtKeysym != XK_Super_R &&
		 xtKeysym != XK_Menu &&
		 xtKeysym != XK_Caps_Lock) {
	cbuf = XKeysymToString(xtKeysym);
	if (strcmp(cbuf, "space") == 0 ||
	    strcmp(cbuf, "1"    ) == 0   ) {
	  *outstr = 'G';
	  *(outstr + 1) = '1';
	  *(outstr+2) = (char) NULL;
	} else if (strcmp(cbuf, "2") == 0) {
	  *outstr = 'X';
	  *(outstr + 1) = '2';
	  *(outstr+2) = (char) NULL;
	} else if (strcmp(cbuf, "3") == 0) {
	  *outstr = 'X';
	  *(outstr + 1) = '3';
	  *(outstr + 2) = (char) NULL;
	} else  {
	  strncpy(outstr, cbuf, 38);
	  outstr[39]  = (char) NULL;
	}
	*ix = event.xkey.x;
	*iy = event.xkey.y;
	goodString = 1;
      }

    } else  if (event.type == ButtonPress) {
      if (event.xbutton.window != win_id) {
	igw = 1;
	if (win_id == giWinId[0]) igw = 2;
	select_gw(igw);
      }
      strcpy(outstr, "G1  ");
      if (event.xbutton.button == Button2) strcpy(outstr, "X2  ");
      if (event.xbutton.button == Button3) strcpy(outstr, "X3  ");
      /* shift-mousebutton -> pan/zoom gls display, for example */
      if ((event.xbutton.state & ShiftMask)) {
	*(outstr+2) = '-';
	*(outstr+3) = 'S';
      }
      *(outstr+4) = (char) NULL;
      *ix = (int) event.xbutton.x;
      *iy = (int) event.xbutton.y;
      goodString = 1;
    }
  }
  return goodString;
} /* doXWindow_io */

/*--------------------------------------------------------*/
int doTerminal_io(int *ix, int *iy, char *outstr)
{
  Window       jroot, jchild;
  int          ix_root, iy_root, x, y, ixdelta, iydelta, j;
  unsigned int jmask;
  char         c1;

#ifdef VMS
  extern char getchar_vms(int echoflag);

  if (!(c1 = getchar_vms(0))) return 0;
#else
  if (!read(0, &c1, 1)) return 0;
#endif

  if (c1 == 9) {
    if (iMoveFactor == 1)
      iMoveFactor = 10;
    else
      iMoveFactor = 1;
  } else if (c1 == 27) {
    /* Should set to non-blocking */
#ifdef VMS
    c1 = getchar_vms(0);
#else
    read(0, &c1, 1);
#endif
    if (c1 == '[') {
#ifdef VMS
      c1 = getchar_vms(0);
#else
      read(0, &c1, 1);
#endif
      if (c1 == 'A' ||
	  c1 == 'B' ||
	  c1 == 'C' ||
	  c1 == 'D' ) {
	ixdelta = 0; iydelta = 0;       
	switch (c1) {
	case 'A':
	  iydelta = -1;
	  break;
	case 'B':
	  iydelta =  1;
	  break;
	case 'C':
	  ixdelta =  1;
	  break;
	case 'D':
	  ixdelta = -1;
	  break;
	}
	ixdelta *= iMoveFactor;
	iydelta *= iMoveFactor;
	move_gw_pointer(&ixdelta, &iydelta);
      }
    }
  } else {
    for (j=1; j<=2; j++) {
      select_gw(j);
      if (XQueryPointer(disp_id, win_id, &jroot, &jchild,
			&ix_root, &iy_root, &x, &y, &jmask) &&
	  x >= 0 && x <= win_width &&
	  y >= 0 && y <= win_height) {
	*ix = (int) x;
	*iy = (int) y;
	*outstr = c1;
	*(outstr+1) = (char) NULL;
	return 1;
      }
    }
  }
  return 0;
} /* doTerminal_io */
