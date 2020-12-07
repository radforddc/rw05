#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#define D_NUM_MENUS 20
#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"

Display              *disp_id;
Screen               *screen_id;
Window               root_win_id;
XSetWindowAttributes setwinattr;
XGCValues            xgcvl;
int                  bstat;
XEvent               event;
Window               menu_win_id[D_NUM_MENUS];
int                  menu_width[D_NUM_MENUS], menu_height[D_NUM_MENUS];
GC                   mgc_id[D_NUM_MENUS], mgc_comp_id[D_NUM_MENUS];
Window               jroot, jchild;
unsigned int         jmask;
int                  ix_root, iy_root, ix, iy, ixp, iyp;

/*--------------------------------------------------------*/
void init_menu(int nmenu, int nchars, int nlines, char *choices,
	       char *label, int x_loc, int y_loc)
{

#define MAX_NUM_ITEMS 15
  char              MyChoices[MAX_NUM_ITEMS][24];
  unsigned long     valuemask;
  int               font, i, iopt, j_width, ixm, iym;
  static XSizeHints win_position;   /*position and size for window manager.*/
  static int        first = 1;

  if (first) {
    first = 0;
    /* Initialize display id and screen id.... */
    disp_id   = XOpenDisplay(0);
    screen_id = DefaultScreenOfDisplay(disp_id);
    /* Initialize some global vars. */
    root_win_id = XRootWindowOfScreen(screen_id);
    setwinattr.bit_gravity = SouthWestGravity;
    setwinattr.backing_store = Always;
    setwinattr.background_pixel = XWhitePixelOfScreen(screen_id);
  }

  /* Initialize mychoices for fixed size array. */
  for (i = 0; i < nlines; i++) {
    strncpy(MyChoices[i], choices + i*24, 23);
    MyChoices[i][23] = '\0';
  }

  /* Next instruction for debugging only. */
  /* XSynchronize(disp_id, 1); */

  /* Create the window and load the font for text writing. */
  j_width = nchars + 1;
  if (j_width < 12) j_width = 12;
  menu_width[nmenu]   = 7 * j_width;
  menu_height[nmenu]  = nlines * 16 + 4;
      
  /* Get position of mouse pointer in root window. */
  bstat = XQueryPointer(disp_id, root_win_id, &jroot, &jchild,
                        &ix_root,& iy_root, &ixp,& iyp, &jmask);

  /* If specified location is neg.,
     position new menu relative to current pointer location. */
  if (x_loc ==-1)
    ixm = ix_root - menu_width[nmenu]/2;
  else if (x_loc == -2)
    ixm = ix_root + 40;
  else if (x_loc == -3)
    ixm = ix_root + 120;
  else
    ixm = x_loc;
  
  if (y_loc < 0)
    iym = iy_root - menu_height[nmenu]/2 + y_loc;
  else
    iym = y_loc;

  valuemask = CWBitGravity | CWBackingStore | CWBackPixel;
  menu_win_id[nmenu] = XCreateWindow(disp_id, root_win_id, ixm, iym,
				     menu_width[nmenu], menu_height[nmenu], 10,
				     XDefaultDepthOfScreen(screen_id), InputOutput,
				     XDefaultVisualOfScreen(screen_id),
				     valuemask, &setwinattr);
  /* WMHints structure */
  win_position.x = ixm;
  win_position.y = iym;
  win_position.width     = menu_width[nmenu];
  win_position.min_width = menu_width[nmenu];
  win_position.max_width = menu_width[nmenu];
  win_position.height     = menu_height[nmenu];
  win_position.min_height = menu_height[nmenu];
  win_position.max_height = menu_height[nmenu];
  win_position.flags = USPosition|USSize|PMinSize|PMaxSize;
  
  XSetWMNormalHints(disp_id, menu_win_id[nmenu], &win_position); 
  XStoreName(disp_id, menu_win_id[nmenu], label);
  
  /* Create graphics context. */
  xgcvl.background = XWhitePixelOfScreen(screen_id);
  xgcvl.foreground = XBlackPixelOfScreen(screen_id);
  mgc_id[nmenu] = XCreateGC(disp_id, menu_win_id[nmenu], 
			   GCForeground | GCBackground, &xgcvl);
  xgcvl.function = GXinvert;
  mgc_comp_id[nmenu] = XCreateGC(disp_id, menu_win_id[nmenu], GCFunction, &xgcvl);

  /* Load the font for text writing. */
  font = XLoadFont(disp_id, FONTNAME);
  XSetFont(disp_id, mgc_id[nmenu], font);

  /* Map the window. */
  XSelectInput(disp_id, menu_win_id[nmenu], ExposureMask|VisibilityChangeMask);
  XMapRaised(disp_id, menu_win_id[nmenu]);

  /*
   * Wait for the window to be raised. Some X servers do not generate
   * an initial expose event, so also check the visibility event.
   */
  XMaskEvent(disp_id,ExposureMask|VisibilityChangeMask,&event);
  XSelectInput(disp_id, menu_win_id[nmenu], NoEventMask);
  XSync(disp_id,1);

  /* Write choices/options to new menu window. */
  ix = 5;
  iy = -2;

  for (iopt = 0; iopt < nlines; iopt++) {
    iy += 16;
    XDrawString(disp_id, menu_win_id[nmenu], mgc_id[nmenu], ix, iy, 
		MyChoices[iopt], strlen(MyChoices[iopt]));
    XFlush(disp_id);
  }
}

/*--------------------------------------------------------*/
void delete_menu(int nmenu)
{
  XDestroyWindow(disp_id, menu_win_id[nmenu]);
  XSync(disp_id,0);
}

/*--------------------------------------------------------*/
void get_selection(int nmenu, int *item_sel, int *menu_sel,
		   int *ibutton, int *nopts, int *menu_up)
{
  int      i, j, iyi, jmenu, iwin_up[4], nwin_up, imenu;
  int      menu_save, item_save;
  XPoint   points[5];
  int      bGotCommand;

  /* Create table of windows that are mapped at present time. */
  if (nmenu < 11) {
    nwin_up = 0;
    for (i=0; i < 4; i++) {
      if (menu_up[i]) iwin_up[nwin_up++] = i;
    }
  } else {
    nwin_up = 0;
    iwin_up[nwin_up] = nmenu;
  }
  
  for (i = 0; i < nwin_up; i++) {
    XSelectInput(disp_id, menu_win_id[iwin_up[i]],
		 PointerMotionMask | ButtonPressMask);
  }
  
  menu_save = -1;
  item_save = -1;
  bGotCommand = 0;
  while (!bGotCommand) {
    XNextEvent(disp_id, &event);
    switch (event.type) {
    case MotionNotify:
      /* Find window in our look-up table which got event. */
      imenu = -1;
      ixp = event.xmotion.x;
      iyp = event.xmotion.y;
      for (i = 0; i < nwin_up; i++) {
	j = iwin_up[i];
	if (menu_win_id[j] == event.xmotion.window) imenu = iwin_up[i];
      }

      if (imenu != -1 && ixp > 0 && ixp < menu_width[imenu] &&
	                 iyp > 0 && iyp < menu_height[imenu]) {
	*menu_sel = imenu;
	*item_sel = (iyp-2)/16;
	if (*item_sel < 0) *item_sel = 0;
	if (*item_sel > nopts[imenu]) *item_sel = nopts[imenu];
      }
      else {
	*menu_sel = 0;
	*item_sel = 0;
      }
      if (*item_sel != item_save || *menu_sel != menu_save) {
	if (item_save != -1) {
	  /* Remove selected box (complement). */
	  XDrawLines(disp_id, menu_win_id[menu_save], mgc_comp_id[menu_save],
		     points, 5, CoordModeOrigin);
	}
	/* Draw selected box (complement). */
	menu_save = *menu_sel;
	item_save = *item_sel;
	if (*item_sel != -1) {
	  iyp = (*item_sel + 1)*16 - 14;
	  points[0].x = 3;
	  points[0].y = iyp + 15;
	  points[1].x = menu_width[menu_save] - 3;
	  points[1].y = iyp + 15;
	  points[2].x = menu_width[menu_save] - 3;
	  points[2].y = iyp;
	  points[3].x = 3;
	  points[3].y = iyp;
	  points[4].x = 3;
	  points[4].y = iyp + 14;
	  XDrawLines(disp_id, menu_win_id[menu_save], mgc_comp_id[menu_save],
		     points, 5, CoordModeOrigin);
	}
	XFlush(disp_id);
      }
      break;
    case ButtonPress:
      imenu = iwin_up[0];
      for (i = 0; i < nwin_up; i++) {
	if (menu_win_id[iwin_up[i]] == event.xbutton.window) imenu = iwin_up[i];
      }

      *ibutton = 1;
      if (event.xbutton.button == Button2) *ibutton = 2;
      if (event.xbutton.button == Button3) *ibutton = 3;
      iyi = event.xbutton.y;
      *item_sel = (iyi-2)/16;
      if (*item_sel < 0) *item_sel = 0;
      if (*item_sel > nopts[imenu]) *item_sel = nopts[imenu];
      *menu_sel = imenu;

      /* Remove mouse input events. */
      for (j = 0; j < nwin_up; j++) {
	jmenu = iwin_up[j];
	XSelectInput(disp_id, menu_win_id[jmenu],0);
      }
      /* Discard all input events. */
      XSync(disp_id, 1);

      /* Remove selected box (complement). */
      if (item_save != -1)
	XDrawLines(disp_id, menu_win_id[menu_save], mgc_comp_id[menu_save],
		   points, 5, CoordModeOrigin);
      XFlush(disp_id);
      bGotCommand = 1;
      break;
    default:
      /* All events selected by StructureNotifyMask
       * except ConfigureNotify are thrown away here,
       * since nothing is done with them.
       */
      break;
    } /* end switch */
  } /* end while */
}
