#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "minig.h"

struct {
  float x1, y1;  
} dsgd;
float ds_lwfact = 0.0f;

/* ======================================================================= */
int stroke(float x, float y, char move_flag)
{
  extern int vect(float, float);
  extern int pspot(float, float);

  /* move to coordinates X,Y if MOVE_FLAG is true
     draw from current position to coordinates X,Y
     if MOVE_FLAG is false */
  if (move_flag) {
    pspot(x, y);
  } else {
    vect(x, y);
  }
  return 0;
} /* stroke */

/* ======================================================================= */
int ds_set_lwfact(float f)
{
  ds_lwfact = f;
  return 0;
} /* ds_moveto */

/* ======================================================================= */
int ds_moveto(float x, float y)
{
  dsgd.x1 = x;
  dsgd.y1 = y;
  return 0;
} /* ds_moveto */

/* ======================================================================= */
int ds_rmoveto(float x, float y)
{
  dsgd.x1 += x;
  dsgd.y1 += y;
  return 0;
} /* ds_rmoveto */

/* ======================================================================= */
int drawstring(char *ostring, int ncs, char orient, 
	       float x, float y, float csx, float csy)
{
  /* draw character string OSTRING at coordinates X,Y
     ostring may be up to 256 characters
     ncs     = # of characters in ostring
     orient  = 'h'/'H' for horizontal orientation, centered at (x,y)
	       'c'/'C' - same as 'h'/'H'; centered horizontal text
               'v'/'V' for vertical orientation, centered at (x,y)
	       'l'/'L' for horizontal text left-justified at (x,y)
	               rather than centered there
	       'r'/'R' for horizontal text right-justified at (x,y)
	       'x'/'X' to continue text from current location; (x,y) ignored
     csx,csy = character width and height respectively
     commands for:
        superscr subscr italic symbol greek  overstrike bold
	{u...}   {d...} {i...} {s...} {g...} {o...}     {b...} */

  extern FILE *open_readonly(char *);
  extern int get_directory(char *, char *, int);
  extern void warn(const char *fmt, ...);

  static int   first = 1;
  static char  ccom[] = "udisgobxUDISGOBX";
  static float x_font[3000], y_font[3000], dx_font[300];
  static int   start_font[300], end_font[300];
  static char  move_flag[3000];

  float x2, y2, x_length, wx, wy, dx_chars[256];
  int   i1, jcom, ncom, icom[10], command[10];
  int   c_italics[256], c_subscr[256], c_superscr[256], c_bold[256];
  int   str_chars[256], c_symbol[256];
  int   i, j, k, jchar=0, ix, iy, ixd, istroke=0, n_str_chars;
  char  line[256], fullname[200];
  FILE *file;

  if (ncs <= 0) return 0;

  if (first) {
    first = 0;
    /* open font file and read character look-up tables and fonts*/
    get_directory("RADWARE_FONT_LOC", fullname, 200);
    strcat(fullname, "font.dat");
    if (!(file = open_readonly(fullname))) {
      warn(" -- Check that the environment variable or logical name\n"
	   "     RADWARE_FONT_LOC is properly defined, and that the file\n"
	   "     font.dat exists in the directory to which it points.\n");
      exit(-1);
    }
    for (i = 0; i < 300; ++i) {
      start_font[i] = 1;
      end_font[i]   = 0;
      dx_font[i]    = 0.5f;
    }
    if (!fgets(line, 80, file)) goto DONE;
    for (k = 1; k <= 300; ++k) {
      /* read and decode character to be drawn
	 second character indicates font type */
      jchar = line[0] - 32;
      if (line[1] == 's' || line[1] == 'S') {
	jchar += 100;
      } else if (line[1] == 'g' || line[1] == 'G') {
	jchar += 200;
      }
      start_font[jchar] = istroke;
      sscanf(line+2, "%5i", &ixd);
      dx_font[jchar] = (float) ixd / 1e3f;

      /* read and decode strokes used to draw character */
      if (!fgets(line, 80, file)) goto DONE;
      while (line[0] == ' ') {
	j = 0;
	i1 = strlen(line)/12; /* number of strokes on line
				 (12 chars per stroke) */
	for (i = 0; i < i1; ++i) {
	  sscanf(line+j, "%5i%5i", &ix, &iy);
	  x_font[istroke] = (float) ix / 1e3f;
	  y_font[istroke] = (float) iy / 1e3f;
	  if (line[j+11] == 'd') {
	    move_flag[istroke++] = 0;
	  } else {
	    move_flag[istroke++] = 1;
	  }
	  j += 12;
	}
	if (!fgets(line, 80, file)) goto DONE;
      }
      end_font[jchar] = istroke - 1;
    }
  DONE:
    fclose(file);
    end_font[jchar] = istroke - 1;
    for (i = 0; i < 100; ++i) {
      i1 = i + 200;
      for (j = i + 100; j <= i+200; j += 100) {
	if (end_font[j] == 0) {
	  start_font[j] = start_font[i];
	  end_font[j]   = end_font[i];
	  dx_font[j]    = dx_font[i];
	}
      }
    }
  }

  wx = csx * 1.2f;
  wy = csy;
  ncom = 0;
  n_str_chars = 0;
  for (i = 0; i < 8; ++i) {
    command[i] = 0;
  }

  /* read through input string and look for imbedded commands
     copy characters to be output to STR_CHARS 
     n_str_chars = number of characters to be output */
  for (k = 0; k < ncs; ++k) {
    if (k > 0 && ncom > 0 &&
	ostring[k-1] == '{' && icom[ncom-1] != 0) continue;

    if (ostring[k] == '{') {
      /* look for superscript/subscript/italics commands etc
	 allow up to 10 nested commands */
      if (ncom < 10) ncom++;
      icom[ncom-1] = 0;
      for (jcom = 0; jcom < 8; ++jcom) {
	if (ostring[k+1] == ccom[jcom] ||
	    ostring[k+1] == ccom[jcom+8]) {
	  icom[ncom-1] = jcom + 1;
	  command[jcom] = 1;
	  break;
	}
      }
      continue;
    } else if (ncom > 0 && ostring[k] == '}') {
      /* forget last command */
      ncom--;
      if (icom[ncom] != 0) command[icom[ncom] - 1] = 0;
      continue;
    }

    /* character is not a command; copy to output string */
    jchar = ostring[k] - 32;
    if (jchar < 0 || jchar > 94) continue;
    if (command[3]) {
      jchar += 100;
    } else if (command[4]) {
      jchar += 200;
    }
    str_chars[n_str_chars] = jchar;
    dx_chars[n_str_chars] = dx_font[jchar] * wx;
    if (!command[2] && n_str_chars > 1 &&
	c_italics[n_str_chars - 1]) dx_chars[n_str_chars - 1] += wx * 0.2f;
    if (command[7]) dx_chars[n_str_chars] = wx;
    if (command[0]) dx_chars[n_str_chars] *= 0.7f;
    if (command[1]) dx_chars[n_str_chars] *= 0.7f;
    if (command[6]) dx_chars[n_str_chars] *= 1.05f;
    if (command[5] && n_str_chars > 1)
      dx_chars[n_str_chars - 2] = (dx_chars[n_str_chars - 1] - 
				   dx_chars[n_str_chars]) / 2.0f;
    c_superscr[n_str_chars]  = command[0];
    c_subscr[n_str_chars]    = command[1];
    c_italics[n_str_chars] = command[2];
    c_symbol[n_str_chars] = command[7];
    c_bold[n_str_chars++] = command[6];
  }

  /* calculate total length of label */
  x_length = 0.0f;
  for (i = 0; i < n_str_chars; ++i) {
    x_length += dx_chars[i];
  }

  /* output label */
  if (orient == 'V' || orient == 'v') {  /* vertical */
    dsgd.x1 = x;
    dsgd.y1 = y - x_length / 2.f;
    for (i = 0; i < n_str_chars; ++i) {
      wx = csx * 1.2f;
      wy = csy;
      if (c_superscr[i]) {
	wx *= 0.7f;
	wy *= 0.7f;
      }
      if (c_subscr[i]) {
	wx *= 0.7f;
	wy *= 0.7f;
      }
      if (c_bold[i]) {
	set_minig_line_width((int) (wy * 0.15f * ds_lwfact + 0.5f));
      } else {
	set_minig_line_width((int) (wy * 0.09f * ds_lwfact + 0.5f));
      }
      if (c_symbol[i]) {
	j = str_chars[i] - 16;
	if (j >= 1 && j <= 13) 
	  symbg(j, dsgd.x1 + 0.5f*wy, dsgd.y1 + 0.5f*wx,
		(int) (0.6f*wx*ds_lwfact));
      } else {
	jchar = str_chars[i];
	for (j = start_font[jchar]; j <= end_font[jchar]; ++j) {
	  x2 = dsgd.x1 - y_font[j] * wy;
	  y2 = dsgd.y1 + x_font[j] * wx;
	  if (c_superscr[i]) x2 -= wy * .8f;
	  if (c_subscr[i]) x2 += wy * .8f;
	  if (c_italics[i]) y2 += y_font[j] * .2f * wx;
	  stroke(x2, y2, move_flag[j]);
	}
      }
      dsgd.y1 += dx_chars[i];
    }

  } else {    /* horizontal */
    if (orient == 'H' || orient == 'h' || orient == 'C' || orient == 'c') {
      dsgd.x1 = x - x_length / 2.0f;
      dsgd.y1 = y;
    } else if (orient == 'L' || orient == 'l') {
      dsgd.x1 = x;
      dsgd.y1 = y;
    } else if (orient == 'R' || orient == 'r') {
      dsgd.x1 = x - x_length;
      dsgd.y1 = y;
    }

    for (i = 0; i < n_str_chars; ++i) {
      wx = csx * 1.2f;
      wy = csy;
      if (c_superscr[i]) {
	wx *= 0.7f;
	wy *= 0.7f;
      }
      if (c_subscr[i]) {
	wx *= 0.7f;
	wy *= 0.7f;
      }
      if (c_bold[i]) {
	set_minig_line_width((int) (wy * 0.15f * ds_lwfact + 0.5f));
      } else {
	set_minig_line_width((int) (wy * 0.09f * ds_lwfact + 0.5f));
      }
      if (c_symbol[i]) {
	j = str_chars[i] - 16;
	if (j >= 1 && j <= 13) 
	  symbg(j, dsgd.x1 + 0.5f*wx, dsgd.y1 + 0.5f*wy,
		(int) (0.6f*wx*ds_lwfact));
      } else {
	jchar = str_chars[i];
	for (j = start_font[jchar]; j <= end_font[jchar]; ++j) {
	  x2 = dsgd.x1 + x_font[j] * wx;
	  y2 = dsgd.y1 + y_font[j] * wy;
	  if (c_superscr[i]) y2 += wy * 0.8f;
	  if (c_subscr[i]) y2 -= wy * 0.8f;
	  if (c_italics[i]) x2 += y_font[j] * 0.2f * wx;
	  stroke(x2, y2, move_flag[j]);
	}
      }
      dsgd.x1 += dx_chars[i];
    }
  }

  return 0;
} /* drawstring */
