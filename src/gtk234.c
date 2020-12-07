#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <X11/Xlib.h>
#include <math.h>
#include <ctype.h>
#include <signal.h>

#include "util.h"
#include "mingtk.h"
#ifdef GTK2D
#include "escl8r.h"
#else
#include "levit8r.h"
#endif


/*************************************************
 *    C source code for gtkesc, gtklev and gtk4dg.
 *    Compiled with commands like:
 *	$cc -c -o gtkescc.o -DGTK2D gtk234.c
 *	$cc -c -o gtklevc.o -DGTK3D gtk234.c
 *	$cc -c -o gtk4dgc.o -DGTK4D gtk234.c
 *    to generate the different object codes for different programs.
 *  Author  D.C. Radford   Oct 2003
 **************************************************/

/* external variables defined in miniga.c */
extern Display     *disp_id;
extern Screen      *screen_id;
extern GdkWindow   *win_id;
extern GdkGC       *xmgc, *gc_id, *gc_comp_id;
extern GdkColormap *colormap;
extern GdkFont     *font_id;
extern GdkPoint    points[512];
extern GdkColor    color_id[20];
extern int         nstored;
extern int         win_width, win_height;
extern int         hcopy_use_preset_values, hcopy_use_color, hcopy_print_now;
extern char        hcopy_filename[80], hcopy_print_cmd[80];

extern int     glsundo_ready;

/* function defined in gtkgls_utils.c */
extern void move_win_to(GtkWidget *w, int x, int y);
extern void set_title(GtkWidget *w, char *text);
/* from gtkminig2 */
extern int select_gw(int);

GtkTextBuffer *output_text_buffer;
GtkWidget     *output_text_view, *text_scrolled;
GtkWidget     *toplevel, *menubar, *drawing_area_gls, *drawing_area_sp;
GtkWidget     *toolbar, *pos_text, *label, *gls_scrolled, *sp_scrolled, *command_entry;
GtkWidget     *undo_button, *redo_button, *save_button, *zin_button, *zout_button;
GtkWidget     *undo_menu_item, *redo_menu_item, *save_menu_item, *zout_menu_item;
GtkWidget     *backg_button, *fwdg_button;
GtkWidget     *backg_menu_item, *fwdg_menu_item;
GtkTooltips   *tooltips;
GdkPixmap     *pixmap;
/* winpix = gls pixmap for zoom < 8; otherwise winpix = gls window */
GdkWindow     *window, *winpix;
Cursor    watchCursor;
int       ww = 600, hh = 500, width = 600, height = 500;
int       zoom = 1, scr_width, scr_height;
int       igw_id, crosshairs = 0, chx = -1;
float     xin, yin, xmg[4];
char      cin[40];
int       win_origin_x, win_origin_y, win_w, win_h, focus = 0;

static struct {
  GtkWidget *w, *entry[20], *label[13];
  char      x0[20], y0[20], nx[20], ny[20], my[20], ng[20], sigma[20];
  int       loch, hich, ngd, locnt, nchs, ncnts, iyaxis, lox, numx;
  int       auto_x, auto_y, single_gate, disp_calc, disp_diff, pkfind;
  int       done, active;
} sp_disp_dlg; /* spectrum display setup dialog */

static struct {
  GtkWidget *w, *entry[5], *label[3];
  char      filnam[80], printcmd[80];
  int       done, use_color, print_now;
} sp_ps_dlg; /*  spectrum postscript hardcopy dialog */

static struct {
  GtkWidget *w, *entry[8], *label[7];
  char      filnam[80];
  int       write[5], expand;
  int       done;
} write_sp_dlg; /* write-spectrum dialog */

static int get_sp_disp_details(void);
static int sp_hcopy(void);
static int write_sp_2(void);

#define FONTNAME "-*-HELVETICA-MEDIUM-R-*-*-12-*-*-*-*-*"

typedef struct {
  char  *label;
  char  *action;
  int	 item_type;
} MenuFactory_dcr;

/* item_type: 0 = menu item, 1 = new menu,       2 = separator,
              3 = submenu,  -3 = end of submenu, 4 = last new menu (right justified) */
static MenuFactory_dcr mf_items[] = {
  { "File",                                        NULL, 1 },
  { "Save Level Scheme",                           "sl", 0 },
  { "Save Level Scheme as...",                     "wg", 0 },
  { "Make Postscript Hardcopy of Level Scheme...", "hc", 0 },
  { "", NULL, 2 },
  { "Open New Level Scheme File...",               "rg", 0 },
  { "Add a Second .gls Level Scheme File...",      "ad", 0 },
  { "", NULL, 2 },
  { "Test Gamma-ray Energy and Intensity Sums",    "ts", 0 },
  { "Undo Level Scheme Edit",                      "ue", 0 },
  { "Redo Level Scheme Edit",                      "re", 0 },
  { "", NULL, 2 },
  { "Make Postscript Hardcopy of Spectra...",     "shc", 0 },
  { "Write Gate as gf3 Spectrum File...",          "ws", 0 },
  { "Execute Command File...",                     "cf", 0 },
  { "", NULL, 2 },
  { "Exit Program",                                "st", 0 },
  { "Display",                                     NULL, 1 },
  { "Zoom In",                                     "zi", 0 },
  { "Zoom Out",                                    "zo", 0 },
  { "", NULL, 2 },
  { "Redraw Level Scheme",                         "rd", 0 },
  { "Redraw Entire Level Scheme",                 "rd1", 0 },
  { "", NULL, 2 },
  { "Expand Level Scheme or Spectra",              "ex", 0 },
  { "Change Figure Parameters...",                 "fp", 0 },
  { "Open or Close Gap in Scheme",                 "oc", 0 },
  { "Bands",                                       NULL, 1 },
  { "Add E2 Gammas to Top of Band",                "ab", 0 },
  { "Add M1 Gammas to Top of Band",               "ab1", 0 },
  { "Delete Bands",                                "db", 0 },
  { "Edit Bands...",                               "eb", 0 },
  { "Move Bands",                                  "mb", 0 },
  { "Move Multiple Bands",                         "mm", 0 },
  { "Move Spin Labels in Bands",                   "lb", 0 },
  { "Move Gammas inside Bands",                    "gb", 0 },
  { "Gammas",                                      NULL, 1 },
  { "Add Gammas",                                  "ag", 0 },
  { "Delete Gammas",                               "dg", 0 },
  { "Edit Gammas...",                              "eg", 0 },
  { "Move Gammas",                                 "mg", 0 },
  { "Move Labels of Gammas",                       "lg", 0 },
  { "Reorder Gammas in and out of Levels",         "sg", 0 },
  { "Toggle Tentative-Normal Gammas",              "tg", 0 },
  { "Examine Gates with Cursor",                   "xg", 0 },
  { "Levels",                                      NULL, 1 },
  { "Add Levels",                                  "al", 0 },
  { "Delete Levels",                               "dl", 0 },
  { "Edit Levels...",                              "el", 0 },
  { "Move Levels Between Bands",                   "ml", 0 },
  { "Move Spin Labels of Levels",                  "ll", 0 },
  { "Move Energy Labels of Levels",                "le", 0 },
  { "Toggle Tentative-Thick-Normal Levels",        "tl", 0 },
  { "Change Widths of Levels",                     NULL, 3 },
  { "Move left edge",                             "lwl", 0 },
  { "Move right edge",                            "lwr", 0 },
  { "", NULL, -3 },
  { "Move Connected Levels Up or Down in Energy",  "ud", 0 },
  { "Text",                                        NULL, 1 },
  { "Add Text Labels",                             "at", 0 },
  { "Delete Text Labels",                          "dt", 0 },
  { "Edit Text Labels...",                         "et", 0 },
  { "Move Text Labels",                            "mt", 0 },
  { "", NULL, 2 },
  { "Change Level Spin Labels",                    NULL, 3 },
  { "spin label =  Jpi",                          "cl0", 0 },
  { "spin label = (J)pi",                         "cl1", 0 },
  { "spin label = J(pi)",                         "cl2", 0 },
  { "spin label = (Jpi)",                         "cl3", 0 },
  { "spin label =   J",                           "cl4", 0 },
  { "spin label =  (J)",                          "cl5", 0 },
  { "no spin label",                              "cl6", 0 },
  { "toggle through spin labels",                 "cl7", 0 },
  { "change size of spin labels",                 "cl8", 0 },
  { "Change Level Energy Labels",                  NULL, 3 },
  { "no energy label",                            "ce0", 0 },
  { "energy label =  123",                        "ce1", 0 },
  { "energy label = (123)",                       "ce2", 0 },
  { "energy label =  123.4",                      "ce3", 0 },
  { "energy label = (123.4)",                     "ce4", 0 },
  { "toggle through energy labels",               "ce5", 0 },
  { "change size of energy labels",               "ce6", 0 },
  { "Change Gamma Labels",                         NULL, 3 },
  { "gamma label =  123",                         "cg0", 0 },
  { "gamma label = (123)",                        "cg1", 0 },
  { "gamma label =  123.4",                       "cg3", 0 },
  { "gamma label = (123.4)",                      "cg4", 0 },
  { "no gamma label",                             "cg2", 0 },
  { "toggle through gamma labels",                "cg5", 0 },
  { "change size of gamma labels",                "cg6", 0 },
  { "", NULL, -3 },
  { "Fit",                                         NULL, 1 },
  { "Fit Intensities of Gammas",                   "fi", 0 },
  { "Fit Energies of Gammas",                      "fe", 0 },
  { "Fit Both Intensities & Energies of Gammas",   "fb", 0 },
  { "Fit Energies of Levels",                      "fl", 0 },
  { "Fit Peak Width Parameters",                  "fwr", 0 },
  { "Spectrum",                                    NULL, 1 },
  { "Take Full Projection",                        "pr", 0 },
  { "Examine Gates with Cursor",                   "xg", 0 },
  { "", NULL, 2 },
  { "Set up Spectrum Display...",                  "sd", 0 },
  { "Redraw Spectra",                              "ds", 0 },
  { "Redraw Full Spectra",                        "ds1", 0 },
  { "Expand Level Scheme or Spectra",              "ex", 0 },
  { "", NULL, 2 },
  { "Make Postscript Hardcopy of Spectra...",     "shc", 0 },
  { "Write Gate as gf3 Spectrum File...",          "ws", 0 },
  { "Save Gate to .gat File",                      "ss", 0 },
  { "Read Gate from .gat File",                    "rs", 0 },
  { "", NULL, 2 },
#ifdef GTK2D
  { "Undo Changes to escl8r Data",                 "uc", 0 },
  { "Redo Changes to escl8r Data",                 "rc", 0 },
#endif
#ifdef GTK3D
  { "Undo Changes to levit8r Data",                "uc", 0 },
  { "Redo Changes to levit8r Data",                "rc", 0 },
#endif
#ifdef GTK4D
  { "Undo Changes to 4dg8r Data",                  "uc", 0 },
  { "Redo Changes to 4dg8r Data",                  "rc", 0 },
#endif
  { "Go Back to Previous Gate",                    "bg", 0 },
  { "Go Forward to Next Gate",                     "fg", 0 },
  { "GateLists",                                   NULL, 1 },
  { "Create or Edit Lists of Gates",               "l ", 0 },
  { "Sum List of Gates",                           "gl", 0 },
  { "Examine List of Gates",                       "xl", 0 },
  { "Read List File",                              "rl", 0 },
  { "Write List File",                             "wl", 0 },
#ifdef GTK3D
  { "3D",                                          NULL, 1 },
  { "Examine Triples with Cursor",                 "xt", 0 },
  { "Type Triples Gates",                          "t ", 0 },
  { "Fit Intensities of Gammas in Cube",          "fti", 0 },
  { "Fit Energies of Gammas in Cube",             "fte", 0 },
  { "Fit Both Ints and Energies in Cube",         "ftb", 0 },
  { "Search for (SD) Bands in 3D",                 "sb", 0 },
#endif
#ifdef GTK4D
  { "3D&4D",                                       NULL, 1 },
  { "Examine Triples with Cursor",                 "xt", 0 },
  { "Type Triples Gates",                          "t ", 0 },
  { "Fit Intensities of Gammas in Cube",          "fti", 0 },
  { "Fit Energies of Gammas in Cube",             "fte", 0 },
  { "Fit Both Ints and Energies in Cube",         "ftb", 0 },
  { "Search for (SD) Bands in 3D",                 "sb", 0 },
  { "Examine Quadruples with Cursor",              "xq", 0 },
  { "Type Quadruples Gates",                       "q ", 0 },
  { "Search for (SD) Bands in 4D",                 "sq", 0 },
#endif
  { "Info",                                        NULL, 1 },
  { "Set Gate Limits with Cursor",                 "g ", 0 },
  { "Sum Counts with Cursor",                      "su", 0 },
  { "Check for Energy Sums with Cursor",           "es", 0 },
  { "Test Gamma-ray Energy and Intensity Sums",    "ts", 0 },
  { "Help",                                        NULL, 4 },
  { "on Commands",                                 "he", 0 }
};

static char *commandList[] = {
    "Make Postscript Hardcopy of Level Scheme", "hc",
    "Save Level Scheme",                        "sl",
    "Save Level Scheme as...",                  "wg",
    "Open New Level Scheme File",               "rg",
    "Add a Second .gls Level Scheme File",      "ad",
    "Test Gamma-ray Energy and Intensity Sums", "ts",
    "Undo Level Scheme Edit",                   "ue",
    "Redo Level Scheme Edit",                   "re",
    "Exit Program",                             "st",
    "Zoom In",                                  "zi",
    "Zoom Out",                                 "zo",
    "Redraw Entire Level Scheme",              "rd1",
    "Redraw Level Scheme",                      "rd",
    "Expand Level Scheme or Spectra",           "ex",
    "Change Figure Parameters",                 "fp",
    "Open/Close Gap in Scheme",                 "oc",
    "Add M1 Gammas to Top of Band",            "ab1",
    "Add E2 Gammas to Top of Band",             "ab",
    "Delete Bands",                             "db",
    "Edit Bands",                               "eb",
    "Move Bands",                               "mb",
    "Move Multiple Bands",                      "mm",
    "Move Spin Labels in Bands",                "lb",
    "Move Gammas inside Bands",                 "gb",
    "Add Gammas",                               "ag",
    "Delete Gammas",                            "dg",
    "Edit Gammas",                              "eg",
    "Move Gammas",                              "mg",
    "Move Labels of Gammas",                    "lg",
    "Reorder Gammas in/out of Levels",          "sg",
    "Toggle Tentative/Normal Gammas",           "tg",
    "Examine Gates with Cursor",                "xg",
    "Add Levels",                               "al",
    "Delete Levels",                            "dl",
    "Edit Levels",                              "el",
    "Move Levels Between Bands",                "ml",
    "Move Spin Labels of Levels",               "ll",
    "Move Energy Labels of Levels",             "le",
    "Toggle Tentative/Thick/Normal Levels",     "tl",
    "Change Widths of Levels (Left Edge)",     "lwl",
    "Change Widths of Levels (Right Edge)",    "lwr",
    "Change Widths of Levels",                  "lw",
    "Fit Energies of Levels",                   "fl",
    "Move Connected Levels Up/Down in Energy",  "ud",
    "Add Text Labels",                          "at",
    "Delete Text Labels",                       "dt",
    "Edit Text Labels",                         "et",
    "Move Text Labels",                         "mt",
    "Change Level Spin Labels",                 "cl",
    "Change Level Energy Labels",               "ce",
    "Change Gamma Labels",                      "cg",
    "Help on commands",                         "he",

    "Write Gate as gf3 Spectrum File",          "ws",
    "Save Gate to .gat File",                   "ss",
    "Read Gate from .gat File",                 "rs",
    "Execute Command File",                     "cf",
    "Change Energy, Efficiency Calibs",         "ca",
    "Change 2D Background",                     "cb",
    "Change Peak Shape and Width",              "ps",
    "Renormalise Gates:Scheme",                 "rn",
    "Fit Intensities of Gammas",                "fi",
    "Fit Energies of Gammas",                   "fe",
    "Fit Both Intensities and Energies",        "fb",
    "Fit Level Energies",                       "fl",
    "Fit Peak Width Parameters",                "fw",
    "Take Full Projection",                     "pr",
    "Examine Gates with Cursor",                "xg",
    "Show Only one Gate",                      "ng1",
    "Show Only one Gate",                     "ng 1",
    "Show More than one Gate (Stacked)",        "ng",
    "ReDraw Full Spectra",                     "ds1",
    "ReDraw Spectra",                           "ds",
    "Set up Spectrum Display",                  "sd",
    "Change X-Axis X0, NX",                     "xa",
    "Change Y-Axis Y0, NY",                     "ya",
    "Move X-Axis Up",                           "mu",
    "Move X-Axis Down",                         "md",
    "Toggle Display of Calculated Sp.",         "dc",
    "Make Postscript Hardcopy of Spectra",     "shc",
    "Undo Changes to esc/lev/4dg Data",         "uc",
    "Redo Changes to esc/lev/4dg Data",         "rc",
    "Go Back to Previous Gate",                 "bg",
    "Go Forward to Next Gate",                  "fg",
    "Create/Edit Lists of Gates",               "l ",
    "Sum List of Gates",                        "gl",
    "Examine List of Gates",                    "xl",
    "Read List File",                           "rl",
    "Write List File",                          "wl",
    "Setup PeakFind on Sp. Display",            "pf",
    "Call Cursor on Sp. Display",               "cr",
    "Set Gate Limits with Cursor",              "g ",
    "Sum Counts with Cursor",                   "su",
    "Check for Energy Sums with Cursor",        "es",
    "Test Gamma-ray Energy and Intensity Sums", "ts",
#ifndef GTK2D
    "Examine Triples with Cursor",              "xt",
    "Type Triples Gates",                       "t ",
    "Fit Intensities of Gammas in Cube",       "fti",
    "Fit Energies of Gammas in Cube",          "fte",
    "Fit Both Intensities & Energies in Cube", "ftb",
    "Search for (SD) Bands in 3D",              "sb",
#endif
#ifdef GTK4D
    "Examine Quadruples with Cursor",           "xq",
    "Type Quadruples Gates",                    "q ",
    "Search for (SD) Bands in 4D",              "sq",
#endif
    "", ""
};
           
/*---------------------------------------*/
void set_gate_button_sens(int sens)
{
  /* this function is called from esc_undo.c or lev4d_undo.c */
  if (sens == -1) gtk_widget_set_sensitive(backg_button, FALSE);
  if (sens == -2) gtk_widget_set_sensitive(fwdg_button, FALSE);
  if (sens == 1)  gtk_widget_set_sensitive(backg_button, TRUE);
  if (sens == 2)  gtk_widget_set_sensitive(fwdg_button, TRUE);

  if (sens == -1) gtk_widget_set_sensitive(backg_menu_item, FALSE);
  if (sens == -2) gtk_widget_set_sensitive(fwdg_menu_item, FALSE);
  if (sens == 1)  gtk_widget_set_sensitive(backg_menu_item, TRUE);
  if (sens == 2)  gtk_widget_set_sensitive(fwdg_menu_item, TRUE);
}

/*---------------------------------------*/
void removecrosshairs(void)
{
  if (crosshairs) {
    gdk_draw_line(drawing_area_sp->window, gc_comp_id, chx, 0, chx, win_height);
    XFlush(disp_id);
    crosshairs = 0;
  }
} /* removecrosshairs */

void drawcrosshairs(void)
{
  if (chx>=0) {
    gdk_draw_line(drawing_area_sp->window, gc_comp_id, chx, 0, chx, win_height);
    XFlush(disp_id);
    crosshairs = 1;
  }
} /* drawcrosshairs */

/* ======================================================================= */
static int exit_gtkgls(void)
{
  if (glsundo_ready &&
      glsundo.mark != glsundo.next_pos) read_write_gls("WRITE");
  save_xwg("gtk_234_");
  gtk_exit(0);
  return FALSE;
} /* exit_gtkgls */

static int configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
  gdk_window_get_root_origin(toplevel->window, &win_origin_x, &win_origin_y);
  win_w = toplevel->allocation.width;
  win_h = toplevel->allocation.height;
  gtk_window_set_default_size(GTK_WINDOW(toplevel), win_w, win_h);
  return FALSE;
} /* configure_event */

/* ======================================================================= */
int set_size(void)
{
  select_gw(2);
  win_id = winpix;
  win_width = (int) width;
  win_height = (int) height;
  select_gw(1);
  display_gls(-1);
  return 0;
} /* set_size */

/* Subroutine to copy pixmap to drawing area */
void copy_area(void)
{
  if (zoom < 8)
    gdk_draw_pixmap(drawing_area_gls->window, xmgc, pixmap,
		    0, 0, 0, 0, width, height);
} /* copy_area */

/* ======================================================================= */
/* Callback for the drawing area expose event */
static int DAexposeCB(GtkWidget *widget, GdkEventExpose *event, gpointer cb_data)
{
  GtkAdjustment *hadj;
  static int old_offset = 0, new_offset;

  if (widget == drawing_area_gls) {
    if (zoom < 8) {
      copy_area();
    } else {
      display_gls(-1);
    }
  } else {
    /* The position of the gate label needs to be moved so that it's always visible */
    hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(sp_scrolled));
    new_offset = 27 + (int) hadj->upper - (int) hadj->value - sp_scrolled->allocation.width;
    if (new_offset != old_offset) {
      /* if scroll-bar has been moved, then we need to redraw the whole window! */
      gtk_widget_queue_draw_area(drawing_area_sp, 0, 0,
				 drawing_area_sp->allocation.width,
				 drawing_area_sp->allocation.height);
      old_offset = new_offset;
      return FALSE;
    }
    set_gate_label_h_offset(new_offset);
    disp_dat(3);
    if (crosshairs) drawcrosshairs();
  }
  return FALSE;
} /* DAexposeCB */

/* ======================================================================= */
/* Callback for the drawing area key event */
static int DAkeyCB(GtkWidget *widget, GdkEventKey *event, gpointer cb_data)
{
  int          ixi, iyi;
  unsigned int state;
  char         response[40];

  if (!focus ||
      (event->state & GDK_CONTROL_MASK) ||  // control key down
      (event->state & GDK_MOD1_MASK) ||     // alt key down
      // (event->state & GDK_MOD2_MASK) ||  // numlock on; ignore this one
      (event->state & GDK_MOD3_MASK) ||     // ?? don't know meaning of mod3 or mod5
      (event->state & GDK_MOD4_MASK) ||     // windows key down
      (event->state & GDK_MOD5_MASK)) return FALSE;

  strcpy(response, event->string);
  // printf("string: >%s< %d\n", response, (int) response[0]);
  if ((int) response[0] > 26 && (int) response[0] < 127) {
    /* response is something that retic routine would not ignore */
    gdk_window_get_pointer(sp_scrolled->window, &ixi, &iyi, &state);
    if (ixi < 0 || iyi < 0 ||
	ixi > sp_scrolled->allocation.width ||
	iyi > sp_scrolled->allocation.height + gls_scrolled->allocation.height) return FALSE;
    if (iyi > gls_scrolled->allocation.height) {
      gdk_window_get_pointer(drawing_area_sp->window, &ixi, &iyi, &state);
      igw_id = 1;
    } else if (ixi < gls_scrolled->allocation.width) {
      gdk_window_get_pointer(drawing_area_gls->window, &ixi, &iyi, &state);
      igw_id = 2;
    } else {
      return FALSE;
    }

//     tell("igw_id: %d\n", igw_id);
    strcpy(cin, response);
    select_gw(igw_id);
    iyi = win_height - iyi;
    cvxy(&xin, &yin, &ixi, &iyi, 2);
  }
  return FALSE;
}
/* DAkeyCB */

/* Callback for the drawing area mouse button event */
static int DAbuttonCB(GtkWidget *widget, GdkEventButton *event, gpointer cb_data)
{
  int          ixi, iyi;
  unsigned int iflag;


  ixi = event->x;
  iyi = event->y;
  if ((event->state & ShiftMask) != 0 && widget == drawing_area_gls) {
    /* shift-mousebutton -> pan/zoom gls display */
    iflag = 1;
    if (event->button == Button2) iflag = 2;
    if (event->button == Button3) iflag = 3;
    select_gw(2);
    iyi = height - iyi;
    cvxy(&xin, &yin, &ixi, &iyi, 2);
    pan_gls(xin, yin, iflag);
  } else {
    if (widget == drawing_area_gls) {
      igw_id = focus = 2;
      gtk_widget_grab_focus(GTK_WIDGET(text_scrolled));  // remove focus from command entry widget
//       tell("focus=2\n");
    } else if (widget == drawing_area_sp) {
      igw_id = focus = 1;
      gtk_widget_grab_focus(GTK_WIDGET(text_scrolled));  // remove focus from command entry widget
//       tell("focus=1\n");
    } else {
      focus = 0;
//       tell("focus=0\n");
      return FALSE;
    }

    strcpy(cin, "G1");
    if (event->button == Button2) strcpy(cin, "X2");
    if (event->button == Button3) strcpy(cin, "X3");
    select_gw(igw_id);
    iyi = win_height - iyi;
    cvxy(&xin, &yin, &ixi, &iyi, 2);
  }
  return FALSE;
} /* DAbuttonCB */

/* ======================================================================= */
/* subroutine to execute commands */
void execute(char *command)
{
  GtkAdjustment *hadj, *vadj;
  static char title[40][80];
  static int  titleNum = 0;
  float       h1, h2, h3, v1, v2, v3;
  int         nc, i;
  char        cmd[80];
  void        removecrosshairs();

  /* set window title to command */
  if (titleNum == 0) {
#ifdef GTK2D
    strcpy(title[titleNum], "GtkEscl8r");
#endif
#ifdef GTK3D
    strcpy(title[titleNum], "GtkLevit8r");
#endif
#ifdef GTK4D
    strcpy(title[titleNum], "Gtk4dg8r");
#endif
  }

  if (titleNum == 39) {
    titleNum--;
    for (i=0; i<titleNum; i++) strcpy(title[i], title[i+1]);
  }
  if (titleNum > 9) {
    tell(" *** Warning - you should not nest so many commands.\n"
	 " *** Consider exiting from more previous modes\n"
	 " ***     before entering new ones.\n");
  }
  titleNum++;
#ifdef GTK2D
  strcpy(title[titleNum], "GtkEscl8r -- ");
#endif
#ifdef GTK3D
  strcpy(title[titleNum], "GtkLevit8r -- ");
#endif
#ifdef GTK4D
  strcpy(title[titleNum], "Gtk4dg8r -- ");
#endif
  strcat(title[titleNum], command);
  for (i = 0; (*commandList[i]); i += 2) {
    if (!strncmp(command, commandList[i+1], strlen(commandList[i+1]))) {
      strcat(title[titleNum], "  -- ");
      strcat(title[titleNum], commandList[i]);
      break;
    }
  }
  gtk_window_set_title(GTK_WINDOW(toplevel), title[titleNum]);

  if (strncmp(command, "zi", 2) == 0) {
  /* Zoom in */
    hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gls_scrolled));
    vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(gls_scrolled));
    h1 = hadj->value;
    h2 = hadj->upper;
    h3 = hadj->page_size;
    v1 = vadj->value;
    v2 = vadj->upper;
    v3 = vadj->page_size;

    gtk_widget_set_sensitive(zout_button, TRUE);
    gtk_widget_set_sensitive(zout_menu_item, TRUE);
    if (zoom < 16) {
      zoom = zoom*2;
      width = width*2;
      height = height*2;
    }
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area_gls), width, height);
    if (zoom < 8) {
      winpix = pixmap;
    } else {
      winpix = window;
    }
    set_size();

    /* adjust position of scrollbars to center zoomed region */
    while (gtk_events_pending()) gtk_main_iteration();
    hadj->value = (h1 + h3/4.0f) * hadj->upper / h2;
    vadj->value = (v1 + v3/4.0f) * vadj->upper / v2;
    if (hadj->value > hadj->upper - hadj->page_size)
      hadj->value = hadj->upper - hadj->page_size;
    if (vadj->value > vadj->upper - vadj->page_size)
      vadj->value = vadj->upper - vadj->page_size;
    if (hadj->value < 0.0) hadj->value = 0.0;
    if (vadj->value < 0.0) vadj->value = 0.0;
    gtk_adjustment_value_changed(hadj);
    gtk_adjustment_value_changed(vadj);

  } else if (strncmp(command, "zo", 2) == 0) {
    /* Zoom out */

    hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gls_scrolled));
    vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(gls_scrolled));
    h1 = hadj->value;
    h2 = hadj->upper;
    h3 = hadj->page_size;
    v1 = vadj->value;
    v2 = vadj->upper;
    v3 = vadj->page_size;

    if (zoom > 1) {
      zoom = zoom/2;
      width = width/2;
      height = height/2;
    }
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area_gls), width, height);
    if (zoom < 8) {
      winpix = pixmap;
    } else {
      winpix = window;
    }
    set_size();
    if (zoom < 2) {
      gtk_widget_set_sensitive(zout_button, FALSE);
      gtk_widget_set_sensitive(zout_menu_item, FALSE);
    }

    /* adjust position of scrollbars to center unzoomed region */
    while (gtk_events_pending()) gtk_main_iteration();
    if ((hadj->value = (h1 - h3/2.0f) * hadj->upper / h2) < 0.0f) hadj->value = 0.0f;
    if ((vadj->value = (v1 - v3/2.0f) * vadj->upper / v2) < 0.0f) vadj->value = 0.0f;
    gtk_adjustment_value_changed(hadj);
    gtk_adjustment_value_changed(vadj);

  } else if (strncmp(command, "shc", 3) == 0) {
    sp_hcopy();

  } else if (strncmp(command, "ws", 2) == 0) {
    write_sp_2();

  } else if (strncmp(command, "st", 2) == 0) {
    if (askyn("Are you sure you want to exit? (Y/N)")) exit_gtkgls();

  } else if (strncmp(command, "sd", 2) == 0 ) {
    get_sp_disp_details();

  } else {

    removecrosshairs();
    strcpy(cmd, command);
    nc = strlen(cmd);
    /* if log command file open, copy response */
    log_to_cmd_file(cmd);
 
    /* execute command */

#ifdef GTK2D
    e_gls_exec(cmd, nc);
    /* get commands from command file, if it is open */
    while ((nc = read_cmd_file(cmd, 80)) >= 0) e_gls_exec(cmd, nc);
#endif
#ifdef GTK3D
    l_gls_exec(cmd, nc);
    /* get commands from command file, if it is open */
    while ((nc = read_cmd_file(cmd, 80)) >= 0) l_gls_exec(cmd, nc);
#endif
#ifdef GTK4D
    fdg8r_exec(cmd, nc);
    /* get commands from command file, if it is open */
    while ((nc = read_cmd_file(cmd, 80)) >= 0) fdg8r_exec(cmd, nc);
#endif
  }

  /* restore last window title */
  titleNum--;
  gtk_window_set_title(GTK_WINDOW(toplevel), title[titleNum]);

} /* execute */

/* ======================================================================= */
/* Callback for the pushbuttons in the toolbar,
   and menu items in the menubar */
static int ButtonCB (GtkWidget *widget, gpointer call_data)
{
  GtkWidget   *make_cmd_button(const char *lbl, const char *cmd, const char *tip);
  int         i, nc;
  char        command[80], *ans;

  strcpy (command, call_data);

  if (strncmp(command, "aa", 2) == 0) {
    /* add another command widget/button */
    ans = malloc(40);
    if (!(nc = ask(ans, 39, "Command for new button = ?"))) return FALSE;
    ans[39] = 0;
    /* make sure command is in lower case */
    for (i = 0; i < 2; i++) {
      ans[i] = tolower(ans[i]);
    }
 
    for (i = 0; (*commandList[i]); i += 2) {
     /* if command is known, set tooltip */
     if (!strncmp(ans, commandList[i+1], strlen(commandList[i+1]))) break;
    }
    gtk_widget_show(make_cmd_button(ans, ans, commandList[i]));

  } else if (strlen(command)) {
    /* Execute command */
    /* printf ("command: %d %s\n", strlen(command), command); */
    execute(command);
  }
  return FALSE;
} /* ButtonCB */

/* ======================================================================= */
/* Callback for the command line widget */
static int CommandCB (GtkWidget *widget, gpointer call_data)
{
  char command[80];
  int  i;

  /* get command from text entry widget */
  strncpy(command, gtk_entry_get_text(GTK_ENTRY(widget)), 79);
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  /* make sure command is in lower case */
  for (i = 0; i < 2; i++) {
    command[i] = tolower(command[i]);
  }
  /* execute command */
  if (strlen(command)) execute(command);
  return FALSE;
} /* CommandCB */

/* ======================================================================= */
static int ReportPointerPos(GtkWidget *w, GdkEventMotion *event, gpointer cb_data)
{
  float   e, de, x1, y1;
  int     ilev, igam, ixi, iyi;
  extern int nearest_gamma(float, float);
  extern int nearest_level(float, float);
  char    buf[120];
  GdkModifierType state;

  if (event->is_hint) {
    //gdk_window_get_pointer(drawing_area_gls->window, &ixi, &iyi, &state);
    gdk_window_get_pointer(w->window, &ixi, &iyi, &state);
  } else {
    ixi = event->x; iyi = event->y;
  }
  removecrosshairs();
  if (w == drawing_area_gls) {
    chx = -1;
    select_gw(2);
    iyi = win_height - iyi;
    cvxy(&x1, &y1, &ixi, &iyi, 2);
    sprintf(buf, "y = %.0f", y1);
    igam = nearest_gamma(x1, y1);
    if (igam >= 0)
      sprintf(buf + strlen(buf), "    Gamma: %.1f keV,  I = %.2f",
	      glsgd.gam[igam].e, glsgd.gam[igam].i);
    ilev = nearest_level(x1, y1);
    if (ilev >= 0)
      sprintf(buf + strlen(buf), "    Level: %s,  E = %.1f",
	      glsgd.lev[ilev].name, glsgd.lev[ilev].e);
    gtk_label_set_text(GTK_LABEL(pos_text), buf);
  } else {
    chx = ixi;
    select_gw(1);
    drawcrosshairs();
    iyi = win_height - iyi;
    cvxy(&x1, &y1, &ixi, &iyi, 2);
    energy(x1 - 0.5f, 0.0f, &e, &de);
    sprintf(buf, "Egamma = %.1f", e);
    gtk_label_set_text(GTK_LABEL(pos_text), buf);
  }
  return FALSE;
} /* ReportPointerPos */

/* ======================================================================= */
/* Callback for allowing us to delete command buttons from the toolbar */
static int button_delete_check(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  if ((event->button == Button2 || event->button == Button3) &&
      widget != undo_button &&
      widget != redo_button &&
      widget != save_button &&
      widget != zin_button  &&
      widget != zout_button &&
      askyn("Delete this button from the toolbar? (Y/N)"))
    gtk_widget_destroy(widget);
  return FALSE;
} /* button_delete_check */

/* ======================================================================= */
/* convenience function to make command buttons in the toolbar */
GtkWidget* make_cmd_button(const char *lbl, const char *cmd, const char *tip)
{
  GtkWidget *button;

  button = gtk_button_new_with_label(lbl);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(ButtonCB), (gpointer) cmd);
  gtk_signal_connect(GTK_OBJECT(button), "button_press_event",
		     (GtkSignalFunc) button_delete_check, NULL);
  gtk_box_pack_start(GTK_BOX(toolbar), button, FALSE, TRUE, 3);
  if (*tip) gtk_tooltips_set_tip(tooltips, button, tip, NULL);
  return button;
}

/* ======================================================================= */
/***** Main Logic *****/
int main(int argc, char **argv)
{
  int       i, nmf_items = sizeof (mf_items) / sizeof (mf_items[0]);
  GtkWidget *button, *vbox, *hbox, *vbox2, *vpaned, *hpaned;
  GtkWidget *menu=NULL, *menu_button, *item, *menu1=NULL;
  char      buf[80], *c;
  int       w_in, h_in;
  int       xorig = 0, yorig = 0;


  /* Initialize some global variables */
  disp_id = XOpenDisplay(0);
  if (!disp_id) {
    fprintf(stderr, "Display not opened!\n");
    exit(-1);
  }
  screen_id  = XDefaultScreenOfDisplay(disp_id);
  scr_width  = XWidthOfScreen(screen_id);
  scr_height = XHeightOfScreen(screen_id);

  gtk_init (&argc, &argv);
  
  /* Extract pixmap width and height from RADWARE_XMG_SIZE */
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
  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
		     GTK_SIGNAL_FUNC(exit_gtkgls), NULL);
  gtk_signal_connect(GTK_OBJECT(toplevel),"configure_event",
     (GtkSignalFunc) configure_event, NULL);
#ifdef GTK2D
  gtk_window_set_title(GTK_WINDOW(toplevel), "GtkEscl8r");
#endif
#ifdef GTK3D
  gtk_window_set_title(GTK_WINDOW(toplevel), "GtkLevit8r");
#endif
#ifdef GTK4D
  gtk_window_set_title(GTK_WINDOW(toplevel), "Gtk4dg8r");
#endif
  tooltips = gtk_tooltips_new();

  /* Set size and position of main window */
  xmg[2] = 0;
  set_xwg("gtk_234_");
  if (xmg[2] == 0) {
    xmg[0] = 0.0f;
    xmg[1] = 0.0f;
    xmg[2] = 0.6f;
    xmg[3] = 0.8f;
  }
  xorig = xmg[0]*(float)scr_width + 0.5f;
  yorig = xmg[1]*(float)scr_height + 0.5f;
  ww = xmg[2]*(float)scr_width + 0.5f;
  hh = xmg[3]*(float)scr_height + 0.5f;

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(vbox), 1);
  gtk_container_add(GTK_CONTAINER(toplevel), vbox);

  /* Make main menu bar and menus */
  menubar = gtk_menu_bar_new();
  for (i=0; i<nmf_items; i++) {
    if (mf_items[i].item_type == 1 || mf_items[i].item_type == 4) {
      /* create a new menu in the menu bar */
      menu_button = gtk_menu_item_new_with_label(mf_items[i].label);
      if (mf_items[i].item_type == 4) {
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_button));
      }
      gtk_menu_bar_append(GTK_MENU_BAR(menubar), menu_button);
      gtk_widget_show(menu_button);
      menu = gtk_menu_new();    /* don't need to show menus, only menu items and menu buttons */
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_button), menu);
      menu1 = menu;
    } else if (mf_items[i].item_type == 2) {
      /* create a new separator in the menu */
      item = gtk_separator_menu_item_new();
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      gtk_widget_show(item);
    } else if (mf_items[i].item_type == 3) {
      /* create a new submenu in the menu */
      /* create the menu item and add it to the menu */
      item = gtk_menu_item_new_with_label(mf_items[i].label);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu1), item);
      gtk_widget_show(item);
      /* create the new submenu */
      menu = gtk_menu_new();    /* don't need to show menus, only menu items and menu buttons */
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    } else if (mf_items[i].item_type == -3) {
      /* end the submenu, go back to adding items to the original menu */
      menu  = menu1;
    } else if (mf_items[i].item_type == 0) {
      /* create a new menu item in the menu or submenu and add it to the menu */
      item = gtk_menu_item_new_with_label(mf_items[i].label);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      /* attach the callback functions to the activate signal, and show the menu item */
      gtk_signal_connect(GTK_OBJECT(item), "activate",
			 GTK_SIGNAL_FUNC(ButtonCB), (gpointer) mf_items[i].action);
      gtk_widget_show(item);
      /* save selected menuitem widgets for enable/disable later */
      if (strncmp(mf_items[i].action, "ue", 2) == 0) undo_menu_item = item;
      if (strncmp(mf_items[i].action, "re", 2) == 0) redo_menu_item = item;
      if (strncmp(mf_items[i].action, "sl", 2) == 0) save_menu_item = item;
      if (strncmp(mf_items[i].action, "zo", 2) == 0) zout_menu_item = item;
      if (strncmp(mf_items[i].action, "bg", 2) == 0) backg_menu_item = item;
      if (strncmp(mf_items[i].action, "fg", 2) == 0) fwdg_menu_item = item;
    }
  }

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  /* Create horizontal container box (toolbar) below menus */
  toolbar = gtk_hbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(toolbar), 1);
  gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);

  /* Create undo and redo buttons etc*/
  undo_button = make_cmd_button("UnDo",    "ue", "Undo previous level scheme edits");
  redo_button = make_cmd_button("ReDo",    "re", "Re-do previously undone level scheme edits");
  save_button = make_cmd_button("Save",    "sl", "Save level scheme");
  zin_button  = make_cmd_button(" ZIn ",   "zi", "Zoom in level scheme display");
  zout_button = make_cmd_button("ZOut",    "zo", "Zoom out level scheme display");
  button = make_cmd_button("Exam Gam", "xg", "Examine gamma-ray parameters");
  backg_button = make_cmd_button("Back Sp",  "bg", "Go Back to Previous Gate");
  fwdg_button  = make_cmd_button("Fwd Sp",   "fg", "Go Forward to Next Gate");
  button = make_cmd_button("Sp Disp",  "sd", "Set up Spectrum Display");
  gtk_widget_set_sensitive(undo_button, FALSE);
  gtk_widget_set_sensitive(redo_button, FALSE);
  gtk_widget_set_sensitive(save_button, FALSE);
  gtk_widget_set_sensitive(zout_button, FALSE);
  gtk_widget_set_sensitive(backg_button, FALSE);
  gtk_widget_set_sensitive(fwdg_button, FALSE);

  gtk_widget_set_sensitive(undo_menu_item, FALSE);
  gtk_widget_set_sensitive(redo_menu_item, FALSE);
  gtk_widget_set_sensitive(save_menu_item, FALSE);
  gtk_widget_set_sensitive(zout_menu_item, FALSE);
  gtk_widget_set_sensitive(backg_menu_item, FALSE);
  gtk_widget_set_sensitive(fwdg_menu_item, FALSE);

  button = gtk_button_new_with_label("Add Button");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(ButtonCB), (gpointer) "aa");
  gtk_box_pack_end(GTK_BOX(toolbar), button, FALSE, TRUE, 3);
  gtk_tooltips_set_tip(tooltips, button, "Add a new command button to this toolbar", NULL);

  /* Create a vertical 2-pane window */
  vpaned = gtk_vpaned_new();
  gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
 
  /* Create a horizontal 2-pane window */
  hpaned = gtk_hpaned_new();
  gtk_paned_pack1(GTK_PANED(vpaned), hpaned, TRUE, FALSE);
 
  /* Create the level scheme display drawing area
     inside a scrolled window for the top left pane */
  gls_scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(gls_scrolled), 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gls_scrolled),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_pack1(GTK_PANED(hpaned), gls_scrolled, TRUE, FALSE);
  drawing_area_gls = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area_gls), width, height);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(gls_scrolled), drawing_area_gls);

  /* Create the output text area in top right pane */
  vbox2 = gtk_vbox_new(FALSE, 1);
  text_scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(text_scrolled), 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(text_scrolled),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(vbox2), text_scrolled, TRUE, TRUE, 0);
  output_text_view = gtk_text_view_new();
  gtk_tooltips_set_tip(tooltips, output_text_view,
		       "Displays information and messages from the program", NULL);
  gtk_tooltips_set_tip(tooltips, text_scrolled,
		       "Displays information and messages from the program", NULL);
  output_text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_text_view));
  gtk_text_view_set_editable(GTK_TEXT_VIEW(output_text_view), FALSE);
  // gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output_text_view), GTK_WRAP_WORD);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(text_scrolled), output_text_view);
  // gtk_box_pack_start(GTK_BOX(vbox2), output_text_view, TRUE, TRUE, 0);

  /* Create the spectrum display drawing area
     inside a scrolled window for the bottom pane */
  sp_scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(sp_scrolled), 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sp_scrolled),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_pack2(GTK_PANED(vpaned), sp_scrolled, FALSE, TRUE);
  drawing_area_sp = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area_sp), ww - 40, hh*2/5 - 120);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sp_scrolled), drawing_area_sp);

  /* Signals used to handle backing pixmap */
  gtk_signal_connect(GTK_OBJECT(drawing_area_gls), "expose_event",
		     (GtkSignalFunc) DAexposeCB, NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area_sp), "expose_event",
		     (GtkSignalFunc) DAexposeCB, NULL);

  /* Event signals */
  gtk_signal_connect(GTK_OBJECT(drawing_area_gls), "motion_notify_event",
		     (GtkSignalFunc) ReportPointerPos, NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area_gls), "button_press_event",
		     (GtkSignalFunc) DAbuttonCB, NULL);
//   gtk_signal_connect(GTK_OBJECT(drawing_area_gls), "key_press_event",
// 		     (GtkSignalFunc) DAkeyCB, NULL);  // has no effect!
  gtk_signal_connect(GTK_OBJECT(drawing_area_sp), "motion_notify_event",
		     (GtkSignalFunc) ReportPointerPos, NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area_sp), "button_press_event",
		     (GtkSignalFunc) DAbuttonCB, NULL);
//   gtk_signal_connect(GTK_OBJECT(drawing_area_sp), "key_press_event",
// 		     (GtkSignalFunc) DAkeyCB, NULL);  // has no effect!
  gtk_signal_connect(GTK_OBJECT(toplevel), "key_press_event",
		     (GtkSignalFunc) DAkeyCB, NULL);

  gtk_widget_set_events(drawing_area_gls, GDK_EXPOSURE_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_KEY_PRESS_MASK
                         | GDK_POINTER_MOTION_MASK
		         | GDK_POINTER_MOTION_HINT_MASK );
  gtk_widget_set_events(drawing_area_sp, GDK_EXPOSURE_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_KEY_PRESS_MASK
                         | GDK_POINTER_MOTION_MASK
		         | GDK_POINTER_MOTION_HINT_MASK );

  /* Create entry widget for commands */
  hbox = gtk_hbox_new(FALSE, 1);
  label = gtk_label_new("Use menus/buttons, or enter commands here:");
  gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 3);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 2);
  // gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, FALSE, 2);
  hbox = gtk_hbox_new(FALSE, 1);
  command_entry = gtk_entry_new();
  gtk_tooltips_set_tip(tooltips, command_entry,
		       "Allows you to enter standard escl8r commands", NULL);
  gtk_entry_set_max_length(GTK_ENTRY(command_entry), 80);
  gtk_widget_set_usize(command_entry, 250, 0);
  gtk_signal_connect(GTK_OBJECT(command_entry), "activate",
		     GTK_SIGNAL_FUNC(CommandCB), NULL);
  gtk_box_pack_end(GTK_BOX(hbox), command_entry,TRUE, FALSE, 3);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox,  FALSE, FALSE, 2);
  // gtk_box_pack_start(GTK_BOX(vbox2), command_entry, FALSE, FALSE, 2);

  /* Create label for pointer-over information on gammas, levels etc */
  hbox = gtk_hbox_new(FALSE, 1);
  /* add one constant label to avoid vertical resizing requests */
  pos_text = gtk_label_new(" ");
  gtk_label_set_justify (GTK_LABEL(pos_text), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox), pos_text, FALSE, FALSE, 3);
  pos_text = gtk_label_new(" ");
  gtk_label_set_justify (GTK_LABEL(pos_text), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox), pos_text, FALSE, TRUE, 3);
  /* gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2); */
  /* gtk_paned_pack2(GTK_PANED(hpaned), hbox, FALSE, TRUE); */
  gtk_box_pack_end(GTK_BOX(vbox2), hbox, FALSE, FALSE, 2);
  gtk_paned_pack2(GTK_PANED(hpaned), vbox2, FALSE, TRUE);
  gtk_signal_connect(GTK_OBJECT(command_entry), "button_press_event",
		     (GtkSignalFunc) DAbuttonCB, NULL);

  gtk_widget_show_all(toplevel);
  /* move and resize top level window to saved values */
  gdk_window_resize(toplevel->window, ww, hh);
  move_win_to(toplevel, xorig, yorig);
 
  /* Create pixmap the maximum size of drawing area */
  /* printf("Creating pixmap of size 4*%d by 4*%d\n", width, height); */
  pixmap = gdk_pixmap_new(drawing_area_gls->window, 4*width, 4*height, -1);

  /* Create graphics contexts */
  xmgc       = drawing_area_gls->style->white_gc;
  gc_id      = gdk_gc_new(drawing_area_gls->window);
  gc_comp_id = gdk_gc_new(drawing_area_gls->window);
  gdk_gc_copy(gc_id, xmgc);
  gdk_gc_copy(gc_comp_id, xmgc);

  /* Get named color values */

  colormap = gdk_colormap_get_system();
  set_minig_color_rgb(0,  0.0f, 0.0f, 0.0f);
  set_minig_color_rgb(1,  1.0f, 1.0f, 1.0f); /* white */
  set_minig_color_rgb(2,  0.0f, 0.0f, 0.0f); /* black */
  set_minig_color_rgb(3,  1.0f, 0.0f, 0.0f); /* red */
  set_minig_color_rgb(4,  0.0f, 0.0f, 1.0f); /* blue */
  set_minig_color_rgb(5,  1.0f, 0.0f, 1.0f); /* magenta */
  set_minig_color_rgb(6,  0.4f, 0.2f, 0.8f); /* slate blue */
  set_minig_color_rgb(7,  1.0f, 0.2f, 0.2f); /* orange red */
  set_minig_color_rgb(8,  0.0f, 0.6f, 0.0f); /* forest green */
  set_minig_color_rgb(9,  0.2f, 1.0f, 0.2f); /* green yellow */
  set_minig_color_rgb(10, 0.7f, 0.0f, 0.1f); /* maroon */
  set_minig_color_rgb(11, 1.0f, 0.4f, 0.4f); /* coral */
  set_minig_color_rgb(12, 0.3f, 0.7f, 0.7f); /* cadet blue */
  set_minig_color_rgb(13, 1.0f, 0.4f, 0.4f); /* orange */
  set_minig_color_rgb(14, 0.6f, 1.0f, 0.6f); /* pale green */
  set_minig_color_rgb(15, 0.2f, 0.2f, 0.2f); /* grey20 */
  set_minig_color_rgb(16, 0.4f, 0.4f, 0.4f); /* grey40 */

  gdk_gc_set_foreground(xmgc, &color_id[1]);
  gdk_gc_set_foreground(gc_id, &color_id[2]);
  gdk_gc_set_background(gc_id, &color_id[1]);
  gdk_gc_set_foreground(gc_comp_id, &color_id[2]);
  /* set GC for complement drawing */
  gdk_gc_set_function  (gc_comp_id, GDK_INVERT);

  /* Load the font for text writing */
  font_id = gdk_font_load(FONTNAME);
  /* gdk_gc_set_font(gc_id, font_id); */

  window = drawing_area_gls->window;

  /* Initialise gls etc subroutines */
  select_gw(1);
  win_id  = drawing_area_sp->window;
  select_gw(2);
  win_width = (int) width;
  win_height = (int) height;
  win_id  = pixmap;
  winpix = pixmap;
#ifdef GTK2D
  e_gls_init(argc, argv);
#endif
#ifdef GTK3D
  l_gls_init(argc, argv);
#endif
#ifdef GTK4D
  fdg8r_init(argc, argv);
#endif

  /* set position of paned window gutter */
  while (gtk_events_pending()) gtk_main_iteration();
  gtk_paned_set_position(GTK_PANED(vpaned), 5*hh/9);
  gtk_paned_set_position(GTK_PANED(hpaned), 5*ww/9);
  win_w = ww;
  win_h = hh;

  execute("xg");
  /* Get and dispatch events */
  gtk_main ();
  return 0;
} /* main */

/* ==========  dialog for setting parameters of spectrum display  ========== */

void apply_sp_disp_dlg(void)
{

  if (!sp_disp_dlg.active) return;

  strncpy(sp_disp_dlg.x0,    gtk_entry_get_text(GTK_ENTRY(sp_disp_dlg.entry[1])), 20);
  strncpy(sp_disp_dlg.nx,    gtk_entry_get_text(GTK_ENTRY(sp_disp_dlg.entry[2])), 20);
  strncpy(sp_disp_dlg.y0,    gtk_entry_get_text(GTK_ENTRY(sp_disp_dlg.entry[8])), 20);
  strncpy(sp_disp_dlg.ny,    gtk_entry_get_text(GTK_ENTRY(sp_disp_dlg.entry[9])), 20);
  strncpy(sp_disp_dlg.my,    gtk_entry_get_text(GTK_ENTRY(sp_disp_dlg.entry[10])), 20);
  strncpy(sp_disp_dlg.ng,    gtk_entry_get_text(GTK_ENTRY(sp_disp_dlg.entry[12])), 20);
  strncpy(sp_disp_dlg.sigma, gtk_entry_get_text(GTK_ENTRY(sp_disp_dlg.entry[16])), 20);

  elgd.lox = atoi(sp_disp_dlg.x0);
  if ((sp_disp_dlg.auto_x =
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[0])))) {
    elgd.numx = 0;
  } else {
    if ((elgd.numx = atoi(sp_disp_dlg.nx)) < 0) elgd.numx = 0;
    if (elgd.numx <= 0) {
      strcpy(sp_disp_dlg.nx, "1000");
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[2]), sp_disp_dlg.nx);
    }
  }

  elgd.locnt = atoi(sp_disp_dlg.y0);
  if ((sp_disp_dlg.auto_y =
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[7])))) {
    elgd.ncnts = 0;
    if ((elgd.min_ny = atoi(sp_disp_dlg.my)) < 0.1) {
#ifdef GTK2D
      elgd.min_ny = 100.0;
      strcpy(sp_disp_dlg.my, "100");
#else
      elgd.min_ny = 20.0;
      strcpy(sp_disp_dlg.my, "20");
#endif
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[10]), sp_disp_dlg.my);
    }
  } else {
    if ((elgd.ncnts = atoi(sp_disp_dlg.ny)) < 0) elgd.ncnts = 0;
    if (elgd.ncnts <= 0) {
      strcpy(sp_disp_dlg.ny, "200");
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[9]), sp_disp_dlg.ny);
    }
  }

  if ((sp_disp_dlg.single_gate =
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[11])))) {
    elgd.ngd = 1;
  } else {
    if ((elgd.ngd = atoi(sp_disp_dlg.ng)) < 1) elgd.ngd = 1;
    if (elgd.ngd <= 1) {
      strcpy(sp_disp_dlg.ng, "2");
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[12]), sp_disp_dlg.ng);
    }
    if (elgd.ngd > 8) {
      elgd.ngd = 8;
      strcpy(sp_disp_dlg.ng, "8");
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[12]), sp_disp_dlg.ng);
    }
  }

  elgd.disp_calc = 0;
  if ((sp_disp_dlg.disp_calc =
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[13])))) {
    elgd.disp_calc = 2;
    if ((sp_disp_dlg.disp_diff =
	 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[14])))) {
      elgd.disp_calc = 1;
    }
  }

  if ((elgd.pkfind = sp_disp_dlg.pkfind = 
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[15])))) {
    if ((elgd.isigma = atoi(sp_disp_dlg.sigma)) < 1) elgd.isigma = 1;
    if (elgd.isigma <= 1) {
      strcpy(sp_disp_dlg.sigma, "1");
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[16]), sp_disp_dlg.sigma);
    }
  }

  execute("ds");
} /* apply_sp_disp_dlg */

void destroy_sp_disp_dlg(void)
{
  if (sp_disp_dlg.done > 0) apply_sp_disp_dlg();

  gtk_widget_destroy(sp_disp_dlg.w);
  while (gtk_events_pending()) gtk_main_iteration();
  sp_disp_dlg.w = NULL;
} /* destroy_sp_disp_dlg */

int delete_sp_disp_dlg(void)
{
  sp_disp_dlg.done = -1;
  destroy_sp_disp_dlg();
  return TRUE;
} /* delete_sp_disp_dlg */

void reset_sp_disp_dlg(void)
{
  sprintf(sp_disp_dlg.x0, "%d", elgd.lox);
  if ((sp_disp_dlg.auto_x = (elgd.numx < 1))) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[0]), TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[1], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[2], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[1], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[2], FALSE);
  } else {
    sprintf(sp_disp_dlg.nx, "%d", elgd.numx);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[0]), FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[1], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[2], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[1], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[2], TRUE);
  }
  sprintf(sp_disp_dlg.y0, "%d", elgd.locnt);
  if ((sp_disp_dlg.auto_y = (elgd.ncnts < 1))) {
    sprintf(sp_disp_dlg.my, "%.0f", elgd.min_ny);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[7]), TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[8], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[9], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[10], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[4], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[5], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[6], TRUE);
  } else {
    sprintf(sp_disp_dlg.ny, "%d", elgd.ncnts);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[7]), FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[8], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[9], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[10], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[4], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[5], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[6], FALSE);
  }
  if ((sp_disp_dlg.single_gate = (elgd.ngd < 2))) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[11]), TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[12], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[8], FALSE);
  } else {
    sprintf(sp_disp_dlg.ng, "%d", elgd.ngd);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[11]), FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[12], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[8], TRUE);
  }
  if ((sp_disp_dlg.disp_calc = (elgd.disp_calc != 0))) {
    sp_disp_dlg.disp_diff = (elgd.disp_calc == 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[13]), TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[14], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[10], TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[13]), FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[14], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[10], FALSE);
  }
  if ((sp_disp_dlg.disp_diff = (elgd.disp_calc == 1))) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[14]), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[14]), FALSE);
  }
  sprintf(sp_disp_dlg.sigma, "%d", elgd.isigma);
  if ((sp_disp_dlg.pkfind = elgd.pkfind)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[15]), TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[16], TRUE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[12], TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_disp_dlg.entry[15]), FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.entry[16], FALSE);
    gtk_widget_set_sensitive(sp_disp_dlg.label[12], FALSE);
  }
  gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[1]), sp_disp_dlg.x0);
  gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[2]), sp_disp_dlg.nx);
  gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[8]), sp_disp_dlg.y0);
  gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[9]), sp_disp_dlg.ny);
  gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[10]), sp_disp_dlg.my);
  gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[12]), sp_disp_dlg.ng);
  gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[16]), sp_disp_dlg.sigma);
} /* reset_sp_disp_dlg */

void sp_disp_dlg_cb(GtkWidget *w, gpointer data)
{

  if (!strcmp((char *)data, "Ok")) {
    sp_disp_dlg.done = 1;
    destroy_sp_disp_dlg();
  } else if (!strcmp((char *)data, "Reset")) {
    reset_sp_disp_dlg();
  } else if (!strcmp((char *)data, "Cancel")) {
    sp_disp_dlg.done = -1;
    destroy_sp_disp_dlg();
  } else if (!strcmp((char *)data, "Expand")) {
    execute("ex");
    tell("%d %d\n", elgd.lox, elgd.numx);
    if (sp_disp_dlg.w && elgd.numx > 1) {
      gdk_window_raise(sp_disp_dlg.w->window);
      sprintf(sp_disp_dlg.x0, "%d", elgd.lox);
      sprintf(sp_disp_dlg.nx, "%d", elgd.numx);
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[1]), sp_disp_dlg.x0);
      gtk_entry_set_text(GTK_ENTRY(sp_disp_dlg.entry[2]), sp_disp_dlg.nx);
      reset_sp_disp_dlg();
    }
  } else if (!strcmp((char *)data, "DS1")) {
    execute("ds1");
  } else if (!strcmp((char *)data, "Zoom_in")) {
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area_sp),
			  13*drawing_area_sp->allocation.width/10,
			  drawing_area_sp->allocation.height);
  } else if (!strcmp((char *)data, "Zoom_out")) {
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area_sp),
			  10*drawing_area_sp->allocation.width/13,
			  drawing_area_sp->allocation.height);
  } else if (!strcmp((char *)data, "ResetY")) {
    elgd.ncnts /= elgd.multy;
    elgd.multy = 1.0f;
    reset_sp_disp_dlg();
    apply_sp_disp_dlg();
  } else if (!strcmp((char *)data, "reset")) {
    reset_sp_disp_dlg();
  } else if ((!strncmp((char *)data, "tog", 3) ||
	      !strncmp((char *)data, "act", 3)) && sp_disp_dlg.active) {
    if ((!strncmp((char *)data, "tog7", 4))) elgd.multy = 1.0f;
    apply_sp_disp_dlg();
    reset_sp_disp_dlg();
  }
} /* sp_disp_dlg_cb */

int get_sp_disp_details(void)
{
  GtkWidget *button_ok, *button_reset, *button_cancel;
  GtkWidget *vbox, *hbox, *sep;

  static int  first = 1;

  if (sp_disp_dlg.w) return 1;  /* do not do anything if widget already in existence */

  sp_disp_dlg.active = 0;
  if (first) {
    first = 0;
    strcpy(sp_disp_dlg.x0, "0");
    strcpy(sp_disp_dlg.nx, "1000");
    strcpy(sp_disp_dlg.y0, "0");
    strcpy(sp_disp_dlg.ny, "200");
    strcpy(sp_disp_dlg.ng, "2");
    strcpy(sp_disp_dlg.sigma, "5");
    sp_disp_dlg.w = 0;
  } else if (sp_disp_dlg.w) {
    sp_disp_dlg.done = -1;
    destroy_sp_disp_dlg();
  }

  sp_disp_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sp_disp_dlg.w)->vbox), vbox);
  set_title(sp_disp_dlg.w, "Spectrum display options");

  sp_disp_dlg.entry[0] = gtk_check_button_new();
  sp_disp_dlg.label[0] = gtk_label_new(" Autoscale X-axis ");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.label[0], FALSE, TRUE, 0);

  sp_disp_dlg.label[1] = gtk_label_new("X origin: ");
  sp_disp_dlg.entry[1] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_disp_dlg.entry[1]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[1], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.label[1], FALSE, TRUE, 0);

  sp_disp_dlg.label[2] = gtk_label_new("X range: ");
  sp_disp_dlg.entry[2] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_disp_dlg.entry[2]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[2], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.label[2], FALSE, TRUE, 0);

  sp_disp_dlg.entry[3] = gtk_button_new_with_label("Expand X");
  sp_disp_dlg.entry[4] = gtk_button_new_with_label("Redraw full");
  sp_disp_dlg.entry[5] = gtk_button_new_with_label("Zoom in");
  sp_disp_dlg.entry[6] = gtk_button_new_with_label("Zoom out");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[3], TRUE, TRUE, 3);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[4], TRUE, TRUE, 3);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[5], TRUE, TRUE, 3);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[6], TRUE, TRUE, 3);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  sp_disp_dlg.entry[7] = gtk_check_button_new();
  sp_disp_dlg.label[3] = gtk_label_new(" Autoscale Y-axis ");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[7], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.label[3], FALSE, TRUE, 0);

  sp_disp_dlg.label[4] = gtk_label_new("Y origin: ");
  sp_disp_dlg.entry[8] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_disp_dlg.entry[8]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[8], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.label[4], FALSE, TRUE, 0);

  sp_disp_dlg.label[5] = gtk_label_new("Y range: ");
  sp_disp_dlg.entry[9] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_disp_dlg.entry[9]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[9], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.label[5], FALSE, TRUE, 0);

  sp_disp_dlg.label[6] = gtk_label_new("Minimum Y scale: ");
  sp_disp_dlg.entry[10] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_disp_dlg.entry[10]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[10], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.label[6], FALSE, TRUE, 0);

  sp_disp_dlg.entry[17] = gtk_button_new_with_label("Shrink Y-scale");
  sp_disp_dlg.entry[18] = gtk_button_new_with_label("Expand Y-scale");
  sp_disp_dlg.entry[19] = gtk_button_new_with_label("Reset Y-scale");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[19], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[18], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[17], FALSE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  sp_disp_dlg.entry[11] = gtk_check_button_new();
  sp_disp_dlg.label[7] = gtk_label_new(" Display only one gate at a time ");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[11], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.label[7], FALSE, TRUE, 0);

  sp_disp_dlg.label[8] = gtk_label_new("Number of gates to display: ");
  sp_disp_dlg.entry[12] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_disp_dlg.entry[12]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[12], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.label[8], FALSE, TRUE, 0);

  sp_disp_dlg.entry[13] = gtk_check_button_new();
  sp_disp_dlg.label[9] = gtk_label_new(" Display calculated spectra ");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[13], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.label[9], FALSE, TRUE, 0);

  sp_disp_dlg.entry[14] = gtk_check_button_new();
  sp_disp_dlg.label[10] = gtk_label_new(" Display difference spectra ");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[14], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.label[10], FALSE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  sp_disp_dlg.entry[15] = gtk_check_button_new();
  sp_disp_dlg.label[11] = gtk_label_new(" Use peak-find on displayed spectra ");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.entry[15], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_disp_dlg.label[11], FALSE, TRUE, 0);

  sp_disp_dlg.label[12] = gtk_label_new("Peak-find sigma limit: ");
  sp_disp_dlg.entry[16] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_disp_dlg.entry[16]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.entry[16], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_disp_dlg.label[12], FALSE, TRUE, 0);

  button_ok     = gtk_button_new_with_label("Okay");
  button_reset  = gtk_button_new_with_label("Reset");
  button_cancel = gtk_button_new_with_label("Cancel");

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sp_disp_dlg.w)->action_area),
		    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sp_disp_dlg.w)->action_area),
		    button_reset);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sp_disp_dlg.w)->action_area),
		    button_cancel);

  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[3]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "Expand");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[4]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "DS1");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[5]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "Zoom_in");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[6]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "Zoom_out");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[0]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "tog0");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[7]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "tog7");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[11]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "tog11");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[13]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "tog13");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[14]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "tog14");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[15]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "tog15");
  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "Ok");
  gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "Reset");
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), (gpointer) "Cancel");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_sp_disp_dlg), NULL);
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[1]), "activate",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "act");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[2]), "activate",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "act");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[8]), "activate",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "act");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[9]), "activate",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "act");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[10]), "activate",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "act");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[12]), "activate",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "act");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[16]), "activate",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "act");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[17]), "clicked",
		     GTK_SIGNAL_FUNC(ButtonCB), (gpointer) "ny+");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[18]), "clicked",
		     GTK_SIGNAL_FUNC(ButtonCB), (gpointer) "ny-");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[17]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "reset");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[18]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "reset");
  gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[19]), "clicked",
		     GTK_SIGNAL_FUNC(sp_disp_dlg_cb), "ResetY");
  //gtk_signal_connect(GTK_OBJECT(sp_disp_dlg.entry[19]), "clicked",
  //		     GTK_SIGNAL_FUNC(ButtonCB), (gpointer) "ny0");

  /* Display the dialog */
  gtk_widget_show_all(sp_disp_dlg.w);
  gtk_widget_set_usize(sp_disp_dlg.entry[1], 80, 0);
  gtk_widget_set_usize(sp_disp_dlg.entry[2], 80, 0);
  gtk_widget_set_usize(sp_disp_dlg.entry[8], 80, 0);
  gtk_widget_set_usize(sp_disp_dlg.entry[9], 80, 0);
  gtk_widget_set_usize(sp_disp_dlg.entry[10], 80, 0);
  gtk_widget_set_usize(sp_disp_dlg.entry[12], 80, 0);
  gtk_widget_set_usize(sp_disp_dlg.entry[16], 80, 0);

  gtk_window_set_transient_for(GTK_WINDOW(sp_disp_dlg.w), GTK_WINDOW(toplevel)); /* set parent */
  move_win_to(sp_disp_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(sp_disp_dlg.w), button_ok);
  reset_sp_disp_dlg();

  sp_disp_dlg.done = 0;
  sp_disp_dlg.active = 1;
  return 0;
} /* get_sp_disp_details */

/* ==========  dialog for creating ps plot of spectrum display  ========== */

int delete_sp_ps_dlg(void)
{
  sp_ps_dlg.done = -2;
  sp_ps_dlg.w = NULL;
  return FALSE;
} /* delete_sp_ps_dlg */

void sp_ps_dlg_cb(GtkWidget *w, gpointer data)
{
  char  ans[80];

  if (!strcmp((char *)data, "Ok")) {
    sp_ps_dlg.done = 1;

    log_to_cmd_file("shc");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_ps_dlg.entry[0]))) {
      sp_ps_dlg.use_color = 1;
      log_to_cmd_file("y");
    } else {
      sp_ps_dlg.use_color = 0;
      log_to_cmd_file("n");
    }
    strncpy(sp_ps_dlg.filnam, gtk_entry_get_text(GTK_ENTRY(sp_ps_dlg.entry[2])), 80);
    log_to_cmd_file(sp_ps_dlg.filnam);
    strncpy(sp_ps_dlg.printcmd, gtk_entry_get_text(GTK_ENTRY(sp_ps_dlg.entry[4])), 80);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_ps_dlg.entry[1]))) {
      sp_ps_dlg.print_now = 0;
      log_to_cmd_file("n");
    } else {
      sp_ps_dlg.print_now = 1;
      log_to_cmd_file("y");
      log_to_cmd_file(sp_ps_dlg.printcmd + 4);
    }

    gtk_widget_destroy(sp_ps_dlg.w);
    while (gtk_events_pending()) gtk_main_iteration();
    sp_ps_dlg.w = NULL;

    hcopy_use_preset_values = 1;
    hcopy_use_color = sp_ps_dlg.use_color;
    hcopy_print_now = sp_ps_dlg.print_now;
    strncpy(hcopy_filename, sp_ps_dlg.filnam, 80);
    strncpy(hcopy_print_cmd, sp_ps_dlg.printcmd, 80);
    hcopy();
    hcopy_use_preset_values = 0;

  } else if (!strcmp((char *)data, "Cancel")) {
    sp_ps_dlg.done = -1;
    gtk_widget_destroy(sp_ps_dlg.w);
    sp_ps_dlg.w = NULL;
  } else if (!strcmp((char *)data, "Browse") &&
	     askfn(ans, 80, sp_ps_dlg.filnam, ".ps",
		   "Output postscript file = ?\n")) {
    strcpy(sp_ps_dlg.filnam, ans);
    gtk_entry_set_text(GTK_ENTRY(sp_ps_dlg.entry[2]), sp_ps_dlg.filnam);
  } else if (!strcmp((char *)data, "tog1")) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp_ps_dlg.entry[1]))) {
      gtk_widget_set_sensitive(sp_ps_dlg.entry[2], TRUE);
      gtk_widget_set_sensitive(sp_ps_dlg.entry[3], TRUE);
      gtk_widget_set_sensitive(sp_ps_dlg.entry[4], FALSE);
      gtk_widget_set_sensitive(sp_ps_dlg.label[2], FALSE);
    } else {
      gtk_widget_set_sensitive(sp_ps_dlg.entry[2], FALSE);
      gtk_widget_set_sensitive(sp_ps_dlg.entry[3], FALSE);
      gtk_widget_set_sensitive(sp_ps_dlg.entry[4], TRUE);
      gtk_widget_set_sensitive(sp_ps_dlg.label[2], TRUE);
    }
  }
} /* sp_ps_dlg_cb */

int sp_hcopy(void)
{
  GtkWidget *button_ok, *button_cancel, *vbox, *hbox, *sep;

  static int  first = 1;

  if (sp_ps_dlg.w) return 1;  /* do not do anything if widget already in existence */

  if (first) {
    first = 0;
    sp_ps_dlg.use_color = 1;
    sp_ps_dlg.print_now = 1;
    strcpy(sp_ps_dlg.printcmd, "lpr ");
  }
  strcpy(sp_ps_dlg.filnam, "mhc.ps");

  sp_ps_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sp_ps_dlg.w)->vbox), vbox);

  set_title(sp_ps_dlg.w, "Spectrum Hardcopy Options");

  sp_ps_dlg.entry[0] = gtk_check_button_new();
  sp_ps_dlg.label[0] = gtk_label_new("Use color");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_ps_dlg.entry[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_ps_dlg.label[0], FALSE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  sp_ps_dlg.entry[1] = gtk_check_button_new();
  sp_ps_dlg.label[1] = gtk_label_new("Print to file:");
  sp_ps_dlg.entry[2] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_ps_dlg.entry[2]), 80);
  sp_ps_dlg.entry[3] = gtk_button_new_with_label("Browse...");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), sp_ps_dlg.entry[1], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_ps_dlg.label[1], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_ps_dlg.entry[2], TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), sp_ps_dlg.entry[3], FALSE, TRUE, 0);

  sp_ps_dlg.label[2] = gtk_label_new("Print command: ");
  sp_ps_dlg.entry[4] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(sp_ps_dlg.entry[4]), 80);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), sp_ps_dlg.entry[4], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), sp_ps_dlg.label[2], FALSE, TRUE, 0);

  button_ok     = gtk_button_new_with_label("Okay");
  button_cancel = gtk_button_new_with_label("Cancel");

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sp_ps_dlg.w)->action_area),
		    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sp_ps_dlg.w)->action_area),
		    button_cancel);

  gtk_signal_connect(GTK_OBJECT(sp_ps_dlg.entry[1]), "clicked",
		     GTK_SIGNAL_FUNC(sp_ps_dlg_cb), (gpointer) "tog1");
  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(sp_ps_dlg_cb), (gpointer) "Ok");
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(sp_ps_dlg_cb), (gpointer) "Cancel");
  gtk_signal_connect(GTK_OBJECT(sp_ps_dlg.entry[3]), "clicked",
		     GTK_SIGNAL_FUNC(sp_ps_dlg_cb), (gpointer) "Browse");
  gtk_signal_connect(GTK_OBJECT(sp_ps_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_sp_ps_dlg), NULL);

  /* Display the dialog */
  gtk_widget_show_all(sp_ps_dlg.w);
  gtk_window_set_transient_for(GTK_WINDOW(sp_ps_dlg.w), GTK_WINDOW(toplevel)); /* set parent */
  move_win_to(sp_ps_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(sp_ps_dlg.w), button_ok);

  if (sp_ps_dlg.use_color) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_ps_dlg.entry[0]), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_ps_dlg.entry[0]), FALSE);
  }
  if (sp_ps_dlg.print_now) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_ps_dlg.entry[1]), FALSE);
    gtk_widget_set_sensitive(sp_ps_dlg.entry[2], FALSE);
    gtk_widget_set_sensitive(sp_ps_dlg.entry[3], FALSE);
    gtk_widget_set_sensitive(sp_ps_dlg.entry[4], TRUE);
    gtk_widget_set_sensitive(sp_ps_dlg.label[2], TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp_ps_dlg.entry[1]), TRUE);
    gtk_widget_set_sensitive(sp_ps_dlg.entry[2], TRUE);
    gtk_widget_set_sensitive(sp_ps_dlg.entry[3], TRUE);
    gtk_widget_set_sensitive(sp_ps_dlg.entry[4], FALSE);
    gtk_widget_set_sensitive(sp_ps_dlg.label[2], FALSE);
  }
  gtk_entry_set_text(GTK_ENTRY(sp_ps_dlg.entry[2]), sp_ps_dlg.filnam);
  gtk_entry_set_text(GTK_ENTRY(sp_ps_dlg.entry[4]), sp_ps_dlg.printcmd);

  sp_ps_dlg.done = 0;
  while (!sp_ps_dlg.done) gtk_main_iteration();

  sp_ps_dlg.w = NULL;
  return 0;
} /* sp_hcopy */

/* ==========  dialog for writing out gate spectra  ========== */

int delete_write_sp_dlg(void)
{
  write_sp_dlg.done = -2;
  write_sp_dlg.w = NULL;
  return FALSE;
} /* delete_write_sp_dlg */

void write_sp_dlg_cb(GtkWidget *w, gpointer data)
{
  int   i, nc;
  char  filnam[80], ans[80];
  float spec[MAXCHS];

  if (!strcmp((char *)data, "Ok")) {
    write_sp_dlg.done = 1;

    log_to_cmd_file("ws");
#ifndef GTK2D
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(write_sp_dlg.entry[7]))) {
      write_sp_dlg.expand = 1;
      log_to_cmd_file("y");
    } else {
      write_sp_dlg.expand = 0;
      log_to_cmd_file("n");
    }
#endif
    strncpy(write_sp_dlg.filnam, gtk_entry_get_text(GTK_ENTRY(write_sp_dlg.entry[0])), 80);
    log_to_cmd_file(write_sp_dlg.filnam);
    for (i=0; i<5; i++) {
      write_sp_dlg.write[i] =
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(write_sp_dlg.entry[2+i]));
    }

    gtk_widget_destroy(write_sp_dlg.w);
    while (gtk_events_pending()) gtk_main_iteration();
    write_sp_dlg.w = NULL;

    strncpy(filnam, write_sp_dlg.filnam, 78);
    nc = strlen(filnam);
    strcpy(filnam + nc, "e");

    if (write_sp_dlg.write[0]) {
      log_to_cmd_file("y");
      wspec2(xxgd.spec[0], filnam, 'e', write_sp_dlg.expand);
    } else {
      log_to_cmd_file("n");
    }
    if (write_sp_dlg.write[1]) {
      log_to_cmd_file("y");
      wspec2(xxgd.spec[3], filnam, 'u', write_sp_dlg.expand);
    } else {
      log_to_cmd_file("n");
    }
    if (write_sp_dlg.write[2]) {
      log_to_cmd_file("y");
      wspec2(xxgd.spec[2], filnam, 'c', write_sp_dlg.expand);
    } else {
      log_to_cmd_file("n");
    }
    if (write_sp_dlg.write[3]) {
      log_to_cmd_file("y");
      for (i = 0; i < xxgd.numchs; ++i) {
	spec[i] = xxgd.spec[0][i] - xxgd.spec[2][i];
      }
      wspec2(spec, filnam, 'd', write_sp_dlg.expand);
    } else {
      log_to_cmd_file("n");
    }
    if (write_sp_dlg.write[4]) {
      log_to_cmd_file("y");
      for (i = 0; i < xxgd.numchs; ++i) {
	spec[i] = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
      }
      wspec2(spec, filnam, 'r', write_sp_dlg.expand);
    } else {
      log_to_cmd_file("n");
    }

  } else if (!strcmp((char *)data, "Cancel")) {
    write_sp_dlg.done = -1;
    gtk_widget_destroy(write_sp_dlg.w);
    write_sp_dlg.w = NULL;
  } else if (!strcmp((char *)data, "Browse") &&
	     askfn(ans, 80, write_sp_dlg.filnam, "",
		   "First part of spectrum files names = ?\n")) {
    strcpy(write_sp_dlg.filnam, ans);
    gtk_entry_set_text(GTK_ENTRY(write_sp_dlg.entry[0]), write_sp_dlg.filnam);
  }
} /* write_sp_dlg_cb */

int write_sp_2(void)
{
  GtkWidget *button_ok, *button_cancel, *vbox, *hbox, *sep;

  static int i, first = 1;

  if (write_sp_dlg.w) return 1;  /* do not do anything if widget already in existence */

  if (first) {
    first = 0;
    strcpy(write_sp_dlg.filnam, "gate_sp_");
    write_sp_dlg.expand = 1;
    write_sp_dlg.write[0] = 1;
    for (i=1; i<5; i++) {
      write_sp_dlg.write[i] = 0;
    }
  }

  write_sp_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(write_sp_dlg.w)->vbox), vbox);

  set_title(write_sp_dlg.w, "Spectrum Writing Options");

  write_sp_dlg.label[0] = gtk_label_new("First part of spectrum files names:");
  write_sp_dlg.entry[0] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(write_sp_dlg.entry[0]), 80);
  write_sp_dlg.entry[1] = gtk_button_new_with_label("Browse...");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), write_sp_dlg.label[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), write_sp_dlg.entry[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), write_sp_dlg.entry[1], TRUE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

#ifndef GTK2D
  write_sp_dlg.entry[7] = gtk_check_button_new();
  write_sp_dlg.label[6] = gtk_label_new("Expand spectra to linear ADC channels");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), write_sp_dlg.entry[7], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), write_sp_dlg.label[6], FALSE, TRUE, 0);
  if (write_sp_dlg.expand) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(write_sp_dlg.entry[7]), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(write_sp_dlg.entry[7]), FALSE);
  }

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);
#endif

  write_sp_dlg.label[1] = gtk_label_new("Write observed spectrum");
  write_sp_dlg.label[2] = gtk_label_new("Write unsubtracted spectrum");
  write_sp_dlg.label[3] = gtk_label_new("Write calculated spectrum");
  write_sp_dlg.label[4] = gtk_label_new("Write difference spectrum");
  write_sp_dlg.label[5] = gtk_label_new("Write residual spectrum");
  for (i=0; i<5; i++) {
    write_sp_dlg.entry[2+i] = gtk_check_button_new();
    hbox = gtk_hbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(vbox), hbox);
    gtk_box_pack_start(GTK_BOX(hbox), write_sp_dlg.entry[2+i], FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), write_sp_dlg.label[1+i], FALSE, TRUE, 0);
    if (write_sp_dlg.write[i]) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(write_sp_dlg.entry[2+i]), TRUE);
    } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(write_sp_dlg.entry[2+i]), FALSE);
    }
  }

  button_ok     = gtk_button_new_with_label("Okay");
  button_cancel = gtk_button_new_with_label("Cancel");

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(write_sp_dlg.w)->action_area),
		    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(write_sp_dlg.w)->action_area),
		    button_cancel);

  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(write_sp_dlg_cb), (gpointer) "Ok");
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(write_sp_dlg_cb), (gpointer) "Cancel");
  gtk_signal_connect(GTK_OBJECT(write_sp_dlg.entry[1]), "clicked",
		     GTK_SIGNAL_FUNC(write_sp_dlg_cb), (gpointer) "Browse");
  gtk_signal_connect(GTK_OBJECT(write_sp_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_write_sp_dlg), NULL);

  gtk_entry_set_text(GTK_ENTRY(write_sp_dlg.entry[0]), write_sp_dlg.filnam);

  /* Display the dialog */
  gtk_widget_show_all(write_sp_dlg.w);
  gtk_window_set_transient_for(GTK_WINDOW(write_sp_dlg.w), GTK_WINDOW(toplevel)); /* set parent */
  move_win_to(write_sp_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(write_sp_dlg.w), button_ok);

  write_sp_dlg.done = 0;
  while (!write_sp_dlg.done) gtk_main_iteration();

  write_sp_dlg.w = NULL;
  return 0;
} /* write_sp_2 */

/* ======================================================================= */
int busy_dlg_cb(GtkWidget *w, gpointer data)
{
  elgd.gotsignal = 1;
  return FALSE;
} /* warn_dlg_button_cb */

int busy_del_cb(GtkWidget *w, gpointer data)
{
  elgd.gotsignal = 2;
  return FALSE;
} /* warn_dlg_button_cb */

void breakhandler(int dummy)
{
  elgd.gotsignal = 1;
}

void set_signal_alert(int mode, char *mesag)
{
  static GtkWidget *button, *label, *vbox, *dialog;
  int          ix, iy;
  unsigned int state;
  char mesag2[256];

  if (mode) {
    elgd.gotsignal = 0;
    signal(SIGINT, breakhandler);

    elgd.gotsignal = 0;
    dialog = gtk_dialog_new();
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);
    set_title(dialog, "Busy");

    sprintf(mesag2, "%s\n Click Cancel to interupt...", mesag);
    label = gtk_label_new(mesag2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    button = gtk_button_new_with_label("Cancel");
    gtk_container_add(GTK_CONTAINER(vbox), label);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
		      button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(busy_dlg_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
		       GTK_SIGNAL_FUNC(busy_del_cb), NULL);

    gtk_widget_show_all(dialog);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(toplevel)); /* set parent */
    gdk_window_get_pointer(toplevel->window, &ix, &iy, &state);
    move_win_to(dialog, ix-100, iy-30);

    gtk_window_set_focus(GTK_WINDOW(dialog), button);

    gtk_widget_set_sensitive(menubar, FALSE);
    gtk_widget_set_sensitive(toolbar, FALSE);

  } else {
    signal(SIGINT, SIG_DFL);

    gtk_widget_set_sensitive(menubar, TRUE);
    gtk_widget_set_sensitive(toolbar, TRUE);
    if (elgd.gotsignal < 2) {
      gtk_widget_destroy(dialog);
      while (gtk_events_pending()) gtk_main_iteration();
    }
  }
}

int check_signal_alert(void)
{
  while (gtk_events_pending()) gtk_main_iteration();
  /* printf(" %d", elgd.gotsignal); fflush(stdout); */
  return elgd.gotsignal;
}
