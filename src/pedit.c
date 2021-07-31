#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "minig.h"
#include "util.h"
extern Display   *disp_id;
extern Screen    *screen_id;
extern Window    root_win_id, win_id;
extern GC        gc_id;
extern XEvent    event;
extern int       color[20], ixp, iyp;

extern int drawstring(char *, int, char, float, float, float, float);
extern int get(int *i, char *in, float *result, int line);
extern void init_menu(int nmenu, int nchars, int nlines, char *choices,
		      char *label, int x_loc, int y_loc);
extern void delete_menu(int nmenu);
extern void get_selection(int nmenu, int *item_sel, int *menu_sel,
			  int *ibutton, int *nopts, int *menu_up);
extern int ds_moveto(float, float);
extern int ds_rmoveto(float, float);
extern int ds_set_lwfact(float);

float xzero, hix, yzero, hiy, xmin, xmax, ymin, ymax, spec[16384];
float window[4], lox, numx, loy, numy;
int   numch, logscale, nrec;
FILE  *file1, *file2, *file3, *specfile;

char  saved_lines[100][256];
int   saved_recs[100], nsaved = 0;

int g_disp_id, g_screen_id, g_root_win_id, g_win_id, g_gc_id, g_win_width;
int g_win_height, g_color[16];
extern int nowatx, nowaty, menu_mode;

int drawplot(void);
int peaks(void);
int findpeaks(void);
int pfind(float *chanx, float *psize, int n1, int n2,
	  int ifwhm, float sigma, int maxpk, int *npk);
int pmenu(char *menu_ans);
int editlab(char *ans);
int editvect(char *ans);
int xy_to_win(float x, float y, float *wx, float *wy);
int zoom(char *ans);
int pick_color(float *r, float *g, float *b);
int save_for_undo(int irec, char *save_line);
int replace_line(int irec, char *save_line);
int undo(void);
int rmsread(char *fn, float *sp, char *namesp, int *numch, int idimsp);

/* ====================================================================== */
int main(int argc, char **argv)
{
  float flen, a1, a2, a3, a4, c1, c2, dx, dy, r, g, b;
  int   irec, nsx, nsy, copytext = 0;
  char  line[256], filnam[80], fn2[80], ans[80];

  /* program to display and edit spectrum plots from PLOTSPEC and PLOTDATA */

  /* set menu_mode to true for subroutine set_focus */
  menu_mode = 1;

  /* welcome and get data file */
  set_xwg("p?editx_");
  initg(&nsx, &nsy);
  finig();
  printf("\n\n  Welcome to pedit\n\n"
	 "   Version 2.1   D. C. Radford    Dec 2001\n\n");

  /* ask for data file name and open file */
 OPENNEW:
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      cask("Data file = ? (default .ext = .pdg, .psg)", filnam, 80);
    }
    strcpy(fn2, filnam);
    setext(filnam, ".pdg", 80);
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

  /* copy file to direct access .tmp file */
  file2 = tmpfile();

 RELOAD:
  nrec = 0;
  while (fgets(line, 256, file1)) {
    fwrite(line, 256, 1, file2);
    nrec++;
  }
  rewind(file1);

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

  fclose(file1);
  dx = (xmax - xmin) * 1.1f;
  dy = (ymax - ymin) * 1.1f;
  xzero = xmin - dx * 0.05f;
  yzero = ymin - dy * 0.05f;
  hix = dx + xzero;
  hiy = dy + yzero;

  /* draw plot */
  if (drawplot()) goto ERR;

  while (1) {
    pmenu(ans);
    if (ans[1] == 'V' || ans[1] == 'R') {
      if (editvect(ans) || drawplot()) goto ERR;
    } else if (ans[1] == 'L' || ans[1] == 'T') {
      if (editlab(ans) || drawplot()) goto ERR;
    } else if (*ans == 'P') {
      peaks();
    } else if (*ans == 'F') {
      findpeaks();
    } else if (*ans == 'Z' || *ans == 'E') {
      zoom(ans);
      drawplot();
    } else if (*ans == 'C') {
      pick_color(&r, &g, &b);
      sprintf(line, "\\RGB %.2f %.2f %.2f\n", r, g, b);
      replace_line(nrec++, line);
    } else if (*ans == 'U') {
      undo();
    } else if (*ans == 'O') {
      /* if (!caskyn("Are you sure? (Y/N)")) continue; */
      erase();
      fclose(file2);
      argc = 1;
      goto OPENNEW;
    } else if (*ans == 'R') {
      /* if (!caskyn("Are you sure? (Y/N)")) continue; */
      erase();
      rewind(file2);
      if ((file1 = fopen(filnam, "r"))) {
	printf("Reopened file %s\n", filnam);
	goto RELOAD;
      }
      file_error("open", filnam);
      return 1;
    } else if (*ans == 'X') {
      if (!caskyn("Are you sure you want to exit? (Y/N)")) continue;
      erase();
      /* copy direct access .tmp file to new file */
      if ((file1 = open_new_file(filnam, 0))) {
	rewind(file2);
	for (irec = 0; irec < nrec; ++irec) {
	  fread(line, 256, 1, file2);
	  if (strlen(line) > 0) fputs(line, file1);
	}
	fclose(file1);
      }
      fclose(file2);
      save_xwg("p?editx_");
      return 0;
    }
  }

ERR:
  if (file1) fclose(file1);
  fclose(file2);
  file_error("read", filnam);
  return -1;
} /* main */

/*========================================================================*/
int drawplot(void)
{
  static float symbolsize = 5.0f;
  static float fline[4] = { 0.f,0.f,0.f,0.0f };
  static float flinesum[4] = { 0.f,0.f,0.f,0.0f };
  static int   symbol = 1, joinflag = 1, color = 0, symbol2 = 1;

  float x[10], y[10], x1, x2, ch, dx, dy, a1, a2, a3, a4;
  float r1, dxin, dyin, d, fdist, fd, fx, indata[20], fy;
  float wx, wy, wx1, wy1, wx2, wy2, lwfact, csx, csy, xin, yin;
  float linesp = 1.7f;
  int   dataset, irec = 0, icol, even, i, j, k, ich, ncs, nsx, nsy, jline;
  int   first, id, np, col_dx, col_dy, col_x, col_y, maxcol, isize;
  int   copytext = 0, lw0, lw1, lw2;
  char  line[256], filnam[80], spfilnam[80], namesp[8], label[80], *c;
  short sspec[4096];
  int   ispec[8192], spnum;

/* subroutine to display plots on a graphics terminal */
/* linear y-axis, but no axes drawn */
/* erase graphics screen, define axes */
  dx = hix - xzero;
  dy = hiy - yzero;
  initg(&nsx, &nsy);
  erase();
  setcolor(1);
  limg(nsx, 0, nsy, 0);
  trax(dx, xzero, dy, yzero, 0);
  
  lwfact = 1.3f * (((float) nsx)/dx <  ((float)nsy)/dy ?
	    ((float) nsx)/dx : ((float)nsy)/dy);
  ds_set_lwfact(lwfact);
  lw0 = 0.7f * lwfact + 0.5f;
  lw1 = lw2 = lw0;
  set_minig_line_width(lw0);

/* read, decode and output lines from file */
  rewind(file2);
  for (irec = 0; irec < nrec; ++irec) {
    fread(line, 256, 1, file2);
    if (strlen(line) == 0) continue;

    if (copytext && *line != '\\') {
/* ------------------------------ copy text to output */
      if (csx <= 0.0f || csy <= 0.0f) continue;
      /* parse input line, looking for {\...} commands;
	 output remaining characters as strings */
      i = j = 0;
      k = strlen(line) - 1;
      ds_moveto(*x, *y);
      while (i < k) {
	if (!strncmp(line + i, "\{\\", 2)) {
	  /* output string */
	  if ((ncs = i - j) > 0) drawstring(line, ncs, 'X', *x, *y, csx, csy);
	  /* ---------- execute command in {\...} */
	  i += 2;
	  if (!strncmp(line + i, "RGB", 3)) {
	    /* define new color */
	    if (sscanf(line + 4, "%f%f%f", &a1, &a2, &a3) != 3) goto ERR;
	    set_minig_color_rgb(17, a1, a2, a3);
	  } else if (line[i] == 'X' || line[i] == 'Y') {
	    /* relative move */
	    if (sscanf(line + i + 1, "%f", &a1) != 1) goto ERR;
	    if (line[i] == 'Y') {
	      ds_rmoveto(0.0f, a1*csy);
	      y[0] += a1*csy;
	    } else {
	      ds_rmoveto(a1*csx, 0.0f);
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
      if ((ncs = k - j) > 0) drawstring(line, ncs, 'X', *x, *y, csx, csy);
      y[0] -= linesp*csy;

    } else if (*line == 'V') {
/* ------------------------------ vector */
      np = sscanf(line + 1, "%f%f%f%f%f%f%f%f%f%f",
		  &x[0], &y[0], &x[1], &y[1], &x[2],
		  &y[2], &x[3], &y[4], &x[4], &y[4]);
      if (np < 4 || np%2 != 0) return 1;
      set_minig_line_width(lw2);
      pspot(x[0], y[0]);
      for (i = 1; i < np/2; ++i) {
	vect(x[i], y[i]);
      }

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
      fread(line, 256, 1, file2);
      irec++;
      if (sscanf(line + 1, "%f%f%f%f",
		 &window[0], &window[1],
		 &window[2], &window[3]) != 4) return 1;
      fread(line, 256, 1, file2);
      irec++;
      if (sscanf(line + 1, "%f%f%f%f",
		 &lox, &numx, &loy, &numy) != 4) return 1;

      if (strstr(spfilnam, ".spn") ||
	  strstr(spfilnam, ".mat") ||
	  strstr(spfilnam, ".sec")) {
	if (!(specfile = fopen(spfilnam, "r"))) return 1;
	if (strstr(spfilnam, ".spn")) {
	  fseek(specfile, spnum*4096*sizeof(int), SEEK_SET);
	  if (fread(ispec, 16384, 1, specfile) != 1) return 1;
	  for (j=0; j<4096; j++) spec[j] = ispec[j];
	  numch = 4096;
	  // printf(" read spn file %s, sp # %d\n", spfilnam, spnum);
	} else if (strstr(spfilnam, ".mat")) {
	  fseek(specfile, spnum*8192, SEEK_SET);
	  if (fread(sspec, 8192, 1, specfile) != 1) return 1;
	  for (j=0; j<4096; j++) spec[j] = sspec[j];
	  numch = 4096;
	} else if (strstr(spfilnam, ".sec")) {
	  fseek(specfile, spnum*32768, SEEK_SET);
	  if (fread(ispec, 32768, 1, specfile) != 1) return 1;
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
	return 1;
      }

      set_minig_line_width(lw1);

      x1 = lox + (xzero - window[0]) * numx / window[1];
      if (x1 < lox) x1 = lox;
      if (x1 > lox + numx) x1 = lox + numx;
      x2 = lox + (hix - window[0]) * numx / window[1];
      if (x2 > lox + numx) x2 = lox + numx;
      if (x2 < x1) x2 = x1;
      ch = x1;
      xy_to_win(ch, spec[(int) (0.5f + x1)], &wx, &wy);
      pspot(wx, wy);
      for (ich = (int) (0.5f + x1); ich <= (int) (0.5f + x2); ++ich) {
	xy_to_win(ch, spec[ich], &wx, &wy);
	vect(wx, wy);
	ch += 1.0f;
	xy_to_win(ch, spec[ich], &wx, &wy);
	vect(wx, wy);
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
      sscanf(line+3, "%f %f", &a1, &a2);
      lw1 = a1*lwfact + 0.5f;
      lw2 = a2*lwfact + 0.5f;
      if (lw1 <= 0.0f) lw1 = lw0;
      if (lw2 <= 0.0f) lw2 = lw0;

    } else if (!strncmp(line, "LIN", 3) || !strncmp(line, "LOG", 3)) {
/* ------------------------------ data plot */
      /* test to see if y-axis should be log scale */
      logscale = 0;
      if (!strncmp(line, "LOG", 3)) logscale = 1;

      /* get window limits,lox,numx,loy,numy */
      if (sscanf(line + 4, "%f%f%f%f",
		 &window[0], &window[1], &window[2], &window[3]) != 4)
	goto ERR;
      fread(line, 256, 1, file2);
      irec++;
      if (sscanf(line + 1, "%f%f%f%f",
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
	    if (line[i] == '\n' || line[i] == '\r') break;
	  }
	}
      }
      /* setcolor(color + 1); */

      /* calculate symbol size in graphics pixels */
      r1 = xzero + symbolsize;
      cvxy(&r1, &yzero, &isize, &j, 1);
      if (isize < 5) isize = 5;
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
	  fread(line, 256, 1, file2);
	  ++irec;
	} else {
	  /* read data from external data file */
	  if (!fgets(line, 256, file3)) break;
	}
	if (!strncmp(line, "ENDDATA", 7)) break;
	for (i = 0; line[i] == ' '; i++);
	if (line[i] == '\n' || line[i] == '\r') break;
	/* lines not beginning with ',' through '9', or with ' ',
	   are considered to be comments; ignore them */
	if (*line != ' ' && (*line < ',' || *line > '9')) continue;

	/* decode data */
	i = 0;
	for (icol = 0; icol < maxcol; ++icol) {
	  indata[icol] = 0.0f;
	  if (get(&i, line, &indata[icol], irec) < 0) goto ERR2;
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
	xy_to_win(xin, yin, &wx, &wy);
	if (!first && joinflag != 0) {
	  /* draw joining vector */
	  set_minig_line_width(lw1);
	  if (fline[0] <= 0.0f) {
	    pspot(wx1, wy1);
	    vect(wx, wy);
	  } else {
	    if (jline == 1 || jline == 3) pspot(wx, wy);
	    xy_to_win(xin, yin, &wx, &wy);
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
		vect(wx2, wy2);
	      } else {
		pspot(wx2, wy2);
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
	if (symbol > 0 &&
	    wx >= xzero && wx <= xzero + dx &&
	    wy >= yzero && wy <= yzero + dy) {
	  set_minig_line_width(lw2);
	  if (even) symbg(symbol2, wx, wy, isize);
	  if (!even) symbg(symbol, wx, wy, isize);
	}
	if (dyin > 0.0f) {
	  /* draw error bar */
	  set_minig_line_width(lw2);
	  xy_to_win(xin, yin + dyin, &wx, &wy);
	  pspot(wx - symbolsize / 2.0f, wy);
	  vect(wx + symbolsize / 2.0f, wy);
	  pspot(wx, wy);
	  xy_to_win(xin, yin - dyin, &wx, &wy);
	  vect(wx, wy);
	  pspot(wx - symbolsize / 2.0f, wy);
	  vect(wx + symbolsize / 2.0f, wy);
	  xy_to_win(xin, yin, &wx, &wy);
	}
	if (dxin > 0.0f) {
	  /* draw error bar */
	  set_minig_line_width(lw2);
	  xy_to_win(xin + dxin, yin, &wx, &wy);
	  pspot(wx, wy - symbolsize / 2.0f);
	  vect(wx, wy + symbolsize / 2.0f);
	  pspot(wx, wy);
	  xy_to_win(xin - dxin, yin, &wx, &wy);
	  vect(wx, wy);
	  pspot(wx, wy - symbolsize / 2.0f);
	  vect(wx, wy + symbolsize / 2.0f);
	  xy_to_win(xin, yin, &wx, &wy);
	}
      }
      if (strncmp(filnam, "    ", 4)) fclose(file3);
      /* setcolor(1); */

    } else if (*line == 'C') {
/* ------------------------------ label */
      /* decode position and string */
      if (sscanf(line + 2, "%f%f%f%f",
		 &x[0], &y[0], &csx, &csy) != 4) return 1;
      if (csx <= 0.0f || csy <= 0.0f) continue;
      strcpy(label, line + 39);
      ncs = strlen(label) - 1;
      /* output string */
      drawstring(label, ncs, line[1], *x, *y, csx, csy);

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
	\SP f: set vertical line spacing to f* default
      */

      if (!strncmp(line, "\\RF", 3)) {
/* ------------------------------ filled rectangle */
	if (sscanf(line + 4, "%f%f%f%f", &a1, &a2, &a3, &a4) != 4) goto ERR;
	set_minig_line_width(lw2);
	pspot(a1, a3);
	vect(a1, a3+a4);
	vect(a1+a2, a3+a4);
	vect(a1+a2, a3);
	vect(a1, a3);
	minig_fill();
      } else if (!strncmp(line, "\\RGB", 4)) {
/* ------------------------------ define new color */
	if (sscanf(line + 5, "%f%f%f", &a1, &a2, &a3) != 3) goto ERR;
	set_minig_color_rgb(17, a1, a2, a3);
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
  finig();
  return 0;

/* error messages */
ERR2:
  file_error("open or read", filnam);
ERR:
  printf("Line number %d\n", irec+1);
  return 1;
} /* drawplot */

/*========================================================================*/
int peaks0(int pkfind)
{
  static float a = 0.0f, b = 0.5f, charsiz = 3.0f;
  static float fwhm = 6.0f, sigma = 8.0f, percent = 5.0f;

  float pmax, e, h, x = 0.0f, y, chanx[500], psize[500], x1, y1;
  float r1, r2, fj, wx, wy, fj2, ref = 0.0f;
  int   irec, iphi, iplo, vertical, j, check = 1, nspec, ifwhm;
  int   maxpk, j1, j2, nc, idspec, length, ich;
  int   ipk = 0, npk, nsx, nsy;
  char  line[256], junk[80], spfilnam[80], namesp[8];
  char  label[80], ans[80], q[80], *c;
  short sspec[4096];
  int   ispec[8192], spnum;

 START:
  while ((nc = cask(" Label peaks in spectrum number = ?", ans, 80)) &&
	 inin(ans, nc, &idspec, &j1, &j2));
  if (nc == 0) return 0;
  nspec = 0;
  rewind(file2);
  for (irec = 0; irec < nrec; ++irec) {
    fread(line, 256, 1, file2);
    if (strncmp(line, "SLIN", 4) && strncmp(line, "SLOG", 4)) continue;
    ++nspec;
    if (nspec < idspec) continue;
    /* test to see if y-axis should be log scale */
    logscale = 0;
    if (!strncmp(line, "SLOG", 4)) logscale = 1;
    /* get spectrum file name,window,lox,numx,loy,numy */
    strncpy(spfilnam, line + 5, 40);
    if (sscanf(line + 45, "%d", &spnum) != 1) spnum = 0;
    if ((c = strchr(spfilnam, ' ')) ||
	(c = strchr(spfilnam, '\n'))) *c = '\0';
    fread(line, 256, 1, file2);
    sscanf(line + 1, "%f%f%f%f",
	   &window[0], &window[1], &window[2], &window[3]);
    fread(line, 256, 1, file2);
    sscanf(line + 1, "%f%f%f%f", &lox, &numx, &loy, &numy);
    if (strstr(spfilnam, ".spn") ||
	strstr(spfilnam, ".mat") ||
	strstr(spfilnam, ".sec")) {
      specfile = fopen(spfilnam, "r");
      if (strstr(spfilnam, ".spn")) {
	fseek(specfile, spnum*16384, SEEK_SET);
	fread(ispec, 16384, 1, specfile);
	for (j=0; j<4096; j++) spec[j] = ispec[j];
	numch = 4096;
      } else if (strstr(spfilnam, ".mat")) {
	fseek(specfile, spnum*8192, SEEK_SET);
	fread(sspec, 8192, 1, specfile);
	for (j=0; j<4096; j++) spec[j] = sspec[j];
	numch = 4096;
      } else if (strstr(spfilnam, ".sec")) {
	fseek(specfile, spnum*32768, SEEK_SET);
	fread(ispec, 32768, 1, specfile);
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
    } else {
      read_spe_file(spfilnam, spec, namesp, &numch, 16384);
    }
  }
  if (nspec < idspec) {
    printf("ERROR: No such spectrum....\n");
    goto START;
  }

  printf("Default energy calibration is (offset, keV/ch): %.5f %.5f\n", a, b);
  while ((nc = cask(" Energy calibration = ? (rtn for default)", ans, 80)) &&
	 (ffin(ans, nc, &r1, &r2, &fj) || r2 <= 0.0f));
  if (nc != 0) {
    a = r1;
    b = r2;
  }
  vertical = caskyn("Put peak labels vertically? (Y/N)");

  printf("Default character size is: %.5f\n", charsiz);
  while ((nc = cask(" Character size = ? (rtn for default)", ans, 80)) &&
	 (ffin(ans, nc, &r1, &fj, &fj2) || r1 <= 0.0f));
  if (nc != 0) charsiz = r1;

  if (pkfind) {
    printf("Default FWHM, Sigma, Percent = %f, %f, %f\n",
	   fwhm, sigma, percent);
    while ((nc = cask("...Enter FWHM, Sigma, Percent (rtn for default)",
		      ans, 80)) &&
	   ffin(ans, nc, &fwhm, &sigma, &percent));

    while (1) {
      nc = cask(" Min, Max chs for peak search = ?\n"
		"           (rtn for whole display)", ans, 80);
      iplo = 0;
      iphi = numch - 1;
      if (nc == 0 ||
	  (!inin(ans, nc, &iplo, &iphi, &j) &&
		 iplo <= (int) (0.5f + lox + numx - fwhm * 2.0f) &&
		 iphi >= (int) (0.5f + lox + fwhm * 2.0f)))
	break;
    }
    if (iplo < (int) (0.5f + lox - fwhm)) iplo = (int) (0.5f + lox - fwhm);
    if (iphi > (int) (0.5f + lox + numx + fwhm))
      iphi = (int) (0.5f + lox + numx + fwhm);

    /* do peak search */
    maxpk = (float) (iphi - iplo + 1) * 2.355f / fwhm;
    if (maxpk > 500) maxpk = 500;
    ifwhm = (int) (0.5f + fwhm);
    pfind(chanx, psize, iplo, iphi, ifwhm, sigma, maxpk, &npk);
    pmax = -100.0f;
    for (ipk = 0; ipk < npk; ++ipk) {
      if (pmax < psize[ipk]) pmax = psize[ipk];
    }
    ref = percent * pmax / 100.0f;
    ipk = 0;
  }

  while (1) {
    while (!pkfind) {
      printf("Peak position = ?"
	     " (press X to exit, U to move up, D to move down)\n");
      initg(&nsx, &nsy);
      retic(&wx, &wy, ans);
      finig();
      if (*ans == 'X' || *ans == 'x') return 0;
      if (*ans == 'U' || *ans == 'u') {
	printf(" ...Lower channel = ?\n");
	retic(&x1, &y1, junk);
	hix = hix + x1 - xzero;
	xzero = x1;
      } else if (*ans == 'D' || *ans == 'd') {
	printf(" ...Upper channel = ?\n");
	retic(&x1, &y1, junk);
	xzero = xzero + x1 - hix;
	hix = x1;
      } else {
	/* find channel number and energy */
	x = lox + (wx - window[0]) * numx / window[1];
	break;
      }
      /* redraw spectrum with display moved up or down */
      if (drawplot()) return 1;
      /* now need to restore logscale, window etc */
      fseek(file2, irec*256, SEEK_SET);
      fread(line, 256, 1, file2);
      logscale = 0;
      if (!strncmp(line, "SLOG", 4)) logscale = 1;
      fread(line, 256, 1, file2);
      sscanf(line, "%f%f%f%f",
	     &window[0], &window[1], &window[2], &window[3]);
      fread(line, 256, 1, file2);
      sscanf(line, "%f%f%f%f", &lox, &numx, &loy, &numy);
    }

    while (pkfind) {
      if (ipk >= npk) return 0;
      x = chanx[ipk] - 1.0f;
      if (psize[ipk++] >= ref) break;
    }

    /* write energy into default label,
       remove leading blanks and find length */
    e = a + b * x;
    sprintf(label, "%.0f", e);
    length = strlen(label);
    if (pkfind && check) {
      sprintf(q, "Label %s? (Y/N/Go/Quit)", label);
    GETL:
      nc = cask(q, ans, 1);
      if (nc == 0 || *ans == 'N' || *ans == 'n' || *ans == '0') {
	continue;
      } else if (*ans == 'G' || *ans == 'g') {
	check = 0;
      } else if (*ans == 'Q' || *ans == 'q') {
	return 0;
      } else if (*ans != 'Y' && *ans != 'y' && *ans != '1') {
	goto GETL;
      }
    }
    if (check) {
      /* Writing concatenation */
      sprintf(q, "...Enter label (rtn for %s)", label);
      if ((nc = cask(q, ans, 40))) {
	strcpy(label, ans);
	length = nc;
      }
    }
    y = 0.0f;
    ich = (int) (0.5f + x);
    if (ich >= 0 && ich < numch) y = spec[ich];
    if (ich + 1 >= 0 && ich + 1 < numch && y < spec[ich + 1])
      y = spec[ich + 1];
    if (ich - 1 >= 0 && ich - 1 < numch && y < spec[ich - 1])
      y = spec[ich - 1];
    xy_to_win(x, y, &wx, &wy);
    if (vertical) {
      strcpy(line, "CV");
      wx += charsiz * .6f;
      wy += charsiz * (float) (length + 3) * 0.5f;
      h = wy + charsiz * (float) (length + 2) * 0.5f;
      if (h > window[2] + window[3]) {
	wx -= charsiz;
	wy = window[2] + window[3] - charsiz * (float) (length + 3) * 0.5f;
      }
    } else {
      strcpy(line, "CH");
      wy += charsiz * 1.5f;
      h = wy + charsiz;
      if (h > window[2] + window[3]) {
	wx -= charsiz * (float) (length + 1) * 0.5f;
	wy = window[2] + window[3] - charsiz * 2.7f;
      }
    }
    /* display new label */
    initg(&nsx, &nsy);
    r1 = charsiz * 1.2f;
    drawstring(label, length, line[1], wx, wy, charsiz, r1);
    finig();
    /* write new line to file */
    sprintf(line+2, " %8.2f %8.2f %8.2f %8.2f %s\n",
	    wx, wy, charsiz, charsiz * 1.2f, label);
    replace_line(nrec++, line);
  }
} /* peaks0 */

int peaks(void)
{
  return peaks0(0);
}

int findpeaks(void)
{
  return peaks0(1);
}

/* ====================================================================== */
int pfind(float *chanx, float *psize, int n1, int n2,
	  int ifwhm, float sigma, int maxpk, int *npk) {
  float deno, save[16384], root, peak1, peak2, peak3;
  float p1, p2, p3, s2, w4, pc, sum, sum2;
  int   iadd, jodd;
  int   lock, miny, i, j, n, wings, j1, j2, jd, nd;
  int   jl, jr, np, ctr;

/* **************************************************************** */
/* PEAK-FINDER ROUTINE    ORNL 5/4/76 JDL. */
/* N1      = CHANNEL # AT START OF SCAN */
/* N2      = CHANNEL # AT STOP  OF SCAN */
/* IFWHM   = FWHM ESTIMATE SUPPLIED BY CALLING PROG */
/* SIGMA   = PEAK DETECTION THRESHOLD (STD DEV ABOVE BKG) */
/* SPEC(I) = DATA ARRAY TO BE SCANNED */
/* CHANX(I)= LOCATION OF ITH PEAK (INDEX BASIS, NOT CHAN# BASIS) */
/* PSIZE(I)= VERY ROUGH ESTIMATE OF PEAK AREA (MAY BY WAY! OFF!!) */
/* **************************************************************** */

  *npk = 0;
  np = 0;
  n = 0;
  lock = 0;
  p1 = 0.0f;
  p2 = 0.0f;
  p3 = 0.0f;
  peak1 = 0.0f;
  peak2 = 0.0f;
  peak3 = 0.0f;
  sum = 0.0f;
  sum2 = 0.0f;
  if (ifwhm < 1) ifwhm = 1;
  j1 = -((2*ifwhm) - 1) / 4;
  j2 = j1 + ifwhm - 1;
  jodd = ifwhm % 2;
  /* tst for any negative, if yes add to data to make all positive */
  miny = 0;
  iadd = 0;
  for (i = n1; i <= n2; ++i) {
    if (spec[i] < (float) miny) miny = spec[i];
  }
  if (miny < 0) {
    iadd = -miny;
    for (i = n1; i <= n2; ++i) {
      save[i] = spec[i];
      spec[i] += iadd;
    }
  }
  /* now go ahead and do the peakfind bit */
  for (nd = n1 + ifwhm; nd < n2 - ifwhm; ++nd) {
    /* sum counts in center and wings of differential function */
    ctr = 0;
    wings = 0;
    for (j = j1; j <= j2; ++j) {
      jd = nd + j;
      ctr += spec[jd];
      if (j <= 0) continue;
      jl = nd - j + j1;
      jr = jd + j2;
      wings = wings + spec[jl] + spec[jr];
    }
    /* if ifwhm is odd, average data in split cells at ends */
    w4 = 0.0f;
    if (jodd == 0) {
      jl = nd + j1 + j1;
      jr = nd + j2 + j2;
      w4 = (spec[jl - 1] + spec[jl] + spec[jr] + spec[jr + 1]) * 0.25f;
    }
    /* compute height of second derivative (neg) relative to noise */
    s2 = sum;
    sum = (float) (ctr - wings) - w4;
    root = sqrt((float) (ctr + wings + 1) + w4);
    p1 = p2;
    p2 = p3;
    deno = root;
    if (deno < 1e-6f) deno = 1e-6f;
    p3 = sum / deno;
    if (lock) {
      if (p2 > peak2 && p3 < p2) {
	/* save three values at relative maximum */
	peak1 = p1;
	peak2 = p2;
	peak3 = p3;
	sum2 = s2;
	np = nd - 1;
      }
      if (p3 < sigma) {
	/* estimate location and crude size of peak */
	if (++n > maxpk) n = maxpk;
	deno = peak1 - peak2 + peak3 - peak2;
	if (fabs(deno) < 1e-6f) deno = 1e-6f;
	pc = (peak1 - peak3) * 0.5f / deno;
	chanx[n - 1] = (float) np + pc + (float) jodd * 0.5f;
	psize[n - 1] = sum2 + sum2;
	lock = 0;
	peak2 = 0.0f;
      }
    }
    if (p3 >= sigma) lock = 1;
  }
  *npk = n;
  /* tst for offset added - i.e. is data restore required */
  if (iadd != 0) {
    for (i = n1; i <= n2; ++i) {
      spec[i] = save[i];
    }
  }
  return 0;
} /* pfind */

/* ======================================================================= */
int pmenu(char *menu_ans)
{
  static int   menu_up[4] = { 0,0,0,0 }, nopts[4] = { 11,8,5,0 };
  static int   nchars[4]  = { 20,17,18,0 };
  static int   x_loc[4]   = { 0,-2,-2,-2 };
  static int   y_loc[4]   = { 100,-1,-1,-1 }, focus_id = 0;
  static char  title[4][16] =
  { "Main Menu","Vectors","Labels",""};
  static char  options[4][11][24] =
  { { "Vectors","Labels","Label Peaks w/Cursor","Find & Label Peaks",
      "Zoom/Expand Display","UnZoom Display","Pick color",
      "Undo", "Open new file", "Reload file", "Exit Program" },
    { "Move Vectors","Delete Vectors","Add Vectors","Break Vectors",
      "Fill Rectangles","Move Rectangles","Delete Rectangles",
      "Resize Rectangle" ,"","" },
    { "Move Text/Labels","Delete Text/Labels","Add Labels","Add Text",
      "Resize Text/Labels","","","","","" },
    { "","","","","","","","","","" } };
  static char  ans_return[4][11][4] =
  { { "ME1","ME2","P ","F ","E ","Z ","C ","U ","O ","R ","X "},
    { "MV","DV","AV","BV","FR","MR","DR","RR","  ","  ","  "},
    { "ML","DL","AL","AT","RL","  ","  ","  ","  ","  ","  "},
    { "  ","  ","  ","  ","  ","  ","  ","  ","  ","  ","  "} };

  int   item_sel, menu_sel, i, nmenu, ibutton;

/* display menu of options */
  nmenu = 0;
  if (menu_up[nmenu] == 0) {
    init_menu(nmenu, nchars[nmenu], nopts[nmenu], options[nmenu][0],
	      title[nmenu], x_loc[nmenu], y_loc[nmenu]);
    menu_up[nmenu] = 1;
  }

/* set input focus to last used menu window */
  if (focus_id != 0) set_focus(focus_id);
  while (1) {
    nmenu = 0;
    get_selection(nmenu, &item_sel, &menu_sel, &ibutton, nopts, menu_up);
    strcpy(menu_ans, ans_return[menu_sel][item_sel]);
    if (strncmp(menu_ans, "ME", 2)) {
      /* save information about current input focus menu window */
      get_focus(&focus_id);
      return 0;
    }
    for (i = 1; i < 4; ++i) {
      if (menu_up[i] == 1) delete_menu(i);
      menu_up[i] = 0;
    }
    nmenu = menu_ans[2] - '0';
    init_menu(nmenu, nchars[nmenu], nopts[nmenu], options[nmenu][0],
	      title[nmenu], x_loc[nmenu], y_loc[nmenu]);
    menu_up[nmenu] = 1;
  }
} /* psmenu */

/*========================================================================*/
int editlab(char *ans)
{
  float x[5], y[5], x1, y1, x2, y2, fj1, fj2, xhi, yhi, csx, csy, xlo, ylo;
  float a3, a4;
  float linesp = 1.7f;
  int   irec, nc, mda, ncs, nsx, nsy, txtrec = 0, copytext = 0, j;
  char  line[256], jc[4], label[256];

  if (*ans == 'M') {
    mda = 1;
  } else if (*ans == 'D') {
    mda = 2;
  } else if (!strncmp(ans, "AL", 2)) {
    mda = 3;
  } else if (!strncmp(ans, "AT", 2)) {
    mda = 4;
  } else if (*ans == 'R') {
    mda = 5;
  } else {
    return 0;
  }

  while (mda < 3 || mda == 5) {
    printf("Select text or label, press X to exit...\n");
    initg(&nsx, &nsy);
    retic(&x1, &y1, jc);
    finig();
    if (*jc == 'X' || *jc == 'x') return 0;

    /* read, decode and output lines from file */
    rewind(file2);
    for (irec = 0; irec < nrec; ++irec) {
      fread(line, 256, 1, file2);
      if (copytext && *line != '\\' && *line != '\0') {
	if (csx <= 0.0f || csy <= 0.0f) continue;
	ncs = strlen(line) - 1;
	if (ncs < 1) ncs = 1;
	xlo = x1 - csx * 0.5f * (float) ncs;
	xhi = x1;
	ylo = y1 - csy;
	yhi = y1;
	if (x[0] < xlo || x[0] > xhi ||
	    y[0] < ylo || y[0] > yhi) {
	  y[0] -= linesp*csy;
	  continue;
	}

	if (mda == 1) {
	  /* move label */
	  printf("New position = ?\n");
	  initg(&nsx, &nsy);
	  retic(&x2, &y2, jc);
	  /* rewrite line with new position */
	  fseek(file2, txtrec*256, SEEK_SET);
	  fread(line, 256, 1, file2);
	  sscanf(line + 5, "%f%f", &x[0], &y[0]);
	  x[0] = x[0] + x2 - x1;
	  y[0] = y[0] + y2 - y1;
	  sprintf(line + 5, " %.2f %.2f %.2f %.2f\n",
		  x[0], y[0], csx, csy);
	  replace_line(txtrec, line);
	} else if (mda == 2) {
	  /* delete label */
	  *line = '\0';
	  replace_line(irec, line);
	} else if (mda == 5) {
	  while ((nc = cask("New character size = ?", ans, 80)) &&
		 ffin(ans, nc, &csx, &fj1, &fj2));
	  if (nc == 0 || csx <= 0.0f) break;
	  csy = csx * 1.2f;
	  /* rewrite line with new size */
	  fseek(file2, txtrec*256, SEEK_SET);
	  fread(line, 256, 1, file2);
	  sscanf(line + 5, "%f%f", &x[0], &y[0]);
	  sprintf(line + 5, " %.2f %.2f %.2f %.2f\n",
		  x[0], y[0], csx, csy);
	  replace_line(txtrec, line);
	}
	drawplot();
	break;

      } else if (*line == 'C') {
	/* decode position */
	if (sscanf(line+2, "%f%f%f%f", &x[0], &y[0], &csx, &csy) != 4)
	  return 1;
	if (csx <= 0.0f || csy <= 0.0f) continue;
	strcpy(label, line + 39);
	ncs = strlen(label) - 1;
	label[ncs] = '\0';
	if (line[1] == 'V') {
	  xlo = x1;
	  xhi = x1 + csy;
	  ylo = y1 - csx * 0.5f * (float) ncs;
	  yhi = y1 + csx * 0.5f * (float) ncs;
	} else {
	  xlo = x1 - csx * 0.5f * (float) ncs;
	  xhi = x1 + csx * 0.5f * (float) ncs;
	  ylo = y1 - csy;
	  yhi = y1;
	}
	if (x[0] < xlo || x[0] > xhi ||
	    y[0] < ylo || y[0] > yhi) continue;

	if (mda == 1) {
	  /* move label */
	  printf("New position = ?\n");
	  initg(&nsx, &nsy);
	  retic(&x2, &y2, jc);
	  x[0] = x[0] + x2 - x1;
	  y[0] = y[0] + y2 - y1;
	  /* rewrite line with new position */
	  sprintf(line + 2, " %8.2f %8.2f %8.2f %8.2f %s\n",
		  x[0], y[0], csx, csy, label);
	  /* this next line shouldn't be necessary - bug in solaris? */
          *line = 'C';
	  replace_line(irec, line);
	  /* output label in new position */
	  drawstring(label, ncs, line[1], *x, *y, csx, csy);
	  finig();
	} else if (mda == 2) {
	  /* delete label */
	  *line = '\0';
	  replace_line(irec, line);
	} else if (mda == 5) {
	  while ((nc = cask("New character size = ?", ans, 80)) &&
		 ffin(ans, nc, &csx, &fj1, &fj2));
	  if (nc == 0 || csx <= 0.0f) break;
	  csy = csx * 1.2f;
	  /* rewrite line with new size */
	  sprintf(line + 2, " %8.2f %8.2f %8.2f %8.2f %s\n",
		  x[0], y[0], csx, csy, label);
	  replace_line(irec, line);
	  /* output label with new size */
	  drawstring(label, ncs, line[1], *x, *y, csx, csy);
	  finig();
	}
	break;
      } else if (!strncmp(line, "\\TXT", 4)) {
	/* start copying text to output */
	/* decode position */
	if ((j = sscanf(line + 5, "%f%f%f%f",
			&x[0], &y[0], &a3, &a4)) < 2) continue;
	if (j > 2) csx = a3;
	if (j > 3) csy = a4;
	if (csx <= 0.0f || csy <= 0.0f) continue;
	txtrec = irec;
	copytext = 1;
      } else if (!strncmp(line, "\\ENDTXT", 7)) {
	/* no longer copy text to output */
	copytext = 0;
      } else if (!strncmp(line, "\\SP", 3)) {
	/* define line spacing */
	sscanf(line + 3, "%f", &linesp);
	linesp *= 1.7f;
      }
    }

    if (irec >= nrec) printf("No matching label found...\n");
  }

  while (mda == 3) {
    /* add label */
    printf("Give position for new label, press X to exit...\n");
    initg(&nsx, &nsy);
    retic(&x1, &y1, jc);
    finig();
    if (*jc == 'X' || *jc == 'x') return 0;
    if (!(ncs = cask("Enter new label?", label, 40))) continue;
    while ((nc = cask("Enter character size?", ans, 80)) &&
	   ffin(ans, nc, &csx, &fj1, &fj2));
    if (nc == 0 || csx <= 0.0f) continue;
    csy = csx * 1.2f;
    if (caskyn("...Put label vertically? (Y/N)")) {
      strcpy(line, "CV");
      x1 += csx / 2.0f;
    } else {
      strcpy(line, "CH");
      y1 -= csy / 2.0f;
    }
    initg(&nsx, &nsy);
    drawstring(label, ncs, line[1], x1, y1, csx, csy);
    finig();
    /* write new line to file */
    sprintf(line + 2, " %8.2f %8.2f %8.2f %8.2f %s\n",
	    x1, y1, csx, csy, label);
    replace_line(nrec++, line);
  }
  if (mda == 4) {
    /* add text */
    printf("Give position for new text, press X to abort...\n");
    initg(&nsx, &nsy);
    retic(&x1, &y1, jc);
    finig();
    if (*jc == 'X' || *jc == 'x') return 0;
    while ((nc = cask("Enter character size?", ans, 80)) &&
	   ffin(ans, nc, &csx, &fj1, &fj2));
    if (csx <= 0.0f) return 0;
    csy = csx * 1.2f;
    sprintf(line, "\\TXT %.2f %.2f %.2f %.2f\n", x1, y1, csx, csy);
    fwrite(line, 256, 1, file2);
    ++nrec;

    printf("Enter text, q to end...\n");
    while (1) {
      ncs = cask("Text>", line, 256);
      if (ncs == 1 &&
	  (*line == 'Q' || *line == 'q')) break;
      initg(&nsx, &nsy);
      drawstring(line, ncs, 'L', x1, y1, csx, csy);
      finig();
      /* write new line to file */
      line[ncs] = '\n';
      replace_line(nrec++, line);
      y1 -= linesp * csy;
    }
    sprintf(line, "\\ENDTXT\n");
    fwrite(line, 256, 1, file2);
    ++nrec;
  }
  return 0;
} /* editlab */

/*========================================================================*/
int editvect(char *ans)
{
  float dist, dist1, f, x[5], y[5], x1, y1, x2, y2, gxl, gyl, fnb3;
  int   mdab, irec, jrec, j1, j2, ib, nc, nb, np, i, nsx, nsy, copytext = 0;
  char  line[256], jc[4];

  if (!strncmp(ans, "MV", 2)) {
    mdab = 1;
  } else if (!strncmp(ans, "DV", 2)) {
    mdab = 2;
  } else if (!strncmp(ans, "AV", 2)) {
    mdab = 3;
  } else if (!strncmp(ans, "BV", 2)) {
    mdab = 4;
  } else if (!strncmp(ans, "MR", 2)) {
    mdab = 5;
  } else if (!strncmp(ans, "DR", 2)) {
    mdab = 6;
  } else if (!strncmp(ans, "FR", 2)) {
    mdab = 7;
  } else if (!strncmp(ans, "RR", 2)) {
    mdab = 8;
  } else {
    return 0;
  }

  /* ask to select vector with cursor */
  while (mdab != 3 && mdab != 7) {
    if (mdab < 5) {
      printf("Select vector, press X to exit...\n");
    } else {
      printf("Select rectangle, press X to exit...\n");
    }
    initg(&nsx, &nsy);
    retic(&x1, &y1, jc);
    finig();
    if (*jc == 'X' || *jc == 'x') return 0;
    /* read, decode and output lines from file */
    /* select vector with midpoint closest to cursor selection */
    jrec = -1;
    dist = 1e6f;
    rewind(file2);
    for (irec = 0; irec < nrec; ++irec) {
      fread(line, 256, 1, file2);
      if (copytext && *line != '\\') continue;
      if (mdab < 5 && *line == 'V') {
	/* decode vector */
	if (sscanf(line+1, "%f%f%f%f", &x[0], &y[0], &x[1], &y[1]) != 4)
	  return 1;
	if (x[0] < xzero || x[0] > hix ||
	    y[0] < yzero || y[0] > hiy ||
	    x[1] < xzero || x[1] > hix ||
	    y[1] < yzero || y[1] > hiy) continue;
	x2 = (x[0] + x[1]) / 2.0f;
	y2 = (y[0] + y[1]) / 2.0f;
	dist1 = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
	if (dist > dist1) {
	  jrec = irec;
	  dist = dist1;
	}
      } else if (mdab > 4 && !strncmp(line, "\\RF", 3)) {
	if (sscanf(line+4, "%f%f%f%f", &x[0], &x[1], &y[0], &y[1]) != 4)
	  return 1;
	if (x[0] < x1 && x[0] + x[1] > x1 &&
	    y[0] < y1 && y[0] + y[1] > y1) {
	  jrec = irec;
	  break;
	}
      } else if (!strncmp(line, "\\TXT", 4)) {
	copytext = 1;
      } else if (!strncmp(line, "\\ENDTXT", 7)) {
	copytext = 0;
      }
    }
    if (jrec < 0) {
      printf("No match found...\n");
      continue;
    }

    if (mdab == 1 || mdab == 5) {
      /* move vector or rectangle */
      fseek(file2, jrec*256, SEEK_SET);
      fread(line, 256, 1, file2);
      printf("New position = ?\n");
      initg(&nsx, &nsy);
      retic(&x2, &y2, jc);
      if (mdab == 1) {
	/* output vector in new position */
	np = sscanf(line+1, "%f%f%f%f%f%f%f%f%f%f",
		    &x[0], &y[0], &x[1], &y[1], &x[2],
		    &y[2], &x[3], &y[4], &x[4], &y[4]);
	if (np < 4 || np%2 != 0) return 1;
	np /= 2;
	line[2] = '\0';
	for (i = 0; i < np; i++) {
	  x[i] += x2 - x1;
	  y[i] += y2 - y1;
	  sprintf(line + strlen(line) - 1, " %.2f %.2f\n", x[i], y[i]);
	  if (i == 0) {
	    pspot(x[i], y[i]);
	  } else {
	    vect(x[i], y[i]);
	  }
	}
      } else {
	sscanf(line+3, "%f%f%f%f", &x[0], &x[1], &y[0], &y[1]);
	setcolor(0);
	pspot(x[0], y[0]);
	vect(x[0], y[0]+y[1]);
	vect(x[0]+x[1], y[0]+y[1]);
	vect(x[0]+x[1], y[0]);
	vect(x[0], y[0]);
	minig_fill();
	x[0] += x2 - x1;
	y[0] += y2 - y1;
	setcolor(1);
	pspot(x[0], y[0]);
	vect(x[0], y[0]+y[1]);
	vect(x[0]+x[1], y[0]+y[1]);
	vect(x[0]+x[1], y[0]);
	vect(x[0], y[0]);
	minig_fill();
	sprintf(line, "\\RF %.2f %.2f %.2f %.2f\n", x[0], x[1], y[0], y[1]);
      }
      finig();
      /* rewrite line with new position */
      replace_line(jrec, line);
    } else if (mdab == 2 || mdab == 6) {
      /* delete vector or rectangle */
      *line = '\0';
      replace_line(jrec, line);
    } else if (mdab == 4) {
      /* put NB breaks into vector */
      while ((nc = cask("Number of breaks = ?", ans, 80)) &&
	     inin(ans, nc, &nb, &j1, &j2));
      if (nc == 0 || nb == 0) continue;
      fnb3 = (float) (nb * 3 + 2);
      /* decode vector */
      fseek(file2, jrec*256, SEEK_SET);
      fread(line, 256, 1, file2);
      if (sscanf(line+1, "%f%f%f%f", &x1, &y1, &x[1], &y[1]) != 4) return 1;
      gxl = x[1] - x1;
      gyl = y[1] - y1;
      f = (float) (nb * 3) / fnb3;
      x[0] = x1 + f * gxl;
      y[0] = y1 + f * gyl;
      sprintf(line + 1, " %.2f %.2f %.2f %.2f\n", x[0], y[0], x[1], y[1]);
      replace_line(jrec, line);
      for (ib = 0; ib < nb; ++ib) {
	f = (float) (ib * 3) / fnb3;
	x[0] = x1 + f * gxl;
	y[0] = y1 + f * gyl;
	f = (float) (ib * 3 + 2) / fnb3;
	x[1] = x1 + f * gxl;
	y[1] = y1 + f * gyl;
	sprintf(line + 1, " %.2f %.2f %.2f %.2f\n",
		x[0], y[0], x[1], y[1]);
	replace_line(nrec++, line);
      }
    } else if (mdab == 8) {
      /* resize rectangle */
      printf("New position for upper right corner = ?\n");
      initg(&nsx, &nsy);
      retic(&x2, &y2, jc);
      sscanf(line+3, "%f%f%f%f", &x[0], &x[1], &y[0], &y[1]);
      setcolor(0);
      pspot(x[0], y[0]);
      vect(x[0], y[0]+y[1]);
      vect(x[0]+x[1], y[0]+y[1]);
      vect(x[0]+x[1], y[0]);
      vect(x[0], y[0]);
      minig_fill();
      x[1] = x2 - x[0];
      y[1] = y2 - y[0];
      setcolor(1);
      pspot(x[0], y[0]);
      vect(x[0], y[0]+y[1]);
      vect(x[0]+x[1], y[0]+y[1]);
      vect(x[0]+x[1], y[0]);
      vect(x[0], y[0]);
      minig_fill();
      sprintf(line, "\\RF %.2f %.2f %.2f %.2f\n", x[0], x[1], y[0], y[1]);
      /* rewrite line with new position */
      replace_line(jrec, line);
    }
  }

  while (1) {
    /* add vector or filled rectangle */
    if (mdab < 5) {
      printf("Give positions for new vector, press X to exit...\n");
    } else {
      printf("Specify corners of rectangle, press X to exit...\n");
    }
    for (np = 0; np < 2; ++np) {
      initg(&nsx, &nsy);
      retic(&x[np], &y[np], jc);
      finig();
      if (*jc == 'X' || *jc == 'x') return 0;
    }
    if (mdab < 5) {
      /* draw vector */
      initg(&nsx, &nsy);
      pspot(x[0], y[0]);
      vect(x[1], y[1]);
      finig();
      /* write new line to file */
      sprintf(line, "V %.2f %.2f %.2f %.2f\n", x[0], y[0], x[1], y[1]);
    } else {
      if (x[0] > x[1]) {
	f = x[0]; x[0] = x[1]; x[1] = f;
      }
      if (y[0] > y[1]) {
	f = y[0]; y[0] = y[1]; y[1] = f;
      }
      pspot(x[0], y[0]);
      vect(x[0], y[1]);
      vect(x[1], y[1]);
      vect(x[1], y[0]);
      vect(x[0], y[0]);
      minig_fill();
      sprintf(line, "\\RF %.2f %.2f %.2f %.2f\n",
	      x[0], x[1]-x[0], y[0], y[1]-y[0]);
    }
    replace_line(nrec++, line);
  }
} /* editvect */

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
int zoom(char *ans)
{
  float x1, y1, x2, y2, dx, dy;

  if (*ans == 'Z' || *ans == 'z') {
    dx = (xmax - xmin) * 1.1f;
    dy = (ymax - ymin) * 1.1f;
    xzero = xmin - dx * 0.05f;
    yzero = ymin - dy * 0.05f;
  } else {
    retic(&x1, &y1, ans);
    retic(&x2, &y2, ans);
    dx = fabs(x2 - x1);
    xzero = (x1 < x2 ? x1 : x2);
    dy = fabs(y2 - y1);
    yzero = (y1 < y2 ? y1 : y2);
  }
  hix = dx + xzero;
  hiy = dy + yzero;
  return 0;
} /* zoom */

#define WINDNAME "Color Picker"

/* ====================================================================== */
int pick_color(float *r, float *g, float *b)
{
  XSetWindowAttributes setwinattr;
  static Window cp_win_id;

  static int         depth;        /* number of planes */
  static Visual     *visual;       /* visual type */
  static XSizeHints  win_position; /* position and size for window manager */

  static float cva [11] =
  { 1.0,  1.0,  1.0,  1.0,  1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  static float cvb [11] =
  {-0.1, -0.2, -0.3, -0.5, -0.7, 1.0, 0.8, 0.6, 0.45, 0.3, 0.2};
  /* {-0.1, -0.2, -0.3, -0.5, -0.7, 1.0, 0.9, 0.8, 0.7, 0.5, 0.3}; */
  static float cf [19] [3] = {
    {1.0f, 1.0f, 1.0f},

    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.33f, 1.0f},
    {0.0f, 0.67f, 1.0f},
    {0.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.67f},
    {0.0f, 1.0f, 0.33f},
    {0.0f, 1.0f, 0.0f},
    {0.33f, 1.0f, 0.0f},
    {0.67f, 1.0f, 0.0f},

    {1.0f, 1.0f, 0.0f},
    {1.0f, 0.67f, 0.0f},
    {1.0f, 0.33f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.33f},
    {1.0f, 0.0f, 0.67f},
    {1.0f, 0.0f, 1.0f},
    {0.67f, 0.0f, 1.0f},
    {0.33f, 0.0f, 1.0f} };
  static int cl[19] = {0, 10,11,12,13,14,15,16,17,18, 1,2,3,4,5,6,7,8,9};
  static int first = 1;

  unsigned int  w = 380, h = 220;
  unsigned long valuemask;
  float c;
  int   scr_width, scr_height, ix, iy, i, j, k;
  short points[5][2];
 
#ifdef PSHC_CP
  extern int closeps(void);
  extern int connpt(float, float);
  extern int openps(char *, char *, char *, float, float, float, float);
  extern int setrgbcolor(float, float, float);
  extern int strtpt(float, float);
  extern int wfill(void);
  extern int wstroke(void);
#endif

  initg(&ix, &iy);

  if (!disp_id) {
    printf("Display not opened!\n");
    return 1;
  }
  if (first) {
#ifdef PSHC_CP
    /* grey scale */
    openps("pick.ps", "Color Picker (Author: D.C. Radford)", "",
	   -20.0f, 420.f, -20.0f, 260.f);
    for (j = 0; j < 11; j++) {
      c = ((float) (10 - j)) * 0.1f;
      setrgbcolor(c, c, c);
      strtpt(1.0f, (float)((10-j)*20+1));
      connpt(1.0f, (float)((10-j)*20+19));
      connpt(19.0f, (float)((10-j)*20+19));
      connpt(19.0f, (float)((10-j)*20+1));
      wfill();
    }
    /* colors */
    for (i = 1; i < 19; i++) {
      for (j = 0; j < 11; j++) {
	c = cvb[j];
	k = (j < 5 ? cl[i] : i);
	setrgbcolor(cva[j] + cf[k][0]*c,
		    cva[j] + cf[k][1]*c,
		    cva[j] + cf[k][2]*c);
	strtpt((float)(i*20+1), (float)((10-j)*20+1));
	connpt((float)(i*20+1), (float)((10-j)*20+19));
	connpt((float)(i*20+19), (float)((10-j)*20+19));
	connpt((float)(i*20+19), (float)((10-j)*20+1));
	wfill();
      }
    }
    closeps();
#endif
    first = 0;
    scr_width  = XWidthOfScreen(screen_id);
    scr_height = XHeightOfScreen(screen_id);
    depth  = XDefaultDepthOfScreen(screen_id);
    visual = XDefaultVisualOfScreen(screen_id);
    valuemask = CWBitGravity | CWBackingStore | CWBackPixel;
    setwinattr.bit_gravity = SouthWestGravity;
    setwinattr.backing_store = Always;
    setwinattr.background_pixel = XWhitePixelOfScreen(screen_id);
    ix = 0.5f * (float)scr_width;
    iy = 0.5f * (float)scr_height;
    ix -= w/2;
    iy -= h/2;

    /* Create the window */
    cp_win_id = XCreateWindow(disp_id, root_win_id, ix, iy, w, h,
			      0, depth, InputOutput, visual,
			      valuemask, &setwinattr);

    /* Window Manager Hints */
    win_position.x = ix;
    win_position.y = iy;
    win_position.width  = w;
    win_position.height = h;
    win_position.flags  = USPosition|USSize;
    XSetWMNormalHints(disp_id, cp_win_id, &win_position); 
    XStoreName(disp_id, cp_win_id, WINDNAME);
  }

  /* Map the window */
  XSelectInput(disp_id, cp_win_id, ExposureMask|VisibilityChangeMask);
  XMapRaised(disp_id, cp_win_id);
  /* Wait for the window to be raised. Some X servers do not generate
     an initial expose event, so also check the visibility event. */
  XMaskEvent(disp_id, ExposureMask|VisibilityChangeMask, &event);
  XSelectInput(disp_id, cp_win_id, 0);

  /* grey scale */
  points[0][0] = points[1][0] = 1;
  points[2][0] = points[3][0] = 19;
  for (j = 0; j < 11; j++) {
    points[0][1] = points[3][1] = j * 20 + 1;
    points[1][1] = points[2][1] = j * 20 + 19;
    c = ((float) (10 - j)) * 0.1f;
    set_minig_color_rgb(17, c, c, c);
    XFillPolygon(disp_id, cp_win_id, gc_id, (XPoint *)points[0], 4,
		 Convex, CoordModeOrigin);
  }
  /* colors */
  for (i = 1; i < 19; i++) {
    points[0][0] = points[1][0] = i * 20 + 1;
    points[2][0] = points[3][0] = i * 20 + 19;
    for (j = 0; j < 11; j++) {
      points[0][1] = points[3][1] = j * 20 + 1;
      points[1][1] = points[2][1] = j * 20 + 19;
      c = cvb[j];
      k = (j < 5 ? cl[i] : i);
      set_minig_color_rgb(17,
			  cva[j] + cf[k][0]*c,
			  cva[j] + cf[k][1]*c,
			  cva[j] + cf[k][2]*c);
      XFillPolygon(disp_id, cp_win_id, gc_id, (XPoint *)points[0], 4,
		   Convex, CoordModeOrigin);
    }
  }
  XFlush(disp_id);

  XSelectInput(disp_id, cp_win_id, ButtonPressMask);
  XWindowEvent(disp_id, cp_win_id, ButtonPressMask, &event);
  i = event.xbutton.x / 20;
  if (i > 18) i = 18;
  j = event.xbutton.y / 20;
  if (j > 10) j = 10;
  if (i == 0) {
    *r = *g = *b = ((float) (10 - j)) * 0.1f;
  } else {
    c = cvb[j];
    k = (j < 5 ? cl[i] : i);
    *r = cva[j] + cf[k][0]*c;
    *g = cva[j] + cf[k][1]*c;
    *b = cva[j] + cf[k][2]*c;
  }
  printf("RGBcolor: %.2f %.2f %.2f\n", *r, *g, *b);
  set_minig_color_rgb(17, *r, *g, *b);
  
  XUnmapWindow(disp_id, cp_win_id);
  return 0;
}

/* ====================================================================== */
int replace_line(int irec, char *save_line)
{

  if (irec > nrec || irec < 0) {
    printf("ACK! Severe error with irec in replace_line!\n");
    exit(-1);
  } else if (irec == nrec) {
    *saved_lines[(nsaved%100)] = '\0';
  } else {
    fseek(file2, irec*256, SEEK_SET);
    fread(saved_lines[(nsaved%100)], 256, 1, file2);
  }
  saved_recs[nsaved++] = irec;
  fseek(file2, irec*256, SEEK_SET);
  fwrite(save_line, 256, 1, file2);
  return 0;
}

/* ====================================================================== */
int undo(void)
{
  int irec;

  if (nsaved < 1) return 1;
  irec = saved_recs[--nsaved];
  fseek(file2, irec*256, SEEK_SET);
  fwrite(saved_lines[nsaved], 256, 1, file2);
  drawplot();
  return 0;
}
