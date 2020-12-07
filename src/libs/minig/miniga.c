/* C version of minig_x fortran subroutines
   version 0.0   D.C. Radford    July 1999
   last edit     D.C. Radford    Oct  2019
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>

#include "minig.h"

/* global data */
float fdx, fx0, fdy, fy0;
int   idx, ix0, idy, iy0, yflag;
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

Display   *disp_id;
Screen    *screen_id;
Window    root_win_id, win_id;
GC        gc_id, gc_comp_id;
XEvent    event;
int   win_width, win_height, nowatx, nowaty;
int   menu_mode = 0;
int   iMoveFactor = 1;       /* Cursor keys move factor. */
int   nstored = 0;
short points[512][2];
int   color[20], ixp, iyp;
float xwg[8];
FILE  *hcscrfile;
int   hclog = 0;

#define HCL(a,b) fwrite(a,b,1,hcscrfile)
#define R(a,b) (fread(a,b,1,hcscrfile) != 1)
#define ERR { warn("ERROR - cannot read hardcopy data. %c\n", c); return 1; }

/* ======================================================================= */
int bell(void)
{
  /* ring bell */
  char *bellenv;

  bellenv = getenv("RADWARE_CURSOR_BELL");
  if (!bellenv ||
      *bellenv == '1' ||
      *bellenv == 'T' || 
      *bellenv == 't' ||
      *bellenv == 'Y' ||
      *bellenv == 'y' ||
      *bellenv == '/' ) {
    printf("%c",(char) (7));
    fflush(stdout);
  }
  return 0;
} /* bell */

/* ======================================================================= */
int hcopy(void)
{
  static char colorstr[16][16] =
  { "0 0 0", /* black */
    "1 0 0", /* red */
    "0 0 1", /* blue */
    "1 0 1", /* magenta */
    "0 0 .7", /* slate blue */
    "1 .2 .2", /* orange red */
    "0 .8 0", /* forest green */
    ".2 1 .2", /* green yellow */
    ".7 0 0", /* maroon */
    "1 .4 .4", /* coral */
    ".2 .2 1", /* cadet blue */
    "1 .4 .4", /* orange */
    ".2 1 .2", /* pale green */
    ".2 .2 .2",
    ".4 .4 .4" };

  float  scale, r1, r2, red, green, blue;
  int    x = 0, y = 0, i, nc, color, width;
  char   s = 'm', c, dattim[20], text[80], ans[80], fn[80] = "mhc.ps";
  FILE   *psfile;
  Window junk_id;
  unsigned int  w, h, junk_border, junk_depth;

  extern int askfn(char *ans, int mca,
		   const char *default_fn, const char *default_ext,
		   const char *fmt, ...);
  extern int askyn(const char *fmt, ...);
  extern void warn(const char *fmt, ...);
  extern int datetime(char *dattim);
  extern FILE *open_new_file(char *filnam, int force_open);
  extern int pr_and_del_file(char *filnam);
  extern int setext(char *filnam, const char *cext, int filnam_len);

  if (!hcscrfile) return 1;
  HCL("Done",4);
  fflush(hcscrfile);
  rewind(hcscrfile);
  if (R(&c,1) || c == 'D') {
    warn("ERROR - no hardcopy data stored.\n");
    if (c == 'D') fseek(hcscrfile, -1, SEEK_CUR);
    return 1;
  }
  color = askyn("Use color? (Y/N)");
  askfn(ans, 80, fn, ".ps", "Output file name = ? (rtn for %s)", fn);
  strcpy(fn, ans);
  if (!(psfile = open_new_file(fn, 0))) {
    fseek(hcscrfile, -4, SEEK_END);
    return 0;
  }
  /* write prolog to postscript file */
  datetime(dattim);
  fprintf(psfile,
	  "%%!PS-Adobe-1.0\n"
	  "%%%%Creator: minig hardcopy (Author: D.C. Radford)\n"
	  "%%%%Title: minig hardcopy\n"
	  "%%%%CreationDate: %s\n"
	  "%%%%Pages: 1\n"
	  "%%%%BoundingBox: 20 20 590 770\n"
	  "%%%%EndComments\n"
	  "/s {stroke} def\n"
	  "/m {newpath moveto} def\n"
	  "/d {lineto} def\n"
	  "/x {currentpoint exch pop lineto} def\n"
	  "/y {currentpoint pop exch lineto} def\n"
	  "/f {closepath gsave fill grestore stroke} def\n"
	  "%%%%EndProlog\n\n"
	  "%%%%Page: 1 1 save\n",
	  dattim);

  (void) XGetGeometry(disp_id, win_id, &junk_id, &x, &y,
		       &w, &h, &junk_border, &junk_depth);
  h += 60; w += 60;
  if (h > w) {
    r1 = 550.0f / (float) w, r2 = 730.0f / (float) h;
    fprintf(psfile,"30 30 translate\n");
  } else {
    r1 = 730.0f / (float) w, r2 = 550.0f / (float) h;
    fprintf(psfile,"580 30 translate 90 rotate\n");
  }
  scale = (r1 < r2 ? r1 : r2);
  fprintf(psfile,
	  "%.5f %.5f scale 30 30 translate 0.8 setlinewidth\n"
	  "/Helvetica findfont 12 scalefont setfont\n"
	  "-30 -30 m %d x %d y -30 x -30 y s\n 1.6 setlinewidth\n",
	  scale, scale, w-30, h-30);

  fflush(hcscrfile);
  rewind(hcscrfile);
  while ((!R(&c,1)) && c != 'D') {      /* D = Done (finished drawing) */
    if (c == 'c') {                     /* c = set color */
      if (R(&i,4)) ERR;                 /* i = color index */
      if (!color) continue;
      if (i < 1 || i > 15) i = 1;
      if (s != 'm') fprintf(psfile, "s ");
      fprintf(psfile, "%s setrgbcolor\n", colorstr[i-1]);
      s = 'm';
    } else if (c == 'm' || c == 'd') {  /* m = moveto, d = drawto */
      if (R(&x,4) || R(&y,4)) ERR;
      if (s != 'm' && c == 'm') fprintf(psfile, "s ");
      fprintf(psfile, "%d %d %c\n", x, y, c);
      s = c;
    } else if (c == 'x') {              /* x = drawto on x; y unchanged */
      if (R(&x,4)) ERR;
      fprintf(psfile, "%d %c\n", x, c);
      s = c;
    } else if (c == 'y') {              /* y = drawto on y; x unchanged */
      if (R(&y,4)) ERR;
      fprintf(psfile, "%d %c\n", y, c);
      s = c;
    } else if (c == 't') {              /* t = draw text */
      if (s != 'm') fprintf(psfile, "s ");
      if (R(&i,4)) ERR;                 /* nc = kad in putg */
      if (R(&nc,4)) ERR;                /* nc = number of chars */
      if (R(text,nc)) ERR;
      text[nc] = '\0';
      y += 2;
      if (i == 3 || i == 6 || i == 9) y -= 14;
      if (i > 6) {
	x -= nc*7;
      } else if (i > 3) {
	x -= (nc*7)/2;
      }
      fprintf(psfile, "%d %d moveto (%s) show\n", x, y, text);
      s = 'm';
    } else if (c == 'r') {              /* r = set red-green-blue color */
      if (R(&i,4)) ERR;
      if (R(&red,4)) ERR;
      if (R(&green,4)) ERR;
      if (R(&blue,4)) ERR;
      fprintf(psfile, "%.3f %.3f %.3f setrgbcolor\n", red, green, blue);
    } else if (c == 'w') {              /* w = set line width */
      if (R(&width,4)) ERR;
      fprintf(psfile, "%d setlinewidth\n", width);
    } else if (c == 'f') {              /* f = fill path */
      fprintf(psfile, "f\n");
      s = 'm';
    } else {
      ERR;
    }
  }
  if (c == 'D') fseek(hcscrfile, -1, SEEK_CUR);
  if (s != 'm') fprintf(psfile, "s\n");
  fprintf(psfile,
	  "\nshowpage\n"
	  "%%%%Trailer\n%%%%Pages: 1\n");
  fclose(psfile);
  if (askyn("Print and delete the postscript file now? (Y/N)\n"
	    "                (If No, the file will be saved)"))
    pr_and_del_file(fn);
  return 0;
} /* hcopy */

/* ======================================================================= */
int cvxy(float *x, float *y, int *ix, int *iy, int mode)
{
  /* mode = 1: convert x,y to ix,iy  (axes to screen pixels)
     mode = 2: convert ix,iy to x,y  (screen pixels to axes) */

  float a, yc;

  if (mode == 1) {
    /* conversion from axes to virtual display coords */
    if (yflag == 1) {         /* linear y-axis */
      yc = *y - fy0;
    } else if (yflag == 2) {  /* sqrt y-axis */
      yc = 0.f;
      if (*y > 0.f) {
	yc = sqrt(*y) - sqrt(fy0);
      }
    } else {                          /* logarithmic y-axis */
      yc = *y;
      if (yc < fy0) yc = fy0;
      yc = log(yc) - log(fy0);
    }
    *ix = ix0 + rint((*x - fx0) / fdx * (float) idx);
    *iy = iy0 + rint(yc / fdy * (float) idy);
    if (*ix < 0) *ix = 0;
    if (*ix > idx + ix0) *ix = idx + ix0;
    if (*iy < 0) *iy = 0;
    if (*iy > idy + iy0) *iy = idy + iy0;

  } else {
    /* conversion from virtual display to axes coords */
    *x = fx0 + (float) (*ix - ix0) * fdx / (float) idx;
    a = (float) (*iy - iy0) * fdy / (float) idy;
    if (yflag == 1) {         /* linear y-axis */
      *y = fy0 + a;
    } else if (yflag == 2) {  /* sqrt y-axis */
      *y = fy0 + a * (a + sqrt(fy0) * 2.f);
    } else {                          /* logarithmic y-axis */
      *y = fy0 * pow(2.71828, (double) a);
    }
  }

  return 0;
} /* cvxy */

/* ======================================================================= */
int get_focus(int *focus_id)
{
  int    revert_to;
  Window current_focus;

  /* get information about current input focus
     if necessary, first open display */
  if (!disp_id) {
    disp_id = XOpenDisplay(0);
    if (!disp_id) {
      printf("Display not opened!\n");
      exit(-1);
    }
  }

  XGetInputFocus(disp_id, &current_focus, &revert_to);
  *focus_id = (int) current_focus;
  return 0;
} /* get_focus */

/* ======================================================================= */
int grax(float a, float b, float *dex, int *nv, int yflag)
{
  /* define tick-points for marking of axes
      a = low axis value
      b = high axis value
      dex = returned array of tick points
      nv  = number of tick points
      yflag = 1/2/3 for lin/sqrt/log axis */

  float c, x, tx, ctx;
  int   i, j, k;

  if (yflag < 3) {
    /* linear or sqrt axis */
    x = b - a;
    c = 1.0f;
    while (x >= 15.0f) {
      c *= 10.0f;
      x /= 10.0f;
    }
    if (x < 1.5f) {
      tx = 0.1f;
    } else if (x < 3.f) {
      tx = 0.2f;
    } else if (x < 7.5f) {
      tx = 0.5f;
    } else {
      tx = 1.0f;
    }
    ctx = c * tx;
    j = (int) (0.5f + a / ctx);
    c = ctx * (float) j;
    if (c <= a + 0.1f * ctx) c += ctx;
    *nv = 0;
    while (*nv < 14) {
      dex[(*nv)++] = c;
      if ((c += ctx) >= b) return 0;
    }
  } else {
    /* logarithmic axis */
    j = log10(a);
    if (a < 1.0f) j--; 
    k = log10(b - 1.0f);
    c = pow(10.0, (double) j);
    if (k - j < 4) {
      dex[0] = c * 2.f;
      dex[1] = c * 5.f;
      dex[2] = c * 10.f;
      dex[3] = c * 20.f;
      dex[4] = c * 50.f;
      dex[5] = c * 100.f;
      dex[6] = c * 200.f;
      dex[7] = c * 500.f;
      dex[8] = c * 1000.f;
      dex[9] = c * 2000.f;
      dex[10] = c * 5000.f;
      *nv = 11;
    } else {
      *nv = k - j;
      for (i = 0; i < *nv; ++i) {
	c *= 10.f;
	dex[i] = c;
      }
    }
  }
  return 0;
} /* grax */

/*--------------------------------------------------------*/
int define_color(char *color_name)
{
/* Allocate a color
   Input: Pointer to char string: color_name
   Output: Allocated color index or index for BlackPixel
   Side effects: relies on global vars */

  XColor screen_color, exact_color;
  Status istat;

  /* Create color */
  istat = XAllocNamedColor(disp_id,
			   DefaultColormapOfScreen(screen_id),
			   color_name, &screen_color, &exact_color);
  if (istat) 
    return screen_color.pixel;
  else
    return (int) BlackPixelOfScreen(screen_id);
}

/* ======================================================================= */
int ktras(int ix, int iy, int mode)
{
  int newx, newy;
  static int old_mode = -1;

  /* ix, iy  = destination in screen pixels
     mode    = 0/1/2 for move/draw/point */

  newx = ix;
  newy = win_height - iy;
  if (mode != 1 && nstored >= 2) {
    /* empty buffer of stored x,y values */
    XDrawLines(disp_id, win_id, gc_id, (XPoint *)points[0], nstored,
	       CoordModeOrigin);
    nstored = 0;
  }
  if (mode == 2) {
    /* point mode */
    XDrawPoint(disp_id, win_id, gc_id, newx, newy);
  } else if (mode == 1) {
    /* draw mode
       store newx,newy in buffer of stored x,y values */
    if (nstored == 0) {
      points[0][0] = (short) nowatx;
      points[0][1] = (short) nowaty;
      nstored++;
    }
    points[nstored][0] = (short) newx;
    points[nstored][1] = (short) newy;
    nstored++;
    if (nstored == 512) {
      /* empty buffer of stored x,y values */
      XDrawLines(disp_id, win_id, gc_id, (XPoint *)points[0], nstored,
		 CoordModeOrigin);
      nstored = 0;
    }
  }

  /* log info to temp file for possible later hardcopy */
  if (hclog > 0) {
    if (mode == 1) {
      if (nowatx == newx) {
	if (nowaty != newy || old_mode != 1) {
	  HCL("y",1); HCL(&iy, 4);
	}
      } else if (nowaty == newy) {
	HCL("x",1); HCL(&ix, 4);
      } else {
	HCL("d",1); HCL(&ix, 4); HCL(&iy,4);
      }
    } else {
      HCL("m",1); HCL(&ix, 4); HCL(&iy,4);
      if (mode == 2) {
	HCL("x",1); HCL(&ix, 4);
      }
    }
  }

  nowatx = newx;
  nowaty = newy;
  old_mode = mode;
  return 0;
} /* ktras */

/* ======================================================================= */
int limg(int nx, int ix, int ny, int iy)
{
  /* define region of screen to hold display (i.e. screen window) */
  idx = nx;
  ix0 = ix;
  idy = ny;
  iy0 = iy + 20;
  return 0;
} /* limg */

/* ======================================================================= */
int mspot(int nx, int ny)
{
  /* move graphics cursor to (mx,my) in screen pixels */
  return ktras(nx, ny, 0);
} /* mspot */

/* ======================================================================= */
int ivect(int nx, int ny)
{
  /* move graphics cursor to (mx,my) in screen pixels */
  return ktras(nx, ny, 1);
} /* ivect */

/* ======================================================================= */
int pspot(float x, float y)
{
  /* position graphics cursor at (x,y) w.r.to axes */
  int ix, iy;
  cvxy(&x, &y, &ix, &iy, 1);
  return ktras(ix, iy, 0);
} /* pspot */

/* ======================================================================= */
int vect(float x, float y)
{
  /* draw line to (x,y) w.r.to axes */
  int ix, iy;
  cvxy(&x, &y, &ix, &iy, 1);
  return ktras(ix, iy, 1);
} /* vect */

/* ======================================================================= */
int point(float x, float y)
{
  /* draw point at (x,y) w.r.to axes */
  int ix, iy;
  cvxy(&x, &y, &ix, &iy, 1);
  return ktras(ix, iy, 2);
} /* point */

/* ======================================================================= */
int minig_fill(void)
{
  /* empty buffer of stored x,y values */
  if (nstored >= 2) {
    XFillPolygon(disp_id, win_id, gc_id, (XPoint *)points[0], nstored,
		 Convex, CoordModeOrigin);
    XDrawLines(disp_id, win_id, gc_id, (XPoint *)points[0], nstored,
	       CoordModeOrigin);
    nstored = 0;
  }
  XFlush(disp_id);
  if (hclog) fwrite("f", 1, 1, hcscrfile);
  return 0;
} /* minig_fill */

/* ======================================================================= */
int putg(char *text, int nc, int kad)
{
  /* write nc characters from text to terminal as graphtext
     kad = 1-3/4-6/7-9 for text to the right/centered/to the left
          of the current graphics cursor position
     kad = 3,6 or 9 for text below the current position */
  int ix, iy, nchars;

  nchars = abs(nc);
  if (nchars == 0) return 0;

  ix = nowatx;
  iy = nowaty - 2;
  if (kad == 3 || kad == 6 || kad == 9) iy += 15;
  if (kad > 6) {
    ix -= nchars*6;
  } else if (kad > 3) {
    ix -= nchars*3;
  }
  if (ix < 0) ix = 0;
  if (iy < 0) iy = 0;
  XDrawString(disp_id, win_id, gc_id, ix, iy, text, nchars);

  /* log info to temp file for possible later hardcopy */
  if (hclog > 0) {
    HCL("t",1); HCL(&kad,4); HCL(&nchars,4); HCL(text,nchars);
  }
  return 0;
} /* putg */

/* ======================================================================= */
int trax(float tx, float xi, float ty, float yi, int k)
{
  /* draw axes in graphics window
     x-axis from xi to xi+tx
     y-axis from yi to yi+ty
     k = 1/2/3 for lin/sqrt/log y-axis on left hand side
     k = -1/-2/-3 for lin/sqrt/log y-axis on right hand side
     k = 0 for linear y-axis, no axes drawn
     k = 4 for log y-axis, no axes drawn */

  float v, x, x1, x2, y1, y2, dex[14];
  int   i, n, nc, ix, nv, iy;
  char  bc[12];

  fdx = tx;
  fx0 = xi;
  fy0 = yi;
  yflag = abs(k);
  if (yflag > 3) yflag = 3;
  if (k == 0) yflag = 1;
  if (yflag == 1) {
    fdy = ty;
  } else if (yflag == 2) {
    fdy = sqrt(fy0 + ty) - sqrt(fy0);
  } else {
    if (fy0 <= 0.0f) fy0 = 0.5f;
    fdy = log(fy0 + ty) - log(fy0);
  }
  if (k == 0 || k > 3) return 0;
  x1 = fx0;
  x2 = fx0 + fdx;
  y1 = fy0;
  y2 = fy0 + ty;

  /* draw x-axis */
  pspot(x1, y1);
  vect(x2, y1);
  grax(x1, x2, dex, &nv, 1);
  for (n = 1; n <= nv; ++n) {
    v = dex[n - 1];
    if (v != x1) {
      cvxy(&v, &y1, &ix, &iy, 1);
      mspot(ix, iy+3);
      ivect(ix, iy-3);
      if ((n/2) << 1 == n) {
	/* write value in graphtext */
	if (tx <= 15.f) {
	  sprintf(bc, "%7.2f ", v);
	} else {
	  sprintf(bc, "%8.0f", v);
	}
	i = 0;
	while (i < 7 && bc[i] == ' ') i++;
	nc = 8 - i;
	putg(bc+i, nc, 6);
      }
    }
  }
  /* draw y-axis */
  x = x1;
  if (k < 0) x = x2;
  pspot(x, y1);
  vect(x, y2);
  grax(y1, y2, dex, &nv, yflag);
  for (n = 1; n <= nv; ++n) {
    v = dex[n - 1];
    if (v > y1 && v < y2) {
      cvxy(&x, &v, &ix, &iy, 1);
      mspot(ix+4, iy);
      ivect(ix-4, iy);
      if (yflag == 3 || (n + 1) / 4 << 2 == n + 1) {
	/* write value in graphtext */
	if (ty <= 15.f) {
	  sprintf(bc, "%7.2f ", v);
	} else {
	  sprintf(bc, "%8.0f", v);
	}
	i = 0;
	while (i < 7 && bc[i] == ' ') i++;
	nc = 8 - i;
	if (k < 0) {
	  mspot(ix-6, iy-7);
	  putg(bc+i, nc, 8);
	} else {
	  mspot(ix+6, iy-7);
	  putg(bc+i, nc, 2);
	}
      }
    }
  }
  return 0;
} /* trax */

/* ======================================================================= */
int symbg(int symbol, float x, float y, int size)
{
  /* put a graphics symbol at (x,y) w.r.to the axes
     size = size of symbol in graphics pixels
     symbol = 1/2       for circle/filled circle
     symbol = 3/4       for square/filled square
     symbol = 5/6       for diamond/filled diamond
     symbol = 7/8       for up triangle/filled up triangle
     symbol = 9/10      for down triangle/filled down triangle
     symbol = 11/12/13  for + / x / *
   */

  float s;
  int   j, k, n, il, jl, kl, ml, nl, ix, iy;

  if (symbol <= 0 || size <= 1) return 0;

  s = (float) size;
  ml = (int) (s*0.5f + 0.5f);
  nl = (int) (s*0.4f + 0.5f);
  jl = (int) (s*0.6f + 0.5f);
  cvxy(&x, &y, &ix, &iy, 1);

  switch (symbol) {
  case 1:  /* circle */
    for (n = -1; n <= 1; n += 2) {
      mspot(ix-ml, iy);
      for (j = ix-ml+1; j <= ix+ml; ++j) {
	k = sqrt((float) (ml*ml - (ix-j)*(ix-j))) + 0.5f;
	ivect(j, iy + k*n);
      }
    }
    break;
  case 2:  /* filled circle */
    for (j = ix-ml; j <= ix+ml; ++j) {
      k = sqrt((float) (ml * ml-(ix-j) * (ix-j)))+.5f;
      mspot(j, iy-k);
      ivect(j, iy+k);
    }
    break;
  case 3:  /* square */
    mspot(ix-nl, iy-nl);
    ivect(ix-nl, iy+nl);
    ivect(ix+nl, iy+nl);
    ivect(ix+nl, iy-nl);
    ivect(ix-nl, iy-nl);
    break;
  case 4:  /* filled square */
    for (j = iy-nl; j <= iy+nl; ++j) {
      mspot(ix-nl, j);
      ivect(ix+nl, j);
    }
    break;
  case 5:  /* diamond */
    mspot(ix-ml, iy);
    ivect(ix,    iy-ml);
    ivect(ix+ml, iy);
    ivect(ix,    iy+ml);
    ivect(ix-ml, iy);
    break;
  case 6:  /* filled diamond */
    for (j = 0; j <= ml; ++j) {
      mspot(ix-j, iy-ml+j);
      ivect(ix+j, iy-ml+j);
      mspot(ix-j, iy+ml-j);
      ivect(ix+j, iy+ml-j);
    }
    break;
  case 7:  /* upwards pointing triangle */
    mspot(ix-ml, iy-nl);
    ivect(ix+ml, iy-nl);
    ivect(ix,    iy+jl);
    ivect(ix-ml, iy-nl);
    break;
  case 8:  /* filled upwards pointing triangle */
    kl = nl + jl;
    for (j = 0; j <= kl; ++j) {
      il = (ml*(kl-j) + kl/2) / kl;
      mspot(ix-il, iy-nl+j);
      ivect(ix+il, iy-nl+j);
    }
    break;
  case 9:  /* downwards pointing triangle */
    mspot(ix-ml, iy+nl);
    ivect(ix+ml, iy+nl);
    ivect(ix,    iy-jl);
    ivect(ix-ml, iy+nl);
    break;
  case 10:  /* filled downwards pointing triangle */
    kl = nl + jl;
    for (j = 0; j <= kl; ++j) {
      il = (ml*(kl-j) + kl/2) / kl;
      mspot(ix-il, iy+nl-j);
      ivect(ix+il, iy+nl-j);
    }
    break;
  case 11:  /* horizontal cross */
  case 13:  /* asterisk */
    mspot(ix-ml, iy);
    ivect(ix+ml, iy);
    mspot(ix, iy-ml);
    ivect(ix, iy+ml);
    if (symbol == 11) break;
  case 12:  /* inclined cross */
    mspot(ix+ml, iy+ml);
    ivect(ix-ml, iy-ml);
    mspot(ix+ml, iy-ml);
    ivect(ix-ml, iy+ml);
  }
  return 0;
} /* symbg */

/* ======================================================================= */
int set_minig_color_rgb(int color_num, float red, float grn, float blu)
{
  XColor screen_color;
  Status istat;
  int i;

  if (color_num < 0 || color_num > 19) return 1;
  /* call finig to dump stored plot array before changing color */
  if (nstored > 0) finig();
  /* create color */
  if (red > 1.0f) red = 1.0f;
  if (grn > 1.0f) grn = 1.0f;
  if (blu > 1.0f) blu = 1.0f;
  if (red < 0.0f) red = 0.0f;
  if (grn < 0.0f) grn = 0.0f;
  if (blu < 0.0f) blu = 0.0f;
  screen_color.red   = (unsigned short) (65535.1f * red);
  screen_color.green = (unsigned short) (65535.1f * grn);
  screen_color.blue  = (unsigned short) (65535.1f * blu);
  istat = XAllocColor(disp_id, 
		      DefaultColormapOfScreen(screen_id), 
		      &screen_color);
  if (istat) {
    i = screen_color.pixel;
  } else {
    i = color[1];
  }

  if (gc_id) XSetForeground(disp_id, gc_id, i);

  /* log info to temp file for possible later hardcopy */
  if (hclog) {
    HCL("r",1); HCL(&color_num, 4); HCL(&red, 4); HCL(&grn,4); HCL(&blu,4);
  }
  return 0;
} /* set_minig_color_rgb */

/* ======================================================================= */
int set_minig_line_width(int w)
{
  static int clw = 0;

  if (w < 2) w = 0;
  if (w == clw) return 0;

  /* call finig to dump stored plot array
     before changing attribute block */
  finig();
  clw = w;
  XSetLineAttributes(disp_id, gc_id, w, LineSolid, CapRound, JoinRound);

  /* log info to temp file for possible later hardcopy */
  if (hclog) {
    HCL("w",1); HCL(&w, 4);
  }
  return 0;
} /* set_minig_line_width */
