/* This file contains subroutines that are partially common to all versions
   of escl8r and levit8r.
   It is (sorry!) #include'ed in escl8ra.c and lev4d.c */

/* the flag ESCL8R is #defined to disinguish between escl8r (linear gain)
   and levit8r/4dg8r (nonlinear gain) */
/* D.C. Radford            Oct 1999 */

/* ====================================================================== */
int sum0(float egam, float wfact)
{
  float x, dx;

#ifdef ESCL8R
  x = channo(egam);
#else
  if (egam < xxgd.energy_sp[0] ||
      egam > xxgd.energy_sp[xxgd.numchs - 1]) return 1;
  x = egam;
#endif
  if (wfact <= 0.0f) wfact = 1.0f;
  dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		    elgd.swpars[2] * x * x) / 2.0f;
#ifdef ESCL8R
  return sum_chs((int) (0.5f + x - dx), (int) (0.5f + x + dx));
#else
  return sum_eng(x - dx, x + dx);
#endif
} /* sum0 */

/* ====================================================================== */
int chk_fit_egam(int i, int *igam, int *npars, float *saveegam, float *saveerr)
{
  float eg1, eg2;
  int   j, k;

  /* check that energy of gamma # i can be fitted */

  for (j = 0; j < *npars; ++j) {
    if (igam[j] == i) {
      /* gamma already selected, deselect it */
      --(*npars);
      for (k = j; k < *npars; ++k) {
	igam[k] = igam[k + 1];
	saveegam[k] = saveegam[k + 1];
	saveerr[k] = saveerr[k + 1];
      }
      remove_hilite(i);
      return 0;
    }
  }
#ifdef ESCL8R
  energy((float) (xxgd.numchs), 0.0f, &eg1, &eg2);
  eg2 = 50.0f;
#else
  eg1 = xxgd.elo_sp[xxgd.numchs - 1];
  eg2 = xxgd.ehi_sp[0];
#endif
  if (glsgd.gam[i].e > eg1 || glsgd.gam[i].e < eg2) {
    tell("E(gamma) %.1f from %s to %s is outside fittable range.\n",
	 glsgd.gam[i].e,
	 glsgd.lev[glsgd.gam[i].li].name,
	 glsgd.lev[glsgd.gam[i].lf].name);
    return 0;
  }
  if (levemit.n[glsgd.gam[i].lf] == 0) {
    /* check that gamma has feeding coincidences */
    for (j = 0; j < glsgd.ngammas; j++) {
      if (glsgd.gam[j].lf == glsgd.gam[i].li &&
	  glsgd.gam[j].e <= eg1 && glsgd.gam[j].e >= eg2) break;
    }
    if (j >= glsgd.ngammas) {
      tell("E(gamma) %.1f from %s to %s has no fittable coincidences.\n",
	   glsgd.gam[i].e,
	   glsgd.lev[glsgd.gam[i].li].name,
	   glsgd.lev[glsgd.gam[i].lf].name);
      return 0;
    }
  }
  if (glsgd.gam[i].i < glsgd.gam[i].di * 4.0f) {
    if (!askyn("Weak gamma %.1f from %s to %s, I = %.2f +-%.2f\n"
	       "   ...Include? (Y/N)",
	       glsgd.gam[i].e,
	       glsgd.lev[glsgd.gam[i].li].name,
	       glsgd.lev[glsgd.gam[i].lf].name,
	       glsgd.gam[i].i, glsgd.gam[i].di)) return 0;
  }
  igam[*npars] = i;
  saveegam[*npars] = glsgd.gam[i].e;
  saveerr[(*npars)++] = glsgd.gam[i].de;
  hilite(i);
  return 0;
} /* chk_fit_egam */

/* ======================================================================= */
int curse(void)
{
  float x, y, eg, dx, deg;
  int   j, idata;
  char  ans[4];

  /* call cursor */
  tell("Type any character; X or right mouse button to exit.\n");
  while (1) {
    retic(&x, &y, ans);
    if (*ans == 'X' || *ans == 'x') return 0;
#ifdef ESCL8R
    idata = x;
    x = (float) idata;
    dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
	      elgd.swpars[2] * x * x) / 2.0f;
    energy(x, dx, &eg, &deg);
#else
    idata = ichanno(x);
    dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
	      elgd.swpars[2] * x * x) / 2.0f;
    eg = x;
    deg = dx;
#endif
    tell(" Ch = %4i  Eg = %7.2f  Cnts = %6.0f  Calc = %6.0f\n",
	 idata, eg, xxgd.spec[0][idata], xxgd.spec[2][idata]);
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (glsgd.gam[j].e >= eg - deg && glsgd.gam[j].e <= eg + deg) {
	tell("  Gamma #%4i   E = %7.2f   I = %6.2f,  from %s to %s\n",
	     j+1, glsgd.gam[j].e, glsgd.gam[j].i,
	     glsgd.lev[glsgd.gam[j].li].name,
	     glsgd.lev[glsgd.gam[j].lf].name);
      }
    }
  }
} /* curse */

/* ======================================================================= */
int cur_gate(void)
{
#ifdef ESCL8R
  int   lo, hi, j;
#else
  float lo, hi, j;
#endif
  float x, y;
  int   i, nc;
  char  ans[80];

  /* use cursor to specify limits for gate */
  tell("Type any character; A to abort, T to type limits.\n");
  for (i = 0; i < 2; ++i) {
    retic(&x, &y, ans);
    if (i == 0) lo = x;
    if (i == 1) hi = x;
    if (*ans == 'A' || *ans == 'a') return 1;
    if (*ans == 'T' || *ans == 't') {
      while (1) {
#ifdef ESCL8R
	if (!(nc = ask(ans, 80, " Limits = ?"))) return 1;
	if (!inin(ans, nc, &lo, &hi, &j) &&
	    lo >= 0 && lo < xxgd.numchs &&
	    hi >= 0 && hi < xxgd.numchs) break;
#else
	if (!(nc = ask(ans, 80, " Limits (in keV) = ?"))) return 1;
	if (!ffin(ans, nc, &lo, &hi, &j)) break;
#endif
      }
      break;
    }
  }
  save_esclev_now(4);
  get_gate(lo, hi);
  return 0;
} /* cur_gate */

/* ======================================================================= */
int energy_sum(void)
{
  float e1, e2, x1, x2, y, eg;
  int   ilev1, ilev2, iwin1, iwin2;
  char  ans[4];

 START:
  tell("\n"
       "Energy-sum search routine...\n"
       " ...Click twice in spectrum window to look for\n"
       "    sum and/or difference between two peaks, or\n"
       "    click twice in level scheme window to look for\n"
       "    difference in energy between two levels.\n\n");
  while (1) {
    tell("First energy or level = ? (X or button 3 to exit)\n");
    retic3(&x1, &y, ans, &iwin1);
    if (*ans == 'X' || *ans == 'x') return 0;
    if (iwin1 == 1) {
      tell("...Second energy = ? (X or button 3 to exit)\n");
      retic3(&x2, &y, ans, &iwin2);
      if (*ans == 'X' || *ans == 'x') return 0;
      if (iwin1 != iwin2) goto START;
      hilite(-1);
      energy(x1 - 0.5f, 0.0f, &e1, &y);
      energy(x2 - 0.5f, 0.0f, &e2, &y);
      tell("Gamma energies: %.1f %.1f\n"
	   "   Sum: %.1f   Difference: %.1f\n",
	   e1, e2, e1 + e2, fabs(e1 - e2));
      sum0(e1 + e2, 1.0f);
      sum0(fabs(e1 - e2), 1.0f);
    } else {
      if ((ilev1 = nearest_level(x1, y)) < 0) {
	tell("No level selected, try again...\n");
	continue;
      }
      tell("...Second level = ? (X or button 3 to exit)\n");
      retic3(&x2, &y, ans, &iwin2);
      if (*ans == 'X' || *ans == 'x') return 0;
      if (iwin1 != iwin2) goto START;
      if ((ilev2 = nearest_level(x2, y)) < 0) {
	tell("No level selected, try again...\n");
	continue;
      }
      eg = fabs(glsgd.lev[ilev1].e - glsgd.lev[ilev2].e);
      tell("Levels %s and %s selected.\n"
	   "   Energy difference: %.1f\n",
	   glsgd.lev[ilev1].name, glsgd.lev[ilev2].name, eg);
      hilite(-1);
      sum0(eg, 1.0f);
    }
  }
} /* energy_sum */

/* ======================================================================= */
int eval0(int iy, int ix, float *fit)
{
  int   i, j, ii, jj, ix1, iy1;

  /* calculate fit only, using present values of the pars */

  *fit = 0.0f;
  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
    i = xxgd.pks_ch[iy][ii];
    iy1 = iy - xxgd.lo_ch[i];
    for (jj = 0; jj < xxgd.npks_ch[ix]; ++jj) {
      j = xxgd.pks_ch[ix][jj];
      if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	ix1 = ix - xxgd.lo_ch[j];
	*fit += elgd.levelbr[j][glsgd.gam[i].lf] * glsgd.gam[i].i *
	  xxgd.pk_shape[i][iy1] * xxgd.pk_shape[j][ix1];
      } else if (elgd.levelbr[i][glsgd.gam[j].lf] != 0.0f) {
	ix1 = ix - xxgd.lo_ch[j];
	*fit += elgd.levelbr[i][glsgd.gam[j].lf] * glsgd.gam[j].i *
	  xxgd.pk_shape[i][iy1] * xxgd.pk_shape[j][ix1];
      }
    }
  }
  return 0;
} /* eval0 */

/* ======================================================================= */
int eval_fdb(int iy, int ix, float *fit, float *derivs)
{
  float a, b, pp;
  int   i, j, k, npars, hi, ii, jj, kk, lo, ix1, iy1;

  /* calculate fit and derivatives w.r.to BOTH Egamma and Igamma */
  /* using present values of the pars */

  npars = fgd.nepars + fgd.nipars;
  *fit = 0.0f;
  for (i = 0; i < npars; ++i) {
    derivs[i] = 0.0f;
  }
  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
    i = xxgd.pks_ch[iy][ii];
    iy1 = iy - xxgd.lo_ch[i];
    for (jj = 0; jj < xxgd.npks_ch[ix]; ++jj) {
      j = xxgd.pks_ch[ix][jj];
      if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	hi = i;
	lo = j;
      } else if (elgd.levelbr[i][glsgd.gam[j].lf] != 0.0f) {
	hi = j;
	lo = i;
      } else {
	continue;
      }
      ix1 = ix - xxgd.lo_ch[j];
      pp = xxgd.pk_shape[i][iy1] * xxgd.pk_shape[j][ix1];
      b = elgd.levelbr[lo][glsgd.gam[hi].lf];
      a = b * glsgd.gam[hi].i;
      *fit += a * pp;
      for (kk = 0; kk < fgd.nepars; ++kk) {
	k = fgd.idat[kk];
	if (k == i) {
	  derivs[kk] += a * xxgd.pk_deriv[i][iy1] * xxgd.pk_shape[j][ix1];
	} else if (k == j) {
	  derivs[kk] += a * xxgd.pk_shape[i][iy1] * xxgd.pk_deriv[j][ix1];
	}
      }
      for (kk = 0; kk < fgd.nipars; ++kk) {
	k = fgd.idat2[kk];
	if (k == hi) {
	  derivs[kk + fgd.nepars] += b * pp;
	} else if (k == lo) {
	  if (levemit.n[glsgd.gam[k].li] > 1)
	    derivs[kk + fgd.nepars] +=  a * pp *
	      (1.0f -
	       elgd.levelbr[lo][glsgd.gam[lo].li] * (glsgd.gam[lo].a + 1.0f)) /
	      glsgd.gam[lo].i;
	} else if (elgd.levelbr[k][glsgd.gam[hi].lf] != 0.0f && 
		   elgd.levelbr[lo][glsgd.gam[k].li] != 0.0f && 
		   levemit.n[glsgd.gam[k].li] > 1) {
	  derivs[kk + fgd.nepars] += pp * glsgd.gam[hi].i *
	    elgd.levelbr[k][glsgd.gam[hi].lf] * (glsgd.gam[k].a + 1.0f) *
	    (elgd.levelbr[lo][glsgd.gam[k].lf] -
	     elgd.levelbr[lo][glsgd.gam[k].li]) / glsgd.gam[k].i;
	}
      }
    }
  }
  return 0;
} /* eval_fdb */

/* ======================================================================= */
int eval_fde(int iy, int ix, float *fit, float *derivs)
{
  float a;
  int   i, j, k, ii, jj, kk, ix1, iy1;

  /* calculate fit and derivatives w.r.to Egamma, */
  /* using present values of the pars */

  *fit = 0.0f;
  for (i = 0; i < fgd.npars; ++i) {
    derivs[i] = 0.0f;
  }
  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
    i = xxgd.pks_ch[iy][ii];
    iy1 = iy - xxgd.lo_ch[i];
    for (jj = 0; jj < xxgd.npks_ch[ix]; ++jj) {
      j = xxgd.pks_ch[ix][jj];
      if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	a = elgd.levelbr[j][glsgd.gam[i].lf] * glsgd.gam[i].i;
      } else if (elgd.levelbr[i][glsgd.gam[j].lf] != 0.0f) {
	a = elgd.levelbr[i][glsgd.gam[j].lf] * glsgd.gam[j].i;
      } else {
	continue;
      }
      ix1 = ix - xxgd.lo_ch[j];
      *fit += a * xxgd.pk_shape[i][iy1] * xxgd.pk_shape[j][ix1];
      for (kk = 0; kk < fgd.npars; ++kk) {
	k = fgd.idat[kk];
	if (k == i) {
	  derivs[kk] += a * xxgd.pk_deriv[i][iy1] * xxgd.pk_shape[j][ix1];
	} else if (k == j) {
	  derivs[kk] += a * xxgd.pk_shape[i][iy1] * xxgd.pk_deriv[j][ix1];
	}
      }
    }
  }
  return 0;
} /* eval_fde */

/* ======================================================================= */
int eval_fdi(int iy, int ix, float *fit, float *derivs)
{
  float a, b, pp;
  int   i, j, k, hi, ii, jj, kk, lo;
  int   ix1, iy1;

  /* calculate fit and derivatives w.r.to Igamma, */
  /* using present values of the pars */

  *fit = 0.0f;
  for (i = 0; i < fgd.npars; ++i) {
    derivs[i] = 0.0f;
  }
  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
    i = xxgd.pks_ch[iy][ii];
    iy1 = iy - xxgd.lo_ch[i];
    for (jj = 0; jj < xxgd.npks_ch[ix]; ++jj) {
      j = xxgd.pks_ch[ix][jj];
      if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	hi = i;
	lo = j;
      } else if (elgd.levelbr[i][glsgd.gam[j].lf] != 0.0f) {
	hi = j;
	lo = i;
      } else {
	continue;
      }
      ix1 = ix - xxgd.lo_ch[j];
      pp = xxgd.pk_shape[i][iy1] * xxgd.pk_shape[j][ix1];
      b = elgd.levelbr[lo][glsgd.gam[hi].lf];
      a = b * glsgd.gam[hi].i;
      *fit += a * pp;
      for (kk = 0; kk < fgd.npars; ++kk) {
	k = fgd.idat[kk];
	if (k == hi) {
	  derivs[kk] += b * pp;
	} else if (k == lo) {
	  if (levemit.n[glsgd.gam[k].li] > 1)
	    derivs[kk] += a * pp * 
	      (1.0f -
	       elgd.levelbr[lo][glsgd.gam[lo].li] * (glsgd.gam[lo].a + 1.0f)) /
	      glsgd.gam[lo].i;
	} else if (elgd.levelbr[k][glsgd.gam[hi].lf] != 0.0f && 
		   elgd.levelbr[lo][glsgd.gam[k].li] != 0.0f && 
		   levemit.n[glsgd.gam[k].li] > 1) {
	  derivs[kk] += pp * glsgd.gam[hi].i *
	    elgd.levelbr[k][glsgd.gam[hi].lf] * (glsgd.gam[k].a + 1.0f) * 
	    (elgd.levelbr[lo][glsgd.gam[k].lf] - 
	     elgd.levelbr[lo][glsgd.gam[k].li]) / glsgd.gam[k].i;
	}
      }
    }
  }
  return 0;
} /* eval_fdi */

/* ====================================================================== */
int eval_fwp(int ix, float *fit, float *derivs, int mode)
{
  float fwhm, x, f1, p1, p2, p3, eg, dw;
  int   i, npars, ii, ix1;

  /* calculate fit and derivatives w.r.to width parameters */
  /* using present values of the pars */

  npars = 3;
  *fit = 0.0f;
  /* calculate derivs only for mode.ge.1 */
  if (mode < 1) {
    for (ii = 0; ii < xxgd.npks_ch[ix]; ++ii) {
      i = xxgd.pks_ch[ix][ii];
      ix1 = ix - xxgd.lo_ch[i];
      *fit += elgd.hpk[0][i] * xxgd.pk_shape[i][ix1];
    }
    return 0;
  }
  for (i = 0; i < npars; ++i) {
    derivs[i] = 0.0f;
  }
  /* fit SQRT(SWPARS) */
  p1 = sqrt(elgd.swpars[0]) * 2.0f;
  p2 = sqrt(elgd.swpars[1]) * 2.0f;
  p3 = sqrt(elgd.swpars[2]) * 2.0f;
  for (ii = 0; ii < xxgd.npks_ch[ix]; ++ii) {
    i = xxgd.pks_ch[ix][ii];
    ix1 = ix - xxgd.lo_ch[i];
    *fit += elgd.hpk[0][i] * xxgd.pk_shape[i][ix1];
    eg = glsgd.gam[i].e;
#ifdef ESCL8R
    x = channo(eg);
#else
    x = eg;
#endif
    fwhm = sqrt(elgd.swpars[0] + elgd.swpars[1] * x + elgd.swpars[2] * x * x);
    f1 = 0.5f / fwhm;
    dw = elgd.hpk[0][i] * xxgd.w_deriv[i][ix1] * f1;
    /* fit SQRT(SWPARS) */
    derivs[0] += dw * p1;
    derivs[1] += dw * x * p2;
    derivs[2] += dw * x * x * p3;
  }
  return 0;
} /* eval_fwp */

/* ======================================================================= */
int examine(char *ans, int nc)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */
  float eg, wf, fj1, fj2;
  int   i, j, nl;

  /* sum results over range taken from specified Egamma */
  /* and parameterized_FWHM */
  hilite(-1);
  if (strncmp(ans, "XL", 2)) {
    if (!ffin(ans + 1, nc - 1, &eg, &wf, &fj2)) return sum0(eg, wf);
    return 1;
  }
  while (1) {
    for (i = 0; i < 55; ++i) {
      if (ans[2] == listnam[i] ||
	  (ans[2] == ' ' && ans[3] == listnam[i])) {
	nl = i;
	wf = 1.0f;
	if (nc >= 5) ffin(ans + 4, nc - 4, &wf, &fj1, &fj2);
	for (j = 0; j < elgd.nlist[nl]; ++j) {
	  sum0(elgd.list[nl][j], wf);
	}
	return 0;
      }
    }
    if (!ask(ans + 2, 1, "List name (a-z, A-Z) = ?")) return 0;
  }
} /* examine */

/* ======================================================================= */
int fitter2d(char mode, int maxits)
{
  /* 2D fitter subroutine for escl8r / levit8r / 4dg8r */
  /* call with mode = E to fit Energies,
                      I to fit Intensities,
                   or B to fit Both energies and intensities */

  static char fmt[] =
    " ****** Cannot - diag. element eq. to 0.0 ******\n"
    " Gamma energy = %.3f\n";

  float r1, r2;
  float diff, y, chisq = 0.0f, chisq1, flamda, dat, fit;
  int   conv = 0, nits, i, j, k, l, m, ii, jj, iy, ix, ich, ndf;
#ifndef ESCL8R
  int   ichy;
#endif

  /* this subroutine is a modified version of 'CURFIT', in Bevington */
  /* see page 237 */

  if (mode == 'B') fgd.npars = fgd.nepars + fgd.nipars;
  path(1);
  calc_peaks();
 START:
  flamda = 1.0f;
  nits = 0;

  /* turn on trapping of control-C / interrupt */
  set_signal_alert(1, "Busy fitting...");
  /* evaluate fit, alpha & beta matrices, & chisq */
 NEXT_IT:
  for (j = 0; j < fgd.npars; ++j) {
    if (mode == 'B') {
      if (j < fgd.nepars) {
	fgd.pars[j] = glsgd.gam[fgd.idat[j]].e;
      } else {
	fgd.pars[j] = glsgd.gam[fgd.idat2[j - fgd.nepars]].i;
      }
    } else if (mode == 'E') {
      fgd.pars[j] = glsgd.gam[fgd.idat[j]].e;
    } else {
      fgd.pars[j] = glsgd.gam[fgd.idat[j]].i;
    }
    fgd.beta[j] = 0.0f;
    for (k = 0; k <= j; ++k) {
      fgd.alpha[k][j] = 0.0f;
    }
  }
  ndf = 0;
  chisq1 = 0.0f;
  for (iy = 0; iy < xxgd.numchs; ++iy) {
    if (check_signal_alert()) goto DONE;
    for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
      i = xxgd.pks_ch[iy][ii];
      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
	j = elgd.ifitdat[i][jj];
	if (iy <= xxgd.hi_ch[j]) goto L96;
      }
    }
    continue;
  L96:
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      xxgd.fitchx[ix] = 0;
    }
    for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
      i = xxgd.pks_ch[iy][ii];
      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
	j = elgd.ifitdat[i][jj];
	if (xxgd.hi_ch[j] < iy) continue;
	for (ich = xxgd.lo_ch[j]; ich <= xxgd.hi_ch[j]; ++ich) {
	  xxgd.fitchx[ich] = 1;
	}
      }
    }
    GETDAT;
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      if (!xxgd.fitchx[ix]) continue;
      ndf++;

      if (mode == 'B') {
	eval_fdb(iy, ix, &fit, fgd.derivs);
      } else if (mode == 'E') {
	eval_fde(iy, ix, &fit, fgd.derivs);
      } else {
	eval_fdi(iy, ix, &fit, fgd.derivs);
      }

      SUBDAT;
      if (dat < 1.0f) dat = 1.0f;
      diff = y - fit;
      chisq1 += diff * diff / dat;
      k = 0;
      for (l = 0; l < fgd.npars; ++l) {
	if (fgd.derivs[l] != 0.0f) {
	  fgd.nextp[k++] = l;
	  fgd.beta[l] += diff * fgd.derivs[l] / dat;
	  for (m = 0; m < k; ++m) {
	    fgd.alpha[fgd.nextp[m]][l] += fgd.derivs[l] * 
	      fgd.derivs[fgd.nextp[m]] / dat;
	  }
	}
      }
    }
  }
  ndf -= fgd.npars;
  chisq1 /= (float) ndf;

  /* invert modified curvature matrix to find new parameters */
  for (j = 0; j < fgd.npars; ++j) {
    if (fgd.alpha[j][j] == 0.0f) {

      if (mode == 'B') {
	if (j < fgd.nepars) {
	  tell(fmt, glsgd.gam[fgd.idat[j]].e);
	  if (prfile) fprintf(prfile, fmt, glsgd.gam[fgd.idat[j]].e);
	} else {
	  tell(fmt, glsgd.gam[fgd.idat2[j - fgd.nepars]].e);
	  if (prfile) fprintf(prfile, fmt,
			      glsgd.gam[fgd.idat2[j - fgd.nepars]].e);
	}
      } else {
	tell(fmt, glsgd.gam[fgd.idat[j]].e);
	if (prfile) fprintf(prfile, fmt, glsgd.gam[fgd.idat[j]].e);
      }

      if (fgd.npars <= 1 ||
	  !askyn("...Delete from fitted parameters? (Y/N)")) {
	tell("***** Failed to converge after %i iterations.\n"
	     "      Chisq/D.O.F.= %.4f\n", nits, chisq);
	if (prfile) fprintf(prfile,
	     "***** Failed to converge after %i iterations.\n"
	     "      Chisq/D.O.F.= %.4f\n", nits, chisq);
	return 1;
      }
      --fgd.npars;

      if (mode == 'B') {
	if (j < fgd.nepars) {
	  fgd.nepars--;
	  for (i = j; i < fgd.nepars; ++i) {
	    fgd.idat[i] = fgd.idat[i+1];
	    fgd.savedat[i] = fgd.savedat[i+1];
	    fgd.saveerr[i] = fgd.saveerr[i+1];
	  }
	} else {
	  fgd.nipars--;
	  for (i = j - fgd.nepars; i < fgd.nipars; ++i) {
	    fgd.idat2[i] = fgd.idat2[i+1];
	    fgd.savedat2[i] = fgd.savedat2[i+1];
	    fgd.saveerr2[i] = fgd.saveerr2[i+1];
	  }
	}
      } else {
	for (i = j; i < fgd.npars; ++i) {
	  fgd.idat[i] = fgd.idat[i+1];
	  fgd.savedat[i] = fgd.savedat[i+1];
	  fgd.saveerr[i] = fgd.saveerr[i+1];
	}
      }

      goto START;
    }
    for (k = 0; k < j; ++k) {
      fgd.alpha[j][k] = fgd.alpha[k][j];
    }
  }
  matinv_float(fgd.alpha[0], fgd.npars, MAXFIT);
  if (check_signal_alert()) goto DONE;
  for (j = 0; j < fgd.npars; ++j) {
    fgd.delta[j] = 0.0f;
    for (k = 0; k < fgd.npars; ++k) {
      fgd.delta[j] += fgd.beta[k] * fgd.alpha[k][j];
    }
  }

  /* if chisq increased, decrease flamda & try again */
 L255:

  if (mode == 'B') {
    for (l = 0; l < fgd.npars; ++l) {
      if (l < fgd.nepars) {
	glsgd.gam[fgd.idat[l]].e = fgd.pars[l] + flamda * fgd.delta[l];
      } else {
	glsgd.gam[fgd.idat2[l - fgd.nepars]].i = fgd.pars[l] +
	  flamda * fgd.delta[l];
      }
    }
    path(0);
    calc_peaks();
    ndf = 0;
  } else if (mode == 'E') {
    for (l = 0; l < fgd.npars; ++l) {
      glsgd.gam[fgd.idat[l]].e = fgd.pars[l] + flamda * fgd.delta[l];
    }
    calc_peaks();
    ndf = 0;
  } else {
    for (l = 0; l < fgd.npars; ++l) {
      glsgd.gam[fgd.idat[l]].i = fgd.pars[l] + flamda * fgd.delta[l];
    }
    path(0);
  }

  chisq = 0.0f;
  for (iy = 0; iy < xxgd.numchs; ++iy) {
    if (check_signal_alert()) goto DONE;
    for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
      i = xxgd.pks_ch[iy][ii];
      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
	j = elgd.ifitdat[i][jj];
	if (iy <= xxgd.hi_ch[j]) goto L276;
      }
    }
    continue;
  L276:
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      xxgd.fitchx[ix] = 0;
    }
    for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
      i = xxgd.pks_ch[iy][ii];
      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
	j = elgd.ifitdat[i][jj];
	if (xxgd.hi_ch[j] < iy) continue;
	for (ich = xxgd.lo_ch[j]; ich <= xxgd.hi_ch[j]; ++ich) {
	  xxgd.fitchx[ich] = 1;
	}
      }
    }
    GETDAT;
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      if (!xxgd.fitchx[ix]) continue;
      if (mode != 'I') ++ndf;
      eval0(iy, ix, &fit);
      SUBDAT;
      if (dat < 1.0f) dat = 1.0f;
      diff = y - fit;
      chisq += diff * diff / dat;
    }
  }
  if (mode != 'I') ndf -= fgd.npars;
  chisq /= (float) ndf;
  tell("*** nits, flamda, chisq, chisq1 = %i %.2e %.4f %.4f\n",
       nits, flamda, chisq, chisq1);
  if (chisq > chisq1 * 1.0001f && flamda > 1e-6f) {
    flamda /= 4.0f;
    goto L255;
  }

  /* evaluate parameters and errors */
  /* test for convergence */
  conv = 1;
  for (j = 0; j < fgd.npars; ++j) {
    if (fgd.alpha[j][j] < 0.0f) fgd.alpha[j][j] = 0.0f;
    fgd.ers[j] = sqrt(fgd.alpha[j][j]);
    if (flamda * fabs(fgd.delta[j]) >= fgd.ers[j] / 100.0f) conv = 0;
  }
  if (flamda < 1.0f) flamda *= 2.0f;
  if (++nits < maxits && !conv) goto NEXT_IT;

 DONE:
  if (check_signal_alert()) {
    tell("**** Fit Interrupted ****\n"
	 "**** WARNING: Parameter values and errors may be undefined!\n");
    if (prfile)
      fprintf(prfile, "**** Fit Interrupted ****\n"
	      "**** WARNING: Parameter values and errors may be undefined!\n");
  }
  /* turn off trapping of control-C / interupt */
  set_signal_alert(0, "");
  /* list data and exit */
  r1 = chisq;
  if (chisq < 1.0f) r1 = 1.0f;
  for (l = 0; l < fgd.npars; ++l) {

    if (mode == 'B') {
      if (l < fgd.nepars) {
	r2 = elgd.encal_err;
	glsgd.gam[fgd.idat[l]].de =
	  sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
      } else {
	r2 = elgd.effcal_err * glsgd.gam[fgd.idat2[l - fgd.nepars]].i;
	glsgd.gam[fgd.idat2[l - fgd.nepars]].di =
	  sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
      }
    } else if (mode == 'E') {
      r2 = elgd.encal_err;
      glsgd.gam[fgd.idat[l]].de =
	sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
    } else {
      r2 = elgd.effcal_err * glsgd.gam[fgd.idat[l]].i;
      glsgd.gam[fgd.idat[l]].di = sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
    }

  }
  tell("\n%i indept. pars   %i degrees of freedom.\n", fgd.npars, ndf);
  if (prfile) fprintf(prfile,
	 "\n%i indept. pars   %i degrees of freedom.\n", fgd.npars, ndf);
  if (conv) {
    tell("%i iterations,    Chisq/D.O.F.= %.4f\n", nits, chisq);
    if (prfile) fprintf(prfile,
	   "%i iterations,    Chisq/D.O.F.= %.4f\n", nits, chisq);
    return 0;
  }
  tell("***** Failed to converge after %i iterations.\n"
       "      Chisq/D.O.F.= %.4f\n", nits, chisq);
  if (prfile) fprintf(prfile,
       "***** Failed to converge after %i iterations.\n"
       "      Chisq/D.O.F.= %.4f\n", nits, chisq);
  return 1;
} /* fitter2d */

/* ====================================================================== */
int fitterwp(int npars, int maxits)
{
  static char fmt_420[] = "\n"
    "%i indept. pars   %i degrees of freedom.\n";
  static char fmt_440[] =
    "%i iterations,    Chisq/D.O.F.= %.4f\n"
    "  Parameters: %.4f +- %.4f  %.4f +- %.4f  %.4f +- %.4f\n\n";
  static char fmt_500[] =
    "Failed to converge after %i iterations.    Chisq/D.O.F.= %.4f\n"
    "  Parameters: %.4f +- %.4f  %.4f +- %.4f  %.4f +- %.4f\n\n";
  static char fmt_600[] =
    " ****** Cannot - diag. element eq. to 0.0 ******\n";

  float r1;
  float diff, y, chisq = 0.0f, chisq1, flamda;
  float dat, fit, par, par2;
  int   conv, nits, j, k, l, m, ix, ndf;

  /* this subroutine is a modified version of 'CURFIT', in Bevington */
  /* see page 237 */
  flamda = 1.0f;
  nits = 0;
  ndf = xxgd.numchs - npars;
  /* evaluate fit, alpha & beta matrices, & chisq */
 NEXT_IT:
  for (j = 0; j < npars; ++j) {
    fgd.beta[j] = 0.0f;
    for (k = 0; k <= j; ++k) {
      fgd.alpha[k][j] = 0.0f;
    }
  }
  /* fit SQRT(SWPARS) */
  fgd.pars[0] = sqrt(elgd.swpars[0]);
  fgd.pars[1] = sqrt(elgd.swpars[1]);
  fgd.pars[2] = sqrt(elgd.swpars[2]);
  chisq1 = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    eval_fwp(ix, &fit, fgd.derivs, 1);
    y = xxgd.spec[0][ix];
    dat = xxgd.spec[5][ix];
    diff = y - fit;
    chisq1 += diff * diff / dat;
    k = 0;
    for (l = 0; l < npars; ++l) {
      if (fgd.derivs[l] != 0.0f) {
	fgd.nextp[k++] = l;
	fgd.beta[l] += diff * fgd.derivs[l] / dat;
	for (m = 0; m < k; ++m) {
	  fgd.alpha[fgd.nextp[m]][l] += fgd.derivs[l] * 
	    fgd.derivs[fgd.nextp[m]] / dat;
	}
      }
    }
  }
  chisq1 /= (float) ndf;
  /* invert modified curvature matrix to find new parameters */
  for (j = 0; j < npars; ++j) {
    if (fgd.alpha[j][j] == 0.0f) {
      warn1(fmt_600);
      if (prfile) fprintf(prfile, "%s", fmt_600);
      conv = 0;
      goto L380;
    }
    for (k = 0; k < j; ++k) {
      fgd.alpha[j][k] = fgd.alpha[k][j];
    }
  }
  matinv_float(fgd.alpha[0], fgd.npars, MAXFIT);
  for (j = 0; j < npars; ++j) {
    fgd.delta[j] = 0.0f;
    for (k = 0; k < npars; ++k) {
      fgd.delta[j] += fgd.beta[k] * fgd.alpha[k][j];
    }
  }
  /* if chisq increased, decrease flamda & try again */
 L255:
  for (l = 0; l < npars; ++l) {
    par = fgd.pars[l] + flamda * fgd.delta[l];
    par2 = (1e-5f > par ? 1e-5f : par);
    /* fit SQRT(SWPARS) */
    elgd.swpars[l] = par2 * par2;
  }
  calc_peaks();
  chisq = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    eval_fwp(ix, &fit, fgd.derivs, 0);
    y = xxgd.spec[0][ix];
    dat = xxgd.spec[5][ix];
    diff = y - fit;
    chisq += diff * diff / dat;
  }
  chisq /= (float) ndf;
  tell("*** nits, flamda, chisq, chisq1 = %i %.2e %.4f %.4f\n",
       nits, flamda, chisq, chisq1);
  if (chisq > chisq1 * 1.000001f && flamda > 1e-6f) {
    flamda /= 4.0f;
    goto L255;
  }
  /* evaluate parameters and errors */
  /* test for convergence */
  conv = 1;
  for (j = 0; j < npars; ++j) {
    if (fgd.alpha[j][j] < 0.0f) fgd.alpha[j][j] = 0.0f;
    fgd.ers[j] = sqrt(fgd.alpha[j][j]);
    if (flamda * fabs(fgd.delta[j]) >= fgd.ers[j] / 1e3f) conv = 0;
  }
  if (flamda < 1.0f) flamda *= 2.0f;
  ++nits;
  if (!conv && nits < maxits) goto NEXT_IT;
  /* list data and exit */
 L380:
  r1 = sqrt(chisq);
  if (r1 < 1.0f) r1 = 1.0f;
  for (l = 0; l < npars; ++l) {
    fgd.pars[l] = sqrt(elgd.swpars[l] * pow(1e3f, (float) l));
    fgd.ers[l] = fgd.ers[l] * sqrt(pow(1e3f, (float) l)) * r1;
  }
  tell(fmt_420, npars, ndf);
  if (prfile) fprintf(prfile, fmt_420, npars, ndf);
  if (conv) {
    tell(fmt_440, nits, chisq,
	 fgd.pars[0], fgd.ers[0], fgd.pars[1],
	 fgd.ers[1], fgd.pars[2], fgd.ers[2]);
    if (prfile) fprintf(prfile, fmt_440, nits, chisq,
			fgd.pars[0], fgd.ers[0], fgd.pars[1],
			fgd.ers[1], fgd.pars[2], fgd.ers[2]);
    return 0;
  }
  tell(fmt_500, nits, chisq,
       fgd.pars[0], fgd.ers[0], fgd.pars[1],
       fgd.ers[1], fgd.pars[2], fgd.ers[2]);
  if (prfile) fprintf(prfile, fmt_500, nits, chisq,
		      fgd.pars[0], fgd.ers[0], fgd.pars[1],
		      fgd.ers[1], fgd.pars[2], fgd.ers[2]);
  return 1;
} /* fitterwp */

/* ======================================================================= */
#ifdef ESCL8R
int fit_both(void)
#else
int fit_both(int fit_trip)
#endif
{
  static char fmt_350[] = "\n\n"
    "   initial      final     old energy   err.     new energy   err.   new-old\n";
  static char fmt_360[] = "%s %s%15.3f%7.3f%15.3f%7.3f%10.3f\n";
  static char fmt_450[] = "\n\n"
    "  energy    initial      final     old int.  err.    new int.  err.   diff.\n";
  static char fmt_460[] = "%8.2f %s %s%11.2f%7.2f%11.2f%7.2f%8.2f\n";

  int   i, j, k, epars, nsave, ipars, j1, j2, kk;
  int   fit_it, maxits, jng;
  short jgam[MAXGAM];
  char  ans[80];

  /* fit energies and intensities of up to MAXFIT/2 gammas
   in the level scheme */
  /* ask for gammas to be fitted */

  epars = 0;
  ipars = 0;
  jng = 0;
  while (1) {
    get_fit_gam(&jng, jgam);
    if (jng == 0) break;
    nsave = epars + ipars;
    for (j = 0; j < jng; ++j) {
      i = jgam[j];
      chk_fit_egam(i, fgd.idat, &epars, fgd.savedat, fgd.saveerr);
#ifdef ESCL8R
      chk_fit_igam(i, fgd.idat2, &ipars, fgd.savedat2, fgd.saveerr2);
#else
      chk_fit_igam(i, fgd.idat2, &ipars, fgd.savedat2, fgd.saveerr2,
		   fit_trip);
#endif
      if (epars + ipars >= MAXFIT - 1) break;
    }
    tell("%d parameters included.\n", epars + ipars - nsave);
    if (epars + ipars >= MAXFIT - 1) break;
  }
  fgd.nepars = epars;
  fgd.nipars = ipars;
  if (fgd.nepars + fgd.nipars < 1) {
    tell(" Not enough parameters to fit...\n");
    return 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.nfitdat[i] = 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (elgd.levelbr[j][glsgd.gam[i].lf] < 0.003f) continue;
      if (elgd.levelbr[j][glsgd.gam[i].lf] >= 0.05f) {
	for (kk = 0; kk < fgd.nepars; ++kk) {
	  k = fgd.idat[kk];
	  if (k == i || k == j) {
	    elgd.ifitdat[i][elgd.nfitdat[i]++] = (short) j;
	    elgd.ifitdat[j][elgd.nfitdat[j]++] = (short) i;
	    goto L265;
	  }
	}
      }
      for (kk = 0; kk < fgd.nipars; ++kk) {
	k = fgd.idat2[kk];
	fit_it = 0;
	if (k == i) {
	  if (elgd.levelbr[j][glsgd.gam[k].lf] > 0.03f) fit_it = 1;
	} else if (k == j) {
	  if ((1.0f -
	       elgd.levelbr[k][glsgd.gam[k].li] * (glsgd.gam[k].a + 1.0f)) *
	      glsgd.gam[i].i > 0.03f) fit_it = 1;
	} else if (elgd.levelbr[k][glsgd.gam[i].lf] *
		   fabs(elgd.levelbr[j][glsgd.gam[k].li] -
			elgd.levelbr[j][glsgd.gam[k].lf]) > 0.03f) {
	  fit_it = 1;
	}
	if (fit_it) {
	  elgd.ifitdat[i][elgd.nfitdat[i]++] = (short) j;
	  elgd.ifitdat[j][elgd.nfitdat[j]++] = (short) i;
	  break;
	}
      }
    L265:
      ;
    }
  }
  tell("\nParameters are energies for level scheme gammas:");
  for (j = 0; j < fgd.nepars; ++j) {
    if (j%8 == 0) tell("\n");
    tell("%8.2f", glsgd.gam[fgd.idat[j]].e);
  }
  tell("\n     and intensities for level scheme gammas:");
  for (j = 0; j < fgd.nipars; ++j) {
    if (j%8 == 0) tell("\n");
    tell("%8.2f", glsgd.gam[fgd.idat2[j]].e);
  }
  tell("\n");
  /* do fit */
  save_gls_now(2); /* save gamma data for possible undo */
  while (1) {
    maxits = 2;
    if ((k = ask(ans, 80, "Max. no. of iterations = ? (rtn for 2)")) &&
	inin(ans, k, &maxits, &j1, &j2)) continue;
    if (maxits <= 0) return 0;
#ifdef ESCL8R
    if (!fitter2d('B', maxits) ||
	!askyn("...Continue fitting? (Y/N)")) break;
#else
    if (fit_trip) {
      if (!fitter3d('B', maxits) ||
	  !askyn("...Continue fitting? (Y/N)")) break;
    } else {
      if (!fitter2d('B', maxits) ||
	  !askyn("...Continue fitting? (Y/N)")) break;
    }
#endif
  }
  tell(fmt_350);
  if (prfile) fprintf(prfile, "%s", fmt_350);
  for (i = 0; i < fgd.nepars; ++i) {
    j = fgd.idat[i];
    tell(fmt_360,
	 glsgd.lev[glsgd.gam[j].li].name,
	 glsgd.lev[glsgd.gam[j].lf].name,
	 fgd.savedat[i], fgd.saveerr[i], 
	 glsgd.gam[j].e, glsgd.gam[j].de,
	 glsgd.gam[j].e - fgd.savedat[i]);
    if (prfile) fprintf(prfile, fmt_360,
	   glsgd.lev[glsgd.gam[j].li].name,
	   glsgd.lev[glsgd.gam[j].lf].name,
	   fgd.savedat[i], fgd.saveerr[i], 
	   glsgd.gam[j].e, glsgd.gam[j].de,
	   glsgd.gam[j].e - fgd.savedat[i]);
  }
  tell(fmt_450);
  if (prfile) fprintf(prfile, "%s", fmt_450);
  for (i = 0; i < fgd.nipars; ++i) {
    j = fgd.idat2[i];
    tell(fmt_460, glsgd.gam[j].e,
	 glsgd.lev[glsgd.gam[j].li].name,
	 glsgd.lev[glsgd.gam[j].lf].name,
	 fgd.savedat2[i], fgd.saveerr2[i], 
	 glsgd.gam[j].i, glsgd.gam[j].di,
	 glsgd.gam[j].i - fgd.savedat2[i]);
    if (prfile) fprintf(prfile, fmt_460, glsgd.gam[j].e,
	   glsgd.lev[glsgd.gam[j].li].name,
	   glsgd.lev[glsgd.gam[j].lf].name,
	   fgd.savedat2[i], fgd.saveerr2[i], 
	   glsgd.gam[j].i, glsgd.gam[j].di,
           glsgd.gam[j].i - fgd.savedat2[i]);
  }
  return 0;
} /* fit_both */

/* ====================================================================== */
#ifdef ESCL8R
int fit_egam(void)
#else
int fit_egam(int fit_trip)
#endif
{
  static char fmt_350[] = "\n\n"
    "   initial      final     old energy   err.     new energy   err.   new-old\n";
  static char fmt_360[] = "%s %s%15.3f%7.3f%15.3f%7.3f%10.3f\n";

  int   i, j, k, nsave, j1, j2, kk, maxits, jng;
  short jgam[MAXGAM];
  char  ans[80];

  /* fit energies of up to MAXFIT gammas in the level scheme */
  /* ask for gammas to be fitted */

  fgd.npars = 0;
  jng = 0;
  while (1) {
    get_fit_gam(&jng, jgam);
    if (jng == 0) break;
    nsave = fgd.npars;
    for (j = 0; j < jng; ++j) {
      i = jgam[j];
      chk_fit_egam(i, fgd.idat, &fgd.npars, fgd.savedat, fgd.saveerr);
      if (fgd.npars == MAXFIT) break;
    }
    tell("%d gammas included.\n", fgd.npars - nsave);
    if (fgd.npars == MAXFIT) break;
  }
  if (fgd.npars < 1) {
    tell(" Not enough parameters to fit...\n");
    return 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.nfitdat[i] = 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (elgd.levelbr[j][glsgd.gam[i].lf] < 0.05f) continue;
      for (kk = 0; kk < fgd.npars; ++kk) {
	k = fgd.idat[kk];
	if (k == i || k == j) {
	  elgd.ifitdat[i][elgd.nfitdat[i]++] = (short) j;
	  elgd.ifitdat[j][elgd.nfitdat[j]++] = (short) i;
	  break;
	}
      }
    }
  }
  tell("\nParameters are energies for level scheme gammas:");
  for (j = 0; j < fgd.npars; ++j) {
    if (j%8 == 0) tell("\n");
    tell("%8.2f", glsgd.gam[fgd.idat[j]].e);
  }
  tell("\n");
  /* do fit */
  save_gls_now(2); /* save gamma data for possible undo */
  while (1) {
    maxits = 2;
    if ((k = ask(ans, 80, "Max. no. of iterations = ? (rtn for 2)")) &&
	inin(ans, k, &maxits, &j1, &j2)) continue;
    if (maxits <= 0) return 0;
#ifdef ESCL8R
    if (!fitter2d('E', maxits) ||
	!askyn("...Continue fitting? (Y/N)")) break;
#else
    if (fit_trip) {
      if (!fitter3d('E', maxits) ||
	  !askyn("...Continue fitting? (Y/N)")) break;
    } else {
      if (!fitter2d('E', maxits) ||
	  !askyn("...Continue fitting? (Y/N)")) break;
    }
#endif
  }
  tell(fmt_350);
  if (prfile) fprintf(prfile, "%s", fmt_350);
  for (i = 0; i < fgd.npars; ++i) {
    j = fgd.idat[i];
    tell(fmt_360,
	 glsgd.lev[glsgd.gam[j].li].name,
	 glsgd.lev[glsgd.gam[j].lf].name,
	 fgd.savedat[i], fgd.saveerr[i], 
	 glsgd.gam[j].e, glsgd.gam[j].de,
	 glsgd.gam[j].e - fgd.savedat[i]);
    if (prfile) fprintf(prfile, fmt_360,
	   glsgd.lev[glsgd.gam[j].li].name,
	   glsgd.lev[glsgd.gam[j].lf].name,
	   fgd.savedat[i], fgd.saveerr[i], 
           glsgd.gam[j].e, glsgd.gam[j].de,
           glsgd.gam[j].e - fgd.savedat[i]);
  }
  return 0;
} /* fit_egam */

/* ====================================================================== */
#ifdef ESCL8R
int fit_igam(void)
#else
int fit_igam(int fit_trip)
#endif
{
  static char fmt_350[] = "\n\n"
    "  energy    initial      final     old int.  err.    new int.  err.   diff.\n";
  static char fmt_360[] = "%8.2f %s %s%11.2f%7.2f%11.2f%7.2f%8.2f\n";

  int   i, j, k, nsave, j1, j2, kk, fit_it, maxits, jng;
  short jgam[MAXGAM];
  char  ans[80];

  /* fit intensities of up to MAXFIT gammas in the level scheme */
  /* ask for gammas to be fitted */

  fgd.npars = 0;
  jng = 0;
  while (1) {
    get_fit_gam(&jng, jgam);
    if (jng == 0) break;
    nsave = fgd.npars;
    for (j = 0; j < jng; ++j) {
      i = jgam[j];
#ifdef ESCL8R
      chk_fit_igam(i, fgd.idat, &fgd.npars, fgd.savedat, fgd.saveerr);
#else
      chk_fit_igam(i, fgd.idat, &fgd.npars, fgd.savedat, fgd.saveerr, 
		   fit_trip);
#endif
      if (fgd.npars == MAXFIT) break;
    }
    tell("%d gammas included.\n", fgd.npars - nsave);
    if (fgd.npars == MAXFIT) break;
  }
  if (fgd.npars < 1) {
    tell(" Not enough parameters to fit...\n");
    return 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.nfitdat[i] = 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (elgd.levelbr[j][glsgd.gam[i].lf] < 0.003f) continue;
      for (kk = 0; kk < fgd.npars; ++kk) {
	k = fgd.idat[kk];
	fit_it = 0;
	if (k == i) {
	  if (elgd.levelbr[j][glsgd.gam[k].lf] > 0.03f) fit_it = 1;
	} else if (k == j) {
	  if ((1.0f -
	       elgd.levelbr[k][glsgd.gam[k].li] * (glsgd.gam[k].a + 1.0f)) *
	      glsgd.gam[i].i > 0.03f) fit_it = 1;
	} else if (elgd.levelbr[k][glsgd.gam[i].lf] *
		   fabs(elgd.levelbr[j][glsgd.gam[k].li] -
			elgd.levelbr[j][glsgd.gam[k].lf]) > 0.03f) {
	  fit_it = 1;
	}
	if (fit_it) {
	  if (xxgd.lo_ch[i] <= xxgd.lo_ch[j]) {
	    elgd.ifitdat[i][elgd.nfitdat[i]++] = (short) j;
	    if (xxgd.hi_ch[i] > xxgd.hi_ch[j])
	      elgd.ifitdat[j][elgd.nfitdat[j]++] = (short) i;
	  } else {
	    elgd.ifitdat[j][elgd.nfitdat[j]++] = (short) i;
	    if (xxgd.hi_ch[j] > xxgd.hi_ch[i])
	      elgd.ifitdat[i][elgd.nfitdat[i]++] = (short) j;
	  }
	  break;
	}
      }
    }
  }
  tell("\nParameters are intensities for level scheme gammas:");
  for (j = 0; j < fgd.npars; ++j) {
    if (j%8 == 0) tell("\n");
    tell("%8.2f", glsgd.gam[fgd.idat[j]].e);
  }
  tell("\n");
  save_gls_now(2); /* save gamma data for possible undo */
  /* do fit */
  while (1) {
    maxits = 2;
    if ((k = ask(ans, 80, "Max. no. of iterations = ? (rtn for 2)")) &&
	inin(ans, k, &maxits, &j1, &j2)) continue;
    if (maxits <= 0) return 0;
#ifdef ESCL8R
    if (!fitter2d('I', maxits) ||
	!askyn("...Continue fitting? (Y/N)")) break;
#else
    if (fit_trip) {
      if (!fitter3d('I', maxits) ||
	  !askyn("...Continue fitting? (Y/N)")) break;
    } else {
      if (!fitter2d('I', maxits) ||
	  !askyn("...Continue fitting? (Y/N)")) break;
    }
#endif
  }
  tell(fmt_350);
  if (prfile) fprintf(prfile, "%s", fmt_350);
  for (i = 0; i < fgd.npars; ++i) {
    j = fgd.idat[i];
    tell(fmt_360, glsgd.gam[j].e,
	 glsgd.lev[glsgd.gam[j].li].name,
	 glsgd.lev[glsgd.gam[j].lf].name,
	 fgd.savedat[i], fgd.saveerr[i], 
	 glsgd.gam[j].i, glsgd.gam[j].di,
	 glsgd.gam[j].i - fgd.savedat[i]);
    if (prfile) fprintf(prfile, fmt_360, glsgd.gam[j].e,
	   glsgd.lev[glsgd.gam[j].li].name,
	   glsgd.lev[glsgd.gam[j].lf].name,
	   fgd.savedat[i], fgd.saveerr[i], 
	   glsgd.gam[j].i, glsgd.gam[j].di,
           glsgd.gam[j].i - fgd.savedat[i]);
  }
  return 0;
} /* fit_igam */

/* ======================================================================= */
int fit_width_pars(void)
{
  float savepars[3];
  int   i, k, npars, j1, j2, maxits;
  char  ans[80];

  /* fit width parameters to current gate spectrum */
  savepars[0] = elgd.swpars[0];
  savepars[1] = elgd.swpars[1];
  savepars[2] = elgd.swpars[2];
  npars = 0;
  for (i = 0; i < 3; ++i) {
    if (elgd.swpars[i] == 0.0f) break;
    ++npars;
  }
  if (npars < 3)
    tell("NOTE: The value of at least one parameter is zero,\n"
	 "  and these parameters cannot therefore be fitted.\n\n");
  if (npars < 0) {
    warn("The first two parameters should ALWAYS be\n"
	 " nonzero. Change them (command PS) and try again.\n");
    return 0;
  }
  save_esclev_now(1);
  /* do fit */
  while (1) {
    maxits = 20;
    if ((k = ask(ans, 80, "Max. no. of iterations = ? (rtn for 20)")) &&
	inin(ans, k, &maxits, &j1, &j2)) continue;
    if (maxits <= 0) return 0;
    if (!fitterwp(npars, maxits) ||
	!askyn("...Continue fitting? (Y/N)")) break;
  }
  tell(" Old parameter values are: %7.4f %7.4f %7.4f\n"
       "       ... New values are: %7.4f %7.4f %7.4f\n",
       sqrt(savepars[0]), sqrt(savepars[1] * 1e3f),
       sqrt(savepars[2]) * 1e3f, sqrt(elgd.swpars[0]),
       sqrt(elgd.swpars[1] * 1e3f), sqrt(elgd.swpars[2]) * 1e3f);
  if (askyn("  ... Adopt new values? (Y/N)")) {
    tell(" NOTE: The gamma-ray energies and intensities will now change\n"
	 "       if you re-fit them. This may then change the best-fit\n"
	 "       values of the width parameters again.\n\n");
    calc_peaks();
#ifdef ESCL8R
    SAVEPARS;
#else
    read_write_l4d_file("WRITE");
#endif
    return 0;
  }
  elgd.swpars[0] = savepars[0];
  elgd.swpars[1] = savepars[1];
  elgd.swpars[2] = savepars[2];
  calc_peaks();
  return 0;
} /* fit_width_pars */

/* ======================================================================= */
int multiply(char *ans, int nc)
{
  float fact, fj1, fj2;
  int   i, j, ix;
  char   *c;

  /* multiply data by a constant */
  if (ffin(ans + 1, nc - 1, &fact, &fj1, &fj2)) return 0;
  save_esclev_now(4);
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    for (j = 0; j < 4; ++j) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] *= fact;
    }
    for (j = 4; j < 6; ++j) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = xxgd.spec[j][ix] * fact * fact;
    }
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[0][i] *= fact;
  }
  c = xxgd.old_name_gat;
  if (strlen(c) > 28) c += strlen(c) - 28;
  sprintf(xxgd.name_gat, "...%s * %.3f", c, fact);
  return 0;
} /* multiply */

/* ======================================================================= */
int pfind(float *chanx, float *psize, int n1, int n2, int ifwhm,
	  float sigma, int maxpk, int *npk, float spec[6][MAXCHS])
{
  float deno, ctre, root, peak1, peak2, peak3, wings;
  float p1, p2, p3, s2, w4;
  float pc, wingse, w4e, ctr, sum, sum2;
  int   jodd, lock, j, n, j1, j2, nd, jl, jr, np;

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
  j1 = -(2*ifwhm - 1) / 4;
  j2 = j1 + ifwhm - 1;
  jodd = 1 - (ifwhm - (ifwhm / 2 << 1));
  for (nd = n1 + ifwhm; nd < n2 - ifwhm; ++nd) {
    /* sum counts in center and wings of differential function */
    ctr = 0.0f;
    ctre = 0.0f;
    wings = 0.0f;
    wingse = 0.0f;
    for (j = j1; j <= j2; ++j) {
      ctr  += spec[0][nd + j];
      ctre += spec[5][nd + j];
      if (j > 0) {
	wings  = wings  + spec[0][nd - j + j1] + spec[0][nd + j + j2];
	wingse = wingse + spec[5][nd - j + j1] + spec[5][nd + j + j2];
      }
    }
    /* if ifwhm is odd, average data in split cells at ends */
    w4 = 0.0f;
    w4e = 0.0f;
    if (jodd == 0) {
      jl = nd + j1 + j1;
      jr = nd + j2 + j2;
      w4 = (spec[0][jl-1] + spec[0][jl] + spec[0][jr] + spec[0][jr+1]) * 0.25f;
      w4e = (spec[5][jl-1] + spec[5][jl] + spec[5][jr] + spec[5][jr+1]) * 0.25f;
    }
    /* compute height of second derivative (neg) relative to noise */
    s2 = sum;
    sum = ctr - wings - w4;
    root = sqrt(ctre + wingse + w4e);
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
	np = nd;
      }
      if (p3 >= sigma) continue;
      /* estimate location and crude size of peak */
      ++n;
      if (n > maxpk) n = maxpk;
      deno = peak1 - peak2 + peak3 - peak2;
      if (fabs(deno) < 1e-6f) deno = 1e-6f;
      pc = (peak1 - peak3) * 0.5f / deno;
      chanx[n - 1] = (float) np + pc + (float) jodd * 0.5f;
      psize[n - 1] = sum2 + sum2;
      lock = 0;
      peak2 = 0.0f;
    }
    if (p3 >= sigma) lock = 1;
  }
  *npk = n;
  return 0;
} /* pfind */

/* ======================================================================= */
int sum_cur(void)
{
  float x, y;
#ifdef ESCL8R
  int   lo, hi;
#else
  float lo, hi;
#endif
  char  ans[80];

  /* sum results over range specified with cursor */
  tell("Use cursor to specify limits, X to exit.\n");
  while (1) {
    retic(&x, &y, ans);
    if (*ans == 'X' || *ans == 'x') break;
    lo = x;
    retic(&x, &y, ans);
    if (*ans == 'X' || *ans == 'x') break;
    hi = x;
    if (hi < lo) {
      hi = lo;
      lo = x;
    }
#ifdef ESCL8R
    sum_chs(lo, hi);
#else
    sum_eng(lo, hi);
#endif
  }
  return 0;
} /* sum_cur */

/* ====================================================================== */
int rw_saved_spectra(char *ans)
{
  int   i, j;
  char  filnam[80], name[80];

/* ....write/read gamma-ray gate spectra to/from disk file */
  if (strlen(ans) > 2) {
    strcpy(filnam, ans + 2);
  } else {
  START:
   if (!askfn(filnam, 80, "", ".gat", "Gate spectrum file name = ?")) return 1;
  }
  setext(filnam, ".gat", 80);
  if (*ans == 'S') {
    /* save spectra to file */
    filez[1] = open_new_file(filnam, 0);
    if (!filez[1]) return 1;
    fwrite(xxgd.name_gat, 80, 1, filez[1]);
    fwrite(&xxgd.numchs, 4, 1, filez[1]);
    for (i = 0; i < 6; ++i) {
      fwrite(xxgd.spec[i], 4, xxgd.numchs, filez[1]);
    }
    fwrite(&glsgd.ngammas, 4, 1, filez[1]);
    fwrite(elgd.hpk[0], 4, glsgd.ngammas, filez[1]);
  } else {
    /* read spectra from file */
    if (!(filez[1] = open_readonly(filnam))) goto START;
    fread(name, 80, 1, filez[1]);
    fread(&j, 4, 1, filez[1]);
    if (j != xxgd.numchs) {
      warn("Error - saved spectra are %d channels, should be %d channels!\n",
	   j, xxgd.numchs);
      fclose(filez[1]);
      return 1;
    }
    hilite(-1);
    save_esclev_now(4);
    strcpy(xxgd.old_name_gat, xxgd.name_gat);
    strcpy(xxgd.name_gat, name);
    for (i = 0; i < 6; ++i) {
      memcpy(xxgd.old_spec[i], xxgd.spec[i], 4*xxgd.numchs);
      fread(xxgd.spec[i], 4, xxgd.numchs, filez[1]);
    }
    for (i = 0; i < glsgd.ngammas; ++i) {
      elgd.hpk[1][i] = elgd.hpk[0][i];
      elgd.hpk[0][i] = 0.0f;
    }
    fread(&j, 4, 1, filez[1]);
    if (j != glsgd.ngammas)
      tell("%cWarning: Number of gammas has changed;\n"
	   "   ... This may confuse the calculated spectra!\n", (char) 7);
    if (j > glsgd.ngammas) j = glsgd.ngammas;
    fread(elgd.hpk[0], 4, j, filez[1]);
  }
  fclose(filez[1]);
  return 0;
} /* rw_saved_spectra */
