#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ScrolledW.h>
#include <Xm/CascadeB.h>

#include "gls.h"
#include "minig.h"

/* Program to create and edit a level scheme graphically */
/* Version 3.0  (C version)     D.C. Radford  Sept 1999 */

/* external variables defined in miniga.c */
extern Display  *disp_id;
extern Screen   *screen_id;
extern Window   win_id;
extern GC       gc_id, gc_comp_id;
extern int      win_width, win_height, menu_mode;
extern int      nstored;
extern short    points[512][2];
extern int      color[20];

#define  MAX_MENU  6
#define  MAX_OPT   10

XtAppContext app_context;
Widget       toplevel, menubar, mainWindow, drawingArea, posText;
Widget       menubarBtn[MAX_MENU], pulldowns[MAX_MENU];
Widget       pdmenuBtn[MAX_MENU][MAX_OPT];
Pixmap       pixmap;
Window       window, root_id;
Display      *display;
GC           xmgc;
Arg          args [10];
Position     xorig, yorig;
Dimension    ww, hh, width = 600, height = 500;
int          zoom = 1, revert_to, scr_width, scr_height;
float        xin, yin, xmg[4];
char         cin[40];

#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"

int set_xwg(char *cstr)
{
  FILE *fp;
  char filename[80], line[120];

  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xmg");
#else
  strcat (filename, "/.radware.xmg");
#endif
  fp = fopen(filename, "r+");
  if (!fp) return 1;

  while (fgets(line, 120, fp)) {
    if (!strncmp(line,cstr,8)) {
      sscanf((char *) strchr(line,' '), " %f%f%f%f",
	     &xmg[0], &xmg[1], &xmg[2], &xmg[3]);
    }
  }
  fclose(fp);
  return 0;
} /* set_xwg */

int save_xwg(char *cstr)
{
  FILE         *fp;
  char         filename[80], line[20][120];
  int          lineNum = 0, done = 0, i;
  Window       junk;
  int          ix, iy;
  unsigned int w, h, junk_border, junk_depth;

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

  XGetGeometry(display, XtWindow(toplevel), &junk, &ix, &iy,
	       &w, &h, &junk_border, &junk_depth);
  XTranslateCoordinates(display, XtWindow(toplevel), root_id,
			0, 0, &ix, &iy, &junk);
  xmg[0] = (float)ix / (float)scr_width;
  xmg[1] = (float)iy / (float)scr_height;
  xmg[2] = (float)w  / (float)scr_width;
  xmg[3] = (float)h  / (float)scr_height;
  
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
    if (!strncmp(line[i],cstr,8)) {
      done = 1;
      if (line[i][8] == ' ')
	sprintf(&line[i][8], " %7.5f %7.5f %7.5f %7.5f\n",
		xmg[0], xmg[1], xmg[2], xmg[3]);
    }
    fprintf(fp, "%s", line[i]);
  }
  if (done == 0) {
    strncpy(line[0], cstr, 8);
    line[0][8] = '\0';
    fprintf(fp, "%s %7.5f %7.5f %7.5f %7.5f\n", line[0],
	    xmg[0], xmg[1], xmg[2], xmg[3]);
  }
  fclose(fp);
  return 0;
} /* save_xwg */

/* ======================================================================= */
int set_size(void)
{

  win_width = (int) width;
  win_height = (int) height;
  display_gls(-1);
  return 0;
} /* set_size */

/* Subroutine to copy pixmap to drawing area */
void copy_area()
{
  if (zoom < 8) {
    XCopyArea(display, pixmap, window, xmgc,
	      0, 0, width, height, 0, 0);
    XSync(display, False);
  }
} /* copy_area */

/* Subroutine to clear pixmap and drawing area */
int erase(void)
{
  if (zoom < 8) {
    XFillRectangle(display, pixmap, xmgc, 0, 0, width, height);
    copy_area();
  } else {
    XClearWindow(display, window);
  }
  return 0;
} /* erase */

/* Callback for the drawing area expose event */
void DAexposeCB(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
  if (zoom < 8) {
    copy_area();
  } else {
    display_gls(-1);
  }
} /* DAexposeCB */

/* Callback for the drawing area input event */
void DAinputCB(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
  static int   incr = 8, control_yes = 0;
  Window       jroot, jchild;
  int          ix, iy, ixi, iyi;
  unsigned int state, iflag;
  char         response[40];

  XmProcessTraversal(w, XmTRAVERSE_CURRENT);

  ixi = event->xbutton.x;
  iyi = height - event->xbutton.y;
  cvxy(&xin, &yin, &ixi, &iyi, 2);

  if (event->xany.type == ButtonPress) {
    if ((event->xbutton.state & ShiftMask) != 0) {
      /* shift-mousebutton -> pan/zoom gls display */
      iflag = 1;
      if (event->xbutton.button == Button2) iflag = 2;
      if (event->xbutton.button == Button3) iflag = 3;
      pan_gls(xin, yin, iflag);
    } else {
      strcpy(cin, "G ");
      if (event->xbutton.button == Button2) strcpy(cin, "X2");
      if (event->xbutton.button == Button3) strcpy(cin, "X3");
    }

  } else if (event->xany.type == KeyRelease) {
    strcpy(response, XKeysymToString(XLookupKeysym((XKeyEvent *)event,0)));
    if (!strncmp(response, "Con", 3)) control_yes = 0;

  } else if (event->xany.type == KeyPress) {
    strcpy(response, XKeysymToString(XLookupKeysym((XKeyEvent *)event,0)));
    if (response[1] == 0) strcpy(&response[1], "   ");
    /* check for <select> or <tab> key */
    if ((strncmp(response, "Tab", 3) == 0) ||
	(strncmp(response, "Sel", 3) == 0)) {
      if (incr == 1) incr = 8;
      else incr = 1;

    /* check for arrow keys and move cursor appropriately */
    } else if (!strncmp(response, "Up", 2) ||
	       !strncmp(response, "Down", 4) ||
	       !strncmp(response, "Left", 4) ||
	       !strncmp(response, "Right", 5)) {
      (void) XQueryPointer(display, window, &jroot, &jchild,
			   &ix, &iy, &ixi, &iyi, &state);
      ix = ixi;
      iy = iyi;
      if      (!strncmp(response, "Up", 2))    iyi = iyi - incr;
      else if (!strncmp(response, "Down", 4))  iyi = iyi + incr;
      else if (!strncmp(response, "Left", 4))  ixi = ixi - incr;
      else if (!strncmp(response, "Right", 5)) ixi = ixi + incr;
      XWarpPointer(display, window, window, ix, iy, width, height, ixi, iyi);

    } else if (!strncmp(response, "Con", 3)) {
      control_yes = 1;

    } else if (control_yes && *response == 'u') {
      undo_gls(-1);
      path(1);
    } else if (control_yes && *response == 'r') {
      undo_gls(1);
      path(1);

    } else if (strncmp(response, "Alt", 3) &&
	       strncmp(response, "Mul", 3) &&
	       strncmp(response, "Cap", 3) &&
	       strncmp(response, "Con", 3) &&
	       strncmp(response, "Shi", 3) &&
	       strncmp(response, "Del", 3)) strcpy(cin, response);
  }
} /* DAinputCB */

/* Callback for the pushbuttons in the menubar */
void ButtonCB (Widget w, XtPointer client_data, XtPointer call_data)
{
  XmString    label;
  static char title[40][50];
  static int  titleNum = 0;
  int         i;
  char        *btnLabel, pCommand[80];

  static int focus = 0;
  extern int get_focus(int *);
  extern int set_focus(int);

  strcpy (pCommand, client_data);
  if (titleNum == 0) strcpy(title[titleNum], "XmGLS");

  if (strncmp(pCommand, "zi", 2) == 0) {
    /* Zoom in */
    if (zoom < 16) {
      zoom = zoom*2;
      width = width*2;
      height = height*2;
    }
    XtSetArg(args[0], XmNwidth, width);
    XtSetArg(args[1], XmNheight, height);
    XtSetValues(drawingArea, args, 2);
    if (zoom < 8) win_id = (Window) pixmap;
    else          win_id = window;
    set_size();

  } else if (strncmp(pCommand, "zo", 2) == 0) {
    /* Zoom out */
    if (zoom > 1) {
      zoom = zoom/2;
      width = width/2;
      height = height/2;
    }
    XtSetArg(args[0], XmNwidth, width);
    XtSetArg(args[1], XmNheight, height);
    XtSetValues(drawingArea, args, 2);
    if (zoom < 8) win_id = (Window) pixmap;
    else          win_id = window;
    set_size();

  } else {
    /* get pulldown menu button label and use it as window title */
    if (titleNum == 39) {
      titleNum--;
      for (i=0; i<titleNum; i++) strcpy(title[i], title[i+1]);
    }
    if (titleNum > 9) {
      printf(" * Warning - you should not nest so many commands.\n");
      printf(" * Consider exiting from more previous modes\n");
      printf(" *     before entering new ones.\n");
    }
    titleNum++;
    XtSetArg(args[0], XmNlabelString, &label);
    XtGetValues(w, args, 1);
    XmStringGetLtoR(label, XmSTRING_DEFAULT_CHARSET, &btnLabel);
    XmStringFree(label);
    strcpy(title[titleNum], "XmGLS -- ");
    strcpy(&title[titleNum][9], btnLabel);
    XtFree(btnLabel);
    XStoreName(display, XtWindow(toplevel), title[titleNum]);

    /* Execute command */
    /* printf ("command: %d %s\n", strlen(pCommand), pCommand); */
    if (strlen(pCommand)) {
      if (!focus) get_focus(&focus);
      gls_exec(pCommand);
      set_focus(focus);
    }
    /* restore last window title */
    titleNum--;
    XStoreName(display, XtWindow(toplevel), title[titleNum]);
  }
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
	   XmStringCreate(ButtonText, XmSTRING_DEFAULT_CHARSET));
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

/* ======================================================================= */
void ReportPointerPos(Widget w, XMotionEvent *event,
		      String *params, Cardinal *nparams)
{
  float   x1, y1;
  int     ilev, igam, ixi, iyi;
  extern int nearest_gamma(float, float);
  extern int nearest_level(float, float);
  char    buf[120];

  ixi = event->x; iyi = height - event->y;
  cvxy(&x1, &y1, &ixi, &iyi, 2);
  sprintf(buf, "y = %.0f", y1);
  igam = nearest_gamma(x1, y1);
  if (igam >= 0) {
    sprintf(buf + strlen(buf), "    Gamma: %.1f keV,  I = %.2f",
	    glsgd.gam[igam].e, glsgd.gam[igam].i);
  }
  ilev = nearest_level(x1, y1);
  if (ilev >= 0) {
    sprintf(buf + strlen(buf), "    Level: %s,  E = %.1f",
	    glsgd.lev[ilev].name, glsgd.lev[ilev].e);
  }
  XmTextSetString(posText, buf);
} /* ReportPointerPos */

/***** Main Logic *****/
int main(int argc, char **argv)
{
  String trans = 
    "<MotionNotify>: ReportPointerPos() \n"
    "<Expose>: DAexposeCB() \n"
    "<ButtonPress>: DAinputCB() \n"
    "<KeyPress>: DAinputCB() \n"
    "<KeyRelease>: DAinputCB()";
  static XtActionsRec win_actions[] = { 
    {"ReportPointerPos", (XtActionProc) ReportPointerPos},
    {"DAexposeCB",       (XtActionProc) DAexposeCB},
    {"DAinputCB",        (XtActionProc) DAinputCB} };
  Widget            scrolled, undoButton, redoButton;
  XGCValues         gcv;
  XWindowAttributes win_attr;
  Window            focus, junk;
  XEvent            event1, event;
  char              buf[80], *c, *input_file_name;
  int               w_in, h_in, ix1, iy1;
  long              save_mask;

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

  /* Create main window and menubar */
  toplevel = XtAppInitialize (&app_context, "GraphicsWindow", NULL, 0,
			      &argc, argv, NULL, NULL, 0);
  mainWindow = XmCreateForm(toplevel, "MainWindow", args, 0);
  XtSetArg(args[0], XmNtopAttachment, XmATTACH_FORM);
  XtSetArg(args[1], XmNrightAttachment, XmATTACH_FORM);
  XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
  menubar = XmCreateMenuBar(mainWindow, "menubar", args, 3);

  /* Create Pulldown MenuPanes and cascade buttons in menubar */
  create_pdm(0,"pd0","Files  ",'F');
  create_pdm(1,"pd1","Display  ",'D');
  create_pdm(2,"pd2","Bands  ",'B');
  create_pdm(3,"pd3","Gammas  ",'G');
  create_pdm(4,"pd4","Levels  ",'L');
  create_pdm(5,"pd5","Text  ",'T');
  XtManageChildren(menubarBtn, 6);

  /* Create pushbuttons in pulldown menus */
  create_pdm_sel(0,0,"b0.0","Create Postscript Hardcopy File",'P',"hc");
  create_pdm_sel(0,1,"b0.1","Save Level Scheme in New File",'L',"sl");
  create_pdm_sel(0,2,"b0.2","Read New .gls Level Scheme File",'N',"rg");
  create_pdm_sel(0,3,"b0.3","Add a second .gls level scheme file",'A',"ad");
  create_pdm_sel(0,4,"b0.4","Test gamma-ray energy and intensity sums",'T',"ts");
  create_pdm_sel(0,5,"b0.5","Undo level scheme edit (C-u)",'U',"ue");
  create_pdm_sel(0,6,"b0.6","Redo level scheme edit (C-r)",'R',"re");
  create_pdm_sel(0,7,"b0.7","Exit program",'x',"st");
  XtManageChildren(pdmenuBtn[0], 8);

  create_pdm_sel(1,0,"b1.0","Zoom In",'I',"zi");
  create_pdm_sel(1,1,"b1.1","Zoom Out",'O',"zo");
  create_pdm_sel(1,2,"b1.2","Redraw Level Scheme",'R',"rd");
  create_pdm_sel(1,3,"b1.3","Redraw Entire Level Scheme",'E',"rd1");
  create_pdm_sel(1,4,"b1.4","Expand Level Scheme",'x',"ex");
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
  create_pdm_sel(3,7,"b3.7","Examine Gammas",'G',"xg");
  XtManageChildren(pdmenuBtn[3], 8);

  create_pdm_sel(4,0,"b4.0","Add Levels",'A',"al");
  create_pdm_sel(4,1,"b4.1","Delete Levels",'D',"dl");
  create_pdm_sel(4,2,"b4.2","Edit Levels",'E',"el");
  create_pdm_sel(4,3,"b4.3","Move Levels Between Bands",'M',"ml");
  create_pdm_sel(4,4,"b4.4","Move Spin Labels of Levels",'S',"ll");
  create_pdm_sel(4,5,"b4.5","Move Energy Labels of Levels",'L',"le");
  create_pdm_sel(4,6,"b4.6","Toggle Tentative/Thick/Normal Levels",'T',"tl");
  create_pdm_sel(4,7,"b4.7","Change Widths of Levels",'W',"lw");
  create_pdm_sel(4,8,"b4.8","Fit Energies of Levels",'F',"fl");
  create_pdm_sel(4,9,"b4.9","Move Connected Levels Up/Down in Energy",'U',"ud");
  XtManageChildren(pdmenuBtn[4], 10);

  create_pdm_sel(5,0,"b5.0","Add Text Labels",'A',"at");
  create_pdm_sel(5,1,"b5.1","Delete Text Labels",'D',"dt");
  create_pdm_sel(5,2,"b5.2","Edit Text Labels",'E',"et");
  create_pdm_sel(5,3,"b5.3","Move Text Labels",'M',"mt");
  create_pdm_sel(5,4,"b5.4","Change Level Spin Labels",'S',"cl");
  create_pdm_sel(5,5,"b5.5","Change Level Energy Labels",'L',"ce");
  create_pdm_sel(5,6,"b5.6","Change Gamma Labels",'G',"cg");
  XtManageChildren(pdmenuBtn[5], 7);

  /* Create pixmap the maximum size of drawing area */
  display = XtDisplay(mainWindow);
  root_id = RootWindowOfScreen(XtScreen(mainWindow));
  XSync(display, False);
  XSetErrorHandler(pixmapErrorHandler);
  while (!pixmap) {
    printf("Trying to create pixmap of size %d by %d\n", width, height);
    pixmap = XCreatePixmap(display, root_id, 4*width, 4*height,
			   DefaultDepthOfScreen(XtScreen(mainWindow)));
    XSync(display, False);
  }
  XSetErrorHandler(0);

  /* Create undo and redo buttons */
  XtSetArg(args[0], XmNvalue, "UnDo");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_FORM);
  XtSetArg(args[2], XmNbottomAttachment, XmATTACH_FORM);
  undoButton = XmCreatePushButton(mainWindow, "undo", args, 3);
  XtAddCallback(undoButton, XmNactivateCallback, ButtonCB, "ue");
  XtSetArg(args[0], XmNvalue, "ReDo");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNleftWidget, undoButton);
  XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
  redoButton = XmCreatePushButton(mainWindow, "redo", args, 4);
  XtAddCallback(redoButton, XmNactivateCallback, ButtonCB, "re");

  /* Create text widget */
  XtSetArg(args[0], XmNvalue, " ");
  XtSetArg(args[1], XmNleftAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNleftWidget, redoButton);
  XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
  XtSetArg(args[4], XmNrightAttachment, XmATTACH_FORM);
  posText = XmCreateText(mainWindow, "posText", args, 5);

  /* Create drawingArea widget for level scheme */
  XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
  XtSetArg(args[1], XmNtopAttachment, XmATTACH_WIDGET);
  XtSetArg(args[2], XmNtopWidget, menubar);
  XtSetArg(args[3], XmNrightAttachment, XmATTACH_FORM);
  XtSetArg(args[4], XmNleftAttachment, XmATTACH_FORM);
  XtSetArg(args[5], XmNbottomAttachment, XmATTACH_WIDGET);
  XtSetArg(args[6], XmNbottomWidget, posText);
  XtSetArg(args[7], XmNheight, (int)height*9/10);
  scrolled = XmCreateScrolledWindow(mainWindow, "scrolled", args, 8);
  XtSetArg(args[0], XmNwidth, width);
  XtSetArg(args[1], XmNheight, height);
  drawingArea = XmCreateDrawingArea(scrolled, "levelscheme", args, 2);
  XtOverrideTranslations(drawingArea,XtParseTranslationTable(trans)); 
  XtAppAddActions(app_context, win_actions, XtNumber(win_actions));

  /* Set size and position of main window */
  scr_width  = XWidthOfScreen(XtScreen(toplevel));
  scr_height = XHeightOfScreen(XtScreen(toplevel));
  xmg[2] = 0;
  set_xwg("gls_std_");
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
  }

  /* Create graphics context */
  gcv.foreground = WhitePixelOfScreen(XtScreen(drawingArea));
  xmgc = XCreateGC(display, root_id, GCForeground, &gcv);

  /* Set background color to white */
  XtSetArg(args[0], XmNbackground, WhitePixelOfScreen(XtScreen(drawingArea)));
  XtSetValues(drawingArea, args, 1);

  /* Save ID of window that has input focus */
  XGetInputFocus(display, &focus, &revert_to);

  /* Manage widgets */
  XtManageChild(menubar);
  XtManageChild(drawingArea);
  XtManageChild(undoButton);
  XtManageChild(redoButton);
  XtManageChild(posText);
  XtManageChild(scrolled);
  XtManageChild(mainWindow);
  XtRealizeWidget(toplevel);
  if (xmg[2] != 0) {
    window = XtWindow(toplevel);
    XMaskEvent(display, ExposureMask, &event1);

    /* move window to where I think it should be and wait for notify event */
    XGetWindowAttributes(display, window, &win_attr);
    save_mask = win_attr.your_event_mask;
    while (XCheckTypedWindowEvent(display, window, ConfigureNotify, &event));
    XSelectInput(display, window, StructureNotifyMask);
    XMoveWindow(display, window, (int)xorig, (int)yorig);
    while (!XCheckTypedWindowEvent(display, window, ConfigureNotify, &event));
    XSelectInput(display, window, save_mask);

    /* check position of window, if it's wrong move it again */
    XTranslateCoordinates(display, window, root_id,
			  0, 0, &ix1, &iy1, &junk);
    if (ix1 != xorig || iy1 != yorig)
      XMoveWindow(display, window, 2*(int)xorig-ix1, 2*(int)yorig-iy1);
    XPutBackEvent(display, &event1);
  }
  window = XtWindow(drawingArea);

  /* Restore input focus to original window */
  XSync(display, False);
  XSetInputFocus(display, focus, revert_to, CurrentTime);

  /* Initialise gls subroutines */
  input_file_name = "";
  if (argc>1) {
    input_file_name = argv[1];
    printf("\nInput file name: %s\n", input_file_name);
  }
  menu_mode = 1;
  disp_id = display;
  win_id = (Window) pixmap;
  gc_id = xmgc;
  win_width = (int) width;
  win_height = (int) height;
  gls_init(input_file_name);

  /* Get and dispatch events */
  XtAppMainLoop(app_context);
  return 0;
}

int retic2(float *xout, float *yout, char *cout)
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

  /* set input focus to required window */
   XSetInputFocus(display, window, revert_to, CurrentTime);

  /* process events until we get a valid input event in the drawing area */
  bell();
  cin[0] = 0;
  while (cin[0] == 0) XtAppProcessEvent(app_context, XtIMAll);

  /* restore incoming addresses and return values */
  callCount--;
  xout = ixp[callCount];
  yout = iyp[callCount];
  cout = icp[callCount];
  strncpy(cout, cin, 4);
  *xout = xin;
  *yout = yin;
  cin[0] = 0;
  return 0;
} /* retic */

/* ======================================================================= */
int initg2(int *nx, int *ny)
{
  XGCValues     xgcvl;
  static int    first = 1;
  int           font, black, white;

  if (first) {
    first = 0;
    /* Next instruction for debugging only */
    /* XSynchronize(disp_id, 1); */

    screen_id =  XDefaultScreenOfDisplay(disp_id);
    black =      BlackPixelOfScreen(screen_id);
    white =      WhitePixelOfScreen(screen_id);

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

    /* Create graphics context */
    xgcvl.background = color[1];
    xgcvl.foreground = color[2];
    gc_id = XCreateGC(disp_id, win_id, GCForeground | GCBackground, &xgcvl);
    xgcvl.function = GXinvert;
    gc_comp_id = XCreateGC(disp_id, win_id, GCFunction, &xgcvl);
    XSetFont(disp_id, gc_id, font);
  }
  *nx = win_width - 5;
  *ny = win_height - 25;
  return 0;
} /* initg2 */

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
  return 0;
} /* finig */

/* ======================================================================= */
int setcolor(int icol)
{
  static int j;
  GC tmp;

  /* call finig to dump stored plot array
     before changing attribute block */
  finig();

  if (icol == -1) {
    /* interchange the graphics contexts to use do hiliting */
    tmp = gc_id;
    gc_id = gc_comp_id;
    gc_comp_id = tmp;
   return 0;
  }
  j = icol + 1;
  if (j < 1 || j > 16) return 1;
  XSetForeground(disp_id, gc_id, color[j]);
  return 0;
} /* setcolor */
