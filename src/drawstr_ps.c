#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "util.h"

int choose_font(int);
int closeps(void);
int connpt(float, float);
int drawstr_ps(char *, int, char, float, float, float, float);
int moveto(float, float);
int openps(char *, char *, char *, float, float, float, float);
int rmoveto(float, float);
int setrgbcolor(float, float, float);
int setdash(float, float);
int setgray(float);
int setlinewidth(float);
int spec2ps(float *, int, float, float);
int strtpt(float, float);
int stroke_ps(float, float, char);
int symbgps(int, float, float, float);
int wfill(void);
int wstroke(void);
int w_ps_file(int, int, char *, int);

#define LOGO \
"%% radware logo and file info: -------------\n"\
"gsave %d 17 translate gsave 0.7 setlinewidth 0.7 setgray 0.5 0.5 scale\n"\
" 9 30 m 1050 0 translate 25 30 d str 0.95 0.7 0.7 setrgbcolor\n"\
" 2 30 m 2 13 d 0 13 d 9 5 d 18 13 d 16 13 d 16 30 d cstr\n"\
" 26 30 m 44 4 d 42 3 d 53 0 d 54 10 d 52 9 d 38 30 d cstr\n"\
" 36 30 m 53.4 4 d str\n"\
"/AvantGarde-DemiOblique findfont 17 scalefont setfont\n"\
"0.4 0.6 0.8 setrgbcolor -10 15 moveto (Rad) show 23 10 moveto (Ware) show\n"\
"grestore 0.3 setgray /Helvetica findfont 6 scalefont setfont 220 7 moveto\n"\
"(%s, created %s) show grestore\n"\
"%% -------------\n"

struct {
  float sx[100], sy[100];
  int   font_num, npts;
  float x1, y1;  
} dspsgd;
int   large_scale = 0, encapps = 0;
FILE  *psfile;

/* ======================================================================= */
int choose_font(int in_font)
{
  /* choose font to use for drawstring_ps */
  /* in_font = font number to use, or -1 to prompt for choice. */

  static char *psfonts[4][4] = {
    {"AvantGarde-Book","AvantGarde-BookOblique",
     "AvantGarde-Demi","AvantGarde-DemiOblique"},
    {"Helvetica","Helvetica-Oblique",
     "Helvetica-Bold","Helvetica-BoldOblique"},
    {"NewCenturySchlbk-Roman","NewCenturySchlbk-Italic",
     "NewCenturySchlbk-Bold","NewCenturySchlbk-BoldItalic"},
    {"Times-Roman","Times-Italic",
     "Times-Bold","Times-BoldItalic"}};
  static char *choices[5] =
    {"The Standard RadWare Font",
     "AvantGarde-Book",
     "Helvetica",
     "NewCenturySchlbk-Roman",
     "Times-Roman"};
  int  i;

  if (!psfile) return 1;
  dspsgd.font_num = in_font;

  while (dspsgd.font_num < 0 || dspsgd.font_num > 4) {
    dspsgd.font_num = ask_selection(5, 2, (const char **) choices,
				    "", "Please choose a font:", "");
    /* if (dspsgd.font_num > 0 && dspsgd.font_num <= 4)
       tell("You chose %s.\n", psfonts[dspsgd.font_num-1][0]); */
  }

  if (dspsgd.font_num > 0) {
    for (i=0; i<4; i++) {
      fprintf(psfile, "/f%i {/%s findfont exch scalefont setfont} bind def\n",
	      i+1, psfonts[dspsgd.font_num-1][i]);
    }
  }

  return 0;
} /* choose_font */

/* ======================================================================= */
int drawstr_ps(char *ostring, int ncs, char orient,
	       float x, float y, float csx, float csy)
{
  /* draw character string ostring at coordinates X,Y
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
        superscr subscr italic symbol greek  overstrike bold   data_symbol
	{u...}   {d...} {i...} {s...} {g...} {o...}     {b...} {x#} */

  static float cfs = -1.23456f;
  static int   first = 1, cfn = -1;
  static char  ccom[] = "udisgobxUDISGOBX";
  static char  *proc[10] = {"zero","one","two","three","four",
			      "five","six","seven","eight","nine"};
  static float x_font[3000], y_font[3000], dx_font[300];
  static int   start_font[300], end_font[300];
  static char  move_flag[3000];

  float x2, y2, x_length, wx, wy, dx_chars[256], ww;
  int   i1, jcom, ncom, icom[10], command_true[10];
  int   c_italics[256], c_subscr[256], c_superscr[256], c_bold[256];
  int   str_chars[256], c_symbol[256];
  int   i, j, k, jchar=0, ix, iy, ixd, istroke=0, n_str_chars;
  int   nchar, j0, j9, iichar;
  char  outstr[512], line[256], fullname[200];
  FILE *file;

  if (ncs < 0) {
    /* use ncs < 0 for reinitialising cfs for new plots */
    cfs = -1.23456f;
    return 0;
  }

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

  if (!psfile) return 1;
  wx = csx * 1.2f;
  wy = csy;
  ncom = 0;
  n_str_chars = 0;
  for (i = 0; i < 8; ++i) {
    command_true[i] = 0;
  }

  /* read through input string and look for imbedded commands
     copy characters to be output to STR_CHARS 
     n_str_chars = number of characters to be output */
  for (k = 0; k < ncs; ++k) {
    if (k > 0 && ncom > 0 &&
	ostring[k-1] == '{' && icom[ncom-1] != 0) continue;

    if (ostring[k] == '{') {
      /* look for superscript/subscript/italics/bold commands etc
	 allow up to 10 nested commands */
      if (ncom < 10) ncom++;
      icom[ncom-1] = 0;
      for (jcom = 0; jcom < 8; ++jcom) {
	if (ostring[k+1] == ccom[jcom] ||
	    ostring[k+1] == ccom[jcom+8]) {
	  icom[ncom-1] = jcom + 1;
	  command_true[jcom] = 1;
	  break;
	}
      }
      continue;
    } else if (ncom > 0 && ostring[k] == '}') {
      /* forget last command */
      ncom--;
      if (icom[ncom] != 0) command_true[icom[ncom] - 1] = 0;
      continue;
    }

    /* character is not a command; copy to output string */
    jchar = ostring[k] - 32;
    if (jchar < 0 || jchar > 94) continue;
    if (command_true[3]) {
      jchar += 100;
    } else if (command_true[4]) {
      jchar += 200;
    }
    str_chars[n_str_chars] = jchar;
    dx_chars[n_str_chars] = dx_font[jchar] * wx;
    if (!command_true[2] && n_str_chars > 1 &&
	c_italics[n_str_chars - 1]) dx_chars[n_str_chars - 1] += wx * 0.2f;
    if (command_true[7]) dx_chars[n_str_chars] = wx;
    if (command_true[0]) dx_chars[n_str_chars] *= 0.7f;
    if (command_true[1]) dx_chars[n_str_chars] *= 0.7f;
    if (command_true[6]) dx_chars[n_str_chars] *= 1.05f;
    if (command_true[5] && n_str_chars > 1)
      dx_chars[n_str_chars - 2] = (dx_chars[n_str_chars - 1] - 
				   dx_chars[n_str_chars]) / 2.0f;
    c_superscr[n_str_chars]  = command_true[0];
    c_subscr[n_str_chars]    = command_true[1];
    c_italics[n_str_chars] = command_true[2];
    c_symbol[n_str_chars] = command_true[7];
    c_bold[n_str_chars++] = command_true[6];
  }

  /* calculate total length of label */
  x_length = 0.0f;
  for (i = 0; i < n_str_chars; ++i) {
    x_length += dx_chars[i];
  }

  /* output label */
  if (dspsgd.font_num > 0) {
    /* use postscript fonts rather than drawn radware font */
    wstroke();
    ww = csy * 8.8f / 6.0f;  /* font size */
    if (dspsgd.font_num == 4) ww *= 1.11f;
    j = 1 + c_italics[0] + 2*c_bold[0];
    if (dspsgd.font_num*10 + j != cfn || ww != cfs) {
      fprintf(psfile, "%.3f f%i\n", ww, j);
      cfn = dspsgd.font_num*10 + j;
      cfs = ww;
    }
    if (orient == 'H' || orient == 'h' || orient == 'C' || orient == 'c') {
      dspsgd.x1 = x - x_length / 2.0f;
      dspsgd.y1 = y;
      fprintf(psfile, "%.1f %.1f m\n", dspsgd.x1, dspsgd.y1);
    } else if (orient == 'V' || orient == 'v') {
      dspsgd.x1 = x;
      dspsgd.y1 = y - x_length / 2.0f;
      fprintf(psfile, "gsave %.1f %.1f m 90 rotate\n", dspsgd.x1, dspsgd.y1);
    } else if (orient == 'L' || orient == 'l') {
      dspsgd.x1 = x;
      dspsgd.y1 = y;
      fprintf(psfile, "%.1f %.1f m\n", dspsgd.x1, dspsgd.y1);
    } else if (orient == 'R' || orient == 'r') {
      dspsgd.x1 = x - x_length;
      dspsgd.y1 = y;
      fprintf(psfile, "%.1f %.1f m\n", dspsgd.x1, dspsgd.y1);
    }

    iichar = 0;
    while (iichar < n_str_chars) {
      nchar = 0;
      jchar = str_chars[iichar];
      if (!c_symbol[iichar] && jchar < 95) {
	/* regular characters, not symbol or greek */
	j = 1 + c_italics[iichar] + 2*c_bold[iichar];
	if (dspsgd.font_num*10 + j != cfn || ww != cfs) {
	  fprintf(psfile, "%.3f f%i\n", ww, j);
	  if (!(orient == 'V' || orient == 'v')) {
	    /* save font number and size only for horizontal orientation */
	    /* for vertical orientation, fonts will be forgotten by grestore */
	    cfn = dspsgd.font_num*10 + j;
	    cfs = ww;
	  }
	}
	nchar = 1;
	if ((char) (jchar + 32) == '(' ||
	    (char) (jchar + 32) == ')' ||
	    (char) (jchar + 32) == '\\') {  /* need to escape (, ), \ */
	  outstr[nchar-1] = '\\';
	  ++nchar;
	}
	outstr[nchar-1] = (char) (jchar + 32);
	jchar = str_chars[iichar+1];
	while (iichar < n_str_chars-1 &&
	       c_superscr[iichar+1] == c_superscr[iichar] &&
	       c_subscr[iichar+1]   == c_subscr[iichar]   &&
	       c_italics[iichar+1]  == c_italics[iichar]  &&
	       c_bold[iichar+1]     == c_bold[iichar]     &&
	       c_symbol[iichar+1]   == c_symbol[iichar]   &&
	       jchar < 95) {
	  /* no change in font size, position or type */
	  ++iichar;
	  ++nchar;
	  if ((char) (jchar + 32) == '(' ||
	      (char) (jchar + 32) == ')' ||
	      (char) (jchar + 32) == '\\') {  /* need to escape (, ), \ */
	    outstr[nchar-1] = '\\';
	    ++nchar;
	  }
	  outstr[nchar-1] = (char) (jchar + 32);
	  jchar = str_chars[iichar+1];
	}
	outstr[nchar] = '\0';
	if (c_superscr[iichar]) {
	  fprintf(psfile,
		  "0 %.3f rmoveto 0.7 0.7 scale\n"
		  "(%s) show\n"
		  "1 0.7 div dup scale 0 %.3f rmoveto\n",
		  csy*0.56f, outstr, csy*-0.56f);
	} else if (c_subscr[iichar]) {
	  fprintf(psfile,
		  "0 %.3f rmoveto 0.7 0.7 scale\n"
		  "(%s) show\n"
		  "1 0.7 div dup scale 0 %.3f rmoveto\n",
		  csy*-0.56f, outstr, csy*0.56f);
	} else {
	  fprintf(psfile, "(%s) show\n", outstr);
	}
      } else {
	/* symbol or greek characters, use drawn radware font */
	wx = csx * 1.2f;
	wy = csy;
	if (c_superscr[iichar]) {
	  wx *= 0.7f;
	  wy *= 0.7f;
	}
	if (c_subscr[iichar]) {
	  wx *= 0.7f;
	  wy *= 0.7f;
	}
	if (c_bold[iichar]) {
	  setlinewidth(wy * 0.15f);
	} else {
	  setlinewidth(wy * 0.09f);
	}
	fprintf(psfile, "gsct\n");
	if (c_symbol[iichar]) {
	  j = str_chars[iichar] - 16;
	  if (j >= 1 && j <= 13) symbgps(j, 0.5f*wx, 0.5f*wy, 0.6f*wx);
	} else {
	  for (j = start_font[jchar]; j <= end_font[jchar]; ++j) {
	    x2 = x_font[j] * wx;
	    y2 = y_font[j] * wy;
	    if (c_superscr[iichar]) y2 += wy * 0.8f;
	    if (c_subscr[iichar]) y2 -= wy * 0.8f;
	    if (c_italics[iichar]) x2 += y_font[j] * 0.2f * wx;
	    stroke_ps(x2, y2, move_flag[j]);
	  }
	  wstroke();
	}
	if (iichar == n_str_chars-1) {
	  fprintf(psfile, "grestore\n");
	} else {
	  fprintf(psfile, "grestore %.3f 0 rmoveto\n", dx_chars[iichar]);
	}
      }
      ++iichar;
    }
    if (orient == 'V' || orient == 'v') fprintf(psfile, "grestore\n");
    return 0;
  }

  /* use drawn radware font rather than postscript fonts */
  if (orient == 'V' || orient == 'v') {  /* vertical */
    dspsgd.x1 = x;
    dspsgd.y1 = y - x_length / 2.f;
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
	setlinewidth(wy * 0.15f);
      } else {
	setlinewidth(wy * 0.09f);
      }
      jchar = str_chars[i];
      j0 = '0' - 32;
      j9 = '9' - 32;
      if (c_symbol[i]) {
	j = str_chars[i] - 16;
	if (j >= 1 && j <= 13) 
	  symbgps(j, dspsgd.x1 + 0.5f*wy, dspsgd.y1 + 0.5f*wx, 0.6f*wx);
      } else if (dspsgd.font_num == 0 && !c_italics[i] &&
		 jchar >= j0 && jchar <= j9) {
	wstroke();
	x2 = dspsgd.x1;
	y2 = dspsgd.y1;
	if (c_superscr[i]) x2 -= wy * 0.8f;
	if (c_subscr[i])   x2 += wy * 0.8f;
	if (x2 > 999.9f || y2 > 999.9f) {
	  fprintf(psfile, "90 %.4f %.4f %.1f %.1f %s\n",
	  wx/1e3f, wy/1e3f, x2, y2, proc[jchar - j0]);
	} else {
	  fprintf(psfile, "90 %.4f %.4f %.3f %.3f %s\n",
	  wx/1e3f, wy/1e3f, x2, y2, proc[jchar - j0]);
	}
      } else {
	for (j = start_font[jchar]; j <= end_font[jchar]; ++j) {
	  x2 = dspsgd.x1 - y_font[j] * wy;
	  y2 = dspsgd.y1 + x_font[j] * wx;
	  if (c_superscr[i]) x2 -= wy * .8f;
	  if (c_subscr[i]) x2 += wy * .8f;
	  if (c_italics[i]) y2 += y_font[j] * .2f * wx;
	  stroke_ps(x2, y2, move_flag[j]);
	}
      }
      dspsgd.y1 += dx_chars[i];
    }

  } else {   /* horizontal */
    if (orient == 'H' || orient == 'h' || orient == 'C' || orient == 'c') {
      dspsgd.x1 = x - x_length / 2.0f;
      dspsgd.y1 = y;
    } else if (orient == 'L' || orient == 'l') {
      dspsgd.x1 = x;
      dspsgd.y1 = y;
    } else if (orient == 'R' || orient == 'r') {
      dspsgd.x1 = x - x_length;
      dspsgd.y1 = y;
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
	setlinewidth(wy * 0.15f);
      } else {
	setlinewidth(wy * 0.09f);
      }
      jchar = str_chars[i];
      j0 = '0' - 32;
      j9 = '9' - 32;
      if (c_symbol[i]) {
	j = str_chars[i] - 16;
	if (j >= 1 && j <= 13) 
	  symbgps(j, dspsgd.x1 + 0.5f*wx, dspsgd.y1 + 0.5f*wy, 0.6f*wx);
      } else if (dspsgd.font_num == 0 && !c_italics[i] &&
		 jchar >= j0 && jchar <= j9) {
	wstroke();
	x2 = dspsgd.x1;
	y2 = dspsgd.y1;
	if (c_superscr[i]) y2 += wy * 0.8f;
	if (c_subscr[i])   y2 -= wy * 0.8f;
	if (x2 > 999.9f || y2 > 999.9f) {
	  fprintf(psfile, "0 %.4f %.4f %.1f %.1f %s\n",
	  wx/1e3f, wy/1e3f, x2, y2, proc[jchar - j0]);
	} else {
	  fprintf(psfile, "0 %.4f %.4f %.3f %.3f %s\n",
	  wx/1e3f, wy/1e3f, x2, y2, proc[jchar - j0]);
	}
      } else {
	for (j = start_font[jchar]; j <= end_font[jchar]; ++j) {
	  x2 = dspsgd.x1 + x_font[j] * wx;
	  y2 = dspsgd.y1 + y_font[j] * wy;
	  if (c_superscr[i]) y2 += wy * 0.8f;
	  if (c_subscr[i]) y2 -= wy * 0.8f;
	  if (c_italics[i]) x2 += y_font[j] * 0.2f * wx;
	  stroke_ps(x2, y2, move_flag[j]);
	}
      }
      dspsgd.x1 += dx_chars[i];
    }
  }

  return 0;
} /* drawstr_ps */

/* ======================================================================= */
int strtpt(float x, float y)
{
  if (!psfile) return 1;
  wstroke();
  dspsgd.sx[dspsgd.npts]   = x;
  dspsgd.sy[dspsgd.npts++] = y;
  return 0;
} /* strtpt */

int connpt(float x, float y)
{
  if (!psfile) return 1;
  dspsgd.sx[dspsgd.npts]   = x;
  dspsgd.sy[dspsgd.npts++] = y;
  return 0;
} /* connpt */

/* ======================================================================= */
int stroke_ps(float x, float y, char move_flag)
{
  /* move to coordinates x,y if move_flag is true */
  /* draw from current position to coordinates x,y */
  /* if move_flag is false */

  if (move_flag) wstroke();
  dspsgd.sx[dspsgd.npts]   = x;
  dspsgd.sy[dspsgd.npts++] = y;
  return 0;
} /* stroke_ps */

/* ======================================================================= */
int w_ps_file(int npw, int ipw, char *instr, int instr_len)
{
  /* write coordinates and instruction to postscript file */
  /* npw = # of points to write */
  /* ipw = first point to write */
  /* instr = instruction character string */

  if (!psfile) return 1;
  if (large_scale > 2) {
    if (npw == 2) {
      fprintf(psfile, " %.0f %.0f %.0f %.0f %s\n",
	      dspsgd.sx[ipw+1], dspsgd.sy[ipw+1],
	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    } else if (npw == 1) {
      fprintf(psfile, " %.0f %.0f %s\n",
	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    }
  } else if (large_scale > 1) {
    if (npw == 2) {
      fprintf(psfile, " %.1f %.1f %.1f %.1f %s\n",
	      dspsgd.sx[ipw+1], dspsgd.sy[ipw+1],
	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    } else if (npw == 1) {
      fprintf(psfile, " %.1f %.1f %s\n",
	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    }
  } else if (large_scale > 0) {
    if (npw == 2) {
      fprintf(psfile, " %.2f %.2f %.2f %.2f %s\n",
	      dspsgd.sx[ipw+1], dspsgd.sy[ipw+1],
	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    } else if (npw == 1) {
      fprintf(psfile, " %.2f %.2f %s\n",
	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    }
  } else {
    if (npw == 2) {
      fprintf(psfile, " %.3f %.3f %.3f %.3f %s\n",
	      dspsgd.sx[ipw+1], dspsgd.sy[ipw+1],
	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    } else if (npw == 1) {
      fprintf(psfile, " %.3f %.3f %s\n",
 	      dspsgd.sx[ipw], dspsgd.sy[ipw], instr);
    }
  }
  return 0;
} /* w_ps_file */

/* ======================================================================= */
int wfill(void)
{
  int i;

  /* write path out to postscript file, close and fill it */
  if (!psfile) return 1;
  if (dspsgd.npts > 1) {
    if (dspsgd.npts == 2) {
      w_ps_file(2, 0, "draw", 4);
    } else {
      w_ps_file(2, 0, "sline", 5);
      for (i = 2; i < dspsgd.npts - 2; i += 2) {
	w_ps_file(2, i, "cline", 5);
      }
      if (dspsgd.npts % 2 != 0) {
	w_ps_file(1, dspsgd.npts-1, "1 efline", 8);
      } else {
	w_ps_file(2, dspsgd.npts-2, "2 efline", 8);
      }
    }
  }
  dspsgd.npts = 0;
  return 0;
} /* wfill */

/* ======================================================================= */
int wstroke(void)
{
  int i;

  /* write path out to postscript file */
  if (!psfile) return 1;
  if (dspsgd.npts > 1) {
    if (dspsgd.npts == 2) {
      w_ps_file(2, 0, "draw", 4);
    } else {
      w_ps_file(2, 0, "sline", 5);
      for (i = 2; i < dspsgd.npts - 2; i += 2) {
	w_ps_file(2, i, "cline", 5);
      }
      if (dspsgd.npts % 2 != 0) {
	if (dspsgd.sx[0] == dspsgd.sx[dspsgd.npts-1] &&
	    dspsgd.sy[0] == dspsgd.sy[dspsgd.npts-1]) {
	  w_ps_file(1, dspsgd.npts-1, "1 ecline", 8);
	} else {
	  w_ps_file(1, dspsgd.npts-1, "1 eline", 7);
	}
      } else {
	if (dspsgd.sx[0] == dspsgd.sx[dspsgd.npts-1] &&
	    dspsgd.sy[0] == dspsgd.sy[dspsgd.npts-1]) {
	  w_ps_file(2, dspsgd.npts-2, "2 ecline", 8);
	} else {
	  w_ps_file(2, dspsgd.npts-2, "2 eline", 7);
	}
      }
    }
  }
  dspsgd.npts = 0;
  return 0;
} /* wstroke */

/* ======================================================================= */
int moveto(float x, float y)
{
  dspsgd.x1 = x;
  dspsgd.y1 = y;
  if (!psfile) return 1;
  wstroke();
  fprintf(psfile, "%.3f %.3f moveto\n", x, y);
  return 0;
} /* moveto */

/* ======================================================================= */
int rmoveto(float x, float y)
{
  dspsgd.x1 += x;
  dspsgd.y1 += y;
  if (!psfile) return 1;
  wstroke();
  fprintf(psfile, "%.3f %.3f rmoveto\n", x, y);
  return 0;
} /* rmovet */

/* ======================================================================= */
int closeps(void)
{
  if (!psfile) return 1;
  wstroke();
  if (encapps) {
    fprintf(psfile, "%%%%Trailer\n");
  } else {
    fprintf(psfile, "\nshowpage\n%%%%Trailer\n%%%%Pages: 1\n");
  }
  fclose(psfile);
  psfile = 0;
  return 0;
} /* closeps */

/* ======================================================================= */
int setlinewidth(float lw)
{
  static float clw = 0.0f;

  if (!psfile) return 1;
  if (fabs(clw - lw) < 0.02f*clw) return 0;
  wstroke();
  fprintf(psfile, "%.4f setlinewidth\n", lw);
  clw = lw;
  return 0;
} /* setlinewidt */

/* ======================================================================= */
int setdash(float l1, float l2)
{
  if (!psfile) return 1;
  wstroke();
  if (l1 > 0.0f && l2 > 0.0f) {
    fprintf(psfile, "[%.2f %.2f] 0 setdash\n", l1, l2);
  } else {
    fprintf(psfile, "[] 0 setdash\n");
  }
  return 0;
} /* setdash */

/* ======================================================================= */
int setrgbcolor(float r, float g, float b)
{
  if (!psfile) return 1;
  wstroke();
  fprintf(psfile, "%.2f %.2f %.2f setrgbcolor\n", r, g, b);
  return 0;
} /* setrgbcolor */

/* ======================================================================= */
int setgray(float val)
{
  if (!psfile) return 1;
  wstroke();
  fprintf(psfile, "%.2f setgray\n", val);
  return 0;
} /* setgray */

/* ======================================================================= */
int openps(char *filnam, char *progname, char *title,
	   float xlo, float xhi, float ylo, float yhi)
{
  /* open output postscript file */

  float scale, scaley, xshift, yshift;
  int   jext, j = 35, copy = 1;
  char  dattim[20], fontline[120], fullname[200];
  FILE  *file;
  int   default_options = 0;

  if (strstr(title, " (defaults)")) default_options = 1;

  if (psfile) closeps();

  jext = setext(filnam, ".ps", 80);
  psfile = open_new_file(filnam, 1+default_options);

  /* write prolog to postscript file */
  datetime(dattim);
  if (!strcmp(filnam + jext, ".eps")) {
    fprintf(psfile,
	    "%%!PS-Adobe-2.0 EPSF-1.2\n"
	    "%%%%Creator: program %s\n"
	    "%%%%Title: %s\n"
	    "%%%%CreationDate: %s\n"
	    "%%%%BoundingBox: 36 36 576 756\n",
	    progname, title, dattim);
    encapps = 1;
  } else {
    fprintf(psfile,
	    "%%!PS-Adobe-1.0\n"
	    "%%%%Creator: program %s\n"
	    "%%%%Title: %s\n"
	    "%%%%CreationDate: %s\n"
	    "%%%%Pages: 1\n"
	    "%%%%DocumentFonts: \n"
	    "%%%%BoundingBox: 36 36 576 756\n",
	    progname, title, dattim);
  }
  if ((xhi - xlo) > (yhi - ylo)) {
    fprintf(psfile,  "%%%%Orientation: Landscape\n%%%%EndComments\n");
  } else {
    fprintf(psfile,  "%%%%EndComments\n");
  }
  if (default_options) {
    choose_font(2);
  } else {
    choose_font(-1);
  }

  /* copy header info. and definitions from file font_ps.dat */
  get_directory("RADWARE_FONT_LOC", fullname, 200);
  strcat(fullname, "font_ps.dat");
  if (!(file = open_readonly(fullname))) {
    warn(" -- Check that the environment variable or logical name\n"
	 "     RADWARE_FONT_LOC is properly defined, and that the file\n"
	 "     font_ps.dat exists in the directory to which it points.\n");
    exit(-1);
  }
  while (fgets(fontline, 120, file)) {
    if (dspsgd.font_num > 0) {
      if (strstr(fontline, "initchar")) copy = 0;
      if (strstr(fontline, "Linecap")) copy = 1;
    }
    if (copy) fprintf(psfile, "%s", fontline);
  }
  fclose(file);
  if (!encapps) {
    fprintf(psfile, "\n%%%%EndProlog\n\n%%%%Page: 1 1 save\n");
  }

  /* set up postscript coordinate system etc */
  if ((xhi - xlo) < (yhi - ylo)) {
    scale  = 540.0f / (xhi - xlo);
    scaley = 720.0f / (yhi - ylo);
    if (scale > scaley) scale = scaley;
    xshift = (540.0f - (xhi - xlo)*scale) * 0.5f + 36.0f;
    yshift = (720.0f - (yhi - ylo)*scale) * 0.5f + 36.0f;
    j = 35;
  } else {
    fprintf(psfile,"612 0 translate 90 rotate\n");
    scale  = 720.0f / (xhi - xlo);
    scaley = 540.0f / (yhi - ylo);
    if (scale > scaley) scale = scaley;
    xshift = (720.0f - (xhi - xlo)*scale) * 0.5f + 36.0f;
    yshift = (540.0f - (yhi - ylo)*scale) * 0.5f + 36.0f;
    j = 120;
  }
  if (!default_options &&
      !askyn("  Suppress logo and file info? (Y/N)"))
    fprintf(psfile, LOGO, j, filnam, dattim);
  fprintf(psfile,
	  "%.1f %.1f translate\n"
	  "%.5f %.5f scale %.2f %.2f translate\n",
	  xshift, yshift, scale, scale, -xlo, -ylo);
  setlinewidth(0.7f/scale);
  large_scale = 0;
  if (scale < 1.0f) large_scale = 1;
  if (scale < 0.1f) large_scale = 2;
  if (scale < 0.01f) large_scale = 3;
  return 0;
} /* openps */

/* ======================================================================= */
int spec2ps(float *data, int nc, float incr, float x)
{
  int i, j, k, l = 0;

  if (!psfile) return 1;
  wstroke();
  fprintf(psfile,
	  "/ch {currentpoint pop exch lineto %.6f 0 rlineto} bind def\n"
	  "newpath %.3f %.3f moveto\n",
	  incr, x, data[0]);
  for (i = 0; i < nc; i += 7) {
    k = i + 7;
    if (k > nc) k = nc;
    for (j = k-1; j >= i; j--) {
      fprintf(psfile,"%.3f ", data[j]);
    }
    fprintf(psfile,"%i {ch} repeat\n", k-i);
    if (++l > 7 && k < nc) {
      l = 0;
      fprintf(psfile,"currentpoint stroke newpath moveto\n");
    }
  }
  wstroke();
  fprintf(psfile,"stroke\n");
  return 0;
} /* spec2ps */

/* ======================================================================= */
int symbgps(int symbol, float x, float y, float size)
{
  /* put a graphics symbol at (x,y)  in window coordinates
     size = size of symbol in  window coordinates
     symbol = 1/2       for circle  / filled circle
     symbol = 3/4       for square  / filled square
     symbol = 5/6       for diamond / filled diamond
     symbol = 7/8       for up   triangle / filled up   triangle
     symbol = 9/10      for down triangle / filled down triangle
     symbol = 11/12/13  for + / x / *
  */

  float  ml, nl, jl;

  if (symbol < 1 || size < 0.0f || !psfile) return 1;
  wstroke();

  ml = 0.5f * size;
  nl = 0.4f * size;
  jl = 0.6f * size;
  switch (symbol) {
  case 1:  /* circle */
    fprintf(psfile,
	    "%.3f %.3f m %.3f %.3f %.3f 0 360 arc stroke\n",
	    x+ml, y, x, y, ml);
    break;
  case 2:  /* filled circle */
    fprintf(psfile,
	    "%.3f %.3f m %.3f %.3f %.3f 0 360 arc\n"
	    " gsave fill grestore stroke\n",
	    x+ml, y, x, y, ml);
    break;
  case 3:  /* square */
  case 4:  /* filled square */
    strtpt(x-nl,y-nl);
    connpt(x-nl,y+nl);
    connpt(x+nl,y+nl);
    connpt(x+nl,y-nl);
    connpt(x-nl,y-nl);
    if (symbol == 4) wfill();
    break;
  case 5:  /* diamond */
  case 6:  /* filled diamond */
    strtpt(x-ml,y);
    connpt(x,   y-ml);
    connpt(x+ml,y);
    connpt(x,   y+ml);
    connpt(x-ml,y);
    if (symbol == 6) wfill();
    break;
  case 7:  /* upwards pointing triangle */
  case 8:  /* filled upwards pointing triangle */
    strtpt(x-ml,y-nl);
    connpt(x+ml,y-nl);
    connpt(x,   y+jl);
    connpt(x-ml,y-nl);
    if (symbol == 8) wfill();
    break;
  case 9:   /* downwards pointing triangle */
  case 10:  /* filled downwards pointing triangle */
    strtpt(x-ml,y+nl);
    connpt(x+ml,y+nl);
    connpt(x,   y-jl);
    connpt(x-ml,y+nl);
    if (symbol == 10) wfill();
    break;
  case 11:  /* horizontal cross */
  case 13:  /* asterisk */
    strtpt(x-ml,y);
    connpt(x+ml,y);
    strtpt(x,   y-ml);
    connpt(x,   y+ml);
    if (symbol == 11) break;
  case 12:  /* inclined cross */
    strtpt(x+ml,y+ml);
    connpt(x-ml,y-ml);
    strtpt(x+ml,y-ml);
    connpt(x-ml,y+ml);
  }
  wstroke();
  return 0;
} /* symbgps */
