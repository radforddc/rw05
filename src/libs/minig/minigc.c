/* C version of minig_x fortran subroutines
   version 0.0   D.C. Radford    July 1999
   last edit     D.C. Radford    Oct  2019
 */
#include <stdio.h>
#ifndef VMS
#include <termios.h>
#endif
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "minig.h"

extern Display   *disp_id;
extern Window    win_id;
extern GC        gc_comp_id;
extern XEvent    event;
extern int   win_width, win_height;
extern int   ixp, iyp;
extern FILE  *hcscrfile;
extern int   hclog;

extern int doTerminal_io(int *, int *, char *);
extern int doXWindow_io(int *, int *, char *);
int done_report_curs(void);

/* ======================================================================= */
int erase(void)
{
  /* rewind scratch file for storage of data for possible hardcopy */
  if (hclog > 0) {
    fflush(hcscrfile);
    rewind(hcscrfile);
  }

  XClearWindow(disp_id,win_id);
  return 0;
} /* erase */

/*--------------------------------------------------------*/
void setup_read(int enable)
{
#ifndef VMS
  static struct termios   tbuf, tbufsave;
  static int              first = 1;
  
  if (first == 1) {
    first = 0;
    /* Save the terminal settings */
    if (tcgetattr(0, &tbufsave) == -1)
      perror("setup_read: save terminal tcgetattr call failed");

    tbuf = tbufsave;
    tbuf.c_lflag &= ~(ICANON | ECHO);
    tbuf.c_cc[VMIN]  = 10;  /* MIN no. of chars to be accepted */
    tbuf.c_cc[VTIME] =  1;  /* TIMEout period in 0.1-second units */
  }

  if (enable == 1) {
    /* Turn off echo and blocking for terminal;
       i.e. do not wait for <return> */
    if (tcsetattr(0,TCSANOW,&tbuf) == -1)
      perror("setup_read: set terminal tcsetattr call failed");
  }
  else {
    /* Turn echo and blocking back on */
    if (tcsetattr(0,TCSANOW,&tbufsave) == -1)
      perror("setup_read: reset terminal tcsetattr call failed");
  }
#endif
} /* setup_read */

/*--------------------------------------------------------*/
void set_up_cursor()
{
  Window          jroot,jchild;
  int             ix_root,iy_root;
  unsigned int    jmask;

  /* accept mouse pointer move and button move events */
  XSelectInput(disp_id, win_id,
	       (PointerMotionMask | KeyPressMask | ButtonPressMask));
  /* get position of mouse pointer */
  if (!XQueryPointer(disp_id, win_id, &jroot, &jchild,
		     &ix_root, &iy_root, &ixp, &iyp, &jmask)) {
    ixp = 100;
    iyp = 80;
  }

  /* draw cross hairs (complement) */
  XDrawLine(disp_id, win_id, gc_comp_id, ixp, 0, ixp, win_height);
  XDrawLine(disp_id, win_id, gc_comp_id, 0, iyp, win_width, iyp);
  XFlush(disp_id);

  /* enable input from terminal window */
  setup_read(1);
} /* set_up_cursor */

/*--------------------------------------------------------*/
void done_cursor()
{
  /* remove mouse input events */
  XSelectInput(disp_id, win_id, 0);
  /* discard all input events */
  XSync(disp_id, 1);
  /* remove cross hairs (complement) */
  XDrawLine(disp_id, win_id, gc_comp_id, ixp, 0, ixp, win_height);
  XDrawLine(disp_id, win_id, gc_comp_id, 0, iyp, win_width, iyp);
  XFlush(disp_id);
  done_report_curs();
  /* disable input from terminal window */
  setup_read(0);
} /* done_cursor */

/*--------------------------------------------------------*/
void move_gw_pointer(int *ixi, int *iyi)
{
  Window       jroot,jchild;
  int          ix_root, iy_root, ixin, iyin, ixout, iyout;
  unsigned int jmask;

  /* Get position of mouse pointer */
  if (XQueryPointer(disp_id, win_id, &jroot, &jchild,
			&ix_root, &iy_root, &ixin, &iyin, &jmask) &&
      ixin >= 0 && ixin <= win_width &&
      iyin >= 0 && iyin <= win_height) {
    /* Move pointer to new location. */
    ixout = ixin + (short) (*ixi);
    iyout = iyin + (short) (*iyi);
    XWarpPointer(disp_id, win_id, win_id, ixin, iyin,
		 win_width, win_height, ixout, iyout);
    XFlush(disp_id);
  }
} /* move_gw_pointer */

/* ======================================================================= */
int retic(float *x, float *y, char *cout)
{
  /* call up cursor
     x,y  returns position w.r.to axes when cursor terminated
     cout returns pressed key, or G for mouse button */

  int           i, ixi, iyi, validRtnString = 0, ilx, ily;
  char          lstring[40];
  static int    iFdDisplay = 0, first = 1;
  static fd_set ReadFds;
  fd_set        chkReadFds;
#ifdef VMS
  struct timeval tv;
#endif

  /* ring bell */
  if (strncmp(cout, "NOBELL", 6)) bell();
  set_focus(win_id);

  set_up_cursor();
  /* wait for mouse button or text window keyboard button to be pressed
     check to see if mouse button has been pressed */
  if (first) {
    first = 0;
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
    /* set timeout for select to 1 second */
    /* tv.tv_sec = 1;
       tv.tv_usec = 0; */
    /* Wait for data on xterminal or graphics window. */
    /* if (select(iFdDisplay + 1, &chkReadFds, NULL, NULL, &tv) > 0) { */
    if (select(iFdDisplay + 1, &chkReadFds, NULL, NULL, NULL) > 0) {
      /* Determine which channel was triggered and process event. */
      if (FD_ISSET(STDIN_FILENO, &chkReadFds))
	validRtnString = doTerminal_io(&ilx, &ily, lstring);
      if (FD_ISSET(iFdDisplay, &chkReadFds))
	validRtnString = doXWindow_io(&ilx, &ily, lstring);
    }
  }
#endif
  ixi = ilx;
  iyi = ily;
  strcpy(cout, lstring);
  done_cursor();

  /* convert coordinates and exit */
  *x = 0.f;
  *y = 0.f;
  if (ixi >= 0 && ixi <= win_width &&
      iyi >= 0 && iyi <= win_height) {
    i = win_height - iyi;
    cvxy(x, y, &ixi, &i, 2);
  }
  return 0;
} /* retic */
