/*************************************************
 *    C source code for xmesc, xmlev and xm4dg.
 *    Compiled with commands like:
 *	$cc -c -o xmescc.o -DXM2D xm234.c
 *	$cc -c -o xmlevc.o -DXM3D xm234.c
 *	$cc -c -o xm4dgc.o -DXM4D xm234.c
 *    to generate the different object codes for different programs.
 *  Author  D.C. Radford
 *  Last revised  August 1999
 **************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <X11/cursorfont.h>
#include <Xm/MainW.h>
#include <Xm/CascadeB.h>
#include <Xm/Command.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>

#include "util.h"
#include "minig.h"
#ifdef XM2D
#include "escl8r.h"
#else
#include "levit8r.h"
#endif

/* external variables defined in miniga.c */
extern Display  *disp_id;
extern Screen   *screen_id;
extern Window   win_id;
extern GC       gc_id, gc_comp_id;
extern int      win_width, win_height, menu_mode;

/* from xmminig2 */
extern int select_gw(int);
extern int set_size(int winpix, int width, int height);

#define  MAX_MENU  12
#define  MAX_OPT   15

XtAppContext app_context;
Widget       toplevel, menubar, mainWindow1, mainWindow2;
Widget       drawingArea1, drawingArea2, command, posText;
Widget       menubarBtn[MAX_MENU], pulldowns[MAX_MENU];
Widget       pdmenuBtn[MAX_MENU][MAX_OPT];
Pixmap       pixmap;
Window       window1, window2, root_id;
Display      *disp_id;
GC           xmgc;
Cursor       watchCursor;
Arg          args [10];
Position     xorig, yorig;
Dimension    ww, hh, width = 600, height = 500;
int          zoom = 1, revert_to, scr_width, scr_height;
int          igw_id, crosshairs = 0, chx = -1;
float        xin, yin, xmg[8];
char         cin[40];

/* Subroutine to copy pixmap to drawing area */
void copy_area(void)
{
  if (zoom < 8) {
    XCopyArea(disp_id, pixmap, window2, xmgc, 0, 0, width, height, 0, 0);
    XSync(disp_id, False);
  }
} /* copy_area */

/* Callback for the drawingArea2 expose event */
void DAexposeCB(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
  if (zoom < 8) {
    copy_area();
  } else {
    display_gls(-1);
  }
} /* DAexposeCB */

/* Callback for the drawingArea1 resize event */
void DAresizeCB(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
  disp_dat(1);
} /* DAresizeCB */

/* Callback for the drawing area input event */
void DAinputCB(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
  static int   incr = 8, control_yes = 0;
  Window       jroot, jchild;
  int          ix, iy, ixi, iyi;
  unsigned int state, iflag;
  char         response[40];
  unsigned int ww, wh, wb, wd;

  if (w == drawingArea2) {
    igw_id = 2;
    (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);
    select_gw(igw_id);
    ww = width;
    wh = height;
  }
  else {
    igw_id = 1;
    XGetGeometry(disp_id, window1, &jroot, &ix, &iy, &ww, &wh, &wb, &wd);
  }
  ixi = event->xbutton.x;
  iyi = wh - event->xbutton.y;
  cvxy(&xin, &yin, &ixi, &iyi, 2);
  if (igw_id == 2) select_gw(1);

  if (event->xany.type == ButtonPress) {
    if (igw_id == 2 && (event->xbutton.state & ShiftMask) != 0) {
      /* shift-mousebutton -> pan/zoom gls display */
      iflag = 1;
      if (event->xbutton.button == Button2) iflag = 2;
      if (event->xbutton.button == Button3) iflag = 3;
      pan_gls(xin, yin, iflag);
      igw_id = 0;
    } else {
      strcpy(cin, "G1");
      if (event->xbutton.button == Button2) strcpy(cin, "X2");
      if (event->xbutton.button == Button3) strcpy(cin, "X3");
    }

  } else if (event->xany.type == KeyRelease) {
    igw_id = 0;
    strcpy(response, XKeysymToString(XLookupKeysym((XKeyEvent *)event,0)));
    if (!strncmp(response, "Con", 3)) control_yes = 0;

  } else if (event->xany.type == KeyPress) {
    strcpy(response, XKeysymToString(XLookupKeysym((XKeyEvent *)event,0)));
    /* check for <select> or <tab> key */
    if ((strncmp(response, "Tab", 3) == 0) ||
	(strncmp(response, "Sel", 3) == 0)) {
      if (incr == 1) incr = 8;
      else incr = 1;
      igw_id = 0;

    /* check for arrow keys and move cursor appropriately */
    } else if (!strncmp(response, "Up", 2) ||
	       !strncmp(response, "Down", 4) ||
	       !strncmp(response, "Left", 4) ||
	       !strncmp(response, "Right", 5)) {
      (void) XQueryPointer(disp_id, XtWindow(w), &jroot, &jchild,
			   &ix, &iy, &ixi, &iyi, &state);
      ix = ixi;
      iy = iyi;
      if      (!strncmp(response, "Up", 2) == 0)    iyi = iyi - incr;
      else if (!strncmp(response, "Down", 4) == 0)  iyi = iyi + incr;
      else if (!strncmp(response, "Left", 4) == 0)  ixi = ixi - incr;
      else if (!strncmp(response, "Right", 5) == 0) ixi = ixi + incr;
      XWarpPointer(disp_id, XtWindow(w), XtWindow(w),
		   ix, iy, ww, wh, ixi, iyi);
      igw_id = 0;

    } else if (!strncmp(response, "Con", 3)) {
      control_yes = 1;
      igw_id = 0;

    } else if (control_yes && *response == 'u') {
      undo_gls(-1);
      path(1);
      igw_id = 0;
    } else if (control_yes && *response == 'r') {
      undo_gls(1);
      path(1);
      igw_id = 0;

    } else if (!strncmp(response, "Alt", 3) ||
	       !strncmp(response, "Mul", 3) ||
	       !strncmp(response, "Cap", 3) ||
	       !strncmp(response, "Con", 3) ||
	       !strncmp(response, "Shi", 3) ||
	       !strncmp(response, "Del", 3)) {
      igw_id = 0;

    } else {
      strcpy(cin, response);
    }
  } else {
    igw_id = 0;
  }
} /* DAinputCB */

/* subroutine to execute commands */
void execute(char *text, char *heading)
{
  static char title[40][50];
  static int  titleNum = -1;
  int         nc, i, iwidth, iheight;
  char        cmd[80];
  void        removecrosshairs();

  static int focus = 0;


  /* set window title to command heading */
  if (titleNum == -1) {
    titleNum++;
#ifdef XM2D
    strcpy(title[titleNum], "XmEscl8r");
#endif
#ifdef XM3D
    strcpy(title[titleNum], "XmLevit8r");
#endif
#ifdef XM4D
    strcpy(title[titleNum], "Xm4dg8r");
#endif
  }else if (titleNum == 39) {
    titleNum--;
    for (i=0; i<titleNum; i++) strcpy(title[i], title[i+1]);
  }
  if (titleNum > 9) {
    printf(" *** Warning - you should not nest so many commands.\n");
    printf(" *** Consider exiting from more previous modes\n");
    printf(" ***     before entering new ones.\n");
  }
  titleNum++;
#ifdef XM2D
  strcpy(title[titleNum], "XmEscl8r -- ");
#endif
#ifdef XM3D
  strcpy(title[titleNum], "XmLevit8r -- ");
#endif
#ifdef XM4D
  strcpy(title[titleNum], "Xm4dg8r -- ");
#endif
  strcat(title[titleNum], heading);
  XStoreName(disp_id, XtWindow(toplevel), title[titleNum]);

  /* set cusor to watch to indicate busy */
  XDefineCursor(disp_id, XtWindow(mainWindow2), watchCursor);

  if (strncmp(text, "zi", 2) == 0) {
  /* Zoom in */
    if (zoom < 16) {
      zoom = zoom*2;
      width = width*2;
      height = height*2;
    }
    XtSetArg(args[0], XmNwidth, width);
    XtSetArg(args[1], XmNheight, height);
    XtSetValues(drawingArea2, args, 2);
    iwidth = (int) width;
    iheight = (int) height;

    /* Set global minig_x variables. */
    select_gw(2);
    win_width = width;
    win_height = height;
    win_id  = pixmap;
    if (zoom < 8)
      set_size(pixmap, iwidth, iheight);
    else
      set_size(window2, iwidth, iheight);
  } else if (strncmp(text, "zo", 2) == 0) {
    /* Zoom out */
    if (zoom > 1) {
      zoom = zoom/2;
      width = width/2;
      height = height/2;
    }
    XtSetArg(args[0], XmNwidth, width);
    XtSetArg(args[1], XmNheight, height);
    XtSetValues(drawingArea2, args, 2);
    iwidth = (int) width;
    iheight = (int) height;

    /* Set global minig_x variables. */
    select_gw(2);
    win_width = width;
    win_height = height;
    win_id  = pixmap;
    if (zoom < 8)
      set_size(pixmap, iwidth, iheight);
    else
      set_size(window2, iwidth, iheight);
  } else {

    removecrosshairs();
    strcpy(cmd, text);
    nc = strlen(cmd);
    /* if log command file open, copy response */
    log_to_cmd_file(cmd);
 
    if (!focus) get_focus(&focus);
    /* execute command */

#ifdef XM2D
    e_gls_exec(cmd, nc);
    /* get commands from command file, if it is open */
    while ((nc = read_cmd_file(cmd, 80)) >= 0) e_gls_exec(cmd, nc);
#endif
#ifdef XM3D
    l_gls_exec(cmd, nc);
    /* get commands from command file, if it is open */
    while ((nc = read_cmd_file(cmd, 80)) >= 0) l_gls_exec(cmd, nc);
#endif
#ifdef XM4D
    fdg8r_exec(cmd, nc);
    /* get commands from command file, if it is open */
    while ((nc = read_cmd_file(cmd, 80)) >= 0) fdg8r_exec(cmd, nc);
#endif
    set_focus(focus);
  }

  /* restore last window title */
  titleNum--;
  XStoreName(disp_id, XtWindow(toplevel), title[titleNum]);

  /* remove watch cusor to indicate open for business */
  XUndefineCursor(disp_id, XtWindow(mainWindow2));
} /* execute */

/* Callback for the pushbuttons in the menubar */
void ButtonCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  XmString     label;
  char         *btnLabel;

  /* get pulldown menu button label for use in window title */
  XtSetArg(args[0], XmNlabelString, &label);
  XtGetValues(w, args, 1);
  XmStringGetLtoR(label, XmSTRING_DEFAULT_CHARSET, &btnLabel);
  XmStringFree(label);
  /* Execute command */
  execute((char *)client_data, btnLabel);
  XtFree(btnLabel);
} /* ButtonCB */

/* Callback for the command line widget */
void CommandCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  char *text;

  /* get command from text widget */
  XtSetArg(args[0], XmNvalue, &text);
  XtGetValues(w, args, 1);
  (void) puts(text);
  XtSetArg(args[0], XmNvalue, "");
  XtSetValues(w, args, 1);

  /* execute command */
  execute(text, text);
  XtFree(text);
} /* CommandCB */

/* subroutine to create pulldown menu panes and cascade buttons */
void create_pdm(int pd, char *pdname, char *ButtonText, char ButtonMnemonic)
{
  pulldowns[pd] = XmCreatePulldownMenu(menubar, pdname, NULL, 0);
  XtSetArg(args[0], XmNsubMenuId, pulldowns[pd]);
  XtSetArg(args[1], XmNlabelString,
	   XmStringCreate(ButtonText, XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[2], XmNmnemonic, ButtonMnemonic);
  menubarBtn[pd] = XmCreateCascadeButton(menubar, ButtonText, args, 3);
} /* create_pdm */

/* subroutine to create pulldown menu entries and buttons */
void create_pdm_sel(int pd, int pdb, char *pdbname, char *ButtonText,
		    char ButtonMnemonic, char *ReturnText)
{
  XtSetArg(args[0], XmNlabelString,
	   XmStringCreate(ButtonText,XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[1], XmNmnemonic, ButtonMnemonic);
  pdmenuBtn[pd][pdb] = XmCreatePushButtonGadget(pulldowns[pd], pdbname,
						args, 2);
  XtAddCallback(pdmenuBtn[pd][pdb], XmNactivateCallback,
		ButtonCB, ReturnText);
} /* create_pdm_sel */

/* error handler subroutine for pixmap creation */
int pixmapErrorHandler(Display *d, XErrorEvent *error)
{
  pixmap = 0;
  width = (int)width*9/10;
  height = (int)height*9/10;
  return 0;
} /* pixmapErrorHandler */

/*---------------------------------------*/
void removecrosshairs(void)
{
  if (crosshairs) {
    XDrawLine(disp_id, window1, gc_comp_id, chx, 0, chx, 1024);
    XFlush(disp_id);
    crosshairs = 0;
  }
} /* removecrosshairs */

void drawcrosshairs(void)
{
  if (chx>=0) {
    XDrawLine(disp_id, window1, gc_comp_id, chx, 0, chx, 1024);
    XFlush(disp_id);
    crosshairs = 1;
  }
} /* drawcrosshairs */

void ReportPointerPos(Widget w, XMotionEvent *event,
		      String *params, Cardinal *nparams)
{
  float e, de, x1, y1;
  int   l, g, ixi, iyi, iwin;
  char  buf[120];

  removecrosshairs();
  ixi = event->x;
  iyi = height - event->y;
  if (w == drawingArea1) {
    chx = ixi;
    drawcrosshairs();
    iwin = 1;
  } else {
    chx = -1;
    iwin = 2;
  }

  if (iwin == 1) {
    cvxy(&x1, &y1, &ixi, &iyi, 2);
    energy(x1 - 0.5f, 0.0f, &e, &de);
    sprintf(buf, "x = %.1f", e);
  } else {
    select_gw(iwin);
    cvxy(&x1, &y1, &ixi, &iyi, 2);
    select_gw(1);
    sprintf(buf, "y = %.0f", y1);
    g = nearest_gamma(x1, y1);
    if (g >= 0) {
      sprintf(buf + strlen(buf), "    Gamma: %.1f keV,  I = %.2f",
	      glsgd.gam[g].e, glsgd.gam[g].i);
    }
    l = nearest_level(x1, y1);
    if (l >= 0) {
      sprintf(buf + strlen(buf), "    Level: %s,  E = %.1f",
	      glsgd.lev[l].name, glsgd.lev[l].e);
    }
  }

  XmTextSetString(posText, buf);
} /* ReportPointerPos */

/***** Main Logic *****/
int main(int argc, char **argv)
{
  String trans1 = 
    "<MotionNotify>: ReportPointerPos() \n"
    "<Configure>: DAresizeCB() \n"
    "<ButtonPress>: DAinputCB() \n"
    "<KeyPress>: DAinputCB() \n"
    "<KeyRelease>: DAinputCB()";
  String trans2 = 
    "<MotionNotify>: ReportPointerPos() \n"
    "<Expose>: DAexposeCB() \n"
    "<ButtonPress>: DAinputCB() \n"
    "<KeyPress>: DAinputCB() \n"
    "<KeyRelease>: DAinputCB()";
  static XtActionsRec win_actions[] = {
    {"ReportPointerPos", (XtActionProc) ReportPointerPos},
    {"DAexposeCB",       (XtActionProc) DAexposeCB },
    {"DAresizeCB",       (XtActionProc) DAresizeCB },
    {"DAinputCB",        (XtActionProc) DAinputCB  } };
  Widget               label, scrolled, undoGlsButton, redoGlsButton;
  Widget               undoDatButton, redoDatButton;
  Cursor               bigcross;
  XGCValues            gcv;
  XWindowAttributes    win_attr;
  XSetWindowAttributes attrs;
  Pixmap               bigcrossPixmap;
  Window               focus, junk;
  XColor               dummy, black;
  XEvent               event1, event;
  char                 bigcrossBits[32], prog_name[32], buf[80], *c;
  int                  i, w_in, h_in, ix1, iy1;
  long                 save_mask;

  /* extract pixmap width and height from RADWARE_XMG_SIZE */
  if ((c = getenv("RADWARE_XMG_SIZE"))) { 
    strcpy(buf, c);
    if ((c = strchr(buf,'x'))) *c = ' ';
    if ((c = strchr(buf,'X'))) *c = ' ';
    if (sscanf(buf, "%d%d", &w_in, &h_in) > 0) {
      if (w_in >= 8) {
	width  = w_in;
	height = w_in;
	if (h_in >= 8) height = h_in;
      }
    }
  }

  /* Create main level scheme window and menubar */
  toplevel = XtAppInitialize (&app_context, "TopLevel", NULL, 0,
			      &argc, argv, NULL, NULL, 0);
  mainWindow2 = XmCreateForm(toplevel, "MainWindow", args, 0);
  XtSetArg(args[0], XmNtopAttachment, XmATTACH_FORM);
  XtSetArg(args[1], XmNrightAttachment, XmATTACH_FORM);
  XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
  menubar = XmCreateMenuBar(mainWindow2, "menubar", args, 3);

  /* Create Pulldown MenuPanes and cascade buttons in menubar */
  create_pdm(0,"pd0","Files",'F');
  create_pdm(1,"pd1","Display",'D');
  create_pdm(2,"pd2","Bands",'B');
  create_pdm(3,"pd3","Gammas",'G');
  create_pdm(4,"pd4","Levels",'L');
  create_pdm(5,"pd5","Text",'x');
  create_pdm(6,"pd6","Fit",'t');
  create_pdm(7,"pd7","Spectrum",'S');
  create_pdm(8,"pd8","GateLists",'a');
#ifdef XM2D
  create_pdm(9,"pd9","Info",'I');
  XtManageChildren(menubarBtn, 10);
#endif
#ifdef XM3D
  create_pdm(9,"pd9","Triples",'r');
  create_pdm(10,"pdA","Info",'I');
  XtManageChildren(menubarBtn, 11);
#endif
#ifdef XM4D
  create_pdm(9,"pd9","3D&4D ",'3');
  create_pdm(10,"pdA","Info",'I');
  XtManageChildren(menubarBtn, 11);
#endif

  /* Create pushbuttons in pulldown menus */
  create_pdm_sel(0,0,"b0.0","Create Postscript Hardcopy File",'P',"hc");
  create_pdm_sel(0,1,"b0.1","Save Level Scheme in New File",'L',"sl");
  create_pdm_sel(0,2,"b0.2","Read New .gls Level Scheme File",'N',"rg");
  create_pdm_sel(0,3,"b0.3","Add a second .gls level scheme file",'A',"ad");
  create_pdm_sel(0,4,"b0.4","Write Gate as gf3 Spectrum File",'W',"ws");
  create_pdm_sel(0,5,"b0.5","Execute Command File",'C',"cf");
  create_pdm_sel(0,6,"b0.6","Change Energy, Efficiency Calibs",'E',"ca");
  create_pdm_sel(0,7,"b0.7","Change 2D Background",'B',"cb");
  create_pdm_sel(0,8,"b0.8","Change Peak Shape and Width",'S',"ps");
  create_pdm_sel(0,9,"b0.9","Renormalise Gates:Scheme",'n',"rn");
  create_pdm_sel(0,10,"b0.10","Undo level scheme edit (C-u)",'U',"ue");
  create_pdm_sel(0,11,"b0.11","Redo level scheme edit (C-r)",'R',"re");
  create_pdm_sel(0,12,"b0.12","Exit program",'x',"st");
  XtManageChildren(pdmenuBtn[0], 13);

  create_pdm_sel(1,0,"b1.0","Zoom In",'I',"zi");
  create_pdm_sel(1,1,"b1.1","Zoom Out",'O',"zo");
  create_pdm_sel(1,2,"b1.2","Redraw Level Scheme",'R',"rd");
  create_pdm_sel(1,3,"b1.3","Redraw Entire Level Scheme",'E',"rd1");
  create_pdm_sel(1,4,"b1.4","Expand Level Scheme or Spectra",'x',"ex");
  create_pdm_sel(1,5,"b1.5","Change Figure Parameters",'F',"fp");
  create_pdm_sel(1,6,"b1.6","Open/Close Gap in Scheme",'G',"oc");
  XtManageChildren(pdmenuBtn[1], 7);

  create_pdm_sel(2,0,"b2.0","Add E2 Gammas to Top of Band",'2',"ab");
  create_pdm_sel(2,1,"b2.1","Add M1 Gammas to Top of Band",'1',"ab1");
  create_pdm_sel(2,2,"b2.2","Delete Bands",'D',"db");
  create_pdm_sel(2,3,"b2.3","Edit Bands",'E',"eb");
  create_pdm_sel(2,4,"b2.4","Move Bands",'B',"mb");
  create_pdm_sel(2,5,"b2.5","Move Multiple Bands",'M',"mm");
  create_pdm_sel(2,6,"b2.6","Move Spin Labels in Bands",'L',"lb");
  create_pdm_sel(2,7,"b2.7","Move Gammas inside Bands",'G',"gb");
  XtManageChildren(pdmenuBtn[2], 8);

  create_pdm_sel(3,0,"b3.0","Add Gammas",'A',"ag");
  create_pdm_sel(3,1,"b3.1","Delete Gammas",'D',"dg");
  create_pdm_sel(3,2,"b3.2","Edit Gammas",'E',"eg");
  create_pdm_sel(3,3,"b3.3","Move Gammas",'M',"mg");
  create_pdm_sel(3,4,"b3.4","Move Labels of Gammas",'L',"lg");
  create_pdm_sel(3,5,"b3.5","Reorder Gammas in/out of Levels",'R',"sg");
  create_pdm_sel(3,6,"b3.6","Toggle Tentative/Normal Gammas",'T',"tg");
  create_pdm_sel(3,7,"b3.7","Examine Gates with Cursor",'G',"xg");
  XtManageChildren(pdmenuBtn[3], 8);

  create_pdm_sel(4,0,"b4.0","Add Levels",'A',"al");
  create_pdm_sel(4,1,"b4.1","Delete Levels",'D',"dl");
  create_pdm_sel(4,2,"b4.2","Edit Levels",'E',"el");
  create_pdm_sel(4,3,"b4.3","Move Levels Between Bands",'M',"ml");
  create_pdm_sel(4,4,"b4.4","Move Spin Labels of Levels",'S',"ll");
  create_pdm_sel(4,5,"b4.5","Move Energy Labels of Levels",'L',"le");
  create_pdm_sel(4,6,"b4.6","Toggle Tentative/Thick/Normal Levels",'T',"tl");
  create_pdm_sel(4,7,"b4.7","Change Widths of Levels",'W',"lw");
  create_pdm_sel(4,8,"b4.8","Move Connected Levels Up/Down in Energy",'U',"ud");
  XtManageChildren(pdmenuBtn[4], 9);

  create_pdm_sel(5,0,"b5.0","Add Text Labels",'A',"at");
  create_pdm_sel(5,1,"b5.1","Delete Text Labels",'D',"dt");
  create_pdm_sel(5,2,"b5.2","Edit Text Labels",'E',"et");
  create_pdm_sel(5,3,"b5.3","Move Text Labels",'M',"mt");
  create_pdm_sel(5,4,"b5.4","Change Level Spin Labels",'S',"cl");
  create_pdm_sel(5,5,"b5.5","Change Level Energy Labels",'L',"ce");
  create_pdm_sel(5,6,"b5.6","Change Gamma Labels",'G',"cg");
  XtManageChildren(pdmenuBtn[5], 7);

  create_pdm_sel(6,0,"b6.0","Fit Intensities of Gammas",'I',"fi");
  create_pdm_sel(6,1,"b6.1","Fit Energies of Gammas",'E',"fe");
  create_pdm_sel(6,2,"b6.2","Fit Both Intensities and Energies",'B',"fb");
  create_pdm_sel(6,3,"b6.3","Fit Level Energies",'L',"fl");
  create_pdm_sel(6,4,"b6.4","Fit Peak Width Parameters",'W',"fw");
  XtManageChildren(pdmenuBtn[6], 5);

  create_pdm_sel(7,0, "b7.0", "Take Full Projection",'P',"pr");
  create_pdm_sel(7,1, "b7.1", "Examine Gates with Cursor",'G',"xg");
  create_pdm_sel(7,2, "b7.2", "Show more than 1 Gate (Stacked)",'S',"ng");
  create_pdm_sel(7,3, "b7.3", "Show only 1 gate",'1',"ng1");
  create_pdm_sel(7,4, "b7.4", "ReDraw Spectra",'R',"ds");
  create_pdm_sel(7,5, "b7.5", "ReDraw Full Spectra",'F',"ds1");
  create_pdm_sel(7,6, "b7.6", "Expand Level Scheme or Spectra",'x',"ex");
  create_pdm_sel(7,7, "b7.7", "Change X-Axis X0, NX",'X',"xa");
  create_pdm_sel(7,8, "b7.8", "Change Y-Axis Y0, NY",'Y',"ya");
  create_pdm_sel(7,9, "b7.9", "Move X-Axis Up",'U',"mu");
  create_pdm_sel(7,10,"b7.10","Move X-Axis Down",'D',"md");
  create_pdm_sel(7,11,"b7.11","Toggle Display of Calculated sp.",'T',"dc");
  create_pdm_sel(7,12,"b7.12","Hardcopy Spectrum Display Window",'H',"shc");
  XtManageChildren(pdmenuBtn[7], 13);

  create_pdm_sel(8,0,"b8.0","Create/Edit Lists of Gates",'L',"l ");
  create_pdm_sel(8,1,"b8.1","Sum List of Gates",'S',"gl");
  create_pdm_sel(8,2,"b8.2","Examine List of Gates",'E',"xl");
  create_pdm_sel(8,3,"b8.3","Read List File",'R',"rl");
  create_pdm_sel(8,4,"b8.4","Write List File",'W',"wl");
  XtManageChildren(pdmenuBtn[8], 5);

#ifdef XM2D
  create_pdm_sel(9,0,"b9.0","Setup PeakFind on Sp. Display",'P',"pf");
  create_pdm_sel(9,1,"b9.1","Call Cursor on Sp. Display",'C',"cr");
  create_pdm_sel(9,2,"b9.2","Set Gate Limits with Cursor",'G',"g ");
  create_pdm_sel(9,3,"b9.3","Sum Counts with Cursor",'S',"su");
  create_pdm_sel(9,4,"b9.4","Check for Energy Sums with Cursor",'E',"es");
  create_pdm_sel(9,5,"b9.5","Test gamma-ray energy and intensity sums",'T',"ts");
  create_pdm_sel(9,6,"b9.6","Help on Commands",'H',"he");
  XtManageChildren(pdmenuBtn[9], 7);
#else
  create_pdm_sel(9,0,"b9.0","Examine Triples with Cursor",'E',"xt");
  create_pdm_sel(9,1,"b9.1","Type Triples Gates",'T',"t ");
  create_pdm_sel(9,2,"b9.2","Fit Intensities of Gammas in cube",'I',"fti");
  create_pdm_sel(9,3,"b9.3","Fit Energies of Gammas in cube",'F',"fte");
  create_pdm_sel(9,4,"b9.4","Fit Both Ints and Energies in cube",'B',"ftb");
  create_pdm_sel(9,5,"b9.5","Search for (SD) bands in 3D",'S',"sb");
#ifdef XM3D
  XtManageChildren(pdmenuBtn[9], 6);
#endif
#ifdef XM4D
  create_pdm_sel(9,6,"b9.6","Examine Quadruples with Cursor",'x',"xq");
  create_pdm_sel(9,7,"b9.7","Type Quadruples Gates",'Q',"q ");
  create_pdm_sel(9,8,"b9.8","Search for (SD) bands in 4D",'4',"sq");
  XtManageChildren(pdmenuBtn[9], 9);
#endif

  create_pdm_sel(10,0,"bA.0","Setup PeakFind on Sp. Display",'P',"pf");
  create_pdm_sel(10,1,"bA.1","Call Cursor on Sp. Display",'C',"cr");
  create_pdm_sel(10,2,"bA.2","Set Gate Limits with Cursor",'G',"g ");
  create_pdm_sel(10,3,"bA.3","Sum Counts with Cursor",'S',"su");
  create_pdm_sel(10,4,"bA.4","Check for Energy Sums with Cursor",'E',"es");
  create_pdm_sel(10,5,"bA.5","Test gamma-ray energy and intensity sums",'T',"ts");
  create_pdm_sel(10,6,"bA.6","Help on Commands",'H',"he");
  XtManageChildren(pdmenuBtn[10], 7);
#endif

  /* Create command widget */
  XtSetArg(args[0], XmNlabelString,
	   XmStringCreate("Use menus or enter commands here:",
			  XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_FORM);
  XtSetArg(args[2], XmNbottomAttachment, XmATTACH_FORM);
  label = XmCreateLabel(mainWindow2, "label", args, 3);
  XtSetArg(args[0], XmNleftAttachment, XmATTACH_WIDGET);
  XtSetArg(args[1], XmNleftWidget, label);
  XtSetArg(args[2], XmNbottomAttachment, XmATTACH_FORM);
  command = XmCreateText(mainWindow2, "command", args, 3);
  XtAddCallback(command, XmNactivateCallback, CommandCB, NULL);

  /* Create undo and redo buttons */
  XtSetArg(args[0], XmNvalue, "UnDoGls");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNleftWidget, command);
  XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
  undoGlsButton = XmCreatePushButton(mainWindow2, "undo\ngls", args, 4);
  XtAddCallback(undoGlsButton, XmNactivateCallback, ButtonCB, "ue");
  XtSetArg(args[0], XmNvalue, "ReDoGls");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNleftWidget, undoGlsButton);
  XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
  redoGlsButton = XmCreatePushButton(mainWindow2, "redo\ngls", args, 4);
  XtAddCallback(redoGlsButton, XmNactivateCallback, ButtonCB, "re");

  XtSetArg(args[0], XmNvalue, "UnDoDat");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNleftWidget, redoGlsButton);
  XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
  undoDatButton = XmCreatePushButton(mainWindow2, "undo\ndat", args, 4);
  XtAddCallback(undoDatButton, XmNactivateCallback, ButtonCB, "uc");
  XtSetArg(args[0], XmNvalue, "ReDoDat");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNleftWidget, undoDatButton);
  XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
  redoDatButton = XmCreatePushButton(mainWindow2, "redo\ndat", args, 4);
  XtAddCallback(redoDatButton, XmNactivateCallback, ButtonCB, "rc");

  /* Create text widget */
  XtSetArg(args[0], XmNvalue, " ");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_FORM);
  XtSetArg(args[2], XmNbottomAttachment, XmATTACH_WIDGET);
  XtSetArg(args[3], XmNbottomWidget, undoGlsButton);
  XtSetArg(args[4], XmNrightAttachment, XmATTACH_FORM);
  posText = XmCreateText(mainWindow2, "posText", args, 5);

  /* Create drawingArea2 widget for level scheme */
  XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
  XtSetArg(args[1], XmNtopAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNtopWidget, menubar);
  XtSetArg(args[3], XmNrightAttachment, XmATTACH_FORM);
  XtSetArg(args[4], XmNleftAttachment, XmATTACH_FORM);
  XtSetArg(args[5], XmNbottomAttachment, XmATTACH_WIDGET);
  XtSetArg(args[6], XmNbottomWidget, posText);
  XtSetArg(args[7], XmNheight, (int)height*9/10);
  scrolled = XmCreateScrolledWindow(mainWindow2, "scrolled", args, 8);
  XtSetArg(args[0], XmNwidth, width);
  XtSetArg(args[1], XmNheight, height);
  drawingArea2 = XmCreateDrawingArea(scrolled, "levelscheme", args, 2);
  XtOverrideTranslations(drawingArea2,XtParseTranslationTable(trans2)); 
  XtAppAddActions(app_context, win_actions, XtNumber(win_actions));

  /* Set size and position of main window */
  scr_width  = XWidthOfScreen(XtScreen(toplevel));
  scr_height = XHeightOfScreen(XtScreen(toplevel));
  xmg[2] = 0;
#ifdef XM2D
  strcpy(prog_name,"escl8r__");
#endif
#ifdef XM3D
  strcpy(prog_name,"levit8r_");
#endif
#ifdef XM4D
  strcpy(prog_name,"4dg8r___");
#endif
  set_xwg(prog_name);
  if (xmg[2] != 0) {
    xorig = xmg[0]*(float)scr_width;
    yorig = xmg[1]*(float)scr_height;
    ww = xmg[2]*(float)scr_width;
    hh = xmg[3]*(float)scr_height;
    XtSetArg(args[0], XmNwidth, ww);
    XtSetArg(args[1], XmNheight, hh);
    XtSetArg(args[2], XmNx, xorig+1);
    XtSetArg(args[3], XmNy, yorig-1);
    XtSetArg(args[4], XmNborderWidth, (Dimension)0);
    XtSetValues(toplevel, args, 5);
  } else {
    hh = scr_height/2;
    XtSetArg(args[0], XmNheight, hh);
    XtSetValues(toplevel, args, 1);
  }

  /* Create graphics context */
  disp_id = XtDisplay(toplevel);
  root_id = RootWindowOfScreen(XtScreen(toplevel));
  gcv.foreground = WhitePixelOfScreen(XtScreen(toplevel));
  xmgc = XCreateGC(disp_id, root_id, GCForeground, &gcv);

  /* Save ID of window that has input focus */
  XGetInputFocus(disp_id, &focus, &revert_to);

  /* Manage widgets */
  XtManageChild(menubar);
  XtManageChild(label);
  XtManageChild(command);
  XtManageChild(undoGlsButton);
  XtManageChild(redoGlsButton);
  XtManageChild(undoDatButton);
  XtManageChild(redoDatButton);
  XtManageChild(posText);
  XtManageChild(drawingArea2);
  XtManageChild(scrolled);
  XtManageChild(mainWindow2);
  XtRealizeWidget(toplevel);
  if (xmg[2] != 0) {
    window2 = XtWindow(toplevel);
    XMaskEvent(disp_id, ExposureMask, &event1);

    /* move window to where I think it should be and wait for notify event */
    XGetWindowAttributes(disp_id, window2, &win_attr);
    save_mask = win_attr.your_event_mask;
    while (XCheckTypedWindowEvent(disp_id, window2, ConfigureNotify, &event));
    XSelectInput(disp_id, window2, StructureNotifyMask);
    XMoveWindow(disp_id, window2, (int)xorig, (int)yorig);
    while (!XCheckTypedWindowEvent(disp_id, window2, ConfigureNotify, &event));
    XSelectInput(disp_id, window2, save_mask);

    /* check position of window, if it's wrong move it again */
    XTranslateCoordinates(disp_id, window2, root_id,
			  0, 0, &ix1, &iy1, &junk);
    if (ix1 != xorig || iy1 != yorig)
      XMoveWindow(disp_id, window2, 2*(int)xorig-ix1, 2*(int)yorig-iy1);
    XPutBackEvent(disp_id, &event1);
  }
  window2 = XtWindow(drawingArea2);

  /* Create spectrum display window and drawingArea1 */
  if (xmg[2] != 0) {
    xorig = xmg[4]*(float)scr_width;
    yorig = xmg[5]*(float)scr_height;
    ww = xmg[6]*(float)scr_width;
    hh = xmg[7]*(float)scr_height;
  } else {
    xorig = scr_width/20 - 15;
    yorig = scr_height*3/5 - 15;
    ww = scr_width*19/20;
    hh = scr_height*2/5;
  }
  XtSetArg(args[0], XmNx, xorig+1);
  XtSetArg(args[1], XmNy, yorig-1);
  XtSetArg(args[2], XmNborderWidth, (Dimension)0);
  mainWindow1 = XtCreatePopupShell("Spectrum Window",
				   topLevelShellWidgetClass, mainWindow2, args, 3);
  XtSetArg(args[0], XmNwidth, ww);
  XtSetArg(args[1], XmNheight, hh);
  drawingArea1 = XmCreateDrawingArea(mainWindow1, "spectra", args, 2);
  XtOverrideTranslations(drawingArea1,XtParseTranslationTable(trans1)); 

  /* Manage widgets */
  XtManageChild(drawingArea1);
  XtManageChild(mainWindow1);
  if (xmg[2] != 0) {
    window1 = XtWindow(mainWindow1);
    XWindowEvent(disp_id, XtWindow(drawingArea1), ExposureMask, &event1);

    /* move window to where I think it should be and wait for notify event */
    XGetWindowAttributes(disp_id, window1, &win_attr);
    save_mask = win_attr.your_event_mask;
    while (XCheckTypedWindowEvent(disp_id, window1, ConfigureNotify, &event));
    XSelectInput(disp_id, window1, StructureNotifyMask);
    XMoveWindow(disp_id, window1, (int)xorig, (int)yorig);
    XSelectInput(disp_id, window1, save_mask);
    while (!XCheckTypedWindowEvent(disp_id, window1, ConfigureNotify, &event));
	 
    /* check position of window, if it's wrong move it again */
    XTranslateCoordinates(disp_id, window1, root_id, 0,0,&ix1,&iy1,&junk);
    if (ix1 != xorig || iy1 != yorig)
      XMoveWindow(disp_id, window1, 2*(int)xorig-ix1, 2*(int)yorig-iy1);
    XPutBackEvent(disp_id, &event1);
  }
  window1 = XtWindow(drawingArea1);

  /* Create crosshairs cursor to use on spectrum window */
  for (i = 0; i < 16; i++) {
    bigcrossBits[i*2] = 0x80;
    bigcrossBits[i*2+1] = 0x00;
  }
  bigcrossBits[16] = 0xff;
  bigcrossBits[17] = 0x7f;
  bigcrossPixmap = XCreatePixmapFromBitmapData(disp_id, window1,
					       bigcrossBits, 16,16, 1, 0, 1);
  XLookupColor(disp_id, XDefaultColormapOfScreen(XtScreen(drawingArea1)),
	       "black", &dummy, &black);
  bigcross = XCreatePixmapCursor(disp_id, bigcrossPixmap, bigcrossPixmap,
				 &black, &black, 7,8);
  if (bigcross)
    XDefineCursor(disp_id, window1, bigcross);
  XFreePixmap(disp_id, bigcrossPixmap);

  /* Set background color of drawing areas to white */
  XtSetArg(args[0], XmNbackground, WhitePixelOfScreen(XtScreen(drawingArea2)));
  XtSetValues(drawingArea1, args, 1);
  XtSetValues(drawingArea2, args, 1);

  /* Tell drawingArea1 to forget about bit gravity
     so that ALL resize events generate expose events */
  attrs.bit_gravity = ForgetGravity;
  XChangeWindowAttributes(disp_id, window1, CWBitGravity, &attrs);
  /* Tell drawingArea1 to use backing store */
  attrs.backing_store = Always;
  XChangeWindowAttributes(disp_id, window1, CWBackingStore, &attrs);

  /* Create pixmap the maximum size of drawingArea2 */
  XSync(disp_id, False);
  (void) XSetErrorHandler(pixmapErrorHandler);
  while (!pixmap) {
    printf("Trying to create pixmap of size %d by %d\n", width, height);
    pixmap = XCreatePixmap(disp_id, root_id, 4*width, 4*height,
			   DefaultDepthOfScreen(XtScreen(mainWindow2)));
    XSync(disp_id, False);
  }
  (void) XSetErrorHandler(0);

  /* Create watch cursor to use on toplevel window when program is busy */
  watchCursor = XCreateFontCursor(XtDisplay(toplevel), XC_watch);

  /* Restore input focus to original window */
  XSetInputFocus(disp_id, focus, revert_to, CurrentTime);

  /* Initialise gls etc subroutines */
  menu_mode = 1;
  select_gw(1);
  win_id  = window1;
  select_gw(2);
  win_width = width;
  win_height = height;
  win_id  = pixmap;
#ifdef XM2D
  e_gls_init(argc, argv);
#endif
#ifdef XM3D
  l_gls_init(argc, argv);
#endif
#ifdef XM4D
  fdg8r_init(argc, argv);
#endif

  /* Get and dispatch events */
  XtAppMainLoop(app_context);
  return 0;
} /* main */

void breakhandler(int dummy)
{
  elgd.gotsignal = 1;
}

void set_signal_alert(int mode, char *mesag)
{

  if (mode) {
    tell("%s\nYou can press control-C to interrupt...\n", mesag);
    elgd.gotsignal = 0;
    signal(SIGINT, breakhandler);
  } else {
    signal(SIGINT, SIG_DFL);
  }
}

int check_signal_alert(void)
{
  return elgd.gotsignal;
}
