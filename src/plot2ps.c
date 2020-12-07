#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

/* ==== extern routines from drawstr_ps ==== */
extern int closeps(void);
extern int connpt(float, float);
extern int drawstr_ps(char *, int, char, float, float, float, float);
extern int moveto(float, float);
extern int openps(char *, char *, char *, float, float, float, float);
extern int rmoveto(float, float);
extern int setrgbcolor(float, float, float);
extern int setdash(float, float);
extern int setgray(float);
extern int setlinewidth(float);
extern int spec2ps(float *, int, float, float);
extern int strtpt(float, float);
extern int symbgps(int, float, float, float);
extern int wfill(void);
extern int wstroke(void);

extern int get(int *i, char *in, float *result, int line);
int rmsread(char *fn, float *sp, char *namesp, int *numch, int idimsp);

float window[4], lox, numx, loy, numy;
int   logscale;
FILE  *file1, *file3, *specfile;

int xy_to_win(float x, float y, float *wx, float *wy);

/* ======================================================================= */
int main(int argc, char **argv)
{
  static float symbolsize = 5.0f;
  static float fline[4] = { 0.f,0.f,0.f,0.0f };
  static float flinesum[4] = { 0.f,0.f,0.f,0.0f };
  static int   symbol = 1;
  static int   joinflag = 1, color = 0, symbol2 = 1;
  static int   copytext = 0, default_options = 0;

  float r1, r2, flen, dxin, dyin, xmin, ymin, xmax, ymax, spec[16384];
  float d, scale, fdist, a1, a2, a3, a4, c1, c2, fd, fj;
  float x[10], y[10], fx, fy, indata[20];
  float ranges[4], wx, wy, lw0, lw1, lw2, ch_incr;
  float wx1, wy1, wx2, wy2, csx, csy, xin, yin;
  float linesp = 1.7f;
  int   icol, even, iext, i, j, k, jline, col_x, col_y, numch, np, ich;
  int   first, id, nc, col_dx, col_dy, maxcol, ncs, dataset, ich1, ich2;
  char  filnam[80], fn2[80], label[80], ans[80];
  char  line[256], spfilnam[80], namesp[8], *c;
  short sspec[4096];
  int   ispec[8192], spnum;


  if (argc > 1 && strstr(argv[1], "-h")) {
    printf("\n  Usage: %s <input_file_name> [-d]\n"
           "     input_file_name should have a .psg or .pdg extension\n"
           "     -d specifies that default options should be used\n"
           "          (to bypass stdin for ease of use in shell scripts etc,)\n", argv[0]);
    return 0;
  }
    
  /* open input graphics metafile and output postscript file */
  printf("\n\n"
	 " Welcome to plot2ps\n\n"
	 "    This program produces PostScript files from\n"
	 "     .pdg and .psg graphics metafiles.\n"
	 "    See doc/plot.hlp for information on generating\n"
	 "     and editing .pdg and .psg files.\n\n"
	 "    D.C. Radford    Oct. 1999\n\n");

  if (argc > 2 && strstr(argv[2], "-d")) {
    default_options = 1;
    printf(" Using default_options for output file name, helvetica font, etc.\n");
  }
  /* ask for data file name and open file */
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      cask("Data file = ? (default .ext = .pdg, .psg)", filnam, 80);
    }
    strcpy(fn2, filnam);
    iext = setext(filnam, ".pdg", 80);
    if ((file1 = fopen(filnam, "r"))) break;
    if (strcmp(fn2, filnam)) {
      setext(fn2, ".psg", 80);
      if ((file1 = fopen(fn2, "r"))) {
	strcpy(filnam, fn2);
	break;
      }
      printf("Sorry... neither %s nor %s exists.\n", filnam, fn2);
    } else {
      file_error("open", filnam);
    }
  }

/* find dimensions of drawing, i.e. max,min values of x,y */
  xmin = ymin = 99999.0f;
  xmax = ymax = -99999.0f;
  while (fgets(line, 256, file1)) {
    if (copytext && *line != '\\') continue;
    if (*line == 'V') {
/* ------------------------------ vector */
      if (sscanf(line + 1, "%f%f%f%f", &a1, &a2, &a3, &a4) != 4) goto ERR;
      if (xmin > a1) xmin = a1;
      if (xmin > a3) xmin = a3;
      if (ymin > a2) ymin = a2;
      if (ymin > a4) ymin = a4;
      if (xmax < a1) xmax = a1;
      if (xmax < a3) xmax = a3;
      if (ymax < a2) ymax = a2;
      if (ymax < a4) ymax = a4;

    } else if (!strncmp(line, "SLIN", 4) || !strncmp(line, "SLOG", 4)) {
/* ------------------------------ spectrum plot */
      if (!fgets(line, 256, file1) ||
	  sscanf(line + 1, "%f%f%f%f", &a1, &a2, &a3, &a4) != 4) goto ERR;
      if (xmin > a1) xmin = a1;
      if (ymin > a3) ymin = a3;
      if (xmax < a1 + a2) xmax = a1 + a2;
      if (ymax < a3 + a4) ymax = a3 + a4;

    } else if (!strncmp(line, "LIN", 3) ||
	       !strncmp(line, "LOG", 3) ||
/* ------------------------------ data plot */
	       !strncmp(line, "\\RF", 3)) {
/* ------------------------------ filled rectangle */
      if (sscanf(line + 4, "%f%f%f%f", &a1, &a2, &a3, &a4) != 4) goto ERR;
      if (xmin > a1) xmin = a1;
      if (ymin > a3) ymin = a3;
      if (xmax < a1 + a2) xmax = a1 + a2;
      if (ymax < a3 + a4) ymax = a3 + a4;

    } else if (*line == 'C') {
/* ------------------------------ label */
      if (sscanf(line + 2, "%f%f%f%f", &a1, &a2, &c1, &c2) != 4) goto ERR;
      c1 *= 0.5f;
      c2 *= 0.5f;
      flen = (float) (strlen(line) - 39) * c1;
      if (line[1] == 'V') {
	if (xmin > a1 - c2) xmin = a1 - c2;
	if (xmax < a1 + c2) xmax = a1 + c2;
	if (ymin > a2 - flen) ymin = a2 - flen;
	if (ymax < a2 + flen) ymax = a2 + flen;
      } else {
	if (xmin > a1 - flen) xmin = a1 - flen;
	if (xmax < a1 + flen) xmax = a1 + flen;
	if (ymin > a2 - c2) ymin = a2 - c2;
	if (ymax < a2 + c2) ymax = a2 + c2;
      }
    } else if (!strncmp(line, "\\PG", 3)) {
/* ------------------------------ page dimensions */
      if (sscanf(line + 3, "%f%f%f%f", &a1, &a2, &a3, &a4) != 4) goto ERR;
      xmin = a1;
      ymin = a3;
      xmax = a1 + a2;
      ymax = a3 + a4;
      break;
    } else if (!strncmp(line, "\\TXT", 4)) {
/* ------------------------------ start copying text to output */
      copytext = 1;
    } else if (!strncmp(line, "\\ENDTXT", 7)) {
/* ------------------------------ no longer copy text to output */
      copytext = 0;
    }
  }
  copytext = 0;

  /* leave a 3% margin around drawing */
  ranges[0] = xmin - (xmax - xmin) * 0.03f;
  ranges[2] = (xmax - xmin) * 1.06f;
  ranges[1] = ymin - (ymax - ymin) * 0.03f;
  ranges[3] = (ymax - ymin) * 1.06f;

  /* get x0,nx,y0,ny (scale for output drawing) */
  printf("Full scales are:\n");
  printf("     X0, NX: %.2f %.2f\n", ranges[0], ranges[2]);
  printf("     Y0, NY: %.2f %.2f\n", ranges[1], ranges[3]);
  if (!default_options) {
    while ((nc = cask("Enter X0,NX? (rtn for full scale)", ans, 80)) &&
           ffin(ans, nc, &ranges[0], &ranges[2], &fj));
    while ((nc = cask("Enter Y0,NY? (rtn for full scale)", ans, 80)) &&
           ffin(ans, nc, &ranges[1], &ranges[3], &fj));
  }

  /* open output file */
  strcpy(fn2 + iext, ".ps ");
  if (!default_options) {
    printf("Output file name = ? (rtn for %s)\n", fn2);
    if (cask("             (default .ext = .ps)", ans, 80)) strcpy(fn2, ans);
  }
  sprintf(line, "PLOT  metafile %s", filnam);
  if (default_options) strcat(line, " (defaults)");
  openps(fn2, "PLOT2PS (Author: D.C. Radford)", line,
	 ranges[0], ranges[0] + ranges[2],
	 ranges[1], ranges[1] + ranges[3]);
  if (ranges[3] > ranges[2]) {
    r1 = 562.0f / ranges[2], r2 = 720.0f / ranges[3];
  } else {
    r1 = 720.0f / ranges[2], r2 = 562.0f / ranges[3];
  }
  scale = (r1 < r2 ? r1 : r2);
  lw0 = 0.7f / scale;
  lw1 = lw2 = lw0;

  /* get plot info and draw figure */
  rewind(file1);
  while (fgets(line, 256, file1)) {
    if (copytext && *line != '\\') {
/* ------------------------------ copy text to output */
      if (csx <= 0.0f || csy <= 0.0f) continue;
      /* parse input line, looking for {\...} commands;
	 output remaining characters as strings */
      i = j = 0;
      k = strlen(line) - 1;
      if (k > 0 && (int)line[k-1] == 13) k--;
      while (k > 0 && line[k-1] == ' ') k--;

      if (k > 0) moveto(*x, *y);
      while (i < k) {
	if (!strncmp(line + i, "{\\", 2)) {
	  /* output string */
	  if ((ncs = i - j) > 0) drawstr_ps(line, ncs, 'X', *x, *y, csx, csy);
	  /* ---------- execute command in {\...} */
	  i += 2;
	  if (!strncmp(line + i, "RGB", 3)) {
	    /* define new color */
	    if (sscanf(line + 4, "%f%f%f", &a1, &a2, &a3) != 3) goto ERR;
	    setrgbcolor(a1, a2, a3);
	  } else if (line[i] == 'X' || line[i] == 'Y') {
	    /* relative move */
	    if (sscanf(line + i + 1, "%f", &a1) != 1) goto ERR;
	    if (line[i] == 'Y') {
	      rmoveto(0.0f, a1*csy);
	      y[0] += a1*csy;
	    } else {
	      rmoveto(a1*csx, 0.0f);
	    }
	  }
	  /* ---------- */
	  while (++i < k && line[i] != '}') ;
	  j = i;
	} else {
	  i++;
	}
      }
      /* output string */
      if ((ncs = k - j) > 0) drawstr_ps(line, ncs, 'X', *x, *y, csx, csy);
      y[0] -= linesp*csy;

    } else if (*line == 'V') {
/* ------------------------------ vector */
      np = sscanf(line + 1, "%f%f%f%f%f%f%f%f%f%f",
		  &x[0], &y[0], &x[1], &y[1], &x[2],
		  &y[2], &x[3], &y[4], &x[4], &y[4]);
      if (np < 4 || np%2 != 0)  goto ERR;
      setlinewidth(lw2);
      strtpt(x[0], y[0]);
      for (i = 1; i < np/2; ++i) {
	connpt(x[i], y[i]);
      }

    } else if (!strncmp(line, "LS", 2)) {
/* ------------------------------ line style */
      sscanf(line+3, "%f%f%f%f",
	     &fline[0], &fline[1], &fline[2], &fline[3]);
      flinesum[0] = fline[0];
      flinesum[1] = fline[0] + fline[1];
      flinesum[2] = fline[0] + fline[1] + fline[2];
      flinesum[3] = fline[0] + fline[1] + fline[2] + fline[3];

/* -----------------------------------------------line width */
    } else if (!strncmp(line, "LW", 2)) {
      sscanf(line+3, "%f%f", &lw1, &lw2);
      if (lw1 <= 0.0f) lw1 = lw0;
      if (lw2 <= 0.0f) lw2 = lw0;

    } else if (!strncmp(line, "SLIN", 4) || !strncmp(line, "SLOG", 4)) {
/* ------------------------------ spectrum plot */
      /* test to see if y-axis should be log scale */
      logscale = 0;
      if (!strncmp(line, "SLOG", 4)) logscale = 1;

      /* get spectrum file name, window, lox,numx,loy,numy */
      strncpy(spfilnam, line + 5, 40);
      if (sscanf(line + 45, "%d", &spnum) != 1) spnum = 0;
      if ((c = strchr(spfilnam, ' ')) ||
	  (c = strchr(spfilnam, '\n'))) *c = '\0';
      if (!fgets(line, 256, file1) ||
	  sscanf(line + 1, "%f%f%f%f",
		 &window[0], &window[1],
		 &window[2], &window[3]) != 4) goto ERR;
      if (!fgets(line, 256, file1) ||
	  sscanf(line + 1, "%f%f%f%f",
		 &lox, &numx, &loy, &numy) != 4) goto ERR;

      if (strstr(spfilnam, ".spn") ||
	  strstr(spfilnam, ".mat") ||
	  strstr(spfilnam, ".sec")) {
	if (!(specfile = fopen(spfilnam, "r"))) goto ERR;
	if (strstr(spfilnam, ".spn")) {
	  fseek(specfile, spnum*16384, SEEK_SET);
	  if (fread(ispec, 16384, 1, specfile) != 1) goto ERR;
	  for (j=0; j<4096; j++) spec[j] = ispec[j];
	  numch = 4096;
	} else if (strstr(spfilnam, ".mat")) {
	  fseek(specfile, spnum*8192, SEEK_SET);
	  if (fread(sspec, 8192, 1, specfile) != 1) goto ERR;
	  for (j=0; j<4096; j++) spec[j] = sspec[j];
	  numch = 4096;
	} else if (strstr(spfilnam, ".sec")) {
	  fseek(specfile, spnum*32768, SEEK_SET);
	  if (fread(ispec, 32768, 1, specfile) != 1) goto ERR;
	  for (j=0; j<8192; j++) spec[j] = ispec[j];
	  numch = 8192;
	}
	fclose(specfile);
      } else if (strstr(spfilnam, ".rms")) {
        sprintf(line, "%s %d", spfilnam, spnum);
        if (rmsread(line, spec, namesp, &numch, 16384)) {
          printf("ERROR: cannot read spectrum file: %s\n", spfilnam);
          return 1;
        }
      } else if (read_spe_file(spfilnam, spec, namesp, &numch, 16384)) {
	goto ERR;
      }

      setlinewidth(lw1);
      ich1 = (int) (0.5f + lox);
      ich2 = (int) (0.5f + lox + numx);
      i = 0;
      for (ich = ich1; ich < ich2; ++ich) {
	xy_to_win(lox, spec[ich], &x[0], &spec[i++]);
      }
      ch_incr = window[1] / numx;
      nc = ich2 - ich1;
      spec2ps(spec, nc, ch_incr, x[0]);

    } else if (!strncmp(line, "LIN", 3) || !strncmp(line, "LOG", 3)) {
/* ------------------------------ data plot */
      /* test to see if y-axis should be log scale */
      logscale = 0;
      if (!strncmp(line, "LOG", 3)) logscale = 1;

      /* get window limits,lox,numx,loy,numy */
      if (sscanf(line + 4, "%f%f%f%f",
		 &window[0], &window[1], &window[2], &window[3]) != 4)
	goto ERR;
      if (!fgets(line, 256, file1) ||
	  sscanf(line + 1, "%f%f%f%f",
		 &lox, &numx, &loy, &numy) != 4) goto ERR;

    } else if (!strncmp(line, "S ", 2)) {
/* ------------------------------ symbol definition */
      /* decode symbol number, size, join-by-vector flag, color */
      sscanf(line + 1, "%i%f%i%i%i",
	     &symbol, &symbolsize, &joinflag, &color, &symbol2);

    } else if (*line == 'D') {
/* ------------------------------ data */
      sscanf(line+2, "%i%i%i%i%40c%i",
	     &col_x, &col_dx, &col_y, &col_dy, filnam, &dataset);
      if (strncmp(filnam, "    ", 4)) {
	if ((c = strchr(filnam, ' ')) ||
	    (c = strchr(filnam, '\n'))) *c = '\0';
	/* open external data file */
	if (!(file3 = fopen(filnam, "r+"))) goto ERR2;
	/* skip forward (dataset - 1) data sets */
	for (id = 0; id < dataset - 1; ++id) {
	  while (1) {
	    if (!fgets(line, 256, file3)) goto ERR2;
	    if (!strncmp(line, "ENDDATA", 7)) break;
	    for (i = 0; line[i] == ' '; i++);
	    if (line[i] == '\n') break;
	  }
	}
      }
      /* setcolor(color + 1); */

      /* read and draw points */
      first = 1;
      even = 1;
      fdist = 0.0f;
      jline = 1;
      maxcol = col_x;
      if (maxcol < col_y) maxcol = col_y;
      if (maxcol < col_dx) maxcol = col_dx;
      if (maxcol < col_dy) maxcol = col_dy;
      if (maxcol > 20) {
	printf("ERROR: Column number too high; maximum is 20!\n");
	goto ERR;
      }

      while (1) {
	if (!strncmp(filnam, "    ", 4)) {
	  /* read data from command file */
	  fgets(line, 256, file1);
	} else {
	  /* read data from external data file */
	  if (!fgets(line, 256, file3)) break;
	}
	if (!strncmp(line, "ENDDATA", 7)) break;
	for (i = 0; line[i] == ' '; i++);
	if (line[i] == '\n') break;
	/* lines not beginning with ',' through '9', or with ' ',
	   are considered to be comments; ignore them */
	if (*line != ' ' && (*line < ',' || *line > '9')) continue;

	/* decode data */
	i = 0;
	for (icol = 0; icol < maxcol; ++icol) {
	  indata[icol] = 0.0f;
	  if (get(&i, line, &indata[icol], 0) < 0) goto ERR2;
	}
	dxin = 0.0f;
	dyin = 0.0f;
	xin = indata[col_x - 1];
	yin = indata[col_y - 1];
	if (col_dx > 0) dxin = indata[col_dx - 1];
	if (col_dy > 0) dyin = indata[col_dy - 1];

	/* draw point */
	/* if symbol is 0 then no symbol will be traced */
	wx1 = wx;
	wy1 = wy;
	/* if data is outside range, get next point */
	if (xy_to_win(xin, yin, &wx, &wy)) continue;
	if (!first && joinflag) {
	  /* draw joining vector */
	  setlinewidth(lw1);
	  if (fline[0] <= 0.0f) {
	    strtpt(wx1, wy1);
	    connpt(wx, wy);
	  } else {
	    if (jline == 1 || jline == 3) strtpt(wx1, wy1);
	    fx = wx - wx1;
	    fy = wy - wy1;
	    fd = sqrt(fx * fx + fy * fy);
	    d = 0.0f;
	    while (d < fd) {
	      d = flinesum[jline - 1] - fdist;
	      if (d > fd) d = fd;
	      wx2 = wx1 + d * fx / fd;
	      wy2 = wy1 + d * fy / fd;
	      if (jline == 1 || jline == 3) {
		connpt(wx2, wy2);
	      } else {
		strtpt(wx2, wy2);
	      }
	      if (d < fd) {
		++jline;
		if (jline == 5) {
		  jline = 1;
		  fdist -= flinesum[3];
		}
	      }
	    }
	    fdist += d;
	  }
	}
	first = 0;
	even = !even;

	/* draw symbol */
	if (symbol > 0) {
	  setlinewidth(lw2);
	  if (even) symbgps(symbol2, wx, wy, symbolsize);
	  if (!even) symbgps(symbol, wx, wy, symbolsize);
	}
	if (dyin > 0.0f) {
	  /* draw error bar */
	  setlinewidth(lw2);
	  xy_to_win(xin, yin + dyin, &wx, &wy);
	  strtpt(wx - symbolsize / 2.0f, wy);
	  connpt(wx + symbolsize / 2.0f, wy);
	  strtpt(wx, wy);
	  xy_to_win(xin, yin - dyin, &wx, &wy);
	  connpt(wx, wy);
	  strtpt(wx - symbolsize / 2.0f, wy);
	  connpt(wx + symbolsize / 2.0f, wy);
	  xy_to_win(xin, yin, &wx, &wy);
	}
	if (dxin > 0.0f) {
	  /* draw error bar */
	  setlinewidth(lw2);
	  r1 = xin + dxin;
	  xy_to_win(xin + dxin, yin, &wx, &wy);
	  strtpt(wx, wy - symbolsize / 2.0f);
	  connpt(wx, wy + symbolsize / 2.0f);
	  strtpt(wx, wy);
	  xy_to_win(xin - dxin, yin, &wx, &wy);
	  connpt(wx, wy);
	  strtpt(wx, wy - symbolsize / 2.0f);
	  connpt(wx, wy + symbolsize / 2.0f);
	  xy_to_win(xin, yin, &wx, &wy);
	}
      }
      if (strncmp(filnam, "    ", 4)) fclose(file3);
      /* setcolor(1) */

    } else if (*line == 'C') {
/* ------------------------------ label */
      /* decode position and string */
      if (sscanf(line + 2, "%f%f%f%f",
		 &x[0], &y[0], &csx, &csy) != 4) return 1;
      if (csx <= 0.0f || csy <= 0.0f) continue;
      strcpy(label, line + 39);
      ncs = strlen(label) - 1;
      /* output string */
      drawstr_ps(label, ncs, line[1], *x, *y, csx, csy);
 
    } else if (*line == '\\') {
      /*
	added Feb 2001:
	\PG  x nx y ny: define default page size and location
	\RF  x nx y ny: fill rectangle
	\RGB r g b: set color; r,g,b = red,green,blue values, 0-1 
	\TXT x y csx csy: start of text to be output to file; all following 
	lines not starting with \ (backslash) are output until a line
	containing \ENDTXT is encountered
	\ENDTXT: end of text
	\X x: space forward along x-axis by x*csx (x may be < 0)
	\Y y: space up along y-axis by y*csy (y may be < 0)
      */

      if (!strncmp(line, "\\RF", 3)) {
/* ------------------------------ filled rectangle */
	if (sscanf(line + 4, "%f%f%f%f", &a1, &a2, &a3, &a4) != 4) goto ERR;
	setlinewidth(lw2);
	strtpt(a1, a3);
	connpt(a1, a3+a4);
	connpt(a1+a2, a3+a4);
	connpt(a1+a2, a3);
	wfill();
      } else if (!strncmp(line, "\\RGB", 4)) {
/* ------------------------------ define new color */
	if (sscanf(line + 5, "%f%f%f", &a1, &a2, &a3) != 3) goto ERR;
	setrgbcolor(a1, a2, a3);
      } else if (line[1] == 'X' || line[1] == 'Y') {
/* ------------------------------ relative move */
	if (sscanf(line + 2, "%f", &a1) != 1) goto ERR;
	if (line[1] == 'Y') {
	  y[0] += a1*csy;
	} else {
	  x[0] += a1*csx;
	}
      } else if (!strncmp(line, "\\TXT", 4)) {
/* ------------------------------ start copying text to output */
	if ((j = sscanf(line + 5, "%f%f%f%f",
			&x[0], &y[0], &a3, &a4)) < 2) goto ERR;
	if (j > 2) csx = a3;
	if (j > 3) csy = a4;
	copytext = 1;
      } else if (!strncmp(line, "\\ENDTXT", 7)) {
/* ------------------------------ no longer copy text to output */
	copytext = 0;
      } else if (!strncmp(line, "\\SP", 3)) {
/* ------------------------------ define line spacing */
	if (sscanf(line + 3, "%f", &linesp) != 1) goto ERR;
	linesp *= 1.7f;
      }

    }
  }
  closeps();
  return 0;
ERR2:
  file_error("open or read", filnam);
  closeps();
  return -1;
ERR:
  printf("Input data file read error!\n");
  closeps();
  return -1;
} /* main */

/* ======================================================================= */
int xy_to_win(float x, float y, float *wx, float *wy)
{
/* This routine converts data coordinates (X,Y) to window coordinates (WX,WY) */

  int flag = 0;

  if (logscale) {    /* logarithmic y-axis */
    *wy = window[2] + window[3] *
      (log((y > 1e-12f ? y : 1e-12f)) - log(loy)) /
      (log(numy + loy) - log(loy));
  } else {           /* linear y-axis */
    *wy = window[2] + window[3] * (y - loy) / numy;
  }
  *wx = window[0] + window[1] * (x - lox) / numx;
  if (*wx < window[0]) {
    *wx = window[0];
    flag = 1;
  }
  if (*wx > window[0] + window[1]) {
    *wx = window[0] + window[1];
    flag = 1;
  }
  if (*wy < window[2]) {
    *wy = window[2];
    flag = 1;
  }
  if (*wy > window[2] + window[3]) {
    *wy = window[2] + window[3];
    flag = 1;
  }
  return flag;
} /* xy_to_win */
