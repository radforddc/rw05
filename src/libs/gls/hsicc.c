#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

struct {
  float a[23], b[4], c[23], x[23], y[23], ee[23], alpha[23];
  float e1, e2;
  int   ns, l, n,  np;
} hgd;

int iloop(void);
int swap_bytes_i(int *);
int swap_bytes_r(float *);
int fit(void);
int spline(int, int);

int hsicc(int zin, float *egam, int ngam, float *alphat)
{
  static float ek[100], data[9], etab[10][23];
  static float alphe[8][10][23], alftab[8][10][23];
  static float tcc[8], tcl[8], tcm[8], tle[8], tme[8];
  static int zold = 0, first = 1;
  static int i, j, k, m, ztab, tblkey, igf;
  static int jj, kk, npold, swapbytes, ie, z, ioffset;
  static int skip[10], netab[10];
  static char shtab[4], shold[4];
  static char fullname[200];
  int  n, idx = 0, gammanum = 0, ish = 0;
  FILE *file1, *file2;

  if (first) {
    first = 0;
    memset(hgd.a, 0, 148*4);
  }

  z = zin;
  if (z == -1) z = zold;
  if (z > 103 || z < 14) return 1;
  if (z == zold) goto NEXTDATA;

  /* open icc table data files */
  ioffset = get_directory("RADWARE_ICC_LOC", fullname, 200);
  strcat(fullname, "iccndx.dat");
  file1 = open_readonly(fullname);
  strcpy(fullname+ioffset, "icctbl.dat");
  if (!file1 || !(file2 = open_readonly(fullname))) {
    warn("  -- Check that the environment variable or logical name\n"
	 "    RADWARE_ICC_LOC is properly defined, and that the files\n"
	 "    iccndx.dat and icctbl.dat exist in the directory\n"
	 "    to which it points.\n");
    exit(-1);
  }

  /* check to see if files have wrong byte-order.... */
  fseek(file1, 200, SEEK_SET);
  fread(&tblkey, 1, 4, file1);
  swapbytes = (tblkey > 10000 || tblkey < 0);

  /* INITIALIZE READ AND CALCULATIONAL VARIABLES */
  for (j = 0; j < 10; ++j) {
    netab[j] = 0;
    for (i = 0; i < 23; ++i) {
      etab[j][i] = 0.f;
      for (k = 0; k < 8; ++k) {
	alftab[k][j][i] = 0.f;
	alphe[k][j][i] = 0.f;
      }
    }
  }
  for (i = 0; i < 23; ++i) {
    hgd.alpha[i] = 0.f;
    hgd.ee[i] = 0.f;
  }
  hgd.n = 0;
  strncpy(shold, "  ", 2);
  fseek(file1, (z-1)*4, SEEK_SET);
  fread(&tblkey, 1, 4, file1);
  if (swapbytes) swap_bytes_i(&tblkey);
  if (tblkey == 0) return 1;
  ie = 0;

  /* read conversion coeff. tables from icctbl.dat.... */
  while (1) {
    fseek(file2, (tblkey-1)*44, SEEK_SET);
    fread(&ztab, 1, 4, file2);
    fread(shtab, 1, 2, file2);
    fread(data, 9, 4, file2);
    if (swapbytes) {
      swap_bytes_i(&ztab);
      for (i = 0; i < 9; ++i) swap_bytes_r(&data[i]);
    }
    if (ztab != z) break;
    if (strncmp(shold, shtab, 2)) {
      strncpy(shold, shtab, 2);
      if (ie != 0) netab[ish] = ie;
      ie = 0;
      if (*shtab == 'K') {
	ish = 0;
      } else if (*shtab == 'L') {
	ish = shtab[1] - '0';
      } else if (*shtab == 'M') {
	ish = 3 + shtab[1] - '0';
      } else if (*shtab == 'N') {
	ish = 9;
      }
    }
    /* ETAB IS TABLE GAMMA ENERGY, ALFTAB IS TABULAR ICC FOR THE 8 MULTIPO */
    etab[ish][ie] = data[0];
    for (i = 0; i < 8; ++i) {
      alftab[i][ish][ie] = data[i+1];
    }
    if (++tblkey > 13004) break;
    ++ie;
  }
  fclose(file1);
  fclose(file2);
  zold = z;
  if (ie != 0) netab[ish] = ie;

  /* finished reading conversion coeff. tables from icctbl.dat.... */
  /* now assemble gamma energies for conv. coef. calculations.... */
 NEXTDATA:
  hgd.np = 0;
  for (i = idx; i < idx + 23 && i < ngam; ++i) {
    hgd.ee[hgd.np] = ek[hgd.np] = egam[i];
    hgd.np++;
  }
  idx += 23;
  npold = hgd.np;

  /* now calculate conv. coeffs. for up to 23 gammas at a time.... */
  /* K-SHELL */
  hgd.ns = 0;
  if (z < 30) hgd.ns = 4;
  k = 0;
  if ((hgd.n = netab[k]) != 0) {
    hgd.e2 = hgd.e1 = etab[k][0];
    iloop();
    for (i = 0; i < 8; ++i) {
      hgd.l = i+1;
      if (hgd.l > 4) hgd.l = i - 3;
      for (j = 0; j < hgd.n; ++j) {
	hgd.alpha[j] = alftab[i][k][j];
      }
      fit();
      /* RESET FOR NEXT CALL TO FIT */
      hgd.e1 = etab[k][0];
      for (j = 0; j < npold; ++j) {
	if (ek[j] < hgd.e1) {
	  for (m = npold; m >= j && m >= 2; --m) {
	    hgd.alpha[m - 1] = hgd.alpha[m - 2];
	  }
	  hgd.alpha[j] = -1.f;
	}
	alphe[i][k][j] = hgd.alpha[j];
      }
    }
    hgd.np = npold;
    for (m = 0; m < npold; ++m) {
      hgd.ee[m] = ek[m];
    }
  }

  /* L-SUBSHELL */
  hgd.ns = 1;  if (z < 30) hgd.ns = 4;
  for (k = 1; k < 4; ++k) {
    if ((hgd.n = netab[k]) == 0) continue;
    hgd.e1 = etab[k][0];
    iloop();
    for (i = 0; i < 8; ++i) {
      hgd.e2 = etab[2][0];
      hgd.l = i+1;
      if (hgd.l > 4) hgd.l = i - 3;
      for (j = 0; j < hgd.n; ++j) {
	hgd.alpha[j] = alftab[i][k][j];
      }
      fit();
      hgd.e1 = etab[k][0];
      for (j = 0; j < npold; ++j) {
	if (ek[j] < hgd.e1) {
	  for (m = npold; m >= j && m >= 2; --m) {
	    hgd.alpha[m - 1] = hgd.alpha[m - 2];
	  }
	  hgd.alpha[j] = -1.f;
	}
	alphe[i][k][j] = hgd.alpha[j];
      }
    }
    hgd.np = npold;
    for (m = 0; m < npold; ++m) {
      hgd.ee[m] = ek[m];
    }
  }

  /* M-SUBSHELL */
  hgd.ns = 2;
  for (k = 4; k < 9; ++k) {
    if ((hgd.n = netab[k]) == 0) continue;
    hgd.e1 = etab[k][0];
    iloop();
    for (i = 0; i < 8; ++i) {
      hgd.e2 = etab[8][0];
      hgd.l = i+1;
      if (hgd.l > 4) hgd.l = i - 3;
      for (j = 0; j < hgd.n; ++j) {
	hgd.alpha[j] = alftab[i][k][j];
      }
      fit();
      hgd.e1 = etab[k][0];
      for (j = 0; j < npold; ++j) {
	if (ek[j] < hgd.e1) {
	  for (m = npold; m >= j && m >= 2; --m) {
	    hgd.alpha[m - 1] = hgd.alpha[m - 2];
	  }
	  hgd.alpha[j] = -1.f;
	}
	alphe[i][k][j] = hgd.alpha[j];
      }
    }
    hgd.np = npold;
    for (m = 0; m < npold; ++m) {
      hgd.ee[m] = ek[m];
    }
  }

  /* N+O+... SHELL */
  hgd.n = netab[9];
  if (hgd.n != 0) {
    hgd.ns = 3;
    hgd.e1 = etab[9][0];
    iloop();
    for (i = 0; i < 8; ++i) {
      hgd.l = i+1;
      if (hgd.l > 4) hgd.l = i - 3;
      for (j = 0; j < hgd.n; ++j) {
	hgd.alpha[j] = alftab[i][9][j];
      }
      fit();
      hgd.e1 = etab[9][0];
      for (j = 0; j < npold; ++j) {
	if (ek[j] < hgd.e1) {
	  for (m = npold; m >= j && m >= 2; --m) {
	    hgd.alpha[m - 1] = hgd.alpha[m - 2];
	  }
	  hgd.alpha[j] = -1.f;
	}
	alphe[i][9][j] = hgd.alpha[j];
      }
    }
    hgd.np = npold;
    for (m = 0; m < hgd.np; ++m) {
      hgd.ee[m] = ek[m];
    }
  }

  /* OUTPUT LOOP */
  for (i = 0; i < hgd.np; ++i) {
    /* INITIALIZE OUTPUT VARIABLES */
    for (kk = 0; kk < 8; ++kk) {
      tcl[kk] = tle[kk] = tcm[kk] = tme[kk] = tcc[kk] = 0.f;
    }
    for (jj = 0; jj < 10; ++jj) {
      skip[jj] = 0;
    }
    /* IF GAMMA ENERGY < BINDING ENERGY, ICC = 0 */
    for (k = 0; k < 10; ++k) {
      if (k != 9 &&
	  z >= 30 &&
	  /* modified by DCR 2001-10-04: */
 	  /* etab[k][0] - 1.f > ek[i]) { */
 	  etab[k][0] > ek[i]) {
	  /* end modified by DCR 2001-10-04 */
	for (igf = 0; igf < 8; ++igf) {
	  alphe[igf][k][i] = 0.f;
	}
      }
      if (alphe[0][k][i] < 0.f) skip[k] = 1;
    }
    /* DRAGOUN ET AL LIST NO ICC FOR Z < 37 */
    if (z < 37) skip[9] = 1;
    if (z < 30) skip[4] = 1;

    for (n = 0; n < 8; ++n) {
      /* TOTAL-L */
      /* TOTAL L IS CALCULATED THOUGH SOME OF THE L'S SHOULD BE SKIPPED */
      for (k = 1; k < 4; ++k) {
	if (!skip[k]) tcl[n] += alphe[n][k][i];
      }
      if (!skip[0]) tle[n] = alphe[n][0][i] + tcl[n] * 1.33f;
      /* TOTAL-M */
      for (k = 4; k < 9; ++k) {
	if (!skip[k]) tcm[n] += alphe[n][k][i];
      }
      /* Do not CALCULATE TOTAL if L's and M's are to skip */
      for (kk = 0; kk < 9; ++kk) {
	if (skip[kk]) goto SKIP;
      }
      tme[n] = alphe[n][0][i] + tcl[n] + tcm[n] * 1.33f;
      /* TO AVOID TME VALUE SET TO -1. WHEN EVERYHING ENEGY OUTSIDE RANGE */
      if (tme[n] < 0.f) tme[n] = 0.f;
      if (!skip[9]) tcc[n] = alphe[n][0][i] + alphe[n][9][i] + tcl[n] + tcm[n];
    SKIP:
      continue;
    }
    if (tcc[0] != 0.f) {
      for (j = 0; j < 8; ++j) {
	alphat[gammanum*8 + j] = tcc[j];
      }
    } else if (tme[0] != 0.f) {
      for (j = 0; j < 8; ++j) {
	alphat[gammanum*8 + j] = tme[j];
      }
    } else {
      for (j = 0; j < 8; ++j) {
	alphat[gammanum*8 + j] = tle[j];
      }
    }
    ++gammanum;
  }
  if (idx < ngam) goto NEXTDATA;
  return 0;
} /* hsicc */


int swap_bytes_i(int *in)
{
  static int equiv_0[1];
  static char c;
#define cwork ((char *)equiv_0)
#define iwork (equiv_0)

  *iwork = *in;
  c = cwork[0];
  cwork[0] = cwork[3];
  cwork[3] = c;
  c = cwork[1];
  cwork[1] = cwork[2];
  cwork[2] = c;
  *in = *iwork;
  return 0;
} /* swap_bytes_i */
#undef iwork
#undef cwork


int swap_bytes_r(float *in)
{
  static float equiv_0[1];
  static char c;
#define cwork ((char *)equiv_0)
#define rwork (equiv_0)

  *rwork = *in;
  c = cwork[0];
  cwork[0] = cwork[3];
  cwork[3] = c;
  c = cwork[1];
  cwork[1] = cwork[2];
  cwork[2] = c;
  *in = *rwork;
  return 0;
} /* swap_bytes_r */
#undef rwork
#undef cwork


int fit(void)
{
  static float erg[5][23] =
  { { 1.f, 1.7f, 3.f, 6.2f, 10.f, 14.f, 20.f, 28.f, 40.f, 53.f, 70.f, 83.f, 100.f,
      123.f, 150.f, 215.f, 300.f, 390.f, 500.f, 730.f, 1e3f, 1250.f, 1500.f },
    { 1.f, 1.7f, 3.f, 6.2f, 10.f, 14.f, 20.f, 28.f, 40.f, 53.f, 70.f, 83.f, 100.f,
      123.f, 150.f, 215.f, 300.f, 390.f, 500.f, 730.f, 1e3f, 1500.f, 0.f },
    { 1.f, 2.f, 4.f, 8.f, 15.f, 25.f, 40.f, 52.f, 70.f, 103.f, 150.f, 280.f,
      500.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f },
    { 50.f, 70.f, 100.f, 150.f, 200.f, 500.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f },
    { 15.f, 17.f, 20.f, 25.f, 32.f, 40.f, 50.f, 65.f, 80.f, 100.f, 120.f, 150.f,
      200.f, 300.f, 450.f, 650.f, 1e3f, 2e3f, 6e3f, 0.f, 0.f, 0.f, 0.f } };

  static int i, j, m, ne2;
  static float w, en[23];

/* ALL ENTRIES GIVEN FOR A PARTICULAR CASE MUST BE USED */
/* E1 = LOWEST TABULATED ENERGY OF SUBSHELL TO BE INTERPOLATED, EXCEPT */
/*   *TOTAL* L- OR M-SHELLS IN WHICH CASE E1=-1.0  WE DO NOT INTERPOLATE */
/*   IN TOTAL L OR M SHELL , WE INTERPOLATE IN SUBSHELLS AND THEN */
/*   SUM THE SEPARATE SUBSHELL VALUES */
/* E2 = LOWEST TABULATED ENERGY OF THE K, L2, M5 SUBSHELLS FOR THE */
/*   INTERPOLATION IN THE K, L, M SHELLS, RESPECTIVELY */
/* NS = 1, 2, 3 FOR THE K, L, M SHELLS, RESPECTIVELY */
/* L = MULTIPOLE ORDER FOR THE CASE TO BE INTERPOLATED */
/* N = NUMBER OF VALUES GIVEN IN EACH CASE IN THE TABLE */
/* ALPHA = LIST OF CONVERSION COEFFICIENTS FROM THE TABLE */
/* NP = NUMBER OF CONVERSION COEFFICIENTS TO BE INTERPOLATED, MUST BE LESS THAN 24 */
/* EE = LIST OF GAMMA RAY ENERGIES OF THE VALUES TO BE DETERMINED */

  if (hgd.ns < 3) {
    if (hgd.ns == 0) hgd.e2 = hgd.e1;
    hgd.e1 = -hgd.e1 + 1.f;
    hgd.e2 = -hgd.e2 + 1.f;
    ne2 = (int) (-hgd.e2 + .501f);
    if (hgd.n >= 6) {
      for (i = 5; i < hgd.n; ++i) {
	m = (int) (erg[hgd.ns][i] + .01f);
	j = m + ne2;
	if (m < 150) {
	  j = (j * 10 + 5) / 10;
	} else if (m < 390) {
	  j = 5 * ((j * 10 + 25) / 50);
	} else if (m < 1000) {
	  j = 10 * ((j + 5) / 10);
	} else {    /* M >= 1000 */
	  j = 50 * ((j + 25) / 50);
	}
	en[i] = (float) j;
	hgd.c[i] = en[i] + hgd.e1;
      }
    }
    for (i = 0; i < 5; ++i) {
      hgd.c[i] = erg[hgd.ns][i];
      en[i] = erg[hgd.ns][i] - hgd.e1;
    }
    for (i = 0; i < hgd.n; ++i) {
      hgd.c[i] = log(sqrt((hgd.c[i] + 1021.952f) * hgd.c[i]));
    }
    for (i = 0; i < hgd.np; ++i) {
      w = hgd.ee[i] + hgd.e1;
      if (w >= 0.f) hgd.x[i] = log(sqrt((w + 1021.952f) * w));
    }
  } else {
    m = 2 * hgd.l + 1;
    for (i = 0; i < hgd.n; ++i) {
      en[i] = erg[hgd.ns][i];
      hgd.c[i] = log(sqrt((en[i] + 1021.952f) * en[i]));
      hgd.a[i] = log(hgd.alpha[i] * pow(en[i], (float) m));
    }
    for (i = 0; i < hgd.np; ++i) {
      if (hgd.ee[i] >= hgd.e1)
	hgd.x[i] = log(sqrt((hgd.ee[i] + 1021.952f) * hgd.ee[i]));
    }
  }

  m = 2 * hgd.l + 1;
  for (i = 0; i < hgd.n; ++i) {
    hgd.a[i] = log(hgd.alpha[i] * pow(en[i], (float) m));
  }
  spline(hgd.n, hgd.np);
  for (i = 0; i < hgd.np; ++i) {
    /* modified by DCR 2007-06-06: */
    /* if (hgd.ee[i] < en[0] || hgd.ee[i] > en[hgd.n - 1]) { */
    if (hgd.ee[i] < en[0] || hgd.ee[i] >= en[hgd.n - 1]) {
    /* end modified by DCR 2007-06-06: */
      hgd.alpha[i] = -1.f;
    } else {
      hgd.alpha[i] = exp(hgd.y[i]) / pow(hgd.ee[i], (float) m);
    }
  }
  return 0;
} /* fit */

int spline(int n, int np)
{
  float e[23], f[23], h[23], s[23], t[23], d[23][4];
  float aa, z, zp;
  int i, k, l, m, ms;

  m  = n - 1;
  ms = n - 2;
  for (i = 0; i < m; ++i) {
    e[i] = hgd.c[i+1] - hgd.c[i];
    f[i] = hgd.a[i+1] - hgd.a[i];
  }
  for (i = 0; i < ms; ++i) {
    d[i][0] =  e[i+1];
    d[i][1] = (e[i] + e[i+1]) * 2.f;
    d[i][2] =  e[i];
    d[i][3] = (e[i+1] * f[i] / e[i] + e[i] * f[i+1] / e[i+1]) * 3.f;
  }
  s[0] = -1.f;
  t[0] = f[0] * 2.f / e[0];
  for (i = 1; i < m; ++i) {
    l = i - 1;
    s[i] = -(d[l][0] + d[l][1] * s[l]) / (d[l][2]*s[l]);
    t[i] = (d[l][3] - t[l] * (d[l][1] + d[l][2] * s[i])) / d[l][2];
  }
  aa = f[ms] * 2.f / e[ms];
  h[m] = (t[ms] + aa * s[ms]) / (s[ms] + 1.f);
  for (i = 1; i <= m; ++i) {
    k = n - i - 1;
    h[k] = (h[k+1] - t[k]) / s[k];
  }

  for (i = 0; i < np; ++i) {
    k = 0;
    while (k < m && hgd.c[k+1] <= hgd.x[i]) k++;
    z = (hgd.c[k+1] - hgd.c[k]) / 2.f;
    hgd.b[2] = (h[k+1] - h[k]) / (z * 4.f);
    hgd.b[3] = (h[k+1] + h[k] - (hgd.a[k+1] - hgd.a[k])/z) / (z*z * 4.f);
    hgd.b[0] = (hgd.a[k+1] + hgd.a[k] - hgd.b[2] * 2.f * z*z) / 2.f;
    hgd.b[1] = (h[k+1] + h[k] - hgd.b[3] * 6.f * z*z) / 2.f;
    zp = hgd.x[i] - (hgd.c[k+1] + hgd.c[k]) / 2.f;
    hgd.y[i] = hgd.b[0] + zp*(hgd.b[1] + zp*(hgd.b[2] + zp*hgd.b[3]));
  }
  return 0;
} /* spline */

int iloop(void)
{
  int i = 0, m;

  while (i < hgd.np) {
    if (hgd.ee[i] < hgd.e1 && hgd.np > 1) {
      hgd.np--;
      for (m = i; m < hgd.np; ++m) {
	hgd.ee[m] = hgd.ee[m+1];
      }
    } else {
      i++;
    }
  }
  return 0;
} /* iloop */

