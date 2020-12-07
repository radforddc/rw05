#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <math.h>
#include <ctype.h>

#include "util.h"
#include "gls.h"
#include "mingtk.h"

/* Program to create and edit a level scheme graphically */
/* GTK+ Version 1.0     D.C. Radford     May - Sept 2003 */

/* external variables defined in miniga.c */
extern Display     *disp_id;
extern Screen      *screen_id;
extern GdkWindow   *win_id;
extern GdkGC       *xmgc, *gc_id, *gc_comp_id;
extern GdkColormap *colormap;
extern GdkFont     *font_id;
extern GdkPoint    points[512];
extern GdkColor    color_id[20];
extern int         nstored, win_width, win_height, glsundo_ready;

/* function defined in gtkgls_utils.c */
extern void move_win_to(GtkWidget *w, int x, int y);

GtkTextBuffer *output_text_buffer;
GtkWidget     *output_text_view, *text_scrolled;
GtkWidget     *toplevel, *menubar, *drawing_area, *toolbar, *scrolled, *pos_text;
GtkWidget     *undo_button, *redo_button, *save_button, *zin_button, *zout_button;
GtkWidget     *undo_menu_item, *redo_menu_item, *save_menu_item, *zout_menu_item;
GtkTooltips   *tooltips;
GdkPixmap     *pixmap;
GdkWindow     *window;
int       ww = 600, hh = 500, width = 600, height = 500;
int       zoom = 1, scr_width, scr_height;
float     xin, yin, xmg[4];
char      cin[40];
int       win_origin_x, win_origin_y, win_w, win_h;
 
#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"

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
  { "Exit Program",                                "st", 0 },
  { "Display",                                     NULL, 1 },
  { "Zoom In",                                     "zi", 0 },
  { "Zoom Out",                                    "zo", 0 },
  { "", NULL, 2 },
  { "Redraw Level Scheme",                         "rd", 0 },
  { "Redraw Entire Level Scheme",                 "rd1", 0 },
  { "", NULL, 2 },
  { "Expand Level Scheme",                         "ex", 0 },
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
  { "Fit Energies of Levels",                      "fl", 0 },
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
    "Expand Level Scheme",                      "ex",
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
    "Examine Gammas",                           "xg",
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
    "", ""
};
           
/* ======================================================================= */
static int exit_gtkgls(void)
{
  if (glsundo_ready &&
      glsundo.mark != glsundo.next_pos) read_write_gls("WRITE");
  save_xwg("gls_gtk_");
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
  win_width = (int) width;
  win_height = (int) height;
  display_gls(-1);
  return 0;
} /* set_size */

/* Subroutine to copy pixmap to drawing area */
void copy_area(void)
{
  if (zoom < 8)
    gdk_draw_pixmap(drawing_area->window, xmgc, pixmap,
		    0, 0, 0, 0, width, height);
} /* copy_area */

/* Subroutine to clear pixmap and drawing area */
int erase(void)
{
  if (zoom < 8) {
    gdk_draw_rectangle(pixmap, xmgc, TRUE, 0, 0, width, height);
    copy_area();
  } else {
    gdk_draw_rectangle(window, xmgc, TRUE, 0, 0, width, height);
  }
  return 0;
} /* erase */

/* ======================================================================= */
/* Callback for the drawing area expose event */
static int DAexposeCB(GtkWidget *widget, GdkEventExpose *event, gpointer cb_data)
{
  if (zoom < 8) {
    copy_area();
  } else {
    display_gls(-1);
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

  if ((event->state & GDK_CONTROL_MASK) ||
      (event->state & GDK_MOD1_MASK) ||
      (event->state & GDK_MOD2_MASK) ||
      (event->state & GDK_MOD3_MASK) ||
      (event->state & GDK_MOD4_MASK) ||
      (event->state & GDK_MOD5_MASK)) return FALSE;

  gdk_window_get_pointer(drawing_area->window, &ixi, &iyi, &state);
  if (ixi < 0 || iyi < 0 || ixi > width || iyi > height) return FALSE;
  iyi = height - iyi;
  cvxy(&xin, &yin, &ixi, &iyi, 2);

  strcpy(response, event->string);
  if ((int) response[0] > 26 && (int) response[0] < 127) strcpy(cin, response);
  return FALSE;
} /* DAkeyCB */

/* Callback for the drawing area mouse button event */
static int DAbuttonCB(GtkWidget *widget, GdkEventButton *event, gpointer cb_data)
{
  int          ixi, iyi;
  unsigned int iflag;

  ixi = event->x;
  iyi = height - event->y;
  cvxy(&xin, &yin, &ixi, &iyi, 2);

  if ((event->state & ShiftMask) != 0) {
    /* shift-mousebutton -> pan/zoom gls display */
    iflag = 1;
    if (event->button == Button2) iflag = 2;
    if (event->button == Button3) iflag = 3;
    pan_gls(xin, yin, iflag);
  } else {
    strcpy(cin, "G ");
    if (event->button == Button2) strcpy(cin, "X2");
    if (event->button == Button3) strcpy(cin, "X3");
  }
  return FALSE;
} /* DAbuttonCB */

/* ======================================================================= */
/* Callback for the pushbuttons in the toolbar,
   and menu items in the menubar */
static int ButtonCB (GtkWidget *widget, gpointer call_data)
{
  GtkWidget     *make_cmd_button(const char *lbl, const char *cmd, const char *tip);
  GtkAdjustment *hadj, *vadj;
  static char title[40][50];
  static int  titleNum = 0;
  float       h1, h2, h3, v1, v2, v3;
  int         i, nc;
  char        pCommand[80], *ans;
  /* char        *btnLabel; */

  /* tell("ButtonCB: %d %s.\n", strlen(call_data), call_data); */
  strcpy (pCommand, call_data);
  if (titleNum == 0) strcpy(title[titleNum], "GtkGLS");

  if (strncmp(pCommand, "zi", 2) == 0) {
    /* Zoom in */

    hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled));
    vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
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
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);
    if (zoom < 8) {
      win_id = pixmap;
    } else {
      win_id = window;
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

  } else if (strncmp(pCommand, "zo", 2) == 0) {
    /* Zoom out */

    hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled));
    vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
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

    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);
    if (zoom < 8) {
      win_id = pixmap;
    } else {
      win_id = window;
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

  } else if (strncmp(pCommand, "aa", 2) == 0) {
    /* add another command widget/button */
    ans = malloc(40);
    if (!(nc = ask(ans, 39, "Command for new button = ?"))) return FALSE;
    ans[39] = 0;
    /* make sure command is in lower case */
    for (i = 0; i < 2; i++)
      ans[i] = tolower(ans[i]);

    for (i = 0; (*commandList[i]); i += 2) {
     /* if command is known, set tooltip
        if command is not known, will end up with *commandList[i] = null */
     if (!strncmp(ans, commandList[i+1], strlen(commandList[i+1]))) break;
    }
    gtk_widget_show(make_cmd_button(ans, ans, commandList[i]));

  } else if (strncmp(pCommand, "st", 2) == 0) {
    if (askyn("Are you sure you want to exit? (Y/N)")) exit_gtkgls();

  } else {
    /* get command description and use it as window title */
    if (titleNum == 39) {
      titleNum--;
      for (i=0; i<titleNum; i++) strcpy(title[i], title[i+1]);
    }
    if (titleNum > 9) {
      tell(" * Warning - you should not nest so many commands.\n"
	   " * Consider exiting from more previous modes\n"
	   " *     before entering new ones.\n");
    }
    titleNum++;
    strcpy(title[titleNum], "GtkGLS -- ");
    strcat(title[titleNum], pCommand);
    for (i = 0; (*commandList[i]); i += 2) {
      if (!strncmp(pCommand, commandList[i+1], strlen(commandList[i+1]))) {
	strcpy(title[titleNum], "GtkGLS -- ");
	strcat(title[titleNum], commandList[i+1]);
	strcat(title[titleNum], "  -- ");
 	strcat(title[titleNum], commandList[i]);
	break;
      }
    }
    gtk_window_set_title(GTK_WINDOW(toplevel), title[titleNum]);

    /* Execute command */
    /* printf ("command: %d %s\n", strlen(pCommand), pCommand); */
    if (strlen(pCommand)) {
      gls_exec(pCommand);
    }
    /* restore last window title */
    titleNum--;
    gtk_window_set_title(GTK_WINDOW(toplevel), title[titleNum]);
  }
  return FALSE;
} /* ButtonCB */

/* ======================================================================= */
static int ReportPointerPos(GtkWidget *w, GdkEventMotion *event, gpointer cb_data)
{
  float   x1, y1;
  int     ilev, igam, ixi, iyi;
  extern int nearest_gamma(float, float);
  extern int nearest_level(float, float);
  char    buf[120];
  GdkModifierType state;

  if (event->is_hint) {
    /* gdk_window_get_pointer(event->window, &ixi, &iyi, &state); */
    gdk_window_get_pointer(drawing_area->window, &ixi, &iyi, &state);
  } else {
    ixi = event->x; iyi = event->y;
  }
  iyi = height - iyi;
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
  GtkWidget *button, *vbox, *hbox, *vpaned;
  GtkWidget *menu=NULL, *menu_button, *item, *menu1=NULL;
  char      buf[80], *c, *input_file_name;
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
		     GTK_SIGNAL_FUNC(configure_event), NULL);
  gtk_window_set_title(GTK_WINDOW(toplevel), "gtkgls");
  tooltips = gtk_tooltips_new();

  /* Set size and position of main window */
  xmg[2] = 0;
  set_xwg("gls_gtk_");
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
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 1);
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
    }
  }

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  /* Create horizontal container box (toolbar) below menus */
  toolbar = gtk_hbox_new(FALSE, 1);
  gtk_container_set_border_width(GTK_CONTAINER(toolbar), 1);
  gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);

  /* Create undo and redo buttons etc */
  undo_button = make_cmd_button("UnDo",    "ue", "Undo previous level scheme edits");
  redo_button = make_cmd_button("ReDo",    "re", "Re-do previously undone level scheme edits");
  save_button = make_cmd_button("Save",    "sl", "Save level scheme");
  zin_button  = make_cmd_button(" ZIn ",   "zi", "Zoom in level scheme display");
  zout_button = make_cmd_button("ZOut",    "zo", "Zoom out level scheme display");
  button = make_cmd_button("ExamGam", "xg", "Examine gamma-ray parameters");
  gtk_widget_set_sensitive(undo_button, FALSE);
  gtk_widget_set_sensitive(redo_button, FALSE);
  gtk_widget_set_sensitive(save_button, FALSE);
  gtk_widget_set_sensitive(zout_button, FALSE);
  gtk_widget_set_sensitive(undo_menu_item, FALSE);
  gtk_widget_set_sensitive(redo_menu_item, FALSE);
  gtk_widget_set_sensitive(save_menu_item, FALSE);
  gtk_widget_set_sensitive(zout_menu_item, FALSE);

  button = gtk_button_new_with_label("Add Button");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(ButtonCB), (gpointer) "aa");
  gtk_box_pack_end(GTK_BOX(toolbar), button, FALSE, TRUE, 3);
  gtk_tooltips_set_tip(tooltips, button, "Add a new command button to this toolbar", NULL);

  /* Create a vertical 2-pane window */
  vpaned = gtk_vpaned_new();
  gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
 
  /* Create the drawing area inside a scrolled window for top pane */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(scrolled), 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_pack1(GTK_PANED(vpaned), scrolled, TRUE, FALSE);
  drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), drawing_area);

  /* Create the output text area in bottom part of 2-paned window */
  text_scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(text_scrolled), 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(text_scrolled),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_paned_pack2(GTK_PANED(vpaned), text_scrolled, TRUE, TRUE);
  output_text_view = gtk_text_view_new();
  gtk_tooltips_set_tip(tooltips, output_text_view,
		       "Displays information and messages from the program", NULL);
  gtk_tooltips_set_tip(tooltips, text_scrolled,
		       "Displays information and messages from the program", NULL);
  output_text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_text_view));
  gtk_text_view_set_editable(GTK_TEXT_VIEW(output_text_view), FALSE);
  // gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output_text_view), GTK_WRAP_WORD);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(text_scrolled), output_text_view);
  // gtk_paned_pack2(GTK_PANED(vpaned), output_text_view, TRUE, TRUE);

  /* Signals used to handle backing pixmap */
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     (GtkSignalFunc) DAexposeCB, NULL);

  /* Event signals */
  gtk_signal_connect(GTK_OBJECT(drawing_area), "motion_notify_event",
		     (GtkSignalFunc) ReportPointerPos, NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "button_press_event",
		     (GtkSignalFunc) DAbuttonCB, NULL);
  gtk_signal_connect(GTK_OBJECT(toplevel), "key_press_event",
		     (GtkSignalFunc) DAkeyCB, NULL);

  gtk_widget_set_events(drawing_area, GDK_EXPOSURE_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_POINTER_MOTION_MASK
		         | GDK_POINTER_MOTION_HINT_MASK );

  /* Create label for pointer-over information on gammas, levels etc */
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);
  /* add one constant label to avoid vertical resizing requests */
  pos_text = gtk_label_new(" ");
  gtk_label_set_justify (GTK_LABEL(pos_text), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox), pos_text, FALSE, TRUE, 3);

  gtk_widget_show_all(toplevel);
  /* move and resize top level window to saved values */
  gdk_window_resize(toplevel->window, ww, hh);
  move_win_to(toplevel, xorig, yorig);
 
  /* Create pixmap the maximum size of drawing area */
  /* printf("Creating pixmap of size 4*%d by 4*%d\n", width, height); */
  pixmap = gdk_pixmap_new(drawing_area->window, 4*width, 4*height, -1);

  /* Create graphics contexts */
  xmgc       = drawing_area->style->white_gc;
  gc_id      = gdk_gc_new(drawing_area->window);
  gc_comp_id = gdk_gc_new(drawing_area->window);
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

  window = drawing_area->window;

  /* Initialise gls subroutines */
  input_file_name = "";
  if (argc>1) {
    input_file_name = argv[1];
    tell("Input file name: %s\n", input_file_name);
  }
  win_id = pixmap;
  win_width = (int) width;
  win_height = (int) height;
  gls_init(input_file_name);

  /* set position of paned window gutter */
  while (gtk_events_pending()) gtk_main_iteration();
  gtk_paned_set_position(GTK_PANED(vpaned), hh*3/4);
  win_w = ww;
  win_h = hh;

  /* Get and dispatch events */
  gtk_main ();
  return 0;
} /* main */

/* ======================================================================= */
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

  /* process events until we get a valid input event in the drawing area */
  bell();
  cin[0] = 0;
  while (cin[0] == 0) gtk_main_iteration();

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
} /* retic2 */

/* ======================================================================= */
int initg2(int *nx, int *ny)
{

  *nx = win_width - 5;
  *ny = win_height - 25;
  return 0;
} /* initg2 */

/* ======================================================================= */
int finig(void)
{
  /* empty buffer of stored x,y values */
  if (nstored >= 2) {
    gdk_draw_lines(win_id, gc_id, points, nstored);
    nstored = 0;
  }
  XFlush(disp_id);
  copy_area();
  return 0;
} /* finig */

/* ======================================================================= */
int setcolor(int icol)
{
  int i;

  i = icol + 1;
  if (i < 1 || i > 16) return 1;
 
  /* call finig to dump stored plot array
     before changing attribute block */
  finig();
  gdk_gc_set_foreground(gc_id, &color_id[i]);

  return 0;
} /* setcolor */
