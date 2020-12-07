/* C version of minig_x fortran subroutines
   version 0.0   D.C. Radford    July 1999 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "minig.h"

extern Display   *disp_id;
extern Screen    *screen_id;
extern Window    root_win_id, win_id;
extern GC        gc_id, gc_comp_id;
extern XEvent    event;
extern int   win_width, win_height;
extern int   iMoveFactor;       /* Cursor keys move factor. */
extern int   nstored;
extern short points[512][2];
extern int   color[20], ixp, iyp;
extern float xwg[8];
extern FILE  *hcscrfile;
extern int   hclog;

#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"
#define WINDNAME "Graphics Window"

extern int  define_color(char *);
extern void move_gw_pointer(int *, int *);
int doTerminal_io(int *, int *, char *);
int doXWindow_io(int *, int *, char *);
int report_curs(int ix, int iy);

/* ======================================================================= */
int initg(int *nx, int *ny)
{
  XSetWindowAttributes setwinattr;
  XGCValues xgcvl;
  static int    first = 1;
  Window        focus, junk_id;
  int           revert_to, ix, iy;
  unsigned int  w, h, junk_border, junk_depth;
  unsigned long valuemask;
  Time          time;

  int                font, black, white, ix1, iy1, scr_width, scr_height;
  static int         depth;        /* number of planes */
  static Visual     *visual;       /* visual type */
  static XSizeHints  win_position; /* position and size for window manager */

  if (first) {
    first = 0;
    /* open scratch file for storage of data for possible hardcopy */
    hclog = 1;
    if (!(hcscrfile = tmpfile())) {
      printf("ERROR - cannot open temp file!\n");
      hclog = 0;
    }
    /* create new graphics window on first entry
       Initialize display id and screen id */
    disp_id = XOpenDisplay(0);
    if (!disp_id) {
      printf("Display not opened!\n");
      exit(-1);
    }
    /* Next instruction for debugging only */
    /* XSynchronize(disp_id, 1); */
  
    /* Initialize some global variables */
    screen_id =   XDefaultScreenOfDisplay(disp_id);
    root_win_id = XRootWindowOfScreen(screen_id);
    black =       XBlackPixelOfScreen(screen_id);
    white =       XWhitePixelOfScreen(screen_id);
    scr_width =   XWidthOfScreen(screen_id);
    scr_height =  XHeightOfScreen(screen_id);
    depth  =      XDefaultDepthOfScreen(screen_id);
    visual =      XDefaultVisualOfScreen(screen_id);

    /* Set up backing store */
    valuemask = CWBitGravity | CWBackingStore | CWBackPixel;
    setwinattr.bit_gravity = SouthWestGravity;
    setwinattr.backing_store = Always;
    setwinattr.background_pixel = white;

    if (xwg[2] == 0.0) {
      xwg[0] = 0.03;
      xwg[1] = 0.33;
      xwg[2] = 0.95;
      xwg[3] = 0.64;
    }
    ix = xwg[0]*(float)scr_width + 0.5f;
    iy = xwg[1]*(float)scr_height + 0.5f;
    w  = xwg[2]*(float)scr_width + 0.5f;
    h  = xwg[3]*(float)scr_height + 0.5f;

    /* Save ID of window that has input focus */
    XGetInputFocus(disp_id, &focus, &revert_to);

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
    XStoreName(disp_id, win_id, WINDNAME);

    /* Get named color values */
    XFlush(disp_id);
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

    /* Create graphics context */
    xgcvl.background = color[1];
    xgcvl.foreground = color[2];
    gc_id = XCreateGC(disp_id, win_id, GCForeground | GCBackground, &xgcvl);

    /* Create GC for complement drawing. */
    /* There are several ways of doing this. My way may not be the best one. RWM */
    xgcvl.function = GXinvert;
    /* xgcvl.foreground = 1;
       xgcvl.function = GXxor; */
    gc_comp_id = XCreateGC(disp_id, win_id, GCForeground | GCFunction, &xgcvl);

    /* Load the font for text writing */
    font = XLoadFont(disp_id, FONTNAME);
    XSetFont(disp_id, gc_id, font);

    /* Map the window */
    XSelectInput(disp_id,win_id,ExposureMask|VisibilityChangeMask);
    XMapRaised(disp_id,win_id);
    /* Wait for the window to be raised. Some X servers do not generate
       an initial expose event, so also check the visibility event. */
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

    /* Restore input focus to original window */
    time = CurrentTime;
    XSetInputFocus(disp_id, focus, revert_to, time);
  }
  (void) XGetGeometry(disp_id, win_id, &junk_id, &ix, &iy,
		       &w, &h, &junk_border, &junk_depth);
  win_width  = (int) w;
  win_height = (int) h;
  *nx = win_width  - 5;
  *ny = win_height - 25;
  return 0;
} /* initg */

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
  return 0;
} /* finig */

/*--------------------------------------------------------*/
int doTerminal_io(int *ix, int *iy, char *outstr)
{
  Window       jroot, jchild;
  int          ix_root, iy_root, x, y, ixdelta, iydelta;
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
  } else if (XQueryPointer(disp_id, win_id, &jroot, &jchild,
		      &ix_root, &iy_root, &x, &y, &jmask)) {
    *ix = (int) x;
    *iy = (int) y;
    *outstr = c1;
    *(outstr+1) = 0;
    return 1;
  }
  return 0;
} /* doTerminal_io */

/*--------------------------------------------------------*/
int doXWindow_io(int *ix, int *iy, char *outstr)
{
  KeySym xtKeysym;
  int    i, ixMove, iyMove, goodString = 0;
  char   *cbuf;

  while (!goodString &&
	 XCheckWindowEvent(disp_id, win_id,
			   (PointerMotionMask | KeyPressMask | ButtonPressMask),
			   &event)) {

    if (event.type == MotionNotify) {
      /* remove cross hairs (complement) */
      XDrawLine(disp_id, win_id, gc_comp_id, ixp, 0, ixp, win_height);
      XDrawLine(disp_id, win_id, gc_comp_id, 0, iyp, win_width, iyp);
      /* draw cross hairs (complement) */
      ixp = event.xmotion.x;
      iyp = event.xmotion.y;
      XDrawLine(disp_id, win_id, gc_comp_id, ixp, 0, ixp, win_height);
      XDrawLine(disp_id, win_id, gc_comp_id, 0, iyp, win_width, iyp);
      i = win_height - iyp;
      report_curs(ixp, i);

    } else  if (event.type == KeyPress) {
      xtKeysym = XLookupKeysym((XKeyEvent*) &event, 0);
      /* check arrow keys */
      if (xtKeysym == XK_Up ||
	  xtKeysym == XK_Down ||
	  xtKeysym == XK_Left ||
	  xtKeysym == XK_Right ) {
	ixMove = 0; iyMove = 0;
	if (xtKeysym == XK_Up) {
	  iyMove = -1;
	}
	else if (xtKeysym == XK_Down) {
	  iyMove =  1;
	}
	else if (xtKeysym == XK_Right) {
	  ixMove =  1;
	}
	else if (xtKeysym == XK_Left) {
	  ixMove = -1;
	}
	ixMove *= iMoveFactor;
	iyMove *= iMoveFactor;
	move_gw_pointer(&ixMove, &iyMove);
      }
      else if (xtKeysym == XK_Tab) {
	if (iMoveFactor == 1) {
	  iMoveFactor = 10;
	}
	else {
	  iMoveFactor = 1;
	}
      }
      else if (xtKeysym != XK_Shift_L &&
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
	strncpy(outstr, cbuf, 38);
	outstr[39]  = 0;
	*ix = event.xkey.x;
	*iy = event.xkey.y;
	goodString = 1;
      }

    } else  if (event.type == ButtonPress) {
      strcpy(outstr, "G   ");
      if (event.xbutton.button == Button2) strcpy(outstr, "X2  ");
      if (event.xbutton.button == Button3) strcpy(outstr, "X3  ");
      /* shift-mousebutton -> pan/zoom gls display, for example */
      if ((event.xbutton.state & ShiftMask)) {
	*(outstr+2) = '-';
	*(outstr+3) = 'S';
      }
      *(outstr+4) = 0;
      *ix = (int) event.xbutton.x;
      *iy = (int) event.xbutton.y;
      goodString = 1;
    }
  }

  return goodString;
} /* doXWindow_io */

/* ======================================================================= */
int set_xwg(char *in)
{
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
    if (!strncmp(line, in, 8)) {
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
  XGetGeometry(disp_id, win_id, &junk_id, &ix, &iy,
	       &w, &h, &junk_border, &junk_depth);
  XTranslateCoordinates(disp_id, win_id, root_win_id,
			0, 0, &ix,& iy, &junk_id);
  xwg[0] = (float)ix/(float)scr_width;
  xwg[1] = (float)iy/(float)scr_height;
  xwg[2] = (float)w/(float)scr_width;
  xwg[3] = (float)h/(float)scr_height;

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
	sprintf(&line[i][8], " %7.5f %7.5f %7.5f %7.5f\n",
		xwg[0], xwg[1], xwg[2], xwg[3]);
    }
    fprintf(fp, "%s", line[i]);
  }
  if (!done) {
    strncpy(line[0], in, 8);
    line[0][8] = '\0';
    fprintf(fp, "%s %7.5f %7.5f %7.5f %7.5f\n",
	    line[0], xwg[0], xwg[1], xwg[2], xwg[3]);
  }
  fclose(fp);
  return 0;
} /* save_xwg */

/* ======================================================================= */
int setcolor(int icol)
{
  int i;

  i = icol + 1;
  if (i < 1 || i > 16) return 1;

  /* call finig to dump stored plot array
     before changing attribute block */
  finig();
  XSetForeground(disp_id, gc_id, color[i]);

  /* log info to temp file for possible later hardcopy */
  if (hclog) {
    fwrite("c", 1, 1, hcscrfile);
    fwrite(&icol, 4, 1, hcscrfile);
  }
  return 0;
} /* setcolor */
