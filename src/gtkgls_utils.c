#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <math.h>
#include <ctype.h>
#include <glib.h>

#include "util.h"
#include "gls.h"
#include "mingtk.h"

/* Program to create and edit a level scheme graphically */
/* GTK+ Version 1.0     D.C. Radford     May - Sept 2003 */
/* This file conatins widgets and other useful utilities for gtkgls etc */

/* external variables defined in gtkgls.c or minig etc */
extern GtkTextBuffer *output_text_buffer;
extern GtkWidget     *output_text_view, *text_scrolled;
extern GtkWidget *menubar, *toolbar, *toplevel;
extern GtkWidget *undo_button, *redo_button, *save_button;
extern GtkWidget *undo_menu_item, *redo_menu_item, *save_menu_item;
extern GdkColor  color_id[20];
extern float     xin, yin, xmg[4];
extern int       win_origin_x, win_origin_y, win_w, win_h;
extern int       scr_width, scr_height;
extern char      cin[40];
extern int drawstring(char *, int, char, float, float, float, float);
extern int retic2(float *xout, float *yout, char *cout);
extern int initg2(int *nx, int *ny);

GdkFont   *fixed_font_id = NULL;
GtkWidget *label[9];
int       dlg_done = 1, edit_undo_count = 0;

static struct {
  GtkWidget *w;
  char      ans[16];
} yn_dlg; /* yes/no-question dialog */
static struct {
  GtkWidget *w, *entry;
  char      *ans;
} ask_dlg; /* question dialog */
static struct {
  GtkWidget *w;
} warn_dlg; /* warning dialog */
static struct {
  GtkWidget *w, *entry[11];
  int       done;
} fp_dlg; /* set figure parameters dialog */
static struct {
  GtkWidget *w, *entry[5];
  int       iband, status, done, x, y;
  float     low_e, low_j;
} eb_dlg; /* edit bands dialog */
static struct {
  GtkWidget *w, *entry[6], *label[7];
  int       igam, status, done, x, y;
} eg_dlg; /* edit gammas dialog */
static struct {
  GtkWidget *w, *entry[4];
  int       ilev, parity, status, done, x, y;
} el_dlg; /* edit levels dialog */
static struct {
  GtkWidget *w, *entry[3];
  int       itxt, status, done, x, y;
} et_dlg; /* edit text labels dialog */
static struct {
  GtkWidget *w, *entry;
  int       rtn_val;
} sel_dlg; /* question-from-a-selection dialog */
static struct {
  GtkWidget *w, *entry[13];
  char      titpos, scalepos, filnam[80], title[80];
  char      printcmd[80], htitle[20], hyaxis[20];
  int       done, font, nologo, addtit, addyaxis;
} ps_dlg; /*  postscript hardcopy dialog */
static struct {
  GtkWidget *w, *entry[4];
  int       done;
} newgls_dlg; /* dialog for creating a new level scheme */
static struct {
  GtkWidget *w;
  int       x, y, x1, y1, xnow, ynow, status;
} move_win; /* data for moving a window w->window to (x,y) */

#define FIXEDFONTNAME "-misc-fixed-medium-r-*-*-13-*-*-*-*-*-*-*"

/* ======================================================================= */

static int dlg_configure_event(GtkWidget *w, GdkEventConfigure *event)
{
  int  x2, y2;

  if  (w != move_win.w || move_win.status != 2) {
    gtk_signal_disconnect_by_func(GTK_OBJECT(w),
				  (GtkSignalFunc) dlg_configure_event, NULL);
    return FALSE;
  }
  /* find out where the window is now */
  gdk_window_get_root_origin(w->window, &x2, &y2);
  /* printf("config: %d %d %d %d %d %d %d\n",
     (int) w, move_win.x, move_win.y, move_win.x1, move_win.y1, x2, y2); */
  if (x2 != move_win.x && y2 != move_win.y) {
    if (x2 != move_win.xnow && y2 != move_win.ynow) {
      move_win.x1 += move_win.x - x2;
      move_win.y1 += move_win.y - y2;
      move_win.xnow = x2;
      move_win.ynow = y2;
      gdk_window_move(w->window, move_win.x1, move_win.y1);
    }
  } else {
    move_win.status = 0;
    gtk_signal_disconnect_by_func(GTK_OBJECT(w),
				  (GtkSignalFunc) dlg_configure_event, NULL);
  }
  return FALSE;
} /* dlg_configure_event */

static int dlg_expose_event(GtkWidget *w, GdkEventExpose *event)
{
  gtk_signal_disconnect_by_func(GTK_OBJECT(w),
				(GtkSignalFunc) dlg_expose_event, NULL);
  if  (w != move_win.w || move_win.status < 1) return FALSE;
  move_win.status = 2;
  gtk_signal_connect(GTK_OBJECT(w), "configure_event", 
		     (GtkSignalFunc) dlg_configure_event, NULL);
  gdk_window_get_root_origin(w->window, &move_win.xnow, &move_win.ynow);
  gdk_window_move(w->window, move_win.x1, move_win.y1);
  /* printf("expose: %d %d %d %d %d\n",
     (int) w, move_win.x, move_win.y, move_win.xnow, move_win.ynow); */
  gtk_signal_connect(GTK_OBJECT(w), "configure_event", 
		     (GtkSignalFunc) dlg_configure_event, NULL);
  return FALSE;
} /* dlg_expose_event */

void move_win_to(GtkWidget *w, int x, int y)
{
  /* set x = y = -1 to use default position for dialogs */
  if (x == -1 && y == -1 && w != toplevel) {
    x = win_origin_x + 100;
    y = win_origin_y + 150;
  }

  if (x < 0 || y < 0) return;
  if (x > scr_width - 100 || y > scr_height - 50) return;
  move_win.w = w;
  move_win.x = move_win.x1 = x;
  move_win.y = move_win.y1 = y;
  move_win.xnow = 0;
  move_win.ynow = 0;

  move_win.status = 1;
  gtk_signal_connect(GTK_OBJECT(w), "expose_event",
		     (GtkSignalFunc) dlg_expose_event, NULL);
  if (!(w->window)) gtk_widget_show_all(w);

}

void sensitive(void)
{
  /* set menubar and toolbar sensitive */
  gtk_widget_set_sensitive(menubar, TRUE);
  gtk_widget_set_sensitive(toolbar, TRUE);
}

void insensitive(void)
{
  /* set menubar and toolbar insensitive */
  gtk_widget_set_sensitive(menubar, FALSE);
  gtk_widget_set_sensitive(toolbar, FALSE);
}

int delete_dialog(void)
{
  dlg_done = -2;
  return FALSE;
} /* delete_dialog */

void set_title(GtkWidget *w, char *text)
{
  static int  first = 1;
  static char title[120], *c;

  if (first) {
    first = 0;
    strncpy(title, gtk_window_get_title(GTK_WINDOW(toplevel)), 40);
    if (!(c = strstr(title," "))) c = title + strlen(title);
    strcpy(c, " - ");
    c += 3;
  }
  strcpy(c, text);
  gtk_window_set_title(GTK_WINDOW(w), title);
} /* set_title */

/* ================  dialog for yes/no questions  ================ */
int delete_yn_dlg(void)
{
  /* use special character d to tell caskyn() that widget is deleted  */
  yn_dlg.ans[0] = 'd';
  return FALSE;
} /* delete_yn_dlg */

int yn_dlg_button_cb(GtkWidget *w, gpointer data)
{
  strncpy(yn_dlg.ans, data, 8);
  return FALSE;
} /* yn_dlg_button_cb */

int yn_dlg_key_cb(GtkWidget *w, GdkEventKey *event, gpointer data)
{
  char c = *event->string;

  if (c == 'y' || c == 'Y' || c == 'n' || c == 'N')
    yn_dlg.ans[0] = c;
  return FALSE;
} /*  yn_dlg_key_cb*/

int caskyn(char *mesag)
{
  /* mesag:     question to be asked (character string)
     returns 1: answer = Y/y/1   0: answer = N/n/0/<return> */

  int       nc;
  char      ans[80];
  GtkWidget *buttony, *buttonn, *labelyn, *vbox;

  /* first handle cases where we should be reading
     the response from a command file */
  while ((nc = read_cmd_file(ans, 80)) >= 0) {
    if (nc == 0 || ans[0] == 'N' || ans[0] == 'n' || ans[0] == '0') return 0;
    if (ans[0] == 'Y' || ans[0] == 'y' || ans[0] == '1') return 1;
  }

  if (strstr(mesag,"Suppress logo and file info?")) {
    if (ps_dlg.nologo) {
      log_to_cmd_file("y");
    } else {
      log_to_cmd_file("n");
    }
    return ps_dlg.nologo;
  }

  yn_dlg.ans[0] = '\0';
  yn_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(yn_dlg.w)->vbox), vbox);
  set_title(yn_dlg.w, "Y/N");

  labelyn = gtk_label_new(mesag);
  gtk_label_set_justify(GTK_LABEL(labelyn), GTK_JUSTIFY_LEFT);
  buttony = gtk_button_new_with_label("Yes");
  buttonn = gtk_button_new_with_label("No");
  gtk_container_add(GTK_CONTAINER(vbox), labelyn);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(yn_dlg.w)->action_area),
		    buttony);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(yn_dlg.w)->action_area),
		    buttonn);

  gtk_signal_connect(GTK_OBJECT(buttony), "clicked",
		     GTK_SIGNAL_FUNC(yn_dlg_button_cb), (gpointer) "y");
  gtk_signal_connect(GTK_OBJECT(buttonn), "clicked",
		     GTK_SIGNAL_FUNC(yn_dlg_button_cb), (gpointer) "n");
  gtk_signal_connect(GTK_OBJECT(yn_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_yn_dlg), NULL);
  gtk_signal_connect(GTK_OBJECT(yn_dlg.w), "key_press_event",
		     GTK_SIGNAL_FUNC(yn_dlg_key_cb), NULL);

  /* gtk_window_set_position(GTK_WINDOW(yn_dlg.w), GTK_WIN_POS_MOUSE); */
  gtk_window_set_modal(GTK_WINDOW(yn_dlg.w), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(yn_dlg.w), GTK_WINDOW(toplevel)); /* set parent */
  gtk_widget_show_all(yn_dlg.w);
  move_win_to(yn_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(yn_dlg.w), buttonn);

  /* printf("Looping...\n"); */
  insensitive();
  while (!yn_dlg.ans[0]) gtk_main_iteration();
  sensitive();
  /* if widget has been deleted, return (0) immediately */
  if (yn_dlg.ans[0] == 'd') {
    log_to_cmd_file("n");
    return 0;
  }

  gtk_widget_destroy(yn_dlg.w);
  while (gtk_events_pending()) gtk_main_iteration();

  if (yn_dlg.ans[0] == 'Y' || yn_dlg.ans[0] == 'y') {
    log_to_cmd_file("y");
    return 1;
  }
  log_to_cmd_file("n");
  return 0;
} /* caskyn */

/* ================  question dialog  ================ */
int ask_dlg_button_cb(GtkWidget *w, gpointer data)
{
  if (!strcmp((char *)data, "Ok"))
    ask_dlg.ans = (char *) gtk_entry_get_text(GTK_ENTRY(ask_dlg.entry));
  dlg_done = 1;
  return FALSE;
} /* ask_dlg_button_cb */

int ask_dlg_key_cb(GtkWidget *w, GdkEventKey *event, gpointer data)
{
  /* printf("%s %d\n", event->string, (int) *(event->string)); */
  if (strchr(event->string, '\r')) {
    ask_dlg.ans = (char *) gtk_entry_get_text(GTK_ENTRY(ask_dlg.entry));
    dlg_done = 1;
  }
  return FALSE;
} /* ask_dlg_key_cb */

int ask(char *ans, int mca, const char *fmt, ...)
{
  /* fmt, ...:  question to be asked
     ans:       answer recieved, maximum of mca chars will be modified
     mca:       max. number of characters asked for in answer
     returns number of characters received in answer */

  GtkWidget *button_ok, *button_cancel, *label, *vbox;
  va_list ap;
  char    q[4096], j[4096], *c, *c2, default_str[256] = "";
  int     nca;

  /* first handle cases where we should be reading
     the response from a command file */
  if ((nca = read_cmd_file(ans, mca)) >= 0) return nca;

  va_start(ap, fmt);
  vsnprintf(q, 4095, fmt, ap);
  va_end(ap);
  if ((c = strstr(q, "return to ")) ||
      (c = strstr(q, "Return to "))) {
    strncpy(c, "Cancel", 6);
  } else if ((c = strstr(q, "rtn to "))) {
    strcpy(j, c+3);
    strncpy(c, "Cancel", 6);
    strcpy(c+6, j);
  } else if ((c = strstr(q, "(return for ")) ||
	     (c = strstr(q, "(Return for ")) ||
	     (c = strstr(q, "(rtn for "))) {
    c2 = strstr(c, "for") + 4;
    strcpy(default_str, c2);
    if ((c2 = strstr(default_str, ")"))) *c2 = (char) 0;
    if ((c2 = strstr(c, ")"))) memmove(c, c2+1, strlen(c2));
  }

  dlg_done = 0;
  ask_dlg.ans = "";
  ask_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ask_dlg.w)->vbox), vbox);
  set_title(ask_dlg.w, "Question");

  label = gtk_label_new(q);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  ask_dlg.entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(ask_dlg.entry), mca);
  if (*default_str) gtk_entry_set_text(GTK_ENTRY(ask_dlg.entry), default_str);
  button_ok = gtk_button_new_with_label("Okay");
  button_cancel = gtk_button_new_with_label("Cancel");
  gtk_container_add(GTK_CONTAINER(vbox), label);
  gtk_container_add(GTK_CONTAINER(vbox), ask_dlg.entry);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ask_dlg.w)->action_area),
		    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ask_dlg.w)->action_area),
		    button_cancel);

  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(ask_dlg_button_cb), "Ok");
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(ask_dlg_button_cb), "Cancel");
  gtk_signal_connect(GTK_OBJECT(ask_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_dialog), NULL);
  gtk_signal_connect(GTK_OBJECT(ask_dlg.w), "key_press_event",
		     GTK_SIGNAL_FUNC(ask_dlg_key_cb), NULL);

  gtk_widget_show_all(ask_dlg.w);
  gtk_window_set_modal(GTK_WINDOW(ask_dlg.w), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(ask_dlg.w), GTK_WINDOW(toplevel)); /* set parent */
  move_win_to(ask_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(ask_dlg.w), ask_dlg.entry);

  insensitive();
  while (!dlg_done) gtk_main_iteration();
  sensitive();

  strncpy(ans, ask_dlg.ans, mca);
  nca = strlen(ans);
  if (nca > mca) nca = mca;
  /* remove trailing blanks */
  while (nca > 0 && ans[nca-1] == ' ') ans[--nca] = '\0';

  if (dlg_done > -2) {
    gtk_widget_destroy(ask_dlg.w);
    while (gtk_events_pending()) gtk_main_iteration();
  }

  /* printf("ans, nca = %s %d\n", ans, nca); */
  dlg_done = 0;
  log_to_cmd_file(ans);
  return nca;
} /* ask */

/* ================  warning dialog  ================ */
int warn_dlg_button_cb(GtkWidget *w, gpointer data)
{
  dlg_done = 1;
  return FALSE;
} /* warn_dlg_button_cb */

void warn1(const char *fmt, ...)
{
  /* fmt, ...:  warning message */
  /* same as tell() for standard command-line programs
     but redefined here as a popup for GUI versions */

  va_list   ap;
  char      mesag[8192];

  va_start(ap, fmt);
  vsnprintf(mesag, 8191, fmt, ap);
  va_end(ap);

  warn(mesag);
}

void warn(const char *fmt, ...)
{
  /* fmt, ...:  warning message */

  va_list   ap;
  GtkWidget *button, *label, *vbox;
  char      mesag[8192];
  int       ncm, ix, iy;
  unsigned int state;

  va_start(ap, fmt);
  ncm = vsnprintf(mesag, 8191, fmt, ap);
  va_end(ap);
  while (mesag[ncm-1] == '\n') mesag[--ncm] = '\0';

  dlg_done = 0;
  warn_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(warn_dlg.w)->vbox), vbox);
  set_title(warn_dlg.w, "Note");

  label = gtk_label_new(mesag);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  button = gtk_button_new_with_label("Okay");
  gtk_container_add(GTK_CONTAINER(vbox), label);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(warn_dlg.w)->action_area),
		    button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(warn_dlg_button_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(warn_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_dialog), NULL);

  gtk_widget_show_all(warn_dlg.w);
  gtk_window_set_modal(GTK_WINDOW(warn_dlg.w), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(warn_dlg.w), GTK_WINDOW(toplevel)); //set parent
  /* move_win_to(warn_dlg.w, -1, -1); */
  gdk_window_get_pointer(toplevel->window, &ix, &iy, &state);
  move_win_to(warn_dlg.w, ix-100, iy-30);

  gtk_window_set_focus(GTK_WINDOW(warn_dlg.w), button);

  insensitive();
  while (!dlg_done) gtk_main_iteration();
  sensitive();
  if (dlg_done > -2) {
    gtk_widget_destroy(warn_dlg.w);
    while (gtk_events_pending()) gtk_main_iteration();
  }
  dlg_done = 0;
} /* warn */

/* ================  dialog for file selections  ================ */

int askfn(char *ans, int mca,
	  const char *default_fn, const char *default_ext,
	  const char *fmt, ...)
{
  /* fmt, ...:  question to be asked
     ans:       answer recieved, maximum of mca chars will be modified
     mca:       max. number of characters asked for in answer
     returns number of characters received in answer */
  /* this variant of cask asks for filenames;
     default_fn  = default filename (or empty string if no default)
     default_ext = default filename extension (or empty string if no default) */

  GtkFileChooserAction action;
  GtkWidget *labelfs, *dlg;
  va_list   ap;
  char      q[4096], j[4096], *c;
  int       nca, ncq;

  /* first handle cases where we should be reading
     the response from a command file */
  if ((nca = read_cmd_file(ans, mca)) >= 0) {
    if (strlen(default_fn) && (nca == 0 || ans[nca-1] == '/')) {
      strncpy(ans+nca, default_fn, mca-nca);
      nca = strlen(ans);
    }
    setext(ans, default_ext, mca);
    return nca;
  }
  nca = 0;

  va_start(ap, fmt);
  vsnprintf(q, 4095, fmt, ap);
  va_end(ap);
  if ((c = strstr(q,"\n   (Type q ")) ||
      (c = strstr(q,"\n   (rtn to abort)"))) {
    *c = '\0';
  }
  if ((c = strstr(q, "return to ")) ||
      (c = strstr(q, "Return to "))) {
    strcpy(j, c+6);
    strcpy(c, "click Cancel");
    strcpy(c+12, j);
  } else if ((c = strstr(q, "rtn to "))) {
    strcpy(j, c+3);
    strcpy(c, "click Cancel");
    strcpy(c+12, j);
  }
  ncq = strlen(q);

  if (strlen(default_ext)) {
    snprintf(q+ncq, 4095-ncq, "\n   (default .ext = %s)", default_ext);
  }

  if (strstr(q,"new") || strstr(q,"New") || strstr(q,"sav") ||
      strstr(q,"logging") || strstr(q,"utput")) {
    action = GTK_FILE_CHOOSER_ACTION_SAVE;
  } else {
    action = GTK_FILE_CHOOSER_ACTION_OPEN;
  }
  dlg = gtk_file_chooser_dialog_new("gtkgls - FileSelection",
				    GTK_WINDOW(toplevel), action,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  if (strlen(default_fn))
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), default_fn);

  labelfs = gtk_label_new(q);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),
		    labelfs);
  gtk_widget_show(labelfs);

  /* run the dialog */
  insensitive();
  if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK) {
    c = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    strncpy(ans, c, mca);
    setext(ans, default_ext, mca);
    g_free(c);
  } else {
    strncpy(ans, "", mca);
  }
  sensitive();
  gtk_widget_destroy(dlg);

  nca = strlen(ans);

  /* printf("ans, nca = %s %d\n", ans, nca); */
  /* if (strlen(default_fn) && nca == 0) {
     strncpy(ans, default_fn, mca);
     nca = strlen(default_fn);
     } */
  if (strlen(default_fn) && ans[nca-1] == '/') {
    strncpy(ans+nca, default_fn, mca-nca);
    nca = strlen(default_fn);
  }
  log_to_cmd_file(ans);

  return nca;
} /* askfn */

/* ===========  dialog to set level scheme figure parameters  ============ */
int delete_fp_dlg(void)
{
  fp_dlg.done = 1;
  gtk_widget_destroy(fp_dlg.w);
  fp_dlg.w = NULL;
  return FALSE;
} /* delete_fp_dlg */

void reset_fig_pars_current(void)
{
  float angle;
  char  in[80];

  angle = atan(glsgd.max_tangent) * 57.29578f;
  sprintf(in, "%.0f", glsgd.csx);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[0]), in);
  sprintf(in, "%.0f", glsgd.csy);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[1]), in);
  sprintf(in, "%.0f", angle);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[2]), in);
  sprintf(in, "%.0f", glsgd.max_dist);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[3]), in);
  sprintf(in, "%.0f", glsgd.default_width);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[4]), in);
  sprintf(in, "%.0f", glsgd.default_sep);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[5]), in);
  sprintf(in, "%.2f", glsgd.aspect_ratio);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[6]), in);
  sprintf(in, "%.0f", glsgd.arrow_width);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[7]), in);
  sprintf(in, "%.0f", glsgd.arrow_length);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[8]), in);
  sprintf(in, "%.0f", glsgd.arrow_break);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[9]), in);
  sprintf(in, "%.0f", glsgd.level_break);
  gtk_entry_set_text(GTK_ENTRY(fp_dlg.entry[10]), in);
} /* reset_fig_pars_current */

void apply_new_fig_pars(void)
{
  float angle, f1, in[11];
  int   i, err = 0;
  char *c;

  save_gls_now(8);
  edit_undo_count++;

  for (i=0; i<11; i++) {
    c = (char *) gtk_entry_get_text(GTK_ENTRY(fp_dlg.entry[i]));
    in[i] = 0.0f;
    sscanf(c, "%f", &in[i]);
    /* printf("%d %d %s %f\n", i, strlen(c), c, in[i]); */
  }
    
  if ((f1 = in[0]) > 0.0f) {
    if (glsgd.gel_csx == glsgd.csx) glsgd.gel_csx = f1;
    if (glsgd.lsl_csx == glsgd.csx) glsgd.lsl_csx = f1;
    if (glsgd.lel_csx == glsgd.csx) glsgd.lel_csx = f1;
    glsgd.csx = f1;
    glsgd.gel_csy = glsgd.gel_csx*CSY;
    glsgd.lsl_csy = glsgd.lsl_csx*CSY;
    glsgd.lel_csy = glsgd.lel_csx*CSY;
    for (i=0; i<glsgd.ntlabels; i++) glsgd.txt[i].csy = glsgd.txt[i].csx*CSY;
  }
  if ((f1 = in[1]) > 0.0f) {
    glsgd.csy = f1;
    glsgd.gel_csy = glsgd.gel_csx*CSY;
    glsgd.lsl_csy = glsgd.lsl_csx*CSY;
    glsgd.lel_csy = glsgd.lel_csx*CSY;
    for (i=0; i<glsgd.ntlabels; i++) glsgd.txt[i].csy = glsgd.txt[i].csx*CSY;
  }
  angle = in[2];
  glsgd.max_tangent = tan(angle/57.29578f);
  if ((f1 = in[3]) < 0.0f) err = 1;
  else glsgd.max_dist = f1;
  if ((f1 = in[4]) <= 0.0f) err = 1;
  else glsgd.default_width = f1;
  if ((f1 = in[5]) <= 0.0f) err = 1;
  else glsgd.default_sep = f1;
  glsgd.aspect_ratio = in[6];
  if ((f1 = in[7]) <= 0.0f) err = 1;
  else glsgd.arrow_width = f1;
  if ((f1 = in[8]) <= 0.0f) err = 1;
  else glsgd.arrow_length = f1;
  if ((f1 = in[9]) <= 0.0f) err = 1;
  else glsgd.arrow_break = f1;
  if ((f1 = in[10]) <= 0.0f) err = 1;
  else glsgd.level_break = f1;

  if (err) {
    warn("ERROR -- some entries have illegal values\n"
	 "   and have not been modified!\n");
    reset_fig_pars_current();
  }
} /* apply_new_fig_pars */

void fp_dlg_cb(GtkWidget *w, gpointer data)
{
  if        (!strcmp((char *)data, "Ok")) {
    apply_new_fig_pars();
    fp_dlg.done = 1;
    gtk_widget_destroy(fp_dlg.w);
    fp_dlg.w = NULL;
  } else if (!strcmp((char *)data, "Apply")) {
    apply_new_fig_pars();
    calc_band();
    calc_gls_fig();
    display_gls(-1);
  } else if (!strcmp((char *)data, "Reset")) {
    undo_gls(-edit_undo_count);
    edit_undo_count = 0;
    reset_fig_pars_current();
  } else if (!strcmp((char *)data, "Cancel")) {
    fp_dlg.done = 1;
    gtk_widget_destroy(fp_dlg.w);
    fp_dlg.w = NULL;
  }
} /* fp_dlg_cb */

int figure_pars(void)
{
  GtkWidget *button_ok, *button_apply, *button_reset, *button_cancel;
  GtkWidget *label[7], *table, *vbox;
  static int first = 1;
  int   i;
  int   col[11] = {2, 3, 2, 2, 2, 3, 2, 2, 3, 2, 3};
  int   row[11] = {1, 1, 2, 3, 4, 4, 5, 6, 6, 7, 7};

  if (fp_dlg.w) {
    /* Since this dialog is not modal, the user could try to create
       two instances of it.  This prevents that from happening. */
    gdk_window_raise(fp_dlg.w->window);
    return 1;
  }
  if (first) first = 0;
  fp_dlg.done = 0;

  /* all figure distance units are in keV */
  /* gamma arrow widths = intensity*aspect_ratio (keV) */

  fp_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fp_dlg.w)->vbox), vbox);
  set_title(fp_dlg.w, "Level Scheme Parameters");
  for (i=0; i<11; i++) {
    fp_dlg.entry[i] = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(fp_dlg.entry[i]), 15);
    gtk_widget_set_usize(fp_dlg.entry[i], 90, 0);
  }
  reset_fig_pars_current();

  label[0] = gtk_label_new("   Default label character sizes (x,y):");
  label[1] = gtk_label_new("       Max. angle for gammas (degrees):");
  label[2] = gtk_label_new("     Max. sideways distance for gammas:");
  label[3] = gtk_label_new("     Default band width and separation:");
  label[4] = gtk_label_new("Gamma arrow width \"keV\"/intensity unit:");
  label[5] = gtk_label_new("      Gamma arrowhead width and length:");
  label[6] = gtk_label_new("Tentative gamma and level break length:");

  button_ok     = gtk_button_new_with_label("Okay");
  button_apply  = gtk_button_new_with_label("Apply");
  button_reset  = gtk_button_new_with_label("Reset");
  button_cancel = gtk_button_new_with_label("Cancel");

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fp_dlg.w)->action_area),
		    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fp_dlg.w)->action_area),
		    button_apply);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fp_dlg.w)->action_area),
		    button_reset);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fp_dlg.w)->action_area),
		    button_cancel);

  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(fp_dlg_cb), (gpointer) "Ok");
  gtk_signal_connect(GTK_OBJECT(button_apply), "clicked",
		     GTK_SIGNAL_FUNC(fp_dlg_cb), (gpointer) "Apply");
  gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		     GTK_SIGNAL_FUNC(fp_dlg_cb), (gpointer) "Reset");
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(fp_dlg_cb), (gpointer) "Cancel");
  gtk_signal_connect(GTK_OBJECT(fp_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_fp_dlg), NULL);

  table = gtk_table_new(7, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(vbox), table);
  for (i=0; i<7; i++) {
    gtk_table_attach(GTK_TABLE(table), label[i], 0, 1, i, i+1,
		     (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
  }
  for (i=0; i<11; i++) {
    gtk_table_attach(GTK_TABLE(table), fp_dlg.entry[i],
		     col[i]-1, col[i], row[i]-1, row[i],
		     GTK_FILL, GTK_FILL, 0, 0);
  }

  /* Display the dialog */
  gtk_widget_show_all(fp_dlg.w);
  gtk_window_set_transient_for(GTK_WINDOW(fp_dlg.w), GTK_WINDOW(toplevel)); /* set parent */

  move_win_to(fp_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(fp_dlg.w), fp_dlg.entry[0]);

  edit_undo_count = 0;
  while (!fp_dlg.done) gtk_main_iteration();
  /* the dialog actually get destroyed in fp_dlg_cb() or
     delete_fp_dlg().  This is just extra insurance. */
  if (fp_dlg.w) gtk_widget_destroy(fp_dlg.w);
  while (gtk_events_pending()) gtk_main_iteration();
  fp_dlg.w = NULL;

  return 0;
} /* figure_pars */

/* ===========  dialog to set band parameters  ============ */
int delete_eb_dlg(void)
{
  gdk_window_get_root_origin(eb_dlg.w->window, &eb_dlg.x, &eb_dlg.y);
  gtk_widget_hide(eb_dlg.w);
  eb_dlg.status = -2;
  eb_dlg.done = -1;
  return FALSE;
} /* delete_eb_dlg */

void reset_band_pars_current(void)
{
  char  in[80];
  int   i;

  eb_dlg.low_e = eb_dlg.low_j = -1.0f;
  for (i = 0; i < glsgd.nlevels; ++i) {
    if (glsgd.lev[i].band == eb_dlg.iband) {
      if (eb_dlg.low_e < 0.0f || eb_dlg.low_e > glsgd.lev[i].e)
	eb_dlg.low_e = glsgd.lev[i].e;
      if (eb_dlg.low_j < 0.0f || eb_dlg.low_j > glsgd.lev[i].j)
	eb_dlg.low_j = glsgd.lev[i].j;
    }
  }
  sprintf(in, "%s", glsgd.bnd[eb_dlg.iband].name);
  gtk_entry_set_text(GTK_ENTRY(eb_dlg.entry[0]), in);
  sprintf(in, "%.0f", glsgd.bnd[eb_dlg.iband].nx);
  gtk_entry_set_text(GTK_ENTRY(eb_dlg.entry[1]), in);
  sprintf(in, "%.0f", eb_dlg.low_e);
  gtk_entry_set_text(GTK_ENTRY(eb_dlg.entry[2]), in);
  sprintf(in, "%.1f", eb_dlg.low_j);
  gtk_entry_set_text(GTK_ENTRY(eb_dlg.entry[3]), in);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(eb_dlg.entry[4]), FALSE);
} /* reset_band_pars_current */

void apply_new_band_pars(void)
{
  float in, de = 0.0f, ds = 0.0f, f, dp;
  int   i, iis, iis2, iip = 1, bi, bf, ids, ilev, nc, err = 0;
  char  name[8], ans[40], ch;

  save_gls_now(7);
  edit_undo_count++;

  /* extract name of band */
  strncpy(ans, gtk_entry_get_text(GTK_ENTRY(eb_dlg.entry[0])), 8);
  if (strncmp(glsgd.bnd[eb_dlg.iband].name, ans, 8)) {
    ans[8] = '\0';
    nc = strlen(ans);
    /* remove trailing blanks */
    while (nc > 0 && ans[nc-1] == ' ') ans[--nc] = '\0';
    strncpy(name, "         ", 8);
    strncpy(name + 8 - nc, ans, nc);
    err = 0;
    for (i = 0; i < glsgd.nbands; ++i) {
      if (i != eb_dlg.iband && !strncmp(glsgd.bnd[i].name, name, 8)) {
	warn("ERROR - band name %8s already in use, not changed!\n", name);
	err = 1;
	break;
      }
    }
    if (!err) strncpy(glsgd.bnd[eb_dlg.iband].name, name, 8);
  }

  /* extract width of band */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eb_dlg.entry[1])), "%f", &in) == 1 &&
      in > 0.0f && fabs(glsgd.bnd[eb_dlg.iband].nx - in) > 0.1f) {
    glsgd.bnd[eb_dlg.iband].nx = in;
    for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
      if (glsgd.lev[ilev].band == eb_dlg.iband &&
	  glsgd.lev[ilev].dxl - glsgd.lev[ilev].dxr >=
	      glsgd.bnd[eb_dlg.iband].nx * .8f) {
	glsgd.lev[ilev].dxl = 0.f;
	glsgd.lev[ilev].dxr = 0.f;
      }
    }
  }

  /* extract lowest energy of band */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eb_dlg.entry[2])), "%f", &in) == 1 &&
      in >= 0.0f && fabs(in - eb_dlg.low_e) > 0.01f) {
    err = 0;
    de = in - eb_dlg.low_e;
    for (i = 0; (!err && i < glsgd.ngammas); ++i) {
      if (glsgd.lev[glsgd.gam[i].li].band == eb_dlg.iband &&
	  glsgd.lev[glsgd.gam[i].lf].band != eb_dlg.iband) {
	f = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e + de;
      } else if (glsgd.lev[glsgd.gam[i].li].band != eb_dlg.iband && 
		 glsgd.lev[glsgd.gam[i].lf].band == eb_dlg.iband) {
	f = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e - de;
      } else {
	continue;
      }
      if (f < 0.0f) {
	warn("ERROR -- cannot have gammas with energy < 0...\n"
	     "   Band energy not changed!\n");
	err = 1;
      }
    }
    for (i = 0; (!err && i < glsgd.nlevels); ++i) {
      if (glsgd.lev[i].band == eb_dlg.iband &&
	  (glsgd.lev[i].e + de) < -0.0001f) {
	warn("ERROR -- cannot have levels with energy < 0...\n"
	     "   Band energy not changed!\n");
	err = 1;
      }
    }
    if (err) {
      de = 0.0f;
    } else {
      for (i = 0; i < glsgd.nlevels; ++i) {
	if (glsgd.lev[i].band == eb_dlg.iband) glsgd.lev[i].e += de;
      }
      for (i = 0; i < glsgd.ngammas; ++i) {
	if ((glsgd.lev[glsgd.gam[i].li].band == eb_dlg.iband ||
	     glsgd.lev[glsgd.gam[i].lf].band == eb_dlg.iband) &&
	    glsgd.lev[glsgd.gam[i].li].band != glsgd.lev[glsgd.gam[i].lf].band) {
	  f = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e;
	  tell("Gamma %.1f now has energy %.1f\n", glsgd.gam[i].e, f);
	  glsgd.gam[i].e = f;
	}
      }
    }
  }

  /* extract lowest spin of band */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eb_dlg.entry[3])), "%f", &in) == 1 &&
      in >= 0.0f && fabs(in - eb_dlg.low_j) > 0.1f) {
    err = 0;
    ds = in - eb_dlg.low_j;
    iis = (int) rint(in);
    iis2 = (int) rint(in*2.0f);
    if (in != (float) iis2 / 2.0f) {
      warn("ERROR -- spin must be integer or half-integer.\n");
      err = 1;
    }
    iis = (int) rint(ds);
    for (i = 0; (!err && i < glsgd.ngammas); ++i) {
      bi = glsgd.lev[glsgd.gam[i].li].band;
      bf = glsgd.lev[glsgd.gam[i].lf].band;
      if ((bi == eb_dlg.iband || bf == eb_dlg.iband) && bi != bf &&
	  ds != (float) iis) {
	warn("ERROR -- spin change must be integer\n"
	     "    since there are interband transitions.\n");
	err = 1;
      }
    }
    if (err) ds = 0.0f;
  }

  /* toggle parities of band? */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(eb_dlg.entry[4])))
    iip = -1;

  if (ds != 0.0f || iip == -1) {
    for (i = 0; i < glsgd.nlevels; ++i) {
      if (glsgd.lev[i].band == eb_dlg.iband) {
	glsgd.lev[i].j += ds;
	glsgd.lev[i].pi *= (float) iip;
      }
    }
    for (i = 0; i < glsgd.ngammas; ++i) {
      bi = glsgd.lev[glsgd.gam[i].li].band;
      bf = glsgd.lev[glsgd.gam[i].lf].band;
      if ((bi == eb_dlg.iband || bf == eb_dlg.iband) && bi != bf) {
	ids = abs((int) (glsgd.lev[glsgd.gam[i].li].j -
			 glsgd.lev[glsgd.gam[i].lf].j + 0.5f));
	if (ids > 9) ids = 9;
	if (ids == 0) ids = 1;
	dp = glsgd.lev[glsgd.gam[i].li].pi * glsgd.lev[glsgd.gam[i].lf].pi;
	if (dp > 0.0f) {
	  if (ids % 2 == 0) ch = 'E';
	  else ch = 'M';
	} else {
	  if (ids % 2 == 0) ch = 'M';
	  else ch = 'E';
	}
	glsgd.gam[i].em = ch;
	glsgd.gam[i].n = (float) ids;
	tell("Gamma %.1f now has multipolarity %c%i\n",
	     glsgd.gam[i].e, ch, ids);
      }
    }
  }

  if (ds != 0.f || iip != 1 || fabs(de) > 1.0f) {
    tell("Some conversion coefficients may be recalculated...\n");

    for (i = 0; i < glsgd.ngammas; ++i) {
      if ((glsgd.lev[glsgd.gam[i].li].band == eb_dlg.iband || 
	   glsgd.lev[glsgd.gam[i].lf].band == eb_dlg.iband) && 
	  glsgd.lev[glsgd.gam[i].li].band !=
	  glsgd.lev[glsgd.gam[i].lf].band) {
	calc_alpha(i);
      }
    }
  }

  calc_band();
  calc_gls_fig();
  path(1);
  calc_peaks();
  display_gls(-1);
  reset_band_pars_current();
} /* apply_new_band_pars */

void eb_dlg_cb(GtkWidget *w, gpointer data)
{
  if        (!strcmp((char *)data, "Ok")) {
    apply_new_band_pars();
    /* set eb_dlg.done to -1 to hide dialog and exit */
    eb_dlg.done = -1;
  } else if (!strcmp((char *)data, "Apply")) {
    apply_new_band_pars();
  } else if (!strcmp((char *)data, "Reset")) {
    undo_gls(-edit_undo_count);
    edit_undo_count = 0;
    reset_band_pars_current();
  } else if (!strcmp((char *)data, "Cancel")) {
    /* hide dialog and exit */
    eb_dlg.status = -1;
    gdk_window_get_root_origin(eb_dlg.w->window, &eb_dlg.x, &eb_dlg.y);
    gtk_widget_hide(eb_dlg.w);
    eb_dlg.done = -1;
  }
} /* eb_dlg_cb */

int edit_band(void)
{
  static GtkWidget *button_ok, *button_apply, *button_reset;
  static GtkWidget *label[6], *table, *button_cancel, *vbox;
  static int first = 1;
  int    i;
  char   m[80];

  /* eb_dlg.status:
     = 0 when dialog not in use, and at exit of this routine
     = 1 when prompting for band to edit
     = 2 after band selected, when asking for modified band parameters
     = -1 when dialog has just been hidden by delete or cancel buttons
          (in eb_dlg_cb() or delete_eb_dlg())
     = -2 if dialog has been deleted
   */

  if (first) {
    first = 0;
    eb_dlg.x = eb_dlg.y = -1;
    eb_dlg.status = -2;
    eb_dlg.done = 1;
  }

  if (eb_dlg.status < -1) {
    /* dialog does not exist; create it */
    eb_dlg.w = gtk_dialog_new();
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    set_title(eb_dlg.w, "Edit Band");
    label[5] = gtk_label_new("Select band to edit (X or button 3 to exit)...");
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eb_dlg.w)->vbox), label[5]);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eb_dlg.w)->vbox), vbox);

    for (i=0; i<4; i++) {
      eb_dlg.entry[i] = gtk_entry_new();
      gtk_entry_set_max_length(GTK_ENTRY(eb_dlg.entry[i]), 8);
      gtk_widget_set_usize(eb_dlg.entry[i], 90, 0);
    }
    eb_dlg.entry[4] = gtk_check_button_new();

    label[0] = gtk_label_new("Band Name (1 - 8 chars):");
    label[1] = gtk_label_new("             Band Width:");
    label[2] = gtk_label_new("          Lowest energy:");
    label[3] = gtk_label_new("            Lowest spin:");
    label[4] = gtk_label_new("   Invert band parities?");

    button_ok     = gtk_button_new_with_label("Okay");
    button_apply  = gtk_button_new_with_label("Apply");
    button_reset  = gtk_button_new_with_label("Reset");
    button_cancel = gtk_button_new_with_label("Cancel");

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eb_dlg.w)->action_area),
		      button_ok);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eb_dlg.w)->action_area),
		      button_apply);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eb_dlg.w)->action_area),
		      button_reset);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eb_dlg.w)->action_area),
		      button_cancel);

    gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		       GTK_SIGNAL_FUNC(eb_dlg_cb), (gpointer) "Ok");
    gtk_signal_connect(GTK_OBJECT(button_apply), "clicked",
		       GTK_SIGNAL_FUNC(eb_dlg_cb), (gpointer) "Apply");
    gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		       GTK_SIGNAL_FUNC(eb_dlg_cb), (gpointer) "Reset");
    gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		       GTK_SIGNAL_FUNC(eb_dlg_cb), (gpointer) "Cancel");
    gtk_signal_connect(GTK_OBJECT(eb_dlg.w), "delete_event",
		       GTK_SIGNAL_FUNC(delete_eb_dlg), NULL);

    table = gtk_table_new(5, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(vbox), table);
    for (i=0; i<5; i++) {
      gtk_table_attach(GTK_TABLE(table), label[i], 0, 1, i, i+1,
		       (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
      gtk_table_attach(GTK_TABLE(table), eb_dlg.entry[i], 1, 2, i, i+1,
		       GTK_FILL, GTK_FILL, 0, 0);
    }
  }

  while (1) {
    /* Display the dialog if it's not already shown */
    if (eb_dlg.status < 1) {
      gtk_widget_show_all(eb_dlg.w);
      move_win_to(eb_dlg.w, eb_dlg.x, eb_dlg.y);
    }

    /* tell("Select band to edit (X or button 3 to exit)...\n"); */
    gtk_label_set(GTK_LABEL(label[5]),
		  "Select band to edit (X or button 3 to exit)...");
    gtk_widget_set_sensitive(vbox, FALSE);
    gtk_widget_set_sensitive(button_ok, FALSE);
    gtk_widget_set_sensitive(button_apply, FALSE);
    gtk_widget_set_sensitive(button_reset, FALSE);
    gdk_window_raise(eb_dlg.w->window);
    eb_dlg.status = 1;

    /* retic2(&x1, &y1, ans);
       if (*ans == 'X' || *ans == 'x') break; */
    bell();
    cin[0] = 0;
    while (cin[0] == 0 && eb_dlg.status > 0) gtk_main_iteration();
    /* the next line (eb_dlg.status < 0) happens if dialog was hidden
       by killing or canceling it */
    if (eb_dlg.status < 0 || *cin == 'X' || *cin == 'x') break;
    /* the next line (eb_dlg.status == 0) happens if dialog was hidden
       by a later instance of edit_band */
    if (eb_dlg.status == 0) continue;

    eb_dlg.iband = nearest_band(xin, yin);
    if (eb_dlg.iband < 0 || eb_dlg.iband > glsgd.nbands) {
      tell("  ... no band selected, try again...\n");
      continue;
    }
    /* tell(" Band %.8s selected...\n", glsgd.bnd[eb_dlg.iband].name); */
    sprintf(m, " Band %.8s selected...", glsgd.bnd[eb_dlg.iband].name);
    gtk_label_set(GTK_LABEL(label[5]), m);

    gtk_widget_set_sensitive(vbox, TRUE);
    gtk_widget_set_sensitive(button_ok, TRUE);
    gtk_widget_set_sensitive(button_apply, TRUE);
    gtk_widget_set_sensitive(button_reset, TRUE);
    gtk_window_set_focus(GTK_WINDOW(eb_dlg.w), eb_dlg.entry[0]);
    reset_band_pars_current();
    eb_dlg.done = 0;
    edit_undo_count = 0;
    eb_dlg.status = 2;
    insensitive();

    while (1) {
      gdk_window_raise(eb_dlg.w->window);
      cin[0] = 0;
      while (cin[0] == 0 &&
	     !eb_dlg.done && eb_dlg.status > 0) gtk_main_iteration();
      if (cin[0] == 0) break;
      i = nearest_band(xin, yin);
      if (i < 0 || i > glsgd.nbands) {
	tell("  ... no band selected, try again...\n");
      } else {
	eb_dlg.iband = i;
	sprintf(m, " Band %.8s selected...", glsgd.bnd[eb_dlg.iband].name);
	gtk_label_set(GTK_LABEL(label[5]), m);
	label[3] = gtk_label_new("   Conversion coefficient:");
	reset_band_pars_current();
      }
    }
    sensitive();
    if (!eb_dlg.status || eb_dlg.done < 0) break;
  }
  cin[0] = 0;
 
  /* Hide the dialog and return */
  if (eb_dlg.status > 0) {
    gdk_window_get_root_origin(eb_dlg.w->window, &eb_dlg.x, &eb_dlg.y);
    gtk_widget_hide(eb_dlg.w);
  }
  if (eb_dlg.status > -2) eb_dlg.status = 0;
  eb_dlg.done = 0;
  return 0;
} /* edit_band */

/* ===========  dialog to set gamma parameters  ============ */
int delete_eg_dlg(void)
{
  if (eg_dlg.done > 9)  cin[0] = 'A';
  gdk_window_get_root_origin(eg_dlg.w->window, &eg_dlg.x, &eg_dlg.y);
  eg_dlg.status = -2;
  eg_dlg.done = -1;
  return FALSE;
} /* delete_eg_dlg */

void reset_gamma_pars_current(void)
{
  char  in[80];

  sprintf(in, "%.3f", glsgd.gam[eg_dlg.igam].e);
  gtk_entry_set_text(GTK_ENTRY(eg_dlg.entry[0]), in);
  sprintf(in, "%.3f", glsgd.gam[eg_dlg.igam].de);
  gtk_entry_set_text(GTK_ENTRY(eg_dlg.entry[1]), in);
  sprintf(in, "%.3f", glsgd.gam[eg_dlg.igam].i);
  gtk_entry_set_text(GTK_ENTRY(eg_dlg.entry[2]), in);
  sprintf(in, "%.3f", glsgd.gam[eg_dlg.igam].di);
  gtk_entry_set_text(GTK_ENTRY(eg_dlg.entry[3]), in);
  sprintf(in, "%.5g", glsgd.gam[eg_dlg.igam].d);
  gtk_entry_set_text(GTK_ENTRY(eg_dlg.entry[4]), in);
  sprintf(in, "%.5g", glsgd.gam[eg_dlg.igam].a);
  gtk_entry_set_text(GTK_ENTRY(eg_dlg.entry[5]), in);
  sprintf(in, "            Initial level: %s",
	  glsgd.lev[glsgd.gam[eg_dlg.igam].li].name);
  gtk_label_set(GTK_LABEL(eg_dlg.label[5]), in);
  sprintf(in, "              Final level: %s",
	  glsgd.lev[glsgd.gam[eg_dlg.igam].lf].name);
  gtk_label_set(GTK_LABEL(eg_dlg.label[6]), in);
} /* reset_gamma_pars_current */

void apply_new_gamma_pars(void)
{
  float in;

  save_gls_now(2);
  edit_undo_count++;

  /* extract energy of gamma */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eg_dlg.entry[0])), "%f", &in) == 1 &&
      in > 0.0f && fabs(in - glsgd.gam[eg_dlg.igam].e) > 0.0007f)
    glsgd.gam[eg_dlg.igam].e = in;
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eg_dlg.entry[1])), "%f", &in) == 1 &&
      in > 0.0f && fabs(in - glsgd.gam[eg_dlg.igam].de) > 0.0007f)
    glsgd.gam[eg_dlg.igam].de = in;

  /* extract intensity of gamma */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eg_dlg.entry[2])), "%f", &in) == 1 &&
      in >= 0.0f && fabs(in - glsgd.gam[eg_dlg.igam].i) > 0.0007f)
    glsgd.gam[eg_dlg.igam].i = in;
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eg_dlg.entry[3])), "%f", &in) == 1 &&
      in > 0.0f && fabs(in - glsgd.gam[eg_dlg.igam].di) > 0.0007f)
    glsgd.gam[eg_dlg.igam].di = in;

  /* extract mixing ratio and conv coef */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eg_dlg.entry[4])), "%f", &in) == 1 &&
      fabs(in - glsgd.gam[eg_dlg.igam].d) > 0.0007f)
    glsgd.gam[eg_dlg.igam].d = in;
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(eg_dlg.entry[5])), "%f", &in) == 1 &&
      in >= 0.0f && fabs(in - glsgd.gam[eg_dlg.igam].a) > 0.000007f)
    glsgd.gam[eg_dlg.igam].a = in;

  calc_gls_fig();
  path(1);
  calc_peaks();
  display_gls(-1);
  reset_gamma_pars_current();
} /* apply_new_gamma_pars */

void eg_dlg_cb(GtkWidget *w, gpointer data)
{
  if        (!strcmp((char *)data, "Ok")) {
    apply_new_gamma_pars();
    /* set eg_dlg.done to -1 to hide dialog and exit */
    eg_dlg.done = -1;
  } else if (!strcmp((char *)data, "Apply")) {
    apply_new_gamma_pars();
  } else if (!strcmp((char *)data, "Reset")) {
    undo_gls(-edit_undo_count);
    edit_undo_count = 0;
    reset_gamma_pars_current();
  } else if (!strcmp((char *)data, "IChange")) {
    eg_dlg.done = 10;
  } else if (!strcmp((char *)data, "FChange")) {
    eg_dlg.done = 11;
  } else if (!strcmp((char *)data, "Cancel")) {
    if (eg_dlg.done > 9)  cin[0] = 'A';
    /* hide dialog and exit */
    eg_dlg.status = -1;
    gdk_window_get_root_origin(eg_dlg.w->window, &eg_dlg.x, &eg_dlg.y);
    gtk_widget_hide(eg_dlg.w);
    eg_dlg.done = -1;
  } else if (!strcmp((char *)data, "Recalc")) {
    save_gls_now(2);
    edit_undo_count++;
    calc_alpha(eg_dlg.igam);
    reset_gamma_pars_current();
  }
} /* eg_dlg_cb */

int edit_gamma(void)
{
  static GtkWidget *button_ok, *button_apply, *button_reset;
  static GtkWidget *table, *button_cancel, *button_recalc, *vbox;
  static GtkWidget *button_ichange, *button_fchange;
  static int first = 1;
  float  x1, y1, ds, dp;
  int    i, li, lf, ids = 1;
  char   ch, m[80];

  /* eg_dlg.status:
     = 0 when dialog not in use, and at exit of this routine
     = 1 when prompting for gamma to edit
     = 2 after gamma selected, when asking for modified gamma parameters
     = -1 when dialog has just been hidden by delete or cancel buttons
          (in eg_dlg_cb() or delete_eg_dlg())
     = -2 if dialog has been deleted
   */

  if (first) {
    first = 0;
    eg_dlg.x = eg_dlg.y = -1;
    eg_dlg.status = -2;
    eg_dlg.done = 1;
  }

  if (eg_dlg.status < -1) {
    /* dialog does not exist; create it */
    eg_dlg.w = gtk_dialog_new();
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    set_title(eg_dlg.w, "Edit Gamma");
    eg_dlg.label[0] = gtk_label_new("Select gamma to edit (X or button 3 to exit)...");
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eg_dlg.w)->vbox), eg_dlg.label[0]);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eg_dlg.w)->vbox), vbox);

    for (i=0; i<6; i++) {
      eg_dlg.entry[i] = gtk_entry_new();
      gtk_entry_set_max_length(GTK_ENTRY(eg_dlg.entry[i]), 10);
      gtk_widget_set_usize(eg_dlg.entry[i], 90, 0);
    }
    button_ichange = gtk_button_new_with_label("Change...");
    button_fchange = gtk_button_new_with_label("Change...");

    eg_dlg.label[1] = gtk_label_new("   Gamma energy and error:");
    eg_dlg.label[2] = gtk_label_new("Gamma intensity and error:");
    eg_dlg.label[3] = gtk_label_new("             Mixing ratio:");
    eg_dlg.label[4] = gtk_label_new("   Conversion coefficient:");
    eg_dlg.label[5] = gtk_label_new("            Initial level:");
    eg_dlg.label[6] = gtk_label_new("              Final level:");

    button_ok     = gtk_button_new_with_label("Okay");
    button_apply  = gtk_button_new_with_label("Apply");
    button_reset  = gtk_button_new_with_label("Reset");
    button_cancel = gtk_button_new_with_label("Cancel");
    button_recalc = gtk_button_new_with_label("Recalc. default");

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eg_dlg.w)->action_area),
		      button_ok);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eg_dlg.w)->action_area),
		      button_apply);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eg_dlg.w)->action_area),
		      button_reset);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(eg_dlg.w)->action_area),
		      button_cancel);

    gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		       GTK_SIGNAL_FUNC(eg_dlg_cb), (gpointer) "Ok");
    gtk_signal_connect(GTK_OBJECT(button_apply), "clicked",
		       GTK_SIGNAL_FUNC(eg_dlg_cb), (gpointer) "Apply");
    gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		       GTK_SIGNAL_FUNC(eg_dlg_cb), (gpointer) "Reset");
    gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		       GTK_SIGNAL_FUNC(eg_dlg_cb), (gpointer) "Cancel");
    gtk_signal_connect(GTK_OBJECT(button_recalc), "clicked",
		       GTK_SIGNAL_FUNC(eg_dlg_cb), (gpointer) "Recalc");
    gtk_signal_connect(GTK_OBJECT(button_ichange), "clicked",
		       GTK_SIGNAL_FUNC(eg_dlg_cb), (gpointer) "IChange");
    gtk_signal_connect(GTK_OBJECT(button_fchange), "clicked",
		       GTK_SIGNAL_FUNC(eg_dlg_cb), (gpointer) "FChange");
    gtk_signal_connect(GTK_OBJECT(eg_dlg.w), "delete_event",
		       GTK_SIGNAL_FUNC(delete_eg_dlg), NULL);

    table = gtk_table_new(6, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(vbox), table);
    for (i=0; i<4; i++) {
      gtk_table_attach(GTK_TABLE(table), eg_dlg.label[i+1], 0, 1, i, i+1,
		       (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
    }
    gtk_table_attach(GTK_TABLE(table), eg_dlg.label[5], 0, 2, 4, 5,
		     (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eg_dlg.label[6], 0, 2, 5, 6,
		     (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eg_dlg.entry[0], 1, 2, 0, 1,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eg_dlg.entry[1], 2, 3, 0, 1,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eg_dlg.entry[2], 1, 2, 1, 2,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eg_dlg.entry[3], 2, 3, 1, 2,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eg_dlg.entry[4], 1, 2, 2, 3,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eg_dlg.entry[5], 1, 2, 3, 4,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), button_recalc, 2, 3, 3, 4,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), button_ichange, 2, 3, 4, 5,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), button_fchange, 2, 3, 5, 6,
		     GTK_FILL, GTK_FILL, 0, 0);
  }

  while (1) {
    /* Display the dialog if it's not already shown */
    if (eg_dlg.status < 1) {
      gtk_widget_show_all(eg_dlg.w);
      move_win_to(eg_dlg.w, eg_dlg.x, eg_dlg.y);
    }
    /* tell("Select gamma to edit (X or button 3 to exit)...\n"); */
    gtk_label_set(GTK_LABEL(eg_dlg.label[0]),
		  "Select gamma to edit (X or button 3 to exit)...");
    gtk_widget_set_sensitive(vbox, FALSE);
    gtk_widget_set_sensitive(button_ok, FALSE);
    gtk_widget_set_sensitive(button_apply, FALSE);
    gtk_widget_set_sensitive(button_reset, FALSE);
    gdk_window_raise(eg_dlg.w->window);
    eg_dlg.status = 1;

    /* retic2(&x1, &y1, ans);
       if (*ans == 'X' || *ans == 'x') break; */
    bell();
    cin[0] = 0;
    while (cin[0] == 0 && eg_dlg.status > 0) gtk_main_iteration();
    /* the next line (eg_dlg.status < 0) happens if dialog was hidden
       by killing or canceling it */
    if (eg_dlg.status < 0 || *cin == 'X' || *cin == 'x') break;
    /* the next line (eg_dlg.status == 0) happens if dialog was hidden
       by a later instance of edit_band */
    if (eg_dlg.status == 0) continue;

    eg_dlg.igam = nearest_gamma(xin, yin);
    if (eg_dlg.igam < 0 || eg_dlg.igam > glsgd.ngammas) {
      tell("  ... no gamma selected, try again...\n");
      continue;
    }
    /* tell(" Gamma %.1f selected...\n", glsgd.gam[eg_dlg.igam].e); */
    sprintf(m, " Gamma %.1f selected...", glsgd.gam[eg_dlg.igam].e);
    gtk_label_set(GTK_LABEL(eg_dlg.label[0]), m);

    gtk_widget_set_sensitive(vbox, TRUE);
    gtk_widget_set_sensitive(button_ok, TRUE);
    gtk_widget_set_sensitive(button_apply, TRUE);
    gtk_widget_set_sensitive(button_reset, TRUE);
    gtk_window_set_focus(GTK_WINDOW(eg_dlg.w), eg_dlg.entry[0]);
    reset_gamma_pars_current();
    eg_dlg.done = 0;
    edit_undo_count = 0;
    eg_dlg.status = 2;
    insensitive();

    while (1) {
      gdk_window_raise(eg_dlg.w->window);
      cin[0] = 0;
      while (cin[0] == 0 &&
	     !eg_dlg.done && eg_dlg.status > 0) gtk_main_iteration();
      if (eg_dlg.done > 9) {
	/* change initial or final level for gamma... */
	gtk_widget_set_sensitive(vbox, FALSE);
	gtk_widget_set_sensitive(button_ok, FALSE);
	gtk_widget_set_sensitive(button_apply, FALSE);
	gtk_widget_set_sensitive(button_reset, FALSE);
	gdk_window_raise(eg_dlg.w->window);
	if (eg_dlg.done == 10) {
	  /* change initial level... */
	  sprintf(m, "Select new initial level for Gamma %.1f",
		  glsgd.gam[eg_dlg.igam].e);
	} else {
	  /* change final level... */
	  sprintf(m, "Select new final level for Gamma %.1f",
		  glsgd.gam[eg_dlg.igam].e);
	}
	gtk_label_set(GTK_LABEL(eg_dlg.label[0]), m);
	while (1) {
	  retic2(&x1, &y1, m);
	  if (*m == 'X' || *m == 'x' ||
	      *m == 'A' || *m == 'a') break;
	  i = nearest_level(xin, yin);
	  if (i < 0 || i > glsgd.nlevels) {
	    tell("  ... no level selected, try again...\n");
	    continue;
	  }
	  if (eg_dlg.done == 10) {
	    li = i;
	    lf = glsgd.gam[eg_dlg.igam].lf;
	  } else {
	    lf = i;
	    li = glsgd.gam[eg_dlg.igam].li;
	  }
	  if (glsgd.lev[lf].e >= glsgd.lev[li].e) {
	    warn("Error - cannot have initial level lower than final level.");
	    continue;
	  }
	  ds = fabs(glsgd.lev[li].j - glsgd.lev[lf].j);
	  ids = (int) (ds + 0.5f);
	  if (fabs(ds - (float) ids) > 0.1f) {
	    warn("ERROR - cannot have one level with integer spin,\n"
		 "                  other level with half-int spin.\n");
	    continue;
	  }

	  save_gls_now(2);
	  edit_undo_count++;
	  if (ids > 9) ids = 9;
	  if (ids == 0) ids = 1;
	  dp = glsgd.lev[li].pi * glsgd.lev[lf].pi;
	  if (dp > 0.0f) {
	    if (ids % 2 == 0) ch = 'E';
	    else ch = 'M';
	  } else {
	    if (ids % 2 == 0) ch = 'M';
	    else ch = 'E';
	  }
	  glsgd.gam[eg_dlg.igam].em = ch;
	  glsgd.gam[eg_dlg.igam].n = (float) ids;
	  glsgd.gam[eg_dlg.igam].li = li;
	  glsgd.gam[eg_dlg.igam].lf = lf;
	  tell("Gamma %.1f now has multipolarity %c%i\n",
	       glsgd.gam[eg_dlg.igam].e, ch, ids);
	  calc_gls_fig();
	  display_gls(-1);
	  break;
	}
	sprintf(m, " Gamma %.1f selected...", glsgd.gam[eg_dlg.igam].e);
	gtk_label_set(GTK_LABEL(eg_dlg.label[0]), m);
	gtk_widget_set_sensitive(vbox, TRUE);
	gtk_widget_set_sensitive(button_ok, TRUE);
	gtk_widget_set_sensitive(button_apply, TRUE);
	gtk_widget_set_sensitive(button_reset, TRUE);
	gtk_window_set_focus(GTK_WINDOW(eg_dlg.w), eg_dlg.entry[0]);
	reset_gamma_pars_current();
	if (eg_dlg.done >= 0) eg_dlg.done = 0;
	path(1);
	calc_peaks();
	continue;
      }
      if (cin[0] == 0) break;
      i = nearest_gamma(xin, yin);
      if (i < 0 || i > glsgd.ngammas) {
	tell("  ... no gama selected, try again...\n");
      } else {
	eg_dlg.igam = i;
	sprintf(m, " Gamma %.1f selected...", glsgd.gam[eg_dlg.igam].e);
	gtk_label_set(GTK_LABEL(eg_dlg.label[0]), m);
	reset_gamma_pars_current();
      }
    }
    sensitive();
    if (!eg_dlg.status || eg_dlg.done < 0) break;
  }
  cin[0] = 0;
 
  /* Hide the dialog and return */
  if (eg_dlg.status > 0) {
    gdk_window_get_root_origin(eg_dlg.w->window, &eg_dlg.x, &eg_dlg.y);
    gtk_widget_hide(eg_dlg.w);
  }
  if (eg_dlg.status > -2) eg_dlg.status = 0;
  eg_dlg.done = 0;
  return 0;
} /* edit_gamma */

/* ===========  dialog to set level parameters  ============ */
int delete_el_dlg(void)
{
  gdk_window_get_root_origin(el_dlg.w->window, &el_dlg.x, &el_dlg.y);
  el_dlg.status = -2;
  el_dlg.done = -1;
  return FALSE;
} /* delete_el_dlg */

void reset_level_pars_current(void)
{
  char  in[80];

  sprintf(in, "%.3f", glsgd.lev[el_dlg.ilev].e);
  gtk_entry_set_text(GTK_ENTRY(el_dlg.entry[0]), in);
  sprintf(in, "%.3f", glsgd.lev[el_dlg.ilev].de);
  gtk_entry_set_text(GTK_ENTRY(el_dlg.entry[1]), in);
  sprintf(in, "%.1f", glsgd.lev[el_dlg.ilev].j);
  gtk_entry_set_text(GTK_ENTRY(el_dlg.entry[2]), in);
  if (glsgd.lev[el_dlg.ilev].pi > 0.0f) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(el_dlg.entry[3]), 0);
    el_dlg.parity = 1;
  } else {
    gtk_combo_box_set_active(GTK_COMBO_BOX(el_dlg.entry[3]), 1);
    el_dlg.parity = -1;
  }
} /* reset_level_pars_current */

void apply_new_level_pars(void)
{
  int   i, iis, iis2, ids, iip = 1, err = 0;
  float f, in, de = 0.0f, ds = 0.0f, dp;
  char  ch;

  save_gls_now(3);
  edit_undo_count++;

  /* extract energy of level */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(el_dlg.entry[0])), "%f", &in) == 1 &&
      in >= 0.0f && fabs(in - glsgd.lev[el_dlg.ilev].e) > 0.0007f) {
    err = 0;
    for (i = 0; (!err && i < glsgd.ngammas); ++i) {
      if (el_dlg.ilev == glsgd.gam[i].li) {
	f = in - glsgd.lev[glsgd.gam[i].lf].e;
      } else if (el_dlg.ilev == glsgd.gam[i].lf) {
	f = glsgd.lev[glsgd.gam[i].li].e - in;
      } else {
	continue;
      }
      if (f < 0.0f) {
	warn("ERROR -- cannot have gammas with energy < 0...\n"
	     "   Level energy not changed!\n");
	err = 1;
      }
    }
    if (!err) {
      de = in - glsgd.lev[el_dlg.ilev].e;
      glsgd.lev[el_dlg.ilev].e = in;
      for (i = 0; i < glsgd.ngammas; ++i) {
	if (el_dlg.ilev == glsgd.gam[i].li || el_dlg.ilev == glsgd.gam[i].lf) {
	  f = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e;
	  tell("Gamma %.1f now has energy %.1f\n", glsgd.gam[i].e, f);
	  glsgd.gam[i].e = f;
	}
      }
    }
  }
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(el_dlg.entry[1])), "%f", &in) == 1 &&
      in > 0.0f && fabs(in - glsgd.lev[el_dlg.ilev].de) > 0.0007f)
    glsgd.lev[el_dlg.ilev].de = in;

  /* extract spin and parity */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(el_dlg.entry[2])), "%f", &in) == 1 &&
      in >= 0.0f && fabs(in - glsgd.lev[el_dlg.ilev].j) > 0.1f) {
    err = 0;
    ds = in - glsgd.lev[el_dlg.ilev].j;
    iis = (int) rint(in);
    iis2 = (int) rint(in*2.0f);
    if (in != (float) iis2 / 2.0f) {
      warn("ERROR -- spin must be integer or half-integer.\n");
      err = 1;
    } 
    iis = (int) rint(ds);
    for (i = 0; (!err && i < glsgd.ngammas); ++i) {
      if ((el_dlg.ilev == glsgd.gam[i].li || el_dlg.ilev == glsgd.gam[i].lf) &&
	  ds != (float) iis) {
	warn("ERROR -- spin change must be integer\n"
	     "    since there are connecting transitions.\n");
	err = 1;
      }
    }
    for (i = 0; (!err && i < glsgd.nlevels); ++i) {
      if (i != el_dlg.ilev &&
	  glsgd.lev[i].band == glsgd.lev[el_dlg.ilev].band &&
	  fabs(glsgd.lev[i].j - in) < 0.7) {
	if (!askyn("Warning: That band already has a state of the same spin...\n"
		   "   ...Continue anyway? (Y/N)")) err = 1;
	break;
      }
    }
    if (err) ds = 0.0f;
  }
  if (gtk_combo_box_get_active(GTK_COMBO_BOX(el_dlg.entry[3])) == 0) {
    el_dlg.parity = 1;
  } else {
    el_dlg.parity = -1;
  }
  if (glsgd.lev[el_dlg.ilev].pi != (float) el_dlg.parity) iip = -1;

  if (ds != 0.0f || iip == -1) {
    glsgd.lev[el_dlg.ilev].j += ds;
    glsgd.lev[el_dlg.ilev].pi *= (float) iip;
    for (i = 0; i < glsgd.ngammas; ++i) {
      if (el_dlg.ilev == glsgd.gam[i].li || el_dlg.ilev == glsgd.gam[i].lf) {
	ids = abs((int) (glsgd.lev[glsgd.gam[i].li].j -
			 glsgd.lev[glsgd.gam[i].lf].j + 0.5f));
	if (ids > 9) ids = 9;
	if (ids == 0) ids = 1;
	dp = glsgd.lev[glsgd.gam[i].li].pi * glsgd.lev[glsgd.gam[i].lf].pi;
	if (dp > 0.0f) {
	  if (ids % 2 == 0) ch = 'E';
	  else ch = 'M';
	} else {
	  if (ids % 2 == 0) ch = 'M';
	  else ch = 'E';
	}
	glsgd.gam[i].em = ch;
	glsgd.gam[i].n = (float) ids;
	tell("Gamma %.1f now has multipolarity %c%i\n",
	     glsgd.gam[i].e, ch, ids);
      }
    }
  }

  if (ds != 0.0f || iip != 1 || fabs(de) > 1.0f) {
    tell("Some conversion coefficients may be recalculated...\n");
    for (i = 0; i < glsgd.ngammas; ++i) {
      if (glsgd.gam[i].li == el_dlg.ilev ||
	  glsgd.gam[i].lf == el_dlg.ilev) calc_alpha(i);
    }
  }

  calc_band();
  calc_gls_fig();
  path(1);
  calc_peaks();
  display_gls(-1);
  reset_level_pars_current();
} /* apply_new_level_pars */

void el_dlg_cb(GtkWidget *w, gpointer data)
{
  if        (!strcmp((char *)data, "Ok")) {
    apply_new_level_pars();
    /* set el_dlg.done to -1 to hide dialog and exit */
    el_dlg.done = -1;
  } else if (!strcmp((char *)data, "Apply")) {
    apply_new_level_pars();
  } else if (!strcmp((char *)data, "Reset")) {
    undo_gls(-edit_undo_count);
    edit_undo_count = 0;
    reset_level_pars_current();
  } else if (!strcmp((char *)data, "Cancel")) {
    /* hide dialog and exit */
    el_dlg.status = -1;
    gdk_window_get_root_origin(el_dlg.w->window, &el_dlg.x, &el_dlg.y);
    gtk_widget_hide(el_dlg.w);
    el_dlg.done = -1;
  }
} /* el_dlg_cb */

int edit_level(void)
{
  static GtkWidget *button_ok, *button_apply, *button_reset, *button_cancel;
  static GtkWidget *label[4], *table, *vbox;
  static int first = 1;
  int    i;
  char   m[80];

  /* el_dlg.status:
     = 0 when dialog not in use, and at exit of this routine
     = 1 when prompting for level to edit
     = 2 after level selected, when asking for modified level parameters
     = -1 when dialog has just been hidden by delete or cancel buttons
          (in el_dlg_cb() or delete_el_dlg())
     = -2 if dialog has been deleted
   */

  if (first) {
    first = 0;
    el_dlg.x = el_dlg.y = -1;
    el_dlg.status = -2;
    el_dlg.done = 1;
  }

  if (el_dlg.status < -1) {
    /* dialog does not exist; create it */
    el_dlg.w = gtk_dialog_new();
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    set_title(el_dlg.w, "Edit Level");
    label[0] = gtk_label_new("Select level to edit (X or button 3 to exit)...");
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(el_dlg.w)->vbox), label[0]);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(el_dlg.w)->vbox), vbox);

    for (i=0; i<3; i++) {
      el_dlg.entry[i] = gtk_entry_new();
      gtk_entry_set_max_length(GTK_ENTRY(el_dlg.entry[i]), 12);
      gtk_widget_set_usize(el_dlg.entry[i], 90, 0);
    }
    el_dlg.entry[3] = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(el_dlg.entry[3]), "even");
    gtk_combo_box_append_text(GTK_COMBO_BOX(el_dlg.entry[3]), "odd");

    label[1] = gtk_label_new("Level energy and error:");
    label[2] = gtk_label_new(" Level spin and parity:");

    button_ok     = gtk_button_new_with_label("Okay");
    button_apply  = gtk_button_new_with_label("Apply");
    button_reset  = gtk_button_new_with_label("Reset");
    button_cancel = gtk_button_new_with_label("Cancel");

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(el_dlg.w)->action_area),
		      button_ok);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(el_dlg.w)->action_area),
		      button_apply);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(el_dlg.w)->action_area),
		      button_reset);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(el_dlg.w)->action_area),
		      button_cancel);

    gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		       GTK_SIGNAL_FUNC(el_dlg_cb), (gpointer) "Ok");
    gtk_signal_connect(GTK_OBJECT(button_apply), "clicked",
		       GTK_SIGNAL_FUNC(el_dlg_cb), (gpointer) "Apply");
    gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		       GTK_SIGNAL_FUNC(el_dlg_cb), (gpointer) "Reset");
    gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		       GTK_SIGNAL_FUNC(el_dlg_cb), (gpointer) "Cancel");
    gtk_signal_connect(GTK_OBJECT(el_dlg.w), "delete_event",
		       GTK_SIGNAL_FUNC(delete_el_dlg), NULL);

    table = gtk_table_new(3, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(vbox), table);
    for (i=1; i<3; i++) {
      gtk_table_attach(GTK_TABLE(table), label[i], 0, 1, i-1, i,
		       (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
    }
    gtk_table_attach(GTK_TABLE(table), el_dlg.entry[0], 1, 2, 0, 1,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), el_dlg.entry[1], 2, 3, 0, 1,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), el_dlg.entry[2], 1, 2, 1, 2,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), el_dlg.entry[3], 2, 3, 1, 2,
		     GTK_FILL, GTK_FILL, 0, 0);
  }

  while (1) {
    /* Display the dialog if it's not already shown */
    if (el_dlg.status < 1) {
      gtk_widget_show_all(el_dlg.w);
      move_win_to(el_dlg.w, el_dlg.x, el_dlg.y);
    }

    /* tell("Select level to edit (X or button 3 to exit)...\n"); */
    gtk_label_set(GTK_LABEL(label[0]),
		  "Select level to edit (X or button 3 to exit)...");
    gtk_widget_set_sensitive(vbox, FALSE);
    gtk_widget_set_sensitive(button_ok, FALSE);
    gtk_widget_set_sensitive(button_apply, FALSE);
    gtk_widget_set_sensitive(button_reset, FALSE);
    gdk_window_raise(el_dlg.w->window);
    el_dlg.status = 1;

    /* retic2(&x1, &y1, ans);
       if (*ans == 'X' || *ans == 'x') break; */
    bell();
    cin[0] = 0;
    while (cin[0] == 0 && el_dlg.status > 0) gtk_main_iteration();
    /* the next line (el_dlg.status < 0) happens if dialog was hidden
       by killing or canceling it */
    if (el_dlg.status < 0 || *cin == 'X' || *cin == 'x') break;
    /* the next line (el_dlg.status == 0) happens if dialog was hidden
       by a later instance of edit_band */
    if (el_dlg.status == 0) continue;

    el_dlg.ilev = nearest_level(xin, yin);
    if (el_dlg.ilev < 0 || el_dlg.ilev > glsgd.nlevels) {
      tell("  ... no level selected, try again...\n");
      continue;
    }
    /* tell(" Level %s selected...\n", glsgd.lev[el_dlg.ilev].name); */
    sprintf(m, " Level %s selected...", glsgd.lev[el_dlg.ilev].name);
    gtk_label_set(GTK_LABEL(label[0]), m);

    gtk_widget_set_sensitive(vbox, TRUE);
    gtk_widget_set_sensitive(button_ok, TRUE);
    gtk_widget_set_sensitive(button_apply, TRUE);
    gtk_widget_set_sensitive(button_reset, TRUE);
    gtk_window_set_focus(GTK_WINDOW(el_dlg.w), el_dlg.entry[0]);
    reset_level_pars_current();
    el_dlg.done = 0;
    edit_undo_count = 0;
    el_dlg.status = 2;
    insensitive();

    while (1) {
      gdk_window_raise(el_dlg.w->window);
      cin[0] = 0;
      while (cin[0] == 0 &&
	     !el_dlg.done && el_dlg.status > 0) gtk_main_iteration();
      if (cin[0] == 0) break;
      i = nearest_level(xin, yin);
      if (i < 0 || i > glsgd.nlevels) {
	tell("  ... no level selected, try again...\n");
      } else {
	el_dlg.ilev = i;
	sprintf(m, " Level %s selected...", glsgd.lev[el_dlg.ilev].name);
	gtk_label_set(GTK_LABEL(label[0]), m);
	reset_level_pars_current();
      }
    }
    sensitive();
    if (!el_dlg.status || el_dlg.done < 0) break;
  }
  cin[0] = 0;

  /* Hide the dialog and return */
  if (el_dlg.status > 0) {
    gdk_window_get_root_origin(el_dlg.w->window, &el_dlg.x, &el_dlg.y);
    gtk_widget_hide(el_dlg.w);
  }
  if (el_dlg.status > -2) el_dlg.status = 0;
  el_dlg.done = 0;
  return 0;
} /* edit_level */

/* ===========  dialog to set text label parameters  ============ */
int delete_et_dlg(void)
{
  gdk_window_get_root_origin(et_dlg.w->window, &et_dlg.x, &et_dlg.y);
  et_dlg.status = -2;
  et_dlg.done = -1;
  return FALSE;
} /* delete_et_dlg */

void reset_text_pars_current(void)
{
  char  in[80];

  sprintf(in, "%.40s", glsgd.txt[et_dlg.itxt].l);
  gtk_entry_set_text(GTK_ENTRY(et_dlg.entry[0]), in);
  sprintf(in, "%.0f", glsgd.txt[et_dlg.itxt].csx);
  gtk_entry_set_text(GTK_ENTRY(et_dlg.entry[1]), in);
  /* sprintf(in, "%.0f", glsgd.txt[et_dlg.itxt].csy);
     gtk_entry_set_text(GTK_ENTRY(et_dlg.entry[2]), in); */
} /* reset_text_pars_current */

void apply_new_text_pars(void)
{
  float in;
  int   nc;
  char  ans[80];

  save_gls_now(8);
  edit_undo_count++;

  /* extract text */
  strncpy(ans, gtk_entry_get_text(GTK_ENTRY(et_dlg.entry[0])), 40);
  ans[40] = '\0';
  nc = strlen(ans);
  while (nc > 0 && ans[nc-1] == ' ') /* remove trailing blanks */
    ans[--nc] = '\0';
  if (nc > 0) {
    strncpy(glsgd.txt[et_dlg.itxt].l, ans, 40);
    glsgd.txt[et_dlg.itxt].nc = nc;
  }

  /* extract character sizes */
  if (sscanf(gtk_entry_get_text(GTK_ENTRY(et_dlg.entry[1])), "%f", &in) == 1 &&
      in > 0.0f && fabs(glsgd.txt[et_dlg.itxt].csx - in) > 0.1f) {
    glsgd.txt[et_dlg.itxt].csx = in;
    glsgd.txt[et_dlg.itxt].csy = in*CSY;
  }
  /* if (sscanf(gtk_entry_get_text(GTK_ENTRY(et_dlg.entry[2])), "%f", &in) == 1 &&
     in > 0.0f && fabs(glsgd.txt[et_dlg.itxt].csy - in) > 0.1f)
     glsgd.txt[et_dlg.itxt].csy = in;
  */

  display_gls(-1);
  reset_text_pars_current();
} /* apply_new_text_pars */

void et_dlg_cb(GtkWidget *w, gpointer data)
{
  if        (!strcmp((char *)data, "Ok")) {
    apply_new_text_pars();
    /* set et_dlg.done to -1 to hide dialog and exit */
    et_dlg.done = -1;
  } else if (!strcmp((char *)data, "Apply")) {
    apply_new_text_pars();
  } else if (!strcmp((char *)data, "Reset")) {
    undo_gls(-edit_undo_count);
    edit_undo_count = 0;
    reset_text_pars_current();
  } else if (!strcmp((char *)data, "Cancel")) {
    /* hide dialog and exit */
    et_dlg.status = -1;
    gdk_window_get_root_origin(et_dlg.w->window, &et_dlg.x, &et_dlg.y);
    gtk_widget_hide(et_dlg.w);
    et_dlg.done = -1;
  }
} /* et_dlg_cb */

int edit_text(void)
{
  static GtkWidget *button_ok, *button_apply, *button_reset;
  static GtkWidget *label[3], *table, *button_cancel, *vbox;
  static int first = 1;
  int    i;
  char   m[80];

  /* et_dlg.status:
     = 0 when dialog not in use, and at exit of this routine
     = 1 when prompting for text to edit
     = 2 after text selected, when asking for modified text parameters
     = -1 when dialog has just been hidden by delete or cancel buttons
          (in et_dlg_cb() or delete_et_dlg())
     = -2 if dialog has been deleted
   */

  if (first) {
    first = 0;
    et_dlg.x = et_dlg.y = -1;
    et_dlg.status = -2;
    et_dlg.done = 1;
  }

  if (et_dlg.status < -1) {
    /* dialog does not exist; create it */
    et_dlg.w = gtk_dialog_new();
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    set_title(et_dlg.w, "Edit Text");
    label[2] = gtk_label_new("Select text label to edit (X or button 3 to exit)...");
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(et_dlg.w)->vbox), label[2]);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(et_dlg.w)->vbox), vbox);

    for (i=0; i<2; i++) {
      et_dlg.entry[i] = gtk_entry_new();
      gtk_entry_set_max_length(GTK_ENTRY(et_dlg.entry[i]), 8);
    }
    gtk_entry_set_max_length(GTK_ENTRY(et_dlg.entry[0]), 40);
    gtk_widget_set_usize(et_dlg.entry[0], 230, 0);
    gtk_widget_set_usize(et_dlg.entry[1], 90, 0);

    label[0] = gtk_label_new(" Text for label (max. 40 chars):");
    label[1] = gtk_label_new("                 Character size:");

    button_ok     = gtk_button_new_with_label("Okay");
    button_apply  = gtk_button_new_with_label("Apply");
    button_reset  = gtk_button_new_with_label("Reset");
    button_cancel = gtk_button_new_with_label("Cancel");

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(et_dlg.w)->action_area),
		      button_ok);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(et_dlg.w)->action_area),
		      button_apply);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(et_dlg.w)->action_area),
		      button_reset);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(et_dlg.w)->action_area),
		      button_cancel);

    gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		       GTK_SIGNAL_FUNC(et_dlg_cb), (gpointer) "Ok");
    gtk_signal_connect(GTK_OBJECT(button_apply), "clicked",
		       GTK_SIGNAL_FUNC(et_dlg_cb), (gpointer) "Apply");
    gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		       GTK_SIGNAL_FUNC(et_dlg_cb), (gpointer) "Reset");
    gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		       GTK_SIGNAL_FUNC(et_dlg_cb), (gpointer) "Cancel");
    gtk_signal_connect(GTK_OBJECT(et_dlg.w), "delete_event",
		       GTK_SIGNAL_FUNC(delete_et_dlg), NULL);

    table = gtk_table_new(2, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(vbox), table);
    for (i=0; i<2; i++) {
      gtk_table_attach(GTK_TABLE(table), label[i], 0, 1, i, i+1,
		       (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
    }
    gtk_table_attach(GTK_TABLE(table), et_dlg.entry[0], 1, 3, 0, 1,
		     (GTK_FILL | GTK_EXPAND | GTK_SHRINK), GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(table), et_dlg.entry[1], 1, 2, 1, 2,
		     GTK_SHRINK, GTK_FILL, 0, 0);
    /* gtk_table_attach(GTK_TABLE(table), et_dlg.entry[2], 2, 3, 1, 2,
       GTK_FILL, GTK_FILL, 0, 0); */
  }

  while (1) {
    if (glsgd.ntlabels == 0) {
      warn("Have no text labels to edit.\n");
      break;
    }

    /* Display the dialog if it's not already shown */
    if (et_dlg.status < 1) {
      gtk_widget_show_all(et_dlg.w);
      move_win_to(et_dlg.w, et_dlg.x, et_dlg.y);
    }

    /* tell("Select text label to edit (X or button 3 to exit)...\n"); */
    gtk_label_set(GTK_LABEL(label[2]),
		  "Select text label to edit (X or button 3 to exit)...");
    gtk_widget_set_sensitive(vbox, FALSE);
    gtk_widget_set_sensitive(button_ok, FALSE);
    gtk_widget_set_sensitive(button_apply, FALSE);
    gtk_widget_set_sensitive(button_reset, FALSE);
    gdk_window_raise(et_dlg.w->window);
    et_dlg.status = 1;

    /* retic2(&x1, &y1, ans);
       if (*ans == 'X' || *ans == 'x') return 0; */
    bell();
    cin[0] = 0;
    while (cin[0] == 0 && et_dlg.status > 0) gtk_main_iteration();
    /* the next line (et_dlg.status < 0) happens if dialog was hidden
       by killing or canceling it */
    if (et_dlg.status < 0 || *cin == 'X' || *cin == 'x') break;
    /* the next line (et_dlg.status == 0) happens if dialog was hidden
       by a later instance of edit_band */
    if (et_dlg.status == 0) continue;

    et_dlg.itxt = -1;
    for (i = 0; i < glsgd.ntlabels; ++i) {
      if (xin > glsgd.txt[i].x - (float) glsgd.txt[i].nc * glsgd.txt[i].csx / 2.0f &&
	  xin < glsgd.txt[i].x + (float) glsgd.txt[i].nc * glsgd.txt[i].csx / 2.0f &&
	  yin > glsgd.txt[i].y &&
	  yin < glsgd.txt[i].y + glsgd.txt[i].csy) {
	et_dlg.itxt = i;
	break;
      }
    }
    if (et_dlg.itxt < 0) {
      tell(" ... No text label selected, try again ...\n");
      continue;
    }
    sprintf(m, " Label \"%.40s\" selected...", glsgd.txt[et_dlg.itxt].l);
    gtk_label_set(GTK_LABEL(label[2]), m);

    gtk_widget_set_sensitive(vbox, TRUE);
    gtk_widget_set_sensitive(button_ok, TRUE);
    gtk_widget_set_sensitive(button_apply, TRUE);
    gtk_widget_set_sensitive(button_reset, TRUE);
    gtk_window_set_focus(GTK_WINDOW(et_dlg.w), et_dlg.entry[0]);
    reset_text_pars_current();
    et_dlg.done = 0;
    edit_undo_count = 0;
    et_dlg.status = 2;
    insensitive();

    while (1) {
      gdk_window_raise(et_dlg.w->window);
      cin[0] = 0;
      while (cin[0] == 0 &&
	     !et_dlg.done && et_dlg.status > 0) gtk_main_iteration();
      if (cin[0] == 0) break;
      for (i = 0; i < glsgd.ntlabels; ++i) {
	if (xin > glsgd.txt[i].x - (float) glsgd.txt[i].nc * glsgd.txt[i].csx / 2.f &&
	    xin < glsgd.txt[i].x + (float) glsgd.txt[i].nc * glsgd.txt[i].csx / 2.f &&
	    yin > glsgd.txt[i].y &&
	    yin < glsgd.txt[i].y + glsgd.txt[i].csy) {
	  et_dlg.itxt = i;
	  break;
	}
      }
      if (i >= glsgd.ntlabels) {
	tell(" ... No text label selected, try again ...\n");
      } else {
	sprintf(m, " Label \"%.40s\" selected...", glsgd.txt[et_dlg.itxt].l);
	gtk_label_set(GTK_LABEL(label[2]), m);
	reset_text_pars_current();
      }
    }
    sensitive();
    if (!et_dlg.status || et_dlg.done < 0) break;
  }
  cin[0] = 0;
 
  /* Hide the dialog and return */
  if (et_dlg.status > 0) {
    gdk_window_get_root_origin(et_dlg.w->window, &et_dlg.x, &et_dlg.y);
    gtk_widget_hide(et_dlg.w);
  }
  if (et_dlg.status > -2) et_dlg.status = 0;
  et_dlg.done = 0;
  return 0;
} /* edit_text */

/* ============  code to save and restore window pos & size  ============ */
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

  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xmg");
#else
  strcat (filename, "/.radware.xmg");
#endif
  if (!(fp = fopen(filename, "a+"))) {
    warn("Cannot open file %s\n", filename);
    return 1;
  }
  rewind(fp);

  xmg[0] = (float)win_origin_x / (float)scr_width;
  xmg[1] = (float)win_origin_y / (float)scr_height;
  xmg[2] = (float)win_w  / (float)scr_width;
  xmg[3] = (float)win_h  / (float)scr_height;
  
  while (fgets(line[lineNum], 120, fp) && lineNum < 20) {
    lineNum++;
    for (i=0; i<lineNum-1; i++) {
      if (!strncmp(line[i], line[lineNum-1],8)) {
	lineNum--;
	break;
      }
    }
  }

  fclose(fp);
  if (!(fp = fopen(filename, "w+"))) {
    warn("Cannot open file %s\n", filename);
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

/* ====================================================================== */
int askyn(const char *fmt, ...)
{
  /* fmt, ...:  question to be asked (format string and variable no. of args)
     returns 1: answer = Y/y/1   0: answer = N/n/0/<return> */

  va_list ap;
  char    q[4096];

  va_start(ap, fmt);
  vsnprintf(q, 4095, fmt, ap);
  va_end(ap);

  return caskyn(q);
} /* askyn */

/* ====================================================================== */
void tell(const char *fmt, ...)
{
  /* fmt, ...:  string to be output (format string and variable no. of args) */

  va_list     ap;
  char        mesag[8192], *c;
  int         ncm;
  GtkTextIter   iter, iter1, iter2;
  GtkAdjustment *adj;
  int           lines;

  if (!fixed_font_id) fixed_font_id = gdk_font_load(FIXEDFONTNAME);

  va_start(ap, fmt);
  ncm = vsnprintf(mesag, 8191, fmt, ap);
  va_end(ap);

  if ((c = strchr(mesag, (char) 7))) {
    *c = ' ';
    bell();
  }

  /* insert text from mesag at end of text buffer */
  gtk_text_buffer_get_end_iter(output_text_buffer, &iter);
  gtk_text_buffer_insert(output_text_buffer, &iter, mesag, ncm);

  /* if there are more than 1200 lines,
     delete some text from the start of the buffer */
  lines = gtk_text_buffer_get_line_count(output_text_buffer);
  // printf("%d lines\n", lines); fflush(stdout);
  if (lines > 1200) {
    gtk_text_buffer_get_start_iter(output_text_buffer, &iter1);
    gtk_text_buffer_get_iter_at_line(output_text_buffer, &iter2, lines-1000);
    gtk_text_buffer_delete(output_text_buffer, &iter1, &iter2);
  }

  /* scroll the text view widget to make the last character visible */
  gtk_text_buffer_get_end_iter(output_text_buffer, &iter);
  gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(output_text_view), &iter,
			       0.0, FALSE, 0.0, 0.0);
  adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(text_scrolled));
  adj->value = adj->upper;
  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(text_scrolled), adj);

} /* tell */

/* ================  question dialog  ================ */
void sel_dlg_cb(GtkWidget *w, gpointer data)
{
  if (!strcmp((char *)data, "Cancel")) {
    sel_dlg.rtn_val = -1;
  } else if (!strcmp((char *)data, "Ok")) {
    sel_dlg.rtn_val = gtk_combo_box_get_active(GTK_COMBO_BOX(sel_dlg.entry));
  }
  dlg_done = 1;
} /* sel_dlg_button_cb */

int ask_selection(int nsel, int default_sel, const char **choices,
		 const char *flags, const char *head, const char *tail)
{
  /* returns item number selected from among nsel choices (counting from 0)
     default_sel = default choice number (used for simple <rtn>, no choice)
     choices = array of strings from which to choose
     flags = optional array of chars to use as labels for the choices
     head, tail = strings at start and end of question
  */

  GtkWidget *button_ok, *button_cancel, *label, *vbox;
  int    i, nc;
  char   ans[80];

  if (nsel < 2 || nsel > 36) return 0;

  /* first handle cases where we should be reading
     the response from a command file */
  while ((nc = read_cmd_file(ans, 80)) >= 0) {
    if (nc == 0) return default_sel;
    for (i = 0; i < nsel; i++) {
      if (*ans == flags[i] ||
	  *ans == tolower(flags[i]) ||
	  *ans == toupper(flags[i])) return i;
    }
  }

  if (strstr(head,"Please choose a font:")) return ps_dlg.font;

  sel_dlg.rtn_val = -1;
  dlg_done = 0;
  sel_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sel_dlg.w)->vbox), vbox);
  set_title(sel_dlg.w, "Selection");

  if (*head) {
    label = gtk_label_new(head);
    gtk_container_add(GTK_CONTAINER(vbox), label);
  }

  sel_dlg.rtn_val = default_sel;

  sel_dlg.entry = gtk_combo_box_new_text();
  for (i = 0; i < nsel; i++) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(sel_dlg.entry), choices[i]);
  }

  if (default_sel < 0 || default_sel > nsel) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(sel_dlg.entry), 0);
  } else {
    gtk_combo_box_set_active(GTK_COMBO_BOX(sel_dlg.entry), default_sel);
  }
  gtk_container_add(GTK_CONTAINER(vbox), sel_dlg.entry);

  if (*tail) {
    label = gtk_label_new(head);
    gtk_container_add(GTK_CONTAINER(vbox), label);
  }
  button_ok = gtk_button_new_with_label("Okay");
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sel_dlg.w)->action_area),
		    button_ok);
  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(sel_dlg_cb), "Ok");
  if (default_sel < 0 || default_sel > nsel) {
    button_cancel = gtk_button_new_with_label("Cancel");
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sel_dlg.w)->action_area),
		      button_cancel);
    gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		       GTK_SIGNAL_FUNC(sel_dlg_cb), "Cancel");
  }

  gtk_signal_connect(GTK_OBJECT(sel_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_dialog), NULL);

  gtk_widget_show_all(sel_dlg.w);
  gtk_window_set_modal(GTK_WINDOW(sel_dlg.w), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(sel_dlg.w), GTK_WINDOW(toplevel)); /* set parent */
  move_win_to(sel_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(sel_dlg.w), button_ok);

  insensitive();
  while (!dlg_done) gtk_main_iteration();
  sensitive();
  if (dlg_done > -2) {
    gtk_widget_destroy(sel_dlg.w);
    while (gtk_events_pending()) gtk_main_iteration();
  }

  dlg_done = 0;
  if (sel_dlg.rtn_val < 0) {
    log_to_cmd_file("");
    return default_sel;
  }
  ans[0] = flags[sel_dlg.rtn_val];
  ans[1] = '\0';
  log_to_cmd_file(ans);
  return sel_dlg.rtn_val;
} /* ask_selection */

/* ==========  dialog for creating ps plot of level scheme  ========== */
int delete_ps_dlg(void)
{
  ps_dlg.done = -2;
  return FALSE;
} /* delete_ps_dlg */

void reset_ps_dlg(void)
{
  if (ps_dlg.titpos == 'b') {
    gtk_combo_box_set_active(GTK_COMBO_BOX(ps_dlg.entry[1]), 0);
  } else {
    gtk_combo_box_set_active(GTK_COMBO_BOX(ps_dlg.entry[1]), 1);
  }
  if (ps_dlg.scalepos == 'l') {
    gtk_combo_box_set_active(GTK_COMBO_BOX(ps_dlg.entry[5]), 0);
  } else {
    gtk_combo_box_set_active(GTK_COMBO_BOX(ps_dlg.entry[5]), 1);
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(ps_dlg.entry[7]), ps_dlg.font);

  if (ps_dlg.addtit) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[0]), TRUE);
    gtk_widget_set_sensitive(ps_dlg.entry[1], TRUE);
    gtk_widget_set_sensitive(ps_dlg.entry[2], TRUE);
    gtk_widget_set_sensitive(ps_dlg.entry[3], TRUE);
    gtk_widget_set_sensitive(label[1], TRUE);
    gtk_widget_set_sensitive(label[2], TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[0]), FALSE);
    gtk_widget_set_sensitive(ps_dlg.entry[1], FALSE);
    gtk_widget_set_sensitive(ps_dlg.entry[2], FALSE);
    gtk_widget_set_sensitive(ps_dlg.entry[3], FALSE);
    gtk_widget_set_sensitive(label[1], FALSE);
    gtk_widget_set_sensitive(label[2], FALSE);
  }
  if (ps_dlg.addyaxis) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[4]), TRUE);
    gtk_widget_set_sensitive(ps_dlg.entry[5], TRUE);
    gtk_widget_set_sensitive(ps_dlg.entry[6], TRUE);
    gtk_widget_set_sensitive(label[4], TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[4]), FALSE);
    gtk_widget_set_sensitive(ps_dlg.entry[5], FALSE);
    gtk_widget_set_sensitive(ps_dlg.entry[6], FALSE);
    gtk_widget_set_sensitive(label[4], FALSE);
  }
  if (ps_dlg.nologo) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[8]), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[8]), FALSE);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[9]), FALSE);
  gtk_widget_set_sensitive(ps_dlg.entry[10], FALSE);
  gtk_widget_set_sensitive(ps_dlg.entry[11], FALSE);
  gtk_widget_set_sensitive(ps_dlg.entry[12], TRUE);
  gtk_widget_set_sensitive(label[8], TRUE);

  gtk_entry_set_text(GTK_ENTRY(ps_dlg.entry[2]), ps_dlg.htitle);
  gtk_entry_set_text(GTK_ENTRY(ps_dlg.entry[3]), ps_dlg.title);
  gtk_entry_set_text(GTK_ENTRY(ps_dlg.entry[6]), ps_dlg.hyaxis);
  gtk_entry_set_text(GTK_ENTRY(ps_dlg.entry[10]), ps_dlg.filnam);
  gtk_entry_set_text(GTK_ENTRY(ps_dlg.entry[12]), ps_dlg.printcmd);
} /* reset_ps_dlg */

void ps_dlg_cb(GtkWidget *w, gpointer data)
{
  char  ans[80];

  if (!strcmp((char *)data, "Ok")) {
    ps_dlg.done = 1;
  } else if (!strcmp((char *)data, "Reset")) {
    reset_ps_dlg();
  } else if (!strcmp((char *)data, "Cancel")) {
    ps_dlg.done = -1;
  } else if (!strcmp((char *)data, "Browse")) {
    gtk_window_set_modal(GTK_WINDOW(ps_dlg.w), FALSE);
    if (askfn(ans, 80, ps_dlg.filnam, ".ps",
	      "Output postscript file = ?\n")) {
      strcpy(ps_dlg.filnam, ans);
      gtk_entry_set_text(GTK_ENTRY(ps_dlg.entry[10]), ps_dlg.filnam);
    }
    gtk_window_set_modal(GTK_WINDOW(ps_dlg.w), TRUE);
  } else if (!strcmp((char *)data, "tog0")) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[0]))) {
      gtk_widget_set_sensitive(ps_dlg.entry[1], TRUE);
      gtk_widget_set_sensitive(ps_dlg.entry[2], TRUE);
      gtk_widget_set_sensitive(ps_dlg.entry[3], TRUE);
      gtk_widget_set_sensitive(label[1], TRUE);
      gtk_widget_set_sensitive(label[2], TRUE);
    } else {
      gtk_widget_set_sensitive(ps_dlg.entry[1], FALSE);
      gtk_widget_set_sensitive(ps_dlg.entry[2], FALSE);
      gtk_widget_set_sensitive(ps_dlg.entry[3], FALSE);
      gtk_widget_set_sensitive(label[1], FALSE);
      gtk_widget_set_sensitive(label[2], FALSE);
    }
  } else if (!strcmp((char *)data, "tog4")) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[4]))) {
      gtk_widget_set_sensitive(ps_dlg.entry[5], TRUE);
      gtk_widget_set_sensitive(ps_dlg.entry[6], TRUE);
      gtk_widget_set_sensitive(label[4], TRUE);
    } else {
      gtk_widget_set_sensitive(ps_dlg.entry[5], FALSE);
      gtk_widget_set_sensitive(ps_dlg.entry[6], FALSE);
      gtk_widget_set_sensitive(label[4], FALSE);
    }
  } else if (!strcmp((char *)data, "tog9")) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[9]))) {
      gtk_widget_set_sensitive(ps_dlg.entry[10], TRUE);
      gtk_widget_set_sensitive(ps_dlg.entry[11], TRUE);
      gtk_widget_set_sensitive(ps_dlg.entry[12], FALSE);
      gtk_widget_set_sensitive(label[8], FALSE);
    } else {
      gtk_widget_set_sensitive(ps_dlg.entry[10], FALSE);
      gtk_widget_set_sensitive(ps_dlg.entry[11], FALSE);
      gtk_widget_set_sensitive(ps_dlg.entry[12], TRUE);
      gtk_widget_set_sensitive(label[8], TRUE);
    }
  }
} /* ps_dlg_cb */

int gls_get_ps_details(char *title, float *htitle, char *titpos,
		       float *hyaxis, char *scalepos,
		       char *filnam, char *printcmd)
{
  GtkWidget *button_ok, *button_reset, *button_cancel;
  GtkWidget *vbox, *hbox, *sep;

  float fj1, fj2;
  int   i, jext, ix;
  static int  first = 1;
  static char gls_fn_sav[80];
  static char *choices[5] =
    {"The Standard RadWare Font",
     "AvantGarde-Book",
     "Helvetica",
     "NewCenturySchlbk-Roman",
     "Times-Roman"};

  *title    = '\0';
  *scalepos = '\0';
  *filnam   = '\0';
  *printcmd = '\0';
  if (first) {
    first = 0;
    ps_dlg.font = 2;
    ps_dlg.nologo = ps_dlg.addtit = ps_dlg.addyaxis = 0;
    strcpy(gls_fn_sav, "");
    strcpy(ps_dlg.htitle, "200");
    strcpy(ps_dlg.hyaxis, "200");
    strcpy(ps_dlg.printcmd, "lpr");
    ps_dlg.titpos = 'b';
    ps_dlg.scalepos = 'l';
  }
  if (strcmp(gls_fn_sav, glsgd.gls_file_name)) {
    strncpy(gls_fn_sav, glsgd.gls_file_name, 80);
    strncpy(ps_dlg.filnam, glsgd.gls_file_name,80);
    jext = setext(ps_dlg.filnam, ".ps", 80);
    ps_dlg.filnam[jext] = '\0';
    jext = setext(ps_dlg.filnam, ".ps", 80);
    strcpy(ps_dlg.title, "From ");
    strncat(ps_dlg.title, glsgd.gls_file_name, 72);
  }

  ps_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ps_dlg.w)->vbox), vbox);
  set_title(ps_dlg.w, "Print GLS");

  ps_dlg.entry[0] = gtk_check_button_new();
  label[0] = gtk_label_new(" Add title ");
  ps_dlg.entry[1] = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(ps_dlg.entry[1]), "below level scheme");
  gtk_combo_box_append_text(GTK_COMBO_BOX(ps_dlg.entry[1]), "above level scheme");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), ps_dlg.entry[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label[0], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), ps_dlg.entry[1], FALSE, TRUE, 0);

  label[1] = gtk_label_new("Char. size: ");
  ps_dlg.entry[2] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(ps_dlg.entry[2]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), ps_dlg.entry[2], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[1], FALSE, TRUE, 0);

  label[2] = gtk_label_new("Title: ");
  ps_dlg.entry[3] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(ps_dlg.entry[3]), 80);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), ps_dlg.entry[3], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[2], FALSE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  ps_dlg.entry[4] = gtk_check_button_new();
  label[3] = gtk_label_new(" Add energy axis ");
  ps_dlg.entry[5] = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(ps_dlg.entry[5]), "to left of level scheme");
  gtk_combo_box_append_text(GTK_COMBO_BOX(ps_dlg.entry[5]), "to right of level scheme");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), ps_dlg.entry[4], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label[3], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), ps_dlg.entry[5], FALSE, TRUE, 0);

  label[4] = gtk_label_new("Char. size: ");
  ps_dlg.entry[6] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(ps_dlg.entry[6]), 8);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), ps_dlg.entry[6], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[4], FALSE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  label[5] = gtk_label_new(" Font: ");
  ps_dlg.entry[7] = gtk_combo_box_new_text();
  for (i=0; i<5; i++) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(ps_dlg.entry[7]), choices[i]);
  }
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), label[5], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), ps_dlg.entry[7], FALSE, TRUE, 0);

  ps_dlg.entry[8] = gtk_check_button_new();
  label[6] = gtk_label_new("Suppress logo and file info");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), ps_dlg.entry[8], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label[6], FALSE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_container_add(GTK_CONTAINER(vbox), sep);

  ps_dlg.entry[9] = gtk_check_button_new();
  label[7] = gtk_label_new("Print to file:");
  ps_dlg.entry[10] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(ps_dlg.entry[10]), 80);
  ps_dlg.entry[11] = gtk_button_new_with_label("Browse...");
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), ps_dlg.entry[9], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label[7], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), ps_dlg.entry[10], TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), ps_dlg.entry[11], FALSE, TRUE, 0);

  label[8] = gtk_label_new("Print command: ");
  ps_dlg.entry[12] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(ps_dlg.entry[12]), 80);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), ps_dlg.entry[12], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[8], FALSE, TRUE, 0);

  button_ok     = gtk_button_new_with_label("Okay");
  button_reset  = gtk_button_new_with_label("Reset");
  button_cancel = gtk_button_new_with_label("Cancel");

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ps_dlg.w)->action_area),
		    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ps_dlg.w)->action_area),
		    button_reset);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ps_dlg.w)->action_area),
		    button_cancel);

  gtk_signal_connect(GTK_OBJECT(ps_dlg.entry[0]), "clicked",
		     GTK_SIGNAL_FUNC(ps_dlg_cb), (gpointer) "tog0");
  gtk_signal_connect(GTK_OBJECT(ps_dlg.entry[4]), "clicked",
		     GTK_SIGNAL_FUNC(ps_dlg_cb), (gpointer) "tog4");
  gtk_signal_connect(GTK_OBJECT(ps_dlg.entry[9]), "clicked",
		     GTK_SIGNAL_FUNC(ps_dlg_cb), (gpointer) "tog9");
  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(ps_dlg_cb), (gpointer) "Ok");
  gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		     GTK_SIGNAL_FUNC(ps_dlg_cb), (gpointer) "Reset");
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(ps_dlg_cb), (gpointer) "Cancel");
  gtk_signal_connect(GTK_OBJECT(ps_dlg.entry[11]), "clicked",
		     GTK_SIGNAL_FUNC(ps_dlg_cb), (gpointer) "Browse");
  gtk_signal_connect(GTK_OBJECT(ps_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_ps_dlg), NULL);

  /* Display the dialog */
  gtk_widget_show_all(ps_dlg.w);
  ix = ps_dlg.entry[2]->allocation.width;
  gtk_widget_set_usize(ps_dlg.entry[1], ix, 0);
  gtk_widget_set_usize(ps_dlg.entry[5], ix, 0);
  gtk_widget_set_usize(ps_dlg.entry[7], ix*13/10, 0);

  gtk_window_set_modal(GTK_WINDOW(ps_dlg.w), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(ps_dlg.w), GTK_WINDOW(toplevel)); //set parent
  move_win_to(ps_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(ps_dlg.w), button_ok);
  reset_ps_dlg();

  ps_dlg.done = 0;
  insensitive();
  while (!ps_dlg.done) gtk_main_iteration();
  sensitive();

  /* if widget has been deleted, return (1) immediately */
  if (ps_dlg.done < -1) return 1;

  /* if cancelled, destroy dialog and return (1) */
  if (ps_dlg.done < 0) {
    gtk_widget_destroy(ps_dlg.w);
    while (gtk_events_pending()) gtk_main_iteration();
    return 1;
  }

  if (gtk_combo_box_get_active(GTK_COMBO_BOX(ps_dlg.entry[1])) == 0) {
    ps_dlg.titpos = 'b';
  } else {
    ps_dlg.titpos = 't';
  }
  if (gtk_combo_box_get_active(GTK_COMBO_BOX(ps_dlg.entry[5])) == 0) {
    ps_dlg.scalepos = 'l';
  } else {
    ps_dlg.scalepos = 'r';
  }
  ps_dlg.font = gtk_combo_box_get_active(GTK_COMBO_BOX(ps_dlg.entry[7]));
  strncpy(ps_dlg.htitle, gtk_entry_get_text(GTK_ENTRY(ps_dlg.entry[2])), 20);
  strncpy(ps_dlg.title, gtk_entry_get_text(GTK_ENTRY(ps_dlg.entry[3])), 80);
  strncpy(ps_dlg.hyaxis, gtk_entry_get_text(GTK_ENTRY(ps_dlg.entry[6])), 20);
  strncpy(ps_dlg.filnam, gtk_entry_get_text(GTK_ENTRY(ps_dlg.entry[10])), 80);
  strncpy(ps_dlg.printcmd, gtk_entry_get_text(GTK_ENTRY(ps_dlg.entry[12])), 80);

  if ((ps_dlg.addtit =
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[0])))) {
    strncpy(title, ps_dlg.title, 80);
    *titpos = ps_dlg.titpos;
    if (ffin(ps_dlg.htitle, strlen(ps_dlg.htitle), htitle, &fj1, &fj2))
      *htitle = 200.0f;
  }
  if ((ps_dlg.addyaxis =
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[4])))) {
    *scalepos = ps_dlg.scalepos;
    if (ffin(ps_dlg.hyaxis, strlen(ps_dlg.hyaxis), hyaxis, &fj1, &fj2))
      *hyaxis = 200.0f;
  }
  ps_dlg.nologo =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[8]));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ps_dlg.entry[9]))) {
    strncpy(filnam, ps_dlg.filnam, 80);
  } else {
    strncpy(printcmd, ps_dlg.printcmd, 80);
  }

  /* printf("%s %f %c\n%f %c %d\n%s + %s\n", title, *htitle, *titpos,
   *hyaxis, *scalepos, *suppress_info, filnam, printcmd); */
  /* printf("font: %s\n", choices[ps_dlg.font]); */

  gtk_widget_destroy(ps_dlg.w);
  while (gtk_events_pending()) gtk_main_iteration();
  return 0;
} /* gls_get_ps_details */

/* ======================================================================= */
int delete_newgls_dlg(void)
{
  newgls_dlg.done = -2;
  return FALSE;
} /* delete_newgls_dlg */

void reset_newgls_dlg(void)
{
  gtk_entry_set_text(GTK_ENTRY(newgls_dlg.entry[0]), "67");
  gtk_entry_set_text(GTK_ENTRY(newgls_dlg.entry[1]), "0.5");
  gtk_combo_box_set_active(GTK_COMBO_BOX(newgls_dlg.entry[2]), 0);
  gtk_entry_set_text(GTK_ENTRY(newgls_dlg.entry[3]), "gb");
} /* reset_newgls_dlg */

void newgls_dlg_cb(GtkWidget *w, gpointer data)
{
  if (!strcmp((char *)data, "Ok")) {
    newgls_dlg.done = 1;
  } else if (!strcmp((char *)data, "Reset")) {
    reset_newgls_dlg();
  } else if (!strcmp((char *)data, "Cancel")) {
    newgls_dlg.done = -1;
  }
} /* newgls_dlg_cb */

int gls_get_newgls_details(int *z, float *spin, int *parity, char *name)
{
  GtkWidget *button_ok, *button_reset, *button_cancel;
  GtkWidget *vbox, *hbox;

  float fj1, fj2;
  int   i, nc, j1, j2;
  char  ans[80];

  newgls_dlg.w = gtk_dialog_new();
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(newgls_dlg.w)->vbox), vbox);
  set_title(newgls_dlg.w, "Make new GLS");

  label[0] = gtk_label_new(" Atomic number (Z): ");
  newgls_dlg.entry[0] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(newgls_dlg.entry[0]), 3);

  label[1] = gtk_label_new(" Ground state spin: ");
  newgls_dlg.entry[1] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(newgls_dlg.entry[1]), 4);

  label[2] = gtk_label_new(" Ground state parity: ");
  newgls_dlg.entry[2] = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(newgls_dlg.entry[2]), "Even");
  gtk_combo_box_append_text(GTK_COMBO_BOX(newgls_dlg.entry[2]), "Odd");
  gtk_combo_box_set_active(GTK_COMBO_BOX(newgls_dlg.entry[2]), 0);

  label[3] = gtk_label_new(" Ground state band name: ");
  newgls_dlg.entry[3] = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(newgls_dlg.entry[3]), 8);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), newgls_dlg.entry[0], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[0], FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), newgls_dlg.entry[1], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[1], FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), newgls_dlg.entry[2], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[2], FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), newgls_dlg.entry[3], FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), label[3], FALSE, TRUE, 0);

  button_ok     = gtk_button_new_with_label("Okay");
  button_reset  = gtk_button_new_with_label("Reset");
  button_cancel = gtk_button_new_with_label("Cancel");

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(newgls_dlg.w)->action_area),
		    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(newgls_dlg.w)->action_area),
		    button_reset);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(newgls_dlg.w)->action_area),
		    button_cancel);
  gtk_widget_set_sensitive(button_cancel, FALSE);

  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(newgls_dlg_cb), (gpointer) "Ok");
  gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		     GTK_SIGNAL_FUNC(newgls_dlg_cb), (gpointer) "Reset");
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(newgls_dlg_cb), (gpointer) "Cancel");
  gtk_signal_connect(GTK_OBJECT(newgls_dlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(delete_newgls_dlg), NULL);

  /* Display the dialog */
  for (i=0; i<4; i++) {
    gtk_widget_set_usize(newgls_dlg.entry[i], 90, 0);
  }
  gtk_widget_show_all(newgls_dlg.w);
  gtk_window_set_modal(GTK_WINDOW(newgls_dlg.w), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(newgls_dlg.w), GTK_WINDOW(toplevel)); //set parent
  move_win_to(newgls_dlg.w, -1, -1);
  gtk_window_set_focus(GTK_WINDOW(newgls_dlg.w), newgls_dlg.entry[0]);
  reset_newgls_dlg();

  while (1) {
    newgls_dlg.done = 0;
    while (!newgls_dlg.done) gtk_main_iteration();

    /* if widget has been deleted, return (1) immediately */
    if (newgls_dlg.done < -1) return 1;

    /* if cancelled, destroy dialog and return (1) */
    if (newgls_dlg.done < 0) {
      gtk_widget_destroy(newgls_dlg.w);
      while (gtk_events_pending()) gtk_main_iteration();
      return 1;
    }

    strcpy(ans, gtk_entry_get_text(GTK_ENTRY(newgls_dlg.entry[0])));
    if (inin(ans, strlen(ans), z, &j1, &j2) || *z < 3 || *z > 110) {
      warn("Error in Z value; must be between 3 and 110\n");
      continue;
    }
    strcpy(ans, gtk_entry_get_text(GTK_ENTRY(newgls_dlg.entry[1])));
    if (ffin(ans, strlen(ans), spin, &fj1, &fj2) || *spin < 0.0f) {
      warn("Error in spin value; must be >= 0\n");
      continue;
    }
    *spin = ((float) ((int) (*spin*2.0f + 0.5f))) * 0.5f;
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(newgls_dlg.entry[2])) == 0) {
      *parity = 1;
    } else {
      *parity = -1;
    }
    strcpy(ans, gtk_entry_get_text(GTK_ENTRY(newgls_dlg.entry[3])));
    nc = strlen(ans);
    if (nc > 8) nc = 8;
    strncpy(name, "        ", 8);
    strncpy(name + 8 - nc, ans, nc);
    break;
  }
  /* printf("%d %f %d %.8s\n", *z, *spin, *parity, name); */

  gtk_widget_destroy(newgls_dlg.w);
  while (gtk_events_pending()) gtk_main_iteration();
  return 0;
} /* gls_get_newgls_details */

/* ======================================================================= */
void undo_gls_notify(int flag)
{
  /* flag = 1: undo_edits now available
            2: redo_edits now available
            3: undo_edits back at flagged position
           -1: undo_edits no longer available
           -2: redo_edits no longer available
           -3: undo_edits no longer at flagged position */
  /* printf("undo_gls_notify: %d\n", flag); */
  if        (flag ==  1) {
    gtk_widget_set_sensitive(undo_button, TRUE);
    gtk_widget_set_sensitive(undo_menu_item, TRUE);
  } else if (flag == -1) {
    gtk_widget_set_sensitive(undo_button, FALSE);
    gtk_widget_set_sensitive(undo_menu_item, FALSE);
  } else if (flag ==  2) {
    gtk_widget_set_sensitive(redo_button, TRUE);
    gtk_widget_set_sensitive(redo_menu_item, TRUE);
  } else if (flag == -2) {
    gtk_widget_set_sensitive(redo_button, FALSE);
    gtk_widget_set_sensitive(redo_menu_item, FALSE);
  } else if (flag ==  3) {
    gtk_widget_set_sensitive(save_button, FALSE);
    gtk_widget_set_sensitive(save_menu_item, FALSE);
  } else if (flag == -3) {
    gtk_widget_set_sensitive(save_button, TRUE);
    gtk_widget_set_sensitive(save_menu_item, TRUE);
  }
}
