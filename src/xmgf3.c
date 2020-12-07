#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/Command.h>
#include <Xm/CascadeB.h>
#include <Xm/MessageB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Separator.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>

#include "util.h"

struct {
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

extern FILE *scrfilea, *scrfileb;
extern FILE *infile;    /* command file descriptor*/
extern int  menu_mode;

#define  MAX_MENU  10
#define  MAX_OPT   12
#define  MAX_DRB   24
#define  MAX_FFB   114
#define  MAX_FILE  6
#define  LAST      0
XtAppContext app_context;
Widget toplevel, form, menubar, command, CmndHistory;
Widget menubarBtn[MAX_MENU], pulldowns[MAX_MENU], separator[12];
Widget pdmenuBtn[MAX_MENU][MAX_OPT];
Widget drm, drmButton[MAX_DRB];
Widget ffMenu, ffButton[MAX_FFB];
Cursor watchCursor;

char   YNreply, *MenuReply, gf3Cmnd[80], DSRcmnd[80];
Arg    args [10];

void        DSRegion(), fileSelect(), fixFreePars();

extern int gfexec(char *, int), gfinit(int argc, char **argv);
extern int get_focus(int *), set_focus(int);

int caskyn(char *mesag)
{
  char menuyn(char *);
  char ans[40];

  if (!infile) {
    ans[0] = menuyn(mesag);
    ans[1] = '\0';
    /* if log command file open, copy response */
    log_to_cmd_file(ans);
    if (ans[0] == 'Y' || ans[0] == 'y') return 1;
    return 0;
  }

  while (cask(mesag, ans, 1)) {
    if (ans[0] == 'N' || ans[0] == 'n' || ans[0] == '0') return 0;
    if (ans[0] == 'Y' || ans[0] == 'y' || ans[0] == '1') return 1;
  }
  return 0;
} /* caskyn */

void xmgf3exec()
{
  int nc;

  XDefineCursor(XtDisplay(toplevel), XtWindow(form), watchCursor);
  XFlush(XtDisplay(toplevel));
  /* if log command file open, copy command to file */
  log_to_cmd_file(gf3Cmnd);

  gfexec(gf3Cmnd, strlen(gf3Cmnd));
  while (infile) {
    printf ("infile = true...\n");
    if ((nc = cask("?", gf3Cmnd, 80)) > 1) gfexec(gf3Cmnd, nc);
  }
  XUndefineCursor(XtDisplay(toplevel), XtWindow(form));
  set_focus(XtWindow(form));
}

void ButtonCB(Widget w, caddr_t client_data,
	      XmCommandCallbackStruct *call_data)
{
  /* Callback for the pushbuttons in the pulldown menu
     w           widget id
     client_data data from application
     call_data   data from widget class */
  XmString UserCommand;
      
  /* client_data contains command to execute */
  strcpy(gf3Cmnd,client_data);

  /* if necessary, get region of screen for spectrum display */
  if (!strncmp(gf3Cmnd, "ds", 2) || !strncmp(gf3Cmnd, "ov", 2)) {
    DSRegion();
    strcpy(DSRcmnd,MenuReply);
    gf3Cmnd[3] = DSRcmnd[0];
    gf3Cmnd[5] = DSRcmnd[2];
  }
  /* for commands that use files, get file name from file selection box */
  if ((strncmp(gf3Cmnd, "cf  ", 4)) == 0) fileSelect(0);
  if ((strncmp(gf3Cmnd, "in  ", 4)) == 0) fileSelect(1);
  if ((strncmp(gf3Cmnd, "de  ", 4)) == 0) fileSelect(2);
  if ((strncmp(gf3Cmnd, "sp  ", 4)) == 0) fileSelect(3);
  if ((strncmp(gf3Cmnd, "dv  ", 4)) == 0) fileSelect(3);
  if ((strncmp(gf3Cmnd, "lu  ", 4)) == 0) fileSelect(4);
  if ((strncmp(gf3Cmnd, "sl  ", 4)) == 0) fileSelect(5);

  /* if necessary, fix and/or free parameters of fit */
  if (!strncmp(gf3Cmnd, "fx", 2)) fixFreePars();

  /* copy command to command history and execute */
  if (strncmp(gf3Cmnd, "  ", 2)) {
    UserCommand = XmStringCreate(gf3Cmnd,XmSTRING_DEFAULT_CHARSET);
    /* UserCommand = XmStringCreate(gf3Cmnd,XmFONTLIST_DEFAULT_TAG); */
    XmListAddItem(CmndHistory, UserCommand, LAST);
    XmStringFree(UserCommand);
    xmgf3exec();
  }
}

void CommandCB(Widget w, XtPointer client_data,
	       XmCommandCallbackStruct *call_data)
{
  /* Callback for the command line widget
     w           widget id
     client_data data from application
     call_data   data from widget class */
  char *text;

  /* get command from text widget */
  if (XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &text)) {
    strcpy(gf3Cmnd,text);
    XtFree(text);
    xmgf3exec();
  }
}

void create_pdm(int pd, char *pdname, char *ButtonText, char ButtonMnemonic)
{
  /* subroutine to create pulldown menu panes and cascade buttons */

  pulldowns[pd] = XmCreatePulldownMenu(menubar, pdname, NULL, 0);
  XtSetArg(args[0], XmNsubMenuId, pulldowns[pd]);
  XtSetArg(args[1], XmNlabelString,
	   XmStringCreate(ButtonText, XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[2], XmNmnemonic, ButtonMnemonic);
  menubarBtn[pd] = XmCreateCascadeButton(menubar, ButtonText, args, 3);
}

/* subroutine to create pulldown menu entries and buttons */
void create_pdm_sel(int pd, int pdb, char *pdbname, char *ButtonText,
		    char ButtonMnemonic, char *ReturnText)
{
  /* subroutine to create pulldown menu entries and buttons */
  XtSetArg(args[0], XmNlabelString,
	   XmStringCreate(ButtonText, XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[1], XmNmnemonic, ButtonMnemonic);
  pdmenuBtn[pd][pdb] = XmCreatePushButtonGadget(pulldowns[pd],
						pdbname, args, 2);
  XtAddCallback(pdmenuBtn[pd][pdb], XmNactivateCallback,
		(XtCallbackProc) ButtonCB, (XtPointer) ReturnText);
}

int main(int argc, char **argv)
{
  int  nc;

  /* Initialize toolkit and create form and menubar */
  toplevel = XtAppInitialize (&app_context, "PulldownMenu", NULL, 0,
			      &argc, argv, NULL, NULL, 0);
  form = XmCreateForm(toplevel, "form", (ArgList) args, 0);
  XtSetArg(args[0], XmNtopAttachment, XmATTACH_FORM);
  XtSetArg(args[1], XmNrightAttachment, XmATTACH_FORM);
  XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
  menubar = XmCreateMenuBar(form, "menubar", args, 3);

  /* Create Pulldown MenuPanes and cascade buttons in menubar */
  create_pdm(0,"pd0","Files  ",'F');
  create_pdm(1,"pd1","Spectrum  ",'S');
  create_pdm(2,"pd2","Display  ",'D');
  create_pdm(3,"pd3","Axes  ",'A');
  create_pdm(4,"pd4","Fit   ",'t');
  create_pdm(5,"pd5","Info  ",'I');
  create_pdm(6,"pd6","Help",'H');

  /* Help button is special */
  XtSetArg(args[0], XmNmenuHelpWidget, (XtArgVal)menubarBtn[6]);
  XtSetValues(menubar, args, 1);

  /* Create pushbuttons in pulldown menus */
  create_pdm_sel(0,0,"b0.0","Dump Setup",'D',"du");
  create_pdm_sel(0,1,"b0.1","Indump Setup",'I',"in  ");
  separator[0] = XmCreateSeparator(pulldowns[0], "s0", args, 0);
  create_pdm_sel(0,2,"b0.2","Open Lookup Table",'L',"lu  ");
  create_pdm_sel(0,3,"b0.3","Open Slice Window File",'S',"sl  ");
  separator[1] = XmCreateSeparator(pulldowns[0], "s1", args, 0);
  create_pdm_sel(0,4,"b0.4","Execute Command File",'E',"cf  ");
  create_pdm_sel(0,5,"b0.5","Create Command File",'C',"cf log");
  create_pdm_sel(0,6,"b0.6","Close Command File",'o',"cf end");
  create_pdm_sel(0,7,"b0.7","Insert Check in CF",'h',"cf chk");
  create_pdm_sel(0,8,"b0.8","Continue CF",'n',"cf con");
  separator[2] = XmCreateSeparator(pulldowns[0], "s2", args, 0);
  create_pdm_sel(0,9,"b0.9","Exit GF3",'x',"st");
  XtManageChildren(pdmenuBtn[0], 10);
  XtManageChildren(&separator[0], 3);

  create_pdm_sel(1,0,"b1.0","Read Spectrum",'R',"sp  ");
  create_pdm_sel(1,1,"b1.1","Slice Matrix with Cursor",'S',"sp/c");
  separator[3] = XmCreateSeparator(pulldowns[1], "s3", args, 0);
  create_pdm_sel(1,2,"b1.2","Write Spectrum",'W',"ws");
  create_pdm_sel(1,3,"b1.3","Add Spectrum",'A',"as");
  create_pdm_sel(1,4,"b1.4","Multiply Spectrum",'M',"ms");
  create_pdm_sel(1,5,"b1.5","Divide Spectrum",'D',"dv  ");
  create_pdm_sel(1,6,"b1.6","Adjust Gain of Spectrum",'G',"ag");
  create_pdm_sel(1,7,"b1.7","Contract Spectrum",'C',"ct");
  create_pdm_sel(1,8,"b1.8","Set Counts with Cursor",'e',"sc");
  create_pdm_sel(1,9,"b1.9","Add Counts to Spectrum",'d',"ac");
  create_pdm_sel(1,10,"b1.10","Divide by Efficiency",'f',"de  ");
  create_pdm_sel(1,11,"b1.11","Set AutoBackground",'B',"bg  ");
  XtManageChildren(pdmenuBtn[1], 12);
  XtManageChildren(&separator[3], 1);

  create_pdm_sel(2,0,"b2.0","Display Spectrum",'D',"ds 1 1");
  create_pdm_sel(2,1,"b2.1","Overlay Spectrum",'O',"ov 1 1");
  create_pdm_sel(2,2,"b2.2","Display Whole Spectrum",'i',"ds 1 1 1");
  create_pdm_sel(2,3,"b2.3","Overlay Whole Spectrum",'v',"ov 1 1 1");
  separator[4] = XmCreateSeparator(pulldowns[2], "s4", args, 0);
  create_pdm_sel(2,4,"b2.4","Redraw Spectra",'R',"rd");
  create_pdm_sel(2,5,"b2.5","Redraw with Autoscale",'e',"rd 1");
  separator[5] = XmCreateSeparator(pulldowns[2], "s5", args, 0);
  create_pdm_sel(2,6,"b2.6","Display Fit",'F',"df");
  create_pdm_sel(2,7,"b2.7","Display Markers",'M',"dm");
  create_pdm_sel(2,8,"b2.8","Display Windows",'W',"dw");
  separator[6] = XmCreateSeparator(pulldowns[2], "s6", args, 0);
  create_pdm_sel(2,9,"b2.9","Change Colour Map",'C',"co");
  create_pdm_sel(2,10,"b2.10","Make Hardcopy of Display",'H',"hc");
  XtManageChildren(pdmenuBtn[2], 11);
  XtManageChildren(&separator[4], 3);

  create_pdm_sel(3,0,"b3.0","Expand Display with Cursor",'E',"ex");
  create_pdm_sel(3,1,"b3.1","Autoscale X-axis",'A',"nx 0");
  create_pdm_sel(3,2,"b3.2","Move Up",'U',"mu");
  create_pdm_sel(3,3,"b3.3","Move Down",'D',"md");
  create_pdm_sel(3,4,"b3.4","Change X0, NX",'X',"xa");
  separator[7] = XmCreateSeparator(pulldowns[3], "s7", args, 0);
  create_pdm_sel(3,5,"b3.5","Autoscale Y-axis",'u',"ny 0");
  create_pdm_sel(3,6,"b3.6","Change Y0, NY",'Y',"ya");
  create_pdm_sel(3,7,"b3.7","Linear/Log Y-axis",'L',"ny -4");
  XtManageChildren(pdmenuBtn[3], 8);
  XtManageChildren(&separator[7], 1);

  create_pdm_sel(4,0,"b4.0","Setup New Fit",'N',"nf");
  create_pdm_sel(4,1,"b4.1","Do Current Fit",'F',"ft");
  create_pdm_sel(4,2,"b4.2","Auto Fit",'A',"af");
  separator[8] = XmCreateSeparator(pulldowns[4], "s8", args, 0);
  create_pdm_sel(4,3,"b4.3","Reset Free Parameters",'R',"rf");
  create_pdm_sel(4,4,"b4.4","List Parameters",'L',"lp");
  create_pdm_sel(4,5,"b4.5","Store Areas & Centroids",'S',"sa");
  separator[9] = XmCreateSeparator(pulldowns[4], "s9", args, 0);
  create_pdm_sel(4,6,"b4.6","Fix/Free Parameters",'x',"fx");
  create_pdm_sel(4,7,"b4.7","Add Peak",'P',"ap");
  create_pdm_sel(4,8,"b4.8","Delete Peak",'D',"dp");
  create_pdm_sel(4,9,"b4.9","Change Markers",'M',"ma");
  create_pdm_sel(4,10,"b4.10","Change Starting Width",'W',"sw");
  create_pdm_sel(4,11,"b4.11","Change Weight Mode",'C',"wm");
  XtManageChildren(pdmenuBtn[4], 12);
  XtManageChildren(&separator[8], 2);

  create_pdm_sel(5,0,"b5.0","Call Cursor",'C',"cr");
  create_pdm_sel(5,1,"b5.1","Sum Counts with Cursor",'S',"su");
  create_pdm_sel(5,2,"b5.2","Sum Counts with Bkgnd Sub",'B',"sb");
  create_pdm_sel(5,3,"b5.3","Auto Peak Integration",'P',"pk");
  separator[10] = XmCreateSeparator(pulldowns[5], "s10", args, 0);
  create_pdm_sel(5,4,"b5.4","Setup Peak Find",'F',"pf");
  create_pdm_sel(5,5,"b5.5","Setup Energy Calibration",'E',"ec");
  create_pdm_sel(5,6,"b5.6","Add Window to LU/Slice file",'W',"wi");
  separator[11] = XmCreateSeparator(pulldowns[5], "s11", args, 0);
  create_pdm_sel(5,7,"b5.7","Auto Calibrate Spectrum (1)",'1',"ca");
  create_pdm_sel(5,8,"b5.8","Auto Calibrate Spectrum (3)",'3',"ca3");
  create_pdm_sel(5,9,"b5.9","Auto Calibrate Spectrum (4)",'4',"ca4");
  XtManageChildren(pdmenuBtn[5], 10);
  XtManageChildren(&separator[10], 2);

  create_pdm_sel(6,0,"b6.0","on Program",'P',"he int");
  create_pdm_sel(6,1,"b6.1","on Commands",'C',"he");
  XtManageChildren(pdmenuBtn[6], 2);

  /* Create command widget */
  XtSetArg(args[0], XmNtopAttachment, XmATTACH_WIDGET);
  XtSetArg(args[1], XmNtopWidget, menubar);
  XtSetArg(args[2], XmNbottomAttachment, XmATTACH_FORM);
  XtSetArg(args[3], XmNrightAttachment, XmATTACH_FORM);
  XtSetArg(args[4], XmNleftAttachment, XmATTACH_FORM);
  XtSetArg(args[5], XmNpromptString,
	   XmStringCreate("Use menu or enter commands here:",
			  XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[6], XmNhistoryVisibleItemCount, 6);
  command = XmCreateCommand(form, "command", args, 7);
  XtAddCallback(command, XmNcommandEnteredCallback, 
		(XtCallbackProc) CommandCB, NULL);
  CmndHistory = XmCommandGetChild(command,XmDIALOG_HISTORY_LIST);

  /* manage widgets */
  XtManageChildren(menubarBtn, 7);
  XtManageChild(menubar);
  XtManageChild(command);
  XtManageChild(form);
  XtRealizeWidget(toplevel);

  /* Create watch cursor to use on toplevel window when gf3 is busy */
  watchCursor = XCreateFontCursor(XtDisplay(toplevel), XC_watch);

  /* Welcome and get initial data */
  gfinit(argc, argv);
  while (infile) {
    if ((nc = cask("?", gf3Cmnd, 80)) > 1) gfexec(gf3Cmnd, nc);
  }
  menu_mode = 1;

  /* Get and dispatch events */
  XtAppMainLoop(app_context);
  return 0;
}

/* Callbacks for buttons in yes/no question dialog
   w           widget id
   client_data data from application
   call_data   data from widget class */
void YesCB(Widget w, caddr_t client_data, caddr_t call_data)
{
  YNreply = 'y';
}
void NoCB(Widget w, caddr_t client_data, caddr_t call_data)
{
  YNreply = 'n';
}

char menuyn(char *message)
{
  /* subroutine to create yes/no question dialog box */
  static Widget YesNo;
  XmString q;
  /* int      l, i;
     char     *c; */

  /* initialise reply to null so that we can see when response given */
  YNreply = 0;
  if (!YesNo) {
    /* create dialog box */
    YesNo = XmCreateQuestionDialog(toplevel, "Yes/No", NULL, 0);
    XtSetArg(args[0], XmNokLabelString,
	     XmStringCreate("Yes", XmSTRING_DEFAULT_CHARSET));
    XtSetArg(args[1], XmNcancelLabelString,
	     XmStringCreate("No", XmSTRING_DEFAULT_CHARSET));
    XtSetArg(args[2], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetValues(YesNo, args, 3);
    XtSetSensitive(XmMessageBoxGetChild(YesNo, XmDIALOG_HELP_BUTTON), False);
    XtAddCallback(YesNo, XmNokCallback, (XtCallbackProc) YesCB, NULL);
    XtAddCallback(YesNo, XmNcancelCallback, (XtCallbackProc) NoCB, NULL);
  }
  q = XmStringCreate(message, XmSTRING_DEFAULT_CHARSET);
  /* l = XmStringLength(q);
     printf("l = %d\n", l);
     c = (char *) q;
     for (i=0; i<l; i++) {
     if (*c == '\n') {
     printf("i = %d\n", i);
     *c = '\0';
     }
     c++;
     } */
  XtSetArg(args[0], XmNmessageString, q);
  XtSetArg(args[1], XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON);
  XtSetValues(YesNo, args, 2);

  /* popup widget */
  XtManageChild(YesNo);

  /* handle events until response received */
  while (!YNreply) XtAppProcessEvent(app_context, XtIMAll);

  /* popdown widget */
  XtUnmanageChild(YesNo);
  XmStringFree(q);
  XtAppProcessEvent(app_context, XtIMAll);

  return YNreply;
}

void drmCB(Widget w, caddr_t client_data, caddr_t call_data)
{
  /* Callback for display region menu button press
     w           widget id
     client_data data from application
     call_data   data from widget class */
  MenuReply = client_data;
}

void create_drm_sel(int drb, char *drbname, char *ButtonText,
		    int top, int bottom, int left, int right)
{
/* subroutine to create buttons in display region menu box */

  XtSetArg(args[0], XmNlabelString,
	   XmStringCreate(ButtonText, XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[1], XmNtopAttachment, XmATTACH_POSITION);
  XtSetArg(args[2], XmNtopPosition, top);
  XtSetArg(args[3], XmNbottomAttachment, XmATTACH_POSITION);
  XtSetArg(args[4], XmNbottomPosition, bottom);
  XtSetArg(args[5], XmNleftAttachment, XmATTACH_POSITION);
  XtSetArg(args[6], XmNleftPosition, left);
  XtSetArg(args[7], XmNrightAttachment, XmATTACH_POSITION);
  XtSetArg(args[8], XmNrightPosition, right);
  drmButton[drb] = XmCreatePushButtonGadget(drm, drbname, args, 9);
  XtAddCallback(drmButton[drb], XmNactivateCallback,
		(XtCallbackProc) drmCB, ButtonText);
}

void DSRegion()
{
  /* subroutine to create display region menu box */

  /* initialise reply to null so that we can see when response given */
  MenuReply = NULL;
  if (!drm) {
    /* create dialog box */
    XtSetArg(args[0], XmNwidth, 170);
    XtSetArg(args[1], XmNheight, 180);
    XtSetArg(args[2], XmNfractionBase, 24);
    XtSetArg(args[3], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    drm = XmCreateFormDialog(toplevel, "Display_Region",
			     (ArgList) args, 4);

    /* Create buttons in display region menu */
    create_drm_sel( 0, "ds11", "1 1",  0, 24,  0,  4);
    create_drm_sel( 1, "ds22", "2 2",  0, 12,  4,  8);
    create_drm_sel( 2, "ds12", "1 2", 12, 24,  4,  8);
    create_drm_sel( 3, "ds33", "3 3",  0,  8,  8, 12);
    create_drm_sel( 4, "ds23", "2 3",  8, 16,  8, 12);
    create_drm_sel( 5, "ds13", "1 3", 16, 24,  8, 12);
    create_drm_sel( 6, "ds44", "4 4",  0,  6, 12, 16);
    create_drm_sel( 7, "ds34", "3 4",  6, 12, 12, 16);
    create_drm_sel( 8, "ds24", "2 4", 12, 18, 12, 16);
    create_drm_sel( 9, "ds14", "1 4", 18, 24, 12, 16);
    create_drm_sel(10, "ds66", "6 6",  0,  4, 16, 20);
    create_drm_sel(11, "ds56", "5 6",  4,  8, 16, 20);
    create_drm_sel(12, "ds46", "4 6",  8, 12, 16, 20);
    create_drm_sel(13, "ds36", "3 6", 12, 16, 16, 20);
    create_drm_sel(14, "ds26", "2 6", 16, 20, 16, 20);
    create_drm_sel(15, "ds16", "1 6", 20, 24, 16, 20);
    create_drm_sel(16, "ds88", "8 8",  0,  3, 20, 24);
    create_drm_sel(17, "ds78", "7 8",  3,  6, 20, 24);
    create_drm_sel(18, "ds68", "6 8",  6,  9, 20, 24);
    create_drm_sel(19, "ds58", "5 8",  9, 12, 20, 24);
    create_drm_sel(20, "ds48", "4 8", 12, 15, 20, 24);
    create_drm_sel(21, "ds38", "3 8", 15, 18, 20, 24);
    create_drm_sel(22, "ds28", "2 8", 18, 21, 20, 24);
    create_drm_sel(23, "ds18", "1 8", 21, 24, 20, 24);
  }

  /* popup widget */
  XtManageChildren(drmButton, 24);
  XtManageChild(drm);

  /* handle events until response received */
  while (!MenuReply) XtAppProcessEvent(app_context, XtIMAll);

  /* popdown widget */
  XtUnmanageChild(drm);
  XtAppProcessEvent(app_context, XtIMAll);
}

void fsOkCB(Widget w, caddr_t client_data,
	    XmFileSelectionBoxCallbackStruct *call_data)
{
  /* Callback for file selection dialog OK button
     w           widget id
     client_data data from application
     call_data   data from widget class */
  char *text;

  if (XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &text)) {
    strcpy(&gf3Cmnd[3],text);
    XtFree(text);
  }
  MenuReply = "1";
}

void fsCancelCB(Widget w, caddr_t client_data,
		XmFileSelectionBoxCallbackStruct *call_data)
{
  /* Callback for file selection dialog Cancel button
     w           widget id
     client_data data from application
     call_data   data from widget class */

  strcpy(gf3Cmnd,"   ");
  MenuReply = "1";
}

void fileSelect(int fileType)
{
  /* subroutine to create file selection dialog */
  static Widget fsDialog;
  static XmString dirMask[MAX_FILE], dirSpec[MAX_FILE];

  /* initialise reply to null so that we can see when response given */
  MenuReply = NULL;
  if (!fsDialog) {
    /* create dialog box */
    XtSetArg(args[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    fsDialog = XmCreateFileSelectionDialog(toplevel, "File_Selection",
					   (ArgList) args, 1);
    XtSetSensitive(XmFileSelectionBoxGetChild(fsDialog,
					      XmDIALOG_HELP_BUTTON), False);
    XtAddCallback(fsDialog, XmNokCallback, (XtCallbackProc) fsOkCB, NULL);
    XtAddCallback(fsDialog, XmNcancelCallback,
		  (XtCallbackProc) fsCancelCB, NULL);
    dirMask[0] = XmStringCreate("*.cmd", XmSTRING_DEFAULT_CHARSET);
    dirMask[1] = XmStringCreate("*.dmp", XmSTRING_DEFAULT_CHARSET);
    dirMask[2] = XmStringCreate("*.eff", XmSTRING_DEFAULT_CHARSET);
    dirMask[3] = XmStringCreate("*.spe", XmSTRING_DEFAULT_CHARSET);
    dirMask[4] = XmStringCreate("*.tab", XmSTRING_DEFAULT_CHARSET);
    dirMask[5] = XmStringCreate("*.win", XmSTRING_DEFAULT_CHARSET);
  }

  /* set up box to match filetype */
  XmFileSelectionDoSearch(fsDialog, dirMask[fileType]);
  if (dirSpec[fileType]) {
    XtSetArg(args[0], XmNdirSpec, dirSpec[fileType]);
    XtSetValues(fsDialog, args, 1);
  }

  /* popup widget */
  XtManageChild(fsDialog);

  /* handle events until response received */
  while (!MenuReply) XtAppProcessEvent(app_context, XtIMAll);

  /* save bits of box as defaults for next time with this filetype */
  XtSetArg(args[0], XmNdirMask, &dirMask[fileType]);
  XtSetArg(args[1], XmNdirSpec, &dirSpec[fileType]);
  XtGetValues(fsDialog, args, 2);

  /* popdown widget */
  XtUnmanageChild(fsDialog);
  XtAppProcessEvent(app_context, XtIMAll);
}

void ffCB(Widget w, caddr_t client_data,
	  XmToggleButtonCallbackStruct *call_data)
{
  /* Callback for the togglebuttons in the fix/free menu box
     w           widget id
     client_data data from application
     call_data   data from widget class */
  XmString UserCommand;
  int      i, j;
      
  /* if button set, want to fix parameter, else free it */
  if (call_data->set) strcpy(gf3Cmnd,"fx ");
  else                strcpy(gf3Cmnd,"fr ");
  strcat(gf3Cmnd, client_data);

  /* if we are to fix a parameter, first check that it is not already fixed */ 
  j = -1;
  if (call_data->set) {
    for (i = 0; i < gf3gd.npars; i++) {
      if ((w == ffButton[i+2])) j = i;
    }
  }
  /* if (!(call_data->set) || j < 0 || gf3gd.freepars[j]) { */
  if (j < 0 || gf3gd.freepars[j]) {
    /* execute command, and copy it to the command history */
    UserCommand = XmStringCreate(gf3Cmnd,XmSTRING_DEFAULT_CHARSET);
    XmListAddItem(CmndHistory, UserCommand, LAST);
    XmStringFree(UserCommand);
    XDefineCursor(XtDisplay(toplevel), XtWindow(ffMenu), watchCursor);
    xmgf3exec();
    XUndefineCursor(XtDisplay(toplevel), XtWindow(ffMenu));

    /* if we have just fixed R, then Beta may also have been fixed
       automatically. Check for this and set button if necessary */
    if (j == 3) {
      if (!gf3gd.freepars[4])
	XmToggleButtonGadgetSetState(ffButton[6], True, False);
    } 
    set_focus(XtWindow(ffMenu));
  } 
}

void ffDoneCB(Widget w, caddr_t client_data,
	      XmToggleButtonCallbackStruct *call_data)
{
  /* Callback for the Done button in the fix/free menu box
     w           widget id
     client_data data from application
     call_data   data from widget class */

  strcpy(gf3Cmnd,"   ");
  MenuReply = "1";
}

void create_ffm_sel(int ffb, char *ButtonText,
		    char *CBtext, int top, int left)
{
  /* subroutine to create togglebuttons in fix/free menu box */
  char ffbname[8];

  sprintf(ffbname, "ffb%d", ffb);
  XtSetArg(args[0], XmNlabelString,
	   XmStringCreate(ButtonText, XmSTRING_DEFAULT_CHARSET));
  XtSetArg(args[1], XmNtopAttachment, XmATTACH_POSITION);
  XtSetArg(args[2], XmNtopPosition, top);
  XtSetArg(args[3], XmNleftAttachment, XmATTACH_POSITION);
  XtSetArg(args[4], XmNleftPosition, left);
  ffButton[ffb] = XmCreateToggleButtonGadget(ffMenu, ffbname, args, 5);
  XtAddCallback(ffButton[ffb], XmNvalueChangedCallback,
		(XtCallbackProc) ffCB, CBtext);
}

void fixFreePars()
{
  /* subroutine to create fixed-pars menu box */
  int i;
  char bt[16], cbt[8];
  static char pklab[40] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  /* initialise reply to null so that we can see when response given */
  MenuReply = NULL;
  if (!ffMenu) {
    /* create dialog box */
    XtSetArg(args[0], XmNx, 30);
    XtSetArg(args[1], XmNy, 30);
    XtSetArg(args[2], XmNwidth, 480);
    XtSetArg(args[3], XmNheight, 480);
    XtSetArg(args[4], XmNfractionBase, 42);
    XtSetArg(args[5], XmNrubberPositioning, True);
    XtSetArg(args[6], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    ffMenu = XmCreateFormDialog(toplevel, "Fix/Free", (ArgList) args, 7);

    /* Create buttons in fixed-pars menu */
    create_ffm_sel( 0, "rel. widths", "rw",   0,  0);
    create_ffm_sel( 1, "rel. pos.",   "rp",   0, 22);
    create_ffm_sel( 2, "A",           "A",    2,  0);
    create_ffm_sel( 3, "B",           "B",    2,  6);
    create_ffm_sel( 4, "C",           "C",    2, 13);
    create_ffm_sel( 5, "R",           "R",    2, 22);
    create_ffm_sel( 6, "Beta",        "Beta", 2, 28);
    create_ffm_sel( 7, "Step",        "Step", 2, 35);

    for (i=0; i<35; i+=2) {
      sprintf(bt, "Pos %c", pklab[i]);
      sprintf(cbt, "P%c", pklab[i]);
      create_ffm_sel(3*i+ 8, bt, cbt, i+4,  0);
      sprintf(bt, "Width %c", pklab[i]);
      sprintf(cbt, "W%c", pklab[i]);
      create_ffm_sel(3*i+ 9, bt, cbt, i+4,  6);
      sprintf(bt, "Height %c", pklab[i]);
      sprintf(cbt, "H%c", pklab[i]);
      create_ffm_sel(3*i+10, bt, cbt, i+4, 13);
      if (i==34) break;
      sprintf(bt, "Pos %c", pklab[i+1]);
      sprintf(cbt, "P%c", pklab[i+1]);
      create_ffm_sel(3*i+11, bt, cbt, i+4, 22);
      sprintf(bt, "Width %c", pklab[i+1]);
      sprintf(cbt, "W%c", pklab[i+1]);
      create_ffm_sel(3*i+12, bt, cbt, i+4, 28);
      sprintf(bt, "Height %c", pklab[i+1]);
      sprintf(cbt, "H%c", pklab[i+1]);
      create_ffm_sel(3*i+13, bt, cbt, i+4, 35);
    }

    XtSetArg(args[0], XmNlabelString,
	     XmStringCreate("-- Done --", XmSTRING_DEFAULT_CHARSET));
    XtSetArg(args[1], XmNtopAttachment, XmATTACH_POSITION);
    XtSetArg(args[2], XmNtopPosition, 40);
    XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[5], XmNrightAttachment, XmATTACH_FORM);
    ffButton[113] = XmCreatePushButtonGadget(ffMenu, "ffb113", args, 6);
    XtAddCallback(ffButton[113], XmNactivateCallback,
		  (XtCallbackProc) ffDoneCB, NULL);
  }

  /* set up box to match number of pars and fixed pars*/
  XtSetSensitive(ffButton[0], True);
  if (!gf3gd.irelw)
    XmToggleButtonGadgetSetState(ffButton[0],True, False);
  else
    XmToggleButtonGadgetSetState(ffButton[0],False, False);

  XtSetSensitive(ffButton[1], True);
  if (!gf3gd.irelpos)
    XmToggleButtonGadgetSetState(ffButton[1],True, False);
  else
    XmToggleButtonGadgetSetState(ffButton[1],False, False);

  for (i = 0; i < gf3gd.npars; i++) {
    XtSetSensitive(ffButton[i+2], True);
    if (!gf3gd.freepars[i])
      XmToggleButtonGadgetSetState(ffButton[i+2],True, False);
    else
      XmToggleButtonGadgetSetState(ffButton[i+2],False, False);
  } 

  for (i = gf3gd.npars+2; i < 113; i++) {
    XtSetSensitive(ffButton[i], False);
    XmToggleButtonGadgetSetState(ffButton[i], False, False);
  }

  /* popup widget */
  XtManageChildren(ffButton, 114);
  XtManageChild(ffMenu);

  /* handle events until response received */
  while (!MenuReply) XtAppProcessEvent(app_context, XtIMAll);

  /* popdown widget */
  XtUnmanageChild(ffMenu);
  XtAppProcessEvent(app_context, XtIMAll);
  XFlush(XtDisplay(toplevel));
  set_focus(XtWindow(form));
}
