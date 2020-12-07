#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "minig.h"

/* global data */
struct {
  double gain[6];                           /* energy calibration */
  int    ical, nterms;
  int    disp, loch, hich, locnt, nchs;     /* spectrum display stuff */
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
} gf3gd;

struct {
  float pars[3], errs[3];
  int freepar[3], npars, nfp, mch[8], nreg, ready;
} sfgd;

extern FILE *scrfilea, *scrfileb;

int sf_dspfit(void);
int sf_dspmkr(int k);
int sf_eval(float *pars, int ii, float *fit, float *derivs, int mode);
int sf_fitter(int maxits);
int sf_parset(void);
int sfalt(char *cmd, int idata);
int sfhelp(char *ans);
int sfinit(void);
int sfset(void);
int sf_list_params(int imode);

/* in gf3_subs.c: */
extern int encal(char *fn, int nc);
extern int energy(float x, float dx, float *eg, float *deg);
extern int gfexec(char *, int);
extern int gfhelp(char *ans);
extern int gfinit(int argc, char **argv);
extern int wrtsp(char *ans, int nc);

/* ======================================================================= */
int main(int argc, char **argv)
{
  static char fmt[] =
    "\n\n\n   WELCOME TO STATFT\n\n"
    " This program fits portions of spectra with the function\n"
    "        Counts = C * (Egamma ** N) * EXP(-Egamma/T)\n\n"
    " The fitted parameters are :\n"
    "     C :   The height of the fit,\n"
    "     N :   Power of Egamma,\n"
    "     T :   Effective Temperature  ( in MeV ).\n\n"
    " RETURN as an answer to any (Y/N) question is equivalent to N\n"
    "    The default extension for all spectrum file names is .spe\n"
    "    Type HE   to type a list of available commands\n"
    "         HE/P to print a list of available commands.\n\n"
    "    D.C. Radford    Feb. 1989.\n\n"
    "  Press any key to continue...";


  float save[16384], derivs[3];
  int   i, k, idata, j1, j2, nc, in, maxits, in2;
  char  ans[80];

  /* program to fit statistical gammas in spectra */
  /* with function    c * (egamma**n) * exp(-egamma/t) */
  sfgd.npars = 3;
  sfgd.ready = 0;
  gf3gd.ical = 2;
  gf3gd.gain[0] = 0.0;
  gf3gd.gain[1] = 0.5;
  gf3gd.nterms = 2;

  /* welcome and get initial data */
  cask(fmt, ans, 1);
  gfinit(argc, argv);
  encal(ans, 0);

  /* get new command */
  while (1) {
    if ((nc = cask("?", ans, 80)) > 1) {
      /* convert lower case to upper case characters */
      for (i = 0; i < 3; ++i) {
	ans[i] = toupper(ans[i]);
      }

      /* execute statft commands */
      idata = 0;
      if (!strncmp(ans, "SFT", 3)) {
	/* get limits etc. and/or do fit */
	if (inin(ans + 3, nc - 3, &idata, &in, &in2) ||
	    idata > 3) goto ERR;
	if (idata > 0) {
	  sfgd.nreg = idata;
	  /* get limits, peak positions and fixed parameters */
	  sfset();
	  sfgd.ready = 1;
	} else {
	  /* reset initial parameter estimates */
	  if (idata < 0) sf_parset();
	  if (!sfgd.ready ||
	      sfgd.mch[(sfgd.nreg << 1) - 1] > gf3gd.maxch) goto ERR;
	}
	while ((k = cask(" Max. no. of iterations = ? (rtn for 50)",
			 ans, 80)) &&
	       inin(ans, k, &maxits, &j1, &j2));
	if (k == 0) maxits = 50;
	if (maxits <= 0) continue;
	/* do fit */
	if (sf_fitter(maxits) == 1) continue;
	/* display fit and difference and list parameters */
	if (gf3gd.disp) sf_dspfit();
	sf_list_params(2);
      } else if (!strncmp(ans, "SLP", 3)) {
	/* list pars */
	sf_list_params(1);
      } else if (!strncmp(ans, "SFX", 3) ||
		 !strncmp(ans, "SFR", 3) ||
		 !strncmp(ans, "SMA", 3)) {
	if (inin(ans + 3, nc - 3, &idata, &in, &in2)) goto ERR;
	sfalt(ans, idata);
	
      } else if (!strncmp(ans, "SDM", 3)) {
	/* display markers */
	if (!(gf3gd.disp && sfgd.ready)) goto ERR;
	sf_dspmkr(99);
      } else if (!strncmp(ans, "SDF", 3)) {
	/* display fit */
	if (!gf3gd.disp) goto ERR;
	sf_dspfit();
      } else if (!strncmp(ans, "HE", 2)) {
	sfhelp(ans);
      } else if (!strncmp(ans, "WF", 2)) {
	/* write fit to disk */
	for (i = 0; i < gf3gd.maxch + 1; ++i) {
	  save[i] = gf3gd.spec[i];
	  sf_eval(sfgd.pars, i, &gf3gd.spec[i], derivs, 0);
	}
	wrtsp(ans, nc);
	for (i = 0; i < gf3gd.maxch + 1; ++i) {
	  gf3gd.spec[i] = save[i];
	}
      } else {
	/* look for and execute accepted gf3 command */
      ERR:
	gfexec(ans, nc);
      }
    }
  }
} /* main */

/* ====================================================================== */
int sf_dspfit(void)
{
  float y, x1, derivs[3], fdx;
  int   i, hi, lo, dx, nx, ny;

/* display fit */
  lo = gf3gd.loch;
  hi = gf3gd.hich;
  dx = (hi - lo + 1) / 256;
  if (dx < 1) dx = 1;
  fdx = (float) dx;
  sf_eval(sfgd.pars, lo, &y, derivs, 0);
  x1 = (float) lo + 0.5f;
  initg(&nx, &ny);
  pspot(x1, y);
  for (i = lo + dx; i <= hi; i += dx) {
    x1 += fdx;
    sf_eval(sfgd.pars, i, &y, derivs, 0);
    vect(x1, y);
  }
  finig();
  return 0;
} /* sf_dspfit */

/* ====================================================================== */
int sf_dspmkr(int k)
{
  float x, y;
  int   i, nx, ny, ich;

  i = k;
  if (k == 99) i = 1;
  initg(&nx, &ny);
  while (i <= 2*sfgd.nreg) {
    ich = sfgd.mch[i - 1];
    if (ich >= gf3gd.loch && ich <= gf3gd.hich) {
      x = (float) ich + 0.5f;
      y = (float) gf3gd.locnt;
      pspot(x, y);
      vect(x, gf3gd.spec[ich]);
    }
    if (k != 99) break;
    ++i;
  }
  finig();
  return 0;
} /* sf_dspmkr */

/* ====================================================================== */
int sf_eval(float *pars, int ii, float *fit, float *derivs, int mode)
{
  float u, w, eg, deg;

/* this eval is for use with 'statft' version ii */
/* D.C. Radford     Nov. 1985 */
/* calculate the fit using present values of the pars */
  eg = 0.0f;
  energy((float) ii, 0.0f, &eg, &deg);
  if (eg < 0.0f) eg = 0.0f;
  w = 0.0f;
  u = 0.0f;
  eg /= 1e3f;
  if (eg > 0.01f) {
    w = log(eg);
    u = exp(pars[1] * w - eg / pars[2]);
  }
  *fit = pars[0] * u;
  /* calculate derivs only for mode.ge.1 */
  if (mode > 0) {
    derivs[0] = u;
    derivs[1] = *fit * w;
    derivs[2] = *fit * eg / (pars[2] * pars[2]);
  }
  return 0;
} /* sf_eval */

/* ====================================================================== */
int sf_fitter(int maxits)
{
  static char wtc [3][16] = { "fit.", "data.", "sp.         " };
  static char fmt_420[] =
    "  File %s  Spectrum %s     %s\n"
    " %d indept. pars, %d degrees of freedom.   Weighted with %s\n";
  static char fmt_440[] = " %i Iterations.    Chisq/d.o.f.= %6.3f\n";
  static char fmt_500[] =
    "  Failed to converge after %d iterations\n"
    "    Chisq/d.o.f.= %6.3f\n"
    "    WARNING - do not believe quoted errors\n";

  double ddat, alpha[3][3], array[3][3];
  float diff, beta[3], b[3], delta[3], chisq = 0.0f;
  float chisq1, flamda, derivs[3], dat, fit, ers[3];
  int   ireg, ilo, ihi, nip, conv, nits, test, i, j, k, l, m, nextp[3], ndf;
  char  dattim[20];

  /* this subroutine is a modified version of 'curfit', in Bevington */
  /* see page 237 */

  /* nip=no. of independent (non-fixed) pars */
  /* nfp=no. of fixed pars */
  /* npars=total no. of pars = 3 */
  /* ndf=no. of degrees of freedom */
  if (gf3gd.wtmode > 0) strncpy(wtc[2] + 4, gf3gd.nwtsp, 8);
  nip = sfgd.npars - sfgd.nfp;
  ndf = -nip;
  for (i = 0; i < sfgd.nreg; ++i) {
    ndf += sfgd.mch[2*i + 1] - sfgd.mch[2*i] + 1;
  }
  if (ndf < 1) {
    printf("No d.o.f.\n");
    return 1;
  }
  if (nip < 2) {
    printf("Too many fixed pars.\n");
    return 1;
  }
  k = 0;
  for (j = 0; j < sfgd.npars; ++j) {
    if (sfgd.freepar[j]) nextp[k++] = j;
  }
  if (k != nip) {
    printf("Ackkk! nip != sum(freepar)\n");
    return 1;
  }
  flamda = 0.001f;
  nits = 0;
  test = 0;
  derivs[0] = 1.0f;
  for (i = 0; i < sfgd.npars; ++i) {
    sfgd.errs[i] = 0.0f;
    b[i] = sfgd.pars[i];
  }

  /* evaluate fit, alpha & beta matrices, & chisq */
 NEXTIT:
  for (j = 0; j < nip; ++j) {
    beta[j] = 0.0f;
    for (k = 0; k <= j; ++k) {
      alpha[k][j] = 0.0f;
    }
  }
  chisq1 = 0.0f;
  for (ireg = 0; ireg < sfgd.nreg; ++ireg) {
    ilo = sfgd.mch[2*ireg];
    ihi = sfgd.mch[2*ireg + 1];
    for (i = ilo; i <= ihi; ++i) {
      sf_eval(sfgd.pars, i, &fit, derivs, 1);
      diff = gf3gd.spec[i] - fit;
/* weight with fit/data/weight sp. for wtmode=-1/0/1 */
      if (gf3gd.wtmode < 0) {
	dat = fit;
      } else if (gf3gd.wtmode == 0) {
	dat = gf3gd.spec[i];
      } else {
	dat = gf3gd.wtsp[i];
      }
      if (dat < 1.0f) dat = 1.0f;
      ddat = (double) dat;
      chisq1 += diff * diff / dat;
      for (l = 0; l < nip; ++l) {
	j = nextp[l];
	beta[l] += diff * derivs[j] / dat;
	for (m = 0; m <= l; ++m) {
	  alpha[m][l] += (double) derivs[j] * (double) derivs[nextp[m]] / ddat;
	}
      }
    }
  }
  chisq1 /= (float) ndf;
  
  /* invert modified curvature matrix to find new parameters */
 INVMAT:
  array[0][0] = flamda + 1.0f;
  for (j = 1; j < nip; ++j) {
    for (k = 0; k < j; ++k) {
      if (alpha[j][j] * alpha[k][k] == 0.0) {
	printf("Cannot - diag. element = 0.0\n");
	return 1;
      }
      array[k][j] /= sqrt(alpha[j][j] * alpha[k][k]);
      array[j][k] = array[k][j];
    }
    array[j][j] = flamda + 1.0f;
  }
  matinv(array[0], nip, 3);
  if (!test) {
    for (j = 0; j < nip; ++j) {
      if (alpha[j][j] * alpha[j][j] == 0.0) {
	printf("Cannot - diag. element = 0.0\n");
	return 1;
      }
      delta[j] = 0.0f;
      for (k = 0; k < nip; ++k) {
	delta[j] += beta[k] * array[k][j] / sqrt(alpha[j][j] * alpha[k][k]);
      }
    }
    /* if chisq increased, increase flamda & try again */
    chisq = 0.0f;
    for (l = 0; l < nip; ++l) {
      b[nextp[l]] = sfgd.pars[nextp[l]] + delta[l];
    }
    for (ireg = 0; ireg < sfgd.nreg; ++ireg) {
      ilo = sfgd.mch[2*ireg];
      ihi = sfgd.mch[2*ireg + 1];
      for (i = ilo; i <= ihi; ++i) {
	sf_eval(b, i, &fit, derivs, 0);
	diff = gf3gd.spec[i] - fit;
	/* weight with fit/data/weight sp. for wtmode=-1/0/1 */
	if (gf3gd.wtmode < 0) {
	  dat = fit;
	} else if (gf3gd.wtmode == 0) {
	  dat = gf3gd.spec[i];
	} else {
	  dat = gf3gd.wtsp[i];
	}
	if (dat < 1.0f) dat = 1.0f;
	chisq += diff * diff / dat;
      }
    }
    chisq /= (float) ndf;
    if (chisq > chisq1) {
      flamda *= 10.0f;
      goto INVMAT;
    }
  }

  /* evaluate parameters and errors */
  /* test for convergence */
  conv = 1;
  for (j = 0; j < nip; ++j) {
    if (array[j][j] < 0.0) array[j][j] = 0.0;
    ers[j] = sqrt(array[j][j] / alpha[j][j]) * sqrt(flamda + 1.0f);
    if (fabs(delta[j]) > ers[j] / 100.0f) conv = 0;
  }
  if (!test) {
    for (j = 0; j < sfgd.npars; ++j) {
      sfgd.pars[j] = b[j];
    }
    flamda /= 10.0f;
    ++nits;
    if (!conv && nits < maxits) goto NEXTIT;
    /* re-do matrix inversion with flamda=0 */
    /* to calculate errors */
    flamda = 0.0f;
    test = 1;
    goto INVMAT;
  }

  /* list data and exit */
  for (l = 0; l < nip; ++l) {
    sfgd.errs[nextp[l]] = ers[l];
  }
  datetime(dattim);
  printf(fmt_420, gf3gd.filnam, gf3gd.namesp, dattim, nip, ndf,
	 wtc[gf3gd.wtmode + 1]);
  if (conv) {
    printf(fmt_440, nits, chisq);
    return 0;
  }
  printf(fmt_500, nits, chisq);
  return 2;
} /* sf_fitter */

/* ====================================================================== */
int sf_parset(void)
{
  float derivs[3], fit;
  int   i, lo;

  lo = sfgd.mch[0];
  for (i = 0; i < 3; ++i) {
    sfgd.errs[i] = 0.0f;
  }
  if (sfgd.freepar[1] == 1) sfgd.pars[1] = 3.0f;
  if (sfgd.freepar[2] == 1) sfgd.pars[2] = 0.5f;
  if (sfgd.freepar[0] < 1) return 0;
  sfgd.pars[0] = 1.0f;
  sf_eval(sfgd.pars, lo, &fit, derivs, 0);
  sfgd.pars[0] = gf3gd.spec[lo] / fit;
  return 0;
} /* sf_parset */

/* ====================================================================== */
int sfalt(char *cmd, int idata)
{
  float x, y, rj1, rj2, val;
  int   i, k, n, itest, j1, j2, nc, mn;
  char  under, ans[80];

  if (!strncmp(cmd, "SMA", 3)) {
    /* change marker positions */
    mn = idata;
    itest = 0;
    if (idata <= 0 || idata > sfgd.nreg << 1) {
      if (!caskyn("Change limits? (Y/N)")) return 0;
      mn = 1;
    } else {
      itest = 1;
    }
    while (1) {
      printf("New marker position? (type T for type)\n");
      if (gf3gd.disp) {
	retic(&x, &y, ans);
      } else {
	*ans = 'T';
	x = sfgd.mch[mn - 1];
      }
      if ((*ans == 'T' || *ans == 't')  &&
	  (k = cask("New position = ? (rtn for old value)", ans, 80))) {
	if (ffin(ans, k, &x, &rj1, &rj2)) {
	  printf("**** Bad value - try again. ****\n");
	  continue;
	}
	x += 0.5f;
	if (x < 0.5f || (int) x > gf3gd.maxch) {
	  printf("Mkr ch outside spectrum - try again.\n");
	  continue;
	}
      }
      sfgd.mch[mn - 1] = x;
      if (gf3gd.disp) sf_dspmkr(mn);
      if (itest || ++mn > 2*sfgd.nreg) break;
    }
    if (caskyn("Reset free pars. to initial estimates? (Y/N)")) sf_parset();
    return 0;
  }

  /* change fixed parameters */
  n = idata;
  itest = 0;
  if (n > 0 && n <= sfgd.npars) {
    itest = 1;
    if (!strncmp(cmd, "SFR", 3)) {
      sfgd.nfp = sfgd.nfp - 1 + sfgd.freepar[n - 1];
      sfgd.freepar[n - 1] = 1;
    } else {
      sfgd.nfp += sfgd.freepar[n - 1];
      sfgd.freepar[n - 1] = 0;
      while (1) {
	if (!(k = cask("Value = ? (rtn for present value)", ans, 80))) break;
	if (ffin(ans, k, &val, &rj1, &rj2)) continue;
	if (val != 0.0f || n == 2) break;
	printf("Value must be nonzero\n");
      }
      if (k) sfgd.pars[n - 1] = val;
    }
    return 0;
  }

  for (i = 0; i < sfgd.npars; ++i) {
    under = ' ';
    if (!sfgd.freepar[i]) under = '*';
    printf("%3i%c", i+1, under);
  }
  printf("\n  C   N   T\n");

  if (!strncmp(cmd, "SFR", 3)) {
    printf("Parameter numbers to be freed =? (one per line, rtn to end)\n");
    while ((nc = cask(">", ans, 80))) {
      if (inin(ans, nc, &n, &j1, &j2) ||
	  n < 1 ||
	  n > sfgd.npars) {
	printf("Ooops, try again.\n");
	continue;
      }
      sfgd.nfp = sfgd.nfp - 1 + sfgd.freepar[n - 1];
      sfgd.freepar[n - 1] = 1;
    }
  } else {
    printf("Parameter numbers to be fixed =? (one per line, rtn to end)\n");
    while ((nc = cask(">", ans, 80))) {
      if (inin(ans, nc, &n, &j1, &j2) ||
	  n < 1 ||
	  n > sfgd.npars) {
	printf("Ooops, try again.\n");
	continue;
      }
      sfgd.nfp += sfgd.freepar[n - 1];
      sfgd.freepar[n - 1] = 0;
      while (1) {
	if (!(k = cask("Value = ? (rtn for present value)", ans, 80))) break;
	if (ffin(ans, k, &val, &rj1, &rj2)) continue;
	if (val != 0.0f || n == 2) break;
	printf("Value must be nonzero\n");
      }
      if (k) sfgd.pars[n - 1] = val;
    }
  }

  return 0;
} /* sfalt */

/* ====================================================================== */
int sfhelp(char *ans)
{
  static char fmt_5[] =
    "\n  Special statft commands available:\n"
    " SFT [N]   N=1-3 : set-up for n regions and do fit\n"
    "           [N=0] : do fit using present parameter values\n"
    "           N=-1  :  recalculate initial parameter estimates and do fit\n"
    " SFX [N]   fix additional parameters [no. N] or change fixed value(s)\n"
    " SFR [N]   free additional parameters [par. no. N]\n"
    " SMA [N]   change limits for fit\n"
    "           [N=1-6 : change marker no. N]\n"
    " SDM       display markers\n"
    " SDF       display fit for present parameter values\n"
    " SLP       list parameter values\n"
    " WF FN     write fit to disk as a spectrum   (FN = file name)\n\n";
  char  fn[80] = "statft.hlp";
  FILE *file;

  if (!strncmp(ans, "HE/P", 4) || !strncmp(ans, "HE/p", 4)) {
/* open output (print) file */
    if ((file = open_new_file(fn, 0))) {
      fprintf(file, "%s", fmt_5);
      fclose(file);
      pr_and_del_file(fn);
    }
    return 0;
  }
  printf("%s", fmt_5);
  (void) cask("  Press return when done....", ans, 1);
  return gfhelp(ans);
} /* sfhelp */

/* ====================================================================== */
int sfset(void)
{
  float x, y, ch, rj1, rj2;
  int   i, k, n, idata, nn, lo;
  char  ans[80];

/* ask for limits for fit */
  printf("Limits for fit?  (hit T to type)\n");
  for (nn = 0; nn < sfgd.nreg; ++nn) {
    for (n = 2*nn; n <= 2*nn + 1; ++n) {
      if (gf3gd.disp) {
	retic(&x, &y, ans);
	if (*ans != 'T' && *ans != 't') {
	  sfgd.mch[n] = x;
	  continue;
	}
      }
      while (1) {
	k = cask("Limit = ?", ans, 80);
	if (ffin(ans, k, &ch, &rj1, &rj2)) continue;
	sfgd.mch[n] = ch + 0.5f;
	if (sfgd.mch[n] < gf3gd.maxch &&
	    sfgd.mch[n] >= 0) break;
	printf("Mkr ch outside spectrum - try again.\n");
      }
    }
    if (sfgd.mch[2*nn + 1] < sfgd.mch[2*nn]) {
      lo = sfgd.mch[2*nn + 1];
      sfgd.mch[2*nn + 1] = sfgd.mch[2*nn];
      sfgd.mch[2*nn] = lo;
    }
    if (gf3gd.disp) {
      sf_dspmkr(2*nn + 1);
      sf_dspmkr(2*nn + 2);
    }
  }
  /* ask for fixed pars */
  for (i = 0; i < sfgd.npars; ++i) {
    sfgd.freepar[i] = 1;
  }
  sfgd.nfp = 0;
  sf_parset();
  idata = 0;
  sfalt("SFX", idata);
  return 0;
} /* sfset */

/* ====================================================================== */
int sf_list_params(int imode)
{
  static char fmt_30[] =
    "  Parameters: C = %.2f +- %.2f\n"
    "              N = %.4f +- %.4f\n"
    "              T = %.5f +- %.5f\n";
  int i;

  if (imode < 2) {
    printf("Mkr chs:");
    for (i = 0; i < 2*sfgd.nreg; i += 2) {
      printf("  %d to %d", sfgd.mch[i], sfgd.mch[i+1]);
    }
    printf("\n");
  }
  printf(fmt_30,
	 sfgd.pars[0], sfgd.errs[0],
	 sfgd.pars[1], sfgd.errs[1],
	 sfgd.pars[2], sfgd.errs[2]);
  return 0;
} /*  sf_list_params */
