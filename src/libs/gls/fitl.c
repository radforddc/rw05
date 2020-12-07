#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "gls.h"
#include "util.h"
#include "minig.h"

struct {
  int   npars, nepars, nipars, idat[MAXFIT], idat2[MAXFIT] ;
  float savedat[MAXFIT],  saveerr[MAXFIT];
  float savedat2[MAXFIT], saveerr2[MAXFIT];
  float alpha[MAXFIT][MAXFIT];
  float beta[MAXFIT], pars[MAXFIT], derivs[MAXFIT], delta[MAXFIT], ers[MAXFIT];
  int   nextp[MAXFIT];
} fgd;

/* ==== other extern routines ==== */
extern int initg2(int *, int *);
extern int retic2(float *, float *, char *);
extern int drawstring(char *, int, char, float, float, float, float);

/* all figure distance units are in keV */
/* gamma arrow widths = intensity * aspect_ratio (in keV units) */

/* ====================================================================== */
int get_fit_lev(int *jnl, short *jlev)
{
  static int   nsave = 0, isave[MAXBAND];

  float x1, y1, x2, y2;
  int   ilev, i, j, iband, iy, ix1, iy1, ix2, iy2, nsx, nsy;
  char  ans[80];

/* get levels to be fitted in energy */
/* ask for levels to be fitted */

  if (*jnl == 0) nsave = 0;
  if (*jnl <= 0) {
    tell(" Bands of levels to be fitted = ?\n"
	 "  ...X to end, A for all levels in level scheme.\n");
  }
 START:
  retic2(&x1, &y1, ans);
  *jnl = 0;
  if (*ans == 'X' || *ans == 'x') {
    return 0;
  } else if (*ans == 'A' || *ans == 'a') {
    *jnl = glsgd.nlevels;
    for (i = 0; i < glsgd.nlevels; ++i) {
      jlev[i] = (short) i;
    }
    initg2(&nsx, &nsy);
    for (iband = 0; iband < glsgd.nbands; ++iband) {
      for (j = 0; j < nsave; ++j) {
	if (iband == isave[j]) break;
      }
      if (j < nsave) continue;
      setcolor(-1);
      isave[nsave++] = iband;
      x1 = glsgd.bnd[iband].x0 + glsgd.bnd[iband].nx / 2.0f - glsgd.csx * 4.5f;
      x2 = glsgd.bnd[iband].x0 + glsgd.bnd[iband].nx / 2.0f + glsgd.csx * 4.5f;
      y1 = -200.0f - glsgd.csy * 0.5f;
      y2 = glsgd.csy * 1.5f - 200.0f;
      cvxy(&x1, &y1, &ix1, &iy1, 1);
      cvxy(&x2, &y2, &ix2, &iy2, 1);
      for (iy = iy1; iy <= iy2; ++iy) {
	mspot(ix1, iy);
	ivect(ix2, iy);
      }
      setcolor(-1);
      setcolor(0);
      drawstring(glsgd.bnd[iband].name, 8, 'H',
		 glsgd.bnd[iband].x0 + glsgd.bnd[iband].nx / 2.0f, -200.0f,
		 glsgd.csx, glsgd.csy);
    }
    setcolor(1);
    finig();
  } else {
    iband = nearest_band(x1, y1);
    if (iband < 0 || iband >= glsgd.nbands) {
      tell("No band selected, try again...\n");
      goto START;
    }
    for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
      if (glsgd.lev[ilev].band == iband) {
	jlev[(*jnl)++] = (short) ilev;
      }
    }
    for (j = 0; j < nsave; ++j) {
      if (iband == isave[j]) return 0;
    }
    initg2(&nsx, &nsy);
    setcolor(-1);
    isave[nsave++] = iband;
    x1 = glsgd.bnd[iband].x0 + glsgd.bnd[iband].nx/2.0f - glsgd.csx * 4.5f;
    x2 = glsgd.bnd[iband].x0 + glsgd.bnd[iband].nx/2.0f + glsgd.csx * 4.5f;
    y1 = -200.0f - glsgd.csy * 0.5f;
    y2 = glsgd.csy * 1.5f - 200.0f;
    cvxy(&x1, &y1, &ix1, &iy1, 1);
    cvxy(&x2, &y2, &ix2, &iy2, 1);
    for (iy = iy1; iy <= iy2; ++iy) {
      mspot(ix1, iy);
      ivect(ix2, iy);
    }
    setcolor(-1);
    setcolor(0);
    drawstring(glsgd.bnd[iband].name, 8, 'H',
	       glsgd.bnd[iband].x0 + glsgd.bnd[iband].nx/2.0f, -200.0f,
	       glsgd.csx, glsgd.csy);
    setcolor(1);
    finig();
  }
  return 0;
} /* get_fit_lev */

/* ======================================================================= */
int fitterlvls(float *chisq)
{
  float r1;
  float diff, y, chisq1, dat, fit;
  int   i, j, k, l, m, ndf;

  /* this subroutine is a modified version of 'CURFIT', in Bevington */
  /* see page 237 */
 START:
  ndf = -fgd.npars;
  /* evaluate fit, alpha & beta matrices, & chisq */
  for (j = 0; j < fgd.npars; ++j) {
    fgd.pars[j] = glsgd.lev[fgd.idat[j]].e;
    fgd.beta[j] = 0.0f;
    for (k = 0; k < j+1; ++k) {
      fgd.alpha[k][j] = 0.0f;
    }
  }
  chisq1 = 0.0f;
  for (i = 0; i < glsgd.ngammas; ++i) {
    y = glsgd.gam[i].e;
    dat = glsgd.gam[i].de * glsgd.gam[i].de;
    if (dat == 0.0f) dat = 1.0f;
    fit = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e;
    diff = y - fit;
    k = 0;
    for (l = 0; l < fgd.npars; ++l) {
      if (fgd.idat[l] == glsgd.gam[i].li) {
	fgd.derivs[l] = 1.0f;
      } else if (fgd.idat[l] == glsgd.gam[i].lf) {
	fgd.derivs[l] = -1.0f;
      } else {
	continue;
      }
      fgd.nextp[k++] = l;
      fgd.beta[l] += diff * fgd.derivs[l] / dat;
      for (m = 0; m < k; ++m) {
	fgd.alpha[fgd.nextp[m]][l] += fgd.derivs[l] * 
	  fgd.derivs[fgd.nextp[m]] / dat;
      }
    }
    if (k > 0) {
      ++ndf;
      chisq1 += diff * diff / dat;
    }
  }
  if (ndf < 0) {
    warn("*** Number of Degrees of Freedom < 0 -- Fit aborted ***\n");
    return 0;
  }
  if (ndf > 0) chisq1 /= (float) ndf;
  /* invert modified curvature matrix to find new parameters */
  for (j = 0; j < fgd.npars; ++j) {
    if (fgd.alpha[j][j] == 0.0f) {
      if (fgd.npars <= 1) {
	warn(" ****** Cannot - diag. element eq. to 0.0 ******\n"
	     " Level %s\n", glsgd.lev[fgd.idat[j]].name);
	return 0;
      } else {
	if (askyn(" ****** Cannot - diag. element eq. to 0.0 ******\n"
		  " Level %s\n"
		  "...Delete from fitted parameters? (Y/N)",
		  glsgd.lev[fgd.idat[j]].name)) return 0;
      }
      --fgd.npars;
      for (i = j; i < fgd.npars; ++i) {
	fgd.idat[i] = fgd.idat[i+1];
	fgd.savedat[i] = fgd.savedat[i+1];
	fgd.saveerr[i] = fgd.saveerr[i+1];
      }
      goto START;
    }
    for (k = 0; k < j; ++k) {
      fgd.alpha[j][k] = fgd.alpha[k][j];
    }
  }
  matinv_float(fgd.alpha[0], fgd.npars, MAXFIT);
  for (j = 0; j < fgd.npars; ++j) {
    fgd.delta[j] = 0.0f;
    for (k = 0; k < fgd.npars; ++k) {
      fgd.delta[j] += fgd.beta[k] * fgd.alpha[k][j];
    }
  }
  for (l = 0; l < fgd.npars; ++l) {
    glsgd.lev[fgd.idat[l]].e = fgd.pars[l] + fgd.delta[l];
  }
  *chisq = 0.0f;
  for (i = 0; i < glsgd.ngammas; ++i) {
    k = 0;
    for (l = 0; l < fgd.npars; ++l) {
      if (fgd.idat[l] == glsgd.gam[i].li ||
	  fgd.idat[l] == glsgd.gam[i].lf) ++k;
    }
    if (k > 0) {
      y = glsgd.gam[i].e;
      if (glsgd.lev[glsgd.gam[i].li].e !=
	  glsgd.lev[glsgd.gam[i].lf].e + y) {
	dat = glsgd.gam[i].de * glsgd.gam[i].de;
	if (dat == 0.0f) dat = 1.0f;
	fit = glsgd.lev[glsgd.gam[i].li].e -
	      glsgd.lev[glsgd.gam[i].lf].e;
	diff = y - fit;
	*chisq += diff * diff / dat;
      }
    }
  }
  if (ndf > 0) *chisq /= (float) ndf;
  tell("  *** CHISQ,CHISQ1 = %7.4f %7.4f\n", *chisq, chisq1);
  /* evaluate parameters and errors */
  /* list data and exit */
  r1 = (*chisq > 1.0f ? sqrt(*chisq) : 1.0f);
  for (j = 0; j < fgd.npars; ++j) {
    if (fgd.alpha[j][j] < 0.0f) fgd.alpha[j][j] = 0.0f;
    glsgd.lev[fgd.idat[j]].de = sqrt(fgd.alpha[j][j]) * r1;
  }
  tell("%i indept. pars  %i degrees of freedom.\n"
       "     Chisq/D.O.F.= %.4f\n", fgd.npars, ndf, *chisq);
  return 0;
} /* fitterlvls */

/* ====================================================================== */
int fit_lvls(void)
{
  float chisq;
  int   i, j, k, nsave, jnl, jf, ji, repeat;
  short blev[MAXLEV], jlev[MAXLEV], k1, k2;

  /* fit energies of up to MAXFIT levels */
  fgd.npars = 0;
  jnl = 0;

  /* figure out which levels can be fitted */
  /* blev >= 0 for fitted levels */
  /* blev = -2 for base levels */
  for (i = 0; i < glsgd.nlevels; ++i) {
    blev[i] = -1;
    if (levemit.n[i] < 1) {
      blev[i] = -2;
      jnl++;
    }
  }
  /* set blev to level id of base level to which it is related */
  repeat = 1;
  while (repeat) {
    repeat = 0;
    for (i = 0; i < glsgd.ngammas; ++i) {
      ji = glsgd.gam[i].li;
      jf = glsgd.gam[i].lf;
      if (blev[ji] != blev[jf]) {
	if (blev[ji] == -1) {
	  repeat = 1;
	  if (blev[jf] < -1) {
	    blev[ji] = jf;
	  } else {
	    blev[ji] = blev[jf];
	  }
	} else if (blev[jf] == -1) {
	  repeat = 1;
	  blev[jf] = blev[ji];
	} else if (blev[jf] < -1 && jf != blev[ji]) {
	  /* we have a base level related to another different base level */
	  k1 = blev[ji];
	  k2 = jf;
	  if (glsgd.lev[k1].e < glsgd.lev[k2].e ||
	      glsgd.lev[k1].e == 0.0f) {
	    k1 = jf;
	    k2 = blev[ji];
	  }
	  for (k = 0; k < glsgd.nlevels; ++k) {
	    if (blev[k] == k1) blev[k] = k2;
	  }
	  blev[k1] = k2;
	  jnl--;
	} else if (blev[ji] >= 0 && blev[jf] >= 0) {
	  /* level is related to 2 different base levels, choose one of them */
	  repeat = 1;
	  k1 = blev[ji];
	  k2 = blev[jf];
	  if (glsgd.lev[k1].e < glsgd.lev[k2].e ||
	      glsgd.lev[k1].e == 0.0f) {
	    k1 = blev[jf];
	    k2 = blev[ji];
	  }
	  for (k = 0; k < glsgd.nlevels; ++k) {
	    if (blev[k] == k1) blev[k] = k2;
	  }
	  blev[k1] = k2;
	  jnl--;
	}
      }
    }
  }

  if (glsgd.nlevels <= MAXFIT + jnl) {
    /* have enough parameters for all fittable levels */
    tell("Can fit all levels... (%d %d %d)\n", glsgd.nlevels, MAXFIT, jnl);
    for (i = 0; i < glsgd.nlevels; ++i) {
      if (blev[i] < 0) continue;
      fgd.idat[fgd.npars] = (short) i;
      fgd.savedat[fgd.npars] = glsgd.lev[i].e;
      fgd.saveerr[fgd.npars++] = glsgd.lev[i].de;
    }
    goto DOFIT;
  }

  /* ask for levels to be fitted */
  jnl = 0;
 START:
  get_fit_lev(&jnl, jlev);
  if (jnl > 0) {
    nsave = fgd.npars;
    for (j = 0; j < jnl; ++j) {
      i = jlev[j];
      if (blev[i] < 0) continue;
      for (k = 0; k < fgd.npars; ++k) {
	if (i == fgd.idat[k]) break;
      }
      if (k < fgd.npars) continue;
      fgd.idat[fgd.npars] = (short) i;
      fgd.savedat[fgd.npars] = glsgd.lev[i].e;
      fgd.saveerr[fgd.npars] = glsgd.lev[i].de;
      if (++fgd.npars == MAXFIT) {
	tell("Max. of MAXFIT = %d parameters exceded; total levels = %d\n",
	     MAXFIT, glsgd.nlevels);
	break;
      }
    }
    tell("\n%d levels included.\n", fgd.npars - nsave);
    if (jnl < glsgd.nlevels && fgd.npars < MAXFIT) goto START;
  }

  DOFIT:
  if (fgd.npars < 1) {
    warn("Not enough parameters to fit...\n");
    return 0;
  }
  save_gls_now(1);
  tell("\nParameters are energies of levels:\n");
  for (j = 0; j < fgd.npars; ++j) {
    tell("%s", glsgd.lev[fgd.idat[j]].name);
    if (j%7 == 6) tell("\n");
  }
  if (j%7 != 6) tell("\n");

  /* do fit */
  fitterlvls(&chisq);
  tell("\n\n     level     old energy   err.     new energy   err.   new-old\n");
  for (i = 0; i < fgd.npars; ++i) {
    j = fgd.idat[i];
    tell("%s%15.3f%7.3f%15.3f%7.3f%10.3f\n",
	 glsgd.lev[j].name, fgd.savedat[i], fgd.saveerr[i],
	 glsgd.lev[j].e, glsgd.lev[j].de, glsgd.lev[j].e - fgd.savedat[i]);
  }
  calc_peaks();
  return 0;
} /* fit_lvls */
