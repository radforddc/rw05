#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"
extern int get(int *i, char *in, float *result, int line);
extern int base(int *i, char *in);

float charsize[2] = { 5.0f, 7.0f };
float window[4] = { 0.0f, 1024.0f, 0.0f, 512.0f };
float ticksize = 2.0f, lox = 0.0f, numx = 1024.0f, loy = 0.0f, numy = 512.0f;
float ascale, aoffset;
float dminx=0, dmaxx=10, dminy=0, dmaxy=10;   // dataset min, max values
int   top, ticks, numbers, logscale, xaxis;
char  label[80];

int drawaxis(void);
int getfile(int *i, char *in, char *filnam, int line);
int getlabel(int *i, char *in, char *label, int line);
int grax(float a, float b, float *dex, int *nv, int kh);
int outstr(char *str, float x, float y, float charsize, int orient);
int xy_to_win(float x, float y, float *wx, float *wy);
int rmsread(char *fn, float *sp, char *namesp, int *numch, int idimsp);
int data_minmax(char *fn, int cx, int cdx, int cy, int cdy, int ds, char cmd);

FILE  *file1, *file2, *specfile;

/* ======================================================================= */
int main(int argc, char **argv)
{
  static float symbolsize = 5.0f;
  static int   default_options = 0;

  float spec[16384], fline[4], test, w1, w2, ynum, sscale, soffset;
  float autofact;
  int   i2, line, jext, nc, joinflag, i, j, col_x, col_y, color;
  int   col_dx, col_dy, symbol, symbol2, dataset, numch, lo, psg;
  short sspec[4096];
  int   ispec[8192], spnum;
  char  in[256], graphfile[80], cmdfile[80], filnam[80], namesp[8], *c;


  if (argc > 1 && strstr(argv[1], "-h")) {
    printf("\n  Usage: %s <input_file_name> [-d]\n"
           "     input_file_name should have .psc or .pdc extension.\n"
           "     -d specifies that default options should be used\n"
           "          (to bypass stdin for ease of use in shell scripts etc,)\n", argv[0]);
    return 0;
  }
    
  printf("\n\n"
	 " Welcome to plot\n\n"
	 "    This program produces .pdg and .psg graphics metafiles\n"
	 "     from .pdc or .psc input files.\n"
	 "    See doc/plot.hlp for information on generating\n"
	 "     .pdc and .psc files.\n\n"
	 "    D.C. Radford    Oct. 1999\n\n");
  
  if (argc > 1 && strstr(argv[1], "-h")) {
    printf("\n  Usage: %s <input_file_name> [-d]\n"
           "     input_file_name should have a .psc or .pdc extension\n"
           "     -d specifies that default options should be used\n"
           "          (for ease of use in shell scripts etc,)\n", argv[0]);
    return 0;
  }
    
  if (argc > 2 && strstr(argv[2], "-d")) {
    default_options = 1;
    printf(" Using default_options for output file name, etc.\n");
  }
  /* ask for data file name and open file */
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      cask("Input file = ? (default .ext = .pdc, .psc)", filnam, 80);
    }
    strcpy(cmdfile, filnam);
    jext = setext(cmdfile, ".pdc", 80);
    if ((file1 = fopen(cmdfile, "r"))) break;
    if (strcmp(cmdfile, filnam)) {
      setext(filnam, ".psc", 80);
      if ((file1 = fopen(filnam, "r"))) {
	strcpy(cmdfile, filnam);
	break;
      }
      printf("Sorry... neither %s nor %s exists.\n", cmdfile, filnam);
    } else {
      file_error("open", cmdfile);
    }
  }

  strcpy(graphfile, cmdfile);
  strcpy(graphfile + jext + 3, "g");
  strcpy(in, graphfile + jext);
  if (!default_options) {
    printf("Default output graphics file = %s\n"
           "  Default .ext = %s\n", graphfile, in);
    nc = cask("  Output graphics file name = ? (rtn for default)", filnam, 80);
    if (nc) strcpy(graphfile, filnam);
  }
  setext(graphfile, in, 80);
  file2 = open_new_file(graphfile, 1+default_options);
  psg = !strcmp(in, ".psg");

  /* calculate all vector and character information and output it */
  line = 0;
  while (fgets(in, 256, file1)) {
    ++line;
    i = 2;

    /* command is first character of command line
       commands currently implemented:
       C: define character sizes for labels etc., tick length for axes
       SLIN: plot spectrum, define plot window, etc
       SLOG: same as SLIN above, but with log scale for y-axis
       LIN: define data plot window, etc
       LOG: same as LIN above, but with log scale for y-axis
       X: draw x-axis
       Y: draw y-axis
       S: symbol (or same as SLIN for .psc files)
       D: data for data plot
       LW: sets line widths for (1) data (2) axes/vectors/symbols
       LS: sets line style

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
       \SP f: set vertical line spacing to f* default

       added July 2020:
       m  <data for data plot> (same format as D above): find min and max for x, y
                                                    for use in auto-scaling data axes
       M  <data for data plot>: same as for m above, but do not start fresh with min, max
       in LIN command, coordinate limits: a1.1 - use max*1.1 as upper limit
                                                  or min/1.1 as lower limit
                                          m/x3.0 - use range as above
                                                   with a default factor 1.2,
                                                    and a minimum/max of 3.0
          e.g.
             M 1 0 3 4 data.dat 3     get min,max values for x,y from data.dat set 3,
             LIN 100  100  100   70                         using columns 1, (3 +- 4)
                   0 a1.2 a1.1 m6.5   autoscale xmax, ymin, and ymax_with_min_6.5
    */

    if (*in == 'C' || *in == 'c') {
      if (in[1] == 'V' || in[1] == 'v' ||
          in[1] == 'H' || in[1] == 'h') {  // character/string drawing command
	fprintf(file2, "%s", in);
      }
      /* get character sizes, tick mark size */
      for (j = 0; j < 2; ++j) {
	if (get(&i, in, &charsize[j], line) < 0) return 0;
      }
      if (get(&i, in, &ticksize, line)) return 0;

    } else if (!strncmp(in, "M ", 2) ||
	       !strncmp(in, "m ", 2)) {
      sscanf(in+2, "%i %i %i %i %s %i",
         &col_x, &col_dx, &col_y, &col_dy, filnam, &dataset);
      data_minmax(filnam, col_x, col_dx, col_y, col_dy, dataset, in[0]);

    } else if (!strncmp(in, "mdy ", 4)) { // minimax values for y
      float d;
      sscanf(in+4, "%f",  &d);
      printf("mdy %f: %f %f -> ", d, dminy, dmaxy);
      if (dmaxy - dminy < d) {
        d -= dmaxy - dminy;
        dmaxy += d/2.0;
        dminy -= d/2.0;
      }
      printf("%f %f\n", dminy, dmaxy);

    } else if (!strncmp(in, "LIN", 3) ||
	       !strncmp(in, "lin", 3) ||
	       !strncmp(in, "LOG", 3) ||
	       !strncmp(in, "log", 3)) {
      /* define window coordinates, axis lengths
	 test to see if y-axis should be log scale */
      logscale = 0;
      if (!strncmp(in, "LOG", 3) ||
	  !strncmp(in, "log", 3)) logscale = 1;

      /* read window coordinates, axis lengths */
      i = 4;
      for (j = 0; j < 4; ++j) {
	if (get(&i, in, &window[j], line) < 0) return 0;
      }
      if (!fgets(in, 256, file1)) return 0;
      ++line;
      i = 1;
      while (in[i] == ' ') i++;
      if (in[i] == 'a') {
        autofact = 1.2;
        if (in[++i] != ' ') get(&i, in, &autofact, line);
        lox = dminx - (autofact-1.0) * (dmaxx - dminx);
      } else if (get(&i, in, &lox, line) < 0) {
        printf("in %s  i=%d  in[1] = %s\n", in, i, in+i);
        return 0;
      }
      while (in[i] == ' ') i++;
      if (in[i] == 'a') {
        autofact = 1.2;
        if (in[++i] != ' ') get(&i, in, &autofact, line);
        numx = dmaxx + (autofact-1.0) * (dmaxx - dminx) - lox;
      } else if (get(&i, in, &numx, line) < 0)  {
        printf("in %s  i=%d  in[1] = %s\n", in, i, in+i);
        return 0;
      }
      while (in[i] == ' ') i++;
      if (in[i] == 'a') {
        autofact = 1.2;
        if (in[++i] != ' ') get(&i, in, &autofact, line);
        loy = dminy - (autofact-1.0) * (dmaxy - dminy);
      } else if (get(&i, in, &loy, line) < 0)  {
        printf("in %s  i=%d  in[1] = %s\n", in, i, in+i);
        return 0;
      }
      if (logscale && loy <= 1e-12f) loy = 1.0f;
      while (in[i] == ' ') i++;
      if (in[i] == 'a') {
        autofact = 1.2;
        if (in[++i] != ' ') get(&i, in, &autofact, line);
        numy = dmaxy + (autofact-1.0) * (dmaxy - dminy) - loy;
      } else if (get(&i, in, &numy, line) < 0)  {
        printf("in %s  i=%d  in[1] = %s\n", in, i, in+i);
        return 0;
      }
      /* output window data etc to plot file */
      if (logscale) {
	fprintf(file2,
		"LOG  %.3f %.3f %.3f %.3f\n",
		window[0], window[1], window[2], window[3]);
      } else {
	fprintf(file2,
		"LIN  %.3f %.3f %.3f %.3f\n",
		window[0], window[1], window[2], window[3]);
      }
      if (lox < 1.0f || numx < 1.0f ||
	  loy < 1.0f || numy < 1.0f) {
	fprintf(file2,
		"+    %.4e %.4e %.4e %.4e\n",
		lox, numx, loy, numy);
      } else {
	fprintf(file2,
		"+    %.3f %.3f %.3f %.3f\n",
		lox, numx, loy, numy);
      }

    } else if (!strncmp(in, "LW", 2) || !strncmp(in, "lw", 2)) {
      i = 3;
      w1 = w2 = 0.0f;
      if (get(&i, in, &w1, line) < 0) return 0;
      if (get(&i, in, &w2, line) < 0) return 0;
      fprintf(file2, "LW  %.3f %.3f\n", w1, w2);

    } else if (!strncmp(in, "LS", 2) || !strncmp(in, "ls", 2)) {
      i = 3;
      for (j = 0; j < 4; ++j) {
	fline[j] = 0.0f;
	if (get(&i, in, &fline[j], line) < 0) return 0;
      }
      fprintf(file2, "LS  %.3f %.3f %.3f %.3f\n",
	      fline[0], fline[1], fline[2], fline[3]);

    } else if (*in == 'X' || *in == 'x' ||
	       *in == 'Y' || *in == 'y') {
      /* draw and label x or y axis
	 test for x- or y-axis */
      xaxis = 0;
      if (*in == 'X' || *in == 'x') xaxis = 1;
      /* if no further specs, draw axis using defaults */
      top = 0;
      strcpy(label, " ");
      ticks = 1;
      numbers = 0;
      ascale = 1.0f;
      aoffset = 0.0f;
      i = 1;
      base(&i, in);
      if (i <= 110) {
	/* test for axis at top (x-axis) or right (y-axis) */
	if (in[i] == ',') ++i;
	if (in[i] == 'T' || in[i] == 't') top = 1;
	if (in[i] == 'R' || in[i] == 'r') top = 1;
	/* get label for axis */
	++i;
	if (getlabel(&i, in, label, line) < 0) return 0;
	/* test = 0 for no ticks, no numbers
	   test = 1 for ticks, but no numbers
	   test = 2 for both ticks and numbers */
	test = 1.0f;
	if (get(&i, in, &test, line) < 0) return 0;
	if (test < 0.5f) ticks = 0;
	if (test > 1.5f) numbers = 1;
	/* get scale and offset for axis units
	   (axis units) = ascale*(data units) + aoffset */
	if (get(&i, in, &ascale, line) < 0) return 0;
	if (ascale == 0.0f) ascale = 1.0f;
	if (get(&i, in, &aoffset, line) < 0) return 0;
      }
      /* draw and label axis */
      drawaxis();

    } else if ((psg && (*in == 'S' || *in == 's')) ||
	       !strncmp(in, "SL", 2) || !strncmp(in, "sl", 2)) {
      /* plot spectrum */
      /* test to see if y-axis should be log scale */
      i = 2;
      logscale = 0;
      if (!strncmp(in, "SLIN", 4) || !strncmp(in, "slin", 4)) {
	i = 5;
      } else if (!strncmp(in, "SLOG", 4) || !strncmp(in, "slog", 4)) {
	logscale = 1;
	i = 5;
      }
      /* get spectrum file name */
      if (getfile(&i, in, filnam, line) < 0) {
	printf("ERROR: bad spectrum file name, line %d: %s\n",
	       line, filnam);
	return 0;
      }
      if (strstr(filnam, ".spn") ||
	  strstr(filnam, ".mat") ||
	  strstr(filnam, ".sec") ||
	  strstr(filnam, ".rms")) {
	if (sscanf(in + i, "%d", &spnum) != 1) spnum = 0;
	i = strlen(in);
	if (!(specfile = fopen(filnam, "r"))) {
	  printf("ERROR: cannot open spectrum file, line %d: %s\n",
		 line, filnam);
	  return 0;
	}
	if (strstr(filnam, ".spn")) {
	  fseek(specfile, spnum*16384, SEEK_SET);
	  if (fread(ispec, 16384, 1, specfile) != 1) {
	    printf("ERROR: cannot read spectrum file, line %d: %s\n",
		   line, filnam);
	    return 0;
	  }
	  for (j=0; j<4096; j++) spec[j] = ispec[j];
	  numch = 4096;
	} else if (strstr(filnam, ".mat")) {
	  fseek(specfile, spnum*8192, SEEK_SET);
	  if (fread(sspec, 8192, 1, specfile) != 1) {
	    printf("ERROR: cannot read spectrum file, line %d: %s\n",
		   line, filnam);
	    return 0;
	  }
	  for (j=0; j<4096; j++) spec[j] = sspec[j];
	  numch = 4096;
	} else if (strstr(filnam, ".sec")) {
	  fseek(specfile, spnum*32768, SEEK_SET);
	  if (fread(ispec, 32768, 1, specfile) != 1) {
	    printf("ERROR: cannot read spectrum file, line %d: %s\n",
		   line, filnam);
	    return 0;
	  }
	  for (j=0; j<8192; j++) spec[j] = ispec[j];
	  numch = 8192;
	} else if (strstr(filnam, ".rms")) {
          c = strstr(in, filnam);
	  if (rmsread(c, spec, namesp, &numch, 16384)) {
	    printf("ERROR: cannot read spectrum file, line %d: %s\n",
		   line, filnam);
	    return 0;
	  }
	}
	fclose(specfile);
      } else if (read_spe_file(filnam, spec, namesp, &numch, 16384)) {
	printf("ERROR: cannot read spectrum file, line %d: %s\n",
	       line, filnam);
	return 0;
      }

      /* get spectrum scale and offset */
      /* plotted counts = sscale*counts + soffset */
      sscale = 1.0f;
      soffset = 0.0f;
      if (get(&i, in, &sscale, line) < 0) return 0;
      if (get(&i, in, &soffset, line) < 0) return 0;
      /* define window coordinates, axis lengths */
      if (!fgets(in, 256, file1)) return 0;
      ++line;
      i = 0;
      for (j = 0; j < 4; ++j) {
	if (get(&i, in, &window[j], line) < 0) return 0;
      }
      if (!fgets(in, 256, file1)) return 0;
      ++line;
      i = 0;
      lox = loy = ynum  = 0.0f;
      if (get(&i, in, &lox, line) < 0) return 0;
      numx = (float) numch - lox;
      if (get(&i, in, &numx, line) < 0) return 0;
      if (get(&i, in, &loy, line) < 0) return 0;
      if (get(&i, in, &ynum, line) < 0) return 0;
      if (logscale && loy <= 1e-12f) loy = 1.0f;
      if (lox > (float) numch) lox = numch - 1.0f;
      if (ynum > 0.0f) {
	numy = ynum;
      } else if (ynum == 0.0f) {
	lo = (int) (0.5f + lox);
	if (lo < 0) lo = 0;
	test = loy;
	i2 = lo + (int) (0.5f + numx);
	if (i2 > numch) i2 = numch;
	for (j = lo; j < i2; ++j) {
	  if (test < spec[j]) test = spec[j];
	}
	/* leave a 5% gap at the top when autoscaling */
	if (logscale) {
	  numy = pow((double) test, 1.05) - loy;
	} else {
	  numy = (test - loy) * 1.05f;
	}
      }
      if (numy < 1.0f) numy = 1.0f;

      /* output file name etc to plot file */
      if (numx > 0.0f) {
	memset(filnam + strlen(filnam), ' ', 39);
	if (logscale) {
	  fprintf(file2,
		  "SLOG %.40s %d\n"
		  "+  %.3f %.3f %.3f %.3f\n"
		  "+  %.3f %.3f %.3f %.3f\n", filnam, spnum,
		  window[0], window[1], window[2], window[3],
		  lox, numx, loy * sscale + soffset, numy * sscale + soffset);
	} else {
	  fprintf(file2,
		  "SLIN %.40s %d\n"
		  "+  %.3f %.3f %.3f %.3f\n"
		  "+  %.3f %.3f %.3f %.3f\n", filnam, spnum,
		  window[0], window[1], window[2], window[3],
		  lox, numx, loy * sscale + soffset, numy * sscale + soffset);
	}
      }

    } else if (*in == 'S' || *in == 's') {
      /* copy symbol info, data etc. to output file. */
      test = 0.0f;
      if (get(&i, in, &test, line) < 0) return 0;
      symbol = (int) (0.5f + test);
      symbolsize = 5.0f;
      if (get(&i, in, &symbolsize, line) < 0) return 0;
      test = 1.0f;
      if (get(&i, in, &test, line) < 0) return 0;
      joinflag = (int) (0.5f + test);
      test = 0.0f;
      if (get(&i, in, &test, line) < 0) return 0;
      color = (int) (0.5f + test);
      test = (float) symbol;
      if (get(&i, in, &test, line) < 0) return 0;
      symbol2 = (int) (0.5f + test);
      fprintf(file2, "S %i %.3f %i %i %i\n",
	      symbol, symbolsize, joinflag, color, symbol2);

    } else if (*in == 'D' || *in == 'd') {
      /* get column numbers for data containing x, dx, y, dy */
      test = 1.0f;
      if (get(&i, in, &test, line) < 0) return 0;
      col_x = (int) (0.5f + test);
      test = 0.0f;
      if (get(&i, in, &test, line) < 0) return 0;
      col_dx = (int) (0.5f + test);
      test = 2.0f;
      if (get(&i, in, &test, line) < 0) return 0;
      col_y = (int) (0.5f + test);
      test = 3.0f;
      if (get(&i, in, &test, line) < 0) return 0;
      col_dy = (int) (0.5f + test);
      if (col_x <= 0 || col_y <= 0) {
	printf("Data columns for x and y must be > 0!\n");
	return 0;
      }

      /* get optional data file name and data set number */
      dataset = 1;
      if (getfile(&i, in, filnam, line) <= 0) {
	memset(filnam + strlen(filnam), ' ', 40);
	fprintf(file2, "D  %i %i %i %i%.40s %i\n",
		col_x, col_dx, col_y, col_dy, filnam, dataset);
	/* copy data from input file to output file */
	while (1) {
	  if (!fgets(in, 256, file1)) return 0;
	  ++line;
	  if (!strcmp(in, "\n") ||
	      !strcmp(in, " \n")) {
	    fprintf(file2, "ENDDATA\n");
	    break;
	  } else {
	    fprintf(file2, "%s", in);
	  }
	}
      } else {
	setext(filnam, ".dat", 40);
	if (!inq_file(filnam, &j)) {
	  printf("ERROR: File does not exist: %s\n", filnam);
	  printf("line %d, %s", line, in);
	  return 0;
	}
	memset(filnam + strlen(filnam), ' ', 39);
	test = 1.0f;
	if (get(&i, in, &test, line) < 0) return 0;
	dataset = (int) (0.5f + test);
	fprintf(file2, "D  %i %i %i %i%.40s %i\n",
		col_x, col_dx, col_y, col_dy, filnam, dataset);
      }

    } else if (*in == '\\') {
      fprintf(file2, "%s", in);
      if (!strncmp(in + 1, "TXT", 3)) {
	while (fgets(in, 256, file1)) {
	  ++line;
	  fprintf(file2, "%s", in);
	  if (!strncmp(in, "\\ENDTXT", 7)) break;
	}
      }
    }
  }

  return 0;
} /* main */

/* ================================================================ */
int drawaxis(void)
{
  static char fmt_900[] = "V %.2f %.2f %.2f %.2f\n";

  float dlog, xa, ya, yl, xn = 0.0f, yn = 0.0f;
  float xl, xt, yt, factor, wx, wy, dex[14];
  int   j, k, n, idlog, maxnc, increment, nv;
  char  label1[80];

  /* This routine writes vector and character information to the ouput file
     to draw and label the x- or y-axis */

  factor = -1.0f;
  if (top) factor = 1.0f;
  if (xaxis) {
    /* draw and label x-axis */
    ya = window[2];
    if (top) ya = window[2] + window[3];
    yt = ya - factor * ticksize;
    if (numbers) {
      yn = ya + factor * charsize[0] * 1.2f;
      yl = ya + factor * charsize[0] * 2.0f + factor * charsize[1] * 1.2f;
    } else {
      yl = ya + factor * charsize[1] * 1.2f;
    }
    fprintf(file2, fmt_900,
	    window[0], ya, window[0] + window[1], ya);
    if (ticks) {
      grax(ascale * lox + aoffset, ascale * (lox + numx) + aoffset,
	   dex, &nv, 1);
      for (n = 0; n < nv; ++n) {
	xy_to_win((dex[n] - aoffset) / ascale, loy, &wx, &wy);
	fprintf(file2, fmt_900, wx, ya, wx, yt);
      }
      if (numbers) {
	for (n = 0; n < nv; n += 2) {
	  xy_to_win((dex[n] - aoffset) / ascale, loy, &wx, &wy);
	  if (ascale * numx < 0.00075f) {
	    sprintf(label1, "%.5f", dex[n]);
	  } else if (ascale * numx < 0.0075f) {
	    sprintf(label1, "%.4f", dex[n]);
	  } else if (ascale * numx < 0.075f) {
	    sprintf(label1, "%.3f", dex[n]);
	  } else if (ascale * numx < 0.75f) {
	    sprintf(label1, "%.2f", dex[n]);
	  } else if (ascale * numx < 7.5f) {
	    sprintf(label1, "%.1f", dex[n]);
	  } else {
	    sprintf(label1, "%.0f", dex[n]);
	  }
	  outstr(label1, wx, yn, charsize[0], 0);
	}
      }
    }
    if (strlen(label) > 0)
      outstr(label, window[0] + window[1] * 0.5f, yl, charsize[1], 0);

  } else {
    /* draw and label y-axis */
    xa = window[0];
    if (top) xa = window[0] + window[1];
    xt = xa - factor * ticksize;
    if (numbers) {
      xn = xa + factor * charsize[0];
      xl = xa + factor * charsize[0] + factor * charsize[1] * 1.2f;
    } else {
      xl = xa + factor * charsize[1] * 2.0f;
    }
    fprintf(file2, fmt_900, xa, window[2], xa, window[2] + window[3]);
    if (ticks) {
      k = 1;
      if (logscale) k = 3;
      grax(ascale * loy + aoffset, ascale * (loy + numy) + aoffset,
	   dex, &nv, k);
      for (n = 0; n < nv; ++n) {
	xy_to_win(lox, (dex[n] - aoffset) / ascale, &wx, &wy);
	fprintf(file2, fmt_900, xa, wy, xt, wy);
      }
      if (numbers) {
	maxnc = 1;
	increment = 2;
	if (logscale) increment = 1;
	for (n = 0; n < nv; n += increment) {
	  xy_to_win(lox, (dex[n] - aoffset) / ascale, &wx, &wy);
	  if (logscale &&
	      (dex[0] < 9.5e-6f || dex[nv - 1] > 99999.0f)) {
	    dlog = log10(dex[n]);
	    idlog = (int) dlog;
	    if (dlog == (float) idlog) {
	      sprintf(label1, "10{u%i}", idlog);
	    } else {
	      if (dlog < 0.0f) --idlog;
	      sprintf(label1, "%.1fx10{u%i}",
		      dex[n] / pow(10.0, (double) idlog), idlog);
	    }
	  } else if ((logscale && dex[n] < 9.5e-5f) ||
		     ascale * numy < 0.00075f) {
	    sprintf(label1, "%.5f", dex[n]);
	  } else if ((logscale && dex[n] < 9.5e-4f) ||
		     ascale * numy < 0.0075f) {
	    sprintf(label1, "%.4f", dex[n]);
	  } else if ((logscale && dex[n] < 0.0095f) ||
		     ascale * numy < 0.075f) {
	    sprintf(label1, "%.3f", dex[n]);
	  } else if ((logscale && dex[n] < 0.095f) ||
		     ascale * numy < 0.75f) {
	    sprintf(label1, "%.2f", dex[n]);
	  } else if ((logscale && dex[n] < 1.0f) ||
		     ascale * numy < 7.5f) {
	    sprintf(label1, "%.1f", dex[n]);
	  } else {
	    sprintf(label1, "%.0f", dex[n]);
	  }
	  if (logscale &&
	      (dex[0] < 9.5e-6f || dex[nv - 1] > 99999.0f)) {
	    j = strlen(label1) - 3;
	    if (maxnc < j) maxnc = j;
	  } else {
	    j = strlen(label1) - 1;
	    if (maxnc < j + 1) maxnc = j + 1;
	  }
	  outstr(label1,
		 xn + factor * 0.5f * charsize[0] * (float) (j),
		 wy, charsize[0], 0);
	}
	xl += factor * charsize[0] * (float) maxnc;
      }
    }
    if (strlen(label) > 0)
      outstr(label, xl, window[2] + window[3] * 0.5f, charsize[1], 1);
  }
  return 0;
} /* drawaxis */

/* ================================================================ */
int getfile(int *i, char *in, char *filnam, int line)
{
  int len = 0;

  /* This subroutine reads in a character string from the character string in,
     starting from character number i.
     returns length of filnam string */

  strcpy(filnam, "");
  /* get initial starting position of filnam, then get length */
  base(i, in);
  if (*i > 110) return 0;
  if (in[*i] == ',') {
    ++(*i);
    base(i, in);
    if (*i > 110) return 0;
  }
  for (len = 0; len <= 40; ++len) {
    if (in[*i + len] == ' ' ||
	in[*i + len] == '\n' ||
	in[*i + len] == ',') break;
  }
  if (len > 40) {
    printf("ERROR in filnam; max. length = 40, line %d\n", line);
    printf("%s", in);
    len = 40;
  }
  if (len > 0) {
    strncpy(filnam, in + *i, len);
    filnam[len] = '\0';
  }
  /* adjust current index to next character after filnam */
  *i += len;
  return len;
} /* getfile */

/* ==================================================================== */
int getlabel(int *i, char *in, char *label, int line)
{
  int len = 0;

  /* This subroutine reads in a character string from the character string IN,
     starting from character number I.
     returns length of label string, or -1 on error
  */

  strcpy(label, "");
  /* get initial starting position of label then get length */
  base(i, in);
  if (*i > 110) return 0;
  if (in[*i] == ',') {
    ++(*i);
    base(i, in);
    if (*i > 110) return 0;
  }
  if (in[*i] != '"') {
    printf("ERROR in label; must start and end with \", line %d\n", line);
    printf("%s", in);
    return -1;
  }
  ++(*i);
  for (len = 0; len <= 40; ++len) {
    if (in[*i + len] == '"')  break;
  }
  if (len > 40) {
    printf("ERROR in label; max. length = 40, line %d\n", line);
    printf("%s", in);
    len = 40;
  }
  if (len > 0) {
    strncpy(label, in + *i, len);
    label[len] = '\0';
    while (len > 0 && label[len - 1] == ' ') label[--len] = '\0';
  }
  /* adjust current index to next character after label */
  *i += len + 1;
  return len;
} /* getlabel */

/* ======================================================================= */
int grax(float a, float b, float *dex, int *nv, int kh)
{
  float c, x, tx, ctx;
  int   i, j, k;

  /* define tick-points for marking of axes
     a = low axis value
     b = high axis value
     dex = array of tick points
     nv = number of tick points
     kh = 1/2/3 for lin/sqrt/log axis
  */

  if (kh < 3) {
    /* linear or sqrt axis */
    c = 0.0000000001f;
    x = (b - a) / c;
    for (k = -10; k < 10; ++k) {
      if (x < 15.0f) break;
      c *= 10.0f;
      x /= 10.0f;
    }
    if (x < 1.5f) {
      tx = 0.1f;
    } else if (x < 3.0f) {
      tx = 0.2f;
    } else if (x < 7.5f) {
      tx = 0.5f;
    } else {
      tx = 1.0f;
    }
    ctx = c * tx;
    j = (int) (0.5f + a / ctx);
    c = ctx * (float) (j);
    if (c <= a + 0.1f * ctx) c += ctx;
    *nv = 0;
    while (*nv < 14) {
      dex[(*nv)++] = c;
      if ((c += ctx) >= b) return 0;
    }
    return 0;
  }

  /* logarithmic axis */
  j = (int) (log10(a) + 10.0f) - 10;
  k = (int) (log10(b) + 10.0f) - 10;
  c = pow(10.0, (double) j);
  if (k - j < 1) {
    dex[0] = c * 2.0f;
    dex[1] = c * 3.0f;
    dex[2] = c * 4.0f;
    dex[3] = c * 5.0f;
    dex[4] = c * 6.0f;
    dex[5] = c * 7.0f;
    dex[6] = c * 8.0f;
    dex[7] = c * 9.0f;
    dex[8] = c * 10.0f;
    *nv = 9;
  } else if (k - j < 2) {
    dex[0] = c * 2.0f;
    dex[1] = c * 3.0f;
    dex[2] = c * 5.0f;
    dex[3] = c * 7.0f;
    dex[4] = c * 10.0f;
    dex[5] = c * 20.0f;
    dex[6] = c * 30.0f;
    dex[7] = c * 50.0f;
    dex[8] = c * 70.0f;
    dex[9] = c * 100.0f;
    *nv = 10;
  } else if (k - j < 4) {
    dex[0] = c * 2.0f;
    dex[1] = c * 5.0f;
    dex[2] = c * 10.0f;
    dex[3] = c * 20.0f;
    dex[4] = c * 50.0f;
    dex[5] = c * 100.0f;
    dex[6] = c * 200.0f;
    dex[7] = c * 500.0f;
    dex[8] = c * 1e3f;
    dex[9] = c * 2e3f;
    dex[10] = c * 5e3f;
    dex[11] = c * 1e4f;
    *nv = 12;
  } else {
    *nv = k - j;
    if (*nv > 14) *nv = 14;
    for (i = 0; i < *nv; ++i) {
      c *= 10.0f;
      dex[i] = c;
    }
  }
  while (*nv > 0 && dex[*nv - 1] >= b) (*nv)--;
  while (*nv > 0 && dex[0] <= a) {
    (*nv)--;
    for (i = 0; i < *nv; ++i) {
      dex[i] = dex[i + 1];
    }
  }
  return 0;
} /* grax */

/*========================================================================*/
int outstr(char *label, float x, float y, float charsize, int orient)
{
  static char fmt_100[] = "C%c %8.2f %8.2f %8.2f %8.2f %s\n";

  /* outputs label along with its coordinates and the x,y character sizes.
     orient = 0 for horizontal, 1 for vertical. */

  if (strlen(label) == 0) return 0;
  if (orient == 0) {
    fprintf(file2, fmt_100, 'H',
	    x, y - charsize * 0.6f, charsize, charsize * 1.2f, label);
  } else {
    fprintf(file2, fmt_100, 'V',
	    x + charsize * 0.6f, y, charsize, charsize * 1.2f, label);
  }
  return 0;
} /* outstr */

/*=======================================================================*/
int xy_to_win(float x, float y, float *wx, float *wy)
{
/* This routine converts plot units (X,Y)
   to window coordinates (WX,WY) */

  if (logscale) {   /* logarithmic y-axis */
    *wy = window[2] + window[3] *
      (log((y > 1e-12f ? y : 1e-12f)) - log(loy)) /
      (log(numy + loy) - log(loy));
  } else {          /* linear y-axis */
    *wy = window[2] + window[3] * (y - loy) / numy;
  }
  *wx = window[0] + window[1] * (x - lox) / numx;
  if (*wx < window[0]) *wx = window[0];
  if (*wx > window[0] + window[1]) *wx = window[0] + window[1];
  if (*wy < window[2]) *wy = window[2];
  if (*wy > window[2] + window[3]) *wy = window[2] + window[3];
  return 0;
} /* xy_to_win */

/*=======================================================================*/
int data_minmax(char *fn, int col_x, int col_dx, int col_y, int col_dy, int ds, char cmd)
{
/* read through a data set (presumably to be plotted later) to determine 
   min and max values of x and y, for use in auto-scaling data axes  */

  float xin, yin, dxin, dyin, indata[20];
  int   first = 1, id, i, icol, maxcol;
  FILE  *f_in;
  char  line[256];

  /* open external data file */
  if (!(f_in = fopen(fn, "r"))) {
    printf("Error: File %s does not exist?\n", fn);
    return 1;
  }
  /* skip forward (dataset - 1) data sets */
  for (id = 0; id < ds - 1; ++id) {
    while (1) {
      if (!fgets(line, 256, f_in)) {
        printf("Error: Dataset %d does not exist in file %s?\n", ds, fn);
        return 1;
      }
      if (!strncmp(line, "ENDDATA", 7)) break;
      for (i = 0; line[i] == ' '; i++);
      if (line[i] == '\n') break;
    }
  }

  /* read data points */
  first = 1;
  maxcol = col_x;
  if (maxcol < col_y)  maxcol = col_y;
  if (maxcol < col_dx) maxcol = col_dx;
  if (maxcol < col_dy) maxcol = col_dy;
  if (maxcol > 20) {
    printf("ERROR: Column number too high; maximum is 20!\n");
    return 1;
  }

  while (1) {
    /* read data from external data file */
    if (!fgets(line, 256, f_in)) break;
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
      if (get(&i, line, &indata[icol], 0) < 0) {
        printf("ERROR: Cannot deode data in file %s %d\n", fn, ds);
        return 1;
      }
    }
    dxin = 0;
    dyin = 0;
    xin = indata[col_x - 1];
    yin = indata[col_y - 1];
    if (col_dx > 0) dxin = indata[col_dx - 1];
    if (col_dy > 0) dyin = indata[col_dy - 1];

    if (cmd == 'm' && first) {
      dminx = xin - dxin;
      dminy = yin - dyin;
      dmaxx = xin + dxin;
      dmaxy = yin + dyin;
    } else {
      if (dminx > xin - dxin) dminx = xin - dxin;
      if (dminy > yin - dyin) dminy = yin - dyin;
      if (dmaxx < xin + dxin) dmaxx = xin + dxin;
      if (dmaxy < yin + dyin) dmaxy = yin + dyin;
    }
    first = 0;
  }

  fclose(f_in);
  if (0) printf(" >>>  min x,y: %f %f; max x,y: %f %f\n",
                dminx, dminy, dmaxx, dmaxy);
  if (dminx == dmaxx) {
    dminx *= 0.5;
    dmaxx *= 1.5;
  }
  if (dminy == dmaxy) {
    dminy *= 0.5;
    dmaxy *= 1.5;
  }
  if (dminx == dmaxx) {
    dminx -= 0.005;
    dmaxx += 0.005;
  }
  if (dminy == dmaxy) {
    dminy -= 0.005;
    dmaxy += 0.005;
  }
  if (0) printf(" <<<  min x,y: %f %f; max x,y: %f %f\n",
                dminx, dminy, dmaxx, dmaxy);
  return 0;
}
