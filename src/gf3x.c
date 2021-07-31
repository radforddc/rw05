/* X-windows menu version of gf3
   D.C. Radford    Aug. 1999 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "util.h"

/* global data */

extern struct {
  double gain[6];                           /* energy calibration */
  int    ical, nterms;
  int    disp, loch, hich, locnt, nchs;     /* spectrum display stuff */
  int    ncnts, iyaxis, lox, numx;
  float  finest[5], swpars[3];              /* initial parameter estimates */
  int    infix[3], infixrw, infixw;         /*  and flags */
  int    nclook, lookmin, lookmax;          /* window lookup table */
  short  looktab[16384];
  int    mch[2];                            /* fit limits and peak markers */
  float  ppos[35];
  float  pars[111], errs[111];              /* fit parameters and flags */
  int    npars, nfp, freepars[111], npks, irelw, irelpos;
  float  areas[35], dareas[35], cents[35], dcents[35];
  float  covariances[111][111];             /* covariances between parameters */
  int    pkfind, ifwhm, isigma, ipercent;   /* peak find stuff */
  int    maxch;                             /* current spectrum */
  char   namesp[8], filnam[80];
  float  spec[16384];
  float  stoc[20][35], stodc[20][35];       /* stored areas and centroids */
  float  stoa[20][35], stoda[20][35], stoe[20][35], stode[20][35];
  int    isto[20];
  char   namsto[560];
  float  wtsp[16384];                       /* fit weight mode and spectrum */
  int    wtmode;
  char   nwtsp[8];
} gf3gd;

extern FILE *infile;    /* command file descriptor*/
extern int  menu_mode;

extern int gfexec(char *, int);
extern int gfinit(int argc, char **argv);

int  gf3menu(char *);
void init_menu(int, int, int, char *, char *, int, int);
void delete_menu(int);
void get_selection(int, int *, int *, int *, int *, int *);
void get_yn_selection(int, int *, int *, int *);
void get_location(char *);
int  GetDisplayLocation(int);

#define D_NUM_MENUS 20
#define MAX_MENUS 8
#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"

extern Display       *disp_id;
extern Screen        *screen_id;
extern Window        root_win_id;
Window               jroot, jchild, menu_win_id[D_NUM_MENUS];
XSizeHints           win_position;
XSetWindowAttributes setwinattr;
extern XEvent               event;
XGCValues            xgcvl;
XPoint               mpoints[5];
GC                   mgc_id[D_NUM_MENUS], mgc_comp_id[D_NUM_MENUS];
unsigned int         jmask;
unsigned long        valuemask;
extern int           ixp, iyp;
int                  bstat;
int                  menu_width[D_NUM_MENUS], menu_height[D_NUM_MENUS];

/* Data to allow specification of location of viewport. */
int  loc_xmin[24] = { 0, 20, 20, 40, 40, 40,
                      60, 60, 60, 60,
                      80, 80, 80, 80, 80, 80, 
		      100,100,100,100,100,100,100,100};
int  loc_ymin[24] = { 0,  0, 60,  0, 40, 80, 
                      0, 30, 60, 90,
                      0, 20, 40, 60, 80,100,
                      0, 15, 30, 45, 60, 75, 90, 105 };
int  loc_ymax[24] = { 120, 60, 120, 40, 80, 120, 
                      30, 60, 90, 120, 
                      20, 40, 60, 80, 100, 120, 
                      15, 30, 45, 60, 75, 90, 105, 120 };

/* ======================================================================= */
int main (int argc, char **argv)
{
  int  nc;
  char ans[80];

  /* welcome and get initial data */
  gfinit(argc, argv);

  /* enter menu mode unless executing command file gfinit.cmd */
  menu_mode = !infile;

  while (1) {
    if (!menu_mode || infile) {
      /* get new typed command */
      nc = cask("?", ans, 80);
    } else {
      /* get new menu command */
      nc = gf3menu(ans);
      if (nc > 1) printf("%s\n", ans);
    }
    /* decode and execute command */
    if (nc > 1) gfexec(ans, nc);
    if (!strncmp(ans, "ME", 2)) menu_mode = 1;
  }
} /* main */

/* ======================================================================= */
int gf3menu(char *menu_ans)
{
  /* display menu of options */

  int icommand, item_sel, menu_sel, i, nmenu, ibutton;

  static int x_loc[8] = { 0,-2,-2,-2,-2,-2,-3,-3 };
  static int y_loc[8] = { 100,-1,-1,-1,-1,-1,-1,-1 };
  static int menu_up[8] = { 0,0,0,0,0,0,0,0 };
  static int nopts[8] = { 8,12,10,6,9,10,9,9 };
  static int nchars[8] = { 16,22,19,24,21,24,23,24 };

  static char title[8][15] = {
    "Main Menu","Spectrum","Display","Fit",
    "Files","Info","Modify Display","Modify Fit" };

  static char options[8][12][24] = {
    /* Main Menu */
    { "Spectrum","Display","Fit","Files","Info","Help",
      "Command Mode","Exit gf3","","","","" },
    /* Spectrum */
    { "Read Spectrum","Slice Matrix w/Cursor","Write Spectrum","Add Spectrum",
      "Multiply Spectrum","Divide Spectrum","Adjust Gain","Contract Spectrum",
      "Set Counts with Cursor","Add Counts","Divide by Efficiency","AutoBackground" },
    /* Display */
    { "Display Spectrum","Overlay Spectrum","Display Whole Sp.","Overlay Whole Sp.",
      "Re-draw Spectra","Re-draw w/Autoscale","Display Fit","Display Markers",
      "Display Windows","Hardcopy Display","","" },
    /* Fit */
    { "Set-up New Fit","Do Fit","Reset Free Parameters","List Parameters",
      "Store Areas & Centroids","AutoFit","","","","","","" },
    /* Files */
    { "Dump Set-up","Indump Set-up","Create Look-Up Table","Open Slice File",
      "Execute Command File","Create Command File","Close Command File",
      "Command File Check","Continue Command File","","","" },
    /* Info */
    { "Call Cursor","Peak Find on Display","Sum Counts","Sum Counts w/Bkgd Subtr.",
      "Energy Calibration","Add Windows","AutoCalibrate Sp. (1)", 
      "AutoCalibrate Sp. (3)","AutoCalibrate Sp. (4)","AutoPeak Integration","","" },
    /* Modify Display */
    { "Expand Display w/Cursor","Autoscale X-axis","Move Up","Move Down",
      "Change X0, NX","Autoscale Y-axis","Change Y0, NY","Linear/Log. Y-axis",
      "Change Colour Map","","","" },
    /* Modify Fit */
    { "Add Peak","Delete Peak","Fix Parameter(s)","Free Parameter(s)",
      "Fix/Free Relative Widths","Fix/Free Rel. Positions","Change Markers",
      "Change Starting Width","Change Weight Mode","","","" } };

  static char ans_return[8][12][6] = {
    { "ME 1","ME 2","ME 3","ME 4","ME 5","HE","MX 1","ST","","","","" },
    { "SP","SP/C","WS","AS","MS","DV","AG","CT","SC","AC","DE","BG" },
    { "DS  ","OV  ","DS 1","OV 1","RD","RD 1","DF","DM","DW","HC","","" },
    { "NF","FT","RF","LP","SA","AF","","","","","","" },
    { "DU","IN","LU","SL","CF","CF LOG","CF END","CF CHK","CF CON","","","" },
    { "CR","PF","SU","SB","EC","WI","CA","CA3","CA4","PK","","" },
    { "EX","NX 0","MU","MD","XA","NY 0","YA","MX 2","CO","","","" },
    { "AP","DP","FX","FR","MX 3","MX 4","MA","SW","WM","","","" } };

  nmenu = 0;
  if (!menu_up[nmenu]) {
    init_menu(nmenu, nchars[nmenu], nopts[nmenu], options[nmenu][0],
	      title[nmenu], x_loc[nmenu], y_loc[nmenu]);
    menu_up[nmenu] = 1;
  }
  while (1) {
    nmenu = 0;
    get_selection(nmenu, &item_sel, &menu_sel, &ibutton, nopts, menu_up);
    strcpy(menu_ans, ans_return[menu_sel][item_sel]);
    if (strncmp(menu_ans, "ME", 2)) break;
    if (menu_sel == 0) {
      for (i = 1; i < 8; ++i) {
	if (menu_up[i]) delete_menu(i);
	menu_up[i] = 0;
      }
    }
    nmenu = strtod(menu_ans+3, NULL);
    init_menu(nmenu, nchars[nmenu], nopts[nmenu], options[nmenu][0],
	      title[nmenu], x_loc[nmenu], y_loc[nmenu]);
    menu_up[nmenu] = 1;
    if (nmenu == 2 || nmenu == 3) {
      nmenu += 4;
      init_menu(nmenu, nchars[nmenu], nopts[nmenu], options[nmenu][0],
		title[nmenu], x_loc[nmenu], y_loc[nmenu]);
      menu_up[nmenu] = 1;
    }
  }
  if (!strncmp(menu_ans, "MX", 2)) {
    icommand = strtod(menu_ans+3, NULL);
    if (icommand == 1) {
      /* go to command mode */
      for (i = 0; i < 8; ++i) {
	if (menu_up[i]) delete_menu(i);
	menu_up[i] = 0;
      }
      printf("Type ME to get back to menu mode.\n");
      menu_mode = 0;
      strcpy(menu_ans, "");
    } else if (icommand == 2) {
      /* toggle linear / log y-axis */
      if ( gf3gd.iyaxis == -1) strcpy(menu_ans, "NY -3");
      else strcpy(menu_ans, "NY -1");
    } else if (icommand == 3) {
      /* toggle rw0/rw1 */
      if (!gf3gd.irelw) strcpy(menu_ans, "FR RW");
      else strcpy(menu_ans, "FX RW");
    } else if (icommand == 4) {
      /* toggle rp0/rp1 */
      if (!gf3gd.irelpos) strcpy(menu_ans, "FR RP");
      else strcpy(menu_ans, "FX RP");
    }
  }
  if (!strncmp(menu_ans, "DS", 2) || !strncmp(menu_ans, "OV", 2)) {
    strcpy(menu_ans+6, menu_ans+2);
    get_location(menu_ans+2);
  }
  /* if log command file open, copy response */
  log_to_cmd_file(menu_ans);

  return (strlen(menu_ans));
} /* gf3menu */

/* ======================================================================= */
int caskyn(char *mesag)
{
  int  ibutton, item_sel, menu_sel, nmenu = 15;
  char ans[40];
  static char options[2][24] = {"Yes","No"};

  /* modified for use with menu */

  if (!infile && menu_mode) {
    /* output prompt message */
    printf ("%s", mesag);
    fflush(stdout);
    /* create yes/no menu window */
    init_menu(15, 3, 2, options[0], mesag, -1, -20);
    get_yn_selection(nmenu, &item_sel, &menu_sel, &ibutton);
    delete_menu(15);

    if (item_sel == 0) strcpy(ans,"Y");
    else strcpy(ans,"N");
    /* copy response to terminal */
    printf (" %s\n", ans);
    /* if log command file open, copy response */
    log_to_cmd_file(ans);
    if (item_sel == 0) return 1;
    return 0;
  }

  while (cask(mesag, ans, 1)) {
    if (ans[0] == 'N' || ans[0] == 'n' || ans[0] == '0') return 0;
    if (ans[0] == 'Y' || ans[0] == 'y' || ans[0] == '1') return 1;
  }
  return 0;
} /* caskyn */

/* -------------------------------------------------------- */
void init_menu(int nmenu, int nchars, int nlines, char *choices,
	       char *label, int x_loc, int y_loc)
{
  int        font, ix, iy, iopt, j_width, ixm, iym, nc, nl;
  static int ix_root = 0, iy_root = 0, first = 1;
  char       *c, *c2;

  if (first) {
    first = 0;
    /* Initialize display id and screen id */
    /* actually, just use disp_id, screen_id & root_win_id as extern from minig */
    /* disp_id   = XOpenDisplay(0); */
    /* screen_id = DefaultScreenOfDisplay(disp_id); */
    if (!disp_id) {
      printf("Display not opened!\n");
      exit(-1);
    }
    /* Initialize some global vars. */
    /* root_win_id = XRootWindowOfScreen(screen_id); */
    setwinattr.bit_gravity = SouthWestGravity;
    setwinattr.backing_store = Always;
    setwinattr.background_pixel = XWhitePixelOfScreen(screen_id);
  }

  /* Next instruction for debugging only. */
  /* XSynchronize(disp_id, 1); */
  /* Create the window and load the font for text writing. */
  j_width = nchars + 1;
  if (j_width < 12) j_width = 12;
  nl = nlines;

  if (nmenu == 15) {
    c = label;
    while ((c2 = strchr(c, '\n'))) {
      nl++;
      if (j_width < (c2-c)) j_width = (c2-c);
      c = c2+1;
    }
    nl++;
    if (j_width < (nc = strlen(c))) j_width = nc;
  }
  menu_width[nmenu]  = 7 * j_width;
  menu_height[nmenu] = nl * 16 + 4;
      
  /* Get position of mouse pointer in root window. */
  if (x_loc != -3) bstat = XQueryPointer(disp_id,root_win_id,&jroot,&jchild,
					 &ix_root,&iy_root,&ixp,&iyp,&jmask);

  /* If specified location is neg.,
     position new menu relative to current pointer location. */
  if (x_loc ==-1) {
    ixm = ix_root - 40;
    if (ixm < 0) ixm = 0;
  }
  else if (x_loc == -2)
    ixm = ix_root + 40;
  else if (x_loc == -3)
    ixm = ix_root + 180;
  else
    ixm = x_loc;
  
  if (y_loc < 0)
    iym = iy_root - menu_height[nmenu]/2 + y_loc;
  else
    iym = y_loc;

  valuemask = CWBitGravity | CWBackingStore | CWBackPixel;
  menu_win_id[nmenu]  = XCreateWindow(disp_id, root_win_id, ixm, iym,
				      menu_width[nmenu], menu_height[nmenu],
				      10, XDefaultDepthOfScreen(screen_id),
				      InputOutput, XDefaultVisualOfScreen(screen_id),
				      valuemask, &setwinattr);
  /* WMHints structure */
  win_position.x = ixm;
  win_position.y = iym;
  win_position.width      = menu_width[nmenu];
  win_position.min_width  = menu_width[nmenu];
  win_position.max_width  = menu_width[nmenu];
  win_position.height     = menu_height[nmenu];
  win_position.min_height = menu_height[nmenu];
  win_position.max_height = menu_height[nmenu];
  win_position.flags = USPosition|USSize|PMinSize|PMaxSize;
  XSetWMNormalHints(disp_id, menu_win_id[nmenu], &win_position); 
  if (nmenu == 15) {
    XStoreName(disp_id, menu_win_id[nmenu], "Yes/No");
  } else {
    XStoreName(disp_id, menu_win_id[nmenu], label);
  }
  
  /* Create graphics context. */
  xgcvl.background = XWhitePixelOfScreen(screen_id);
  xgcvl.foreground = XBlackPixelOfScreen(screen_id);
  mgc_id[nmenu] = XCreateGC(disp_id, menu_win_id[nmenu],
			    GCForeground | GCBackground, &xgcvl);
  xgcvl.function = GXinvert;
  mgc_comp_id[nmenu] = XCreateGC(disp_id, menu_win_id[nmenu], 
				 GCFunction, &xgcvl);

  /* Load the font for text writing. */
  font = XLoadFont(disp_id, FONTNAME);
  XSetFont(disp_id, mgc_id[nmenu], font);

  /* Map the window. */
  XSelectInput(disp_id, menu_win_id[nmenu], ExposureMask|VisibilityChangeMask);
  XMapRaised(disp_id, menu_win_id[nmenu]);

  /* Wait for the window to be raised. Some X servers do not generate
     an initial expose event, so also check the visibility event. */
  XMaskEvent(disp_id,ExposureMask|VisibilityChangeMask,&event);
  XSelectInput(disp_id, menu_win_id[nmenu], NoEventMask);
  XSync(disp_id,1);

  /* Write choices/options to new menu window. */
  ix = 5;
  iy = -2;
  for (iopt = 0; iopt <nlines; iopt++) {
    iy = iy + 16;
    nc = strlen(choices+iopt*24);
    if (nc > 24) nc = 24;
    XDrawString(disp_id, menu_win_id[nmenu], mgc_id[nmenu], ix, iy, 
		choices+iopt*24, nc);
  }
  if (nmenu == 15) {
    iy += 16;
    c = label;
    while ((c2 = strchr(c, '\n'))) {
      XDrawString(disp_id, menu_win_id[nmenu], mgc_id[nmenu], ix, iy, 
		c, (c2-c));
      c = c2+1;
      iy += 16;
    }
    XDrawString(disp_id, menu_win_id[nmenu], mgc_id[nmenu], ix, iy, 
		c, strlen(c));
  }
  XFlush(disp_id);
} /* init_menu */

/* -------------------------------------------------------- */
void delete_menu(int nmenu)
{
  XDestroyWindow(disp_id, menu_win_id[nmenu]);
  XSync(disp_id,0);

} /* delete_menu */

/* -------------------------------------------------------- */
void get_selection(int nmenu, int *item_sel, int *menu_sel,
		   int *ibutton, int  *nopts, int *menu_up)
{
  int     menu_save, item_save, i, j, iyi, jmenu;
  int     iwin_up[MAX_MENUS], nwin_up, imenu, gotcommand = 0;

  /* Create table of windows that are mapped at present time. */
  if (nmenu < 11) {
    nwin_up = 0;
    for (i=0; i < MAX_MENUS; i++) {
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
  while (!gotcommand) {
    XNextEvent(disp_id, &event);
    switch  (event.type) {
    case MotionNotify:
      /* Find window in our look-up table which got event. */
      imenu = -1;
      ixp = event.xmotion.x;
      iyp = event.xmotion.y;
      for (i = 0; i < nwin_up; i++) {
	j = iwin_up[i];
	if (menu_win_id[j] == event.xmotion.window) imenu = iwin_up[i];
      }

      if (imenu != -1 &&
	  ixp > 0 && ixp < menu_width[imenu] &&
	  iyp > 0 && iyp < menu_height[imenu]) {
	*menu_sel = imenu;
	*item_sel = (iyp-2)/16; /* + 1 in .f version. RWM */
	if (*item_sel < 0) *item_sel = 0;
	if (*item_sel > nopts[imenu]) *item_sel = nopts[imenu];
      } else {
	*menu_sel = 0;
	*item_sel = 0;

      }
      if (*item_sel != item_save || *menu_sel != menu_save) {
	if (item_save != -1) {
	  /* Remove selected box (complement). */
	  XDrawLines(disp_id, menu_win_id[menu_save],
		     mgc_comp_id[menu_save], mpoints, 5, CoordModeOrigin);
	}
	/* Draw selected box (complement). */
	menu_save = *menu_sel;
	item_save = *item_sel;
	if (*item_sel != -1) {
	  iyp = (*item_sel + 1)*16 - 14;
	  mpoints[0].x = 3;
	  mpoints[0].y = iyp + 15;
	  mpoints[1].x = menu_width[menu_save] - 3;
	  mpoints[1].y = iyp + 15;
	  mpoints[2].x = menu_width[menu_save] - 3;
	  mpoints[2].y = iyp;
	  mpoints[3].x = 3;
	  mpoints[3].y = iyp;
	  mpoints[4].x = 3;
	  mpoints[4].y = iyp + 14;
	  XDrawLines(disp_id,menu_win_id[menu_save],
		     mgc_comp_id[menu_save], mpoints, 5, CoordModeOrigin);
	}
	XFlush(disp_id);
      }
      break;
    case ButtonPress:

      imenu = iwin_up[0];
      for (i = 0; i < nwin_up; i++) {
	if (menu_win_id[iwin_up[i]] == event.xbutton.window) {
	  imenu = iwin_up[i];
	}
      }

      *ibutton = 1;
      if (event.xbutton.button == Button2) *ibutton = 2;
      if (event.xbutton.button == Button3) *ibutton = 3;
      iyi = event.xbutton.y;
      *item_sel = (iyi-2)/16;
      if (*item_sel < 0) *item_sel = 0;
      if (*item_sel >= nopts[imenu]) *item_sel = nopts[imenu]-1;
      *menu_sel = imenu;

      /* Remove mouse input events. */
      for (j = 0; j < nwin_up; j++) {
	jmenu = iwin_up[j];
	XSelectInput(disp_id,menu_win_id[jmenu],0);
      }
      /* Discard all input events. */
      XSync(disp_id,1);

      /* Remove selected box (complement). */
      if (item_save != -1) {
	XDrawLines(disp_id,menu_win_id[menu_save],
		   mgc_comp_id[menu_save], mpoints, 5, CoordModeOrigin);
      }
      XFlush(disp_id);
      gotcommand = 1;
      break;
    default:
      /* All events selected by StructureNotifyMask
	 except ConfigureNotify are thrown away here,
	 since nothing is done with them. */
      break;
    }
  }
} /* get_selection */

/* -------------------------------------------------------- */
void get_yn_selection(int nmenu, int *item_sel, int *menu_sel, int *ibutton)
{
  int iyi, imenu, item_save;
  int gotcommand = 0;

  /* Create table of windows that are mapped at present time. */
  imenu = nmenu;

  XSelectInput(disp_id, menu_win_id[imenu], PointerMotionMask | ButtonPressMask);
  item_save = -1;

  while (!gotcommand) {
    XNextEvent(disp_id, &event);
    switch  (event.type) {
    case MotionNotify:
      ixp = event.xmotion.x;
      iyp = event.xmotion.y;
      if ( (ixp > 0) && (ixp < menu_width[imenu]) &&
	   (iyp > 0) && (iyp < menu_height[imenu]) ) {
	*menu_sel = imenu;
	*item_sel = (iyp-2)/16;
	if (*item_sel < 0) *item_sel = 0;
	if (*item_sel > 1) *item_sel = 1;
      } else {
	*menu_sel = 0;
	*item_sel = 0;
      }
      if (*item_sel != item_save) {
	if (item_save != -1) {
	  /* Remove selected box (complement) */
	  XDrawLines(disp_id, menu_win_id[imenu], mgc_comp_id[imenu],
		     mpoints, 5, CoordModeOrigin);
	}
	/* Draw selected box (complement) */
	item_save = *item_sel;
	if (*item_sel != -1) {
	  iyp = (*item_sel + 1)*16 - 14;
	  mpoints[0].x = 3;
	  mpoints[0].y = iyp + 15;
	  mpoints[1].x = menu_width[imenu] - 3;
	  mpoints[1].y = iyp + 15;
	  mpoints[2].x = menu_width[imenu] - 3;
	  mpoints[2].y = iyp;
	  mpoints[3].x = 3;
	  mpoints[3].y = iyp;
	  mpoints[4].x = 3;
	  mpoints[4].y = iyp + 14;
	  XDrawLines(disp_id,menu_win_id[imenu], mgc_comp_id[imenu],
		     mpoints, 5, CoordModeOrigin);
	}
	XFlush(disp_id);
      }
      break;
    case ButtonPress:
      *ibutton = 1;
      if (event.xbutton.button == Button2) *ibutton = 2;
      if (event.xbutton.button == Button3) *ibutton = 3;
      iyi = event.xbutton.y;
      *item_sel = (iyi-2)/16;
      if (*item_sel < 0) *item_sel = 0;
      if (*item_sel > 1) *item_sel = 1;
      *menu_sel = imenu;
        
      /* Remove mouse input events. */
      XSelectInput(disp_id,menu_win_id[imenu],0);
      /* Discard all input events. */
      XSync(disp_id,1);
        
      /* Remove selected box (complement). */
      if (item_save != -1) {
	XDrawLines(disp_id,menu_win_id[imenu], mgc_comp_id[imenu],
		   mpoints, 5, CoordModeOrigin);
      }
      XFlush(disp_id);
      gotcommand = 1;
      break;
    default:
      break;
    }
  }
} /* get_yn_selection */

/* -------------------------------------------------------------------------- */
void get_location(char *string)
{
  /* get location on graphics screen for display of spectrum. */

  static XPoint loc_points[24][5];
  int           ix_root, iy_root, ix, iy, ixm, iym, font, i, item_sel, iopt;
  int           last_num = 0, nmenu = 16;
  char          loclabel[4];
  static int    first  = 1;
  static char   choices[24] = "121321432165432187654321";
  static char   retstr[24][4] = {
    " 1 1"," 2 2"," 1 2"," 3 3"," 2 3"," 1 3"," 4 4"," 3 4",
    " 2 4"," 1 4"," 6 6"," 5 6"," 4 6"," 3 6"," 2 6"," 1 6",
    " 8 8"," 7 8"," 6 8"," 5 8"," 4 8"," 3 8"," 2 8"," 1 8" };

  if (first) {
      first = 0;
      for (iopt = 0; iopt < 24; iopt++) {
        loc_points[iopt][0].x = loc_xmin[iopt];
        loc_points[iopt][0].y = loc_ymin[iopt];
        loc_points[iopt][1].x = loc_xmin[iopt] + 20;
        loc_points[iopt][1].y = loc_ymin[iopt];
        loc_points[iopt][2].x = loc_xmin[iopt] + 20;
        loc_points[iopt][2].y = loc_ymax[iopt];
        loc_points[iopt][3].x = loc_xmin[iopt];
        loc_points[iopt][3].y = loc_ymax[iopt];
        loc_points[iopt][4].x = loc_xmin[iopt];
        loc_points[iopt][4].y = loc_ymin[iopt];
      }
      /* next instruction for debugging only */
      /* XSynchronize(disp_id, 1); */
    }

  /* Create the window. */
  /* Load the font for text writing. */
  menu_width[nmenu]  = 120;
  menu_height[nmenu] = 120;

  /* Get position of mouse pointer in root window. */
  bstat = XQueryPointer(disp_id,root_win_id,
			&jroot,&jchild, &ix_root,&iy_root,&ixp,&iyp,&jmask);

  /* If specified location is neg.,
     position new menu relative to current pointer location. */
  ixm = ix_root - loc_xmin[last_num] - 20;
  iym = iy_root - (loc_ymin[last_num] + loc_ymax[last_num])/2 - 10;
  valuemask = CWBitGravity | CWBackingStore | CWBackPixel;
  menu_win_id[nmenu]  = XCreateWindow(disp_id, root_win_id, ixm, iym,
				      menu_width[nmenu], menu_height[nmenu],
				      10, XDefaultDepthOfScreen(screen_id),
				      InputOutput, XDefaultVisualOfScreen(screen_id),
				      valuemask, &setwinattr);
  /* WMHints structure */
  win_position.x = ixm;
  win_position.y = iym;
  win_position.width = menu_width[nmenu];
  win_position.height = menu_height[nmenu];
  win_position.flags=USPosition|USSize;
  XSetWMNormalHints(disp_id, menu_win_id[nmenu], &win_position); 
  XStoreName(disp_id, menu_win_id[nmenu], "gf3: display location");

  /* Create graphics context. */
  xgcvl.background = XWhitePixelOfScreen(screen_id);
  xgcvl.foreground = XBlackPixelOfScreen(screen_id);
  mgc_id[nmenu] = XCreateGC(disp_id, menu_win_id[nmenu],
			    GCForeground | GCBackground, &xgcvl);
  xgcvl.function = GXinvert;
  mgc_comp_id[nmenu] = XCreateGC(disp_id, menu_win_id[nmenu], 
				 GCFunction, &xgcvl);

  /* Load the font for text writing. */
  font = XLoadFont(disp_id, FONTNAME);
  XSetFont(disp_id, mgc_id[nmenu], font);

  /* Map the window. */
  XSelectInput(disp_id, menu_win_id[nmenu], ExposureMask|VisibilityChangeMask);
  XMapRaised(disp_id, menu_win_id[nmenu]);

  /* Wait for the window to be raised. Some X servers do not generate
     an initial expose event, so also check the visibility event. */
  XMaskEvent(disp_id,ExposureMask|VisibilityChangeMask,&event);
  XSelectInput(disp_id, menu_win_id[nmenu], NoEventMask);
  XSync(disp_id,1);

  /* I don't know why this is needed, but if there is no printf here, then
   *  ultrix versions crash!
   *  DCR, 25 Aug 1994 */
  printf(" ");

  /* Write choices/options to new menu window. */
  for (iopt = 0; iopt < 24; iopt++) {
    ix = loc_xmin[iopt] + 7;
    iy = ((loc_ymin[iopt] + loc_ymax[iopt]) / 2 ) + 5;
    for (i = 0; i < 5; i++) {
      mpoints[i].x = loc_points[iopt][i].x;
      mpoints[i].y = loc_points[iopt][i].y;
    }
    loclabel[0] = choices[iopt];
    loclabel[1] = '\0';
    XDrawString(disp_id, menu_win_id[nmenu], mgc_id[nmenu],
		ix, iy, loclabel, 1);
    XDrawLines(disp_id, menu_win_id[nmenu], mgc_id[nmenu],
	       mpoints, 5, CoordModeOrigin);
  }
  XFlush(disp_id);

  item_sel = GetDisplayLocation(nmenu);
  strncpy(string, retstr[item_sel], 4);
  last_num = item_sel;
  XDestroyWindow(disp_id, menu_win_id[nmenu]);
  XFlush(disp_id);
} /* get_location */

/* -------------------------------------------------------------------------- */
int GetDisplayLocation(int nmenu)
{
  int item_sel, gotlocation = 0;
  int ix_root, iy_root, iopt, item_save; //, ibutton, ixi, iyi;
  unsigned int uiKeyState;

  /* Accept mouse pointer move and button press events */
  XSelectInput(disp_id, menu_win_id[nmenu], PointerMotionMask | ButtonPressMask);
  item_sel = -1;
  bstat = XQueryPointer(disp_id,menu_win_id[nmenu],
			&jroot, &jchild, &ix_root, &iy_root, &ixp, &iyp, &uiKeyState);
  XFlush(disp_id);

  if (ixp > 0 && ixp < menu_width[nmenu] &&
      iyp > 0 && iyp <menu_height[nmenu]) {
    for (iopt = 0; iopt < 24; iopt++) {
      if (ixp >= loc_xmin[iopt] &&
	  ixp <= loc_xmin[iopt]+20 &&
	  iyp >= loc_ymin[iopt] &&
	  iyp <= loc_ymax[iopt])item_sel = iopt;
    }
  }

  /* Draw selected box (complement) */
  item_save = item_sel;
  if (item_sel != -1) {
    mpoints[0].x = loc_xmin[item_sel] + 1;
    mpoints[0].y = loc_ymin[item_sel] + 1;
    mpoints[1].x = loc_xmin[item_sel] + 19;
    mpoints[1].y = loc_ymin[item_sel] + 1;
    mpoints[2].x = loc_xmin[item_sel] + 19;
    mpoints[2].y = loc_ymax[item_sel] - 1;
    mpoints[3].x = loc_xmin[item_sel] + 1;
    mpoints[3].y = loc_ymax[item_sel] - 1;
    mpoints[4].x = loc_xmin[item_sel] + 1;
    mpoints[4].y = loc_ymin[item_sel] + 1;
    XDrawLines(disp_id, menu_win_id[nmenu],
	       mgc_comp_id[nmenu], mpoints, 5, CoordModeOrigin);
    XFlush(disp_id);
  }

  while (!gotlocation) {
    XNextEvent(disp_id, &event);
    switch  (event.type) {
    case MotionNotify:
      ixp = event.xmotion.x;
      iyp = event.xmotion.y;
      if (ixp > 0 && ixp < menu_width[nmenu] &&
	  iyp > 0 && iyp < menu_height[nmenu]) {
	for (iopt = 0; iopt < 24; iopt++) {
	  if (ixp >= loc_xmin[iopt] &&
	      ixp <= loc_xmin[iopt]+20 &&
	      iyp >= loc_ymin[iopt] &&
	      iyp <=loc_ymax[iopt]) {
	    item_sel = iopt;
	  }
	}
      } else {
	item_sel = -1;
      }

      if (item_sel != item_save) {
	if (item_save != -1)
	  /* Remove selected box [complement]. */
	  XDrawLines(disp_id, menu_win_id[nmenu], mgc_comp_id[nmenu],
		     mpoints, 5, CoordModeOrigin);
	/* Draw selected box [complement]. */
	item_save = item_sel;
	if (item_sel != -1) {
	  mpoints[0].x = loc_xmin[item_sel] + 1;
	  mpoints[0].y = loc_ymin[item_sel] + 1;
	  mpoints[1].x = loc_xmin[item_sel] + 19;
	  mpoints[1].y = loc_ymin[item_sel] + 1;
	  mpoints[2].x = loc_xmin[item_sel] + 19;
	  mpoints[2].y = loc_ymax[item_sel] - 1;
	  mpoints[3].x = loc_xmin[item_sel] + 1;
	  mpoints[3].y = loc_ymax[item_sel] - 1;
	  mpoints[4].x = loc_xmin[item_sel] + 1;
	  mpoints[4].y = loc_ymin[item_sel] + 1;
	  XDrawLines(disp_id, menu_win_id[nmenu], mgc_comp_id[nmenu],
		     mpoints, 5, CoordModeOrigin);
	}
	XFlush(disp_id);
      }
      break;
    case ButtonPress:
      /*  FIXME - why are these vakues here but not used??? Button press doesn't work...
      ibutton = 1;
      if (event.xbutton.button == Button2)
	ibutton = 2;
      if (event.xbutton.button == Button3)
	ibutton = 3;
      iyi = event.xbutton.y;
      ixi = event.xbutton.x;
      */

      for (iopt = 0; iopt < 24; iopt++) {
	if (ixp >= loc_xmin[iopt] &&
	    ixp <= loc_xmin[iopt]+20 &&
	    iyp >= loc_ymin[iopt] &&
	    iyp <=loc_ymax[iopt]) {
	  item_sel = iopt;
	}
      }

      /* Remove mouse input events. */
      XSelectInput(disp_id,menu_win_id[nmenu],0);
      /* Discard all input events. */
      XSync(disp_id,1);

      /* Remove selected box (complement). */
      if (item_save != -1)
	XDrawLines(disp_id,menu_win_id[nmenu], mgc_comp_id[nmenu],
		   mpoints, 5, CoordModeOrigin);
      XFlush(disp_id);
      gotlocation = 1;
      break;
    default:
      break;
    }
  }
  return item_sel;
} /* GetDisplayLocation */
