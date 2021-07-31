/* version 0.0     D.C. Radford    July 1999 */
/* version 1.2     D.C. Radford    Oct  2005 */
/* latest update   D.C. Radford    Aug  2017 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>

#include "util.h"
#include "minig.h"
#ifdef HAVE_GNU_READLINE
#include <readline/history.h>
#endif

#ifdef NOGRAPHICS
int bell(void) {return 0;}
int cvxy(float *x, float *y, int *ix, int *iy, int mode) {return 0;}
int define_color(char *color_name) {return 0;}
int erase(void) {return 0;}
int finig(void) {return 0;}
int get_focus(int *focus_id) {return 0;}
int grax(float lo_axis_val, float hi_axis_val,
	 float *axis_tick_vals, int *n_vals, int yflag) {return 0;}
int hcopy(void) {return 0;}
int initg(int *win_size_x, int *win_size_y) {return 0;}
int ivect(int ix, int iy) {return 0;}
int limg(int nx, int ix, int ny, int iy) {return 0;}
int minig_fill(void) {return 0;}
int mspot(int nx, int ny) {return 0;}
int point(float x, float y) {return 0;}
int pspot(float x, float y) {return 0;}
int putg(char *text, int nc, int kad) {return 0;}
int retic(float *x, float *y, char *c) {return 0;}
int save_xwg(char *in) {return 0;}
int set_focus(int focus_id) {return 0;}
int set_xwg(char *in) {return 0;}
int setcolor(int icol) {return 0;}
int symbg(int symbol, float x, float y, int size) {return 0;}
int trax(float nx, float ix, float ny, float iy, int axis_mode) {return 0;}
int vect(float x, float y) {return 0;}
int set_minig_color_rgb(int color_num, float red, float green, float blue) {return 0;}
int set_minig_line_width(int width) {return 0;}
int ktras(int ix, int iy, int mode) {return 0;}

#endif

/* global data */
struct {
  double gain[6];                           /* energy calibration */
  int    ical, nterms;
  int    disp, loch, hich, locnt, nchs;     /* spectrum display stuff */
  int    dxlo, dxnum, no_erase;             /* division of graphics window in x-dir */
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
  int    display_fit_diff;                  /* flag for whether to plot diff */
} gf3gd;

struct {
  int    x_axis_marks;           /* flags for whether to show tick marks and/or numbers */
  int    y_axis_marks;
} gf3gd2;

FILE *file1, *file2, *file3, *scrfilea, *scrfileb, *winfile, *prfile = 0;
char prfilnam[80] = "gf3.log";
static int cfopen = 0;

extern FILE *infile, *cffile;    /* command file flag and file descriptor*/
extern int cflog;                /* command logging flag */

float derivs[111];
int   colormap[15] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
int   ready = 0;
int   winmod = 0;
int   gotsignal;

/* ==== routines defined here ==== */
double channo(float egamma);
double determ(double *array, int norder);
float  getmkrchnl(int mn);
int adddelpk(int mode, int npeak);        /* returns 1 on error */
int addspec(char *command, int nc);
int addwin(void);                         /* returns 1 on error */
int adjgain(int mode, float *oldch1, float *oldch2, float *newch1, float *newch2,
	    float *oldsp, float *newsp, int nco, int ncn, float fact);
int autobkgnd(int *maxspec);
int autocal(void);
int autocal3(void);
int autocal4(void);
int autofit(float *sens, int lo_ch, int hi_ch);
int autopeak(char *command);
int chngmark(int mnum);
int comfil(char *filnam, int nc);
int contract(int *cfact);
int curse(int *chnum);
int dispwin(void);                        /* returns 1 on error */
int diveff(char *filnam, int nc);         /* returns 1 on error */
int dofit(int npks);
int dspfit(void);
int dspmkr(int mkrnum);
int dspsp(int in1, int in2, int in3);     /* returns 1 on error */
int dspsp2(int in1, int in2, int in3, int in4);
int dump(char *filnam, int nc, int mode); /* mode = 0/1 to write/read dump */
int encal(char *fn, int nc);
int energy(float x, float dx, float *eg, float *deg); /* returns 1 on error */
int eval(float *pars, int ichan, float *fit, int npks, int mode);
int find_bg(int loch, int hich, int disp_flag,
	    float *x1, float *y1, float *x2, float *y2);
int find_bg2(int loch, int hich, float *x1, float *y1, float *x2, float *y2);
int find_ge_cent(int loch, int hich, int disp_flag, float *area,
		 float *cent, float *fwhm, float *darea, float *dcent);
int findpks(int npixy);
int fitter(int maxits);                   /* returns 1 on error */
int fix_para(int param, int fix_flag);
int fixorfree(char *command, int nc);
int getsp(char *filnam, int nc);          /* returns 1 on error */
int gfexec(char *command, int nc);
int gffin(void);
int gfhelp(char *ans);
int gfinit(int argc, char **argv);
int gfset(void);
int lookup(char *filnam, int nc);
int matread(char *fn, float *sp, char *namesp, int *numch,
	    int idimsp, int *spmode);    /* returns 1 on error */
int para2num(char *ans, int *param);
int parset(int mode);
                /* mode = -1 to overide fixed relative positions and widths*/
int pfind(float *chanx, float *psize, int loch, int hich,
   	  int ifwhm, float sigma, int maxpk, int *npk);
int polfit(double *x, double *y, double *sigmay, int npts,
   	   int nterms, int mode, double *a, double *chisqr);
int setcts(void);
int slice(char *ans, int nc);
int startwid(char *ans, int nc);
int storac(int in);
int sumcts(int mode, int loch, int hich);
  /* mode=0: sum without background subtraction
     mode=1: sum with    background subtraction */
int sumcts_jch(void);
int list_params(int mode); 
   /* imode = 1 to list limits and peak positions as well as fit pars */
int weight(int weightmode);
int wrtlook(void);
int wrtsp(char *ans, int nc);

/* ======================================================================= */
int adddelpk(int mode, int npeak)
{
  float x0, rj1, rj2, s, x=0, y;
  int   i, j, k, l, n;
  int   ilo, ipp, j2, nc, js, ihi;
  char  ans[80];

  if (!ready || gf3gd.npks <= 0) {
    printf("Cannot - no fit defined.\n");
    return 1;
  }
  if (mode == 1) {
    /* AP; add peak to fit */
    if (gf3gd.npks >= 35) {
      printf("Cannot - too many peaks.\n");
      return 1;
    }
    /* ask for peak position */
    while (1) {
      n = gf3gd.npks;
      if (gf3gd.disp) {
	printf(" New peak position?  (hit T to type, A to abort)\n");
	retic(&x, &y, ans);
	if (ans[0] == 'A' || ans[0] == 'a') return 0;
	gf3gd.ppos[n] = x - .5f;
      } else {
	ans[0] = 'T';
      }
      if (ans[0] == 'T' || ans[0] == 't') {
	/* hit t for type */
	while (1) {
	  k = cask("Peak position = ? (A to abort)", ans, 80);
	  if (ans[0] == 'A' || ans[0] == 'a') return 0;
	  if (!ffin(ans, k, &gf3gd.ppos[n], &rj1, &rj2)) break;
	}
      }
      if ((int) gf3gd.ppos[n] < gf3gd.mch[0]) {
	printf("Peaks must be within limits - try again.\n");
      } else if ((int) gf3gd.ppos[n] <= gf3gd.mch[1]) {
	break;
      }
    }

    dspmkr(n+3);
    gf3gd.npars += 3;
    gf3gd.areas[gf3gd.npks] = 0.f;
    gf3gd.dareas[gf3gd.npks] = 0.f;
    gf3gd.cents[gf3gd.npks] = gf3gd.ppos[gf3gd.npks];
    gf3gd.dcents[gf3gd.npks] = 0.f;
    gf3gd.npks++;
    for (i = gf3gd.npars - 3; i < gf3gd.npars; ++i) {
      gf3gd.errs[i] = 0.f;
      gf3gd.freepars[i] = 1;
    }
    gf3gd.freepars[gf3gd.npars - 2] = gf3gd.infixw;
    gf3gd.nfp = gf3gd.nfp + 1 - gf3gd.infixw;
    ilo = gf3gd.mch[0] + 1;
    ihi = gf3gd.mch[1] + 1;
    x0 = (float) ((ihi + ilo) / 2);
    gf3gd.pars[gf3gd.npars - 3] = gf3gd.ppos[gf3gd.npks - 1];
    gf3gd.pars[gf3gd.npars - 2] = sqrt(gf3gd.swpars[0] + 
		    gf3gd.swpars[1] * gf3gd.ppos[gf3gd.npks - 1] + 
		    gf3gd.swpars[2] * gf3gd.ppos[gf3gd.npks - 1] * 
				      gf3gd.ppos[gf3gd.npks - 1]);
    x = gf3gd.ppos[gf3gd.npks - 1] - x0 + 1.f;
    y = gf3gd.pars[0] + gf3gd.pars[1] * x + gf3gd.pars[2] * x * x;
    ipp = gf3gd.ppos[gf3gd.npks - 1] + 1.5f;
    gf3gd.pars[gf3gd.npars - 1] = gf3gd.spec[ipp - 1] - y;
    if (gf3gd.infixw == 1 &&
	gf3gd.irelw == 0 &&
	caskyn("Reset all non-fixed peak widths? (Y/N)")) {
      for (i = 0; i <= gf3gd.npks; ++i) {
	j = i * 3 + 7;
	if (gf3gd.freepars[j] == 1) {
	  gf3gd.pars[j] = sqrt(gf3gd.swpars[0] + 
				  gf3gd.swpars[1] * gf3gd.ppos[i] + 
				  gf3gd.swpars[2] * gf3gd.ppos[i] * 
				  gf3gd.ppos[i]);
	}
      }
    }
    if (gf3gd.cents[gf3gd.npks - 1] >= gf3gd.cents[gf3gd.npks - 2]) return 0;
    if (!caskyn("Re-order peaks in order of energy? (Y/N)")) return 0;
    for (i = 0; i < gf3gd.npks-1; ++i) {
      for (j = i + 1; j < gf3gd.npks; ++j) {
	if (gf3gd.cents[i] > gf3gd.cents[j]) {
	  s = gf3gd.ppos[i];
	  gf3gd.ppos[i] = gf3gd.ppos[j];
	  gf3gd.ppos[j] = s;
	  s = gf3gd.areas[i];
	  gf3gd.areas[i] = gf3gd.areas[j];
	  gf3gd.areas[j] = s;
	  s = gf3gd.dareas[i];
	  gf3gd.dareas[i] = gf3gd.dareas[j];
	  gf3gd.dareas[j] = s;
	  s = gf3gd.cents[i];
	  gf3gd.cents[i] = gf3gd.cents[j];
	  gf3gd.cents[j] = s;
	  s = gf3gd.dcents[i];
	  gf3gd.dcents[i] = gf3gd.dcents[j];
	  gf3gd.dcents[j] = s;
	  for (k = i * 3 + 6; k < i * 3 + 9; ++k) {
	    l = (j-i) * 3 + k;
	    s = gf3gd.pars[k];
	    gf3gd.pars[k] = gf3gd.pars[l];
	    gf3gd.pars[l] = s;
	    s = gf3gd.errs[k];
	    gf3gd.errs[k] = gf3gd.errs[l];
	    gf3gd.errs[l] = s;
	    js = gf3gd.freepars[k];
	    gf3gd.freepars[k] = gf3gd.freepars[l];
	    gf3gd.freepars[l] = js;
	  }
	}
      }
    }
  } else {
    /* DP; delete peak from fit */
    if (gf3gd.npks <= 1) {
      printf("Cannot - too few peaks.\n");
      return 1;
    }
    while (npeak < 1 || npeak > gf3gd.npks) {
      nc = cask("Number of peak to be deleted = ?", ans, 80);
      if (nc == 0) return 0;
      if (!inin(ans, nc, &npeak, &j, &j2)) break;
    }
    gf3gd.npars += -3;
    j = npeak * 3 + 4;
    gf3gd.nfp = gf3gd.nfp - 3 + gf3gd.freepars[j-1] + gf3gd.freepars[j] 
      + gf3gd.freepars[j + 1];
    if (npeak != gf3gd.npks) {
      for (i = npeak; i <= gf3gd.npks-1; ++i) {
	gf3gd.ppos[i-1] = gf3gd.ppos[i];
	gf3gd.areas[i-1] = gf3gd.areas[i];
	gf3gd.dareas[i-1] = gf3gd.dareas[i];
	gf3gd.cents[i-1] = gf3gd.cents[i];
	gf3gd.dcents[i-1] = gf3gd.dcents[i];
      }
      for (i = j; i <= gf3gd.npars; ++i) {
	gf3gd.pars[i-1] = gf3gd.pars[i + 2];
	gf3gd.errs[i-1] = gf3gd.errs[i + 2];
	gf3gd.freepars[i-1] = gf3gd.freepars[i + 2];
      }
    }
    --gf3gd.npks;
  }
  return 0;
} /* adddelpk */

/* ======================================================================= */
int addspec(char *command, int nc)
{
  /* add spectrum / add counts / multiply or divide spectrum */

  float fact, rj1, rj2;
  int   i, numch;
  static char namesp2[8], ans[80];
  static float save[16384];

  strncpy(gf3gd.namesp + 4, ".MOD", 4);
  if (nc >= 3) {
    strcpy(ans, command + 2);
    if (!strncmp(command, "DV", 2)) goto GETSP;
    if (ffin(command + 2, nc-2, &fact, &rj1, &rj2) &&
	!strncmp(command, "MS", 2))  goto GETSP;
  } else {
    if (!strncmp(command, "DV", 2)) goto GETSFN;

    while (1) {
      if (!strncmp(command, "AC", 2)) {
	nc = cask(" Added counts = ?", ans, 80);
      } else if (!strncmp(command, "AS", 2)) {
	nc = cask(" Mult. factor = ?", ans, 80);
      } else {
	nc = cask(" Mult. factor or spectrum filename = ?", ans, 80);
      }
      if (nc == 0) return 0;
      if (ffin(ans, nc, &fact, &rj1, &rj2)) {
	if (!strncmp(command, "MS", 2) ||
	    !strncmp(command, "DV", 2)) goto GETSP;
      } else {
	break;
      }
    }
  }

  if (!strncmp(command, "AC", 2)) {
    /* add fact to each channel */
    for (i = 0; i <= gf3gd.maxch; ++i) {
      gf3gd.spec[i] += fact;
    }
    return 0;
  } else if (!strncmp(command, "MS", 2)) {
    /* multiply spectrum by fact */
    for (i = 0; i <= gf3gd.maxch; ++i) {
      gf3gd.spec[i] *= fact;
    }
    return 0;
  }

  /* open and read second spectrum file */
 GETSFN:
  nc = cask("Spectrum file or ID = ?", ans, 80);
  if (nc == 0) return 0;
 GETSP:
  if (readsp(ans, save, namesp2, &numch, 16384)) goto GETSFN;
  if (numch != gf3gd.maxch + 1)
    printf("WARNING -- the two spectra have different lengths.\n");
  if (numch > gf3gd.maxch + 1) numch = gf3gd.maxch + 1;
  if (!strncmp(command, "AS", 2)) {
    /* add other spect. multiplied by fact */
    for (i = 0; i < numch; ++i) {
      gf3gd.spec[i] += fact * save[i];
    }
  } else if (!strncmp(command, "MS", 2)) {
    /* multiply spectrum by second spectrum */
    for (i = 0; i < numch; ++i) {
      gf3gd.spec[i] *= save[i];
    }
  } else {
    /* divide spectrum by second spectrum */
    for (i = 0; i < numch; ++i) {
      if (save[i] > .001f) {
	gf3gd.spec[i] /= save[i];
      }
    }
  }
  return 0;
} /* addspec */

/* ======================================================================= */
int addwin(void)
{
  /* add window(s) to look-up file or slice input file
     winmod = 0 : no mode defined
     winmod = 1 : look-up file mode
     winmod = 2 : slice file mode */

  float area, cent, x=0, y, y1, y2, dc = 0.0f, eg, deg, fnc, cou, sum;
  int   i, isave, j1, j2, nc, nx, ny, ihi, ilo, luv;
  char  ans[80];

  if (winmod == 0) {
    printf("Bad command: No window file open...\n");
    return 1;
  } else if (!gf3gd.disp) {
    printf("Bad command: New spectrum not yet displayed...\n");
    return 1;
  }
  printf("Hit X or right mouse button to exit...\n");

  /* get limits for integration, background */
  while (1) {
    while (1) {
      retic(&x, &y, ans);
      if (ans[0] == 'X' || ans[0] == 'x') return 0;
      ilo = x;
      y1 = y;
      retic(&x, &y, ans);
      if (ans[0] == 'X' || ans[0] == 'x') return 0;
      ihi = x;
      y2 = y;
      if (ilo > ihi) {
	ihi = ilo;
	ilo = x;
	y2 = y1;
	y1 = y;
      }
      if (winmod != 1 || ilo < gf3gd.nclook) break;
      printf("Cannot - lower limit exceeds dimension of look-up table (%d)\n",
	     gf3gd.nclook);
    }

    /* display limits */
    isave = gf3gd.mch[0];
    gf3gd.mch[0] = ilo;
    dspmkr(1);
    gf3gd.mch[0] = ihi;
    dspmkr(1);
    gf3gd.mch[0] = isave;
    sum = 0.f;
    area = 0.f;
    cent = 0.f;
    fnc = (float) (ihi - ilo);
    if (fnc == 0.f) fnc = 1.f;
    if (winmod == 2) {
      /* slice mode : background to be subtracted
         display background */
      initg(&nx, &ny);
      x = (float) ilo + .5f;
      pspot(x, y1);
      x = (float) ihi + .5f;
      vect(x, y2);
      finig();
      for (i = ilo; i <= ihi; ++i) {
	sum += gf3gd.spec[i];
	cou = gf3gd.spec[i] - (y1 + (y2 - y1) * (float) (i - ilo) / fnc);
	area += cou;
	cent += cou * (float) (i - ilo);
      }
      if (area == 0.f) {
	cent = 0.f;
      } else {
	cent = cent / area + (float) ilo;
	area /= sum;
	if (!energy(cent, dc, &eg, &deg)) {
	  /* write out results */
	  printf("  Chs%5d to%5d   P/T = %7.4f   Energy =%9.3f\n",
		 ilo, ihi, area, eg);
	  fprintf(winfile, 
		  "  Chs%5d to%5d   P/T = %7.4f   Energy =%9.3f\n",
		  ilo, ihi, area, eg);
	  fflush(winfile);
	  continue;
	}
      }
      /* no energy calibration defined or zero area */
      printf("  Chs%5d to%5d   P/T = %7.4f    Cent. =%9.3f\n",
	     ilo, ihi, area, cent);
      fprintf(winfile, 
	      "  Chs%5d to%5d   P/T = %7.4f    Cent. =%9.3f\n",
	      ilo, ihi, area, cent);
    } else {
      /* look-up mode : no background to be subtracted */
      for (i = ilo; i <= ihi; ++i) {
	area += gf3gd.spec[i];
	cent += gf3gd.spec[i] * (float) (i - ilo);
      }
      /* write out results */
      if (area == 0.f) {
	cent = 0.f;
	printf(" Chs%5d to%5d    Cent. =%9.3f\n", ilo, ihi, cent);
      } else {
	cent = cent / area + (float) ilo;
	if (energy(cent, dc, &eg, &deg)) {
	  printf(" Chs%5d to%5d    Cent. =%9.3f\n", ilo, ihi, cent);
	} else {
	  printf(" Chs%5d to%5d   Energy =%9.3f\n", ilo, ihi, eg);
	}
      }

      /* ask for look-up value */
      while ((nc = cask("  Look-up value = ? (rtn to abort)", ans, 80)) &&
	     inin(ans, nc, &luv, &j1, &j2));
      if (nc) {
	if (luv < gf3gd.lookmin) gf3gd.lookmin = luv;
	if (luv > gf3gd.lookmax) gf3gd.lookmax = luv;
	if (ihi >= gf3gd.nclook) ihi = gf3gd.nclook-1;
	for (i = ilo; i <= ihi; ++i) {
	  gf3gd.looktab[i] = (short) luv;
	}
      }
    }
  }
} /* addwin */

/* ======================================================================= */
int adjgain(int mode, float *oldch1, float *oldch2, float *newch1, float *newch2,
	    float *oldsp, float *newsp, int nco, int ncn, float fact)
{
  /* adjust gain of old spectrum to get newsp
     mode=-1: set newsp to new values * fact
     mode=0:  ask for oldch1,oldch2,newch1,newch2
     then set newsp to new values * fact
     mode=1:  add (new values * fact) to newsp
     nco = no. of chs in oldsp
     ncn = no. of chs in newsp */

  float a, b, ch, cl, oh, ol;
  int   flag, i, k, newch, ioh, iol, ist;
  char  ans[80];

  while (mode == 0) {
    k = cask("Oldch1,Oldch2,Newch1,Newch2 = ?", ans, 80);
    if (k == 0) return 0;
    for (i = 1; i < k; ++i) {
      if (ans[i] == ' ') {
	ans[i] = ',';
      }
    }
    strncpy(ans + k, ",", 80-k);
    if (sscanf(ans, "%f,%f,%f,%f", oldch1, oldch2, newch1, newch2) == 4) break;
  }
  if (mode <= 0) {
    for (i = 0; i < ncn; ++i) {
      newsp[i] = 0.f;
    }
  }
  if (*newch1 == *newch2 ||
    // (a = (*oldch2 - *oldch1) / (*newch2 - *newch1)) <= 0.f) {
      (a = (*oldch2 - *oldch1) / (*newch2 - *newch1)) == 0.f) {
    printf("Error - cannot continue.  Present values of\n"
	   "   oldch1,oldch2,newch1,newch2 are: %.3f %.3f %.3f %.3f\n",
	   *oldch1, *oldch2, *newch1, *newch2);
    return 0;
  }
  if (a < 0) {
    for (i=0; i<nco/2; i++) {
      a = oldsp[i];
      oldsp[i] = oldsp[nco - i - 1];
      oldsp[nco - i - 1] = a;
    }
    *oldch1 = nco - *oldch1 - 1.0;
    *oldch2 = nco - *oldch2 - 1.0;
    a = (*oldch2 - *oldch1) / (*newch2 - *newch1);
  }

  b = *oldch2 - a * *newch2;
  ist = (-0.5f - b) / a + 0.5f;
  if (ist < 0) ist = 0;
  ol = ((float) ist - 0.5f) * a + b;
  if (ol < -0.5f) ol = -0.5f;
  iol = ol + 0.5f;
  cl = oldsp[iol] * ((float) iol + 0.5f - ol);

  flag = 0;
  for (newch = ist; newch < ncn; ++newch) {
    oh = ((float) newch + 0.5f) * a + b;
    ioh = oh + 0.5f;
    if (ioh >= nco) {
      oh = (float) (nco) - 0.5f;
      ioh = nco - 1;
      flag = 1;
    }
    ch = oldsp[ioh] * ((float) ioh + 0.5f - oh);
    while (++iol <= ioh) {
      cl += oldsp[iol];
    }
    newsp[newch] += (cl - ch) * fact;
    if (flag) return 0;
    cl = ch;
    iol = ioh;
  }
  return 0;
} /* adjgain */

/* ======================================================================= */
int autobkgnd(int *maxspec)
{
  /* maxspec needs to be modified since gfexec is called recursively
     and forgets the number of displayed spectra
     find background over +/- w2*FWHM */

  static float w2 = 6.f;
  float fwhm, x, w1, x1, y1, x2, y2, bg, fj2, fj3, r1, save[16384];
  int   i, hi, nc, lo, ich, jch, ihi=0, ilo=0, ihi2, ilo2, save_numx;
  char  ans[80], command[80];

  for (i = 0; i < gf3gd.maxch + 1; ++i) {
    save[i] = gf3gd.spec[i];
  }
  strcpy(command, "DS 1");
  gfexec(command, 4);
  *maxspec = 1;
  printf("\n"
	 "This auto-background procedure uses a multiple of\n"
	 "   the 'starting width' at each channel as a length\n"
	 "   scale for finding the background.  If the results\n"
	 "   are unsatisfactory, check the width parameters\n"
	 "   with the 'SW' command.\n\n"
	 "You can also vary the length scale factor to\n"
	 "   fine-tune the background to your taste.\n\n");

  while (1) {
    nc = cask("Background length factor = ? (Return for 1.0)", ans, 80);
    w1 = 1.f;
    if (nc > 0 && ffin(ans, nc, &w1, &fj2, &fj3)) continue;
    /* do this first time through only */
    if (*maxspec == 1) {
      printf("\nLower limit for background = ? (A to abort)...\n");
      retic(&x1, &y1, ans);
      if (ans[0] == 'A' || ans[0] == 'a') return 0;
      printf("Upper limit for background = ? (A to abort)...\n");
      retic(&x2, &y2, ans);
      if (ans[0] == 'A' || ans[0] == 'a') return 0;
      if (x1 < x2) {
	ilo = x1;
	ihi = x2;
      } else {
	ilo = x2;
	ihi = x1;
      }
      if (ilo < 0) ilo = 0;
      if (ihi > gf3gd.maxch) ihi = gf3gd.maxch;
    }
    ilo2 = ihi;
    ihi2 = ilo;
    /* do three background-smoothing-type iterations */
    for (i = 1; i <= 3; ++i) {
      for (ich = ilo + 2; ich <= ihi - 2; ich += 5) {
	x = (float) ich;
	fwhm = sqrt(gf3gd.swpars[0] + gf3gd.swpars[1] * x + 
		    gf3gd.swpars[2] * x * x);
	r1 = w1 * w2 * fwhm;
	lo = ich - rint(r1);
	r1 = w1 * w2 * fwhm;
	hi = ich + rint(r1);
	if (lo < ilo) lo = ilo;
	if (hi > ihi) hi = ihi;
	find_bg2(lo, hi, &x1, &y1, &x2, &y2);
	if (ilo2 > rint(x1)) ilo2 = rint(x1);
	if (ihi2 < rint(x2)) ihi2 = rint(x2);
	for (jch = rint(x1); jch <= rint(x2); ++jch) {
	  x = (float) jch;
	  bg = y1 - (x1 - x) * (y1 - y2) / (x1 - x2);
	  gf3gd.spec[jch] = bg;
	}
      }
    }
    for (ich = ilo2; ich <= ihi2; ++ich) {
      gf3gd.spec[ich] += sqrt(gf3gd.spec[ich]) * 1.6f;
    }
    save_numx = gf3gd.numx;
    gf3gd.numx = 0;
    strcpy(command, "OV -1");
    gfexec(command, 5);
    gf3gd.numx = save_numx;
    *maxspec = 2;
    if (!caskyn(" ...Change length scale factor? (Y/N)")) return 0;
    for (i = 0; i < gf3gd.maxch + 1; ++i) {
      gf3gd.spec[i] = save[i];
    }
    strcpy(command, "DS 1");
    gfexec(command, 4);
  }
} /* autobkgnd */

/* ======================================================================= */
int autocal(void)
{
  double d1, chisqr, save_gain[6], pfa[6], pfx[50], pfy[50], pfdy[50];
  float area, diff, cent, fmax, fwhm, pmax, darea, x=0, y=0;
  float dcent, resid, x1, x2, eg, deg, ref, fit, dx, dy, rsigma, r1, r2, r3;
  float fx[100], fy[100], fde[100], fdx[100], fdy[100], chanx[100], psize[100];
  float sto[100][6], sou[100][8];
  int   w1 = 20, w2 = 15;
  int   nmap, imax, file_err, nnnx, nnny, nsto, nsou, npts;
  int   numxsave, i, j, save_ical, maxpk, hi, nc, lo, nx, ny, iorder, map[100];
  int   ipk, npk, nnx, nny, loxsave, save_nterms;
  char  q[256], title[256], sinnam[120], ans[120], command[80];
  static char sounam[80] = "eu152.sou";

  /* display over +/- w1,  integrate over +/- w2 */
  file_err = 0;
  while (1) {
   strcpy(q,"Input source data file = ?\n");
    if (!file_err) {
      strcat(q,"    (Return for default = ");
      strcat(q,sounam);
      strcat(q,")\n");
    }
    strcat(q,"    (default .ext = .sou)         ");
    nc = cask(q, ans, 80);
    if (nc == 0 && file_err) return 0;
    if (nc > 0) strncpy(sounam, ans, 80);
    setext(sounam, ".sou", 80);
    if ((file1 = open_readonly(sounam))) break;
    file_err = 1;
  }
  nsou = 0;
  while (fgets(ans, 120, file1) &&
	 4 == sscanf(ans+4, "%f %f %f %f", &sou[nsou][0], 
		     &sou[nsou][1], &sou[nsou][2], &sou[nsou][3])) {
    ++nsou;
  }
  if (nsou < 2) {
    printf("ERROR - need at least two source energies  in .sou file!\n");
    return 1;
  }
  fclose(file1);
  strcpy(command, "DS");
  gfexec(command, 2);
  for (i = 0; i < 6; ++i) {
    save_gain[i] = gf3gd.gain[i];
  }
  save_ical = gf3gd.ical;
  save_nterms = gf3gd.nterms;
  gf3gd.ical = 1;
  gf3gd.nterms = 2;
  /* do peak search */
  rsigma = 3.f;
  maxpk = 100;
  while (1) {
    pfind(chanx, psize, 1, gf3gd.maxch + 1, gf3gd.ifwhm, rsigma, maxpk, &npk);
    if (npk <= 30) break;
    rsigma += 1;
  }
  /* write(iw,*) npk, chanx(1), chanx(npk) */
  pmax = -100.f;
  for (ipk = 1; ipk <= npk; ++ipk) {
    if (psize[ipk-1] > pmax) pmax = psize[ipk-1];
  }
  ref = pmax / 200.f;
  /* do fancy peak search/integration */
  nsto = 0;
  for (ipk = 1; ipk <= npk; ++ipk) {
    if (psize[ipk-1] > ref) {
      x = (float) rint(chanx[ipk-1]);
      lo = x - w2;
      hi = x + w2;
      find_ge_cent(lo, hi, 1, &area, &cent, &fwhm, &darea, &dcent);
      if (area > 0.f && (nsto == 0 || cent > sto[nsto-1][0] + fwhm)) {
	printf(" ...Area, Cent, FWHM: %14.0f %9.3f %7.3f\n", area, cent, fwhm);
	sto[nsto][0] = cent;
	sto[nsto][2] = area;
	sto[nsto][4] = fwhm;
	++nsto;
      }
    }
  }

  while (nsto > 3 && sto[nsto-1][2] < sto[nsto-2][2] / 10.f) {
    --nsto;
  }
  printf("  Found %d peaks.\n", nsto);
  /* try to match peaks with source lines */
  imax = 0;
  gf3gd.gain[0] = 40.f;
  while (fabs(gf3gd.gain[0]) > 30.f && imax < nsou - 2) {
    fmax = sou[imax][2];
    for (i = imax; i < nsou - 1; ++i) {
      if (fmax < sou[i][2]) {
	fmax = sou[i][2];
	imax = i;
      }
    }
    gf3gd.gain[0] = 0.;
    gf3gd.gain[1] = sou[nsou-1][0] / sto[nsto-1][0];
    gf3gd.gain[2] = 0.;
    /* write(iw,*) gain(2) */
    x = sou[imax][0] / gf3gd.gain[1];
    if (x < sto[0][0]) {
      map[imax] = 1;
    } else if (x > sto[nsto-2][0]) {
      map[imax] = nsto - 1;
    } else {
      i = 1;
      while (i < nsto-2 && x > sto[i][0]) {
	++i;
      }
      x1 = (r1 = x - sto[i-1][0], fabs(r1));
      x2 = (r1 = x - sto[i][0], fabs(r1));
      if (x1 * sto[i][2] > x2 * sto[i-1][2]) {
	map[imax] = i + 1;
      } else {
	map[imax] = i;
      }
    }
    gf3gd.gain[1] = (sou[nsou-1][0] - sou[imax][0]) /
                      (sto[nsto-1][0] - sto[map[imax]-1][0]);
    gf3gd.gain[0] =  sou[nsou-1][0] - sto[nsto-1][0] * gf3gd.gain[1];
    gf3gd.gain[2] = 0.;
    imax++;
  }
  j = 0;
  for (i = 0; i < nsou; ++i) {
    x = channo(sou[i][0]);
    for (j = 0; j < nsto; ++j) {
      if ((r1 = x - sto[j][0], fabs(r1)) < sto[j][4]) goto FOUNDIT;
    }
  }
 FOUNDIT:
  gf3gd.gain[1] = (sou[nsou-1][0] - sou[i][0]) / (sto[nsto-1][0] - sto[j][0]);
  gf3gd.gain[0] =  sou[nsou-1][0] - sto[nsto-1][0] * gf3gd.gain[1];
  gf3gd.gain[2] = 0.;

  fmax = 1.f;
  nmap = 0;
  for (i = 0; i < nsou; ++i) {
    map[i] = 0;
    x = channo(sou[i][0]);
    for (j = 0; j < nsto; ++j) {
      if ((r1 = x - sto[j][0], fabs(r1)) < 2.f && map[i] == 0) {
	map[i] = j+1;
	fx[nmap] = sto[j][0];
	fy[nmap] = sou[i][0];
	if (energy(sto[j][0], 0.f, &eg, &deg)) goto QUIT;
	fde[nmap] = eg - sou[i][0];
	if ((r1 = fde[nmap], fabs(r1)) > fmax) {
	  fmax = (r2 = fde[nmap], fabs(r2));
	}
	nmap++;
      }
    }
  }
  if (fmax > 1.5f) {
    printf("*** Sorry, can't seem to match peaks with source!\n");
    goto QUIT;
  }
  initg(&nx, &ny);
  erase();
  finig();
  loxsave = gf3gd.lox;
  numxsave = gf3gd.numx;
  nnny = (nsou + 5) / 6;
  nnnx = (nsou - 1) / nnny + 1;
  nnx = 1;
  nny = nnny;
  nmap = 0;
  w1 = gf3gd.ifwhm << 2;
  w2 = gf3gd.ifwhm * 3;
  for (i = 0; i < nsou; ++i) {
    map[i] = 0;
    x = channo(sou[i][0]);
    gf3gd.lox = rint(x) - w1;
    if (gf3gd.lox < 0) gf3gd.lox = 0;
    gf3gd.numx = w1 << 1;
    lo = rint(x) - w2;
    hi = rint(x) + w2;
    dspsp2(nny, nnny, nnx, nnnx);
    if (++nnx > nnnx) {
      nnx = 1;
      --nny;
    }
    /* do fancy peak search/integration */
    find_ge_cent(lo, hi, 1, &area, &cent, &fwhm, &darea, &dcent);
    if (area > 0.f) {
      printf(" ...E, Area, Cent, FWHM, dCent: %8.2f %14.0f %8.2f %7.3f %6.2f\n",
      sou[i][0], area, cent, fwhm, dcent);
      if (nmap > 0 &&
	  map[i-1] == nmap &&
	  (r1 = cent - fx[nmap - 1], fabs(r1)) < fwhm / 3.f) {
	nmap--;
	printf(" *** OOPS, doublet at %.1f,%.1f (%.1f,%.1f) deleted!\n",
	       sou[i-1][0], sou[i][0], fx[nmap], cent);
	map[i-1] = 0;
      } else {
	map[i] = nmap + 1;
	fx[nmap] = cent;
	fdx[nmap] = dcent;
	fy[nmap] = sou[i][0];
	fdy[nmap] = sou[i][1];
	if (energy(cent, dcent, &eg, &deg)) goto QUIT;
	fde[nmap] = eg - sou[i][0];
	sto[nmap][0] = cent;
	sto[nmap][1] = dcent;
	sto[nmap][2] = area;
	sto[nmap][3] = darea;
	sto[nmap][4] = fwhm;
	nmap++;
      }
    }
  }
  sprintf(q, " Found %d peaks.\n"
	  "...Would you like to remove the data for any of the peaks? (Y/N)",
	  nmap);
  if (caskyn(q)) {
    initg(&nx, &ny);
    limg(nx, 0, ny, 0);
    r1 = (float) nnnx;
    r2 = (float) nnny;
    trax(r1, 0.f, r2, 0.f, 1);
    printf("  ...Select peaks to delete, X when done...\n");
    retic(&x, &y, ans);
    while (ans[0] != 'X' && ans[0] != 'X') {
      ipk = nnnx * (nnny - 1 - (int) y) + 1 + (int) x;
      if (ipk > nsou) {
	printf(" *** No peak selected, try again...\n");
      } else if (map[ipk-1] == 0) {
	printf(" *** Already have no data for that peak, try again...\n");
      } else {
	printf("   *** Peak at %.1f (%.1f) deleted.\n",
	       sou[ipk-1][0], fx[map[ipk-1] - 1]);
	--nmap;
	for (i = map[ipk-1]; i <= nmap; ++i) {
	  fx[i-1] = fx[i];
	  fdx[i-1] = fdx[i];
	  fy[i-1] = fy[i];
	  fdy[i-1] = fdy[i];
	  fde[i-1] = fde[i];
	  for (j = 0; j < 5; ++j) {
	    sto[i-1][j] = sto[i][j];
	  }
	}
	map[ipk-1] = 0;
	for (i = ipk; i < nsou; ++i) {
	  if (map[i] > 0) --map[i];
	}
      }
      retic(&x, &y, ans);
    }
    if (nmap < 2) {
      printf("   ...Less than 2 peaks left, exiting...\n");
      goto QUIT;
    }
  }
  /* save results in .sin file for effit */
  strncpy(sinnam, gf3gd.filnam, 80);
  i = setext(sinnam, ".sin", 90);
  if (!strcmp(sinnam + i, ".SPK") ||
      !strcmp(sinnam + i, ".spk") ||
      !strcmp(sinnam + i, ".MAT") ||
      !strcmp(sinnam + i, ".mat") || 
      !strcmp(sinnam + i, ".SPN") ||
      !strcmp(sinnam + i, ".spn") ||
      !strcmp(sinnam + i, ".M4B") ||
      !strcmp(sinnam + i, ".m4b")) {
    sinnam[i] = '_';
    sinnam[i+4] = '_';
    strncpy(&sinnam[i+5], gf3gd.namesp, 8);
    for (j = i + 5; j <= i + 7; ++j) {
      if (sinnam[j-1] == ' ') sinnam[j-1] = '0';
    }
    i = setext(sinnam, ".sin", 120);
  }
  strcpy(sinnam+i, ".sin");
  file3 = open_save_file(sinnam, 1);
  strcpy(title, "AUTOCAL calibration, spectrum ");
  strcat(title, gf3gd.filnam);
  strcat(title, ",  .sou file: ");
  strcat(title, sounam);
  title[80] = '\0';
  fprintf(file3, "%80s\n", title);
  for (i = 0; i < nsou; ++i) {
    if ((j = map[i]) != 0) {
      fprintf(file3, 
	     "%4i %11.4f %11.4f %11.0f %11.0f %11.4f %11.4f %11.0f %11.0f\n",
	     1, sto[j-1][0], sto[j-1][1], sto[j-1][2], sto[j-1][3],
	     sou[i][0], sou[i][1], sou[i][2], sou[i][3]);
    }
  }
  fclose(file3);
  /* display (matched_energies - slope*ch) -vs- channel number */
  fmax = .5f;
  for (i = 0; i < nmap; ++i) {
    if ((r1 = fde[i], fabs(r1)) > fmax)  fmax = (r2 = fde[i], fabs(r2));
  }
  initg(&nx, &ny);
  erase();
  limg(nx, 0, ny, 0);
  setcolor(1);
  r1 = fx[nmap - 1] * 1.1f;
  r2 = fmax *  3.0f;
  r3 = fmax * -1.5f;
  trax(r1, 0.f, r2, r3, 1);
  pspot(0.f, 0.f);
  vect(fx[nmap-1] * 1.1f, 0.f);
  setcolor(2);
  dx = fx[nmap-1] / 200.f;
  for (i = 0; i < nmap; ++i) {
    d1 = fdx[i] * gf3gd.gain[1];
    dy = sqrt(d1 * d1 + fdy[i] * fdy[i]);
    symbg(1, fx[i], fde[i], 11);
    pspot(fx[i] - dx, fde[i] - dy);
    vect (fx[i] + dx, fde[i] - dy);
    pspot(fx[i] - dx, fde[i] + dy);
    vect (fx[i] + dx, fde[i] + dy);
    pspot(fx[i],      fde[i] - dy);
    vect (fx[i],      fde[i] + dy);
    if (i > 0) {
      pspot(fx[i-1], fde[i-1]);
      vect (fx[i],   fde[i]);
    }
  }
  setcolor(1);
  finig();
  /* fit polynomials to energy calibration */
  npts = nmap;
  for (i = 0; i < npts; ++i) {
    pfx[i] = fx[i];
    pfy[i] = fy[i];
    d1 = fdx[i] * gf3gd.gain[1];
    r1 = fdy[i];
    pfdy[i] = sqrt(d1 * d1 + r1 * r1);
  }
  for (iorder = 1; iorder <= 2; ++iorder) {
    if (npts < iorder + 2) goto DONE;
    gf3gd.nterms = iorder + 1;
    for (i = 0; i < 6; ++i) {
      pfa[i] = 0.f;
    }
    polfit(pfx, pfy, pfdy, npts, gf3gd.nterms, 1, pfa, &chisqr);
    printf(" Order = %2d\n\n  Channel          Energy"
	   "             Fit      Difference    Residual\n", iorder);
    for (i = 0; i < npts; ++i) {
      fit = pfa[gf3gd.nterms - 1];
      for (j = iorder; j >= 1; --j) {
	fit = pfa[j-1] + fit * pfx[i];
      }
      diff = pfy[i] - fit;
      resid = diff / pfdy[i];
      *q = '\0';
      wrresult(q, fx[i], fdx[i], 16);
      wrresult(q, fy[i], fdy[i], 32);
      printf("%s %9.3f %11.3f %11.2f\n", q, fit, diff, resid);
    }
    printf(" Parameters =");
    for (i = 0; i < gf3gd.nterms; ++i) {
      printf("  %15.9f",  pfa[i]);
    }
    printf("\n Chisqr/D.O.F. = %9.3f\n   Display fullscale = %6.3fkeV\n",
	   chisqr, fmax*1.5f);

    initg(&nx, &ny);
    setcolor(3);
    for (i = 5; i <= (int) (fx[nmap - 1] * 1.1f); i += 10) {
      fit = pfa[gf3gd.nterms - 1];
      for (j = iorder; j >= 1; --j) {
	fit = pfa[j-1] + fit * (float) i;
      }
      r1 = (float) i;
      if (energy(r1, 0.f, &eg, &deg)) goto QUIT;
      if (i == 5) {
	r1 = (float) i;
	r2 = eg - fit;
	pspot(r1, r2);
      } else {
	r1 = (float) i;
	r2 = eg - fit;
	vect(r1, r2);
      }
    }
    setcolor(1);
    finig();
    if (caskyn("Do you want to save these values? (Y/N)")) {
      strncpy(ans, sinnam, 120);
      i = setext(ans, ".aca", 120);
      strcpy(ans+i, ".aca");
      if ((file1 = open_new_file(ans, 0))) {
	fprintf(file1, "  ENCAL OUTPUT FILE\n"
		" gf3 AUTOCAL calibration, spectrum %s\n", gf3gd.filnam);
	fprintf(file1, "%5d %18.11E %18.11E %18.11E\n"
		"      %18.11E %18.11E %18.11E\n", iorder,
		pfa[0], pfa[1], pfa[2], pfa[3], pfa[4], pfa[5]);
	fclose(file1);
	printf("   ...File %s created.\n", ans);
      }
    }
  }
 DONE:
  gf3gd.lox = loxsave;
  gf3gd.numx = numxsave;
  printf("\n File %s can be used for effit, encal.\n", sinnam);
 QUIT:
  /* strcpy(command, "DS");
     gfexec(command, 2); */
  gf3gd.disp = 0;
  /* */
  for (i = 0; i < 6; ++i) {
    gf3gd.gain[i] = save_gain[i];
  }
  gf3gd.ical = save_ical;
  gf3gd.nterms = save_nterms;
  return 0;
} /* autocal */

/* ======================================================================= */
int autocal3(void)
{
  float cent[1000][2], fwhm[1000][2];
  float area, a, b, darea, x, y, dcent, edisp, spsum[16384];
  float eg[2], fj1, fj2, oldch1, oldch2, newch1, newch2;
  int   mode, chlo[2], chhi[2], sphi, file_err, splo, nspx, nspy;
  int   fwhm0, numxsave, i, j, numch=0, w2, hi, nc, lo;
  int   ipk, isp, nnx, nny, sumspec, loxsave;
  char  q[256], line[160], dattim[20], ans[80];
  static char limnam[80] = "eu152.lim", outnam[80];

  file_err = 0;
  while (1) {
    strcpy(q,"Input limits file name = ?\n");
    if (!file_err) {
      strcat(q,"    (Return for default = ");
      strcat(q,limnam);
      strcat(q,")\n");
    }
    strcat(q,"    (default .ext = .lim)         ");
    nc = cask(q, ans, 80);
    if (nc == 0 && file_err) return 0;
    if (nc > 0) strncpy(limnam, ans, 80);
    j = setext(limnam, ".lim", 80);
    if ((file2 = open_readonly(limnam))) break;
    file_err = 1;
  }
  strncpy(outnam, gf3gd.filnam, 80);
  j = setext(outnam, ".ca3", 80);
  strcpy(outnam + j, ".ca3");

  strcpy(q,"Output data file name = ?\n    (Return for default = ");
  strcat(q,outnam);
  strcat(q,")\n    (default .ext = .ca3)         ");
  nc = cask(q, ans, 80);
  if (nc > 0) strncpy(outnam, ans, 80);
  setext(outnam, ".ca3", 80);
  if (!(file3 = open_new_file(outnam, 0))) return 1;

  datetime(dattim);
  fprintf(file3, "* gf3 autocalibrate type-3 output,  %.18s\n", dattim);
  fprintf(file3,
	  "* sp         a            b      Cent1  FWHM1      Cent2  FWHM2\n");
  sumspec = 0;
  while (caskyn("  ...Sum gain-adjusted spectra? (Y/N)")) {
    nc = cask("   ...desired energy dispersion = ?", ans, 80);
    if (!ffin(ans, nc, &edisp, &fj1, &fj2) && edisp > 0.f) {
      sumspec = 1;
      for (i = 0; i < 16384; ++i) {
	spsum[i] = 0.f;
      }
      break;
    }
  }

  loxsave = gf3gd.lox;
  numxsave = gf3gd.numx;
  while (fgets(line, 160, file2)) {
    if (strchr(line, '*')) continue;
    if (sscanf(line, " %d, %d, %f, %d, %d, %f, %d, %d, %d",
	       &splo, &sphi, &eg[0], &chlo[0], &chhi[0],
	       &eg[1], &chlo[1], &chhi[1], &fwhm0) != 9 ||
	splo > sphi || splo < 0 || sphi > 99999) {
      file_error("read", limnam);
      if (splo > sphi || splo < 0 || sphi > 99999)
	printf(" Spectrum lo, hi numbers out of order or out of bounds!\n");
      fclose(file2);
      fclose(file3);
      return 1;
    }
    nspx = (int) sqrt((float) (sphi - splo + 1));
    nspy = (sphi - splo) / nspx + 1;
    for (ipk = 0; ipk < 2; ++ipk) {
      nnx = 1;
      nny = 1;
      gf3gd.lox = chlo[ipk];
      gf3gd.numx = chhi[ipk] - chlo[ipk] + 1;
      erase();
      for (isp = splo; isp <= sphi; ++isp) {
	cent[isp][ipk] = 0.f;
	sprintf(ans, "SP %d", isp);
	nc = strlen(ans);
	getsp(ans, nc);
	dspsp2(nny, nspy, nnx, nspx);
	if (++nny > nspy) {
	  ++nnx;
	  nny = 1;
	}
	/* find highest counts in range chlo - chhi */
	x = 0.f;
	y = 0.f;
	for (i = chlo[ipk]; i <= chhi[ipk]; ++i) {
	  if (gf3gd.spec[i] > y) {
	    x = (float) i;
	    y = gf3gd.spec[i];
	  }
	}
	if (y < 10.f) continue;
	/* do fancy peak search/integration */
	w2 = fwhm0 * 3;
	lo = x - w2;
	hi = x + w2;
	find_ge_cent(lo, hi, 1, &area, &cent[isp][ipk],
		     &fwhm[isp][ipk], &darea, &dcent);
	if (area < 10.f) continue;
	printf(" Sp, Area, Cent, FWHM: %4d %14.0f %9.3f %6.3f\n",
	       isp, area, cent[isp][ipk], fwhm[isp][ipk]);
	if (ipk == 1 && sumspec) {
	  oldch1 = cent[isp][0];
	  oldch2 = cent[isp][1];
	  newch1 = eg[0] / edisp;
	  newch2 = eg[1] / edisp;
	  mode = 1;
	  if (numch < gf3gd.maxch + 1) numch = gf3gd.maxch + 1;
	  adjgain(mode, &oldch1, &oldch2, &newch1, &newch2, gf3gd.spec, 
		  spsum, gf3gd.maxch + 1, numch, 1.f);
	}
      }
      if (!caskyn("...OK? (Y/N)")) continue;
    }
    printf("* sp         a            b      Cent1  FWHM1      Cent2  FWHM2\n");
    for (isp = splo; isp <= sphi; ++isp) {
      if (cent[isp][0] == 0.f || cent[isp][1] == 0.f) continue;
      b = (eg[1] - eg[0]) / (cent[isp][1] - cent[isp][0]);
      a = eg[0] - (cent[isp][0] * b);
      printf(" %4d %9.3f %12.8f %10.3f %6.2f %10.3f %6.2f\n", isp, a, b,
	     cent[isp][0], fwhm[isp][0]*b, cent[isp][1], fwhm[isp][1]*b);
      fprintf(file3, " %4d %9.3f %12.8f %10.3f %6.2f %10.3f %6.2f\n", isp, a, b,
	     cent[isp][0], fwhm[isp][0]*b, cent[isp][1], fwhm[isp][1]*b);
    }
  }
  fclose(file2);
  fclose(file3);
  gf3gd.disp = 0;
  gf3gd.lox = loxsave;
  gf3gd.numx = numxsave;
  if (sumspec) {
    for (i = 0; i < numch; ++i) {
      gf3gd.spec[i] = spsum[i];
    }
    strncpy(gf3gd.namesp, "CA3_SUM ", 8);
    gf3gd.maxch = numch - 1;
  }
  return 0;
} /* autocal3 */

/* ======================================================================= */
int autocal4(void)
{
  float area, darea, cent[2], dcent[2], fwhm, fwhm0, a, b, x=0, y, r1;
  float eg[2], fj1, fj2, sou[52];
  int   peak, i, w2, hi, nc, lo, nl = 0;
  char  fullname[120], ans[80], q[256];
  static char id[52] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  printf("If the results of this routine are unsatisfactory,\n"
	 "  try playing with the starting width parameters.\n\n");
  peak = 0;
  while (peak < 2) {
    printf(" Use cursor to select peak number %d, X to exit...\n", peak+1);
    retic(&x, &y, ans);
    if (ans[0] == 'X' || ans[0] == 'x') return 0;
    fwhm0 = sqrt(gf3gd.swpars[0] + gf3gd.swpars[1] * x + 
		 gf3gd.swpars[2] * x * x);
    if (fwhm0 < 3.f) fwhm0 = 3.f;
    i = rint(x);
    y = gf3gd.spec[i];
    if (y < 4.f) {
      printf("Sorry, that peak is too weak!\n");
      continue;
    }
    r1 = fwhm0 * 3.f;
    w2 = rint(r1);
    lo = i - w2;
    hi = i + w2;
    find_ge_cent(lo, hi, 0, &area, &cent[peak], &fwhm, &darea, &dcent[peak]);
    if (area < 10.f) {
      printf("Sorry, that peak is too weak!\n");
      continue;
    }
    r1 = fwhm * 4.f;
    w2 = rint(r1);
    lo = rint(cent[peak]) - w2;
    hi = rint(cent[peak]) + w2;
    find_ge_cent(lo, hi, 1, &area, &cent[peak], &fwhm, &darea, &dcent[peak]);

    /* write results */
    strcpy(q, " Centroid:");
    wrresult(q, cent[peak], dcent[peak], 0);
    strcat(q, "    Area:");
    wrresult(q, area, darea, 0);
    printf("%s    FWHM: %.3f\n", q, fwhm);
    peak++;
  }
  /* now check for a calib.sou file */
  if (!(file1 = fopen("calib.sou", "r"))) {
    /* try in the home directory */
    get_directory("HOME", fullname, 100);
    strcat(fullname, "calib.sou");
    file1 = fopen(fullname, "r");
  }
  if (file1) {
    printf(" Data from file calib.sou:\n ID                Energy\n");
    for (nl = 0; nl < 52; ++nl) {
      if (!fgets(ans, 80, file1)) break;
      printf(" %c %s", id[nl], ans);
      sscanf(ans, " %f", &sou[nl]);
    }
  } else {
    printf(" No file calib.sou or HOME/calib.sou was found.\n"
	   " You can create calib.sou to make this part easier!\n");
  }
  for (peak = 0; peak < 2; ++peak) {
    if (file1) {
      sprintf(q, " Enter an energy or an ID for peak number %d\n"
		 "                              (rtn to exit) ", peak+1);
    } else {
      sprintf(q, " Enter an energy for peak number %d\n"
		 "                     (rtn to exit) ", peak+1);
    }
    while (1) {
      if ((nc = cask(q, ans, 80)) == 0) return 0;
      if (!ffin(ans, nc, &eg[peak], &fj1, &fj2)) break;
      for (i = 0; i < nl; ++i) {
	if (ans[0] == id[i]) {
	  eg[peak] = sou[i];
	  break;
	}
      }
      if (i < nl) break;
      printf("ERROR: no energy or ID could be decoded, try again.\n");
    }
  }
  if (file1) fclose(file1);
  b = (eg[1] - eg[0]) / (cent[1] - cent[0]);
  a = eg[0] - cent[0] * b;
  printf("Energy calibration coefficients are %.3f %.8f\n", a, b);
  gf3gd.ical = 1;
  gf3gd.nterms = 2;
  gf3gd.gain[0] = a;
  gf3gd.gain[1] = b;
  for (i = 2; i < 6; ++i) {
    gf3gd.gain[i] = 0.f;
  }
  return 0;
} /* autocal4 */

/* ======================================================================= */
int autofit(float *sens, int lo_ch, int hi_ch)
{
  float save[16384], pmax, fwhm0, s, w, x, y, chanx[100];
  float psize[100], x1=0, x2=0, rsigma, ref, fj;
  int   i, j, fitter_rtn, jfwhm, maxpk, first;
  int   quit_next, maxits, ipk, npk, nc, j1, j2;
  char  ans[80];

  /* sensitivity */
  while (*sens <= .001f) {
    *sens = 1.f;
    if (!(nc = cask("Autofit sensitivity = ? (rtn for 1.0)", ans, 80))) break;
    if (ffin(ans, nc, sens, &fj, &fj)) *sens = 0.0f;
  }
	
  first = 1;
  quit_next = 0;
  if (lo_ch > 0 && hi_ch - lo_ch > 20) {
    x1 = gf3gd.mch[0] = lo_ch;
    x2 = gf3gd.mch[1] = hi_ch;
  } else {
    printf("If the results of this routine are unsatisfactory,\n"
	   "  try playing with the starting width parameters,\n"
	   "  or the initial estimates of the other parameters.\n\n"
	   "Use cursor to limits of fitted region, X to exit...\n");
    retic(&x1, &y, ans);
    if (ans[0] == 'X' || ans[0] == 'x') return 0;
    retic(&x2, &y, ans);
    if (ans[0] == 'X' || ans[0] == 'x') return 0;
    ready = 0;
    if (x1 > x2) {
      gf3gd.mch[1] = rint(x1);
      gf3gd.mch[0] = rint(x2);
    } else {
      gf3gd.mch[0] = rint(x1);
      gf3gd.mch[1] = rint(x2);
    }
  }
  /* do peak search */
  x = (x1 + x2) / 2.f;
  fwhm0 = sqrt(gf3gd.swpars[0] + gf3gd.swpars[1] * x + gf3gd.swpars[2]
	       * x * x);
  if (fwhm0 < 3.f) fwhm0 = 3.f;
  jfwhm = rint(fwhm0);
  rsigma = 4.f / *sens;
  maxpk = 100;
  while (1) {
    pfind(chanx, psize, gf3gd.mch[0], gf3gd.mch[1], jfwhm, rsigma, maxpk, &npk);
    printf(" %d %.2f %.2f\n", npk, chanx[0]-1.f, chanx[npk-1]-1.f);
    if (npk <= 0) {
      printf("No peaks found in that region, sorry!\n");
      return 1;
    }
    if (npk <= 30) break;
    rsigma *= 1.5f;
  }
  pmax = -100.f;
  for (ipk = 0; ipk < npk; ++ipk) {
    if (psize[ipk] > pmax) pmax = psize[ipk];
  }
  ref = pmax / 50.f;
  gf3gd.npks = 0;
  for (ipk = 0; ipk < npk; ++ipk) {
    if (psize[ipk] >= ref) {
      ++gf3gd.npks;
      gf3gd.ppos[gf3gd.npks - 1] = chanx[ipk] - 1.f;
    }
  }
  printf("PPos:");
  for (j = 0; j < gf3gd.npks; ++j) {
    printf(" %.2f", gf3gd.ppos[j]);
  }
  printf("\n");
  while (1) {
  NEXTFIT:
    dspmkr(99);
    ready = 1;
    gf3gd.npars = (gf3gd.npks + 2) * 3;
    for (i = 0; i < gf3gd.npars; ++i) {
      gf3gd.freepars[i] = 1;
    }
    gf3gd.irelpos = 1;
    gf3gd.nfp = 0;

    /* do fit */
    parset(-1);
    maxits = 50;
    gf3gd.irelpos = 0;
    fitter_rtn = fitter(maxits);
    if (gotsignal) return 1;
    gf3gd.irelpos = 1;
    fitter_rtn = fitter(maxits);
    if (gotsignal) return 1;
    for (i = 1; i <= gf3gd.npks; ++i) {
      if (gf3gd.pars[i*3 + 5] < 0.f) {
	if (!first) {
	  printf("One of the peaks (at %.2f -> %.2f) went negative, ending autofit...\n",
		 gf3gd.ppos[i-1], gf3gd.pars[i*3 + 3]);
	  quit_next = 1;
	}
	printf("Deleting negative peak %d of %d, at %.2f\n", i, gf3gd.npks, gf3gd.ppos[i-1]);
	if (gf3gd.npks < 2) {
	  printf("Sorry, not enough peaks...\n");
	  return 1;
	}
	sprintf(ans, "DP %d", i);
	gfexec(ans, strlen(ans));
	goto NEXTFIT;
      }
      if (gf3gd.pars[i*3 + 3] < (float) gf3gd.mch[0] ||
	  gf3gd.pars[i*3 + 3] > (float) gf3gd.mch[1]) {
	printf("ACKK! One of the peaks moved outside the fitted region!\n"
	       "Deleting peak %d of %d, at %.2f\n", i, gf3gd.npks, gf3gd.ppos[i-1]);
	if (gf3gd.npks < 2) {
	  printf("Sorry, not enough peaks...\n");
	  return 1;
	}
	sprintf(ans, "DP %d", i);
	gfexec(ans, strlen(ans));
	goto NEXTFIT;
      }
      gf3gd.ppos[i-1] = gf3gd.pars[i*3 + 3];
    }

    if (gf3gd.npks == 35 || quit_next) break;
    first = 0;

    /* do peak search on the residuals, and add at most one peak */
    j1 = gf3gd.mch[0] - jfwhm;
    j2 = gf3gd.mch[1] + jfwhm;
    if (j1 < 0) j1 = 0;
    if (j2 > gf3gd.maxch) j2 = gf3gd.maxch;
    eval(gf3gd.pars, 0, &y, gf3gd.npks, -9);
    for (i = j1; i <= j2; ++i) {
      save[i] = gf3gd.spec[i];
      eval(gf3gd.pars, i, &y, gf3gd.npks, 0);
      /* w = gf3gd.spec[i]; */
      w = y;
      if (w < 1.f) w = 1.f;
      gf3gd.spec[i] = (gf3gd.spec[i] - y) / sqrt(w);
    }
    rsigma = 2.f / *sens;
    maxpk = 100;
    printf("fwhm for search is %d\n", jfwhm);
    while (1) {
      /* pfind(chanx, psize, gf3gd.mch[0], gf3gd.mch[1], jfwhm, rsigma, maxpk, &npk); */
      pfind(chanx, psize, j1, j2, jfwhm, rsigma, maxpk, &npk);
      printf("%d residual peaks found...\n", npk);
      if (npk <= 20) break;
      rsigma *= 1.5f;
    }
    for (i = j1; i <= j2; ++i) {
      gf3gd.spec[i] = save[i];
    }
    if (npk <= 0) break;

    /* not all done, there are more peaks to add */
    pmax = -100.f;
    for (ipk = 0; ipk < npk; ++ipk) {
      printf("%3d %9.2f %9.0f\n", ipk+1, chanx[ipk] - 1.f, psize[ipk]);
      if (psize[ipk] > pmax) {
	pmax = psize[ipk];
	gf3gd.ppos[gf3gd.npks] = chanx[ipk] - 1.f;
      }
    }
    printf("Adding peak at %.2f\n", gf3gd.ppos[gf3gd.npks]);
    gf3gd.npks++;
    /* order peak energies */
    for (i = gf3gd.npks-1; i > 0; --i) {
      if (gf3gd.ppos[i] < gf3gd.ppos[i-1]) {
	s = gf3gd.ppos[i-1];
	gf3gd.ppos[i-1] = gf3gd.ppos[i];
	gf3gd.ppos[i] = s;
      }
    }
    printf("PPos:");
    for (j = 0; j < gf3gd.npks; ++j) {
      printf(" %.2f", gf3gd.ppos[j]);
    }
    printf("\n");
  }

  /* all done, show results and return */
  strcpy(ans, "RD");
  gfexec(ans, 2);
  strcpy(ans, "DM");
  gfexec(ans, 2);
  strcpy(ans, "DF");
  gfexec(ans, 2);
  if (fitter_rtn == 2)
    printf(" WARNING - convergence failed.\n"
	   "  Do not believe quoted parameter values!\n");
  gffin();
  return 0;
} /* autofit */

/* ======================================================================= */
int autopeak(char *command)
{
  float area, cent = 0.0f, dcent = 0.0f, fwhm, fwhm0, darea, x=0, y, eg, deg, r1;
  float a2=0, da2=0, da3, r, dr, dr2;
  int   i, cmd = 0, in[3]={0}, w2, hi, lo;
  char  ans[80], l[120];

  printf("If the results of this routine are unsatisfactory,\n"
	 "  try playing with the starting width parameters.\n\n"
	 "Use cursor to select peaks, X to exit...\n");

  if (strlen(command) > 2) {
    // try to see if there is a channel number on the command line
    // if there is, us that instead of cursor
    inin(command+2, strlen(command)-2, in, in+1, in+2);
    if (in[0] > 0 && in[0] < gf3gd.maxch) cmd = in[0];
  }
  if (!cmd && !gf3gd.disp) {
    printf("Bad command: New spectrum not yet displayed...\n");
    return 1;
  }

  if (energy(cent, dcent, &eg, &deg)) {
    printf(" Centroid         Area                 FWHM\n");
  } else {
    printf(" Centroid         Energy           Area                  FWHM\n");
  }
  while (1) {
    if (cmd) {
      x = cmd;
    } else {
      retic(&x, &y, ans);
      if (ans[0] == 'X' || ans[0] == 'x') return 0;
    }
    fwhm0 = sqrt(gf3gd.swpars[0] + gf3gd.swpars[1] * x + gf3gd.swpars[2] * x * x);
    if (fwhm0 < 3.f) fwhm0 = 3.f;
    i = rint(x);
    y = gf3gd.spec[i];
    if (y < 4.f) {
      printf("Sorry, that peak is too weak!\n");
      if (cmd) break;
      continue;
    }
    r1 = fwhm0 * 3.f;
    w2 = rint(r1);
    lo = i - w2;
    hi = i + w2;
    find_ge_cent(lo, hi, 0, &area, &cent, &fwhm, &darea, &dcent);
    if (area < 10.f) {
      printf("Sorry, that peak is too weak!\n");
      if (cmd) break;
      continue;
    }
    r1 = fwhm * 4.f;
    w2 = rint(r1);
    lo = rint(cent) - w2;
    hi = rint(cent) + w2;
    find_ge_cent(lo, hi, 1, &area, &cent, &fwhm, &darea, &dcent);
    if (area < 4.f) {
      printf("Sorry, that peak is too weak!\n");
      if (cmd) break;
      continue;
    }

    /* write results */
    *l= '\0';
    wrresult(l, cent, dcent, 16);
    if (!energy(cent, dcent, &eg, &deg)) {
      wrresult(l, eg, deg, 32);
      energy(cent, fwhm, &eg, &fwhm0);
      wrresult(l, area, darea, 50);
      printf("%s %7.3f chs = %.3f keV", l, fwhm, fwhm0);
    } else {
      wrresult(l, area, darea, 34);
      printf("%s %7.3f", l, fwhm);
    }
    if (a2 > 0 && area > 0) {
      y = darea / area;
      x = da2/a2;
      r = a2/area;
      dr = r * sqrt(x*x + y*y);
      if (area != a2) {
        da3 = (darea*darea - da2*da2);
      } else {
        da3 = 0;
      }
      if (da3 < 0) da3 *= -1.0;
      if (da3 < fabs(area - a2)) da3 = fabs(area - a2);
      da3 /= (area - a2)*(area - a2);
      if (r < 1) {
        dr2 = (1.0 - r) * sqrt(da3 + y*y);
      } else {
        dr2 = (r - 1.0) * sqrt(x*x + da3);
      }
      *l= '\0';
      wrresult(l, r, dr, 10);
      //printf("   area_ratio = %s\n", l);
      printf("   area_ratio = %.4f +- %.4f", r, dr);
      if (dr2 < dr) printf(" (%.4f)", dr2);
      a2 = 0;
    } else {
      a2 = area;
      da2 = darea;
    }
    printf("\n");

    if (in[1] > 0 && in[1] < gf3gd.maxch) {
      cmd = in[1];
      in[1] = in[2];
      in[2] = 0;
    } else if (cmd) {
      break;
    }
  }
  return 0;
} /* autopeak */

/* ======================================================================= */
void breakhandler(int dummy)
{
  gotsignal = 1;
}

/* ======================================================================= */
double channo(float egamma)
{
  static float diff = .002f;
  double e, x1, x2, d1;
  float  ret_val;
  int    i;

  /* the value of diff fixes the precision when nterms > 3 */
  if (gf3gd.nterms <= 3) {
    if (gf3gd.gain[2] < 1e-7) {
      x1 = (egamma - gf3gd.gain[0]) / gf3gd.gain[1];
      ret_val = (egamma - gf3gd.gain[0]) / (gf3gd.gain[1] + 
					    gf3gd.gain[2] * x1);
    } else {
      ret_val = (sqrt(gf3gd.gain[1] * gf3gd.gain[1] -
		      gf3gd.gain[2] * 4.0 * (gf3gd.gain[0] - egamma)) -
		 gf3gd.gain[1]) / (gf3gd.gain[2] * 2.0);
    }
  } else {
    x1 = (egamma - gf3gd.gain[0]) / gf3gd.gain[1];
    x1 = (egamma - gf3gd.gain[0]) / (gf3gd.gain[1] + gf3gd.gain[2] * x1);
    
    while (1) {
      e = gf3gd.gain[gf3gd.nterms - 1];
      for (i = gf3gd.nterms - 2; i > 1; --i) {
	e = gf3gd.gain[i] + e * x1;
      }
      x2 = e * x1 * x1;
      x2 = (egamma - gf3gd.gain[0] - x2) / gf3gd.gain[1];
      if ((d1 = x2 - x1, fabs(d1)) <= diff) break;
      x1 = x2;
    }
    ret_val = x2;
  }
  return ret_val;
} /* channo */

/* ======================================================================= */
int chngmark(int mnum)
{
  /* Allows the user to change the fitting limits and/or
     the peak positions and to optionally reset the free parameters
     to their initial estimates.
     mch(mn),  (mn = 1,2) are the lower and upper bounds
               of the fitting region
     ppos(mn-2), (mn = 3,4..npks+2) are the peak positions */

  int   mn;

  mn = mnum;
  if (mn <= 0 || mn > gf3gd.npks + 2) {
    /* the data passed does not correspond to a marker
       or there was no data passed */
    if (caskyn("Change limits? (Y/N)")) {
      for (mn = 1; mn <= 2; ++mn) {
	gf3gd.mch[mn-1] = rint(getmkrchnl(mn));
	dspmkr(mn);
      }
    }
    if (caskyn("Change peak positions? (Y/N)")) {
      for (mn = 3; mn <= gf3gd.npks + 2; ++mn) {
	gf3gd.ppos[mn-3] = getmkrchnl(mn);
	dspmkr(mn);
      }
    }
    
  } else {
    /* if the user knows what s/he wants to change, do it and return*/
    if (mn < 3) {
      printf("  New marker position? (hit T for type)");
      gf3gd.mch[mn-1] = (int) (0.5f + getmkrchnl(mn));
    } else {
      printf("  New peak position? (hit T for type)");
      gf3gd.ppos[mn-3] = getmkrchnl(mn);
    }
    dspmkr(mn);
  }
  if (caskyn("Reset free pars. to initial estimates? (Y/N)")) parset(0);
  return 0;
} /* chngmark */

/* ======================================================================= */
int comfil(char *filnam, int nc)
{
  int  iw = 0, j;
  char *c;
  FILE *file;
  static FILE *cf_file[20];
  static char cmd_fn[20][80];
  static int  cf_depth = 0;

  if (!strncmp(filnam, "SH", 2)) {
    /* open a pipe to a shell command */
    if (cf_depth > 18) {
      warn("%c*** Sorry.  I can't open another shell or command file.\n"
	   "    You have nested too many already.\n", '\7');
      return 1;
    }
    if (!(file = popen(filnam+2, "r"))) {
      printf("ERROR -- pipe not opened!  Shell command = %s\n", filnam+2);
      return 1;
    }
    if (cflog) fclose(cffile);
    cflog = 0;
    /* if there is a file open, but not actively being read (from cf chk)
       then close it and decriment cf_depth */
    if (cfopen && cf_depth > 0 && !infile) {
      tell("Closing shell or command file %d: %s\n", cf_depth-1, cmd_fn[cf_depth-1]);
      fclose(cffile);
      --cf_depth;
    }

    cfopen = 1;
    infile = cffile = file;
    strncpy(cmd_fn[cf_depth], filnam, 80);
    cf_file[cf_depth++] = file;
    tell("Opened shell %d: %s\n", cf_depth-1, filnam+2);
    return 0;
  }

  /* not a shell; handle command file commands */
  strncpy(filnam, "  ", 2);
  if (nc < 3) {
    /* ask for command file name */
  GETFILNAM:
    infile = 0;
    nc = cask("Command file name = ? (default .ext = .cmd)", filnam, 80);
    if (nc == 0) return 0;
  } else if ((c = strstr(filnam, "WAI")) ||
	     (c = strstr(filnam, "wai"))) {
    inin(c + 3, strlen(c + 3), &iw, &j, &j);
  }
  setext(filnam, ".cmd", 80);

  if (!strncmp(filnam, "END", 3) || !strncmp(filnam, "end", 3)) {
    /* close command file */
    if (cfopen || cflog) {
      tell("Closing shell or command file %d: %s\n", cf_depth-1, cmd_fn[cf_depth-1]);
      fclose(cffile);
      if (--cf_depth > 0) {
	cfopen = 1;
	infile = cffile = cf_file[cf_depth-1];
	tell("Now returning to shell or command file %d: %s\n", cf_depth-1, cmd_fn[cf_depth-1]);
      } else {
	cfopen = 0;
	infile = 0;
      }
    } else {
      printf("Command file not open.\n");
    }
    cflog = 0;
  } else if (!strncmp(filnam, "CON", 3) || !strncmp(filnam, "con", 3)) {
    if (cfopen) {
      infile = cffile;
    } else {
      printf("Command file not open.\n");
    }
  } else if (!strncmp(filnam, "CHK", 3) || !strncmp(filnam, "chk", 3)) {
    if (cfopen) {
      infile = 0;
      if (!caskyn("Proceed? (Y/N)")) return 0;
      infile = cffile;
    } else if (!cflog) {
      printf("Command file not open.\n");
    }
  } else if (!strncmp(filnam, "ERA", 3) || !strncmp(filnam, "era", 3)) {
    erase();
  } else if (!strncmp(filnam, "WAI", 3) || !strncmp(filnam, "wai", 3)) {
    if (iw <= 0) return 0;
    gotsignal = 0;
    signal(SIGINT, breakhandler);
    sleep(iw);
    signal(SIGINT, SIG_DFL);
    if (gotsignal) infile = 0;
  } else if (!strncmp(filnam, "LOG", 3) || !strncmp(filnam, "log", 3)) {
    while (1) {
      nc = askfn(filnam, 80, "", ".cmd", "File name for command logging = ?");
      if (nc == 0) return 0;
      if (strncmp(filnam, "END", 3) && strncmp(filnam, "end", 3) &&
	  strncmp(filnam, "CON", 3) && strncmp(filnam, "con", 3) &&
	  strncmp(filnam, "CHK", 3) && strncmp(filnam, "chk", 3) &&
	  strncmp(filnam, "ERA", 3) && strncmp(filnam, "era", 3) &&
	  strncmp(filnam, "WAI", 3) && strncmp(filnam, "wai", 3) &&
	  strncmp(filnam, "LOG", 3) && strncmp(filnam, "log", 3) &&
	  strncmp(filnam, "SH", 2)) break;
      printf("*** That is an illegal command file name. ***\n");
    }
    /* open log command file for output
       all input from stdin will be copied to log file */
    if ((file = open_new_file(filnam, 0))) {
      if (cfopen || cflog) fclose(cffile);
      cf_depth = 0;
      cfopen = 0;
      cflog = 0;
      infile = 0;
      cflog = 1;
      cffile = file;
    }
  } else {
    /* CF filename
       open command file for input */
    if (cfopen && cf_depth > 0 && !strncmp(filnam, cmd_fn[cf_depth-1], 79)) {
      tell("Rewinding commmand file %d: %s\n", cf_depth-1, filnam);
      rewind(cffile);
      infile = cffile;
      return 0;
    }
    if (cf_depth > 18) {
      warn("%c*** Sorry.  I can't open another shell or command file.\n"
	   "    You have nested too many already.\n", '\7');
      return 1;
    }

    if (!(file = fopen(filnam, "r+"))) {
      file_error("open", filnam);
      goto GETFILNAM;
    }
    if (cflog) fclose(cffile);
    cflog = 0;
    /* if there is a file open, but not actively being read (from cf chk)
       then close it and decriment cf_depth */
    if (cfopen && cf_depth > 0 && !infile) {
      tell("Closing shell or command file %d: %s\n", cf_depth-1, cmd_fn[cf_depth-1]);
      fclose(cffile);
      --cf_depth;
    }

    cfopen = 1;
    infile = cffile = file;
    strncpy(cmd_fn[cf_depth], filnam, 80);
    cf_file[cf_depth++] = file;
    tell("Opened commmand file %d: %s\n", cf_depth-1, filnam);
  }
  return 0;
} /* comfil */

/* ======================================================================= */
int contract(int *cfact)
{
  float a, b, c, d=0.0f, e=0.0f, f, f1;
  int   i, j, j1, j2, nc, ii;
  char  cmess[71], ans[80];

  /* contract spectrum by factor cfact */
  while (*cfact < 2) {
    nc = cask("Contraction factor = ?", ans, 80);
    if (nc == 0) return 0;
    if (inin(ans, nc, cfact, &j1, &j2)) cfact = 0;
  }
  gf3gd.maxch = (gf3gd.maxch + 1) / *cfact - 1;
  for (i = 0; i <= gf3gd.maxch; ++i) {
    ii = i * *cfact;
    gf3gd.spec[i] = gf3gd.spec[ii];
    for (j = ii + 1; j < ii + *cfact; ++j) {
      gf3gd.spec[i] += gf3gd.spec[j];
    }
  }
  strncpy(gf3gd.namesp + 4, ".MOD", 4);
  if (gf3gd.ical == 0) return 0;
  f = (float) (*cfact);
  f1 = (f - 1.f) / 2.f;
  if (!energy(f1, f, &a, &b)) {
    c = gf3gd.gain[2] * f * f;
    if (gf3gd.nterms > 3) {
      d = gf3gd.gain[3] * f * f * f;
      c = gf3gd.gain[3] * f * f * f1 * 3.f + c;
      if (gf3gd.nterms > 4) {
	e = gf3gd.gain[4] * f * f * f * f;
	d = gf3gd.gain[4] * f * f * f * f1 * 4.f + d;
	c = gf3gd.gain[4] * f * f * f1 * f1 * 6.f + c;
	if (gf3gd.nterms > 5) {
	  e = gf3gd.gain[5] * f * f * f * f * f1 * 5.f + e;
	  d = gf3gd.gain[5] * f * f * f * f1 * f1 * 10.f + d;
	  c = gf3gd.gain[5] * f * f * f1 * f1 * f1 * 10.f + c;
	}
      }
    }
    sprintf(cmess, " New gain coeffs %.2f %.4f %.6f etc? (Y/N)", a, b, c);
    if (caskyn(cmess)) {
      gf3gd.gain[0] = a;
      gf3gd.gain[1] = b;
      gf3gd.gain[2] = c;
      if (gf3gd.nterms > 3) gf3gd.gain[3] = d;
      if (gf3gd.nterms > 4) gf3gd.gain[4] = e;
      if (gf3gd.nterms > 5) gf3gd.gain[5] = gf3gd.gain[5] * f * f * f * f * f;
    }
  }
  return 0;
} /* contract */

/* ======================================================================= */
int curse(int *chnum)
{
  float dx = 0.0f, x=0, y=0, eg, deg, r1;
  int   isave, j1, j2, nc;
  char  ans[80];

  /* call or display cursor */
  if (*chnum != 0) {
    isave = gf3gd.mch[0];
    gf3gd.mch[0] = *chnum;
    dspmkr(1);
    gf3gd.mch[0] = isave;
    r1 = (float) (*chnum);
    if (energy(r1, dx, &eg, &deg)) {
      printf("     Ch = %5d  Cnts = %.0f\n",
	     *chnum, gf3gd.spec[*chnum]);
    } else {
      printf("     Ch = %5d  Cnts = %10.0f  Energy = %.2f\n",
	     *chnum, gf3gd.spec[*chnum], eg);
    }
    return 0;
  }
  
  printf("Hit any character;"
	 " X or right mouse button to exit, T to type ch. no.\n");
  while (1) {
    retic(&x, &y, ans);
    if (ans[0] == 'X' || ans[0] == 'x') return 0;
    if (ans[0] == 'T' || ans[0] == 't') {
      nc = cask("Channel number = ?", ans, 39);
      if (nc == 0 || inin(ans, nc, chnum, &j1, &j2)) continue;;
      isave = gf3gd.mch[0];
      gf3gd.mch[0] = *chnum;
      dspmkr(1);
      gf3gd.mch[0] = isave;
      r1 = (float) (*chnum);
      if (energy(r1, dx, &eg, &deg)) {
	printf("     Ch = %5d  Cnts = %.0f\n",
	       *chnum, gf3gd.spec[*chnum]);
      } else {
	printf("     Ch = %5d  Cnts = %10.0f  Energy = %.2f\n",
	       *chnum, gf3gd.spec[*chnum], eg);
      }
    } else {
      *chnum = x;
      r1 = (float) (*chnum);
      if (energy(r1, dx, &eg, &deg)) {
	printf("     Ch = %5d  Cnts = %10.0f  y = %.0f\n",
	       *chnum, gf3gd.spec[*chnum], y);
      } else {
	printf("     Ch = %5d  Cnts = %10.0f  y = %10.0f  Energy = %.2f\n",
	       *chnum, gf3gd.spec[*chnum], y, eg);
      }
    }
  }
} /* curse */

/* ======================================================================= */
double determ(double *array, int norder)
{
  double save, ret_val;
  int    i, j, k;

  ret_val = 1.f;
  for (k = 0; k < norder; ++k) {
    /* interchange columns if diagonal element is zero */
    if (array[k + k*10] == 0.f) {
      for (j = k; j < norder; ++j) {
	if (array[k + j*10] != 0.f) break;;
      }
      if (j >= norder) return 0.f;
      for (i = k; i < norder; ++i) {
	save = array[i + j*10];
	array[i + j*10] = array[i + k*10];
	array[i + k*10] = save;
      }
      ret_val = -ret_val;
    }
    /* subtract row k from lower rows to get diagonal matrix */
    ret_val *= array[k + k*10];
    for (i = k+1; i < norder; ++i) {
      for (j = k+1; j < norder; ++j) {
	array[i + j*10] -= array[i + k*10] * array[k + j*10] / array[k + k*10];
      }
    }
  }
  return ret_val;
} /* determ */

/* ======================================================================= */
int dispwin(void)
{
  /* display windows as they are presently defined
     winmod = 0 : no mode defined
     winmod = 1 : look-up file mode
     winmod = 2 : slice file mode */

  float ptot, x, y, bg, sum;
  int   i, isave, nx, ny, ihi, ilo, luv;
  char  cluv[40], l[120];

  if (winmod == 0) {
    printf("Bad command: No window file open...\n");
    return 1;
  } else if (!gf3gd.disp) {
    printf("Bad command: New spectrum not yet displayed...\n");
    return 1;
  }
  isave = gf3gd.mch[0];
  if (winmod == 2) {
    /* slice mode */
    rewind(winfile);
    luv = 0;
    while (fgets(l, 120, winfile) &&
	   sscanf(l, "%*5c%d%*3c%d%*9c%f", &ilo, &ihi, &ptot) == 3) {
      luv++;
      if (ilo > gf3gd.hich || ihi < gf3gd.loch) continue;
      /* display limits */
      gf3gd.mch[0] = ilo;
      dspmkr(1);
      gf3gd.mch[0] = ihi;
      dspmkr(1);
      /* display background */
      sum = 0.f;
      for (i = ilo; i <= ihi; ++i) {
	sum += gf3gd.spec[i];
      }
      bg = sum * (1.f - ptot) / (float) (ihi - ilo + 1);
      initg(&nx, &ny);
      x = (float) ilo + .5f;
      pspot(x, bg);
      x = (float) ihi + .5f;
      vect(x, bg);
      /* display window number */
      y = (gf3gd.spec[ihi] + gf3gd.locnt) / 2;
      pspot(x, y);
      sprintf(cluv, "%d", luv);
      putg(cluv, strlen(cluv), 9);
      finig();
    }
    gf3gd.mch[0] = isave;
    return 0;
  } else {
    /* look-up mode */
    ilo = 0;
    luv = 0;
    for (i = 0; i < gf3gd.nclook; ++i) {
      if (gf3gd.looktab[i] != luv) {
	ihi = i - 1;
	if (ilo <= gf3gd.hich && ihi >= gf3gd.loch && luv != 0) {
	  /* display limits */
	  gf3gd.mch[0] = ilo;
	  dspmkr(1);
	  gf3gd.mch[0] = ihi;
	  dspmkr(1);
	  /* display background */
	  initg(&nx, &ny);
	  x = (float) ilo + .5f;
	  pspot(x, gf3gd.spec[ilo]);
	  x = (float) ihi + .5f;
	  vect(x, gf3gd.spec[ihi]);
	  /* display look-up value */
	  y = (gf3gd.spec[ihi] + gf3gd.locnt) / 2;
	  pspot(x, y);
	  sprintf(cluv, "%d", luv);
	  putg(cluv, strlen(cluv), 9);
	  finig();
	}
	luv = gf3gd.looktab[i];
	ilo = i;
      }
    }
    if (luv != 0) {
      ihi = gf3gd.nclook-1;
      if (ilo <= gf3gd.hich && ihi >= gf3gd.loch) {
	/* display limits */
	gf3gd.mch[0] = ilo;
	dspmkr(1);
	gf3gd.mch[0] = ihi;
	dspmkr(1);
	/* display background */
	initg(&nx, &ny);
	x = (float) ilo + .5f;
	pspot(x, gf3gd.spec[ilo]);
	x = (float) ihi + .5f;
	vect(x, gf3gd.spec[ihi]);
	/* display look-up value */
	y = (gf3gd.spec[ihi] + gf3gd.locnt) / 2;
	pspot(x, y);
	sprintf(cluv, "%d", luv);
	putg(cluv, strlen(cluv), 9);
	finig();
      }
    }
  }
  gf3gd.mch[0] = isave;
  return 0;
} /* dispwin */

/* ======================================================================= */
int diveff(char *filnam, int nc)
{
  /* correct spectrum for detector efficiency */

  double g, f1, f2, u3, x1, x2;
  float  pars[10], x, ch, dx, dch = 0.0f, eff;
  int    ich;
  char   title[80];

  if (gf3gd.ical == 0) {
    printf("Cannot - no energy calibration.\n");
    return 1;
  }
  strncpy(filnam, "  ", 2);
  nc -= 2;
  
  /* read parameters from disk file */
  while (nc < 1 || read_eff_file(filnam, title, pars)) {
    /* ask for efficiency parameter data file */
    if (!(nc = cask("Eff. parameter data file = ?", filnam, 80))) return 0;
  }
  
  /* divide sp. by calculated efficiency */
  g = pars[6];
  for (ich = 0; ich <= gf3gd.maxch; ++ich) {
    ch = (float) ich;
    if (!energy(ch, dch, &x, &dx)) {
      if (x <= 0.f) continue;
      x1 = log(x / pars[7]);
      x2 = log(x / pars[8]);
      f1 = pars[0] + pars[1] * x1 + pars[2] * x1 * x1;
      f2 = pars[3] + pars[4] * x2 + pars[5] * x2 * x2;
      if (f1 <= 0. || f2 <= 0.) continue;
      u3 = exp(-g * log(f1)) + exp(-g * log(f2));
      if (u3 <= 0.) continue;
      eff = exp(exp(-log(u3) / g));
      gf3gd.spec[ich] /= eff;
    }
  }
  strncpy(gf3gd.namesp + 4, ".MOD", 4);
  return 0;
} /* diveff */

/* ======================================================================= */
int dofit(int npks)
{
  int  k, j1, j2, maxits;
  char ans[80];

  /* get limits etc. and/or do fit */
  if (npks > 0) {
    /* get limits, peak positions and fixed parameters */
    gf3gd.npks = npks;
    gfset();
    if (gf3gd.npks <= 0) {
      ready = 0;
      return 1;
    }
    ready = 1;
  } else if (npks < 0) {
    /* reset initial parameter estimates */
    parset(0);
  }

  while ((k = cask(" Max. no. of iterations = ? (rtn for 50)", ans, 80)) &&
	 inin(ans, k, &maxits, &j1, &j2));
  if (k == 0) maxits = 50;
  if (maxits <= 0) return 1;

  /* do fit */
  k = fitter(maxits);
  if (k == 2) printf(" WARNING - convergence failed.\n"
		     "  Do not believe quoted parameter values!\n");
  /* display fit and list parameters */
  if (k != 1) gffin();
  return 0;
} /* dofit */

/* ======================================================================= */
int dspfit(void)
{
  float y, x1, y1, b[111], save[16384];
  int   i, j, hi, lo, nx, ny, ihi, ilo;

  lo = gf3gd.mch[0];
  hi = gf3gd.mch[1];
  if (lo < gf3gd.loch) lo = gf3gd.loch;
  if (hi > gf3gd.hich) hi = gf3gd.hich;
  if (hi <= lo+1) return 0;
  /* display background */
  eval(gf3gd.pars, 0, &y, gf3gd.npks, -9);
  eval(gf3gd.pars, lo, &y, gf3gd.npks, -1);
  save[lo] = y;
  x1 = (float) lo + .5f;
  initg(&nx, &ny);
  pspot(x1, y);
  for (i = lo+1; i <= hi; ++i) {
    x1 += 1.f;
    eval(gf3gd.pars, i, &y, gf3gd.npks, -1);
    save[i] = y;
    vect(x1, y);
  }
  /* display fit */
  eval(gf3gd.pars, lo, &y, gf3gd.npks, 0);
  x1 = (float) lo + .5f;
  pspot(x1, y);
  for (i = lo+1; i <= hi; ++i) {
    x1 += 1.f;
    eval(gf3gd.pars, i, &y, gf3gd.npks, 0);
    vect(x1, y);
  }
  /* display difference */
  if (gf3gd.display_fit_diff) {
    y1 = (y + (float) gf3gd.locnt) / 2.f;
    x1 = (float) lo + .5f;
    eval(gf3gd.pars, lo, &y, gf3gd.npks, 0);
    y = gf3gd.spec[lo] - y + y1;
    pspot(x1, y);
    for (i = lo+1; i <= hi; ++i) {
      x1 += 1.f;
      eval(gf3gd.pars, i, &y, gf3gd.npks, 0);
      y = gf3gd.spec[i] - y + y1;
      vect(x1, y);
    }
  } else {
    setcolor(3);
  }
  /* display each peak separately */
  if (gf3gd.npks > 1) {
    for (i = 0; i < 6; ++i) {
      b[i] = 0.f;
    }
    b[3] = gf3gd.pars[3];
    b[4] = gf3gd.pars[4];
    for (j = 0; j < gf3gd.npks; ++j) {
      for (i = 6; i < 9; ++i) {
	b[i] = gf3gd.pars[j*3 + i];
      }
      eval(b, 0, &y, 1, -9);
      ilo = b[6] - b[7] * 3.f;
      ihi = b[6] + b[7] * 3.f;
      if (b[3] > 0.01 && ilo > b[6] - b[4] * 6.f) ilo = b[6] - b[4] * 6.f;
      if (ilo < lo) ilo = lo;
      if (ihi > hi) ihi = hi;
      eval(b, ilo, &y, 1, 0);
      x1 = (float) ilo + .5f;
      pspot(x1, y + save[ilo]);
      for (i = ilo+1; i <= ihi; ++i) {
	x1 += 1.f;
	eval(b, i, &y, 1, 0);
	vect(x1, y + save[i]);
      }
    }
  }
  finig();
  return 0;
} /* dspfit */

/* ======================================================================= */
int dspmkr(int mkrnum)
{
  static char mchar[35] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  float x, y;
  int   i, ix, iy, ich;

  if (!gf3gd.disp) return 1;

  initg(&ix, &iy);
  i = mkrnum - 1;
  if (mkrnum == 99) i = 0;

  while (i < mkrnum && i < (gf3gd.npks+2)) {
    if (i < 2) {
      ich = gf3gd.mch[i];
      x = (float) ich + .5f;
    } else {
      x = gf3gd.ppos[i-2] + .5f;
      ich = x;
    }
    if (ich >= gf3gd.loch && ich <= gf3gd.hich) {
      if (i < 2) {
	y = (float) gf3gd.locnt;
	pspot(x, y);
	vect(x, gf3gd.spec[ich]);
      } else {
	y = gf3gd.spec[ich];
	cvxy(&x, &y, &ix, &iy, 1);
	mspot(ix, iy - 30);
	ivect(ix, iy - 10);
	ivect(ix-3, iy - 19);
	ivect(ix+3, iy - 19);
	ivect(ix, iy - 10);
	mspot(ix, iy - 45);
	putg(&mchar[i-2], 1, 5);
      }
    }
    i++;
  }
  finig();
  return 0;
} /* dspmkr */

/* ======================================================================= */
int dspsp(int in1, int in2, int in3)
{
  static float hicnt = 0.f;
  float fncc, x, y, x0, y0, dx, dy;
  int   i1, icol, i, nx0, nx=0, ny=0, ncc, ich, loy;
  char  heading[28];
  int ktras(int ix, int iy, int mode);

  loy = 0;
  initg(&nx, &ny);
  nx /= gf3gd.dxnum;
  nx0 = nx * gf3gd.dxlo;
  if (gf3gd.pkfind) ny -= 10;
  if (in1 > 0 && in2 > 0) {
    if (in1 > in2) in1 = in2;
    ny /= in2;
    loy = ny * (in1 - 1);
    icol = in1 + 1;
  } else {
    icol = 2;
  }
  icol = colormap[(icol-1) % 15];
  limg(nx, nx0, ny, loy);
  gf3gd.loch = gf3gd.lox;
  gf3gd.nchs = gf3gd.numx;
  if ((in1 == 1 && in2 == 0) || in3 == 1 || gf3gd.numx == 0) {
    gf3gd.loch = 0;
    gf3gd.nchs = gf3gd.maxch + 1;
  }
  gf3gd.hich = gf3gd.loch + gf3gd.nchs - 1;
  if (gf3gd.hich < gf3gd.loch  + 8) gf3gd.hich = gf3gd.loch + 8;
  if (gf3gd.hich > gf3gd.maxch) gf3gd.hich = gf3gd.maxch;
  if (gf3gd.loch >= gf3gd.hich - 1) {
    finig();
    return 1;
  }
  gf3gd.nchs = gf3gd.hich - gf3gd.loch + 1;
  ncc = 1;
  if (hicnt == 0.f || ! ((in1 == -1 && in2 == 0) || in3 == -1)) {
    hicnt = (float) (gf3gd.locnt + gf3gd.ncnts);
    if (gf3gd.ncnts <= 0) {
      for (i = gf3gd.loch; i <= gf3gd.hich; ++i) {
	if (hicnt < gf3gd.spec[i]) hicnt = gf3gd.spec[i];
      }
    }
  }
  x0 = (float) gf3gd.loch;
  dx = (float) gf3gd.nchs;
  y0 = (float) gf3gd.locnt;
  dy = hicnt - (float) (gf3gd.locnt - 1);
  if (gf3gd.pkfind) dy *= 1.075f;
  setcolor(1);
  if (gf3gd2.y_axis_marks < 0) {
    // no axis marks to be drawn; tell minig to draw no axes
    if (gf3gd.iyaxis == 3 || gf3gd.iyaxis == -3)
      trax(dx, x0, dy, y0, 4);
    else 
      trax(dx, x0, dy, y0, 0);
    // draw bottom and right axes/boundaries
    ktras(nx0, 20+loy, 0);
    ktras(nx0+nx-5, 20+loy, 1);
    ktras(nx0+nx-5, 20+loy+ny-5, 1);
  } else {
    trax(dx, x0, dy, y0, gf3gd.iyaxis);
  }
  setcolor(icol);
  x = (float) gf3gd.loch;
  if (ncc == 1) {
    pspot(x, gf3gd.spec[gf3gd.loch]);
    for (i = gf3gd.loch; i <= gf3gd.hich; ++i) {
      vect(x, gf3gd.spec[i]);
      x += 1.f;
      vect(x, gf3gd.spec[i]);
    }
  } else {
    y = 0.f;
    for (i = 0; i < ncc; ++i) {
      y += gf3gd.spec[gf3gd.loch + i];
    }
    fncc = (float) ncc;
    y /= fncc;
    pspot(x, y);
    i1 = gf3gd.loch + (gf3gd.nchs / ncc - 1) * ncc;
    for (ich = gf3gd.loch; ncc < 0 ? ich >= i1 : ich <= i1; ich +=ncc) {
      y = 0.f;
      for (i = 0; i < ncc; ++i) {
	y += gf3gd.spec[ich + i];
      }
      y /= fncc;
      vect(x, y);
      x += fncc;
      vect(x, y);
    }
  }
  setcolor(1);
  if (gf3gd2.y_axis_marks > 0) {
    datetime(heading);
    strcpy(heading + 18, "  ");
    strncpy(heading + 20, gf3gd.namesp, 8);
    mspot(nx0+nx-90, loy + ny - 10);
    if (gf3gd.display_fit_diff) putg(heading, 28, 8);
  } else if (gf3gd2.y_axis_marks > -2) {
    mspot(nx0+nx-90, loy + ny - 10);
    if (gf3gd.display_fit_diff) putg(gf3gd.namesp, 8, 8);
  }
  finig();
  gf3gd.disp = 1;
  if (gf3gd.pkfind) findpks(ny);
  return 0;
} /* dspsp */

/* ======================================================================= */
int dspsp2(int in1, int in2, int in3, int in4)
{
  static float hicnt = 0.f;
  float x, x0, y0, dx, dy;
  int   i, lowx, nx0, nx=0, ny=0, loy;
  int ktras(int ix, int iy, int mode);

  loy = 0;
  lowx = 0;
  initg(&nx, &ny);
  nx /= gf3gd.dxnum;
  nx0 = nx * gf3gd.dxlo;
  if (gf3gd.pkfind) ny -= 10;
  if (in1 > 0 && in2 > 0) {
    if (in1 > in2) in1 = in2;
    ny /= in2;
    loy = ny * (in1 - 1);
  }
  if (in3 > 0 && in4 > 0) {
    if (in3 > in4) in3 = in4;
    nx /= in4;
    lowx = nx0 + nx * (in3 - 1);
  }
  limg(nx-5, lowx, ny-5, loy);
  gf3gd.loch = gf3gd.lox;
  gf3gd.nchs = gf3gd.numx;
  if ((in1 == 1 && in2 == 0) || gf3gd.numx == 0) {
    gf3gd.loch = 0;
    gf3gd.nchs = gf3gd.maxch + 1;
  }
  gf3gd.hich = gf3gd.loch + gf3gd.nchs - 1;
  if (gf3gd.hich > gf3gd.maxch) gf3gd.hich = gf3gd.maxch;
  gf3gd.nchs = gf3gd.hich - gf3gd.loch + 1;
  hicnt = (float) (gf3gd.locnt + gf3gd.ncnts);
  if (gf3gd.ncnts <= 0) {
    for (i = gf3gd.loch; i <= gf3gd.hich; ++i) {
      if (hicnt < gf3gd.spec[i]) hicnt = gf3gd.spec[i];
    }
  }
  x0 = (float) gf3gd.loch;
  dx = (float) gf3gd.nchs;
  y0 = (float) gf3gd.locnt;
  dy = hicnt - (float) (gf3gd.locnt - 1);
  if (gf3gd.pkfind) dy *= 1.075f;
  setcolor(1);
  if (gf3gd2.y_axis_marks <= 0) {
    // no axis marks to be drawn; tell minig to draw no axes
    if (gf3gd.iyaxis == 3 || gf3gd.iyaxis == -3)
      trax(dx, x0, dy, y0, 4);
    else 
      trax(dx, x0, dy, y0, 0);
    // draw bottom and right axes/boundaries
    ktras(lowx, 20+loy, 0);
    ktras(lowx+nx-5, 20+loy, 1);
    ktras(lowx+nx-5, 20+loy+ny-5, 1);
    // draw top and left boundaries
    ktras(lowx, 20+loy+ny-5, 1);
    ktras(lowx, 20+loy, 1);
  } else {
    trax(dx, x0, dy, y0, gf3gd.iyaxis);
    // draw top and left boundaries
    ktras(lowx+nx-5, 20+loy+ny-5, 0);
    ktras(lowx, 20+loy+ny-5, 1);
    ktras(lowx, 20+loy, 1);
  }
  setcolor(colormap[1]);
  x = (float) gf3gd.loch;
  pspot(x, gf3gd.spec[gf3gd.loch]);
  for (i = gf3gd.loch; i <= gf3gd.hich; ++i) {
    vect(x, gf3gd.spec[i]);
    x += 1.f;
    vect(x, gf3gd.spec[i]);
  }
  setcolor(1);
  mspot(nx0+lowx+10, loy + ny);
  putg(gf3gd.namesp, 8, 3);
  finig();
  if (gf3gd.pkfind) findpks(ny);
  return 0;
} /* dspsp2 */

/* ====================================================================== */
int dump(char *filnam, int nc, int mode)
{
  int  i, icsave, filerr = 0, rlen, rl = 0, newtype;
  char dattim[20], q[120], buf[19712];
  static char head1[20] = "GELIFT ver. 6.1 dump";
  static char head2[20] = "gf3 version 1.0 dump";

  strncpy(filnam, "  ", 2);
  while (1) {
    if (nc < 3 || filerr) {
      /* ask for dump file name */
      nc = cask("Dump file name = ? (default .ext = .dmp)", filnam, 80);
      if (nc == 0) return 0;
    }
    filerr = 1;
    setext(filnam, ".dmp", 80);
    /* mode = 0/1 to write/read dump */
    if (mode == 0) goto WRITEDUMP;

    /* read dump from disk */
    if (!(file1 = open_readonly(filnam))) continue;

#define RR { rlen = get_file_rec(file1, buf, sizeof(buf), -1); rl = 0; }
#define R(a,b,c) { memcpy(b, buf + rl, c*a); rl += c*a; }
    RR;
    if (rlen < 38 ||
	rlen > 42 ||
	(strncmp(buf, head1, 20) && strncmp(buf, head2, 20))) {
      file_error("read", filnam);
      fclose(file1);
      continue;
    }
    break;
  }
  newtype = strncmp(buf, head1, 20);
  sprintf(q, "Dump was done at %.18s...\n Proceed to read dump? (Y/N)",
	  buf+20);
  if (!caskyn(q)) {
    fclose(file1);
    return 0;
  }

  icsave = gf3gd.ical;
  RR;
  if (rlen <= 0) {
    ready = 0;
    file_error("read", filnam);
    fclose(file1);
    return 1;
  }
  if (newtype) {
    R(2, gf3gd.mch, 4);
    R(35, gf3gd.ppos, 4);
    R(1, &gf3gd.irelw, 4);
    R(111, gf3gd.pars, 4);
    R(111, gf3gd.freepars, 4);
    R(1, &gf3gd.npars, 4);
    R(1, &gf3gd.nfp, 4);
    R(111, gf3gd.errs, 4);
    R(1, &gf3gd.npks, 4);
    R(1, &gf3gd.irelpos, 4);
    R(35, gf3gd.areas, 4);
    R(35, gf3gd.dareas, 4);
    R(35, gf3gd.cents, 4);
    R(35, gf3gd.dcents, 4)
    R(1, &ready, 4);
    R(1, &gf3gd.wtmode, 4);
    R(5, gf3gd.finest, 4);
    R(3, gf3gd.infix, 4);
    R(6, gf3gd.gain, 8);
    R(1, &gf3gd.ical, 4);
    R(1, &gf3gd.nterms, 4);
    R(1, &gf3gd.lox, 4);
    R(1, &gf3gd.numx, 4);
    R(1, &gf3gd.locnt, 4);
    R(1, &gf3gd.ncnts, 4);
    R(1, &gf3gd.iyaxis, 4);
    R(3, gf3gd.swpars, 4);
    R(1, &gf3gd.infixrw, 4);
    R(1, &gf3gd.infixw, 4);
    R(15, colormap, 4);
    R(1, &gf3gd.pkfind, 4);
    R(1, &gf3gd.ifwhm, 4);
    R(1, &gf3gd.isigma, 4);
    R(1, &gf3gd.ipercent, 4);
    if (caskyn("Take stored areas and centroids from dump? (Y/N)")) {
      R(700, gf3gd.stoc[0], 4);
      R(700, gf3gd.stodc[0], 4);
      R(700, gf3gd.stoa[0], 4);
      R(700, gf3gd.stoda[0], 4);
      R(700, gf3gd.stoe[0], 4);
      R(700, gf3gd.stode[0], 4);
      R(20, gf3gd.isto, 4);
      R(20, gf3gd.namsto, 28);
    }

  } else {
    R(2, gf3gd.mch, 4);
    R(15, gf3gd.ppos, 4);
    R(1, &gf3gd.irelw, 4);
    R(51, gf3gd.pars, 4);
    R(51, gf3gd.freepars, 4);
    R(1, &gf3gd.npars, 4);
    R(1, &gf3gd.nfp, 4);
    R(51, gf3gd.errs, 4);
    R(1, &gf3gd.npks, 4);
    R(1, &gf3gd.irelpos, 4);
    R(15, gf3gd.areas, 4);
    R(15, gf3gd.dareas, 4);
    R(15, gf3gd.cents, 4);
    R(1, &ready, 4);
    R(1, &gf3gd.wtmode, 4);
    R(5, gf3gd.finest, 4);
    R(3, gf3gd.infix, 4);
    R(6, gf3gd.gain, 8);
    R(1, &gf3gd.ical, 4);
    R(1, &gf3gd.nterms, 4);
    R(1, &gf3gd.lox, 4);
    R(1, &gf3gd.numx, 4);
    R(1, &gf3gd.locnt, 4);
    R(1, &gf3gd.ncnts, 4);
    R(1, &gf3gd.iyaxis, 4);
    R(3, gf3gd.swpars, 4);
    R(1, &gf3gd.infixrw, 4);
    R(1, &gf3gd.infixw, 4);
    R(15, colormap, 4);
    R(1, &gf3gd.pkfind, 4);
    R(1, &gf3gd.ifwhm, 4);
    R(1, &gf3gd.isigma, 4);
    R(1, &gf3gd.ipercent, 4);
    if (caskyn("Take stored areas and centroids from dump? (Y/N)")) {
      R(300, gf3gd.stoc[0], 4);
      R(300, gf3gd.stodc[0], 4);
      R(300, gf3gd.stoa[0], 4);
      R(300, gf3gd.stoda[0], 4);
      R(300, gf3gd.stoe[0], 4);
      R(300, gf3gd.stode[0], 4);
      R(20, gf3gd.isto, 4);
      R(20, gf3gd.namsto, 28);
    }
    RR;
    if (rlen > 0) {
      R(15, gf3gd.dcents, 4);       /* centroid uncertainties added sept 2005 */
    } else {
      for (i=0; i<15; i++) {
	gf3gd.dcents[i] = gf3gd.errs[i*3 + 6];
      }
    }
  }
#undef RR
#undef R
  fclose(file1);
  if (icsave > 1) {
    if (gf3gd.ical <= 0) {
      gf3gd.ical = icsave;
      nc = 0;
      encal(filnam, nc);
    }
    gf3gd.ical = icsave;
  } else {
    if (gf3gd.ical > 1) gf3gd.ical = 1;
  }
  if (gf3gd.wtmode > 0) weight(3);
  return 0;

  /* write dump to disk */
 WRITEDUMP:
  if (!(file1 = open_new_file(filnam, 0))) {
    printf("...No dump written!\n");
    return 1;
  }
  datetime(dattim);
  strncpy(dattim+18, "  ", 2);
#define W(a,b,c) { memcpy(buf + rl, b, c*a); rl += c*a; }
  W(1, head2, 20);
  W(1, dattim, 20);
  put_file_rec(file1, buf, rl);
  rl = 0;
  W(2, gf3gd.mch, 4);
  W(35, gf3gd.ppos, 4);
  W(1, &gf3gd.irelw, 4);
  W(111, gf3gd.pars, 4);
  W(111, gf3gd.freepars, 4);
  W(1, &gf3gd.npars, 4);
  W(1, &gf3gd.nfp, 4);
  W(111, gf3gd.errs, 4);
  W(1, &gf3gd.npks, 4);
  W(1, &gf3gd.irelpos, 4);
  W(35, gf3gd.areas, 4);
  W(35, gf3gd.dareas, 4);
  W(35, gf3gd.cents, 4);
  W(35, gf3gd.dcents, 4);
  W(1, &ready, 4);
  W(1, &gf3gd.wtmode, 4);
  W(5, gf3gd.finest, 4);
  W(3, gf3gd.infix, 4);
  W(6, gf3gd.gain, 8);
  W(1, &gf3gd.ical, 4);
  W(1, &gf3gd.nterms, 4);
  W(1, &gf3gd.lox, 4);
  W(1, &gf3gd.numx, 4);
  W(1, &gf3gd.locnt, 4);
  W(1, &gf3gd.ncnts, 4);
  W(1, &gf3gd.iyaxis, 4);
  W(3, gf3gd.swpars, 4);
  W(1, &gf3gd.infixrw, 4);
  W(1, &gf3gd.infixw, 4);
  W(15, colormap, 4);
  W(1, &gf3gd.pkfind, 4);
  W(1, &gf3gd.ifwhm, 4);
  W(1, &gf3gd.isigma, 4);
  W(1, &gf3gd.ipercent, 4);
  W(700, gf3gd.stoc[0], 4);
  W(700, gf3gd.stodc[0], 4);
  W(700, gf3gd.stoa[0], 4);
  W(700, gf3gd.stoda[0], 4);
  W(700, gf3gd.stoe[0], 4);
  W(700, gf3gd.stode[0], 4);
  W(20, gf3gd.isto, 4);
  W(20, gf3gd.namsto, 28);
  if (put_file_rec(file1, buf, rl)) {
    file_error("write to", filnam);
    printf("...No dump written!\n");
    fclose(file1);
    return 1;
  }
#undef W
  fclose(file1);
  return 0;
} /* dump */

/* ======================================================================= */
int encal(char *fn, int nc)
{
  /* define/change/delete energy calibration */

  float g[3];
  int   i, iorder;
  char  title[80], q[512];

  strncpy(fn, "  ", 2);
  if (nc > 2 && !read_cal_file(fn, title, &iorder, gf3gd.gain)) {
    gf3gd.nterms = iorder + 1;
    gf3gd.ical = 1;
    return 0;
  }

  while (1) {
    if (gf3gd.ical <= 1) {
      gf3gd.ical = 0;
      if (!caskyn("Do you want an energy calibration? (Y/N)")) return 0;
      gf3gd.ical = 1;
    }

    sprintf(q," The energy calibration (in keV) is"
	    "  E = A + B*X +C*X*X  etc. (X = ch. no.)\n"
	    "   Current values are : %.3f %.5f", gf3gd.gain[0], gf3gd.gain[1]);
    for (i = 2; i < gf3gd.nterms; ++i) {
      sprintf(q+strlen(q), " %.2e", gf3gd.gain[i]);
    }
    strcat(q,"\n New values = ? (rtn for old values, filename for ENCAL file)");
    nc = cask(q, fn, 80);
    if (nc == 0) return 0;
    if (ffin(fn, nc, &g[0], &g[1], &g[2])) {
      if (!read_cal_file(fn, title, &iorder, gf3gd.gain)) {
	gf3gd.nterms = iorder + 1;
	gf3gd.ical = 1;
	return 0;
      }
    } else if (g[1] <= 0.f) {
      printf("B must be greater than zero.\n");
    } else {
      if (g[2] == 0.f) {
	gf3gd.nterms = 2;
      } else {
	gf3gd.nterms = 3;
      }
      gf3gd.gain[0] = g[0];
      gf3gd.gain[1] = g[1];
      gf3gd.gain[2] = g[2];
      return 0;
    }
  }
} /* encal */

/* ======================================================================= */
int energy(float x, float dx, float *eg, float *deg)
{
  int jj;

  if (gf3gd.ical == 0) return 1;

  *deg = 0.f;
  *eg = gf3gd.gain[gf3gd.nterms - 1];
  for (jj = gf3gd.nterms - 1; jj >= 1; --jj) {
    *deg = (float) jj * gf3gd.gain[jj] + *deg * x;
    *eg = gf3gd.gain[jj-1] + *eg * x;
  }
  *deg *= dx;
  return 0;
} /* energy */

/* ======================================================================= */
int eval(float *pars, int ichan, float *fit, int npks, int mode)
{
  /* calculate the fit using present values of the pars */

  float a, h, r, r1, r2, u, u1, u2, u3, u5, u6, u7, u8, w, x, x1, z; 
  static float y[35], y1[35], y2[35], beta, width;
  static int   i, notail, ix0;

  if (mode == -9) {
    /* mode = -9 ; initialise, i.e calculate  IX0,NOTAIL,Y,Y1,Y2 */
    ix0 = (gf3gd.mch[0] + gf3gd.mch[1]) / 2;
    notail = 1;
    if (gf3gd.freepars[3] == 0 && pars[3] == 0.f) return 0;
    notail = 0;
    for (i = 0; i < npks; ++i) {
      y[i] = pars[i*3 + 7] / (pars[4] * 3.33021838f);
      if (y[i] > 4.f) {
	y1[i] = 0.f;
	notail = 1;
      } else {
	y1[i] = erfc(y[i]);
	y2[i] = exp(-y[i] * y[i]) * 1.12837917f / y1[i];
      }
    }
    return 0;
  }
  x = (float) (ichan - ix0);
  *fit = pars[0] + pars[1]*x + pars[2]*x*x;
  if (mode >= 1) {
    derivs[1] = x;
    derivs[2] = x*x;
    derivs[3] = 0.f;
    derivs[4] = 0.f;
    derivs[5] = 0.f;
  }
  x1 = (float) (ichan);
  for (i = 0; i < npks; ++i) {
    x = x1 - pars[i*3 + 6];
    width  = pars[i*3 + 7] / 2.35482f;
    h      = pars[i*3 + 8];
    w = x / (width*1.41421356f);
    if (fabs(w) > 4.f) {
      u1 = 0.f;
      u3 = 0.f;
      if (x < 0.f) u3 = 2.f;
    } else {
      u1 = exp(-w*w);
      u3 = erfc(w);
    }
    if (mode == -1) {
      /* mode = -1; calculate background only */
      *fit += h * pars[5] * u3 / 200.f;
      continue;
    }
    if (notail) {
      /* notail = true; pure gaussians only */
      u = u1 + pars[5] * u3 / 200.f;
      *fit += h * u;
      /* calculate derivs only for mode.ge.1 */
      if (mode >= 1) {
	derivs[5] += h * u3 / 200.f;
	a = u1 * (w + pars[5] / 354.49077f) * 2.f;
	derivs[i*3 + 6] = h * a / (width * 1.41421356f);
	derivs[i*3 + 7] = h * w * a * 1.41421356f / (width * 2.35482f);
	derivs[i*3 + 8] = u;
      }
      continue;
    }
    r = pars[3] / 100.f;
    r1 = 1.f - r;
    beta = pars[4];
    z = w + y[i];
    if ((r2 = x / beta, fabs(r2)) > 12.f) {
      u5 = 0.f;
      u6 = 0.f;
      u7 = 0.f;
    } else {
      u7 = exp(x / beta) / y1[i];
      if (fabs(z) > 4.f) {
	u5 = 0.f;
	if (z < 0.f) u5 = 2.f;
	u6 = 0.f;
      } else {
	u5 = erfc(z);
	u6 = exp(-z * z) * 1.12837917f;
      }
    }
    u2 = u7 * u5;
    u = r1 * u1 + r * u2 + pars[5] * u3 / 200.f;
    *fit += h * u;
    /* calculate derivs only for mode.ge.1 */
    if (mode >= 1) {
      u8 = u5 * y2[i];
      derivs[3] += h * (u2 - u1) / 100.f;
      derivs[4] += r * h * u7 * (y[i] * (u6 - u8) - u5 * x / beta) / beta;
      derivs[5] += h * u3 / 200.f;
      a = u1 * (r1 * w + pars[5] / 354.49077f) * 2.f;
      derivs[i*3 + 6] = h * (a + r*u7*(u6 - u5*2.f*y[i])) / (width*1.41421356f);
      derivs[i*3 + 7] = h * (w*a + r*u7*(u6 * (w - y[i]) + u8*y[i])) *
                        1.41421356f / (width*2.35482f);
      derivs[i*3 + 8] = u;
    }
  }
  return 0;
} /* eval */

/* ======================================================================= */
int find_bg(int loch, int hich, int disp_flag,
	    float *x1, float *y1, float *x2, float *y2)
{
  int   nx, ny;

  find_bg2(loch, hich, x1, y1, x2, y2);
  if (*y1 <= 1.f) {
    *y1 = 1.5f;
  } else {
    *y1 += sqrt(*y1) * .7f;
  }
  if (*y2 <= 1.f) {
    *y2 = 1.5f;
  } else {
    *y2 += sqrt(*y2) * .7f;
  }

  /* interpolate/extrapolate for background. */
  if (disp_flag > 0 && gf3gd.disp) {
    initg(&nx, &ny);
    pspot((float) loch, 0.f);
    vect((float) loch, gf3gd.spec[loch]);
    pspot((float) (hich + 1), 0.f);
    vect((float) (hich + 1), gf3gd.spec[hich]);
    pspot(*x1 + .5f, 0.f);
    vect(*x1 + .5f, *y1);
    vect(*x2 + .5f, *y2);
    vect(*x2 + .5f, 0.f);
    finig();
  }
  return 0;
} /* find_bg */

/* ======================================================================= */
int find_bg2(int loch, int hich, float *x1, float *y1, float *x2, float *y2)
{
  float a, y;
  int   i, mid;

  mid = (loch + hich) / 2;

  /* find channel with least counts on each side of middle */
  *y1 = (gf3gd.spec[loch] + gf3gd.spec[loch + 1] + gf3gd.spec[loch + 2]) / 3.f;
  *x1 = (float) (loch + 1);
  for (i = loch + 2; i <= mid - 2; ++i) {
    a = (gf3gd.spec[i-1] + gf3gd.spec[i] + gf3gd.spec[i+1]) / 3.f;
    if (*y1 > a) {
      *y1 = a;
      *x1 = (float) i;
    }
  }
  *y2 = (gf3gd.spec[mid + 2] + gf3gd.spec[mid + 3] + gf3gd.spec[mid + 4]) / 3.f;
  *x2 = (float) (mid + 3);
  for (i = mid + 4; i <= hich - 1; ++i) {
    a = (gf3gd.spec[i-1] + gf3gd.spec[i] + gf3gd.spec[i+1]) / 3.f;
    if (*y2 > a) {
      *y2 = a;
      *x2 = (float) i;
    }
  }
  /* check that there are no channels between loch and hich
     that are below the background. */
  if (*y2 > *y1) {
    for (i = rint(*x1) + 1; i <= mid - 2; ++i) {
      y = *y1 - (*x1 - (float) i) * (*y1 - *y2) / (*x1 - *x2);
      a = (gf3gd.spec[i-1] + gf3gd.spec[i] + gf3gd.spec[i+1]) / 3.f;
      if (y > a) {
	*y1 = a;
	*x1 = (float) i;
      }
    }
    for (i = rint(*x2) + 1; i <= hich - 1; ++i) {
      y = *y1 - (*x1 - (float) i) * (*y1 - *y2) / (*x1 - *x2);
      a = (gf3gd.spec[i-1] + gf3gd.spec[i] + gf3gd.spec[i+1]) / 3.f;
      if (y > a) {
	*y2 = a;
	*x2 = (float) i;
      }
    }
  } else {
    for (i = rint(*x1) - 1; i >= loch + 1; --i) {
      y = *y1 - (*x1 - (float) i) * (*y1 - *y2) / (*x1 - *x2);
      a = (gf3gd.spec[i-1] + gf3gd.spec[i] + gf3gd.spec[i+1]) / 3.f;
      if (y > a) {
	*y1 = a;
	*x1 = (float) i;
      }
    }
    for (i = rint(*x2) - 1; i >= mid + 3; --i) {
      y = *y1 - (*x1 - (float) i) * (*y1 - *y2) / (*x1 - *x2);
      a = (gf3gd.spec[i-1] + gf3gd.spec[i] + gf3gd.spec[i+1]) / 3.f;
      if (y > a) {
	*y2 = a;
	*x2 = (float) i;
      }
    }
  }
  if (*y1 < 1.f) *y1 = 1.f;
  if (*y2 < 1.f) *y2 = 1.f;
  return 0;
} /* find_bg2 */

/* ======================================================================= */
int find_ge_cent(int loch, int hich, int disp_flag,
		 float *area, float *cent, float *fwhm, float *darea, float *dcent)
{
  static float factor = 1.5f;
  float x, y, x1, y1, x2, y2, sx2y, ymax, ymid, dcsum, sigma;
  float dc, bg, oldcen, xm, flo, fhi, xhi, yhi, xlo, ylo, sxy, r1;
  int   maxw2, i, iteration, nx, ny, hi, lo;

  /* integrate peak over Cent +/- Factor*FWHM */
  lo = loch;
  hi = hich;
  maxw2 = (hi - lo) * (hi - lo) / 10;
  *area = 0.f;
  *cent = 0.f;
  *fwhm = 0.f;
  find_bg(lo, hi, disp_flag, &x1, &y1, &x2, &y2);

  /* integrate peak over Cent +/- Factor*FWHM */
  oldcen = (float) (lo + hi) / 2.f;
  r1 = oldcen - (hich - loch) / 7.f;
  lo = rint(r1);
  r1 = oldcen + (hich - loch) / 7.f;
  hi = rint(r1);
  if (hi - lo < 2) {
    r1 = oldcen - 3;
    lo = rint(r1);
    hi = lo + 6;
  }

  flo = 1.f;
  fhi = 1.f;
  for (iteration = 1; iteration <= 20; ++iteration) {
    xm = oldcen;
    xlo = (float) lo + (1.f - flo) * .5f - xm;
    xhi = (float) hi - (1.f - fhi) * .5f - xm;
    bg = y1 - (x1 - (float) lo) * (y1 - y2) / (x1 - x2);
    ylo = (gf3gd.spec[lo] - bg) * flo;
    dc = flo * flo * (gf3gd.spec[lo] + bg / 25.f);
    *darea = dc;

    dcsum = dc * (xlo * xlo);
    bg = y1 - (x1 - (float) hi) * (y1 - y2) / (x1 - x2);
    yhi = (gf3gd.spec[hi] - bg) * fhi;
    dc = fhi * fhi * (gf3gd.spec[hi] + bg / 25.f);
    *darea += dc;

    dcsum += dc * (xhi * xhi);
    *area = ylo + yhi;
    sxy = ylo * xlo + yhi * xhi;
    sx2y = ylo * xlo * xlo + yhi * xhi * xhi;

    for (i = lo + 1; i <= hi-1; ++i) {
      x = (float) i - xm;
      bg = y1 - (x1 - (float) i) * (y1 - y2) / (x1 - x2);
      y = gf3gd.spec[i] - bg;
      *area += y;
      dc = gf3gd.spec[i] + bg / 25.f;
      *darea += dc;
      dcsum += dc * (x * x);
      sxy += y * x;
      sx2y += y * x * x;
    }

    if (*area < 10.f) {
      *area = 0.f;
      *darea = 0.f;
      *cent = 0.f;
      *fwhm = 0.f;
      return 0;
    }
    *cent = sxy / *area;
    sigma = sx2y / *area - *cent * *cent;
    *cent += xm;
    if (sigma <= 0.f || sigma > (float) maxw2) {
      *area = 0.f;
      *darea = 0.f;
      *cent = 0.f;
      *fwhm = 0.f;
      return 0;
    }

    sigma = sqrt(sigma);
    *fwhm = sigma * 2.355f;
    *dcent = sqrt(dcsum) / *area;
    *darea = sqrt(*darea);
    if ((r1 = *cent - oldcen, fabs(r1)) < *dcent / 20.f ||
	iteration == 20) {
      ymax = gf3gd.spec[(int) (*cent)];
      ymid = (gf3gd.spec[lo] + gf3gd.spec[hi] + 2.0f*ymax)/4.0f;
      if (disp_flag > 0 && gf3gd.disp) {
	initg(&nx, &ny);
	pspot((float) lo, 0.f);
	vect((float) lo, ymid);
	pspot((float) (hi+1), 0.f);
	vect((float) (hi+1), ymid);
	pspot(*cent + .5f, 0.f);
	vect(*cent + .5f, ymax);
	finig();
      }
      return 0;
    }

    oldcen = *cent;
    lo = (int) (*cent - factor * *fwhm);
    hi = (int) (*cent + factor * *fwhm);
    if (lo < 1 || hi > gf3gd.maxch) {
      *area = 0.f;
      *darea = 0.f;
      *cent = 0.f;
      *fwhm = 0.f;
      return 0;
    }
    flo = factor * *fwhm - *cent + (float) (lo + 1);
    fhi = factor * *fwhm + *cent - (float) hi;
  }
  return 0;
} /* find_ge_cent */

/* ======================================================================= */
int findpks(int npixy)
{
  float pmax, x, y, chanx[100], xchan, eg, deg, dx = 0.0f, rsigma, ref, psize[100];
  int   maxpk, ix, iy, nx, ny, ipk, npk, ich;
  char  mchar[8];

  /* do peak search */
  rsigma = (float) gf3gd.isigma;
  maxpk = (float) (gf3gd.hich - gf3gd.loch + 1) * 2.355f / (float) gf3gd.ifwhm;
  if (maxpk > 100) maxpk = 100;
  pfind(chanx, psize, gf3gd.loch+1, gf3gd.hich+1, gf3gd.ifwhm, rsigma, maxpk, &npk);
  pmax = -100.f;
  for (ipk = 1; ipk <= npk; ++ipk) {
    if (psize[ipk-1] > pmax) pmax = psize[ipk-1];
  }
  ref = (float) gf3gd.ipercent * pmax / 100.f;

  /* display markers and energies at peak positions */
  initg(&nx, &ny);
  for (ipk = 1; ipk <= npk; ++ipk) {
    if (psize[ipk-1] > ref) {
      xchan = chanx[ipk-1] - 1.f;
      eg = xchan;
      energy(xchan, dx, &eg, &deg);
      ich = xchan + .5f;
      y = gf3gd.spec[ich];
      x = xchan + .5f;
      sprintf(mchar, "%6.1f", eg);
      cvxy(&x, &y, &ix, &iy, 1);
      mspot(ix, iy + 3 + npixy/60);
      ivect(ix, iy + 6 + npixy/20);
      mspot(ix, iy + 6 + npixy/18);
      putg(mchar, 6, 5);
    }
  }
  finig();
  return 0;
} /* findpks */

/* ======================================================================= */
int fitter(int maxits)
{
  /* this subroutine is a modified version of 'CURFIT', in Bevington
     see page 237 */

  double alpha[111][111], array[111][111], ddat, beta[111], delta[111], ers[111], flamda;
  float  b[111], fixed[111], diff, chisq=0.0f, chisq1, dat, fit, r1;
  int    i, j, k, l, m, conv = 0, nits, test, nipos, nextp[111];
  int    ndf, ihi, ilo, niw, rpfixed, rwfixed, mip=0, nip=0, miw=0, nip1=0;
  char   dattim[20];
  static char wtc[3][16] = {"fit.","data.","sp.         "};

  ilo = gf3gd.mch[0];
  ihi = gf3gd.mch[1];
  nip = gf3gd.npars - gf3gd.nfp;
  /* nip   = no. of independent (non-fixed) pars
     nfp   = no. of fixed pars
     npars = total no. of pars = 3 * no.of peaks + 6
     ndf   = no. of degrees of freedom */
  if (gf3gd.wtmode > 0) strncpy(&wtc[2][4], gf3gd.nwtsp, 8);
  for (i = 6; i < gf3gd.npars; ++i) {
    fixed[i] = (float) gf3gd.freepars[i];
  }
  /* set up fixed relative widths */
  rwfixed = 0;
  if (gf3gd.irelw <= 0) {
    niw = 0;
    for (j = 7; j < gf3gd.npars; j += 3) {
      if (gf3gd.freepars[j] == 1) miw = j;
      /* miw = highest fitted (non-fixed) width par. no. */
      niw += gf3gd.freepars[j];
    }
    /* niw = no. of fitted (non-fixed) widths */
    if (niw > 1) {
      for (j = 7; j <= miw; j += 3) {
	gf3gd.freepars[j] = 0;
      }
      rwfixed = 1;
      nip = nip - niw + 1;
      nip1 = nip;
    }
  }
  /* set up fixed relative positions */
  rpfixed = 0;
  if (gf3gd.irelpos <= 0) {
    nipos = 0;
    for (j = 6; j < gf3gd.npars; j += 3) {
      if (gf3gd.freepars[j] == 1) mip = j;
      /* mip = highest fitted (non-fixed) position par. no. */
      nipos += gf3gd.freepars[j];
    }
    /* nipos = no. of fitted (non-fixed) positions */
    if (nipos > 1) {
      for (j = 6; j <= mip; j += 3) {
	gf3gd.freepars[j] = 0;
      }
      rpfixed = 1;
      nip = nip - nipos + 1;
      nip1 = nip - 1;
    }
  }
  ndf = ihi - ilo + 1 - nip;
  if (ndf < 1) {
    printf("No d.o.f.\n");
    goto QUIT;
  }
  if (nip < 2) {
    printf("Too many fixed pars.\n");
    goto QUIT;
  }
  /* set up array nextp, pointing to free pars */
  k = 0;
  for (j = 0; j < gf3gd.npars; ++j) {
    if (gf3gd.freepars[j] != 0) nextp[k++] = j;
  }
  if (rwfixed) nextp[k++] = miw;
  if (rpfixed) nextp[k++] = mip;
  if (k != nip) {
    printf("nip != sum(freepars)\n");
    goto QUIT;
  }
  /* initialise for fitting */
  flamda = 0.001;
  nits = 0;
  test = 0;
  derivs[0] = 1.f;
  for (i = 0; i < gf3gd.npars; ++i) {
    gf3gd.errs[i] = 0.f;
    b[i] = gf3gd.pars[i];
  }
  gotsignal = 0;
  signal(SIGINT, breakhandler);
  /* printf("You can press control-C to interrupt this fit...\n"); */

  /* evaluate fit, alpha & beta matrices, & chisq */
 NEXT_ITERATION:
  for (j = 0; j < nip; ++j) {
    beta[j] = 0.f;
    for (k = 0; k <= j; ++k) {
      alpha[k][j] = 0.f;
    }
  }
  chisq1 = 0.f;
  eval(gf3gd.pars, 0, &fit, gf3gd.npks, -9);
  for (i = ilo; i <= ihi; ++i) {
    eval(gf3gd.pars, i, &fit, gf3gd.npks, 1);
    diff = gf3gd.spec[i] - fit;
    /* weight with fit/data/weight sp. for wtmode=-1/0/1 */
    if (gf3gd.wtmode < 0) {
      dat = fit;
    } else if (gf3gd.wtmode == 0) {
      dat = gf3gd.spec[i];
    } else {
      dat = gf3gd.wtsp[i];
    }
    if (dat < 1.f) dat = 1.f;
    ddat = (double) dat;
    chisq1 += diff * diff / dat;
    if (rwfixed) {
      for (k = 7; k < miw; k += 3) {
	derivs[miw] += fixed[k] * derivs[k];
      }
    }
    if (rpfixed) {
      for (k = 6; k < mip; k += 3) {
	derivs[mip] += fixed[k] * derivs[k];
      }
    }
    for (l = 0; l < nip; ++l) {
      j = nextp[l];
      beta[l] += diff * derivs[j] / dat;
      for (m = 0; m <= l; ++m) {
	alpha[m][l] += (double)derivs[j] * (double)derivs[nextp[m]] / ddat;
      }
    }
  }
  chisq1 /= (float) ndf;
  /* invert modified curvature matrix to find new parameters */
 INVERT_MATRIX:
  for (j = 0; j < nip; ++j) {
    if (alpha[j][j] * alpha[j][j] == 0.) {
      printf("Cannot - diagonal element no. %d equal to zero.\n", j);
      goto QUIT;
    }
  }
  array[0][0] = flamda + 1.0;
  for (j = 1; j < nip; ++j) {
    for (k = 0; k < j; ++k) {
      array[k][j] = alpha[k][j] / sqrt(alpha[j][j] * alpha[k][k]);
      array[j][k] = array[k][j];
    }
    array[j][j] = flamda + 1.0;
  }
  matinv(array[0], nip, 111);
  if (!test) {
    for (j = 0; j < nip; ++j) {
      delta[j] = 0.f;
      for (k = 0; k < nip; ++k) {
	delta[j] += beta[k] * array[k][j] / sqrt(alpha[j][j] * alpha[k][k]);
      }
    }
    /* calculate new par. values */
    for (l = 0; l < nip; ++l) {
      j = nextp[l];
      b[j] = gf3gd.pars[j] + delta[l];
    }
    if (rwfixed) {
      for (j = 7; j <= miw - 3; j += 3) {
	b[j] = gf3gd.pars[j] + fixed[j] * delta[nip1-1];
      }
    }
    if (rpfixed) {
      for (j = 6; j <= mip - 3; j += 3) {
	b[j] = gf3gd.pars[j] + fixed[j] * delta[nip-1];
      }
    }
    for (j = 7; j <= gf3gd.npars; j += 3) {
      if (b[j] < 0.0f) b[j] = fabs(b[j]);
    }
    /* if chisq increased, increase flamda and try again */
    chisq = 0.f;
    eval(b, gf3gd.freepars[3], &fit, gf3gd.npks, -9);
    for (i = ilo; i <= ihi; ++i) {
      eval(b, i, &fit, gf3gd.npks, 0);
      diff = gf3gd.spec[i] - fit;
      /* weight with fit/data/weight sp. for wtmode=-1/0/1 */
      if (gf3gd.wtmode < 0) {
	dat = fit;
      } else if (gf3gd.wtmode == 0) {
	dat = gf3gd.spec[i];
      } else {
	dat = gf3gd.wtsp[i];
      }
      if (dat < 1.f) dat = 1.f;
      chisq += diff * diff / dat;
    }
    chisq /= (float) ndf;
    if (chisq > chisq1 && flamda < 2.0) {
      flamda *= 10.0;
      goto INVERT_MATRIX;
    }
  }
  /* evaluate parameters and errors
     test for convergence */
  conv = 1;
  for (j = 0; j < nip; ++j) {
    if (array[j][j] < 0.) array[j][j] = 0.;
    ers[j] = sqrt(array[j][j] / alpha[j][j]) * sqrt(flamda + 1.0);
    if ((r1 = delta[j], fabs(r1)) >= ers[j] / 100.f) conv = 0;
  }

  /* evaluate covariances between parameters */
  for (l = 0; l < gf3gd.npars; ++l) {
    for (k = 0; k < l; ++k) {
      gf3gd.covariances[k][l] = gf3gd.covariances[l][k] = 0.f;
    }
  }
  for (l = 0; l < nip; ++l) {
    j = nextp[l];
    for (k = 0; k < l; ++k) {
      gf3gd.covariances[j][nextp[k]] = gf3gd.covariances[nextp[k]][j] =
	array[k][l] / sqrt(alpha[k][k] * alpha[l][l]);
    }
  }

  if (!test && !gotsignal) {
    for (j = 0; j < gf3gd.npars; ++j) {
      gf3gd.pars[j] = b[j];
    }
    flamda /= 10.0;
    ++nits;
    if (!conv && nits < maxits) goto NEXT_ITERATION;
    /* re-do matrix inversion with FLAMDA=0
       to calculate errors */
    flamda = 0.0;
    test = 1;
    goto INVERT_MATRIX;
  }

  if (gotsignal) {
    printf("**** Fit Interrupted (Control-C) ****\n"
	   "**** WARNING: Parameter values and errors may be undefined!\n");
    if (prfile)
      fprintf(prfile, "**** Fit Interrupted (Control-C) ****\n"
	      "**** WARNING: Parameter values and errors may be undefined!\n");
  }
  signal(SIGINT, SIG_DFL);
  /* list data and exit */
  for (l = 0; l < nip; ++l) {
    gf3gd.errs[nextp[l]] = ers[l];
  }
  /* deal with errs and covariances in case where relative widths are fixed */
  if (rwfixed) {
    for (j = 7; j <= miw; j += 3) {
      gf3gd.freepars[j] = fixed[j];
      gf3gd.errs[j] = fixed[j] * ers[nip1 - 1];
      for (k = 0; k < nip; ++k) {
	gf3gd.covariances[nextp[k]][j] = gf3gd.covariances[j][nextp[k]] =
	  fixed[j] * array[k][nip1 - 1] / sqrt(alpha[k][k] * alpha[nip1 - 1][nip1 - 1]);
      }
    }
  }
  /* deal with errs and covariances in case where relative positions are fixed */
  if (rpfixed) {
    for (j = 6; j <= mip; j += 3) {
      gf3gd.freepars[j] = fixed[j];
      gf3gd.errs[j] = fixed[j] * ers[nip - 1];
      for (k = 0; k < nip; ++k) {
	gf3gd.covariances[nextp[k]][j] = gf3gd.covariances[j][nextp[k]] =
	  fixed[j] * array[k][nip - 1] / sqrt(alpha[k][k] * alpha[nip - 1][nip - 1]);
      }
    }
  }
  /* deal with special case where both relative widths and positions are fixed */
  if (rwfixed && rpfixed) {
    for (j = 6; j <= mip; j += 3) {
      gf3gd.covariances[j+1][j] = gf3gd.covariances[j][j+1] =
	fixed[j] * fixed[j+1] * array[nip - 1][nip1 - 1] /
	sqrt(alpha[nip - 1][nip - 1] * alpha[nip1 - 1][nip1 - 1]);
    }
  }

  datetime(dattim);
  printf(" File %s  Spectrum %.8s  %.18s\n"
	 " Fitted chs %d to %d, %d peaks\n"
	 " %d indept. pars,  %d degrees of freedom,  weighted with %s\n",
	 gf3gd.filnam, gf3gd.namesp, dattim, gf3gd.mch[0], gf3gd.mch[1],
	 gf3gd.npks, nip, ndf, wtc[gf3gd.wtmode+1]);
  if (rpfixed) printf("Relative peak positions fixed.\n");
  if (rwfixed) printf("Relative widths fixed.\n");

  if (conv) {
    printf(" %d iterations,  Chisq/d.o.f.= %.3f\n", nits, chisq);
    if (prfile) {
      fprintf(prfile, "\n"
	 " File %s  Spectrum %.8s  %.18s\n"
	 " Fitted chs %d to %d, %d peaks\n"
	 " %d indept. pars,  %d degrees of freedom,  weighted with %s\n",
	 gf3gd.filnam, gf3gd.namesp, dattim, gf3gd.mch[0], gf3gd.mch[1],
	 gf3gd.npks, nip, ndf, wtc[gf3gd.wtmode+1]);
      if (rpfixed) fprintf(prfile, "Relative peak positions fixed.\n");
      if (rwfixed) fprintf(prfile, "Relative widths fixed.\n");
      fprintf(prfile," %d iterations,  Chisq/d.o.f.= %.3f\n", nits, chisq);
    }
    return 0;
  }
  printf("Failed to converge after %d iterations,  Chisq/d.o.f.= %.3f\n"
	 "  WARNING - do not believe quoted parameter values!\n", nits, chisq);
  return 2;

 QUIT:
  if (rwfixed || rpfixed) {
    for (i = 6; i < gf3gd.npars; ++i) {
      gf3gd.freepars[i] = fixed[i];
    }
  }
  return 1;
} /* fitter */

/* ======================================================================= */
int fix_para(int param, int fix_flag)
{
  /* fixes or frees, depending on the value of fix_flag,
     a parameter to an inputed value */
  /* called by fixorfree */

  float rj1, rj2, val;
  int   nc;
  char  ans[80];

  if (fix_flag) {
    /* fix parameter */
    if (param <= 0) {
      /* no input or negative number so */
      return 0;
    } else if (param == 1001) {
      gf3gd.irelpos = 0;
      printf("Relative peak positions fixed.\n");
    } else if (param == 1002) {
      gf3gd.irelw = 0;
      printf("Relative widths fixed.\n");
    } else if (param > gf3gd.npars) {
      printf("Parameter number too large, try again.\n");
    } else if (gf3gd.nfp + gf3gd.freepars[param-1] == gf3gd.npars - 1) {
      printf("  Cannot - too many fixed pars.\n");
    } else {
      gf3gd.nfp += gf3gd.freepars[param-1];
      gf3gd.freepars[param-1] = 0;
      while (1) {
	nc = cask("Value = ? (rtn for present value)", ans, 80);
	if (nc > 0) {
	  if (ffin(ans, nc, &val, &rj1, &rj2)) continue;
	  if (val == 0.f && (param + 1) / 3 * 3 == param + 1 && param != 2) {
	    printf("Value must be nonzero.\n");
	    continue;
	  }
	} else {
	  val = gf3gd.pars[param-1];
	}
	if (param != 4 || val != 0.f) {
	  gf3gd.pars[param-1] = val;
	} else if (gf3gd.nfp + gf3gd.freepars[4] == gf3gd.npars - 1) {
	  printf("  Cannot - too many fixed pars.\n");
	  continue;
	} else {
	  gf3gd.nfp += gf3gd.freepars[4];
	  gf3gd.freepars[4] = 0;
	  printf(" >>> Beta fixed at %.3f\n", gf3gd.pars[4]);
	  gf3gd.pars[param-1] = val;
	}
	break;
      }
    }
  } else {
    /* free parameter */
    if (param > 0) {
      if (param == 1001) {
	gf3gd.irelpos = 1;
	printf("Relative peak positions free to vary.\n");
      } else if (param == 1002) {
	gf3gd.irelw = 1;
	printf("Relative widths free to vary.\n");
      } else if (param > gf3gd.npars) {
	printf("Parameter number too large, try again.\n");
      } else {
	gf3gd.nfp = gf3gd.freepars[param-1] + gf3gd.nfp - 1;
	gf3gd.freepars[param-1] = 1;
      }
    }
  }
  return 0;
} /* fix_para */

/* ======================================================================= */
int fixorfree(char *command, int nc)
{
  /* Fixs or frees the parameters, the relative widths
     and/or relative peak positions.
     command = FT  FX  FR */

  int i, lo, fix, param, j1, j2;
  char fixtag[113][4], ans[80];
  static char parc[113][4] = {
    " A "," B "," C "," R ","BTA","STP","P1 ","W1 ","H1 ","P2 ","W2 ","H2 ",
    "P3 ","W3 ","H3 ","P4 ","W4 ","H4 ","P5 ","W5 ","H5 ","P6 ","W6 ","H6 ",
    "P7 ","W7 ","H7 ","P8 ","W8 ","H8 ","P9 ","W9 ","H9 ","PA ","WA ","HA ",
    "PB ","WB ","HB ","PC ","WC ","HC ","PD ","WD ","HD ","PE ","WE ","HE ",
    "PF ","WF ","HF ","PG ","WG ","HG ","PH ","WH ","HH ","PI ","WI ","HI ",
    "PJ ","WJ ","HJ ","PK ","WK ","HK ","PL ","WL ","HL ","PM ","WM ","HM ",
    "PN ","WN ","HN ","PO ","WO ","HO ","PP ","WP ","HP ","PQ ","WQ ","HQ ",
    "PR ","WR ","HR ","PS ","WS ","HS ","PT ","WT ","HT ","PU ","WU ","HU ",
    "PV ","WV ","HV ","PW ","WW ","HW ","PX ","WX ","HX ","PY ","WY ","HY ",
    "PZ ","WZ ","HZ ","RW ","RP "};

  if (!strncmp(command, "FT", 2)) {
    /* asking for fixed parameter(s) to set up a fit */
    fix = 1;
  } else if (!strncmp(command, "FX", 2)) {
    fix = 1;
  } else if (!strncmp(command, "FR", 2)) {
    fix = 0;
  } else {
    return 1;
  }

  if (nc > 2) {
    /* parameter specified in command line */
    if (!inin(command+2, nc-2, &param, &j1, &j2) ||
	!para2num(command+2, &param)) {
      fix_para(param, fix);
      return 0;
    } else {
      printf("Error, parameter unknown.\n");
    }
  }

  /* no parameter specified in command line
     list name and status of the parameters */
  for (i = 0; i < gf3gd.npars; ++i) {
    if (gf3gd.freepars[i] == 0) {
      strcpy(fixtag[i], "  *");
    } else {
      sprintf(fixtag[i], "%3d", i+1);
      fixtag[i][3] = '\0';
    }
  }
  strcpy(fixtag[111], "   ");
  if (!gf3gd.irelw) strcpy(fixtag[111], "  *");
  strcpy(fixtag[112], "   ");
  if (!gf3gd.irelpos) strcpy(fixtag[112], "  *");

  lo = 0;
  while (gf3gd.npars > lo + 24) {
    for (i = lo; i < lo + 24; ++i) {
      printf(" %s", fixtag[i]);
    }
    printf("\n ");
    for (i = lo; i < lo + 24; ++i) {
      printf(" %s", parc[i]);
    }
    printf("\n");
    lo += 24;
  }
  for (i = lo; i < gf3gd.npars; ++i) {
    printf(" %s", fixtag[i]);
  }
  printf(" %s %s", fixtag[111], fixtag[112]);
  printf("\n ");
  for (i = lo; i < gf3gd.npars; ++i) {
    printf(" %s", parc[i]);
  }
  printf(" %s %s", parc[111], parc[112]);

  if (fix) {
    printf("\nParameters to fix =? (one per line, rtn to end)\n");
  } else {
    printf("\nParameters to free =? (one per line, rtn to end)\n");
  }
  while ((nc = cask(">", ans, 80))) {
    if (!inin(ans, nc, &param, &j1, &j2) || !para2num(ans, &param)) {
      fix_para(param, fix);
    } else {
      printf("Parameter unknown, try again.\n");
    }
  }
  return 0;
} /* fixorfree */

/* ======================================================================= */
float getmkrchnl(int mn)
{
  /* gets a channel that is within the acceptable fitting range
     input: mn = the marker number
     returns the channel number */

  float x, y, j1, j2;
  int   i, k;
  char  ans[80];

  while (1) {
    if (gf3gd.disp &&
	gf3gd.hich >= gf3gd.mch[1] &&
	gf3gd.loch <= gf3gd.mch[0]) {
      retic(&x, &y, ans);
      x -= 0.5f;
    } else {
      ans[0] = 'T';
    }
    if (ans[0] == 'T' || ans[0] == 't') {
      while ((k = cask("New position = ? (rtn for old value)\n", ans, 80)) &&
	     ffin(ans, k, &x, &j1, &j2));
      if (k == 0) {
	if (mn > 2) return gf3gd.ppos[mn-3];
	return ((float) gf3gd.mch[mn-1]);
      }
    }
    if (x < 0 || x > gf3gd.maxch) {
      printf(" Marker ch. outside spectrum - try again.\n");
      continue;
    }
    j1 = gf3gd.ppos[0];
    j2 = gf3gd.ppos[0];
    for (i = 1; i < gf3gd.npks; ++i) {
      if (gf3gd.ppos[i] < j1) j1 = gf3gd.ppos[i]; /* lowest peak */
      if (gf3gd.ppos[i] > j2) j2 = gf3gd.ppos[i]; /* highest peak */
    }
    if ((mn > 2 && (x < gf3gd.mch[0] || x > gf3gd.mch[1])) ||
	(mn == 1 && x > j1) || (mn == 2 && x < j2)) {
      printf(" Peaks must be within limits - try again.\n");
      continue;
    }
    return x;
  }
} /* getmkrchnl */

/* ======================================================================= */
int getsp(char *filnam, int nc)
{
  int numch;

  /* ask for spectrum file name and read spectrum from disk */
  strncpy(filnam, "  ", 2);
  if (nc <= 2 &&
      !cask("Spectrum file or ID = ?", filnam, 80)) return 1;
  while (readsp(filnam, gf3gd.spec, gf3gd.namesp, &numch, 16384)) {
    if (!cask("Spectrum file or ID = ?", filnam, 80)) return 1;
  }
  printf(" Sp. %.8s   %d chs read.\n", gf3gd.namesp, numch);
  strncpy(gf3gd.filnam, filnam, 80);
  gf3gd.disp = 0;
  gf3gd.maxch = numch - 1;
  return 0;
} /* getsp */

/* ======================================================================= */
int gfexec(char *ans, int nc)
{
  /* this subroutine decodes and executes the commands */

  float sens, x=0, y=0, x1=0, x2=0, e1, e2, save[16384];
  float fj1, fj2, oldch1, oldch2, newch1, newch2;
  int   mode, imap, nspx, nspy, i, j, k, numch, j1, j2;
  int   in, lo, in2, hi, nnx, nny, numspec, idata = 0;
  char  q[256], cmd[80];
  static int isplo = 1, isphi = 2, ispd=1, ssnum = 0, maxspec = 0, last_snum = -1;
  static int auto_disp_max = 0, auto_disp_num = 0;
  static int ss_disabled[100] = {0};

#ifdef NOGRAPHICS
  gf3gd.disp = 0;
#endif

  /* convert lower case to upper case characters */
  if (ans[0] != '!') {
    ans[0] = toupper(ans[0]);
    ans[1] = toupper(ans[1]);
  }
  idata = 0;
  if (!strncmp(ans, "AC", 2) || !strncmp(ans, "AS", 2) || 
      !strncmp(ans, "MS", 2) || !strncmp(ans, "DV", 2)) {
    /* add spectrum / add counts / multiply/divide spectrum */
    addspec(ans, nc);
    gf3gd.disp = 0;
  } else if (!strncmp(ans, "AF", 2)) {
    /* autofit spectrum */
    if (gf3gd.disp) {
      lo = hi = 0;
      if (ffin(ans + 2, nc-2, &sens, &fj1, &fj2)) goto BADCMD;
      lo = fj1 + 0.5f;
      hi = fj2 + 0.5f;
      autofit(&sens, lo, hi);
    } else {
      printf("Bad command: Spectrum not yet displayed...\n");
    }
  } else if (!strncmp(ans, "AG", 2)) {
    /* adjust gain of spectrum */
    numch = gf3gd.maxch + 1;
    for (i = 0; i < numch; ++i) {
      save[i] = gf3gd.spec[i];
    }
    mode = 0;
    if (ans[2] == '2') {
      while (1) {
	if (!(k = cask("Energy1 Energy2 Newch1 Newch2 = ?", ans, 80))) return 0;
	for (i = 1; i < k; ++i) {
	  if (ans[i] == ' ') ans[i] = ',';
	}
	if (sscanf(ans, "%f,%f,%f,%f", &e1, &e2, &newch1, &newch2) == 4) break;
      }
      oldch1 = channo(e1);
      oldch2 = channo(e2);
      mode = -1;
    }
    adjgain(mode, &oldch1, &oldch2, &newch1, &newch2, save, gf3gd.spec,
	    numch, numch, 1.f);
    strncpy(gf3gd.namesp + 4, ".MOD", 4);
    gf3gd.disp = 0;
  } else if (!strncmp(ans, "AP", 2)) {
    /* AP; add peak to fit */
    mode = 1;
    adddelpk(mode, idata);
  } else if (!strncmp(ans, "BG", 2)) {
    autobkgnd(&j);
    maxspec = j;
  } else if (!strncmp(ans, "CA", 2)) {
    /* autocalibrate spectrum */
    if (!strncmp(ans, "CA3", 3)) {
      autocal3();
    } else if (!strncmp(ans, "CA4", 3)) {
      autocal4();
    } else {
      autocal();
    }
  } else if (!strncmp(ans, "CD", 2)) {
    /* change working directory */
    for (i=2; i<nc && ans[i] == ' '; i++) ;
    chdir(ans + i);
  } else if (!strncmp(ans, "CF", 2)) {
    /* open command file for input on lu IR */
    comfil(ans, nc);
  } else if (!strncmp(ans, "CO", 2)) {
    /* change color map */
    if (nc < 3) {
      printf("Present color map =");
      for (i = 1; i < 15; ++i) {
	printf(" %d", colormap[i]-1);
      }
      printf("\n");
      nc = cask("     New color map = ?", ans, 80);
    } else {
      strncpy(ans, "  ", 2);
    }
    lo = 0;
    for (imap = 1; imap < 15; ++imap) {
      while (ans[lo] == ' ') lo++;
      if (lo >= nc) return 0;
      if (ans[lo] == ',') {  /* comma only; no value entered */
	lo++;
	continue;
      }
      /* find upper delimiter (blank or comma) */
      hi = lo+1;
      while (ans[hi] != ' ' && ans[hi] != ',' && hi < nc) hi++;
      /* decode value and set colormap */
      if (inin(ans + lo, hi-lo, &idata, &j1, &j2)) goto BADCMD;
      colormap[imap] = (idata % 15) + 1;
      lo = hi + 1;
    }
  } else if (!strncmp(ans, "CR", 2)) {
    /* call or dislay cursor */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (!gf3gd.disp) {
      printf("Bad command: New spectrum not yet displayed...\n");
    } else if (idata != 0 && (idata < gf3gd.loch || idata > gf3gd.hich)) {
      printf("Bad command: Channel outside displayed region...\n");
    } else {
      curse(&idata);
    }
  } else if (!strncmp(ans, "CT", 2)) {
    /* contract spectrum by factor idata */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    contract(&idata);
    if (idata > 1) gf3gd.disp = 0;
  } else if (!strncmp(ans, "DD", 2)) {
    /* DD ; toggle display of difference (data-fit) when plotting fits */
    gf3gd.display_fit_diff = 1 - gf3gd.display_fit_diff;
    printf("display_fit_diff = %d\n", gf3gd.display_fit_diff);
  } else if (!strncmp(ans, "DE", 2)) {
    /* DE ; divide the spectrum by the detector efficiency */
    diveff(ans, nc);
    gf3gd.disp = 0;
  } else if (!strncmp(ans, "DF", 2)) {
    /* display fit */
    if (!ready) {
      printf("Bad command: No fit defined...\n");
    } else if (!gf3gd.disp) {
      printf("Bad command: New spectrum not yet displayed...\n");
    } else {
      dspfit();
    }
  } else if (!strncmp(ans, "DL", 2)) {
    /* modify X0,NX */
    if (inin(ans + 2, nc - 2, &j1, &j2, &j)) goto BADCMD;
    if (j1 + 2 > gf3gd.maxch ||
	j1 < 0 ||
	j2 < j1) goto BADCMD;
    gf3gd.lox = j1;
    gf3gd.numx = j2 - j1 + 1;
    if (j1 == 0 && j2 == 0) gf3gd.numx = 0;
  } else if (!strncmp(ans, "DM", 2)) {
    /* display markers */
    if (!ready) {
      printf("Bad command: No fit defined...\n");
    } else if (!gf3gd.disp) {
      printf("Bad command: New spectrum not yet displayed...\n");
    } else {
      dspmkr(99);
    }
  } else if (!strncmp(ans, "DP", 2)) {
    /* DP; delete peak from fit */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    mode = 2;
    adddelpk(mode, idata);
  } else if (!strncmp(ans, "DS", 2) || !strncmp(ans, "OV", 2)) {
    /* display or overlay-display spectrum */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (!strncmp(ans, "DS", 2)) {
      /* clear graphics screen and display spectrum */
      erase();
      rewind(scrfilea);
      maxspec = 0;
    }
    if (dspsp(idata, in, in2)) goto BADCMD;
    /* write display parameters and spectrum to scratch file
       for redrawing later */
    if (maxspec < 20) {
#define W(a,b,c) fwrite(b,c,a,scrfilea)
      W(15, colormap, 4);
      W(1, &gf3gd.locnt, 4);
      W(1, &gf3gd.ncnts, 4);
      W(1, &gf3gd.iyaxis, 4);
      W(1, &idata, 4);
      W(1, &in, 4);
      W(1, gf3gd.namesp, 8);
      W(1, &gf3gd.maxch, 4);
      W(gf3gd.maxch+1, gf3gd.spec, 4);
#undef W
      ++maxspec;
    }
  } else if (!strncmp(ans, "DU", 2)) {
    /* dump parameters,markers,areas,wtmode etc */
    dump(ans, nc, 0);
  } else if (!strncmp(ans, "DW", 2)) {
    /* DW ; display windows as they are presently defined */
    dispwin();
  } else if (!strncmp(ans, "DX", 2)) {
    /* DX ; divide display window into ranges in x-direction */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (in < 1 || idata > in) {
      gf3gd.dxlo  = 0;
      gf3gd.dxnum = 1;
    } else {
      if (idata < 0) idata = 0;
      gf3gd.dxlo  = idata-1;
      gf3gd.dxnum = in;
    }
  } else if (!strncmp(ans, "EC", 2)) {
    /* define/change/delete energy calibration */
    encal(ans, nc);
  } else if (!strncmp(ans, "EX", 2)) {
    /* expand spectrum display using cursor */
    if (gf3gd.disp) {
      retic(&x1, &y, ans);
      retic(&x2, &y, ans);
      gf3gd.numx = fabs(x2 - x1);
      gf3gd.lox = x1;
      if (x1 > x2) gf3gd.lox = x2;
      goto SHRDSP;
    }
    printf("Bad command: New spectrum not yet displayed...\n");
  } else if (!strncmp(ans, "FC", 2)) {
    /* extract actual FWHM of a peak w/o fit or bkgnd subtraction */
    if (gf3gd.disp) {
      retic(&x, &y, ans);
      i = x;
      y = gf3gd.spec[i]/2.0;
      for (j=i; gf3gd.spec[j] > y; j--) ;
      for (; gf3gd.spec[i] > y; i++) ;
      x1 = i -(y - gf3gd.spec[j]) / (gf3gd.spec[j+1] - gf3gd.spec[j]);
      x2 = j + (y - gf3gd.spec[i]) / (gf3gd.spec[i-1] - gf3gd.spec[i]);
      printf("  fwhm = %.2f\n", x1 - x2);
      initg(&i, &j);
      pspot(x1, 0); vect(x1, y); vect(x2, y); vect(x2, 0);
      finig();
    } else {
      printf("Bad command: New spectrum not yet displayed...\n");
    }
  } else if (!strncmp(ans, "FR", 2) || !strncmp(ans, "FX", 2)) {
    /* fix or free parameters */
    (void) fixorfree(ans, nc);
  } else if (!strncmp(ans, "FT", 2)) {
    /* get limits etc. and/or do fit */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (idata <= 0 && (!ready || gf3gd.mch[1] > gf3gd.maxch)) goto BADCMD;
    if (idata > 35) goto BADCMD;
    dofit(idata);
  } else if (!strncmp(ans, "FW", 2)) {
    /* report actual FWHM of fit with tail etc */
    float y, y1, y2, xlo, xhi, fw, b[111], eg, deg;
    char  l[120];
    if (gf3gd.ical) {
      printf("Peak  channel       centroid      energy        FWHM[chs]  FWHM[keV]       FWTM[chs]  FWTM[keV]\n");
    } else {
      printf("Peak  channel       centroid      FWHM[chs]      FWTM[chs]\n");
    }
    /* copy selected parameters */
    for (i = 0; i < 6; ++i) b[i] = 0.f;
    b[3] = gf3gd.pars[3];
    b[4] = gf3gd.pars[4]*20.0f;   // increase beta by factor 20
    for (j = 0; j < gf3gd.npks; ++j) {
      for (i = 6; i < 9; ++i) b[i] = gf3gd.pars[j*3 + i];
      b[7] *= 20.0f;   // increase width of peak by factor 20
      eval(b, 0, &y, 1, -9);  // initialize eval()
      /* find max height of peak */
      i = b[6]+20;
      eval(b, i, &y, 1, 0);
      while (1) {
        eval(b, --i, &y1, 1, 0);
        if (y > y1) break;
        y = y1;
      }
      /* find position of lower half-max-height */
      while (1) {
        eval(b, --i, &y2, 1, 0);
        if (y/2.0f > y2) break;
        y1 = y2;
      }
      xlo = (float) i + (y/2.0f - y2)/(y1-y2);
      /* find position of upper half-max-height */
      i = b[6] + 1;
      eval(b, i, &y1, 1, 0);
      while (1) {
        eval(b, ++i, &y2, 1, 0);
        if (y/2.0f > y2) break;
        y1 = y2;
      }
      xhi = (float) i - (y/2.0f - y2)/(y1-y2);
      fw = (xhi - xlo)/20.0f;   // correct for increase by factor 20
      /* report results */
      energy(gf3gd.cents[j], fw, &eg, &deg);
      //printf("Peak %2d ch = %f (%f)  FWHM = %f (%f)\n",
      //       j+1, gf3gd.cents[j], eg, fw, deg);
      *l= '\0';
      k = j*3 + 6;
      wrresult(l, gf3gd.pars[k], gf3gd.errs[k], 14);
      wrresult(l, gf3gd.cents[j], gf3gd.dcents[j], 28);
      if (!energy(gf3gd.cents[j], gf3gd.dcents[j], &eg, &deg)) {
        wrresult(l, eg, deg, 42);
      }
      wrresult(l, fw, gf3gd.errs[k+1], 53);
      if (!energy(gf3gd.cents[j], fw, &eg, &deg)) {
        wrresult(l, deg, deg*gf3gd.errs[k+1]/gf3gd.pars[k+1], 69);
      }
      printf("%4d %s", j+1, l);
#ifdef NOGRAPHICS
      printf("#sp %8.8s %s;  fwhm %s", gf3gd.namesp, gf3gd.namesp+8, l+55);
#endif

      /* find position of lower tenth-max-height */
      while (1) {
        eval(b, --i, &y2, 1, 0);
        if (y/10.0f > y2) break;
        y1 = y2;
      }
      xlo = (float) i + (y/10.0f - y2)/(y1-y2);
      /* find position of upper tenth-max-height */
      i = b[6] + 1;
      eval(b, i, &y1, 1, 0);
      while (1) {
        eval(b, ++i, &y2, 1, 0);
        if (y/10.0f > y2) break;
        y1 = y2;
      }
      xhi = (float) i - (y/10.0f - y2)/(y1-y2);
      fw = (xhi - xlo)/20.0f;   // correct for increase by factor 20
      /* report results */
      energy(gf3gd.cents[j], fw, &eg, &deg);
      *l= '\0';
      wrresult(l, fw, gf3gd.errs[k+1], 11);
      if (!energy(gf3gd.cents[j], fw, &eg, &deg)) {
        wrresult(l, deg, deg*gf3gd.errs[k+1]/gf3gd.pars[k+1], 0);
      }
      printf("%s\n", l);
#ifdef NOGRAPHICS
      printf(" fwtm %s\n", l);
#endif
    }

  } else if (!strncmp(ans, "HC", 2)) {
    /* HC; generate hardcopy of graphics screen */
    hcopy();
  } else if (!strncmp(ans, "HE", 2)) {
    gfhelp(ans);
#ifdef HAVE_GNU_READLINE
  } else if (!strncmp(ans, "HI", 2)) {  // list or clear gnu readline history
    if (strstr(ans, "-c") || strstr(ans, "-C")) {
      clear_history();
    } else {
      for (i = 0; next_history() != NULL; i++) ;
      for (; i>1; i--) printf("%s\n", previous_history()->line);
    }
  } else if (!strncmp(ans, "!", 1)) {  // recall command from gnu readline history
    char cmd[80], next[80], *c;
    int j;
    for (i = 0; next_history() != NULL; i++) ;             // find history length
    for (j=0; j<i; j++) c = (char *) previous_history()->line;  // rewind
    for (j=0; j<i; j++) {
      strncpy(cmd, next_history()->line, 80);
      if (!strncmp(cmd, ans+1, nc-1)) break;
    }
    if (!strncmp(cmd, ans+1, nc-1)) {
      printf(" >>> %s\n", cmd); fflush(stdout);

      // parse and execute the recalled command
      // use ; to put multiple commands on one line
      while (strncmp(cmd, "SY", 2) && strncmp(cmd, "sy", 2) &&
             (c = strstr(cmd, ";")) && strlen(c) > 2) {
        strncpy(next, c+1, sizeof(next));
        *c = '\0';
        gfexec(cmd, strlen(cmd));
        for (i = 0; next[i] == ' '; i++) ; // remove leading spaces
        strncpy(cmd, next+i, 80);
      }
      gfexec(cmd, strlen(cmd));
    } else {
      printf(" <<< %s not found in history\n", ans+1);
    }
#endif
  } else if (!strncmp(ans, "IN", 2)) {
    /* indump parameters,markers,areas,wtmode etc */
    dump(ans, nc, 1);
  } else if (!strncmp(ans, "IS", 2)) {
    /* integrate spectrum */
    if (!inin(ans + 2, nc - 2, &idata, &in, &in2) && idata >= 0) {
      if (in <= idata || in > gf3gd.maxch) in = gf3gd.maxch;
      /*  could use this code to select integrated region by cursor
      } else if (gf3gd.disp) {
      retic(&x1, &y, ans);
      retic(&x2, &y, ans);
      if (x1 < x2) {
	idata = x1;
	in = x2;
      } else if (x2 < x1) {
	idata = x2;
	in = x1;
      } else {
	idata = 0;
	in = gf3gd.maxch;
      }
      */
    } else {
      idata = 0;
      in = gf3gd.maxch;
    }
    for (i=0; i<idata; i++)   gf3gd.spec[i] = 0;
    for (i=idata; i<=in; i++) gf3gd.spec[i] += gf3gd.spec[i-1];
    for (i=in+1; i<=gf3gd.maxch; i++) gf3gd.spec[i] = gf3gd.spec[i-1];
  } else if (!strncmp(ans, "JH", 2)) {
    sumcts_jch();
  } else if (!strncmp(ans, "LIN", 3) ||
	     !strncmp(ans, "LIn", 3)) {
    gf3gd.iyaxis = -1;
    printf("Linear Y-axis...\n");
  } else if (!strncmp(ans, "LOG", 3) ||
	     !strncmp(ans, "LOg", 3)) {
    gf3gd.iyaxis = -3;
    printf("Logarithmic Y-axis...\n");
  } else if (!strncmp(ans, "LP", 2)) {
    /* list pars */
    list_params(1);
  } else if (!strncmp(ans, "LU", 2)) {
    /* LU [fn] ; create look-up table file (fn = file name) */
    lookup(ans, nc);
  } else if (!strncmp(ans, "MA", 2)) {
    /* change limits or peak positions */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    chngmark(idata);
  } else if (!strncmp(ans, "MD", 2)) {
    /* shift display using cursor */
    if (gf3gd.disp) {
      printf("    Upper channel limit = ?\n");
      retic(&x, &y, ans);
      gf3gd.lox = x - gf3gd.numx;
      if (gf3gd.lox < 0) gf3gd.lox = 0;
      goto SHRDSP;
    }
    printf("Bad command: New spectrum not yet displayed...\n");
  } else if (!strncmp(ans, "ME", 2)) {
    /* ME ; go to menu mode */
    return 0;
  } else if (!strncmp(ans, "MU", 2)) {
    /* shift display using cursor */
    if (gf3gd.disp) {
      printf("    Lower channel limit = ?\n");
     retic(&x, &y, ans);
      gf3gd.lox = x;
      goto SHRDSP;
    }
    printf("Bad command: New spectrum not yet displayed...\n");
  } else if (!strncmp(ans, "NE", 2)) {
    /* NE: toggle no_erase flag for spectrum display OS, SS commands*/
    gf3gd.no_erase = 1 - gf3gd.no_erase;
  } else if (!strncmp(ans, "NF", 2)) {
    /* NF: define new fit; use X with cursor to exit */
    idata = 99;
    dofit(idata);
  } else if (!strncmp(ans, "NX", 2)) {
    /* change NX for sp. display */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    gf3gd.numx = idata;
  } else if (!strncmp(ans, "NY", 2)) {
    /* change NY or y-axis scale for sp. display */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (idata < -3) {
      if (gf3gd.iyaxis == -1) {
	gf3gd.iyaxis = -3;
	printf("Logarithmic Y-axis...\n");
      } else {
	gf3gd.iyaxis = -1;
	printf("Linear Y-axis...\n");
      }
    } else if (idata < 0) {
      gf3gd.iyaxis = idata;
    } else {
      gf3gd.ncnts = idata;
    }
  } else if (!strncmp(ans, "ON", 2) || !strncmp(ans, "SN", 2)) {
    /* no-erase overlay or stack spectra; like NE; SS; NE  */
    gf3gd.no_erase = 1;
    ans[1] = 'S';
    gfexec(ans, nc);
    gf3gd.no_erase = 0;
  } else if (!strncmp(ans, "OS", 2)) {
    in2 = 0;
    if (nc > 2) {
      if (inin(ans + 2, nc - 2, &isplo, &isphi, &in2)) goto BADCMD;
      if (in2 > 1) {
        ispd = in2;
        in2 = 0;
      } else {
        ispd = 1;
      }
    }
    if (in2 >= 0 && !gf3gd.no_erase) erase();  // OS lo hi -1 to NOT clear
    
    j = colormap[1];
    k = 0;
    for (i = isplo; i <= isphi; i += ispd) {
      colormap[1] = 2 + (k++%14);
      sprintf(ans, "SP %d", i);
      if (getsp(ans, strlen(ans))) break;
      if (dspsp(1, 1, 0)) {
        colormap[1] = j;
        goto BADCMD;
      }
    }
    last_snum = i - ispd;
    colormap[1] = j;
    gf3gd.disp = 1;
  } else if (!strncmp(ans, "PF", 2)) {
    /* PF; set up peak find on spectrum display */
    if (!strcmp(ans, "PF0") || !strcmp(ans, "PF 0")) {
      printf(" Peak find deactivated.\n");
      gf3gd.pkfind = 0;
      return 0;
    }
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    while (idata == 0) {
      if (!(gf3gd.pkfind = caskyn("Do you want peak find activated? (Y/N)")))
	return 0;
      printf(" Current values of FWHM, sigma, %% are: %d %d %d\n",
	     gf3gd.ifwhm, gf3gd.isigma, gf3gd.ipercent);
      while ((nc = cask("New values = ? (rtn for old values)", ans, 80)) &&
	     inin(ans, nc, &idata, &in, &in2));
      if (nc == 0) return 0;
    }
    gf3gd.pkfind = 1;
    gf3gd.ifwhm = idata;
    if (in != 0) gf3gd.isigma = in;
    if (in2 >= 0) gf3gd.ipercent = in2;
    printf(" Peak find activated; FWHM, SIGMA, %% = %d %d %d\n",
	   gf3gd.ifwhm, gf3gd.isigma, gf3gd.ipercent);
  } else if (!strncmp(ans, "PK", 2)) {
    /* auto peak information using cursor */
    autopeak(ans);
  } else if (!strncmp(ans, "RD", 2)) {
    /* clear and redraw graphics screen with new display limits */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    goto SHRDSP;
  } else if (!strncmp(ans, "RF", 2)) {
    /* reset free parameters */
    parset(0);
  } else if (!strncmp(ans, "RP", 2)) {
    /* fix/free relative peak positions */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    gf3gd.irelpos = idata;
    if (idata < 1) {
      gf3gd.irelpos = 0;
      printf("Relative peak positions fixed.\n");
    } else {
      gf3gd.irelpos = 1;
      printf("Relative peak positions free to vary.\n");
    }
  } else if (!strncmp(ans, "RS", 2)) {
    /* remove linear slope from spectrum */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2) ||
	idata < 0 || in <= idata || in > gf3gd.maxch) {
      if (!gf3gd.disp) {
	printf("Bad command: New spectrum not yet displayed...\n");
	goto BADCMD;
      }
      retic(&x1, &y, ans);
      retic(&x2, &y, ans);
      if (x1 < x2) {
	idata = x1;
	in = x2;
      } else if (x2 < x1) {
	idata = x2;
	in = x1;
      } else {
	goto BADCMD;
      }
    }
    y = (gf3gd.spec[in] - gf3gd.spec[idata]) / (float) (in - idata);
    x1 = gf3gd.spec[idata] - (float) idata * y;
    for (i=0; i<=gf3gd.maxch; i++) gf3gd.spec[i] -= x1 + (float) i * y;
  } else if (!strncmp(ans, "RW", 2)) {
    /* fix/free relative widths */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (idata < 1) {
      gf3gd.irelw = 0;
      printf("Relative width fixed.\n");
    } else {
      gf3gd.irelw = 1;
      printf("Relative widths free to vary.\n");
    }
  } else if (!strncmp(ans, "SA", 2)) {
    /* store areas and centroids for later analysis */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (storac(idata)) goto BADCMD;
  } else if (!strncmp(ans, "SB", 2)) {
    /* sum counts using cursor, subtracting background */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    sumcts(1, idata, in);
  } else if (!strncmp(ans, "SC", 2)) {
    if (!inin(ans + 2, nc - 2, &idata, &in, &in2) &&
        idata >= 0 && in > 0 && in >= idata && in <= gf3gd.maxch) {
      /* set counts using typed input values */
      printf("Setting chs %d through %d to %d counts\n", idata, in, in2);
      for (i=idata; i<=in; i++) gf3gd.spec[i] = in2;
    } else {
      /* set counts using cursor */
      if (!gf3gd.disp) {
        printf("Bad command: New spectrum not yet displayed...\n");
      } else {
        setcts();
      }
    }
  } else if (!strncmp(ans, "SD", 2)) {
    /* set up automatic Spectrum Display */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (auto_disp_max && !idata) {
      printf("Automatic spectrum display disabled.\n");
      auto_disp_max = 0;
    } else {
      auto_disp_max = idata;
      if (auto_disp_max < 1) auto_disp_max = 1;
      printf("Automatic spectrum display enabled, %i spectra.\n",
	     auto_disp_max);
      auto_disp_num = 1;
      sprintf(cmd, "DS %i %i", auto_disp_num, auto_disp_max);
      gfexec(cmd, strlen(cmd));
    }
  } else if (!strncmp(ans, "SF", 2)) {   /* subtract fitted peaks from spectrum */
    if (gf3gd.npks > 0) {
      eval(gf3gd.pars, 0, &y, gf3gd.npks, -9);
      for (i=gf3gd.mch[0]; i <= gf3gd.mch[1]; i++) {
	eval(gf3gd.pars, i, &y, gf3gd.npks, 0);
	gf3gd.spec[i] -= y;
      }
    }
  } else if (!strncmp(ans, "SH", 2)) {
    if (nc < 3 && !ask(ans+2, 78, "Shell command = ?")) return 0;
    comfil(ans, nc);
  } else if (!strncmp(ans, "SL", 2)) {
    /* SL [fn] ; create slice input file  (fn = file name) */
    slice(ans, nc);
  } else if (!strncmp(ans, "SP", 2) ||
	     !strncmp(ans, "S+", 2) ||
	     !strncmp(ans, "S-", 2)) {
    if (!strncmp(ans, "S+", 2)) {
      if ((j=atoi(ans+2)) < 1) j = 1;  // optional increment, default = 1
      sprintf(ans, "SP %d", last_snum + j);
      nc = strlen(ans);
    } else if (!strncmp(ans, "S-", 2)) {
      if ((j=atoi(ans+2)) < 1) j = 1;  // optional decrement, default = 1
      sprintf(ans, "SP %d", last_snum - j);
      nc = strlen(ans);
    }
    /* get new spectrum */
    if (nc > 2 && !(inin(ans + 2, nc - 2, &idata, &j1, &j2))) {
      if (idata < 0) {
	gfinit(0, 0);
      } else {
	last_snum = idata;
	if (getsp(ans, nc)) return 0;
      }
    } else {
      if (getsp(ans, nc)) return 0;
    }
    if (auto_disp_max) {
      if (++auto_disp_num > auto_disp_max) {
	auto_disp_num = 1;
	sprintf(cmd, "DS %i %i", auto_disp_num, auto_disp_max);
      } else {
	sprintf(cmd, "OV %i %i", auto_disp_num, auto_disp_max);
      }
      gfexec(cmd, strlen(cmd));
    }
    if (gf3gd.wtmode > 0) {
      if (!caskyn("Get new weighting spectrum? (Y/N)")) return 0;
      weight(3);
    }
  } else if (!strncmp(ans, "SS", 2)) {
    if (nc > 2 && (ispd=1) &&
	inin(ans + 2, nc - 2, &isplo, &isphi, &ispd)) {
      if (ans[2] == 'd' || ans[2] == 'D') {        // SSD to set a list of disabled detector channels (0-99)
        nc = cask("List of disabled channels = ?", ans, 80);
        if (nc < 1) return 0;
        FILE *fp;
        if (!(fp = fopen(ans, "r"))) {
          printf("Error: file %s does not exist?", ans);
          return 0;
        }
        for (i=0; i<100; i++) ss_disabled[i] = 0;
        i = 0;
        while(fgets(cmd, 80, fp)) {
          if (strlen(cmd) > 0 && cmd[0] != '#' &&
              (j = atoi(cmd)) > 0 && j < 100 && !ss_disabled[j]) {
            ss_disabled[j] = 1;
            i++;
          }
        }
        fclose(fp);
        printf("%d detector channels disabled\n", i);
        return 0;
      } else {
        goto BADCMD;
      }
    }
    if (ispd >= 0 && !gf3gd.no_erase) erase();    // SS lo hi -1 to NOT clear
    if (ispd < 1) ispd = 1;
    ssnum = 1 + (isphi - isplo) / ispd;
    for (i = isplo; i <= isphi; i += ispd) {
      if (ss_disabled[i%100]) ssnum--;
    }
    if (ssnum < 2) goto BADCMD;
    nspx = (int) sqrt((float) ssnum);
    nspy = (ssnum - 1) / nspx + 1;
    nnx = 1;
    nny = 1;
    for (i = isplo; i <= isphi; i += ispd) {
      if (ss_disabled[i%100]) continue;
      sprintf(ans, "SP %d", i);
      if (getsp(ans, strlen(ans))) break;
      dspsp2(nny, nspy, nnx, nspx);
      if (++nny > nspy) {
	++nnx;
	nny = 1;
      }
    }
    last_snum = i - ispd;
    gf3gd.disp = 0;
  } else if (!strncmp(ans, "ST", 2)) {
#ifdef HAVE_GNU_READLINE
#ifndef NOGRAPHICS
    // printf("writing readline history...\n");
    strcpy (ans, (char *) getenv("HOME"));
    strcat (ans, "/.gf3_history");
    (void) write_history(ans);
#endif
#endif
    /* ST ; stop and exit */
    if (!caskyn("Are you sure you want to exit? (Y/N)")) return 0;
    /* close .sto file */
    storac(-1);
    /* close .win or .tab file */
    if (winmod == 1) wrtlook();
    if (winmod != 0) fclose(winfile);
    /* clear graphics screen */
    erase();
    save_xwg("gf2_std_");
    if (prfile) {
      fclose(prfile);
      while (1) {
	nc = cask("Type P/D/S to Print/Delete/Save log file ?", ans, 1);
	if (nc == 0 || *ans == 'D' || *ans == 'd') {
#ifdef VMS
	  sprintf(q, "delete/noconfirm/nolog %s;0", prfilnam);
#else
	  sprintf(q, "rm -f %s", prfilnam);
#endif
	  if (system(q)) break;
	} else if (*ans == 'P' || *ans == 'p') {
	  pr_and_del_file(prfilnam);
	} else if (*ans != 'S' && *ans != 's') {
	  continue;
	}
	break;
      }
    }
    exit(0);

  } else if (!strncmp(ans, "SU", 2)) {
    /* sum counts using cursor */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    sumcts(0, idata, in);
  } else if (!strncmp(ans, "SW", 2)) {
    /* change starting width */
    startwid(ans, nc);
    if (caskyn("Reset free pars. to initial estimates? (Y/N)")) parset(0);
  } else if (!strncmp(ans, "SY", 2)) {
    /* execute system command */
    system(ans+2);
  } else if (!strncmp(ans, "X0", 2)) {
    /* change X0 for sp. display */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    if (idata + 2 > gf3gd.maxch) goto BADCMD;
    gf3gd.lox = idata;
  } else if (!strncmp(ans, "XA", 2)) {
    /* XA: modify X0,NX */
    sprintf(q, "Old value for X0 = %d\n New value = ? (return for old value)",
	    gf3gd.lox);
    while ((nc = cask(q, ans, 80)) &&
	   (inin(ans, nc, &idata, &j1, &j2) || idata + 2 > gf3gd.maxch));
    if (nc) gf3gd.lox = idata;
    sprintf(q, "Old value for NX = %d\n New value = ? (return for old value)",
	    gf3gd.numx);
    while ((nc = cask(q, ans, 80)) &&
	   (inin(ans, nc, &idata, &j1, &j2) || idata < 0));
    if (nc) gf3gd.numx = idata;
  } else if (!strncmp(ans, "XM", 2)) {
    inin(ans + 2, nc - 2, &idata, &in, &in2);
    gf3gd2.x_axis_marks = idata;
  } else if (!strncmp(ans, "Y0", 2)) {
    /* change Y0 for sp. display */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    gf3gd.locnt = idata;
  } else if (!strncmp(ans, "YA", 2)) {
    /* YA: modify Y0,NY */
    sprintf(q, "Old value for Y0 = %d\n New value = ? (return for old value)",
	    gf3gd.locnt);
    while ((nc = cask(q, ans, 80)) &&
	   inin(ans, nc, &idata, &j1, &j2));
    if (nc) gf3gd.locnt = idata;
    sprintf(q, "Old value for NY = %d\n New value = ? (return for old value)",
	    gf3gd.ncnts);
    while ((nc = cask(q, ans, 80)) &&
	   (inin(ans, nc, &idata, &j1, &j2) || idata < 0));
    if (nc) gf3gd.ncnts = idata;
  } else if (!strncmp(ans, "YM", 2)) {
    inin(ans + 2, nc - 2, &idata, &in, &in2);
    gf3gd2.y_axis_marks = idata;
  } else if (!strncmp(ans, "WI", 2)) {
    /* WI ; add windows to look-up table or slice file using cursor*/
    if (addwin()) goto BADCMD;
  } else if (!strncmp(ans, "WM", 2)) {
    /* change weighting mode */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    weight(idata);
  } else if (!strncmp(ans, "WS", 2)) {
    /* write spectrum to disk */
    wrtsp(ans, nc);
  } else {
    /* command cannot be recognized */
    goto BADCMD;
  }
  return 0;

 BADCMD:
  printf("Bad command.\n");
  if (infile) {
    printf("Bad command: %s\n", ans);
    strcpy(ans, "CF CHK");
    nc = 6;
    comfil(ans, nc);
  }
  return 1;

  /* shared code to redraw spectra on graphics screen */
 SHRDSP:
  rewind(scrfileb);
#define W(a,b,c) fwrite(b,c,a,scrfileb)
  W(15, colormap, 4);
  W(1, &gf3gd.locnt, 4);
  W(1, &gf3gd.ncnts, 4);
  W(1, &gf3gd.iyaxis, 4);
  W(1, &idata, 4);
  W(1, &in, 4);
  W(1, gf3gd.namesp, 8);
  W(1, &gf3gd.maxch, 4);
  W(gf3gd.maxch+1, gf3gd.spec, 4);
#undef W
  in2 = idata;
  erase();
  fflush(scrfilea);
  rewind(scrfilea);
#define R(a,b,c) fread(b,c,a,scrfilea)
  for (numspec = 1; numspec <= maxspec; ++numspec) {
    /* read display parameters and spectra back from scratch file */
    R(15, colormap, 4);
    R(1, &gf3gd.locnt, 4);
    R(1, &gf3gd.ncnts, 4);
    R(1, &gf3gd.iyaxis, 4);
    R(1, &idata, 4);
    R(1, &in, 4);
    R(1, gf3gd.namesp, 8);
    R(1, &gf3gd.maxch, 4);
    R(gf3gd.maxch+1, gf3gd.spec, 4);
    if (idata != -1 && in == 0) idata = 0;
    dspsp(idata, in, in2);
  }
#undef R
  fflush(scrfilea);
  fflush(scrfileb);
  rewind(scrfileb);
#define R(a,b,c) fread(b,c,a,scrfileb)
  R(15, colormap, 4);
  R(1, &gf3gd.locnt, 4);
  R(1, &gf3gd.ncnts, 4);
  R(1, &gf3gd.iyaxis, 4);
  R(1, &idata, 4);
  R(1, &in, 4);
  R(1, gf3gd.namesp, 8);
  R(1, &gf3gd.maxch, 4);
  R(gf3gd.maxch+1, gf3gd.spec, 4);
#undef R
  return 0;
} /* gfexec */

/* ======================================================================= */
int gffin(void)
{
  float d, r, y, r1, eb, eh, er, ew, beta, w, h;
  float dadr, dadb, dadw, dadh, dcdb, dcdr;
  int   i, iw;

  /* calc. areas, centroids and errors */
  r = gf3gd.pars[3] / 100.f;
  r1 = 1.0f - r;
  beta = gf3gd.pars[4];
  for (i = 0; i < gf3gd.npks; ++i) {
    iw = i*3 + 7;
    w  = gf3gd.pars[iw];
    h  = gf3gd.pars[iw+1];

    y = w / (beta * 3.33021838f);
    if (y > 4.0f) {
      d = 0.0f;
    } else {
      d = exp(-y*y) / erfc(y);
    }
    gf3gd.areas[i] = h * (r * 2.0f * beta * d  +  r1 * w * 1.06446705f);
    dadh =                r * 2.0f * beta * d  +  r1 * w * 1.06446705f;
    dadr =   0.01f * h * (    2.0f * beta * d  -       w * 1.06446705f);
    dadb = h * r * 2.0f * d * (1.0f + 2.0f * y*y - d * 1.12837917f * y);
    dadw = h * (r * 2.0f * 0.600561216f * d * (d / 1.77245385f - y) + r1 * 1.06446705f);
    gf3gd.areas[i] = h * dadh;
    er = gf3gd.errs[3]    * dadr;
    eb = gf3gd.errs[4]    * dadb;
    ew = gf3gd.errs[iw]   * dadw;
    eh = gf3gd.errs[iw+1] * dadh;
 
    gf3gd.dareas[i] = sqrt(eh*eh + er*er + eb*eb + ew*ew + 2.0f * (
	 gf3gd.covariances[3][4]     * dadr*dadb +
	 gf3gd.covariances[3][iw]    * dadr*dadw +
	 gf3gd.covariances[3][iw+1]  * dadr*dadh +
	 gf3gd.covariances[4][iw]    * dadb*dadw +
	 gf3gd.covariances[4][iw+1]  * dadb*dadh +
	 gf3gd.covariances[iw][iw+1] * dadw*dadh));
    dcdr =  - 0.02f * beta*beta * d / dadh;
    dcdb =  - r * 2.0f * beta * d * (2.0f + 2.0f * y*y - d * 1.12837917f * y) / dadh;
    gf3gd.cents[i]  = gf3gd.pars[iw-1] - beta * (r * 2.0f * beta * d / dadh);
    eb = gf3gd.errs[4] * dcdb;
    er = gf3gd.errs[3] * dcdr;
    gf3gd.dcents[i] = sqrt(gf3gd.errs[iw-1]*gf3gd.errs[iw-1] + er*er + eb*eb + 2.0f * (
	 gf3gd.covariances[3][4]    * dcdr*dcdb +
	 gf3gd.covariances[3][iw-1] * dcdr +
	 gf3gd.covariances[4][iw-1] * dcdb));
  }
  if (gf3gd.disp) dspfit();
  list_params(2);
  return 0;
} /* gffin */

/* ======================================================================= */
int gfhelp(char *ans)
{
  /* on-line help */

  int  i, j, nc, ilu = 0;
  char line[120], fullname[200];
  FILE *file;
  static char outfn[80] = "gf3help.txt";

  /* convert ans to upper case characters */
  file = NULL;
  for (i = 0; i < 80; ++i) {
    ans[i] = toupper(ans[i]);
  }
  if (!strncmp(ans, "HE/P", 4) || !strncmp(ans, "HELP/P", 6)) {
    /* print help */
    ilu = 3;
    /* open output (print) file */
    if (!(file = open_new_file(outfn, 0))) return 1;
    strcpy(ans, "HE SUM");
  }

  /* replace "HE(LP)" with spaces */
  strncpy(ans, "   ", 3);
  if (!strncmp(ans + 2, "LP", 2)) strncpy(ans + 2, "  ", 2);
  /* then leading spaces are removed and
     the end of command pointer, J, is returned */
  j = setext(ans, "    ", 80);
  /* are there enough characters to specify a command? */
  if (j == 1) {
    printf("Need two or more characters of the command.\n"
	   "Type HE to get list of topics.\n");
  }
  if (j <= 1) strcpy(ans, "TOP");

  get_directory("RADWARE_GFONLINE_LOC", fullname, 180);
  strcat(fullname, "gfonline.hlp");
  if (!(file1 = open_readonly(fullname))) {
    printf(" -- Check that the environment variable RADWARE_GFONLINE_LOC\n"
	   "    is properly defined, and that the file gfonline.hlp exists\n"
	   "    in the directory to which it points.\n");
    if (ilu) {
      fclose(file);
      pr_and_del_file(outfn);
    }
    return 0;
  }

  /* search for indicator plus two or three letter command */
  while (1) {
    if (!fgets(line, 100, file1)) {
      printf("Command %s not found.\n", ans);
      rewind(file1);
      strcpy(ans, "TOP");
      continue;
    }
    *(strchr(line,'\n')) = ' ';
    if (strncmp(line, ">>>", 3) || strncmp(line+3, ans, 3)) continue;

    /* have found command, now copy file to terminal or print file */
    while (strncmp(line, ">>>>END", 7)) {
      if (strncmp(line, ">>>", 3)) {
	/* write line to terminal or file */
	if (ilu) {
	  fprintf(file, "%s", line);
	} else {
	  printf("%s", line);
	}
      } else if (!ilu && !strncmp(line, ">>>>PAGE", 8)) {
	/* end of page */
	nc = cask("   Press any key for more, X to eXit help", ans, 1);
	if (ans[0] == 'X' || ans[0] == 'x') {
	  /* exit */
	  fclose(file1);
	  if (ilu) {
	    fclose(file);
	    pr_and_del_file(outfn);
	  }
	  return 0;
	}
      }
      /* get next line of help file */
      if (!fgets(line, 100, file1)) {
	printf("******* End of file encountered. ********\n");
	break;
      }
    }

    /* end of command listing
       ask for next topic */
    if (ilu) break;
    printf("\n");
    nc = 1;
    while (nc == 1) {
      nc = cask(">Topic = ? (rtn to exit help) ", ans, 40);
      strcat(ans,"   ");
      if (nc == 1 && ans[0] == '?') {
	strcpy(ans, "TOP");
	nc = 3;
      }
    }
    if (nc == 0) break;
    for (i = 0; i < nc; ++i) {
      ans[i] = toupper(ans[i]);
    }
    rewind(file1);
  }
  /* exit */
  fclose(file1);
  if (ilu) {
    fclose(file);
    pr_and_del_file(outfn);
  }
  return 0;
} /* gfhelp */

/* ======================================================================= */
int gfinit(int argc, char **argv)
{
  /* gf3 initialisation routine */

  float rj1, rj2, sw1, sw2, sw3;
  int   k, nx, ny, no_init_file = 0, no_cmd_file = 0;
  char  fullname[120], ans[80], l[120], *input_file_name = "";
  static int first = 1;
  static char *s1 =
    "\n\nWelcome to gf3, version 1.2\n\n"
    "gf3 will now fit up to 35 peaks.\n"
    "Check out the new commands:\n"
    "    S+, S-    - read next/previous spectrum from multi-spectrum file\n\n"
    "<Return> or 0 as an answer to any (Y/N) question is equivalent to N\n"
    "            1 as an answer to any (Y/N) question is equivalent to Y\n"
    " The default extension for all spectrum file names is .spe\n"
    " Type HE   for a list of available commands,\n"
    "      HE/P to print a list of available commands.\n\n"
    " D.C. Radford\n";
  static char *s2 =
    "\n\nWelcome to gf3, version 0.0\n\n"
    " To avoid answering these setup questions each time you enter gf3\n"
    "   create a file gfinit.dat and/or gfinit.cmd in your current\n"
    "   or home directory.  Type HELP GFINIT for details.\n\n"
    " Press any key to continue...";
  static char *s3 =
    "\nThis program fits portions of spectra with up to thirty-five peaks\n"
    "       on a quadratic background.\n"
    "The fitted parameters are :\n"
    "  A, B and C :  Background = A + B*X + C*X*X\n"
    "       where X is the channel number minus an offset.\n"
    "  R, BETA and STEP :  These define the shape of the peaks.\n"
    "       The peak is the sum of a gaussian of height H*(1-R/100) and\n"
    "       a skew gaussian of height H*R/100.  BETA is the decay constant\n"
    "       (skewness) of the skew gaussian, in channels.  STEP is the\n"
    "       relative height (in % of the peak height) of a smoothed step\n"
    "       function which increases the background below each peak.\n"
    "  Pn, Wn and Hn :   The position (centroid of the non-skew gaussian),\n"
    "       width and height of the nth peak.\n"
    "Initial estimates of A,B and C are taken to give a\n"
    "       straight line between the limits for the fit.\n"
    "Initial estimates for Pn and Hn are taken from the given peak\n"
    "       positions ( Hn = counts at peak position - background )\n\n"
    "Initial estimate for R is taken as R = A + B*X  (X = ch. no.)\n"
    "  Enter A,B (rtn for default: A=10., B=0.)";

  if (first) {
    first = 0;
#ifdef HAVE_GNU_READLINE
    // printf("reading readline history...\n");
    strcpy (ans, (char *) getenv("HOME"));
    strcat (ans, "/.gf3_history");
    (void) read_history(ans);
#endif

    /* parse input parameters */
    if (argc > 1) input_file_name = argv[1];
    if (!strcmp(input_file_name, "-h") || !strcmp(input_file_name, "--help")) {
      printf("Usage: gf3 [-l log_file_name] [input_spectrum_file_name]\n"
	     "  Type HE inside gf3 for a list of gf3 commands.\n");
      exit(0);
    } else if (!strcmp(input_file_name, "-l")) {
      if (argc > 2) strcpy(prfilnam, argv[2]);
      setext(prfilnam, ".log", 80);
      prfile = open_new_file(prfilnam, 0);
      input_file_name = "";
      if (argc > 3) input_file_name = argv[3];
    } else if (argc > 2) {
      if (argc > 3) strcpy(prfilnam, argv[3]);
      setext(prfilnam, ".log", 80);
      prfile = open_new_file(prfilnam, 0);
    }

    set_xwg("gf2_std_");
    initg(&nx, &ny);
    finig();

    gf3gd2.x_axis_marks = 2;
    gf3gd2.y_axis_marks = 2;
    gf3gd.dxlo  = 0;
    gf3gd.dxnum = 1;
    gf3gd.no_erase = 0;
    gf3gd.disp = 0;
    gf3gd.numx = 0;
    gf3gd.maxch = 1024;
    strncpy(gf3gd.namesp, "junk    ", 8);
    strcpy(gf3gd.filnam, "junk");
    gf3gd.wtmode = -1;
    gf3gd.iyaxis = -1;
    if (!gf3gd.ical) {
      gf3gd.ical = 1;
      gf3gd.gain[0] = 0.0;
      gf3gd.gain[1] = 0.5;
      gf3gd.nterms = 2;
    }
    /* gf3gd.npks = 1; */
    gf3gd.pkfind = 0;
    gf3gd.ifwhm = 5;
    gf3gd.isigma = 4;
    gf3gd.ipercent = 5;
    gf3gd.swpars[0] = 9.0f;
    gf3gd.swpars[1] = 0.004f;
    gf3gd.swpars[2] = 0.0f;
    gf3gd.irelpos = 1;
    gf3gd.irelw = 1;
    gf3gd.display_fit_diff = 1;

    /* open scratch files for storage of spectra to be redisplayed */
    if (!(scrfilea = tmpfile()) ||
	!(scrfileb = tmpfile())) {
      printf("ERROR - cannot open temp file!\n");
      exit(-1);
    }

    /* try to open and read file gfinit.dat in current directory */
    if (!(file1 = fopen("gfinit.dat", "r")) &&
	!(file1 = fopen(".gfinit.dat", "r"))) {
      /* try in the home directory */
      get_directory("HOME", fullname, 100);
      strcat(fullname, "gfinit.dat");
      if (!(file1 = fopen(fullname, "r"))) {
	get_directory("HOME", fullname, 100);
	strcat(fullname, ".gfinit.dat");
	if (!(file1 = fopen(fullname, "r"))) no_init_file = 1;
      }
    }

    if (!no_init_file) {
      /* read gfinit.dat file */
      if (fgets(l, 120, file1) &&
	  5 == sscanf(l, "%f,%f,%f,%f,%f", &gf3gd.finest[0],
		      &gf3gd.finest[1], &gf3gd.finest[2],
		      &gf3gd.finest[3], &gf3gd.finest[4]) &&
	  fgets(l, 120, file1) &&
	  3 == sscanf(l, "%d,%d,%d", &gf3gd.infix[0],
		      &gf3gd.infix[1], &gf3gd.infix[2]) &&
	  fgets(l, 120, file1) &&
	  3 == sscanf(l, "%f,%f,%f", &sw1, &sw2, &sw3) &&
	  fgets(l, 120, file1) &&
	  2 == sscanf(l, "%d,%d", &gf3gd.infixw, &gf3gd.infixrw)) {
	gf3gd.swpars[0] = sw1 * sw1;
	gf3gd.swpars[1] = sw2 * sw2 / 1e3f;
	gf3gd.swpars[2] = sw3 * sw3 / 1e6f;
      } else {
	file_error("read", fullname);
	no_init_file = 1;
      }
      fclose(file1);
    }

    /* now check for a gfinit.cmd file */
    if (!(file1 = fopen("gfinit.cmd", "r")) &&
	!(file1 = fopen(".gfinit.cmd", "r"))) {
      /* try in the home directory */
      get_directory("HOME", fullname, 100);
      strcat(fullname, "gfinit.cmd");
      if (!(file1 = fopen(fullname, "r"))) {
	get_directory("HOME", fullname, 100);
	strcat(fullname, ".gfinit.cmd");
	if (!(file1 = fopen(fullname, "r"))) no_cmd_file = 1;
	no_cmd_file = 1;
      }
    }
    if (!no_cmd_file) {
      /* execute gfinit.cmd file */
      fclose(file1);
      printf("%s", s1);
      if (strlen(input_file_name)) {
	strcpy(ans+2, input_file_name);
	getsp(ans, strlen(input_file_name) + 2);
      }
      k = cask(" Press any key to continue...", ans, 1);
      strcpy(ans, "CF gfinit");
      comfil(ans, 9);
      return 0;
    }
    if (!no_init_file) goto DONE;
  }

  /* we get here if no gfinit.dat or gfinit.cmd files could be found,
     or this is not first time gfinit is called  */
  k = cask(s2, ans, 1);

  while ((k = cask(s3, ans, 80)) &&
	 ffin(ans, k, gf3gd.finest, &gf3gd.finest[1], &rj2));
  if (k == 0) {
    gf3gd.finest[0] = 10.f;
    gf3gd.finest[1] = 0.f;
  }
  gf3gd.infix[0] = 1 -
    caskyn("Do you want always to fix R at this value? (Y/N)");

  while ((k = cask("Initial estimate for BETA is taken as"
		   " BETA = C + D*X  (X = ch. no.)\n"
		   "  Enter C,D (rtn for default: BETA = st. width/2)",
		   ans, 80)) &&
	 ffin(ans, k, &gf3gd.finest[2], &gf3gd.finest[3], &rj2));
  if (k == 0) {
    gf3gd.finest[2] = 0.f;
    gf3gd.finest[3] = 0.f;
  }
  gf3gd.infix[1] =  1 -
    caskyn("Do you want always to fix BETA at this value? (Y/N)");

  while ((k = cask("Initial estimate for STEP is taken as STEP = E\n"
		   "  Enter E (rtn for default: STEP = 0.25)", ans, 80)) &&
	 ffin(ans, k, &gf3gd.finest[4], &rj1, &rj2));
  if (k == 0) gf3gd.finest[4] = .25f;
  gf3gd.infix[2] =  1 -
    caskyn("Do you want always to fix STEP at this value? (Y/N)");
  startwid(ans, 2);

 DONE:
  printf("%s", s1);
  /* ask for spectrum file name and read spectrum from disk */
  strcpy(ans+2, input_file_name);
  getsp(ans, strlen(input_file_name) + 2);
  strcpy(ans, "DS");
  gfexec(ans, 2);

  return 0;
} /* gfinit */

/* ======================================================================= */
int gfset(void)
{
  float x=0, y, ch, rj1, rj2;
  int   i, k, n, nc, lo;
  char  ans[80];

 START:
  /* ask for limits for fit */
  printf(" Limits for fit?  (hit T to type, R to restart, Q to quit)\n");
  for (n = 0; n < 2; ++n) {
    if (gf3gd.disp) {
      retic(&x, &y, ans);
      gf3gd.mch[n] = x;
      if (ans[0] == 'X' || ans[0] == 'x' || ans[0] == 'Q' || ans[0] == 'q') {
        gf3gd.npks = 0;
        return 0;
      }
    } else {
      ans[0] = 'T';
    }
    while (ans[0] == 'T' || ans[0] == 't') {
      k = cask("Limit = ?", ans, 80);
      if (ans[0] == 'X' || ans[0] == 'x' || ans[0] == 'Q' || ans[0] == 'q') {
        gf3gd.npks = 0;
        return 0;
      }
      if (ffin(ans, k, &ch, &rj1, &rj2)) continue;
      gf3gd.mch[n] = ch + .5f;
      if (gf3gd.mch[n] <= gf3gd.maxch && gf3gd.mch[n] >= 0) break;
      printf("Marker ch. outside spectrum - try again.\n");
      gf3gd.mch[n] = x;
    }
  }
  if (gf3gd.mch[1] <= gf3gd.mch[0]) {
    lo = gf3gd.mch[1];
    gf3gd.mch[1] = gf3gd.mch[0];
    gf3gd.mch[0] = lo;
  }
  if (gf3gd.disp) {
    dspmkr(1);
    dspmkr(2);
  }
  if (gf3gd.mch[1] < gf3gd.mch[0] + 20) {
    printf("Markers are too close together!  Try again...\n");
    goto START;
  }

  /* ask for peak positions */
  if (gf3gd.npks <= 35) {
    printf(" Peak positions? (hit T to type, R to restart, Q to quit\n");
    for (n = 0; n < gf3gd.npks; ++n) {
      while (1) {
	if (gf3gd.disp) {
	  retic(&x, &y, ans);
	  gf3gd.ppos[n] = x - .5f;
	  if (ans[0] == 'R' || ans[0] == 'r') goto START;
	}
	while (!gf3gd.disp || ans[0] == 'T' || ans[0] == 't') {
	  /* hit t for type */
	  k = cask("Peak position = ?", ans, 80);
	  if (ans[0] == 'R' || ans[0] == 'r') goto START;
	  if (!ffin(ans, k, &gf3gd.ppos[n], &rj1, &rj2)) break;
	}
        if (ans[0] == 'Q' || ans[0] == 'q') {
          gf3gd.npks = 0;
          return 0;
        }
	if ((int) gf3gd.ppos[n] >= gf3gd.mch[0] &&
	    (int) gf3gd.ppos[n] <  gf3gd.mch[1]) break;
	printf(" Peaks must be within limits - try again.\n");
      }
      dspmkr(n+3);
    }
  } else {  /* gf3gd.npks > 35 */
    printf(" Peak positions? (hit X when done, T to type, R to restart, Q to quit)\n");
    for (n = 0; n < 35; ++n) {
      while (1) {
	if (gf3gd.disp) {
	  retic(&x, &y, ans);
	  if (ans[0] == 'X' || ans[0] == 'x') goto DONE;
	  gf3gd.ppos[n] = x - .5f;
	}
	while (!gf3gd.disp || ans[0] == 'T' || ans[0] == 't') {
	  /* hit t for type */
	  k = cask("Peak position=? (rtn when done)", ans, 80);
	  if (k == 0) goto DONE;
	  if (!ffin(ans, k, &gf3gd.ppos[n], &rj1, &rj2)) break;
	}
        if (ans[0] == 'R' || ans[0] == 'r')  goto START;
        if (ans[0] == 'Q' || ans[0] == 'q') {
          gf3gd.npks = 0;
          return 0;
        }
	if ((int) gf3gd.ppos[n] >= gf3gd.mch[0] &&
	    (int) gf3gd.ppos[n] <  gf3gd.mch[1]) break;
	printf(" Peaks must be within limits - try again.\n");
      }
      dspmkr(n+3);
    }
  DONE:
    gf3gd.npks = n;
    if (gf3gd.npks <= 0) return 0;
  }

  /* ask for fixed pars */
  gf3gd.npars = (gf3gd.npks + 2) * 3;
  for (i = 0; i <  gf3gd.npars; ++i) {
    gf3gd.freepars[i] = 1;
  }
  gf3gd.irelpos = 1;
  gf3gd.nfp = 0;
  parset(-1);
  strcpy(ans, "FX");
  nc = 2;
  fixorfree(ans, nc);
  return 0;
} /* gfset */

/* ======================================================================= */
int lookup(char *filnam, int nc)
{
  /* open old or create new look-up table file
     (file name stored in FILNAM)
     default file name = .tab
     winmod = 0 : no mode defined
     winmod = 1 : look-up file mode
     winmod = 2 : slice file mode */

  int  i, j, n, j1, j2;
  char cmess[45];

  strncpy(filnam, "  ", 2);
  if (nc < 3) {
    /* ask for output file name */
  START:
    nc = cask("Name of lookup file = ? (default .ext = .tab)", filnam, 80);
    if (nc == 0) return 0;
  }
  /* save any files already created */
  if (winmod == 1) wrtlook();
  if (winmod != 0) fclose(winfile);
  winmod = 1;
  setext(filnam, ".tab", 80);
  /* try to open OLD output file */
  if (inq_file(filnam, &j)) {
    if (read_tab_file(filnam, &gf3gd.nclook, &gf3gd.lookmin,
		      &gf3gd.lookmax, gf3gd.looktab, 16384)) {
      winmod = 0;
      goto START;
    }
    if (caskyn("Modify existing file? (Y/N)")) {
      if ((winfile = fopen(filnam, "r+"))) return 0;
      file_error("open for rewriting", filnam);
      goto START;
    }
    if (!caskyn(" Create new version? (Y/N)")) goto START;
  }
  /* open NEW output file */
  if (!(winfile = open_new_file(filnam, 0))) {
    winmod = 0;
    return 0;
  }
  /* ask for dimension of look-up table
     (default = spectrum dimension) */
  gf3gd.nclook = gf3gd.maxch + 1;
  sprintf(cmess, "Dimension of look-up table = ? (rtn for %d)", gf3gd.nclook);
  while ((nc = cask(cmess, filnam, 80)) &&
	 (inin(filnam, nc, &n, &j1, &j2) || n > 16384 || n < 2));
  if (nc) gf3gd.nclook = n;

  /* initialize look-up table */
  gf3gd.lookmin = 0;
  gf3gd.lookmax = 0;
  for (i = 0; i < gf3gd.nclook; ++i) {
    gf3gd.looktab[i] = 0;
  }
  return 0;
} /* lookup */

/* ======================================================================= */
int matread(char *fn, float *sp, char *namesp, int *numch, int idimsp,
	    int *spmode)
{
  /* subroutine to read spectrum from matrix file
     into array sp, of dimension idimsp
     fn = file name, or contains range of channels to read
     numch = number of channels read
     namesp = name of spectrum (char*8, set to ilo ihi)
     spmode = 2/3 for .mat/.spn files
     file extension must be .mat */

  /* 2000/10/31:  added support for .sec files (like 8192-ch .spn files)
     spmode = 5 for .sec files */

  int i4spn [8192];
#define i2mat ((short *) i4spn)

  float x=0, y;
  int   i4sum[8192], i, j, k, jrecl;
  int   in3, ilo, ihi, nc, iy, nch;
  char  ans[80], q[256];
  FILE  *file;
  static char filnam[80];

  *numch = 0;

  if (!strncmp(fn+2, "/C", 2) || !strncmp(fn+2, "/c", 2)) {
    if (!gf3gd.disp) {
      printf("Bad command: New spectrum not yet displayed...\n");
      return 1;
    }
    /* use cursor to get y-channel limits of gate */
    printf(" Hit any character; A to abort...\n");
    retic(&x, &y, ans);
    if (ans[0] == 'A' || ans[0] == 'a') return 0;
    ilo = x;
    retic(&x, &y, ans);
    if (ans[0] == 'A' || ans[0] == 'a') return 0;
    ihi = x;
    if (*spmode != 2 && *spmode != 3) {
      /* spmode .ne. 2 or 3; ask for matrix file name */
START:
      j = setext(filnam, "    ", 80);
      if (!strcmp(filnam + j, ".MAT") ||
	  !strcmp(filnam + j, ".mat") ||
	  !strcmp(filnam + j, ".SPN") ||
	  !strcmp(filnam + j, ".spn") ||
	  !strcmp(filnam + j, ".SEC") ||
	  !strcmp(filnam + j, ".sec") ||
	  !strcmp(filnam + j, ".M4B") ||
	  !strcmp(filnam + j, ".m4b")) {
	sprintf(q, "Matrix file = ? (default .ext = .mat)\n"
		   "                (rtn for default = %s)", filnam);
      } else {
	strcpy(q, "Matrix file = ? (default .ext = .mat)");
      }
      if (!(k = cask(q, ans, 80))) {
	if (strcmp(filnam + j, ".MAT") &&
	    strcmp(filnam + j, ".mat") &&
	    strcmp(filnam + j, ".SPN") &&
	    strcmp(filnam + j, ".spn") &&
	    strcmp(filnam + j, ".SEC") &&
	    strcmp(filnam + j, ".sec") &&
	    strcmp(filnam + j, ".M4B") &&
	    strcmp(filnam + j, ".m4b")) return 1;
      } else {
	j = setext(ans, ".mat", 80);
	if (strcmp(ans + j, ".MAT") &&
	    strcmp(ans + j, ".mat") &&
	    strcmp(ans + j, ".SPN") &&
	    strcmp(ans + j, ".spn") &&
	    strcmp(ans + j, ".SEC") &&
	    strcmp(ans + j, ".sec") &&
	    strcmp(ans + j, ".M4B") &&
	    strcmp(ans + j, ".m4b")) {
	  goto START;
	}
	if (!inq_file(ans, &jrecl)) {
	  file_error("open", ans);
	  goto START;
	}
	strncpy(filnam, ans, 80);
      }
      *spmode = 3;
      if (!strcmp(filnam + j, ".MAT") ||
	  !strcmp(filnam + j, ".mat")) *spmode = 2;
      if (!strcmp(filnam + j, ".SEC") ||
	  !strcmp(filnam + j, ".sec")) *spmode = 5;
    }
  } else {
    /* SP command does not include "/C"
       look for int chan. no. limits */
    nc = strlen(fn);
    if (inin(fn, nc, &ilo, &ihi, &in3)) {
      /* not ints; fn = file name, check matrix file exists*/
      if (!inq_file(fn, &jrecl)) {
	file_error("open", fn);
	return 1;
      }
      strncpy(filnam, fn, 80);
      /* ask for range of y-channels to be added together */
      while ((nc = cask("Range of y-channels (lo,hi) = ?", ans, 80)) &&
	     inin(ans, nc, &ilo, &ihi, &in3));
      if (nc == 0) return 0;
    }
  }
  if (ihi == 0) ihi = ilo;
  if (ihi < ilo) {
    i = ilo;
    ilo = ihi;
    ihi = i;
  }
  if (ilo < 0 || ihi >= 4096) {
    printf("Bad y-gate channel limits: %d %d", ilo, ihi);
    return 1;
  }
  if (!(file = open_readonly(filnam))) return 1;
  nch = 4096;
  if (*spmode == 5) nch = 8192;
  *numch = nch;
  if (*numch > idimsp) {
    *numch = idimsp;
    printf("First %d chs only taken.", idimsp);
  }
  /* initialize i4sum */
  for (i = 0; i < *numch; ++i) {
    i4sum[i] = 0;
  }
  if (*spmode == 2) {
    /* read matrix rows into i2mat and add into i4sum */
    fseek(file, ilo*8192, SEEK_SET);
    for (iy = ilo + 1; iy <= ihi + 1; ++iy) {
      if (fread(i2mat, 8192, 1, file) != 1) {
	file_error("read", filnam);
	fclose(file);
	return 1;
      }
      for (i = 0; i < *numch; ++i) {
	i4sum[i] += i2mat[i];
      }
    }
  } else {
    /* read matrix rows into i4spn and add into i4sum */
    fseek(file, ilo*nch*4, SEEK_SET);
    for (iy = ilo; iy <= ihi; ++iy) {
      if (fread(i4spn, nch*4, 1, file) != 1) {
	file_error("read", filnam);
	fclose(file);
	return 1;
      }
      for (i = 0; i < *numch; ++i) {
	i4sum[i] += i4spn[i];
      }
    }
  }
  /* convert to float */
  for (i = 0; i < *numch; ++i) {
    sp[i] = (float) i4sum[i];
  }
  fclose(file);
  strncpy(fn, filnam, 80);
  sprintf(namesp, "%4d%4d", ilo, ihi);
  return 0;
} /* matread */
#undef i2mat

/* ======================================================================= */
int para2num(char *ans, int *param)
{
  /* change an alphanumeric ans into param (parameter number)
     returns 1 for unrecognized parameter */
  /* called by fixorfree */

  int  i;
  char tmp_ans[80];
  static char parc[111][4] = {
    "A","B","C","R","BTA","STP","P1","W1","H1","P2","W2","H2",
    "P3","W3","H3","P4","W4","H4","P5","W5","H5","P6","W6","H6",
    "P7","W7","H7","P8","W8","H8","P9","W9","H9","PA","WA","HA",
    "PB","WB","HB","PC","WC","HC","PD","WD","HD","PE","WE","HE",
    "PF","WF","HF","PG","WG","HG","PH","WH","HH","PI","WI","HI",
    "PJ","WJ","HJ","PK","WK","HK","PL","WL","HL","PM","WM","HM",
    "PN","WN","HN","PO","WO","HO","PP","WP","HP","PQ","WQ","HQ",
    "PR","WR","HR","PS","WS","HS","PT","WT","HT","PU","WU","HU",
    "PV","WV","HV","PW","WW","HW","PX","WX","HX","PY","WY","HY",
    "PZ","WZ","HZ"};

  strncpy(tmp_ans, ans, 80);
  /* remove leading spaces */
  while (*(unsigned char *)tmp_ans == ' ') {
    memmove(tmp_ans, tmp_ans + 1, 80);
    tmp_ans[79] = '\0';
  }
  /* convert lower case to upper case characters */
  for (i = 0; i < 4; ++i) {
    tmp_ans[i] = toupper(tmp_ans[i]);
  }
  for (*param = 1; *param <= gf3gd.npars; ++(*param)) {
    if (!strcmp(tmp_ans, parc[*param-1])) return 0;
  }
  if (!strcmp(tmp_ans, "BETA")) {
    *param = 5;
  } else if (!strcmp(tmp_ans, "STEP")) {
    *param = 6;
  } else if (!strcmp(tmp_ans, "RP")) {
    *param = 1001;
  } else if (!strcmp(tmp_ans, "RW")) {
    *param = 1002;
  } else {
    return 1;   /* no match */
  }
  return 0;
} /* para2num */

/* ======================================================================= */
int parset(int mode)
{
  float x, y, x0;
  int   i, j, resetp, resetw, ihi, ilo, ipp;

  ilo = gf3gd.mch[0];
  ihi = gf3gd.mch[1];
  x0 = (float) ((ihi + ilo) / 2);
  for (i = 0; i < gf3gd.npars; ++i) {
    gf3gd.errs[i] = 0.f;
  }
  x = 0.f;
  for (i = 0; i < gf3gd.npks; ++i) {
    x += gf3gd.ppos[i];
  }
  x /= (float) gf3gd.npks;

  if (gf3gd.freepars[0] == 1)
    gf3gd.pars[0] = (gf3gd.spec[ihi] + gf3gd.spec[ilo]) / 2.f;
  if (gf3gd.freepars[1] == 1)
    gf3gd.pars[1] = (gf3gd.spec[ihi] - gf3gd.spec[ilo]) / (float) (ihi-ilo);
  if (gf3gd.freepars[2] == 1) gf3gd.pars[2] = 0.f;
  if (gf3gd.freepars[3] == 1) gf3gd.pars[3] = gf3gd.finest[0] + gf3gd.finest[1] * x;
  if (gf3gd.freepars[4] == 1) gf3gd.pars[4] = gf3gd.finest[2] + gf3gd.finest[3] * x;
  if (gf3gd.pars[4] == 0.f)
    gf3gd.pars[4] = sqrt(gf3gd.swpars[0] +
			 gf3gd.swpars[1] * x + 
			 gf3gd.swpars[2] * x * x) * .5f;
  if (gf3gd.freepars[5] == 1) gf3gd.pars[5] = gf3gd.finest[4];
  if (mode < 0) {
    /* come here only during set-up */
    resetp = 1;
    resetw = 1;
    gf3gd.nfp = 3 - gf3gd.infix[0] - gf3gd.infix[1] - gf3gd.infix[2];
    gf3gd.freepars[3] = gf3gd.infix[0];
    gf3gd.freepars[4] = gf3gd.infix[1];
    gf3gd.freepars[5] = gf3gd.infix[2];
    if (gf3gd.freepars[3] == 0 && gf3gd.pars[3] == 0.f) {
      gf3gd.nfp += gf3gd.freepars[4];
      gf3gd.freepars[4] = 0;
    }
    gf3gd.irelw = gf3gd.infixrw;
    if (gf3gd.infixw == 0) {
      for (i = 1; i <= gf3gd.npks; ++i) {
	gf3gd.freepars[i*3 + 4] = 0;
      }
      gf3gd.nfp += gf3gd.npks;
    }
  } else {
    resetp = 1;
    if (gf3gd.irelpos == 0)
      resetp = caskyn("Relative positions fixed - reset peak positions? (Y/N)");
    resetw = 1;
    if (gf3gd.irelw == 0)
      resetw = caskyn("Relative widths fixed - reset widths? (Y/N)");
  }
  for (i = 0; i < gf3gd.npks; ++i) {
    gf3gd.areas[i] = 0.f;
    gf3gd.dareas[i] = 0.f;
    gf3gd.cents[i] =0.f;
    j = i*3 + 6;
    if (resetp && gf3gd.freepars[j] == 1) gf3gd.pars[j] = gf3gd.ppos[i];
    if (mode < 0 || (resetw && gf3gd.freepars[j+1] == 1))
      gf3gd.pars[j+1] = sqrt(gf3gd.swpars[0] +
			     gf3gd.swpars[1] * gf3gd.ppos[i] +
			     gf3gd.swpars[2] * gf3gd.ppos[i] * gf3gd.ppos[i]);
    if (gf3gd.freepars[j+2] == 1) {
      x = gf3gd.ppos[i] - x0;
      y = gf3gd.pars[0] + gf3gd.pars[1] * x;
      ipp = gf3gd.ppos[i] + 0.5f;
      gf3gd.pars[j+2] = gf3gd.spec[ipp] - y;
      gf3gd.cents[i] = gf3gd.pars[j];
    }
  }
  return 0;
} /* parset */

/* ======================================================================= */
int pfind(float *chanx, float *psize, int loch, int hich,
	  int ifwhm, float sigma, int maxpk, int *npk)
{
  /* modified for gf3: data is float
     peak-finder routine    ornl 5/4/76 jdl.
     ifwhm   = fwhm estimate supplied by calling prog
     sigma   = peak detection threshold (std dev above bkg)
     spec(i) = data array to be scanned
     chanx(i)= location of ith peak (index basis, not chan# basis)
     psize(i)= very rough estimate of peak area (may by way! off!!) */

  float deno, ymin, root, peak1, peak2, peak3;
  float p1, p2, p3, s2, w4, pc, add, sum, sum2, save[16384];
  int   jodd, lock, i, j, n, wings, j1, j2;
  int   jd, nd, jl, jr, np, ctr;

  *npk = 0;
  np = 0;
  n = 0;
  lock = 0;
  p1 = 0.f;
  p2 = 0.f;
  p3 = 0.f;
  peak1 = 0.f;
  peak2 = 0.f;
  peak3 = 0.f;
  sum = 0.f;
  sum2 = 0.f;
  if (ifwhm < 1) ifwhm = 1;
  j1 = -((ifwhm << 1) - 1) / 4;
  j2 = j1 + ifwhm - 1;
  jodd = 1 - (ifwhm - (ifwhm / 2 << 1));
  /* test for any negative, if yes add to data to make all positive */
  ymin = 0.f;
  add = 0.f;
  for (i = loch-1; i < hich; ++i) {
    if (gf3gd.spec[i] < ymin) ymin = gf3gd.spec[i];
  }
  if (ymin < 0.f) {
    add = -ymin;
    for (i = loch-1; i < hich; ++i) {
      save[i] = gf3gd.spec[i];
      gf3gd.spec[i] += add;
    }
  }
  /* now go ahead and do the peakfind bit */
  for (nd = loch + ifwhm; nd <= hich - ifwhm - 1; ++nd) {
    /* sum counts in center and wings of differential function */
    ctr = 0;
    wings = 0;
    for (j = j1; j <= j2; ++j) {
      jd = nd + j;
      ctr += gf3gd.spec[jd - 1];
      if (j > 0) {
	jl = nd - j + j1;
	jr = jd + j2;
	wings = wings + gf3gd.spec[jl-1] + gf3gd.spec[jr-1];
      }
    }
    /* if ifwhm is odd, average data in split cells at ends */
    w4 = 0.f;
    if (jodd == 0) {
      jl = nd + j1 + j1;
      jr = nd + j2 + j2;
      w4 = (gf3gd.spec[jl-2] + gf3gd.spec[jl-1] +
	    gf3gd.spec[jr-1] + gf3gd.spec[jr]) * .25f;
    }
    /* compute height of second derivative (neg) relative to noise*/
    s2 = sum;
    sum = (float) (ctr - wings) - w4;
    root = sqrt((float) (ctr + wings + 1) + w4);
    p1 = p2;
    p2 = p3;
    deno = root;
    if (deno < 1e-6f) deno = 1e-6f;
    p3 = sum / deno;
    if (lock != 0) {
      if (p2 > peak2 && p3 < p2) {
	/* save three values at relative maximum */
	peak1 = p1;
	peak2 = p2;
	peak3 = p3;
	sum2 = s2;
	np = nd - 1;
      }
      if (p3 >= sigma) continue;
      /* estimate location and crude size of peak */
      if (++n > maxpk) n = maxpk;
      deno = peak1 - peak2 + peak3 - peak2;
      if (fabs(deno) < 1e-6f) deno = 1e-6f;
      pc = (peak1 - peak3) * .5f / deno;
      chanx[n-1] = (float) np + pc + (float) jodd * .5f;
      psize[n-1] = sum2 + sum2;
      lock = 0;
      peak2 = 0.f;
    }
    if (p3 >= sigma) lock = 1;
  }
  *npk = n;
  /* test for offset added - i.e. is data restore required */
  if (add != 0.f) {
    for (i = loch-1; i < hich; ++i) {
      gf3gd.spec[i] = save[i];
    }
  }
  return 0;
} /* pfind */

/* ======================================================================= */
int polfit(double *x, double *y, double *sigmay, int npts,
	   int nterms, int mode, double *a, double *chisqr)
{
  double weight, sumx[19], sumy[10];
  double delta, chisq, array[10][10];
  double xterm, yterm, xi, yi;
  int    nmax, i, j, k, l, n;

  /* accumulate weighted sums */
  nmax = 2*nterms - 1;
  for (n = 0; n < nmax; ++n) {
    sumx[n] = 0.;
  }
  for (j = 0; j < nterms; ++j) {
    sumy[j] = 0.;
  }
  chisq = 0.;
  for (i = 0; i < npts; ++i) {
    xi = x[i];
    yi = y[i];
    if (mode < 0) {
      if (yi > 0.f) {
	weight = 1.f / yi;
      } else if (yi < 0.f) {
	weight = 1.f / (-yi);
      } else {
	weight = 1.f;
      }
    } else if (mode == 0) {
      weight = 1.f;
    } else {
      weight = 1.f / (sigmay[i] * sigmay[i]);
    }
    xterm = weight;
    for (n = 0; n < nmax; ++n) {
      sumx[n] += xterm;
      xterm *= xi;
    }
    yterm = weight * yi;
    for (n = 0; n < nterms; ++n) {
      sumy[n] += yterm;
      yterm *= xi;
    }
    chisq += weight * yi * yi;
  }
  /* construct matrices and calculate coefficients */
  for (j = 0; j < nterms; ++j) {
    for (k = 0; k < nterms; ++k) {
      n = j + k;
      array[k][j] = sumx[n];
    }
  }
  delta = determ(array[0], nterms);
  if (delta == 0.f) {
    *chisqr = 0.f;
    for (j = 0; j < nterms; ++j) {
      a[j] = 0.f;
    }
  } else {
    for (l = 0; l < nterms; ++l) {
      for (j = 0; j < nterms; ++j) {
	for (k = 0; k < nterms; ++k) {
	  n = j + k;
	  array[k][j] = sumx[n];
	}
	array[l][j] = sumy[j];
      }
      a[l] = determ(array[0], nterms) / delta;
    }
    /* calculate chi square */
    for (j = 0; j < nterms; ++j) {
      chisq -= a[j] * 2.f * sumy[j];
      for (k = 0; k < nterms; ++k) {
	n = j + k;
	chisq += a[j] * a[k] * sumx[n];
      }
    }
    *chisqr = chisq / (npts - nterms);
  }
  return 0;
} /* polfit */

/* ======================================================================= */
int setcts(void)
{
  float x=0, y, y1=0.0f, y2=0.0f, rj1, rj2, fnc;
  int   i, k, isave, j2, nx, ny, ihi, ilo;
  char  ans[80];

  while (1) {
    if (!gf3gd.disp) {
      ans[0] = 'T';
    } else {
      printf("Hit any character;\n"
	     " X or right mouse button to exit, T to type.\n");
      retic(&x, &y, ans);
      if (ans[0] == 'X' || ans[0] == 'x') return 0;
    }
    if (ans[0] != 'T' && ans[0] != 't') {
      ilo = x;
      y1 = y;
      retic(&x, &y, ans);
      if (ans[0] == 'X' || ans[0] == 'x') return 0;
      ihi = x;
      y2 = y;
    }
    if (ans[0] == 'T' || ans[0] == 't') {
      /* ask for typed limits */
      while (1) {
	k = cask("Limits (chs) = ?", ans, 80);
	if (k == 0) return 0;
	if (inin(ans, k, &ilo, &ihi, &j2)) continue;
	if (ilo > 0 && ihi == 0) ihi = ilo;
	if (ilo > gf3gd.maxch || ihi > gf3gd.maxch || ilo < 0 || ihi < 0) {
	  printf("Marker ch. outside spectrum - try again.\n");
	  continue;
	}
	k = cask("Counts per ch. = ?", ans, 80);
	if (!ffin(ans, k, &y, &rj1, &rj2)) {
	  y1 = y;
	  y2 = y;
	  break;
	}
      }
    }

    if (ilo > ihi) {
      isave = ihi;
      ihi = ilo;
      ilo = isave;
      y2 = y1;
      y1 = y;
    }

    /* modify contents of spectrum */
    strncpy(gf3gd.namesp + 4, ".MOD", 4);
    gf3gd.spec[ilo] = (y1 + y2) / 2.f;
    if (ilo == ihi) continue;
    fnc = (float) (ihi - ilo);
    for (i = ilo; i <= ihi; ++i) {
      gf3gd.spec[i] = y1 + (y2 - y1) * (float) (i - ilo) / fnc;
    }
    /* display modified segment */
    initg(&nx, &ny);
    x = (float) ilo + .5f;
    pspot(x, y1);
    x = (float) ihi + .5f;
    vect(x, y2);
    finig();
  }
} /* setcts */

/* ======================================================================= */
int slice(char *ans, int nc)
{
  int j;

  /* open old or create new slice input file
     (file name stored in ans)
     default file name = .win
     winmod = 0 : no mode defined
     winmod = 1 : look-up file mode
     winmod = 2 : slice file mode */
  strncpy(ans, "  ", 2);
  if (nc < 3) {
    /* ask for output file name */
    nc = cask("Name of window file = ? (default .ext = .win)", ans, 80);
    if (nc == 0) return 0;
  }
  /* save any files already created */
  if (winmod == 1) wrtlook();
  if (winmod != 0) fclose(winfile);
  winmod = 2;
  setext(ans, ".win", 80);

  /* try to open old output file */
  while (inq_file(ans, &j)) {
    if (caskyn(" Add to existing file? (Y/N)")) {
      if ((winfile = fopen(ans, "a+"))) return 0;
      file_error("open for rewriting", ans);
    } else if (caskyn(" Replace existing file? (Y/N)")) {
      if ((winfile = fopen(ans, "w+"))) return 0;
      file_error("open new", ans);
    }
    /* ask for output file name */
    nc = cask("Name of window file = ? (default .ext = .win)", ans, 80);
    if (nc == 0) return 0;
    setext(ans, ".win", 80);
  }
  /* open new output file */
  if (!(winfile = open_new_file(ans, 0))) winmod = 0;
  return 0;
} /* slice */

/* ======================================================================= */
int startwid(char *ans, int nc)
{
  float sw1, sw2, sw3;
  char  q[512];

  if (nc > 2) memmove(ans, ans + 2, 78);
  nc -= 2;

  while (1) {
    if (nc > 0) {
      if (ffin(ans, nc, &sw1, &sw2, &sw3)) continue;
      if (sw1 > 0.f && sw2 >= 0.f) {
	gf3gd.swpars[0] = sw1 * sw1;
	gf3gd.swpars[1] = sw2 * sw2 / 1e3f;
	gf3gd.swpars[2] = sw3 * sw3 / 1e6f;
	break;
      }
      printf("Bad values.  F and G must both be positive.\n");
    }
    sprintf(q,"Initial estimates for the fitted peak widths are taken as:\n"
	    "       FWHM = sqrt(F*F + G*G*x + H*H*x*x)  (x = ch.no. /1000)\n"
	    "  Default values are:  F = %.2f, G = %.2f, H = %.2f\n"
	    "  Enter F,G,H  = ? (rtn for default values)",
	    sqrt(gf3gd.swpars[0]), sqrt(gf3gd.swpars[1] * 1e3f),
	    sqrt(gf3gd.swpars[2]) * 1e3f);
    if (!(nc = cask(q, ans, 80))) break;
  }
  gf3gd.infixw = 1 - caskyn("Do you want all widths to be fixed by default? (Y/N)");
  gf3gd.infixrw = 1 - caskyn("Do you want the relative widths to be fixed by default? (Y/N)");
  return 0;
} /* startwid */

/* ======================================================================= */
int storac(int in)
{
  /* store areas and centroids for later analysis */
 
  int i, j, k, idata, j1, j2;
  char ans[80];
  static char outfn[80] = "gf3.sto";

  idata = in;
  while (idata == 0 || idata > 20) {
    k = cask("N = 1-20: store centroids and areas in one of 20 positions\n"
	     "N = -1:   write stored values to disk file gf3.sto\n"
	     " ...N = ?", ans, 80);
    if (k == 0) return 0;
    inin(ans, k, &idata, &j1, &j2);
  }
  if (idata > 0) {
    if (gf3gd.isto[idata - 1] != 0) {
      if (!caskyn("WARNING: already have values not written to disk!\n"
		  " ...Proceed? (Y/N)")) return 0;
    }
    gf3gd.isto[idata - 1] = gf3gd.npks;
    for (i = 0; i < gf3gd.npks; ++i) {
      gf3gd.stoc [idata-1][i] = gf3gd.cents [i];
      gf3gd.stodc[idata-1][i] = gf3gd.dcents[i];
      gf3gd.stoa [idata-1][i] = gf3gd.areas [i];
      gf3gd.stoda[idata-1][i] = gf3gd.dareas[i];
      gf3gd.stoe [idata-1][i] = 0.f;
      gf3gd.stode[idata-1][i] = 0.f;
      energy(gf3gd.cents[i], gf3gd.dcents[i], 
	     &gf3gd.stoe [idata-1][i],
	     &gf3gd.stode[idata-1][i]);
    }
    strncpy(gf3gd.namsto + (idata-1) * 28, gf3gd.namesp, 8);
    datetime(gf3gd.namsto + ((idata-1) * 28 + 8));
    printf(" %d areas and centroids stored in %d of 20.\n", gf3gd.npks, idata);
    return 0;
  }

  /* idata < 0: store saved values in file gf3.sto */
  for (j = 0; j < 20; ++j) {
     if (gf3gd.isto[j]) goto STORE;
  }
  return 1; /* no data to save */

 STORE:
  if (!(file2 = fopen(outfn, "a+"))) file2 = open_new_file(outfn, 0);
  if (!file2) return 1;
  fprintf(file2, " No.  Centroid +- error      Area +- error  "
	         "    Energy +- error    Sp.name      Date     Time\n");
  for (i = 0; i < 35; ++i) {
    for (j = 0; j < 20; ++j) {
      if (gf3gd.isto[j] > i) {
	fprintf(file2,
		"%3d%11.4f%9.4f%11.0f%9.0f%11.4f%9.4f    %.8s  %s\n", j+1,
		gf3gd.stoc[j][i], gf3gd.stodc[j][i],
		gf3gd.stoa[j][i], gf3gd.stoda[j][i],
		gf3gd.stoe[j][i], gf3gd.stode[j][i],
		gf3gd.namsto + j* 28, gf3gd.namsto + j* 28 + 8);
      }
    }
  }
  fclose(file2);
  for (j = 1; j <= 20; ++j) {
    gf3gd.isto[j-1] = 0;
  }
  return 0;
} /* storac */

/* ======================================================================= */
int sumcts(int mode, int loch, int hich)
{
  /* mode=0: sum without background subtraction
     mode=1: sum with    background subtraction */

  float area, cent, x=0, y, y1=0.0f, y2=0.0f;
  float dc, eg, deg, fnc, cou, cts, sum, r1;
  int   isav, i, isave, j2, nc, nx, ny, repeat=0;
  char  ans[80], l[120];

  if (mode > 0 ||
      (loch == 0 && hich == 0) ||
      loch < 0 || hich < 0 ||
      loch > gf3gd.maxch || hich > gf3gd.maxch) {
    repeat = 1;
    if (!gf3gd.disp && mode > 0) {
      printf("Bad command: New spectrum not yet displayed...\n");
      return 1;
    }
    if (gf3gd.disp) {
      if (mode == 0) {
	printf("Hit any character; T to type limits, X to exit\n");
      } else {
	printf("Hit any character; X to exit\n");
      }
    }

    /* get limits for integration, background */
   REPEAT:
    while (1) {
      if (!gf3gd.disp || ans[0] == 'T' || ans[0] == 't') {
	nc = cask("Limits for integration = ? (rtn to quit)", ans, 80);
	if (nc == 0) return 0;
	y1 = 0;
	y2 = 0;
	if (!inin(ans, nc, &loch, &hich, &j2) &&
	    loch >= 0 && hich > 0 &&
	    loch <= gf3gd.maxch &&
	    hich <= gf3gd.maxch) break;
      } else {
	retic(&x, &y, ans);
	if (ans[0] == 'X' || ans[0] == 'x') return 0;
	if (mode == 0 && (ans[0] == 'T' || ans[0] == 't')) continue;
	loch = x;
	y1 = y;
	retic(&x, &y, ans);
	if (ans[0] == 'X' || ans[0] == 'x') return 0;
	if (mode == 0 && (ans[0] == 'T' || ans[0] == 't')) continue;
	hich = x;
	y2 = y;
	break;
      }
    }
  }

  if (loch > hich) {
    isav = hich;
    hich = loch;
    loch = isav;
    y  = y2;
    y2 = y1;
    y1 = y;
  }

  /* display limits */
  if (gf3gd.disp) {
    isave = gf3gd.mch[0];
    gf3gd.mch[0] = loch;
    dspmkr(1);
    gf3gd.mch[0] = hich;
    dspmkr(1);
    gf3gd.mch[0] = isave;
  }
  sum = 0.f;
  area = 0.f;
  cent = 0.f;
  dc = 0.f;
  fnc = (float) (hich - loch);
  if (fnc < .5f) fnc = 1.f;
  if (mode != 0) {
    /* background to be subtracted */
    initg(&nx, &ny);
    x = (float) (loch) + .5f;
    pspot(x, y1);
    x = (float) (hich) + .5f;
    vect(x, y2);
    finig();
    for (i = loch; i <= hich; ++i) {
      cou = gf3gd.spec[i] - (y1 + (y2 - y1) * (float) (i - loch) / fnc);
      area += cou;
      cent += cou * (float) (i - loch);
    }
  } else {
    /* no background to be subtracted */
    for (i = loch; i <= hich; ++i) {
      area += gf3gd.spec[i];
      cent += gf3gd.spec[i] * (float) (i - loch);
    }
  }
  if (area == 0.f) {
    printf("  Chs %d to %d    Area = %.0f\n", loch, hich, area);
    if (repeat) goto REPEAT;
    return 0;
  }
  cent = cent / area + (float) (loch);
  if (gf3gd.wtmode < 1) {
    /* weight data with data */
    for (i = loch; i <= hich; ++i) {
      cts = gf3gd.spec[i];
      if (cts < 1.f) cts = 1.f;
      sum += cts;
      r1 = ((float) i - cent) / area;
      dc += cts * (r1 * r1);
    }
  } else {
    /* weight data with WTSP */
    for (i = loch; i <= hich; ++i) {
      cts = gf3gd.wtsp[i];
      if (cts < 1.f) cts = 1.f;
      sum += cts;
      r1 = ((float) i - cent) / area;
      dc += cts * (r1 * r1);
    }
  }
  dc = sqrt(dc);

  /* write results */
  sprintf(l, "Chs %d to %d,  Area:", loch, hich);
  wrresult(l, area, sqrt(sum), 0);
  strcat(l, "   Cent:");
  wrresult(l, cent, dc, 0);
  if (!energy(cent, dc, &eg, &deg)) {
    strcat(l, "   Energy:");
    wrresult(l, eg, deg, 0);
  }
  printf("%s\n", l);
  if (repeat) goto REPEAT;
  return 0;
} /* sumcts */

/* ======================================================================= */
int sumcts_jch(void)
{
  float area, area1, area2, cent, x=0, y, b[111], save[16384];
  float dc, eg, deg, fnc, cou, cts, sum, r1;
  int   isav, i, j, isave, j2, nc, lo, hi, loch, hich;
  int   npks2;
  char  ans[80], l[120];

  if (!gf3gd.disp) {
    printf("Bad command: New spectrum not yet displayed...\n");
    return 1;
  }
  printf("Use mouse buttons or any character to enter limits...\n"
	 "                      ...T to type limits, X to exit.\n");

  while (1) {
    /* get limits for integration, background */
    while (1) {
      if (ans[0] == 'T' || ans[0] == 't') {
	nc = cask("Limits for integration = ? (rtn to quit)", ans, 80);
	if (nc == 0) return 0;
	if (!inin(ans, nc, &loch, &hich, &j2) &&
	    loch >= 0 && hich > 0 &&
	    loch <= gf3gd.maxch &&
	    hich <= gf3gd.maxch) break;
      } else {
	retic(&x, &y, ans);
	if (ans[0] == 'X' || ans[0] == 'x') return 0;
	if (ans[0] == 'T' || ans[0] == 't') continue;
	loch = x;
	retic(&x, &y, ans);
	if (ans[0] == 'X' || ans[0] == 'x') return 0;
	if (ans[0] == 'T' || ans[0] == 't') continue;
	hich = x;
	break;
      }
    }

    if (loch > hich) {
      isav = hich;
      hich = loch;
      loch = isav;
    }

    isave = gf3gd.mch[0];
    gf3gd.mch[0] = loch;
    dspmkr(1);
    gf3gd.mch[0] = hich;
    dspmkr(1);
    gf3gd.mch[0] = isave;

    sum = 0.f;
    area1 = area2 = 0.f;
    cent = 0.f;
    dc = 0.f;
    fnc = (float) (hich - loch);
    if (fnc < .5f) fnc = 1.f;
    /* ----------------------------------------------------- */

    /* figure out what peaks are included in integration region */
    for (i = 0; i < 6; ++i) {
      b[i] = 0.f;
    }
    b[3] = gf3gd.pars[3];
    b[4] = gf3gd.pars[4];

    npks2 = 0;
    for (j = 0; j < gf3gd.npks; j++) {
      if (gf3gd.pars[3*j+6] > loch && gf3gd.pars[3*j+6] < hich) {
	for (i = 6; i < 9; ++i) {
	  b[npks2*3 +i] = gf3gd.pars[j*3 + i];
	}
	npks2++;
      }
    }
    if (npks2 < 0) {
      printf("*** ERROR - no fitted peak included in region!\n");
      return 1;
    }
    printf("  Chs %d to %d;  %d fitted peaks\n", loch, hich, npks2);

    lo = gf3gd.mch[0];
    hi = gf3gd.mch[1];
    if (hi < lo) return 1;
    if (hi < hich || lo > loch) {
      printf("*** ERROR - background not fitted for all of this region!\n");
      return 1;
    }

    /* calculate background */
    eval(gf3gd.pars, 0, &y, gf3gd.npks, -9);
    for (i = lo; i <= hi; i++) {
      eval(gf3gd.pars, i, &y, gf3gd.npks, -1);
      save[i] = y;
    }

    /* eval(b, gf3gd.ifixed[3], &y, 1, -9); */
    eval(b, 0, &y, npks2, -9);
    for (i = lo; i <= hi; ++i) {
      if (i < loch || i > hich) {
	eval(b, i, &cou, npks2, 0);
	area2 += cou;
      } else {
	cou  = gf3gd.spec[i] - save[i];
	area1 += cou;
      }
      cent += cou * (float) (i-loch);
    }
    area = area1 + area2;

    if (area <= 0.f) {
      printf("  Chs %d to %d    Area = %.0f\n", loch, hich, area);
      continue;
    }

    cent = cent / area + (float) (loch);
    for (i = lo; i <= hi; ++i) {
      if (i < loch || i > hich) {
	eval(b, i, &cou, npks2, 0);
	/* r1 = gf3gd.errs[3*ipk+8]/gf3gd.pars[3*ipk+8]; */
	/* cts = cou * cou * 4.0f * (r1 * r1); */
 	cts = cou;
      } else if (gf3gd.wtmode < 1) {
	/* weight data with data */
	cts = gf3gd.spec[i];
      } else {
	/* weight data with WTSP */
	cts = gf3gd.wtsp[i];
      }
      if (cts < 1.f) cts = 1.f;
      sum += cts;
      r1 = ((float) i - cent) / area;
      dc += cts * (r1 * r1);
    }
    dc = sqrt(dc);

    /* ----------------------------------------------------- */

    /* write results */
    sprintf(l, "Chs %d to %d,  Area (Int, Tails, Total): %d %d",
	    loch, hich, (int) area1, (int) area2);
    wrresult(l, area, sqrt(sum), 0);
    printf("%s\n", l);
    strcpy(l, "   Cent:");
    wrresult(l, cent, dc, 0);
    if (!energy(cent, dc, &eg, &deg)) {
      strcat(l, "   Energy:");
      wrresult(l, eg, deg, 0);
    }
    printf("%s\n", l);
  }
} /* sumcts_jch */

/* ======================================================================= */
int list_params(int mode)
{
  /* print results of fit - parameters etc. */
  float eg, deg;
  int   i, k;
  char  l[120];

  if (mode == 1) {
    printf(" Mkr chs: limits %5d %5d\n           peaks",
	   gf3gd.mch[0], gf3gd.mch[1]);
    if (prfile) fprintf(prfile, " Mkr chs: limits %5d %5d\n           peaks",
			gf3gd.mch[0], gf3gd.mch[1]);
    for (i = 0; i < gf3gd.npks; i++) {
      printf(" %.2f", gf3gd.ppos[i]);
      if (prfile) fprintf(prfile, " %.2f", gf3gd.ppos[i]);
    }
    printf("\n");
    if (prfile) fprintf(prfile, "\n");
  }
  strcpy(l, " Background: A =");
  wrresult(l, gf3gd.pars[0], gf3gd.errs[0], 0);
  strcat(l, ",  B =");
  wrresult(l, gf3gd.pars[1], gf3gd.errs[1], 0);
  strcat(l, ",  C =");
  wrresult(l, gf3gd.pars[2], gf3gd.errs[2], 0);
  printf("%s\n", l);

  strcpy(l, "      Shape: R =");
  wrresult(l, gf3gd.pars[3], gf3gd.errs[3], 0);
  strcat(l, ",  Beta =");
  wrresult(l, gf3gd.pars[4], gf3gd.errs[4], 0);
  strcat(l, ",  Step =");
  wrresult(l, gf3gd.pars[5], gf3gd.errs[5], 0);
  printf("%s\n", l);
  if (prfile) fprintf(prfile, "%s\n", l);

  if (gf3gd.ical == 0) {
     printf("   position      width       height"
	    "        area          centroid\n");
     if (prfile) fprintf(prfile, "   position      width       height"
			 "        area          centroid\n");
  } else {
     printf("   position      width       height"
	    "        area          centroid      energy\n");
     if (prfile) fprintf(prfile, "   position      width       height"
			 "        area          centroid      energy\n");
  }
  for (i = 0; i < gf3gd.npks; ++i) {
    k = i*3 + 6;
    *l= '\0';
    wrresult(l, gf3gd.pars[k], gf3gd.errs[k], 14);
    wrresult(l, gf3gd.pars[k+1], gf3gd.errs[k+1], 26);
    wrresult(l, gf3gd.pars[k+2], gf3gd.errs[k+2], 40);
    wrresult(l, gf3gd.areas[i], gf3gd.dareas[i], 54);
    wrresult(l, gf3gd.cents[i], gf3gd.dcents[i], 68);
    if (!energy(gf3gd.cents[i], gf3gd.dcents[i], &eg, &deg)) {
      wrresult(l, eg, deg, 0);
    }
    printf("%2d%s\n", i+1, l);
    if (prfile) fprintf(prfile, "%2d%s\n", i+1, l);
  }
  return 0;
} /* list_params */

/* ======================================================================= */
int weight(int weightmode)
{
  /* change weighting mode */

  int  k, numch, j1, j2;
  char ans[80];

  if (weightmode < 1 || weightmode > 3) {
    while ((k = cask("WMODE = 1 : weight fit with results of fit\n"
		     "        2 : weight fit with data\n"
		     "        3 : weight fit with another spectrum\n"
		     "    WMODE = ?", ans, 1)) &&
	   (inin(ans, k, &weightmode, &j1, &j2) ||
	    weightmode <1 || weightmode > 3));
    if (k == 0) return 0;
  }
  gf3gd.wtmode = weightmode - 2;
  if (gf3gd.wtmode < 1) return 0;
  while (!cask("Weighting spectrum file or ID = ?", ans, 80) ||
	 readsp(ans, gf3gd.wtsp, gf3gd.nwtsp, &numch, 16384));
  if (numch != gf3gd.maxch + 1)
    printf("WARNING -- no. of chs in weight spectrum is different from\n"
	   "           no. of chs in fitted spectrum.\n");
  return 0;
} /* weight */

/* ======================================================================= */
int wrtlook(void)
{
  /* write out look-up table to disk file */

  int  rl = 0;
  char buf[32];

  rewind(winfile);
  /* WRITE (13,ERR=800) NCLOOK,LOOKMIN,LOOKMAX
     WRITE (13,ERR=800) (LOOKTAB(I),I=1,NCLOOK) */
#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(&gf3gd.nclook, 4);
  W(&gf3gd.lookmin, 4);
  W(&gf3gd.lookmax, 4);
  if (put_file_rec(winfile, buf, rl) ||
      put_file_rec(winfile, gf3gd.looktab, 2*gf3gd.nclook)) {
    printf("Error: cannot write to look-up file.\n");
    return 1;
  }
  return 0;
} /* wrtlook */

/* ======================================================================= */
int wrtsp(char *ans, int nc)
{
  /* write spectrum in spec array to disk file
     default file extension = .spe */

  int  iext, i2, c1 = 1, rl = 0;
  char fn[80], buf[32];
  FILE *file;

  strncpy(ans, "  ", 2);
  strncpy(fn, ans, 80);
  if (nc < 3) {
    /* ask for output file name */
    nc = cask(" Name of output file = ?", fn, 80);
    if (nc == 0) return 0;
  }

  /* open output file */
  iext = setext(fn, ".spe", 80);
  if (!(file = open_new_file(fn, 0))) return 1;
  strncpy(gf3gd.filnam, fn, 80);

  if (!strcmp(fn+iext, ".dat")) {
    /* user specified .dat file; use simplified float stream instead of .spe format */
    fwrite(gf3gd.spec, sizeof(float), gf3gd.maxch + 1, file);
    fclose(file);
    return 0;
  }

  /* else use .spe format */
  
  /* ask for spectrum name */
  nc = cask("Spectrum name = ? (rtn for same as file name)", ans, 80);
  if (nc > 0) {
    strncpy(gf3gd.namesp, ans, 8);
  } else {
    strncpy(gf3gd.namesp, fn, 8);
    nc = iext;
  }
  if (nc < 8) memset(&gf3gd.namesp[nc], ' ', 8-nc);

  /* write out spectrum */
  i2 = gf3gd.maxch + 1;
  W(gf3gd.namesp,8); W(&i2,4); W(&c1,4); W(&c1,4); W(&c1,4);
#undef W
  if (put_file_rec(file, buf, rl) ||
      put_file_rec(file, gf3gd.spec, 4*i2)) {
    file_error("write to", fn);
    fclose(file);
    return 1;
  }
  fclose(file);
  return 0;
} /* wrtsp */

/* ======================================================================= */
int report_curs(int ix, int iy)
{
  float  x=0, y=0, e, de;

  cvxy(&x, &y, &ix, &iy, 2);
  if (energy(x, 0.0, &e, &de)) {
    printf("\rCh = %.1f, y = %.1f         ", x, y);
  } else {
    printf("\rCh = %.1f, E = %.1f, y = %.1f         ", x, e, y);
  }
  fflush(stdout);
  return 0;
}
/* ======================================================================= */
int done_report_curs(void)
{
  printf("\r                                          \r");
  fflush(stdout);
  return 0;
}
