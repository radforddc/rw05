/* gtk+ version of minig (X11) subroutines
   version 0.0   D.C. Radford    July 2003 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>

#include "minig.h"

extern GdkPixmap   *pixmap;
extern GdkWindow   *win_id;
extern GtkWidget   *toplevel, *drawing_area;
extern int         InReticMode;       /* used to ignore unwanted button presses etc */
extern Display     *disp_id;
extern Screen      *screen_id;
extern GdkGC       *xmgc, *gc_id, *gc_comp_id;
extern GdkColormap *colormap;
extern GdkColor    color_id[20];
extern GdkFont     *font_id;
extern GdkPoint    points[512];
extern int         nstored;
extern int   win_width, win_height, ixp, iyp, ixb, iyb, ibnum;
extern float xwg[8];
extern int   hclog;
extern FILE  *hcscrfile;

#define FONTNAME "-ADOBE-HELVETICA-MEDIUM-R-NORMAL--*-100-*-*-P-*"
#define WINDNAME "Graphics Window"

extern int main(int argc, char *argv[]);
char   argc_copy, **argv_copy, progname_copy[256];
char   cin[40];
int    win_origin_x, win_origin_y, win_w, win_h;

/* ======================================================================= */
void quit_mingtk(void)
{
  save_xwg(progname_copy);
  gtk_exit(0);
} /* quit_mingtk */

static gint configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
  int redraw(void);

  gdk_window_get_root_origin(toplevel->window, &win_origin_x, &win_origin_y);
  win_w = drawing_area->allocation.width;
  win_h = drawing_area->allocation.height;
  if (pixmap && xmgc &&
      (win_width != win_w || win_height != win_h)) {
    redraw();
    win_width = win_w;
    win_height = win_h;
  }
  /* if (pixmap && xmgc) gdk_draw_rectangle (pixmap, xmgc, TRUE,
     0, 0, drawing_area->allocation.width, drawing_area->allocation.height); */
  return TRUE;
} /* configure_event */

/* Redraw the screen from the backing pixmap */
static gint expose_event( GtkWidget      *widget,
                          GdkEventExpose *event )
{
  static int first = 1;
  if (pixmap && xmgc) gdk_draw_pixmap(widget->window, xmgc, pixmap,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);

  if (!first) return FALSE;
  first = 0;
  main(argc_copy, argv_copy);
  return FALSE;
} /* expose_event */

static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
  ibnum = event->button;
  ixb = event->x;
  iyb = event->y;
  return TRUE;
} /* button_press_event */

static gint key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer cb_data)
{
  unsigned int state;
  char         response[40];

  if ((event->state & GDK_CONTROL_MASK) ||
      (event->state & GDK_MOD1_MASK) ||
      (event->state & GDK_MOD2_MASK) ||
      (event->state & GDK_MOD3_MASK) ||
      (event->state & GDK_MOD4_MASK) ||
      (event->state & GDK_MOD5_MASK)) return TRUE;

  gdk_window_get_pointer(drawing_area->window, &ixb, &iyb, &state);
  if (ixb < 0 || iyb < 0) return TRUE;
  /* if (ixb > win_width || iyb > win_height) return TRUE; */

  strcpy(response, event->string);
  if (response[0] > 31) strncpy(cin, response, 3);
  return TRUE;
} /* key_press_event */

static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
  GdkModifierType state;

  if (InReticMode) {
    gdk_draw_line(pixmap, gc_comp_id, ixp, 0, ixp, win_height);
    gdk_draw_line(pixmap, gc_comp_id, 0, iyp, win_width, iyp);
  }
  if (event->is_hint) {
    gdk_window_get_pointer(event->window, &ixp, &iyp, &state);
  } else {
    ixp = event->x;
    iyp = event->y;
  }
  if (InReticMode) {
    gdk_draw_line(pixmap, gc_comp_id, ixp, 0, ixp, win_height);
    gdk_draw_line(pixmap, gc_comp_id, 0, iyp, win_width, iyp);
    gdk_draw_pixmap(drawing_area->window, xmgc, pixmap,
		    0, 0, 0, 0, win_width, win_height);
  }
  return TRUE;
} /* motion_notify_event */

/* ======================================================================= */
int mingtk_init(int argc, char *argv[], char *progname)
{
  static int       first = 1;
  static GtkWidget *vbox;
  int              ix, iy, scr_width, scr_height;
  unsigned int     w, h;

  if (!first) return 1;
  first = 0;

  printf("calling gtk_init...\n");  fflush(stdout);
  gtk_init(&argc, &argv);
  set_xwg(progname);
  strcpy(progname_copy, progname);
  argc_copy = argc;
  argv_copy = argv;

  /* open scratch file for storage of data for possible hardcopy */
  hclog = 1;
  if (!(hcscrfile = tmpfile())) {
    printf("ERROR - cannot open temp file!\n");
    hclog = 0;
  }
  /* create new graphics window on first entry
     Initialize display id and screen id */
  printf("opening display...\n");  fflush(stdout);
  disp_id = XOpenDisplay(0);
  if (!disp_id) {
    printf("Display not opened!\n");
    exit(-1);
  }  
  /* Initialize some global variables */
  screen_id =   XDefaultScreenOfDisplay(disp_id);
  scr_width =   XWidthOfScreen(screen_id);
  scr_height =  XHeightOfScreen(screen_id);

  if (xwg[2] == 0.0) {
    xwg[0] = 0.03;
    xwg[1] = 0.33;
    xwg[2] = 0.95;
    xwg[3] = 0.64;
  }
  if (xwg[0] + xwg[2] > 1.0) xwg[0] = 1.0 - xwg[2];
  if (xwg[1] + xwg[3] > 1.0) xwg[1] = 1.0 - xwg[3];
  if (xwg[0] < 0.00) xwg[0] = 0.0;
  if (xwg[1] < 0.00) xwg[1] = 0.0;
  ix = xwg[0]*(float)scr_width + 0.5f;
  iy = xwg[1]*(float)scr_height + 0.5f;
  w  = xwg[2]*(float)scr_width + 0.5f;
  h  = xwg[3]*(float)scr_height + 0.5f;

  /* Create the top level window and vertical container box */
  printf("creating window...\n");  fflush(stdout);
  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name(toplevel, WINDNAME);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(toplevel), vbox);
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
		     GTK_SIGNAL_FUNC(quit_mingtk), NULL);

  /* Create the drawing area */
  drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), w, h);
  gtk_window_set_policy(GTK_WINDOW(toplevel), TRUE, TRUE, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);

  /* Signals used to handle backing pixmap */
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     (GtkSignalFunc) expose_event, NULL);
  gtk_signal_connect(GTK_OBJECT(toplevel),"configure_event",
		     (GtkSignalFunc) configure_event, NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area),"configure_event",
		     (GtkSignalFunc) configure_event, NULL);

  /* Event signals */
  gtk_signal_connect(GTK_OBJECT(drawing_area), "motion_notify_event",
		     (GtkSignalFunc) motion_notify_event, NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "button_press_event",
		     (GtkSignalFunc) button_press_event, NULL);
  gtk_signal_connect(GTK_OBJECT(toplevel), "key_press_event",
		     (GtkSignalFunc) key_press_event, NULL);
  gtk_widget_set_events(drawing_area, GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_KEY_PRESS_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);

  gtk_widget_show(drawing_area);
  gtk_widget_show(vbox);
  gtk_widget_show(toplevel);
  gdk_window_move(toplevel->window, ix, iy);
  pixmap = gdk_pixmap_new(toplevel->window, scr_width, scr_height, -1);

  win_id = pixmap;

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

  /* Create graphics contexts */
  xmgc       = drawing_area->style->white_gc;
  gc_id      = gdk_gc_new(drawing_area->window);
  gc_comp_id = gdk_gc_new(drawing_area->window);
  gdk_gc_copy(gc_id, xmgc);
  gdk_gc_copy(gc_comp_id, xmgc);
  gdk_gc_set_foreground(xmgc, &color_id[1]);
  gdk_gc_set_foreground(gc_id, &color_id[2]);
  gdk_gc_set_background(gc_id, &color_id[1]);
  gdk_gc_set_foreground(gc_comp_id, &color_id[2]);
  /* set GC for complement drawing */
  gdk_gc_set_function  (gc_comp_id, GDK_INVERT);

  /* Load the font for text writing */
  font_id = gdk_font_load(FONTNAME);
  /* gdk_gc_set_font(gc_id, font_id); */

  printf("calling gtk_main!\n"); fflush(stdout);
  gtk_main();

  gtk_exit(0);

  return 0;
} /* mingtk_init */

/* ======================================================================= */
int initg(int *nx, int *ny)
{
  win_width  = drawing_area->allocation.width;
  win_height = drawing_area->allocation.height;
  *nx = win_width  - 5;
  *ny = win_height - 25;
  return 0;
} /* initg */

/* ======================================================================= */
int finig(void)
{
  /* empty buffer of stored x,y values */
  if (nstored >= 2) {
    gdk_draw_lines(pixmap, gc_id, points, nstored);
    nstored = 0;
  }
  XFlush(disp_id);
  gdk_draw_pixmap(drawing_area->window, xmgc, pixmap,
		  0, 0, 0, 0, win_width, win_height);
  return 0;
} /* finig */

/* ======================================================================= */
int set_xwg(char *in)
{
  FILE   *fp;
  char   filename[80], line[120];

  xwg[2] = 0;
  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xwg");
#else
  strcat (filename, "/.radware.xwg");
#endif
  if (!(fp = fopen(filename, "r+"))) return 1;
  while (fgets(line, 120, fp)) {
    if (!strncmp(line, in, 8)) {
      sscanf((char *) strchr(line,' '), " %f%f%f%f%f%f%f%f",
	     &xwg[0], &xwg[1], &xwg[2], &xwg[3],
	     &xwg[4], &xwg[5], &xwg[6], &xwg[7]);
    }
  }
  fclose(fp);
  return 0;
} /* set_xwg */

/* ======================================================================= */
int save_xwg(char *in)
{
  FILE         *fp;
  char         filename[80], line[20][120];
  int          lineNum = 0, done = 0, i;
  int          scr_width, scr_height;

  strcpy (filename, (char *) getenv("HOME"));
#ifdef VMS
  strcat (filename, "radware.xwg");
#else
  strcat (filename, "/.radware.xwg");
#endif
  if (!(fp = fopen(filename, "a+"))) {
    printf("Cannot open file %s\n", filename);
    return 1;
  }
  rewind(fp);

  scr_width =  XWidthOfScreen(screen_id);
  scr_height = XHeightOfScreen(screen_id);
  xwg[0] = (float)win_origin_x/(float)scr_width;
  xwg[1] = (float)win_origin_y/(float)scr_height;
  xwg[2] = (float)win_w/(float)scr_width;
  xwg[3] = (float)win_h/(float)scr_height;

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
    if (!strncmp(line[i],in,8)) {
      done = 1;
      if (line[i][8] == ' ')
	sprintf(&line[i][8], " %7.5f %7.5f %7.5f %7.5f\n",
		xwg[0], xwg[1], xwg[2], xwg[3]);
    }
    fprintf(fp, "%s", line[i]);
  }
  if (!done) {
    strncpy(line[0], in, 8);
    line[0][8] = '\0';
    fprintf(fp, "%s %7.5f %7.5f %7.5f %7.5f\n",
	    line[0], xwg[0], xwg[1], xwg[2], xwg[3]);
  }
  fclose(fp);
  return 0;
} /* save_xwg */

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

  /* log info to temp file for possible later hardcopy */
  if (hclog) {
    fwrite("c", 1, 1, hcscrfile);
    fwrite(&icol, 4, 1, hcscrfile);
  }
  return 0;
} /* setcolor */
