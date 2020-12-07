#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "minig.h"

#ifdef ESCL8R
#include "escl8r.h"
static int reclen;  /* record length of direct-access .esc or .e4k data file */
#define ICHAN(x) (int) (x + 0.5f)
#else
#include "levit8r.h"
#define ICHAN(x) ichanno(x)
#endif

static int gate_label_h_offset = 0;

/* declared in esclev.c */
extern char listnam[55];

/* ======================================================================= */
int calc_peaks(void)
{
#ifdef ESCL8R
  float fjunk;
#else
  int   ich;
#endif
  float beta, fwhm, a, h, r, u, w, x, z, width, h2;
  float r1, u1, y2, u7, u5, u6, eg, deg, pos, y = 0.0f, y1 = 1.0f;
  int   i, j, ix, jx, notail, ipk;

  /* calculate peak shapes and derivatives for gammas in level scheme */
  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.npks_ch[i] = 0;
  }
  for (ipk = 0; ipk < glsgd.ngammas; ++ipk) {
    eg = glsgd.gam[ipk].e;
#ifdef ESCL8R
    x = channo(eg);
    energy(x, 1.0f, &fjunk, &deg);
#else
    x = eg;
    ich = ichanno(eg);
    deg = xxgd.ewid_sp[ich];
#endif
    fwhm = sqrt(elgd.swpars[0] + elgd.swpars[1] * x + elgd.swpars[2] * x * x);
    r = (elgd.finest[0] + elgd.finest[1] * x) / 100.0f;
    r1 = 1.0f - r;
    beta = elgd.finest[2] + elgd.finest[3] * x;
    if (beta == 0.0f) beta = fwhm / 2.0f;
    /* check for low-energy tail and calculate peak position */
    if (r == 0.0f) {
      notail = 1;
      a = 0.0f;
      pos = x;
    } else {
      notail = 0;
      y = fwhm / (beta * 3.33021838f);
      y1 = erfc(y);
      y2 = exp(-y * y) / y1 * 1.12837917f;
      a = exp(-y * y) / y1 * 2.0f * r * beta;
      pos = x + beta * a / (a + fwhm * 1.06446705f * r1);
    }
    width = fwhm * 1.41421356f / 2.35482f;
    h = effic(eg) / sqrt(elgd.h_norm) / (a + fwhm * 1.06446705f * r1);
    /* find channel region over which peak must be calculated */
    xxgd.lo_ch[ipk] = ICHAN(pos - width * 3.1f);
    xxgd.hi_ch[ipk] = ICHAN(pos + width * 3.1f);
    if (!notail) {
      j = ICHAN(pos - beta * 10.0f);
      if (xxgd.lo_ch[ipk] > j) xxgd.lo_ch[ipk] = j;
    }
    if (xxgd.lo_ch[ipk] < 1) xxgd.lo_ch[ipk] = 1;
    if (xxgd.hi_ch[ipk] > xxgd.numchs) xxgd.hi_ch[ipk] = xxgd.numchs;
#ifdef ESCL8R
    if (xxgd.hi_ch[ipk] - xxgd.lo_ch[ipk] > 39) {
      j = (int) (0.75f * x + 0.25f * pos + 19.0f);
      if (xxgd.hi_ch[ipk] > j) xxgd.hi_ch[ipk] = j;
      xxgd.lo_ch[ipk] = xxgd.hi_ch[ipk] - 39;
    }
#else
    if (xxgd.hi_ch[ipk] - xxgd.lo_ch[ipk] > 14) {
      j = ichanno(0.75f * x + 0.25f * pos) + 7.0f;
      if (xxgd.hi_ch[ipk] > j) xxgd.hi_ch[ipk] = j;
      xxgd.lo_ch[ipk] = xxgd.hi_ch[ipk] - 14;
    }
#endif
    /* calculate peak and derivative */
    for (ix = xxgd.lo_ch[ipk]; ix <= xxgd.hi_ch[ipk]; ++ix) {
      /* put this peak into look-up table for this channel */
      xxgd.pks_ch[ix][xxgd.npks_ch[ix]++] = (short) ipk;
      if (xxgd.npks_ch[ix] > 60) {
	warn("Severe error: no. of peaks in ch. %d is greater than 60.\n", ix);
	exit(1);
      }
      /* jx is index for this channel in pk_shape and pk_deriv */
      jx = ix - xxgd.lo_ch[ipk];
#ifdef ESCL8R
      x = (float) ix - pos;
      h2 = h / deg;
#else
      x = xxgd.energy_sp[ix] - pos;
      h2 = h * xxgd.ewid_sp[ix];
#endif
      w = x / width;
      if (fabs(w) > 3.1f) {
	u1 = 0.0f;
      } else {
	u1 = r1 * exp(-w * w);
      }
      u = u1;
      a = u1 * w * 2.0f / width;
      if (notail) {
	xxgd.pk_deriv[ipk][jx] = h2 * a;
      } else {
	z = w + y;
	if (fabs(x / beta) > 12.0f) {
	  xxgd.pk_deriv[ipk][jx] = h2 * a;
	} else {
	  u7 = exp(x / beta) / y1;
	  if (fabs(z) > 3.1f) {
	    u5 = 0.0f;
	    if (z < 0.0f) u5 = 2.0f;
	    u6 = 0.0f;
	  } else {
	    u5 = erfc(z);
	    u6 = exp(-z * z) * 1.12837917f;
	  }
	  u += r * u5 * u7;
	  xxgd.pk_deriv[ipk][jx] = h2 * (a + r * u7 * (u6 - u5 * 2.0f * y) / width);
	}
      }
      /* Should these really be different? */
#ifdef ESCL8R
      xxgd.pk_shape[ipk][jx] = h * u;
      xxgd.w_deriv[ipk][jx]  = h * (a * w * 1.41421356f / 2.35482f - u / fwhm);
#else
      xxgd.pk_shape[ipk][jx] = h2 * u;
      xxgd.w_deriv[ipk][jx]  = h2 * (a * w * 1.41421356f / 2.35482f - u / fwhm);
#endif
    }
  }
  return 0;
} /* calc_peaks */

/* ======================================================================= */
int disp_dat(int iflag)
{
#ifdef ESCL8R
#else
  float xlo, xhi;
#endif
  float r1, dy_s, x, hicnt, x0, y0, dx, dy, y0_s;
  int   icol, i, nx, ny, ny1, loy, ihi, ilo;

  /* display data (gate and calculated gate) */
  /* iflag = 0 to display new data */
  /* 1 to redraw data */
  /* 2 to redraw data for all channels */
  /* 3 to redraw data reusing previous limits */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */

  if (xxgd.numchs < 1) return 1;

  if (iflag < 3) {
#ifdef ESCL8R
    if (iflag == 2 || elgd.numx == 0) {
      elgd.loch = 0;
      elgd.nchs = xxgd.numchs;
    } else {
      elgd.loch = elgd.lox;
      elgd.nchs = elgd.numx;
    }
    elgd.hich = elgd.loch + elgd.nchs - 1;
    if (elgd.hich > xxgd.numchs - 1) elgd.hich = xxgd.numchs - 1;
#else
    xlo = xxgd.elo_sp[0];
    xhi = xxgd.ehi_sp[xxgd.numchs - 1];
    if (iflag == 2 || elgd.numx == 0) {
      elgd.loch = xlo;
      elgd.nchs = xhi - xlo + 1.0f;
    } else {
      elgd.loch = elgd.lox;
      elgd.nchs = elgd.numx;
    }
    elgd.hich = elgd.loch + elgd.nchs - 1;
    if ((float) elgd.hich > xhi) elgd.hich = xhi;
    if ((float) elgd.loch < xlo) elgd.loch = xlo;
#endif
    elgd.nchs = elgd.hich - elgd.loch + 1;
  }

  /* separate routine for displaying more than 2 stacked gates */
  if (elgd.ngd > 2) return disp_many_sp();

  initg(&nx, &ny);
  if (elgd.pkfind && (elgd.disp_calc != 1)) ny -= 30;
  erase();

#ifdef ESCL8R
  ilo = elgd.loch;
  ihi = elgd.hich;
#else
  ilo = ichanno((float) elgd.loch);
  ihi = ichanno((float) elgd.hich);
#endif

  hicnt = (float) (elgd.locnt + elgd.ncnts);
  if (elgd.ncnts <= 0) {
    for (i = ilo; i <= ihi; ++i) {
      if (hicnt < xxgd.spec[0][i]) hicnt = xxgd.spec[0][i];
      if (elgd.disp_calc && hicnt < xxgd.spec[2][i]) {
	hicnt = xxgd.spec[2][i];
      }
    }
    hicnt *= elgd.multy;
    if (elgd.pkfind) {
      if (hicnt < elgd.min_ny / 1.1f) hicnt = elgd.min_ny / 1.1f;
    } else {
      if (hicnt < elgd.min_ny) hicnt = elgd.min_ny;
    }
  }
  x0 = (float) elgd.loch;
  dx = (float) elgd.nchs;
  y0 = (float) elgd.locnt;
  dy = hicnt - (float) (elgd.locnt - 1);
  if (elgd.pkfind) dy *= 1.1f;
  loy = ny / (elgd.ngd * 20) + ny * (elgd.ngd - 1) / elgd.ngd;
  ny1 = ny * 9 / (elgd.ngd * 20);
  if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
  limg(nx, 0, ny1, loy);
  setcolor(1);
  trax(dx, x0, dy, y0, elgd.iyaxis);
  dy_s = dy;
  y0_s = y0;
  icol = elgd.colormap[1];
  setcolor(icol);
#ifdef ESCL8R
  x = (float) ilo;
#else
  x = xxgd.elo_sp[ilo];
#endif
  pspot(x, xxgd.spec[0][ilo]);
  for (i = ilo; i <= ihi; ++i) {
    vect(x, xxgd.spec[0][i]);
#ifdef ESCL8R
    x += 1.0f;
#else
    x = xxgd.ehi_sp[i];
#endif
    vect(x, xxgd.spec[0][i]);
  }
  if (elgd.disp_calc) {
    icol = elgd.colormap[2];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo + 0.5f;
#else
    x = xxgd.energy_sp[ilo];
#endif
    pspot(x, xxgd.spec[2][ilo]);
    for (i = ilo + 1; i <= ihi; ++i) {
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.energy_sp[i];
#endif
      vect(x, xxgd.spec[2][i]);
    }
    icol = elgd.colormap[3];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo + 0.5f;
#else
    x = xxgd.energy_sp[ilo];
#endif
    pspot(x, xxgd.spec[1][ilo]);
    for (i = ilo + 1; i <= ihi; ++i) {
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.energy_sp[i];
#endif
      vect(x, xxgd.spec[1][i]);
    }
  }

  if (elgd.disp_calc == 1 ) {
    /* display difference (observed - calculated) */
    loy = loy + ny1 + ny / (elgd.ngd << 3);
    ny1 = ny / (elgd.ngd << 3);
    y0 = 0.0f;
    dy = dy * 2.5f / 9.0f;
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, 0);
    pspot(x0, y0);
    vect(x0 + dx, y0);
    icol = elgd.colormap[1];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo;
#else
    x = xxgd.elo_sp[ilo];
#endif
    pspot(x, xxgd.spec[0][ilo] - xxgd.spec[2][ilo]);
    for (i = ilo; i <= ihi; ++i) {
      vect(x, xxgd.spec[0][i] - xxgd.spec[2][i]);
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.ehi_sp[i];
#endif
      vect(x, xxgd.spec[0][i] - xxgd.spec[2][i]);
    }
    /* display residuals */
    loy += ny1 << 1;
    y0 = 0.0f;
    dy = 5.0f;
    for (i = ilo; i <= ihi; ++i) {
      r1 = fabs((xxgd.spec[0][ilo] - xxgd.spec[2][ilo]) /
		sqrt(xxgd.spec[5][ilo]));
      if (dy < r1) dy = r1;
    }
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    icol = elgd.colormap[2];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo;
#else
    x = xxgd.elo_sp[ilo];
#endif
    r1 = (xxgd.spec[0][ilo] - xxgd.spec[2][ilo]) / sqrt(xxgd.spec[5][ilo]);
    pspot(x, r1);
    for (i = ilo; i <= ihi; ++i) {
      r1 = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
      vect(x, r1);
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.ehi_sp[i];
#endif
      vect(x, r1);
    }
  }

  /* display gate label */
  setcolor(1);
  mspot(nx - 120 - gate_label_h_offset, loy + ny1);
  putg(xxgd.name_gat, strlen(xxgd.name_gat), 8);

  /* exit if old data not to be displayed */
  if (elgd.ngd <= 1) goto DONE;

  hicnt = (float) (elgd.locnt + elgd.ncnts);
  if (elgd.ncnts <= 0) {
    for (i = ilo; i <= ihi; ++i) {
      if (hicnt < xxgd.old_spec[0][i]) hicnt = xxgd.old_spec[0][i];
      if (elgd.disp_calc && hicnt < xxgd.old_spec[2][i]) {
	hicnt = xxgd.old_spec[2][i];
      }
    }
    hicnt *= elgd.multy;
    if (hicnt < elgd.min_ny) hicnt = elgd.min_ny;
  }
  y0 = (float) elgd.locnt;
  dy = hicnt - (float) (elgd.locnt - 1);
  if (elgd.pkfind) dy *= 1.1f;
  loy = ny / (elgd.ngd * 20);
  ny1 = ny * 9 / (elgd.ngd * 20);
  if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
  limg(nx, 0, ny1, loy);
  setcolor(1);
  trax(dx, x0, dy, y0, elgd.iyaxis);
  icol = elgd.colormap[1];
  setcolor(icol);
#ifdef ESCL8R
  x = (float) ilo;
#else
  x = xxgd.elo_sp[ilo];
#endif
  pspot(x, xxgd.old_spec[0][ilo]);
  for (i = ilo; i <= ihi; ++i) {
    vect(x, xxgd.old_spec[0][i]);
#ifdef ESCL8R
    x += 1.0f;
#else
    x = xxgd.ehi_sp[i];
#endif
    vect(x, xxgd.old_spec[0][i]);
  }
  if (elgd.disp_calc) {
    icol = elgd.colormap[2];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo + 0.5f;
#else
    x = xxgd.energy_sp[ilo];
#endif
    pspot(x, xxgd.old_spec[2][ilo]);
    for (i = ilo + 1; i <= ihi; ++i) {
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.energy_sp[i];
#endif
      vect(x, xxgd.old_spec[2][i]);
    }
    icol = elgd.colormap[3];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo + 0.5f;
#else
    x = xxgd.energy_sp[ilo];
#endif
    pspot(x, xxgd.old_spec[1][ilo]);
    for (i = ilo + 1; i <= ihi; ++i) {
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.energy_sp[i];
#endif
      vect(x, xxgd.old_spec[1][i]);
    }
  }
  if (elgd.disp_calc == 1 ) {
    /* display difference (observed - calculated) */
    loy = loy + ny1 + ny / (elgd.ngd << 3);
    ny1 = ny / (elgd.ngd << 3);
    y0 = 0.0f;
    dy = dy * 2.5f / 9.0f;
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, 0);
    pspot(x0, y0);
    vect(x0 + dx, y0);
    icol = elgd.colormap[1];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo;
#else
    x = xxgd.elo_sp[ilo];
#endif
    pspot(x, xxgd.old_spec[0][ilo] - xxgd.old_spec[2][ilo]);
    for (i = ilo; i <= ihi; ++i) {
      vect(x, xxgd.old_spec[0][i] - xxgd.old_spec[2][i]);
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.ehi_sp[i];
#endif
      vect(x, xxgd.old_spec[0][i] - xxgd.old_spec[2][i]);
    }
    /* display residuals */
    loy += ny1 << 1;
    y0 = 0.0f;
    dy = 5.0f;
    for (i = ilo; i <= ihi; ++i) {
      r1 = fabs((xxgd.old_spec[0][i] - xxgd.old_spec[2][i]) / 
		sqrt(xxgd.old_spec[5][i]));
      if (dy < r1) dy = r1;
    }
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    icol = elgd.colormap[2];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo;
#else
    x = xxgd.elo_sp[ilo];
#endif
    r1 = (xxgd.old_spec[0][ilo] - xxgd.old_spec[2][ilo]) / 
      sqrt(xxgd.old_spec[5][ilo]);
    pspot(x, r1);
    for (i = ilo; i <= ihi; ++i) {
      r1 = (xxgd.old_spec[0][i] - xxgd.old_spec[2][i]) / 
	sqrt(xxgd.old_spec[5][i]);
      vect(x, r1);
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.ehi_sp[i];
#endif
      vect(x, r1);
    }
  }

  /* display gate label and find peaks */
  setcolor(1);
  mspot(nx - 120 - gate_label_h_offset, loy + ny1);
  putg(xxgd.old_name_gat, strlen(xxgd.old_name_gat), 8);
  if (elgd.pkfind) {
    loy = ny / (elgd.ngd * 20);
    ny1 = ny * 9 / (elgd.ngd * 20);
    if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
    y0 = (float) elgd.locnt;
    dy = 1.1f * (hicnt - (float) (elgd.locnt - 1));
    limg(nx, 0, ny1, loy);
    trax(dx, x0, dy, y0, 0);
    findpks(xxgd.old_spec);
  }
 DONE:
  loy = ny / (elgd.ngd * 20) + ny * (elgd.ngd - 1) / elgd.ngd;
  ny1 = ny * 9 / (elgd.ngd * 20);
  if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
  limg(nx, 0, ny1, loy);
  trax(dx, x0, dy_s, y0_s, 0);
  if (elgd.pkfind) findpks(xxgd.spec);
  finig();
  return 0;
} /* disp_dat */

/* ======================================================================= */
int disp_many_sp(void)
{
  static float dspec[10][2][MAXCHS];
  static char  clabel[10][80];
  float dy_s, x, hicnt, x0, y0, dx, dy, y0_s;
  int   icol, i, j, k, nx, ny, ny1, nnd, loy, store = 0, ilo, ihi;

  /* display data (gates and calculated gates) */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */

  if (elgd.n_stored_sp > elgd.ngd - 1) {
    /* have too many stored spectra */
    for (i = 0; i < elgd.ngd - 1; ++i) {
      strcpy(clabel[i], clabel[i+1 + elgd.n_stored_sp - elgd.ngd]);
      for (j = 0; j < 2; ++j) {
	for (k = 0; k < xxgd.numchs; ++k) {
	  dspec[i][j][k] = dspec[i+1 + elgd.n_stored_sp - elgd.ngd][j][k];
	}
      }
    }
    elgd.n_stored_sp = elgd.ngd - 1;
  }

  /* first check to see if need to store another spectrum */
  if (elgd.n_stored_sp > 0) {
    if (strcmp(clabel[elgd.n_stored_sp - 1], xxgd.old_name_gat)) {
      store = 1;
    } else {
      for (k = 0; k < xxgd.numchs; ++k) {
	if (dspec[elgd.n_stored_sp - 1][0][k] != xxgd.old_spec[0][k] ||
	    dspec[elgd.n_stored_sp - 1][1][k] != xxgd.old_spec[2][k]) {
	  store = 1;
	  break;
	}
      }
    }
  }
  if (store) {
    /* store another spectrum */
    if (elgd.n_stored_sp == elgd.ngd - 1) {
      /* first overwrite oldest spectrum */
      for (i = 0; i < elgd.n_stored_sp - 1; ++i) {
	strcpy(clabel[i], clabel[i+1]);
	for (j = 0; j < 2; ++j) {
	  for (k = 0; k < xxgd.numchs; ++k) {
	    dspec[i][j][k] = dspec[i+1][j][k];
	  }
	}
      }
    } else {
      ++elgd.n_stored_sp;
    }
  }

  if (elgd.n_stored_sp == 0) elgd.n_stored_sp = 1;
  strcpy(clabel[elgd.n_stored_sp - 1], xxgd.old_name_gat);
  strcpy(clabel[elgd.n_stored_sp], xxgd.name_gat);
  for (k = 0; k < xxgd.numchs; ++k) {
    dspec[elgd.n_stored_sp - 1][0][k] = xxgd.old_spec[0][k];
    dspec[elgd.n_stored_sp - 1][1][k] = xxgd.old_spec[2][k];
    dspec[elgd.n_stored_sp][0][k] = xxgd.spec[0][k];
    dspec[elgd.n_stored_sp][1][k] = xxgd.spec[2][k];
  }

  /* can now finally display all the data */
  initg(&nx, &ny);
  if (elgd.pkfind) ny += -60;
  erase();
#ifdef ESCL8R
  ilo = elgd.loch;
  ihi = elgd.hich;
#else
  ilo = ichanno((float) elgd.loch);
  ihi = ichanno((float) elgd.hich);
#endif
  for (nnd = 0; nnd <= elgd.n_stored_sp; ++nnd) {
    hicnt = (float) (elgd.locnt + elgd.ncnts);
    if (elgd.ncnts <= 0) {
      for (i = ilo; i <= ihi; ++i) {
	if (hicnt < dspec[nnd][0][i]) hicnt = dspec[nnd][0][i];
	if (elgd.disp_calc &&
	    hicnt < dspec[nnd][1][i]) hicnt = dspec[nnd][1][i];
      }
      hicnt *= elgd.multy;
      if (hicnt < elgd.min_ny) hicnt = elgd.min_ny;
    }
    x0 = (float) elgd.loch;
    dx = (float) elgd.nchs;
    y0 = (float) elgd.locnt;
    dy = hicnt - (float) (elgd.locnt - 1);
    loy = ny / ((elgd.n_stored_sp + 1) * 20) +
      nnd * ny / (elgd.n_stored_sp + 1);
    ny1 = ny * 19 / ((elgd.n_stored_sp + 1) * 20);
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    dy_s = dy;
    y0_s = y0;
    icol = elgd.colormap[1];
    setcolor(icol);
#ifdef ESCL8R
    x = (float) ilo;
#else
    x = xxgd.elo_sp[ilo];
#endif
    pspot(x, dspec[nnd][0][ilo]);
    for (i = ilo; i <= ihi; ++i) {
      vect(x, dspec[nnd][0][i]);
#ifdef ESCL8R
      x += 1.0f;
#else
      x = xxgd.ehi_sp[i];
#endif
      vect(x, dspec[nnd][0][i]);
    }
    if (elgd.disp_calc) {
      icol = elgd.colormap[2];
      setcolor(icol);
#ifdef ESCL8R
      x = (float) ilo + 0.5f;
#else
      x = xxgd.energy_sp[ilo];
#endif
      pspot(x, dspec[nnd][1][ilo]);
      for (i = ilo + 1; i <= ihi; ++i) {
#ifdef ESCL8R
	x += 1.0f;
#else
	x = xxgd.energy_sp[i];
#endif
	vect(x, dspec[nnd][1][i]);
      }
    }
    /* display gate label */
    setcolor(1);
    mspot(nx - 120 - gate_label_h_offset, loy + ny1);
    putg(clabel[nnd], strlen(clabel[nnd]), 8);
  }
  finig();
  if (elgd.pkfind) findpks(xxgd.spec);
  return 0;
} /* disp_many_sp */

/* ====================================================================== */
int gate_sum_list(char *ans)
{
#ifdef ESCL8R
  float eg1, eg2, deg, a1;
  int   i1, ii, jj, iy, ihi, ilo;
#else
  float elo1, ehi1;
#endif
  float r1, x, listx[40], wfact, dx, ms_err;
  int   numx, i, j, nc, ilist, ix;

  /* sum gates on list of gammas */
  if (((nc = strlen(ans) - 2) <= 0 ||
       get_list_of_gates(ans + 2, nc, &numx, listx, &wfact)) &&
      (!(nc = ask(ans, 80, "Gate list = ?")) ||
       get_list_of_gates(ans, nc, &numx, listx, &wfact))) return 1;
  if (numx == 0) return 1;
  save_esclev_now(4);
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  sprintf(xxgd.name_gat, "LIST %.9s", ans);
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = 0.0f;
    }
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }

#ifdef ESCL8R
  for (ilist = 0; ilist < numx; ++ilist) {
    x = channo(listx[ilist]);
    dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		      elgd.swpars[2] * x * x) / 2.0f;
    ilo = (int) (0.5f + x - dx);
    ihi = (int) (0.5f + x + dx);
    if (ihi < 0 || ilo >= xxgd.numchs) return 1;
    if (ilo < 0) ilo = 0;
    if (ihi >= xxgd.numchs) ihi = xxgd.numchs - 1;
    energy((float) ilo - 0.5f, 0.0f, &eg1, &deg);
    energy((float) ihi + 0.5f, 0.0f, &eg2, &deg);
    tell("Gate is chs %i to %i, %.1f\n"
	 " Gate includes level scheme gammas:\n",
	 ilo, ihi, (eg1 + eg2) / 2.0f);
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (glsgd.gam[j].e >= eg1 && glsgd.gam[j].e <= eg2) {
	hilite(j);
	tell("  Gamma #%4i   E = %7.2f   I = %6.2f,  from %s to %s\n",
	     j+1, glsgd.gam[j].e, glsgd.gam[j].i,
	     glsgd.lev[glsgd.gam[j].li].name,
	     glsgd.lev[glsgd.gam[j].lf].name);
      }
    }
    /* read rows from matrix */
    for (iy = ilo; iy <= ihi; ++iy) {
      fseek(filez[2], iy*reclen, SEEK_SET);
      fread(xxgd.rdata, 4, xxgd.numchs, filez[2]);
      for (ix = 0; ix < xxgd.numchs; ++ix) {
	xxgd.spec[0][ix] += xxgd.rdata[ix];
	xxgd.spec[3][ix] += xxgd.rdata[ix];
	/* subtract background */
	xxgd.spec[0][ix] -= (xxgd.bspec[0][iy] * xxgd.bspec[1][ix] +
			     xxgd.bspec[1][iy] * xxgd.bspec[2][ix] +
			     xxgd.bspec[3][iy] * xxgd.bspec[4][ix] +
			     xxgd.bspec[4][iy] * xxgd.bspec[5][ix]);
	if (abs(ix - iy) < xxgd.v_width) {
	  r1 = (float) (ix - iy) / xxgd.v_width;
	  xxgd.spec[0][ix] += xxgd.v_depth[iy] * (1.0f - r1 * r1);
	}
      }
      if (xxgd.esc_file_flags[2]) {
	fseek(filez[2], (iy + xxgd.numchs + 7)*reclen, SEEK_SET);
	fread(xxgd.rdata2, 4, xxgd.numchs, filez[2]);
	for (ix = 0; ix < xxgd.numchs; ++ix) {
	  xxgd.spec[4][ix] += xxgd.rdata2[ix];
	}
      }
      /* calculate expected spectrum */
      /* spec[1][] = calculated spectrum from gammas outside gate */
      /* spec[2][] = calculated spectrum from all gammas */
      for (i1 = 0; i1 < xxgd.npks_ch[iy]; ++i1) {
	i = xxgd.pks_ch[iy][i1];
	ii = iy - xxgd.lo_ch[i];
	for (j = 0; j < glsgd.ngammas; ++j) {
	  if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	    a1 = elgd.levelbr[j][glsgd.gam[i].lf] *
	      xxgd.pk_shape[i][ii] * glsgd.gam[i].i;
	    elgd.hpk[0][j] += a1;
	    for (ix = xxgd.lo_ch[j]; ix <= xxgd.hi_ch[j]; ++ix) {
	      jj = ix - xxgd.lo_ch[j];
	      xxgd.spec[2][ix] += a1 * xxgd.pk_shape[j][jj];
	      if (glsgd.gam[i].e < eg1 || glsgd.gam[i].e > eg2)
		xxgd.spec[1][ix] += a1 * xxgd.pk_shape[j][jj];
	    }
	  } else if (elgd.levelbr[i][glsgd.gam[j].lf] != 0.0f) {
	    a1 = elgd.levelbr[i][glsgd.gam[j].lf] *
	      xxgd.pk_shape[i][ii] * glsgd.gam[j].i;
	    elgd.hpk[0][j] += a1;
	    for (ix = xxgd.lo_ch[j]; ix <= xxgd.hi_ch[j]; ++ix) {
	      jj = ix - xxgd.lo_ch[j];
	      xxgd.spec[2][ix] += a1 * xxgd.pk_shape[j][jj];
	      if (glsgd.gam[i].e < eg1 || glsgd.gam[i].e > eg2)
		xxgd.spec[1][ix] += a1 * xxgd.pk_shape[j][jj];
	    }
	  }
	}
      }
    }
  }
  for (i = 0; i < xxgd.numchs; ++i) {
    if (!xxgd.esc_file_flags[2]) xxgd.spec[4][i] = xxgd.spec[3][i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
#else
  xxgd.bf1 = 0.0f;
  xxgd.bf2 = 0.0f;
  xxgd.bf4 = 0.0f;
  xxgd.bf5 = 0.0f;
  for (ilist = 0; ilist < numx; ++ilist) {
    x = listx[ilist];
    dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		       elgd.swpars[2] * x * x) / 2.0f;
    elo1 = x - dx;
    ehi1 = x + dx;
    add_gate(elo1, ehi1);
  }
  /* subtract background */
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    xxgd.spec[0][ix] = xxgd.spec[0][ix] - xxgd.bf1 * xxgd.bspec[0][ix] -
      xxgd.bf2 * xxgd.bspec[1][ix] - xxgd.bf4 * xxgd.bspec[3][ix] -
      xxgd.bf5 * xxgd.bspec[4][ix];
  }
  /* correct for width of channel in keV */
  for (i = 0; i < xxgd.numchs; ++i) {
    for (j = 0; j < 4; ++j) {
      xxgd.spec[j][i] /= xxgd.ewid_sp[i];
    }
    if (!xxgd.many_cubes) xxgd.spec[4][i] = xxgd.spec[3][i] / xxgd.ewid_sp[i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
#endif

  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
  }
  ms_err /= (float) xxgd.numchs;
  tell(" Mean square error on calculation = %.2f\n", ms_err);
  return 0;
} /* gate_sum_list */

/* ======================================================================= */
int get_cal(void)
{
#ifdef ESCL8R
  float  a, b, c, d, e, f;
#else
  double gain[6], f1, f2, x1, x2, x3;
  float  eg, eff;
  int    contract, j, j1, j2;
#endif
  float  eff_pars[10], x, fj1, fj2;
  int    i, nc, jj, iorder, nterms;
  char   title[80], filnam[80], dattim[20], ans[80];

  save_esclev_now(1);
  /* get new energy and efficiency calibrations */
  /* get new energy calibration */
  while (1) {
#ifdef ESCL8R
    askfn(filnam, 80, "", "",
	  "Energy calibration file for original matrix = ?\n"
	  "                     (default .ext = .aca, .cal)");
    if (!read_cal_file(filnam, title, &iorder, xxgd.gain)) break;
#else
    askfn(filnam, 80, "", "",
	  "Energy calibration file for original linear data = ?\n"
	  "                     (default .ext = .aca, .cal)");
    if (read_cal_file(filnam, title, &iorder, gain)) break;
#endif
  }
  nterms = iorder + 1;
  datetime(dattim);
  fprintf(filez[26], "%s: Energy calibration file = %s\n", dattim, filnam);
  fflush(filez[26]);

#ifdef ESCL8R
  xxgd.nterms = nterms;
#else
  while (1) {
    contract = 2;
    if ((nc = ask(ans, 80, "Contraction factor between this calibration\n"
		  "  and the ADC data on tape = ?\n"
		  "   (e.g. if the ADC is 16384 chs, but your calibration\n"
		  "    spectrum is 4096 chs, then you should enter 4 here.)\n"
		  "   (rtn for 2)")))
      inin(ans, nc, &contract, &j1, &j2);
    if (contract > 0) break;
    warn1("ERROR -- Illegal value, try again...\n");
  }
  fprintf(filez[26], "     ...Contraction factor = %i\n", contract);
#endif

  /* get efficiency calibration */
  while (1) {
    askfn(filnam, 80, "", "",
	  "Efficiency parameter file = ? (default .ext = .aef, .eff)");
    if (!read_eff_file(filnam, title, eff_pars)) break;
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Efficiency calibration file = %s\n", dattim, filnam);
  fflush(filez[26]);

#ifdef ESCL8R
  for (i = 0; i < 9; ++i) {
    xxgd.eff_pars[i] = eff_pars[i];
  }
  /* if necessary, contract gain */
  if (xxgd.esc_file_flags[0] != 1) {
    a = xxgd.gain[xxgd.nterms - 1];
    b = 0.0f;
    c = 0.0f;
    d = 0.0f;
    e = 0.0f;
    f = 0.0f;
    for (jj = xxgd.nterms - 1; jj >= 1; --jj) {
      a = a / 2.0f + xxgd.gain[jj - 1];
      b += (float) jj * 2.0f * xxgd.gain[jj];
    }
    c = xxgd.gain[2] * 4.0f;
    if (xxgd.nterms > 3) {
      c += xxgd.gain[3] * 6.0f;
      d = xxgd.gain[3] * 8.0f;
      if (xxgd.nterms > 4) {
	c += xxgd.gain[4] * 6.0f;
	d += xxgd.gain[4] * 16.0f;
	e = xxgd.gain[4] * 16.0f;
	if (xxgd.nterms > 5) {
	  c += xxgd.gain[5] * 5.0f;
	  d += xxgd.gain[5] * 20.0f;
	  e += xxgd.gain[5] * 40.0f;
	  f = xxgd.gain[5] * 32.0f;
	}
      }
    }
    xxgd.gain[0] = a;
    xxgd.gain[1] = b;
    xxgd.gain[2] = c;
    xxgd.gain[3] = d;
    xxgd.gain[4] = e;
    xxgd.gain[5] = f;
  }
#else
  /* calculate energy and efficiency spectra */
  jj = 0;
  for (i = 0; i < xxgd.nclook; ++i) {
    if (xxgd.looktab[i] != jj) {
      x = ((float) i - 0.5f) / (float) contract -
	  ((float) contract / 2.0f - 0.5f);
      eg = gain[nterms - 1];
      for (j = nterms - 1; j >= 1; --j) {
	eg = gain[j - 1] + eg * x;
      }
      if (jj != 0) xxgd.ehi_sp[jj - 1] = eg;
      jj = xxgd.looktab[i];
      if (jj != 0) xxgd.elo_sp[jj - 1] = eg;
    }
  }
  if (xxgd.looktab[xxgd.nclook - 1] != 0) {
    x = ((float) xxgd.nclook - 1.0f);
    eg = gain[nterms - 1];
    for (j = nterms - 1; j >= 1; --j) {
      eg = gain[j - 1] + eg * x;
    }
    xxgd.ehi_sp[xxgd.looktab[xxgd.nclook - 1] - 1] = eg;
  }
  for (i = 0; i < xxgd.numchs; ++i) {
    eg = (xxgd.elo_sp[i] + xxgd.ehi_sp[i]) / 2.0f;
    x1 = log(eg / eff_pars[7]);
    x2 = log(eg / eff_pars[8]);
    f1 = eff_pars[0] + eff_pars[1] * x1 + eff_pars[2] * x1 * x1;
    f2 = eff_pars[3] + eff_pars[4] * x2 + eff_pars[5] * x2 * x2;
    if (f1 <= 0. || f2 <= 0.) {
      eff = 1.0f;
    } else {
      x3 = exp(-eff_pars[6] * log(f1)) + exp(-eff_pars[6] * log(f2));
      if (x3 <= 0.) {
	eff = 1.0f;
      } else {
	eff = exp(exp(-log(x3) / eff_pars[6]));
      }
    }
    xxgd.eff_sp[i] = eff;
    xxgd.energy_sp[i] = eg;
    xxgd.ewid_sp[i] = xxgd.ehi_sp[i] - xxgd.elo_sp[i];
  }
#endif

  while ((nc = ask(ans, 80,
		   "This program adds systematic errors for\n"
		   " the energy and efficiency calibrations.\n"
		   "Error for energy calibration in keV = ? (rtn for %.3f)",
		   elgd.encal_err)) &&
	 ffin(ans, nc, &elgd.encal_err, &fj1, &fj2));
  while ((nc = ask(ans, 80,
		   "Percentage error on efficiency = ? (rtn for %.1f)",
		   elgd.effcal_err * 100.0f)) &&
	 ffin(ans, nc, &x, &fj1, &fj2));
  if (nc > 0) elgd.effcal_err = x / 100.0f;
  datetime(dattim);
  fprintf(filez[26], "%s: Error for energy calibration = %f\n",
	  dattim, elgd.encal_err);
  fprintf(filez[26], "%s: Percentage error on efficiency = %f\n",
	  dattim, elgd.effcal_err * 100.0f);
  fflush(filez[26]);
  return 0;
} /* get_cal */

/* ======================================================================= */
int get_list_of_gates(char *ans, int nc, int *outnum, float *outlist,
		      float *wfact)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */
  float eg, fj1, fj2, tmp, elo, ehi;
  int   i, j, k, nl;

  /* read Egamma / list_name and width factor from ANS */
  /* return list of energies OUTLIST with OUTNUM energies */

  *outnum = 0;
  if (*ans == ' ') {
    memmove(ans, ans + 1, nc);
    nc--;
  }
  if (nc < 1) return 0;

#ifdef ESCL8R
  energy(0.0, 1.0f, &elo, &tmp);
  energy((float) (xxgd.numchs - 1), 1.0f, &ehi, &tmp);
#else
  elo = xxgd.energy_sp[0];
  ehi = xxgd.energy_sp[xxgd.numchs - 1];
#endif

  for (i = 0; i < 55; ++i) {
    if (*ans == listnam[i]) {
      /* list name found */
      nl = i;
      *wfact = 1.0f;
      if (nc >= 3 &&
	  ffin(ans + 2, nc - 2, wfact, &fj1, &fj2)) return 1;
      if (*wfact <= 0.0f) *wfact = 1.0f;
      for (j = 0; j < elgd.nlist[nl]; ++j) {
	eg = elgd.list[nl][j];
	if (eg >= elo && eg <= ehi) outlist[(*outnum)++] = eg;
      }
      /* order the energies in the list */
      for (j = 0; j < *outnum - 1; ++j) {
	if (outlist[j] > outlist[j + 1]) {
	  tmp = outlist[j];
	  outlist[j] = outlist[j + 1];
	  outlist[j + 1] = tmp;
	  for (k = j - 1; k >= 0 && outlist[k] > outlist[k + 1]; k--) {
	    tmp = outlist[k];
	    outlist[k] = outlist[k + 1];
	    outlist[k + 1] = tmp;
	  }
	}
      }
      return 0;
    }
  }

  /* list name not found */
  if (ffin(ans, nc, &eg, wfact, &fj2) || eg < elo || eg > ehi) return 1;
  if (*wfact <= 0.0f) *wfact = 1.0f;
  *outnum = 1;
  outlist[0] = eg;
  return 0;
} /* get_list_of_gates */

/* ======================================================================= */
int get_shapes(void)
{
  float rj2, sw1, sw2, sw3, newp[4];
  int   i, k, nc;
  char  dattim[20], ans[80];

  save_esclev_now(1);
  /* get new peak shape and FWHM parameters */
#ifdef ESCL8R
  if (xxgd.esc_file_flags[0] != 1) {
    elgd.finest[0] *= 2.0f;
    elgd.finest[2] *= 2.0f;
    elgd.swpars[0] *= 4.0f;
    elgd.swpars[1] *= 2.0f;
  }
#else
  elgd.finest[0] *= 2.0f;
  elgd.finest[2] *= 2.0f;
  elgd.swpars[0] *= 4.0f;
  elgd.swpars[1] *= 2.0f;
#endif
  newp[0] = 0.0f;
  newp[1] = 0.0f;
  newp[2] = 1.0f;
  newp[3] = 0.0f;
  if (!askyn("Non-Gaussian peak shapes can be used,\n"
	     "   but are not usually necessary.\n"
	     "...Use non-Gaussian peak shapes? (Y/N)")) {
    datetime(dattim);
    fprintf(filez[26], "%s: Gaussian peak shapes selected.\n", dattim);
    fflush(filez[26]);
  } else {
    while ((k = ask(ans, 80,
#ifdef ESCL8R
		    " In the the following, channel numbers always refer to"
		    " the original matrix.\n\n"
#else
		    " In the the following, the energy dispersion is assumed"
		    " to be 0.5 keV/ch.\n\n"
#endif
		    " The parameters R and BETA define the shapes of the peaks.\n"
		    "        The peak is the sum of a gaussian of height H*(1-R/100)\n"
		    "        and a skew gaussian of height H*R/100, where BETA is \n"
		    "        the decay constant of the skew gaussian (in channels).\n\n"
		    " R is taken as R = A + B*x    (x = ch. no.)\n"
		    "Enter A,B (rtn for %.1f, %.5f)",
		    elgd.finest[0], elgd.finest[1])) &&
	   ffin(ans, k, &newp[0], &newp[1], &rj2));
    if (!k) {
      newp[0] = elgd.finest[0];
      newp[1] = elgd.finest[1];
    }
    while ((k = ask(ans, 80,
		    " BETA is taken as BETA = C + D*x  (x = ch. no.)\n"
		    "Enter C,D (rtn for %.1f, %.5f)",
		    elgd.finest[2], elgd.finest[3])) &&
	   ffin(ans, k, &newp[2], &newp[3], &rj2));
    if (!k) {
      newp[2] = elgd.finest[2];
      newp[3] = elgd.finest[3];
    }
    datetime(dattim);
    fprintf(filez[26], "%s: Non-gaussian peak shapes: %f %f %f %f\n",
	    dattim, newp[0], newp[1], newp[2], newp[3]);
    fflush(filez[26]);
  }

  for (i = 0; i < 4; ++i) {
    elgd.finest[i] = newp[i];
  }
  while (1) {
    if (!(nc = ask(ans, 80,
		   " The fitted peak widths are taken as:\n"
		   "     FWHM = SQRT(F*F + G*G*x + H*H*x*x)  (x = ch.no./1000)\n"
		   "Enter F,G,H (rtn for %.2f, %.2f, %.2f)",
		   sqrt(elgd.swpars[0]),  sqrt(elgd.swpars[1] * 1e3f),
		   sqrt(elgd.swpars[2]) * 1e3f))) break;
    if (ffin(ans, nc, &sw1, &sw2, &sw3)) continue;
    if (sw1 > 0.0f && sw2 >= 0.0f) {
      elgd.swpars[0] = sw1 * sw1;
      elgd.swpars[1] = sw2 * sw2 / 1e3f;
      elgd.swpars[2] = sw3 * sw3 / 1e6f;
      break;
    }
    warn1("F must be > 0, G must be >= 0.\n");
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Peak width parameters = %f %f %f\n",
	  dattim, sqrt(elgd.swpars[0]),
	  sqrt(elgd.swpars[1] * 1e3f), sqrt(elgd.swpars[2]) * 1e3f);
  fflush(filez[26]);
  /* if necessary, contract gain */
#ifdef ESCL8R
  if (xxgd.esc_file_flags[0] != 1) {
    elgd.finest[0] /= 2.0f;
    elgd.finest[2] /= 2.0f;
    elgd.swpars[0] /= 4.0f;
    elgd.swpars[1] /= 2.0f;
  }
#else
  elgd.finest[0] *= 0.5f;
  elgd.finest[2] *= 0.5f;
  elgd.swpars[0] *= 0.25f;
  elgd.swpars[1] *= 0.5f;
#endif

  return 0;
} /* get_shapes */

/* ======================================================================= */
int num_gate(char *ans, int nc)
{
#ifdef ESCL8R
  int   ihi, ilo;
#endif
  float x, wfact, eg, dx, fj2, elo, ehi;

  /* calculate gate channels ilo, ihi from specified Egamma
     and width_factor * parameterized_FWHM */

#ifdef ESCL8R
  energy(0.0, 1.0f, &elo, &dx);
  energy((float) (xxgd.numchs - 1), 1.0f, &ehi, &dx);
#else
  elo = xxgd.energy_sp[0];
  ehi = xxgd.energy_sp[xxgd.numchs - 1];
#endif

  if (ffin(ans, nc, &eg, &wfact, &fj2) || eg < elo || eg > ehi) return 1;
  save_esclev_now(4);
  if (wfact <= 0.0f) wfact = 1.0f;
#ifdef ESCL8R
  x = channo(eg);
  dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		    elgd.swpars[2] * x * x) / 2.0f;
  ilo = (int) (0.5f + x - dx);
  ihi = (int) (0.5f + x + dx);
  if (ihi < 0 || ilo >= xxgd.numchs) return 1;
  if (ilo < 0) ilo = 0;
  if (ihi >= xxgd.numchs) ihi = xxgd.numchs - 1;
  get_gate(ilo, ihi);
#else
  x = eg;
  dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		    elgd.swpars[2] * x * x) / 2.0f;
  elo = x - dx;
  ehi = x + dx;
  get_gate(elo, ehi);
#endif

  return 0;
} /* num_gate */

/* ======================================================================= */
int project(void)
{
  float r1, fact, a1, a2, s1, s2, ms_err;
  int   i, j, ii, ix;

  /* set displayed spectrum to background-subtracted projection */
  save_esclev_now(4);
  s1 = 0.0f;
  s2 = 0.0f;
  for (i = 0; i < xxgd.numchs; ++i) {
    s1 += xxgd.bspec[0][i];
    s2 += xxgd.bspec[2][i];
    for (j = 0; j < 6; ++j) {
      xxgd.old_spec[j][i] = xxgd.spec[j][i];
      xxgd.spec[j][i] = 0.0f;
    }
    xxgd.spec[0][i] = xxgd.bspec[2][i];
    xxgd.spec[3][i] = xxgd.bspec[0][i];
#ifdef ESCL8R
    xxgd.spec[4][i] = xxgd.bspec[0][i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
#endif
  }
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  strcpy(xxgd.name_gat, "Projection");
  /* calculate expected spectrum */
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    a1 = effic(glsgd.gam[i].e) * glsgd.gam[i].i / sqrt(elgd.h_norm);
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	elgd.hpk[0][j] += a1 * elgd.levelbr[j][glsgd.gam[i].lf];
	a2 = effic(glsgd.gam[j].e) * glsgd.gam[i].i / sqrt(elgd.h_norm);
	elgd.hpk[0][i] += a2 * elgd.levelbr[j][glsgd.gam[i].lf];
      }
    }
  }
  fact = s1 / s2;
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[0][i] *= fact;
    for (ix = xxgd.lo_ch[i]; ix <= xxgd.hi_ch[i]; ++ix) {
      ii = ix - xxgd.lo_ch[i];
      xxgd.spec[2][ix] += elgd.hpk[0][i] * xxgd.pk_shape[i][ii];
    }
  }

#ifdef ESCL8R
#else
  /* correct for width of channel in keV */
  for (i = 0; i < xxgd.numchs; ++i) {
    for (j = 0; j < 4; ++j) {
      xxgd.spec[j][i] /= xxgd.ewid_sp[i];
    }
    xxgd.spec[4][i] = xxgd.spec[3][i] / xxgd.ewid_sp[i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
#endif

  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
  }
  ms_err /= (float) xxgd.numchs;
  tell("Mean square error on calculation = %.2f\n", ms_err);
  disp_dat(0);
  return 0;
} /* project */

/* ======================================================================= */
int wspec2(float *spec, char *filnam, char sp_nam_mod, int expand)
{
#ifdef ESCL8R
#else
  float spec2[16384];
  int   i;
#endif
  float *sp;
  int   numchs, jext, rl = 0, c1 = 1;
  char  namesp[8], buf[32];

  /* write spectrum to disk file */
  /* default file extension = .spe */
  /* expand is ignored in escl8r;
     provided only for consistency with levit8r / 4dg8r */
  jext = setext(filnam, ".spe", 80);
  filnam[jext-1] = sp_nam_mod;
  if (jext > 8) {
    strncpy(namesp, filnam, 8);
  } else {
    memset(namesp, ' ', 8);
    strncpy(namesp, filnam, jext);
  }
  if (!(filez[1] = open_new_file(filnam, 0))) return 1;

#ifdef ESCL8R
  sp = spec;
  numchs = xxgd.numchs;
#else
  if (expand) {
    for (i = 0; i < xxgd.nclook; ++i) {
      numchs = xxgd.looktab[i];
      if (numchs != 0) {
	spec2[i] = spec[numchs - 1];
      } else {
	spec2[i] = 0.0f;
      }
    }
    sp = spec2;
    numchs = xxgd.nclook;
  } else {
    sp = spec;
    numchs = xxgd.numchs;
  }
#endif

#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(namesp,8); W(&numchs,4); W(&c1,4); W(&c1,4); W(&c1,4);
#undef W
  if (put_file_rec(filez[1], buf, rl) ||
      put_file_rec(filez[1], sp, 4*numchs)) {
    warn("ERROR: could not write to file %s\n"
	 " ...spectrum not written.\n", filnam);
    fclose(filez[1]);
    return 1;
  }
  fclose(filez[1]);
  tell("Spectrum written as %s\n", filnam);
  return 0;
} /* wspec2 */

/* ======================================================================= */
int write_sp(char *ans, int nc)
{
  float spec[MAXCHS];
  int   i, expand = 0;
  char  filnam[80];

#ifdef ESCL8R
#else
  expand = askyn("Expand spectra to linear ADC channels? (Y/N)");
#endif
  strcpy(filnam, ans+2);
  nc -= 2;
  /* ask for output file name */
  if (nc < 1 &&
      (nc = ask(filnam, 70, "Output spectrum name = ?")) == 0) return 0;
  strcpy(filnam + nc, "e");
  if (askyn("Write observed spectrum? (Y/N)"))
    wspec2(xxgd.spec[0], filnam, 'e', expand);
  if (askyn("Write unsubtracted spectrum? (Y/N)"))
    wspec2(xxgd.spec[3], filnam, 'u', expand);
  if (askyn("Write calculated spectrum? (Y/N)"))
    wspec2(xxgd.spec[2], filnam, 'c', expand);
  if (askyn("Write difference spectrum? (Y/N)")) {
    for (i = 0; i < xxgd.numchs; ++i) {
      spec[i] = xxgd.spec[0][i] - xxgd.spec[2][i];
    }
    wspec2(spec, filnam, 'd', expand);
  }
  if (askyn("Write residual spectrum? (Y/N)")) {
    for (i = 0; i < xxgd.numchs; ++i) {
      spec[i] = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
    }
    wspec2(spec, filnam, 'r', expand);
  }
  return 0;
} /* write_sp */

/* ======================================================================= */
float effic(float eg)
{
#ifdef ESCL8R
  float  ret_val;
  double f1, f2, x1, x2, x3;

  x1 = log(eg / xxgd.eff_pars[7]);
  x2 = log(eg / xxgd.eff_pars[8]);
  f1 = xxgd.eff_pars[0] + xxgd.eff_pars[1] * x1 + xxgd.eff_pars[2] * x1 * x1;
  f2 = xxgd.eff_pars[3] + xxgd.eff_pars[4] * x2 + xxgd.eff_pars[5] * x2 * x2;
  if (f1 <= 0. || f2 <= 0.) {
    ret_val = 1.0f;
  } else {
    x3 = exp(-xxgd.eff_pars[6] * log(f1)) + exp(-xxgd.eff_pars[6] * log(f2));
    if (x3 <= 0.) {
      ret_val = 1.0f;
    } else {
      ret_val = exp(exp(-log(x3) / xxgd.eff_pars[6]));
    }
  }
  return ret_val;
#else
  return xxgd.eff_sp[ichanno(eg)];
#endif
} /* effic */

/* ====================================================================== */

void set_gate_label_h_offset(int offset)
{
  gate_label_h_offset = offset;
} /* set_gate_label_h_offset */
