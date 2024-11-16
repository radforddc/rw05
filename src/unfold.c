/* version 0.0   D.C. Radford    July 1999 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

/* global data */

extern struct {
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

#include "util.h"
#include "minig.h"

float resp[25][514], renorma = 1.0f, renormb = 0.0f, thresh;

/* ==== routines from gf3_subs ==== */
double channo(float egamma);
int adjgain(int mode, float *oldch1, float *oldch2, float *newch1, float *newch2,
	    float *oldsp, float *newsp, int nco, int ncn, float fact);
int dspmkr(int mkrnum);
int dspsp(int in1, int in2, int in3);                 /* returns 1 on error */
int encal(char *fn, int nc);
int energy(float x, float dx, float *eg, float *deg); /* returns 1 on error */
int eval(float *pars, int ichan, float *fit, int npks, int mode);
int gfexec(char *, int);
int gfhelp(char *ans);
int gfinit(int argc, char **argv);

/* ==== routines defined here ==== */
int unfoldsp(int, int);
int wrres(char *);
int calres(float *, float);
int ufexec(char *, int);
int ufhelp(char *);
int ufinit(void);
int stores(void);

/* ====================================================================== */
int main (int argc, char **argv)
{
  /* Main program */
  /* program to generate response functions and unfold spectra
     the program also fits ge spectra with up to fifteen
     peaks on a quadratic background
     Version 3.1    D. C. Radford    Oct. 1988.
     Version 4.0    D.C.Radford      April 1991 */

  int  nc;
  char ans[80];

  /* welcome and get initial data */
  ufinit();
  gfinit(argc, argv);

  /* get new command */
  while (1) {
    if ((nc=cask("?", ans, 80)) > 1)
      ufexec(ans, nc);  /* decode and execute command */
  }
} /* main */

/* ====================================================================== */
int ufexec(char *ans, int nc)
{
  float ppch, w, bg, rj1, rj2, fit;
  int   i, k, idata, j1, j2, lo, hi, lo1, ii, in, in2;

  /* convert lower case to upper case characters */
  for (i = 0; i < 2; ++i) {
    ans[i] = toupper(ans[i]);
  }
  idata = 0;

  /* execute unfold commands */
  if (!strncmp(ans, "S1", 2)) {
    /* subtract fitted peak number 1 from spectrum */
    if (gf3gd.npks < 1) goto BADCMD;
    eval(gf3gd.pars, 0, &fit, 1, -6);
    w = gf3gd.pars[7] * 7.0710678f + 0.5f;
    lo = gf3gd.pars[6] - w;
    hi = gf3gd.pars[6] + w;
    lo1 = gf3gd.pars[6] - gf3gd.pars[4] * 13.0f - 0.5f;
    if (lo > lo1) lo = lo1;
    if (lo < 0) lo = 0;
    if (hi > gf3gd.maxch) hi = gf3gd.maxch;
    for (i = lo; i <=  hi; ++i) {
	eval(gf3gd.pars, i, &bg, 1, -1);
	eval(gf3gd.pars, i, &fit, 1, 0);
	gf3gd.spec[i] = gf3gd.spec[i] - fit + bg;
    }
    strncpy(gf3gd.namesp+4, ".MOD", 4);

  } else if (!strncmp(ans, "SR", 2)) {
    /* adjust gain of sp. and store as response function */
    if (resp[0][513] != 0.f) {
      printf("CANNOT - already have 25 response functions stored.\n");
    } else {
      stores();
    }

  } else if (!strncmp(ans, "WR", 2) || !strncmp(ans, "RR", 2)) {
    /* write/read response functions to/from disk file */
    wrres(ans);

  } else if (!strncmp(ans, "DR", 2)) {
    /* delete stored resp. fn. # idata */
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
    while (idata < 1 || idata > 25 || resp[idata-1][513] <= 0.f) {
      printf("Stored response functions are:\n"
	     "   #    Egamma\n");
      for (i = 0; i < 25; ++i) {
	if (resp[i][513] > 0.f) printf("%5d %9.2f\n", i+1, resp[i][513]);
      }
      while (1) {
	if (!(k = cask("Delete response fn. number ?", ans, 80))) return 0;
	if (!inin(ans, k, &idata, &j1, &j2) &&
	    idata > 0 && idata <= 25) break;
      }
    }
    if (idata != 1) {
      for (i = idata-1; i > 0; --i) {
	for (ii = 0; ii < 514; ++ii) {
	    resp[i][ii] = resp[i-1][ii];
	}
      }
    }
    resp[0][513] = 0.f;

  } else if (!strncmp(ans, "RE", 2)) {
    /* set data to stored (changed) resp. fn. no. n (idata=-n)
       or calc. resp. fn. for fixed photopk. ch. (idata=ch.no.) */
    ffin(ans+2, nc-2, &ppch, &rj1, &rj2);
    if (ppch < 0.f || resp[23][513] != 0.f) {
      for (i = 0; i <= gf3gd.maxch; ++i) {
	gf3gd.spec[i] = 0.f;
      }
      calres(&ppch, 1e4f);
      char namesp[16];
      sprintf(namesp, "RE%6.0f", ppch);
      strncpy(gf3gd.namesp, namesp, 8);
      gf3gd.disp = 0;
    } else {
      printf("CANNOT -  not enough response functions.\n");
    }

  } else if (!strncmp(ans, "UF", 2)) {
    /* unfold spectrum */
    if (resp[23][513] != 0.f) {
      if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto BADCMD;
      unfoldsp(idata, in);
    } else {
      printf("CANNOT -  not enough response functions.\n");
    }

  } else if (!strncmp(ans, "RN", 2)) {
    /* get new renormalisation coefficients */
    while (1) {
      if (!(k = cask("Renorm. factor for photopk = A + B*x (x = ch. no.)\n"
		     " .. Enter A,B = ?", ans, 80))) return 0;
      if (!ffin(ans, k, &renorma, &renormb, &rj2)) break;
    }

  } else if (!strncmp(ans, "HE", 2)) {
    ufhelp(ans);

  } else {
  BADCMD:
    gfexec(ans, nc);
  }
  return 0;
} /* ufexec */

/* ====================================================================== */
int calres(float *ppch, float cts)
{
  float fact, ecut, xcut, f;
  float x, f1, f2, de, ee, el, egamma, dx = 0.0f;
  float rj1, rj2, ebs, esc, xhi, xlo, rnc1;
  float save[512];
  int   i, j, k, ich;
  char  ans[80];
  static float cch1 = -0.5f, cch2 = 99.5f, cch3 = 249.5f, cch4 = 129.5f;

  if (*ppch < 0.f) {
    /* set data to stored (changed) resp. fn. no. j */
    j = -0.5f - *ppch;
    if (j > 24) j = 24;
    for (i = 0; i < 510; ++i) {
      gf3gd.spec[i] = resp[j][i] * cts * 100.0f;
    }
    *ppch = resp[j][513];
    gf3gd.maxch = 509;
    printf("   Egamma = %.2f\n", *ppch);
    return 0;
  }

  while (*ppch == 0.0f) {
    while ((k = cask("Photopeak ch. = ?", ans, 80)) &&
	   ffin(ans, k, ppch, &rj1, &rj2));
    if (k == 0) return 0;
  }

  /* add resp. fn. * cts  to data */
  energy(*ppch, dx, &egamma, &de);
  for (j = 0; j < 24; ++j) {
    if (egamma < resp[j+1][513]) break;  /* resp[j][513] = egamma */
  }
  if (j > 23) j = 23;
  if (resp[j][513] == 0.0f) j++;
  if (j == 24) return 1;
  el = resp[j][513];
  f2 = cts * (log(egamma) - log(el)) / (log(resp[j+1][513]) - log(el));
  f1 = cts - f2;
  for (i = 0; i < 510; ++i) {
    save[i] = f1*resp[j][i] + f2*resp[j+1][i];
  }
  if (el - (el * 1.2f / (el / 255.503f + 1.0f)) <= thresh) {
    for (i = 0; i < 100; ++i) {
      save[i] = cts * resp[j+1][i];
    }
  }
  /* resp[j][513] = egamma
     resp[j][512] gives e1area
     resp[j][511] gives e2area */
  ebs = egamma / (1.0f + (egamma/255.503f));
  ecut = egamma - ebs * 1.2f;
  xcut = channo(ecut);
  xlo = channo(ebs * 0.8f);
  xhi = channo(ebs * 1.8f);
  if (xhi > *ppch * 0.99f) xhi = *ppch * 0.99f;
  if (thresh < ecut) {
    rnc1 = channo(thresh);
    fact = ecut - thresh;
    adjgain(1, &cch1, &cch2, &rnc1, &xcut, save, gf3gd.spec,
	    100, gf3gd.maxch+1, fact);
  }
  fact = egamma - ecut;
  adjgain(1, &cch1, &cch3, &xcut, ppch, save+100, gf3gd.spec,
	  280, gf3gd.maxch+1, fact);
  fact = ebs;
  adjgain(1, &cch1, &cch4, &xlo, &xhi, save+380, gf3gd.spec,
	  130, gf3gd.maxch+1, fact);

  /* calculate escape peaks */
  if (egamma < 1100.f) return 0;
  ee = log(log(egamma / 1022.0f)) * 2.935f;
  for (i = 1; i <= 2; ++i) {
    if (resp[24][513-i] == 0.0f) continue;
    x = channo(egamma - ((float) i) * 511.006f);
    ich = (int) x;
    f = x - (float) ich;
    esc = exp(resp[24][513-i] + ee) * cts;
    gf3gd.spec[ich] += (1.0f - f) * esc;
    gf3gd.spec[ich + 1] += f * esc;
  }
  return 0;
} /* calres */

/* ====================================================================== */
int stores(void)
{
  float fact, ppch, ecut, cnts, xcut, x, y, rppch, f1, f2;
  float e1area, e2area, x0, y1, y2, ee, el, egamma, pparea;
  float oc1, rj1, rj2, fnc, ebs, xhi, xlo, save[16384];
  int   i1, ncntssav, i, j, k, isave, i2, ii;
  int   loxsav, ihi, ilo, numxsav;
  char  ans[80];
  static float cch1 = -0.5f, cch2 = 99.5f, cch3 = 249.5f, cch4 = 129.5f;

  if (!caskyn("Have you subtracted the photopeak and noted its area,\n"
	      "         subtracted any 511keV peak and response,\n"
	      "         subtracted any escape peaks and noted the areas,\n"
	      "     and subtracted any X-ray peaks? (Y/N)")) {
    printf("Then do so and then try again!\n");
    return 1;
  }

  while ((k = cask("Photopeak ch. = ?", ans, 80)) &&
	 ffin(ans, k, &rppch, &rj1, &rj2));
  while ((k = cask("Photopeak area = ?", ans, 80)) &&
	 ffin(ans, k, &pparea, &rj1, &rj2));
  while ((k = cask("Egamma (in keV) = ?", ans, 80)) &&
	 ffin(ans, k, &egamma, &rj1, &rj2));
  e1area = 0.0f;
  e2area = 0.0f;
  if (egamma >= 1100.f) {
    while ((k = cask("Single escape peak area = ?", ans, 80)) &&
	   ffin(ans, k, &e1area, &rj1, &rj2));
    while ((k = cask("Double escape peak area = ?", ans, 80)) &&
	   ffin(ans, k, &e2area, &rj1, &rj2));
  }

  ppch = channo(egamma);
  ebs = egamma / (egamma / 255.503f + 1.f);
  ecut = egamma - ebs * 1.2f;
  x0 = channo(0.0);
  xcut = x0 + (channo(ecut) - x0) * (rppch - x0) / (ppch - x0);
  xlo = x0 + (channo(ebs * 0.8f) - x0) * (rppch - x0) / (ppch - x0);
  xhi = x0 + (channo(ebs * 1.8f) - x0) * (rppch - x0) / (ppch - x0);
  if (xhi > rppch * 0.99f) xhi = rppch * 0.99f;

  /* display spectrum around back-scatter peak */
  loxsav = gf3gd.lox;
  numxsav = gf3gd.numx;
  ncntssav = gf3gd.ncnts;
  gf3gd.lox = xlo * 2.f - xhi;
  if (gf3gd.lox < 0) gf3gd.lox = 0;
  if (gf3gd.lox + 2 > gf3gd.maxch) goto ERR;
  gf3gd.numx = (xhi - xlo) * 3.f;
  i1 = xlo;
  i2 = xhi;
  cnts = gf3gd.spec[i1];
  for (i = i1 + 1; i <= i2; ++i) {
    if (cnts < gf3gd.spec[i]) cnts = gf3gd.spec[i];
  }
  gf3gd.ncnts = cnts * 3.0f + 0.5f;

 START_BSP:
  erase();
  if (dspsp(0, 0, 0)) goto ERR;
  gf3gd.disp = 1;
  /* save spectrum */
  for (i = 0; i <= gf3gd.maxch; ++i) {
    save[i] = gf3gd.spec[i];
  }
  /* display cursor at back-scatter peak limits */
  /* set counts to background below b-s peak using cursor */
  isave = gf3gd.mch[0];
  gf3gd.mch[0] = xlo + .99f;
  dspmkr(1);
  gf3gd.mch[0] = xhi;
  dspmkr(1);
  gf3gd.mch[0] = (float) isave;
  printf("Use the cursor to define a (piecewise linear) background\n"
	 " for the backscatter peak between the shown limits.\n"
	 "Hit X to exit, or\n"
	 "    S to take backscatter peak from resp. fns. already stored.\n");
  while (1) {
    retic(&x, &y, ans);
    if (ans[0] == 'X' || ans[0] == 'x') break;
    if (ans[0] == 'S' || ans[0] == 's') {
      if (resp[23][513] != 0.f) goto USE_STORED_BSP;
      printf("Sorry - not enough response functions stored.\n");
      continue;
    }
    ilo = x;
    y1 = y;
    retic(&x, &y, ans);
    if (ans[0] == 'X' || ans[0] == 'x') break;
    if (ans[0] == 'S' || ans[0] == 's') {
      if (resp[23][513] != 0.f) goto USE_STORED_BSP;
      printf("Sorry - not enough response functions stored.\n");
      continue;
    }
    ihi = x;
    y2 = y;
    if (ilo > ihi) {
      isave = ihi;
      ihi = ilo;
      ilo = isave;
      y2 = y1;
      y1 = y;
    }
    gf3gd.spec[ilo] = (y1 + y2) / 2.f;
    if (ilo != ihi) {
      fnc = (float) (ihi - ilo);
      for (i = ilo; i <= ihi; ++i) {
	gf3gd.spec[i] = y1 + (y2 - y1) * (float) (i - ilo) / fnc;
      }
    }
    if (dspsp(0, 0, 0)) goto ERR;
    continue;

  USE_STORED_BSP:
    for (i = 0; i <= gf3gd.maxch; ++i) {
      gf3gd.spec[i] = save[i];
    }
    while (1) {
      cask("Press A (average) or I (interpolate) ?", ans, 1);
      if (ans[0] == 'A' || ans[0] == 'a') {
	for (j = 0; j < 24; ++j) {
	  if (egamma < resp[j+1][513]) break;
	}
	if (j > 23) j = 23;
	if (resp[j][513] == 0.0f) ++j;
	if (j == 24) return 0;
	el = resp[j][513];
	f2 = (log(egamma) - log(el)) / (log(resp[j+1][513]) - log(el));
	f1 = 1.0f - f2;
	for (i = 380; i < 510; ++i) {
	  resp[0][i] = f1 * resp[j][i] + f2 * resp[j+1][i];
	}
	fact = -pparea * ebs;
	break;
      } else if (ans[0] == 'I' || ans[0] == 'i') {
	for (i = 380; i < 510; ++i) {
	  resp[0][i] = 0.0;
	}
	f1 = 0.0f;
	for (j = 1; j < 25; ++j) {
	  if (resp[j][513] != 0.0f) {
	    for (i = 380; i < 510; ++i) {
	      resp[0][i] += resp[j][i];
	    }
	    f1 += 1.0f;
	  }
	}
	fact = -pparea * ebs / f1;
	break;
      }
    }
    adjgain(1, &cch1, &cch4, &xlo, &xhi, resp[0]+380, gf3gd.spec,
	    130, gf3gd.maxch+1, fact);
    if (dspsp(0, 0, 0)) goto ERR;
    break;
  }
  if (!caskyn("Are you happy with this? (Y/N)")) {
    if (!caskyn("Would you like to try again? (Y/N)")) goto ERR;
    for (i = 0; i <= gf3gd.maxch; ++i) {
      gf3gd.spec[i] = save[i];
    }
    goto START_BSP;
  }

  /* store response function */
  for (j = 0; j < 24; ++j) {
    if (egamma < resp[j+1][513]) break;
  }
  if (j != 0 && resp[j][513] != 0.0f && resp[j][513] != egamma) {
    for (i = 0; i < j; ++i) {
      for (ii = 0; ii < 514; ++ii) {
	resp[i][ii] = resp[i+1][ii];
      }
    }
  }
  resp[j][513] = egamma;
  /* store escape peak coefficients */
  resp[j][512] = 0.0f;
  resp[j][511] = 0.0f;
  if (egamma >= 1100.0f) {
    ee = log(log(egamma / 1022.0f)) * 2.935f;
    if (e1area > 0.0f) resp[j][512] = log(e1area / pparea) - ee;
    if (e2area > 0.0f) resp[j][511] = log(e2area / pparea) - ee;
  }

  if (thresh >= ecut) {
    for (i = 0; i < 100; ++i) {
      resp[j][i] = 0.0f;
    }
  } else {
    oc1 = x0 + (channo(thresh) - x0) * (rppch - x0) / (ppch - x0);
    fact = 1.0f / (pparea * (ecut - thresh));
    adjgain(-1, &oc1, &xcut, &cch1, &cch2, gf3gd.spec, resp[j],
	    gf3gd.maxch+1, 100, fact);
  }
  fact = 1.f / (pparea * (egamma - ecut));
  adjgain(-1, &xcut, &rppch, &cch1, &cch3, gf3gd.spec, resp[j]+100,
	  gf3gd.maxch + 1, 280, fact);
  for (i = 0; i <= gf3gd.maxch; ++i) {
    gf3gd.spec[i] = save[i] - gf3gd.spec[i];
  }
  fact = 1.f / (pparea * ebs);
  adjgain(-1, &xlo, &xhi, &cch1, &cch4, gf3gd.spec, resp[j]+380,
	  gf3gd.maxch+1, 130, fact);

  gf3gd.lox = loxsav;
  gf3gd.numx = numxsav;
  gf3gd.ncnts = ncntssav;
  for (i = 0; i <= gf3gd.maxch; ++i) {
    gf3gd.spec[i] = save[i];
  }
  gf3gd.disp = 0;
  return 0;

 ERR:
  printf("ERROR - Cannot continue.\n");
  gf3gd.lox = loxsav;
  gf3gd.numx = numxsav;
  gf3gd.ncnts = ncntssav;
  for (i = 0; i <= gf3gd.maxch; ++i) {
    gf3gd.spec[i] = save[i];
  }
  gf3gd.disp = 0;
  return 1;
} /* stores */

/* ====================================================================== */
int unfoldsp(int ilo, int ihi)
{
  float area, fact, ppch, rj1, rj2, cts, sum, save[16384];
  int   i, j, k, ichan, i0, j1, j2, ie0, inc, ill;
  char  ans[80];

  if (ilo < 0) {
  /* correct spectrum for summing effects */
    while ((k = cask("Coinc. rate/singles rate = ?", ans, 80)) &&
	   ffin(ans, k, &fact, &rj1, &rj2));
    if (k == 0) return 0;
    ie0 = channo(0.0f) + .5f;
    i0 = ie0;
    if (i0 < -1) i0 = -1;
    area = 0.f;
    for (i = 0; i <= gf3gd.maxch; ++i) {
      save[i] = gf3gd.spec[i];
      area += gf3gd.spec[i];
    }
    fact /= area;
    for (i = i0 + 2; i <= gf3gd.maxch; ++i) {
      for (j = i0 + 1; j < i; ++j) {
	if ((k = i - j + ie0) > 1)
	  gf3gd.spec[i] -= gf3gd.spec[j] * save[k] * fact;
      }
    }
    strncpy(gf3gd.namesp + 4, ".MOD", 4);
    return 0;
  }

  /* unfold sp. using stored resp. fns. from ilo to ihi */
  j = 0;
  while (ilo < 1 || ihi < 1 ||
	 ilo > gf3gd.maxch || ihi > gf3gd.maxch) {
    if (j) printf("ERROR - bad limits for unfolding.\n");
    j = 1;
    if (!(k = cask("Lower, upper channel limits for unfolding = ?", ans, 80)))
      return 0;
    if (inin(ans, k, &ilo, &ihi, &j2)) ihi = 0;
  }
  if (ilo > ihi) {
    j = ihi;
    ihi = ilo;
    ilo = j;
  }
  if (caskyn("Calculate resp. fn. for every channel? (Y/N)")) {
    for (ichan = ihi; ichan >= ilo; --ichan) {
      cts = (renorma + renormb * (float) ichan) * gf3gd.spec[ichan];
      ppch = (float) ichan;
      calres(&ppch, -cts);
    }
  } else {
    while ((k = cask("No. of chs. to be grouped = ?", ans, 80)) &&
	   inin(ans, k, &inc, &j1, &j2));
    if (inc < 1) inc = 1;
    ill = ihi+1;
    while (ill > ilo) {
      ihi = ill - 1;
      ill -= inc;
      if (ill < ilo) ill = ilo;
      sum = 0.f;
      ppch = 0.f;
      for (ichan = ill; ichan <= ihi; ++ichan) {
	cts = (renorma + renormb * (float) ichan) * gf3gd.spec[ichan];
	ppch += (float) ichan * cts;
	sum += cts;
      }
      if (sum == 0.f) continue;
      ppch /= sum;
      calres(&ppch, -sum);
    }
  }
  strncpy(gf3gd.namesp + 4, ".MOD", 4);
  return 0;
} /* unfoldsp */

/* ====================================================================== */
int ufhelp(char *ans)
{
  FILE *file;
  char j[4];
  static char outfn[80] = "unfoldhelp.txt";
  static char *help =
    "\nCommands unique to UNFOLD:\n\n"
    " S1       Subtract fitted peak #1 from spectrum\n"
    " SR       Calc. and store response function from spectrum\n"
    " WR       Write stored resp. fns to disk file\n"
    " RR       Read stored resp. fns from disk file\n"
    " DR [N]   Delete stored resp. fn. [no. N]\n"
    " RE -N    Show stored (changed) resp. fn. no. N\n"
    " RE N     Calculate resp. fn. for photopk in ch. N\n"
    " UF N,M   Unfold spectrum for photopk chs N to M\n"
    " UF -1    Correct spectrum for summing effects\n"
    " RN       Renormalize response functions\n\n";
 
  /* list available valid commands */
  if (!strncmp(ans, "HE/P", 4)   || !strncmp(ans, "HE/p", 4) || 
      !strncmp(ans, "HELP/P", 6) || !strncmp(ans, "HElp/p", 6)) {
    /* print help */
    /* open output (print) file */
    if ((file = open_new_file(outfn, 0))) {
      fprintf(file, "%s", help);
      fclose(file);
      pr_and_del_file(outfn);
    }
  } else {
    printf("%s", help);
    cask("   Press any key for more, X to eXit help", j, 1);
    if (j[0] == 'X' || j[0] == 'x') return 0;
  }
  gfhelp(ans);
  return 0;
} /* ufhelp */

/* ====================================================================== */
int ufinit()
{
  float rj1, rj2;
  int   i, k, nc, nx, ny;
  char  ans[80];

  /* initialise program */
  set_xwg("gf2_std_");
  initg(&nx, &ny);
  finig();
  printf("\n\n WELCOME TO UNFOLD.\n\n"
	 "This is a program to store measured response functions\n"
	 "   for gamma-ray detectors.\n"
	 "By interpolating between them, it can then calculate the response\n"
	 "   function for gamma rays of any energy, and unfold spectra.\n\n"
	 " Version 5.0  D.C.Radford  August 1999\n\n");

  for (i = 0; i < 25; ++i) {
    resp[i][513] = 0.0f;
  }
  gf3gd.ical = 2;  /* requires that energy calib always be defined */
  gf3gd.gain[1] = 0.5;
  gf3gd.nterms = 2;
  nc = 0;
  encal(ans, nc);
  while ((k = cask("Enter threshold (in kev) ?", ans, 80)) &&
	 ffin(ans, k, &thresh, &rj1, &rj2));
  return 0;
} /* ufinit */

/* ====================================================================== */
int wrres(char *ans)
{

  FILE *file;
  int  i, write = 0;
  char fn[256], buf[51404];
  static char head[40] = "RESPONSE FILE VER. 1";

  /* write/read response functions to/from disk file */
  if (!strncmp(ans, "WR", 2)) write = 1;
  strncpy(ans, "  ", 2);
  if (strlen(ans) < 3) {
    /* ask for response file name */
  GETFN:
    if (!cask("Response file name = ? (default .ext = .rsp)", ans, 80))
      return 0;
  }
  strncpy(fn, ans, 128);
  (void) setext(fn, ".rsp", 128);
  if (write && (file = open_new_file(fn, 0))) {
    /* write response functions to disk */
    datetime(head+20);
    memcpy(buf, &thresh, 4);
    memcpy(buf+4, resp[0], 2056*25);
    if (put_file_rec(file, head, 40) ||
	put_file_rec(file, buf, sizeof(buf))) {
      fclose(file);
      file_error("write to", fn);
      return 1;
    }
    fclose(file);

  } else {
    /* read response functions from disk */
    if (!(file = fopen(fn, "r"))) {
      file_error("open", fn);
      goto GETFN;
    }
    if (get_file_rec(file, ans, 40, 0) <= 0 ||
	strncmp(ans, head, 20) ||
	get_file_rec(file, buf, sizeof(buf), -1) <= 0) {
      file_error("read", fn);
      fclose(file);
      goto GETFN;
    }
    memcpy(&thresh, buf, 4);
    memcpy(resp[0], buf + 4, 2056*25);
    fclose(file);
    printf("File was written at %.18s\n"
	   "  Threshold = %.1f keV\n"
	   "  Stored response functions are:\n"
	   "    #    Egamma\n", ans+20, thresh);
    for (i = 0; i < 25; i++) {
      if (resp[i][513] > 0.0f) printf("%5d %9.2f\n", i+1, resp[i][513]);
    }
  }
  return 0;
} /* wrres */
