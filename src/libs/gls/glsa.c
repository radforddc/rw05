#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "gls.h"
#include "util.h"

/*  set of non-graphical routines used by ALL gls programs,
    both graphical and non-graphical
    i.e. gls_conv, gls, xmgls, escl8r... etc.  */

/* ==== extern routines from drawstr_ps ==== */
extern int closeps(void);
extern int connpt(float, float);
extern int drawstr_ps(char *, int, char, float, float, float, float);
extern int openps(char *, char *, char *, float, float, float, float);
extern int setdash(float, float);
extern int setgray(float);
extern int setlinewidth(float);
extern int strtpt(float, float);
extern int wfill(void);
extern int wstroke(void);

/* ==== other extern routines ==== */
extern int hsicc(int, float *, int, float *);
int rename_input_file_with_tilde = 0;

Glsgd   glsgd;
Levemit levemit;
Glsundo glsundo;


/* all figure distance units are in keV */
/* gamma arrow widths = intensity * aspect_ratio (in keV units) */

/* ======================================================================= */
int calc_alpha(int igam)
{
  float alpha[2][8], delsq, gd2[2];
  int   j1, j2;

  /* call subroutine hsicc to calculate the conversion coeffient of gamma igam */
  gd2[0] = glsgd.gam[igam].e;
  glsgd.gam[igam].a = 0.0f;
  if (hsicc(glsgd.atomic_no, gd2, 1, alpha[0])) return 0;
  if (glsgd.gam[igam].em == 'E') {
    j1 = (int) (0.5f + glsgd.gam[igam].n) - 1;
    j2 = j1 + 5;
    if (j1 > 3) j1 = 3;
    if (j2 > 7) j2 = 7;
  } else {
    j1 = (int) (0.5f + glsgd.gam[igam].n) + 3;
    j2 = j1 - 3;
    if (j1 > 7) j1 = 7;
    if (j2 > 3) j2 = 3;
  }
  if (glsgd.gam[igam].d != 0.0f) {
    delsq = glsgd.gam[igam].d * glsgd.gam[igam].d;
    glsgd.gam[igam].a = (alpha[0][j1] + delsq * alpha[0][j2]) / (delsq + 1.0f);
  } else {
    glsgd.gam[igam].a = alpha[0][j1];
  }
  return 0;
} /* calc_alpha */

/* ======================================================================= */
int calc_band(void)
{
  float b1, b2, tdx, tdy;
  int   i, lf, li, left_right[MAXBAND];

  glsgd.ty0  = -200.0f;
  glsgd.thiy = 2000.0f;
  for (i = 0; i < glsgd.nlevels; ++i) {
    strncpy(glsgd.lev[i].name, glsgd.bnd[glsgd.lev[i].band].name, 8);
    sprintf(glsgd.lev[i].name + 8, "%.2i", (int) (glsgd.lev[i].j + 0.1f));
    if (glsgd.thiy < glsgd.lev[i].e) glsgd.thiy = glsgd.lev[i].e;
  }
  b1 = glsgd.bnd[0].x0;
  b2 = glsgd.bnd[0].x0 + glsgd.bnd[0].nx;
  for (i = 1; i < glsgd.nbands; ++i) {
    if (b1 > glsgd.bnd[i].x0) b1 = glsgd.bnd[i].x0;
    if (b2 < glsgd.bnd[i].x0 + glsgd.bnd[i].nx)
      b2 = glsgd.bnd[i].x0 + glsgd.bnd[i].nx;
  }
  glsgd.bnd[glsgd.nbands].nx = glsgd.bnd[glsgd.nbands + 1].nx = glsgd.default_width;
  glsgd.bnd[glsgd.nbands].x0 = b1 - glsgd.bnd[glsgd.nbands].nx - glsgd.default_sep;
  glsgd.bnd[glsgd.nbands + 1].x0 = b2 + glsgd.default_sep;
  glsgd.tx0 = glsgd.bnd[glsgd.nbands].x0 - glsgd.bnd[glsgd.nbands].nx;
  glsgd.thix = glsgd.bnd[glsgd.nbands + 1].x0 + glsgd.bnd[glsgd.nbands + 1].nx * 2.f;
  glsgd.thiy *= 1.05f;
  for (i = 0; i < glsgd.ntlabels; ++i) {
    tdx = (glsgd.txt[i].nc / 2 + 4) * glsgd.txt[i].csx;
    tdy = glsgd.txt[i].csy * 3;
    if (glsgd.tx0 > glsgd.txt[i].x - tdx) glsgd.tx0 = glsgd.txt[i].x - tdx;
    if (glsgd.thix < glsgd.txt[i].x + tdx) glsgd.thix = glsgd.txt[i].x + tdx;
    if (glsgd.ty0 > glsgd.txt[i].y - tdy) glsgd.ty0 = glsgd.txt[i].y - tdy;
    if (glsgd.thiy < glsgd.txt[i].y + tdy) glsgd.thiy = glsgd.txt[i].y + tdy;
  }
  for (i = 0; i < glsgd.nbands; ++i) {
    left_right[i] = 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    li = glsgd.gam[i].li;
    lf = glsgd.gam[i].lf;
    if (glsgd.lev[li].band != glsgd.lev[lf].band) {
      if ((glsgd.bnd[glsgd.lev[li].band].x0 +
	   glsgd.bnd[glsgd.lev[li].band].nx) / 2.f <
	  (glsgd.bnd[glsgd.lev[lf].band].x0 +
	   glsgd.bnd[glsgd.lev[lf].band].nx) / 2.f) {
	++left_right[glsgd.lev[li].band];
	--left_right[glsgd.lev[lf].band];
      } else {
	--left_right[glsgd.lev[li].band];
	++left_right[glsgd.lev[lf].band];
      }
    }
  }
  for (i = 0; i < glsgd.nbands; ++i) {
    if (left_right[i] >= 0) {
      glsgd.bnd[i].sl_onleft = 1;
    } else {
      glsgd.bnd[i].sl_onleft = 0;
    }
  }
  return 0;
} /* calc_band */

/* ======================================================================= */
int calc_gam(int igam)
{
  float egam, f, f1, f2, x1, x2, y1, y2, dt, dx, dy, dd;
  float aw, al2, aw1, aw2, xf1, xf2, xi1, xi2, ddx;
  int   lf, li;

  /* X1,Y1 is the arrow tail point, X2,Y2 is the arrow head point */
  li = glsgd.gam[igam].li;
  lf = glsgd.gam[igam].lf;
  y1 = glsgd.lev[li].e;
  y2 = glsgd.lev[lf].e;
  dd = glsgd.max_tangent * fabs(y1 - y2);
  if (dd > glsgd.max_dist) dd = glsgd.max_dist;
  x1 = glsgd.gam[igam].x1;
  x2 = glsgd.gam[igam].x2;
  if (x1 == 0.f) {
    if (glsgd.lev[li].band == glsgd.lev[lf].band) {
      x1 = glsgd.bnd[glsgd.lev[li].band].x0 +
	   glsgd.bnd[glsgd.lev[li].band].nx * .5f;
      x2 = x1;
    } else {
      xi1 = glsgd.lev[li].x[0];
      xi2 = glsgd.lev[li].x[1];
      xf1 = glsgd.lev[lf].x[0];
      xf2 = glsgd.lev[lf].x[1];
      if (xf2 < xi1) {
	x1 = xi1;
	x2 = x1 - dd;
	if (x2 < xf2) x2 = xf2;
      } else if (xf1 > xi2) {
	x1 = xi2;
	x2 = x1 + dd;
	if (x2 > xf1) x2 = xf1;
      } else {
	x1 = (xi1 + xi2) / 2.f;
	x2 = (xf1 + xf2) / 2.f;
	dx = x1 - x2;
	if (fabs(dx) > dd) {
	  ddx = (fabs(dx) - dd) * dx / fabs(dx);
	  x1 -= ddx / 2.f;
	  x2 += ddx / 2.f;
	}
      }
    }
    glsgd.gam[igam].x2 = x1 + x2;
  }
  if (glsgd.lev[li].x[2] > x1) glsgd.lev[li].x[2] = x1;
  if (glsgd.lev[li].x[3] < x1) glsgd.lev[li].x[3] = x1;
  if (glsgd.lev[lf].x[2] > x2) glsgd.lev[lf].x[2] = x2;
  if (glsgd.lev[lf].x[3] < x2) glsgd.lev[lf].x[3] = x2;
  dx = x1 - x2;
  dy = y1 - y2;
  if (dy < 1.0f) dy = 1.0f; /* stops problems if arrow is too short */
  dt = sqrt(dx * dx + dy * dy);
  f1 = dy / dt;
  f2 = dx / dt;
  if (glsgd.gam[igam].i * (glsgd.gam[igam].a + 1.f) < .5f ||
      dy < 10.0f) {         /* stops problems if arrow is too short */
    aw = aw1 = aw2 = 0.f;
    glsgd.gam[igam].np = 6;
  } else {
    aw1 = glsgd.gam[igam].i * glsgd.aspect_ratio;
    aw2 = aw1 * glsgd.gam[igam].a;
    aw  = aw1 + aw2;
    if (glsgd.gam[igam].i * glsgd.gam[igam].a < .5f) {
      aw2 = 0.f;
      glsgd.gam[igam].np = 8;
    } else {
      glsgd.gam[igam].np = 10;
    }
  }
  al2 = glsgd.arrow_length;
  if (al2 > dt * .5f) al2 = dt * .5f;
  glsgd.gam[igam].x[0] = x1 - aw / (f1 * 2.f);
  glsgd.gam[igam].y[0] = y1;
  glsgd.gam[igam].x[1] = x2 - aw * f1 / 2.f + al2 * f2;
  glsgd.gam[igam].y[1] = y2 + aw * f2 / 2.f + al2 * f1;
  glsgd.gam[igam].x[2] = x2 - (aw + glsgd.arrow_width) * f1 / 2.f + al2 * f2;
  glsgd.gam[igam].y[2] = y2 + (aw + glsgd.arrow_width) * f2 / 2.f + al2 * f1;
  glsgd.gam[igam].x[3] = x2;
  glsgd.gam[igam].y[3] = y2;
  glsgd.gam[igam].x[4] = x2 + (aw + glsgd.arrow_width) * f1 / 2.f + al2 * f2;
  glsgd.gam[igam].y[4] = y2 - (aw + glsgd.arrow_width) * f2 / 2.f + al2 * f1;
  glsgd.gam[igam].x[5] = x2 + aw * f1 / 2.f + al2 * f2;
  glsgd.gam[igam].y[5] = y2 - aw * f2 / 2.f + al2 * f1;
  if (aw > 0.f) {
    glsgd.gam[igam].x[6] = x1 + aw / (f1 * 2.f);
    glsgd.gam[igam].y[6] = y1;
    glsgd.gam[igam].x[7] = glsgd.gam[igam].x[0];
    glsgd.gam[igam].y[7] = y1;
    if (aw2 > 0.f) {
      glsgd.gam[igam].x[8] = glsgd.gam[igam].x[7] + aw1 / f1;
      glsgd.gam[igam].y[8] = y1;
      if (aw1 < aw2) {
	f = (aw1 * 2.f + glsgd.arrow_width) / (aw + glsgd.arrow_width);
	glsgd.gam[igam].x[9] = (1.f - f) * glsgd.gam[igam].x[2] + f * x2;
	glsgd.gam[igam].y[9] = (1.f - f) * glsgd.gam[igam].y[2] + f * y2;
      } else {
	f = (aw2 * 2.f + glsgd.arrow_width) / (aw + glsgd.arrow_width);
	glsgd.gam[igam].x[9] = (1.f - f) * glsgd.gam[igam].x[4] + f * x2;
	glsgd.gam[igam].y[9] = (1.f - f) * glsgd.gam[igam].y[4] + f * y2;
      }
    }
  }
  /* write label to string
     gam.elflg = 0 for 123
                 1 for (123)
		 2 for no label
		 3 for 123.4
		 4 for (123.4)
  */
  egam = glsgd.gam[igam].e;
  if (glsgd.gam[igam].elflg == 0) {
    sprintf(glsgd.gam[igam].el, "%.0f", egam);
  } else if (glsgd.gam[igam].elflg == 1) {
    sprintf(glsgd.gam[igam].el, "(%.0f)", egam);
  } else if (glsgd.gam[igam].elflg == 3) {
    sprintf(glsgd.gam[igam].el, "%.1f", egam);
  } else if (glsgd.gam[igam].elflg == 4) {
    sprintf(glsgd.gam[igam].el, "(%.1f)", egam);
  } else {
    strcpy(glsgd.gam[igam].el, "");
  }
  glsgd.gam[igam].elnc = strlen(glsgd.gam[igam].el);
  glsgd.gam[igam].elx = (x1 + x2) * .5f + glsgd.gam[igam].eldx;
  glsgd.gam[igam].ely = (y1 + y2) * .5f + glsgd.gam[igam].eldy -
    glsgd.gel_csy * .5f;
  return 0;
} /* calc_gam */

/* ======================================================================= */
int calc_gls_fig(void)
{
  int igam, ilev;

  for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
    calc_lev(ilev);
  }
  for (igam = 0; igam < glsgd.ngammas; ++igam) {
    calc_gam(igam);
  }
  return 0;
} /* calc_gls_fig */

/* ======================================================================= */
int calc_lev(int ilev)
{
  float elev, spin;
  int   ncs;
  char  pi[8], cbuf[80];

  glsgd.lev[ilev].x[0] = glsgd.bnd[glsgd.lev[ilev].band].x0 +
                         glsgd.lev[ilev].dxl;
  glsgd.lev[ilev].x[1] = glsgd.bnd[glsgd.lev[ilev].band].x0 +
                         glsgd.bnd[glsgd.lev[ilev].band].nx +
                         glsgd.lev[ilev].dxr;
  glsgd.lev[ilev].x[2] = glsgd.lev[ilev].x[0];
  glsgd.lev[ilev].x[3] = glsgd.lev[ilev].x[1];
  /* calculate spin-parity label and position */
  spin = glsgd.lev[ilev].j;
  if (((int) (spin*2.0f + 0.5)) % 2 == 0) {
    sprintf(cbuf, "%.0f", spin);
  } else {
    sprintf(cbuf, "%.0f/2", spin*2.0f);
  }
  ncs = strlen(cbuf);
  if (glsgd.lev[ilev].pi > 0.f) {
    strcpy(pi, "{u{g+}}");
  } else {
    strcpy(pi, "{u{g-}}");
  }
  /* slflg = 0 for spin label =  Jpi */
  /*         1 for spin label = (J)pi */
  /*         2 for spin label = J(pi) */
  /*         3 for spin label = (Jpi) */
  /*         4 for spin label =   J */
  /*         5 for spin label =  (J) */
  /*         6 for no spin label */
  if (glsgd.lev[ilev].slflg == 0) {
    sprintf(glsgd.lev[ilev].sl, "%s%s", cbuf, pi);
    glsgd.lev[ilev].slnc = ncs + 7;
  } else if (glsgd.lev[ilev].slflg == 1) {
    sprintf(glsgd.lev[ilev].sl, "(%s)%s", cbuf, pi);
    glsgd.lev[ilev].slnc = ncs + 9;
  } else if (glsgd.lev[ilev].slflg == 2) {
    sprintf(glsgd.lev[ilev].sl, "%s{u(%s", cbuf, pi+2);
    strcpy(glsgd.lev[ilev].sl + ncs + 7, ")}");
    glsgd.lev[ilev].slnc = ncs + 9;
  } else if (glsgd.lev[ilev].slflg == 3) {
    sprintf(glsgd.lev[ilev].sl, "(%s%s)", cbuf, pi);
    glsgd.lev[ilev].slnc = ncs + 9;
  } else if (glsgd.lev[ilev].slflg == 4) {
    strcpy(glsgd.lev[ilev].sl, cbuf);
    glsgd.lev[ilev].slnc = ncs;
  } else if (glsgd.lev[ilev].slflg == 5) {
    sprintf(glsgd.lev[ilev].sl, "(%s)", cbuf);
    glsgd.lev[ilev].slnc = ncs + 2;
  } else {
    strcpy(glsgd.lev[ilev].sl, " ");
    glsgd.lev[ilev].slnc = 0;
  }
  if (glsgd.lev[ilev].sldx != 0.f) {
    glsgd.lev[ilev].slx = glsgd.lev[ilev].x[0] + glsgd.lev[ilev].sldx;
  } else {
    if (glsgd.bnd[glsgd.lev[ilev].band].sl_onleft) {
      glsgd.lev[ilev].slx = glsgd.lev[ilev].x[0] -
	(float)( ncs-2)*0.3f*glsgd.lsl_csx +
	glsgd.bnd[glsgd.lev[ilev].band].sldx;
    } else {
      glsgd.lev[ilev].slx = glsgd.lev[ilev].x[1] +
	(float) (ncs-2)*0.3f*glsgd.lsl_csx + 
	glsgd.bnd[glsgd.lev[ilev].band].sldx;
    }
  }
  if (glsgd.lev[ilev].sldy != 0.f) {
    glsgd.lev[ilev].sly = glsgd.lev[ilev].e + glsgd.lev[ilev].sldy;
  } else {
    glsgd.lev[ilev].sly = glsgd.lev[ilev].e + 
      glsgd.lsl_csy * .4f + glsgd.bnd[glsgd.lev[ilev].band].sldy;
  }
  /* calculate level energy label and position */
  /* leflg = 0 for no label */
  /*         1 for 123 */
  /*         2 for (123) */
  /*         3 for 123.4 */
  /*         4 for (123.4) */
  if (glsgd.lev[ilev].elflg == 0) {
    strcpy(glsgd.lev[ilev].el, " ");
    glsgd.lev[ilev].elnc = 0;
  } else {
    /* write label to string */
    elev = glsgd.lev[ilev].e;
    if (glsgd.lev[ilev].elflg == 1) {
      sprintf(glsgd.lev[ilev].el, "{i%.0f}", elev);
    } else if (glsgd.lev[ilev].elflg == 2) {
      sprintf(glsgd.lev[ilev].el, "{i(%.0f)}", elev);
    } else if (glsgd.lev[ilev].elflg == 3) {
      sprintf(glsgd.lev[ilev].el, "{i%.1f}", elev);
    } else {
      sprintf(glsgd.lev[ilev].el, "{i(%.1f)}", elev);
    }
    glsgd.lev[ilev].elnc = strlen(glsgd.lev[ilev].el);
    if (glsgd.lev[ilev].eldx != 0.f) {
      glsgd.lev[ilev].elx = glsgd.lev[ilev].x[0] + glsgd.lev[ilev].eldx;
    } else {
      if (glsgd.bnd[glsgd.lev[ilev].band].sl_onleft) {
	glsgd.lev[ilev].elx = glsgd.lev[ilev].x[1] -
	  glsgd.lel_csx * .5f + glsgd.bnd[glsgd.lev[ilev].band].eldx;
      } else {
	glsgd.lev[ilev].elx = glsgd.lev[ilev].x[0] +
	  glsgd.lel_csx * .5f + glsgd.bnd[glsgd.lev[ilev].band].eldx;
      }
    }
    if (glsgd.lev[ilev].eldy != 0.f) {
      glsgd.lev[ilev].ely = glsgd.lev[ilev].e + glsgd.lev[ilev].eldy;
    } else {
      glsgd.lev[ilev].ely = glsgd.lev[ilev].e +
        glsgd.lel_csy * .4f + glsgd.bnd[glsgd.lev[ilev].band].eldy;
    }
  }
  return 0;
} /* calc_lev */

/* ======================================================================= */
int gls_ps(void)
{
  int gls_get_ps_details(char *title, float *htitle, char *titpos,
			 float *hyaxis, char *scalepos,
			 char *filnam, char *printcmd);

  float xtit, ytit, d, r, x1, y1, y2, x2, dd;
  float htitle, hyaxis, dx1, dy1, dx2, dy2, lw0, xyaxis, yyaxis, wyaxis;
  float yya, xya, xlo, ylo, xhi, yhi, xx[7], ranges[4], yy[7];
  int   nc, jj, igam, imax, ilev, i, j, n;
  int   ndgam, ndlev, nctitle = 0, yaxisl = 0;
  int   nextgam[MAXGAM], nextlev[MAXLEV];
  char  title[80], filnam[80], cbuf[256], titpos, scalepos, printcmd[80];

  if (gls_get_ps_details(title, &htitle, &titpos, &hyaxis,
			 &scalepos, filnam, printcmd)) return 1;

  ranges[0] = glsgd.x00;
  ranges[2] = glsgd.dx;
  ranges[1] = glsgd.y00;
  ranges[3] = glsgd.dy;
  /* changes for Walter Reviol :
     if entire scheme is displayed, change x-limits for plot */
  /* if (glsgd.x00 == glsgd.tx0 && glsgd.dx == (glsgd.thix - glsgd.tx0)) {
     ranges[0] += 2.0f * glsgd.bnd[glsgd.nbands].nx;
     ranges[2] -= 2.0f * glsgd.bnd[glsgd.nbands + 1].nx;
     } */

  if ((nctitle = strlen(title))) {
    ranges[3] += htitle * 2.0f;
    if (titpos == 'B' || titpos == 'b') ranges[1] -= htitle * 2.0f;
  }

  if (scalepos) {
    ranges[2] += hyaxis * 6.f;
    if (scalepos == 'L' || scalepos == 'l') {
      yaxisl = 1;
      ranges[0] -= hyaxis * 6.f;
    }
  }

  /* open output file */
  if (*printcmd && !(*filnam)) {
    strcpy(filnam, "gls_hc_tmp_XXXXXX");
    if ((j = mkstemp(filnam)) > 0) {
      close(j);
      remove(filnam);
    }
    j = setext(filnam, ".ps", 80);
  }
  openps(filnam, "GLS  (Author: D.C. Radford)", "GLS level scheme format", 
	 ranges[0], ranges[0] + ranges[2], ranges[1], ranges[1] + ranges[3]);

  lw0 = 5.0f; /* default drawing line width = 5 keV */

  /* dummy call to drawstr_ps to make sure that drawstr_ps
     will specify the font size, even if this is not the first
     .ps hardcopy output file */
  drawstr_ps(" ", -1, 'H', 0.0f, 0.0f, 0.0f, 0.0f);
  /* draw levels */
  /* first decide which levels are in region of interest */
  d = 2.5f * ranges[2];
  if (d < 2.5f * ranges[3]) d = 2.5f * ranges[3];
  xlo = ranges[0] + (ranges[2] - d) / 2.f;
  ylo = ranges[1] + (ranges[3] - d) / 2.f;
  xhi = xlo + d;
  yhi = ylo + d;
  ndlev = 0;
  for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
    if (glsgd.lev[ilev].e > ylo &&
	glsgd.lev[ilev].e < yhi &&
	glsgd.lev[ilev].x[2] < xhi &&
	glsgd.lev[ilev].x[3] > xlo) nextlev[ndlev++] = ilev;
  }
  setlinewidth(lw0);
  setdash(lw0, lw0 * 5.f);
  for (jj = 0; jj < ndlev; ++jj) {
    ilev = nextlev[jj];
    strtpt(glsgd.lev[ilev].x[2], glsgd.lev[ilev].e);
    connpt(glsgd.lev[ilev].x[3], glsgd.lev[ilev].e);
  }
  wstroke();
  setdash(0.f, 0.f);
  for (jj = 0; jj < ndlev; ++jj) {
    ilev = nextlev[jj];
    strtpt(glsgd.lev[ilev].x[0], glsgd.lev[ilev].e);
    connpt(glsgd.lev[ilev].x[1], glsgd.lev[ilev].e);
  }
  wstroke();
  setgray(1.0f);
  for (jj = 0; jj < ndlev; ++jj) {
    ilev = nextlev[jj];
    if (glsgd.lev[ilev].flg == 1) {
      /* tentative level */
      n = (glsgd.lev[ilev].x[1] - glsgd.lev[ilev].x[0] - glsgd.level_break) /
	  (glsgd.level_break * 1.5f);
      if (n < 1) n = 1;
      d = (glsgd.lev[ilev].x[1] - glsgd.lev[ilev].x[0]) / (n * 3 + 2);
      for (i = 1; i <= n; ++i) {
	x1 = glsgd.lev[ilev].x[0] + d * (float) (i * 3);
	strtpt(x1 - d, glsgd.lev[ilev].e);
	connpt(x1, glsgd.lev[ilev].e);
      }
    }
  }
  setgray(0.0f);
  setlinewidth(5.0f*lw0);
  for (jj = 0; jj < ndlev; ++jj) {
    ilev = nextlev[jj];
    if (glsgd.lev[ilev].flg == 2) {
      /* thick level */
      strtpt(glsgd.lev[ilev].x[0], glsgd.lev[ilev].e);
      connpt(glsgd.lev[ilev].x[1], glsgd.lev[ilev].e);
    }
  }
  /* draw gammas */
  /* first decide which levels are in region of interest */
  setlinewidth(lw0);
  ndgam = 0;
  for (igam = 0; igam < glsgd.ngammas; ++igam) {
    /* X1,Y1 is the arrow tail point, X2,Y2 is the arrow head poi
       nt*/
    y1 = glsgd.gam[igam].y[0];
    y2 = glsgd.gam[igam].y[3];
    x1 = glsgd.gam[igam].x1;
    x2 = glsgd.gam[igam].x2;
    if (x1 == 0.f) {
      x2 = glsgd.gam[igam].x[3];
      x1 = glsgd.gam[igam].x2 - x2;
    }
    if (y1 > ylo && y2 < yhi &&
	((x1 > xlo && x1 < xhi) || (x2 > xlo && x2 < xhi))) {
      nextgam[ndgam++] = igam;
    }
  }
  for (jj = 0; jj < ndgam; ++jj) {
    igam = nextgam[jj];
    if (glsgd.gam[igam].flg == 1) {
      /* tentative gamma */
      if (glsgd.gam[igam].np <= 6 ||
	  glsgd.gam[igam].x[0] > glsgd.gam[igam].x[1]) {
	dx1 = glsgd.gam[igam].x[0] - glsgd.gam[igam].x[1];
	dy1 = glsgd.gam[igam].y[0] - glsgd.gam[igam].y[1];
      } else {
	dx1 = glsgd.gam[igam].x[6] - glsgd.gam[igam].x[5];
	dy1 = glsgd.gam[igam].y[6] - glsgd.gam[igam].y[5];
      }
      dd = sqrt(dx1 * dx1 + dy1 * dy1);
      n = (int) (dd / (glsgd.arrow_break * 3.f) + 0.5f);
      if (n < 1) n = 1;
      d = dd / (float) (n * 3);
      dx2 = d / dd * dx1;
      dy2 = d / dd * dy1;
      if (glsgd.gam[igam].np <= 6) {
	strtpt(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	for (i = 0; i < n; ++i) {
	  connpt(glsgd.gam[igam].x[0] - (float) (i*3 + 1) * dx2,
		 glsgd.gam[igam].y[0] - (float) (i*3 + 1) * dy2);
	  strtpt(glsgd.gam[igam].x[0] - (float) (i*3 + 2) * dx2,
		 glsgd.gam[igam].y[0] - (float) (i*3 + 2) * dy2);
	}
	for (i = 1; i < glsgd.gam[igam].np; ++i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	wfill();
      } else if (glsgd.gam[igam].np <= 8) {
	xx[0] = glsgd.gam[igam].x[1] - dx2;
	yy[0] = glsgd.gam[igam].y[1] - dy2;
	xx[1] = glsgd.gam[igam].x[5] - dx2;
	yy[1] = glsgd.gam[igam].y[5] - dy2;
	xx[2] = glsgd.gam[igam].x[5] + dx2;
	yy[2] = glsgd.gam[igam].y[5] + dy2;
	xx[3] = glsgd.gam[igam].x[1] + dx2;
	yy[3] = glsgd.gam[igam].y[1] + dy2;
	strtpt(xx[3], yy[3]);
	for (i = 1; i < 6; ++i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	connpt(xx[2], yy[2]);
	connpt(xx[3], yy[3]);
	wfill();
	for (i = 1; i < n; ++i) {
	  strtpt(xx[3] + dx2 * 3.f, yy[3] + dy2 * 3.f);
	  for (j = 0; j < 4; ++j) {
	    xx[j] += dx2 * 3.f;
	    yy[j] += dy2 * 3.f;
	    connpt(xx[j], yy[j]);
	  }
	  wfill();
	}
	strtpt(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	connpt(xx[0] + dx2 * 3.f, yy[0] + dy2 * 3.f);
	connpt(xx[1] + dx2 * 3.f, yy[1] + dy2 * 3.f);
	connpt(glsgd.gam[igam].x[6], glsgd.gam[igam].y[7]);
	connpt(glsgd.gam[igam].x[7], glsgd.gam[igam].y[6]);
	wfill();
      } else {
	r = 1.f / (glsgd.gam[igam].a + 1.f);
	xx[0] = glsgd.gam[igam].x[1] - dx2;
	yy[0] = glsgd.gam[igam].y[1] - dy2;
	xx[1] = glsgd.gam[igam].x[5] - dx2;
	yy[1] = glsgd.gam[igam].y[5] - dy2;
	xx[2] = glsgd.gam[igam].x[5] + dx2;
	yy[2] = glsgd.gam[igam].y[5] + dy2;
	xx[3] = glsgd.gam[igam].x[1] + dx2;
	yy[3] = glsgd.gam[igam].y[1] + dy2;
	xx[4] = xx[3] + r * (xx[2] - xx[3]);
	yy[4] = yy[3] + r * (yy[2] - yy[3]);
	xx[5] = xx[4] - dx2 * 2.f;
	yy[5] = yy[4] - dy2 * 2.f;
	strtpt(xx[3], yy[3]);
	for (i = 1; i < 4; ++i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	connpt(glsgd.gam[igam].x[9], glsgd.gam[igam].y[9]);
	connpt(xx[4], yy[4]);
	connpt(xx[3], yy[3]);
	wfill();
	strtpt(xx[3], yy[3]);
	connpt(xx[2], yy[2]);
	for (i = 5; i > 2; --i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	for (i = 1; i < n; ++i) {
	  for (j = 0; j < 6; ++j) {
	    xx[j] += dx2 * 3.f;
	    yy[j] += dy2 * 3.f;
	  }
	  strtpt(xx[3], yy[3]);
	  connpt(xx[0], yy[0]);
	  for (j = 5; j > 2; --j) {
	    connpt(xx[j], yy[j]);
	  }
	  wfill();
	  strtpt(xx[3], yy[3]);
	  for (j = 2; j >= 0; --j) {
	    connpt(xx[j], yy[j]);
	  }
	}
	strtpt(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	connpt(xx[0] + dx2 * 3.f, yy[0] + dy2 * 3.f);
	connpt(xx[5] + dx2 * 3.f, yy[5] + dy2 * 3.f);
	connpt(glsgd.gam[igam].x[8], glsgd.gam[igam].y[8]);
	connpt(glsgd.gam[igam].x[7], glsgd.gam[igam].y[6]);
	wfill();
	strtpt(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	connpt(glsgd.gam[igam].x[6], glsgd.gam[igam].y[7]);
	connpt(xx[1] + dx2 * 3.f, yy[1] + dy2 * 3.f);
	connpt(xx[0] + dx2 * 3.f, yy[0] + dy2 * 3.f);
      }
    } else {
      if (glsgd.gam[igam].np <= 8) {
	strtpt(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	for (i = 1; i < glsgd.gam[igam].np; ++i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	wfill();
      } else {
	strtpt(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	for (i = 1; i < 4; ++i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	for (i = 9; i > 6; --i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	wfill();
	strtpt(glsgd.gam[igam].x[3], glsgd.gam[igam].y[3]);
	for (i = 4; i < 8; ++i) {
	  connpt(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
      }
    }
  }
  wstroke();
  /* white space for gamma energy labels */
  setgray(1.0f);
  for (jj = 0; jj < ndgam; ++jj) {
    igam = nextgam[jj];
    if (glsgd.gam[igam].elnc > 0) {
      x1 = glsgd.gam[igam].elx -
	(float) glsgd.gam[igam].elnc * .5f * glsgd.gel_csx - glsgd.gel_csy * .2f;
      x2 = glsgd.gam[igam].elx +
	(float) glsgd.gam[igam].elnc * .5f * glsgd.gel_csx + glsgd.gel_csy * .2f;
      y1 = glsgd.gam[igam].ely - glsgd.gel_csy * .2f;
      y2 = glsgd.gam[igam].ely + glsgd.gel_csy * 1.2f;
      strtpt(x1, y1);
      connpt(x1, y2);
      connpt(x2, y2);
      connpt(x2, y1);
      wfill();
    }
  }
  /* gamma energy labels */
  setgray(0.0f);
  for (jj = 0; jj < ndgam; ++jj) {
    igam = nextgam[jj];
    if (glsgd.gam[igam].elnc > 0)
      drawstr_ps(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H',
		 glsgd.gam[igam].elx, glsgd.gam[igam].ely,
		 glsgd.gel_csx, glsgd.gel_csy);
  }
  /* level spin and energy labels */
  for (jj = 0; jj < ndlev; ++jj) {
    ilev = nextlev[jj];
    if (glsgd.lev[ilev].slnc > 0)
      drawstr_ps(glsgd.lev[ilev].sl, glsgd.lev[ilev].slnc, 'H',
		 glsgd.lev[ilev].slx, glsgd.lev[ilev].sly,
		 glsgd.lsl_csx, glsgd.lsl_csy);
    if (glsgd.lev[ilev].elnc > 0)
      drawstr_ps(glsgd.lev[ilev].el, glsgd.lev[ilev].elnc, 'H',
		 glsgd.lev[ilev].elx, glsgd.lev[ilev].ely,
		 glsgd.lel_csx, glsgd.lel_csy);
  }
  /* general purpose text labels */
  for (i = 0; i < glsgd.ntlabels; ++i) {
    x1 = glsgd.txt[i].x;
    y1 = glsgd.txt[i].y;
    if (y1 > ylo && y1 < yhi && x1 > xlo && x1 < xhi)
     drawstr_ps(glsgd.txt[i].l, glsgd.txt[i].nc, 'H',
		 glsgd.txt[i].x, glsgd.txt[i].y,
		 glsgd.txt[i].csx, glsgd.txt[i].csy);
  }
  /* if necessary, add title for figure */
  if (strlen(title)) {
    xtit = ranges[0] + ranges[2] * .5f;
    if (scalepos) {
      if (yaxisl) {
	xtit += hyaxis * 3.f;
      } else {
	xtit -= hyaxis * 3.f;
      }
    }
    if (titpos == 'T' || titpos == 't') {
      ytit = ranges[1] + ranges[3] - htitle * 1.5f;
    } else {
      ytit = ranges[1] + htitle * 0.5f;
    }
    drawstr_ps(title, nctitle, 'H', xtit, ytit, htitle * .85f, htitle);
    wstroke();
  }
  /* if necessary, add y-axis */
  if (scalepos) {
    yyaxis = ranges[1] + ranges[3] * .5f;
    if (yaxisl) {
      xyaxis = ranges[0] + hyaxis * 2.f;
    } else {
      xyaxis = ranges[0] + ranges[2] - hyaxis * 1.f;
    }
    wyaxis = hyaxis * .85f;
    drawstr_ps("Excitation Energy (MeV)", 23, 'V', xyaxis, yyaxis, wyaxis, hyaxis);
    /* draw y-axis */
    setlinewidth(lw0);
    imax = (int) (ranges[1] + ranges[3]) / MAXLEV;
    if (yaxisl) {
      xyaxis = ranges[0] + hyaxis * 4.5f;
    } else {
      xyaxis = ranges[0] + ranges[2] - hyaxis * 4.5f;
    }
    strtpt(xyaxis, 0.0f);
    connpt(xyaxis, (float) imax * 1e3f);
    hyaxis *= .7f;
    wyaxis = hyaxis * .85f;
    setlinewidth(lw0);
    for (i = 1; i <= imax; i++) {
      yya = (float) i * 1e3f;
      strtpt(xyaxis, (float) i * 1e3f);
      if (yaxisl) {
	connpt(xyaxis + hyaxis * .3f, yya);
      } else {
	connpt(xyaxis - hyaxis * .3f, yya);
      }
    }
    wstroke();
    for (i = 0; i <= imax; i += 2) {
      sprintf(cbuf, "%i", i);
      nc = strlen(cbuf);
      yya = (float) i * 1e3f;
      if (yaxisl) {
	xya = xyaxis - ((float) nc * .5f + 1.f) * wyaxis;
      } else {
	xya = xyaxis + ((float) nc * .5f + 1.f) * wyaxis;
      }
      drawstr_ps(cbuf, nc, 'H', xya, yya - hyaxis * .5f, wyaxis, hyaxis);
    }
  }
  /* ----------------------------------- */

  closeps();
  if (*printcmd) {
    sprintf(cbuf, "%s %s", printcmd, filnam);
    tell("Printing with system command: %s\n",cbuf);
    if (system(cbuf) || remove(filnam)) {
      warn("*** Printing failed; your file was saved. ***\n");
      return 1;
    }
  }

  return 0;
} /* gls_ps */

/* ======================================================================= */
int indices_gls(void)
{
  int i, li;

  for (i = 0; i < MAXLEV; ++i) {
    levemit.n[i] = 0;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    li = glsgd.gam[i].li;
    if (levemit.n[li] >= MAXEMIT) {
      warn("SEVERE ERROR: More than %i gammas emitted by level number %i\n",
	   MAXEMIT, li+1);
      exit(-1);
    }
    levemit.l[li][levemit.n[li]++] = i;
  }
  return 0;
} /* indices_gls */

/* ======================================================================= */
int read_ascii_gls(FILE *file)
{
  int  j, j1;
  char eol, jpi[16], kay[16], line[120], lamda;

  /* read comments (first five lines of file)  */
  tell("Level Scheme File header:\n");
  for (j = 0; j < 5; ++j) {
    fgets(line, 120, file);
    tell("%s", line);
  }
  fgets(line, 120, file);
  if (strncmp(line, " ASCII GLS file format version 1.0", 34)) goto ERR;
  fgets(line, 120, file);
  fgets(line, 120, file);
  /* read data */
  if (sscanf(line, "%i%i%i%i%i%f%f%f",
             &glsgd.atomic_no,
             &glsgd.nlevels, &glsgd.ngammas, &glsgd.nbands, &glsgd.ntlabels,
             &glsgd.csx, &glsgd.csy, &glsgd.aspect_ratio) != 8) goto ERR;
  fgets(line, 120, file);
  fgets(line, 120, file);
  if (sscanf(line, "%f%f%f%f%f%f",
             &glsgd.max_tangent, &glsgd.max_dist,
             &glsgd.default_width, &glsgd.default_sep,
             &glsgd.arrow_width, &glsgd.arrow_length) != 6) goto ERR;
  fgets(line, 120, file);
  fgets(line, 120, file);
  if (sscanf(line, "%f%f%f%f%f%f%f%f",
             &glsgd.arrow_break, &glsgd.level_break,
             &glsgd.lsl_csx, &glsgd.lsl_csy,
             &glsgd.lel_csx, &glsgd.lel_csy,
             &glsgd.gel_csx, &glsgd.gel_csy) != 8) goto ERR;
  fgets(line, 120, file);
  fgets(line, 120, file);
  for (j = 0; j < glsgd.nlevels; ++j) {
    fgets(line, 120, file);
    if (sscanf(line, "%*6c%f%f%8c%6c%i%i%i%i %c",
               &glsgd.lev[j].e, &glsgd.lev[j].de, jpi, kay,
               &glsgd.lev[j].band, &glsgd.lev[j].flg,
               &glsgd.lev[j].slflg, &glsgd.lev[j].elflg, &eol) != 9) goto ERR;
    glsgd.lev[j].band--;
    if (eol == '&') {
      fgets(line, 120, file);
      if (sscanf(line, "%*2c%f%f%f%f%f%f",
                 &glsgd.lev[j].sldx, &glsgd.lev[j].sldy,
                 &glsgd.lev[j].eldx, &glsgd.lev[j].eldy,
                 &glsgd.lev[j].dxl, &glsgd.lev[j].dxr) != 6) goto ERR;
    } else {
      glsgd.lev[j].sldx = glsgd.lev[j].sldy = 0.f;
      glsgd.lev[j].eldx = glsgd.lev[j].eldy = 0.f;
      glsgd.lev[j].dxl  = glsgd.lev[j].dxr  = 0.f;
    }
    if (!strncmp(jpi, "       ", 7)) { // empty spin string; spin unknown??
      j1 = 0;
    } else {
      sscanf(jpi, "%i", &j1);
    }
    if (!strncmp(jpi + 5, "/2", 2)) {
      glsgd.lev[j].j = (float) j1 / 2.f;
    } else {
      glsgd.lev[j].j = (float) j1;
    }
    if (jpi[7] == '-') glsgd.lev[j].pi = -1.f;
    else               glsgd.lev[j].pi = 1.f;
    sscanf(kay, "%i", &j1);
    if (!strncmp(kay + 4, "/2", 2)) {
      glsgd.lev[j].k = (float) j1 / 2.f;
    } else {
      glsgd.lev[j].k = (float) j1;
    }
  }
  fgets(line, 120, file);
  for (j = 0; j < glsgd.nbands; ++j) {
    fgets(line, 120, file);
    if (sscanf(line, "%*6c%8c%f%f%f%f%f%f",
               glsgd.bnd[j].name, &glsgd.bnd[j].x0, &glsgd.bnd[j].nx,
               &glsgd.bnd[j].sldx, &glsgd.bnd[j].sldy,
               &glsgd.bnd[j].eldx, &glsgd.bnd[j].eldy) != 7) goto ERR;
  }
  fgets(line, 120, file);
  fgets(line, 120, file);
  fgets(line, 120, file);
  for (j = 0; j < glsgd.ngammas; ++j) {
    fgets(line, 120, file);
    eol = '&';
    glsgd.gam[j].i = glsgd.gam[j].di = 0.0f;
    if (sscanf(line, "%*6c%f%f%*3c%c%*c%c%i%i%f%f %c",
	       &glsgd.gam[j].e, &glsgd.gam[j].de, &glsgd.gam[j].em, &lamda,
	       &glsgd.gam[j].li, &glsgd.gam[j].lf,
	       &glsgd.gam[j].i, &glsgd.gam[j].di, &eol) < 6) goto ERR;
    if (lamda == ' ') lamda = '0';
    glsgd.gam[j].n = (float) (lamda - '0');
    glsgd.gam[j].li--;
    glsgd.gam[j].lf--;
    if (eol == '&') {
      fgets(line, 120, file);
      if (sscanf(line, "%*2c%f%f%f%f%f%f %c",
                 &glsgd.gam[j].a, &glsgd.gam[j].da,
                 &glsgd.gam[j].br, &glsgd.gam[j].dbr,
                 &glsgd.gam[j].d, &glsgd.gam[j].dd, &eol) != 7) goto ERR;
    } else {
      glsgd.gam[j].a  = glsgd.gam[j].da  = 0.0f;
      glsgd.gam[j].br = glsgd.gam[j].dbr = 0.0f;
      glsgd.gam[j].d  = glsgd.gam[j].dd  = 0.0f;
      calc_alpha(j);
    }
    if (eol == '&') {
      fgets(line, 120, file);
      if (sscanf(line, "%*2c%f%f%f%f%i%i",
                 &glsgd.gam[j].x1, &glsgd.gam[j].x2,
                 &glsgd.gam[j].eldx, &glsgd.gam[j].eldy,
                 &glsgd.gam[j].flg, &glsgd.gam[j].elflg) != 6) goto ERR;
    } else {
      glsgd.gam[j].x1 = glsgd.gam[j].x2 = 0.f;
      glsgd.gam[j].eldx = glsgd.gam[j].eldy = 0.f;
      glsgd.gam[j].flg = glsgd.gam[j].elflg = 0;
    }
  }
  if (glsgd.ntlabels > 0) {
  fgets(line, 120, file);
  fgets(line, 120, file);
    for (j = 0; j < glsgd.ntlabels; ++j) {
      fgets(line, 120, file);
      if (sscanf(line, "%*7c%40c%i",
                glsgd.txt[j].l, &glsgd.txt[j].nc) != 2) goto ERR;
      fgets(line, 120, file);
      if (sscanf(line, "%*2c%f%f%f%f",
                 &glsgd.txt[j].csx, &glsgd.txt[j].csy,
                 &glsgd.txt[j].x, &glsgd.txt[j].y) != 4) goto ERR;
    }
  }
  fclose(file);
  rename_input_file_with_tilde = 1;
  return 0;

 ERR:
  warn("ERROR - cannot read ASCII gls file.\n");
  return 0;
} /* read_ascii_gls */

/* ======================================================================= */
int read_write_gls(char *command)
{
  int   i2, jext, j, j1, j2, rlen, rl = 0, nc;
#define BUFSIZE (24*MAXLEV + 61*MAXGAM + 16*MAXBAND + 60*MAXTXT)
  char  buf[BUFSIZE];
  char  filnam[80], dattim[20], jpi[8], kay[8], ext[8];
  FILE  *file;

  file = NULL;
  if (!strcmp(command, "WRITE") ||
      !strcmp(command, "SAVE") ||
      !strcmp(command, "WRITE_AGS")) {

    /* ask for and open level scheme file */
    if (!strcmp(command, "WRITE_AGS")) strcpy(ext, ".ags");
    else strcpy(ext, ".gls");

    if (!strcmp(command, "SAVE") &&
	*glsgd.gls_file_name != ' ' &&
	*glsgd.gls_file_name != '.' &&
	*glsgd.gls_file_name != '\0') {
      strcpy(filnam, glsgd.gls_file_name);
      file = open_save_file(filnam, rename_input_file_with_tilde);
    } else {
      if (*glsgd.gls_file_name != ' ' &&
	  *glsgd.gls_file_name != '\0') {
	nc = askfn(filnam, 80, glsgd.gls_file_name, ext,
		   "Name for new GLS level scheme file = ?\n"
		   "   (Type q to abort)");
      } else {
	nc = askfn(filnam, 80, "", ext,
		   "Name for new GLS level scheme file = ?\n"
		   "   (Type q or rtn to abort)");
      }
      if (nc == 0 ||
	  (nc == 1 && (filnam[0] == 'Q' || filnam[0] == 'q'))) return 1;
      if (!(file = open_new_file(filnam, 0))) return 1;
    }

    jext = setext(filnam, "", 80);
    if (!strcmp(command, "WRITE_AGS") ||
	!strcmp(filnam + jext, ".AGS") ||
	!strcmp(filnam + jext, ".ags")) {
      /* write out ascii version of gls file (.ags file). */
      datetime(dattim);
      fprintf(file,
              "** ASCII Graphical Level Scheme file.\n"
              "** First five lines are reserved for comments.\n"
              "** This file created %.18s\n"
              "** Program GLS,  author D.C. Radford\n"
              "**\n"
              " ASCII GLS file format version 1.0\n", dattim);
      fprintf(file,
              "**  Z Nlevels Ngammas  Nbands Nlabels CharSizeX CharSizeY ArrowWidFact\n"
              "%5i %7i %7i %7i %7i %9.2f %9.2f %9.2f\n"
              "**  MaxArrowTan MaxArrowDX DefBandWid DefBandSep ArrowWidth ArrowLength\n"
              "%15.6f %10.2f %10.2f %10.2f %10.2f %10.2f\n"
              "** ArrowBreak LevelBreak   LevCSX   LevCSY LevEnCSX LevEnCSY   GamCSX   GamCSY\n"
              "%12.2f %11.2f %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f\n"
              "** Level   Energy +/- err     Jpi     K Band# LevelFlag LabelFlag EnLabFlag\n"
              "++   LabelDX   LabelDY EnLabelDX EnLabelDY  LevelDX1  LevelDX2\n",
              glsgd.atomic_no, glsgd.nlevels, glsgd.ngammas, glsgd.nbands, glsgd.ntlabels,
              glsgd.csx, glsgd.csy, glsgd.aspect_ratio, glsgd.max_tangent, glsgd.max_dist,
              glsgd.default_width, glsgd.default_sep, glsgd.arrow_width, glsgd.arrow_length,
              glsgd.arrow_break, glsgd.level_break, glsgd.lsl_csx, glsgd.lsl_csy,
              glsgd.lel_csx, glsgd.lel_csy, glsgd.gel_csx, glsgd.gel_csy);
      for (j = 0; j < glsgd.nlevels; ++j) {
	j2 = (int)(glsgd.lev[j].j * 2.f + 0.5f);
	if (glsgd.lev[j].j < 0.0f) j2 = (int)(glsgd.lev[j].j * 2.f - 0.5f);
	j1 = j2 / 2;
	if (j2 == j1 << 1) {
	  sprintf(jpi, "%7i", j1);
	} else {
	  sprintf(jpi, "%5i/2", j2);
	}
	if (glsgd.lev[j].pi < 0.f) {
	  jpi[7] = '-';
	} else {
	  jpi[7] = '+';
	}
	j2 = (int)(glsgd.lev[j].k * 2.f + 0.5f);
	j1 = j2 / 2;
	if (j2 == j1 << 1) {
	  sprintf(kay, "%6i", j1);
	} else {
	  sprintf(kay, "%4i/2", j2);
	}
      fprintf(file,
              "%6i %10.3f %7.3f%.8s%.6s %5i %9i %9i %9i &\n"
              "++ %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f\n",
              j+1, glsgd.lev[j].e, glsgd.lev[j].de, jpi, kay, glsgd.lev[j].band+1,
              glsgd.lev[j].flg, glsgd.lev[j].slflg, glsgd.lev[j].elflg,
              glsgd.lev[j].sldx, glsgd.lev[j].sldy, glsgd.lev[j].eldx, glsgd.lev[j].eldy,
              glsgd.lev[j].dxl, glsgd.lev[j].dxr);
      }
      fprintf(file,
              "** Band   Name        X0        NX   LabelDX   LabelDY EnLabelDX EnLabelDY\n");
      for (j = 0; j < glsgd.nbands; ++j) {
        fprintf(file,
                "%5i %.8s %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f\n",
                j+1, glsgd.bnd[j].name, glsgd.bnd[j].x0, glsgd.bnd[j].nx,
                glsgd.bnd[j].sldx, glsgd.bnd[j].sldy, glsgd.bnd[j].eldx, glsgd.bnd[j].eldy);
      }
      fprintf(file,
              "** Gamma   Energy +/- err  Mult  ILev  FLev  Intensity +/- err\n"
              "++     ConvCoef +/- error      BrRatio +/- error     MixRatio +/- error\n"
              "++   GammaX1  GammaX2  LabelDX  LabelDY GammaFlag LabelFlag\n");
      for (j = 0; j < glsgd.ngammas; ++j) {
	i2 = (int) (glsgd.gam[j].n + 0.5f);
        fprintf(file,
                "%6i %10.3f %7.3f   %c%2i %5i %5i %10.4f %7.4f &\n"
                "++ %12.5E %9.3E %12.5E %9.3E %12.5E %9.3E &\n"
                "++ %9.2f %9.2f %9.2f %9.2f %9i %9i\n",
                j+1, glsgd.gam[j].e, glsgd.gam[j].de, glsgd.gam[j].em, i2,
                glsgd.gam[j].li+1, glsgd.gam[j].lf+1, glsgd.gam[j].i, glsgd.gam[j].di,
                glsgd.gam[j].a, glsgd.gam[j].da, glsgd.gam[j].br, glsgd.gam[j].dbr,
                glsgd.gam[j].d, glsgd.gam[j].dd, glsgd.gam[j].x1, glsgd.gam[j].x2,
                glsgd.gam[j].eldx, glsgd.gam[j].eldy, glsgd.gam[j].flg, glsgd.gam[j].elflg);
      }
      fprintf(file,
              "** Label                                   text  NChars\n"
              "++     SizeX     SizeY PositionX PositionY\n");
      for (j = 0; j < glsgd.ntlabels; ++j) {
        fprintf(file,
                "%5i  %-40.40s %7i &\n"
                "++ %9.2f %9.2f %9.2f %9.2f\n",
                j+1, glsgd.txt[j].l, glsgd.txt[j].nc,
                glsgd.txt[j].csx, glsgd.txt[j].csy, glsgd.txt[j].x, glsgd.txt[j].y);
      }
    } else {
      /* write out standard unformatted version of gls file. */
#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
#define WR { put_file_rec(file, buf, rl); rl = 0; }
      W(&glsgd.atomic_no, 4);
      W(&glsgd.nlevels, 4);
      W(&glsgd.ngammas, 4);
      W(&glsgd.nbands, 4);
      W(&glsgd.csx, 4);
      W(&glsgd.csy, 4);
      W(&glsgd.aspect_ratio, 4);
      W(&glsgd.max_tangent, 4);
      W(&glsgd.max_dist, 4);
      W(&glsgd.default_width, 4);
      W(&glsgd.default_sep, 4);
      W(&glsgd.arrow_width, 4);
      W(&glsgd.arrow_length, 4);
      WR;
      for (j = 0; j < glsgd.nlevels; ++j) {
	W(&glsgd.lev[j].e, 4*5);
	j1 = glsgd.lev[j].band + 1;
	W(&j1, 4);
      }
      for (j = 0; j < glsgd.ngammas; ++j) {
	j1 = glsgd.gam[j].li + 1;
	j2 = glsgd.gam[j].lf + 1;
	W(&j1, 4);
	W(&j2, 4);
	W(&glsgd.gam[j].em, 1);
	W(&glsgd.gam[j].e, 4*13);
      }
      for (j = 0; j < glsgd.nbands; ++j) {
	W(glsgd.bnd[j].name, 8);
	W(&glsgd.bnd[j].x0, 4*2);
      }
      WR;
      for (j = 0; j < glsgd.nbands; ++j) {
	W(&glsgd.bnd[j].sldx, 4);
      }
      for (j = 0; j < glsgd.ngammas; ++j) {
	W(&glsgd.gam[j].eldx, 4*2);
      }
      for (j = 0; j < glsgd.nlevels; ++j) {
	W(&glsgd.lev[j].sldx, 4);
      }
      WR;
      W(&glsgd.arrow_break, 4);
      W(&glsgd.level_break, 4);
      W(&glsgd.lsl_csx, 4);
      W(&glsgd.lsl_csy, 4);
      W(&glsgd.lel_csx, 4);
      W(&glsgd.lel_csy, 4);
      W(&glsgd.gel_csx, 4);
      W(&glsgd.gel_csy, 4);
      W(&glsgd.ntlabels, 4);
      WR;
      for (j = 0; j < glsgd.ngammas; ++j) {
	W(&glsgd.gam[j].flg, 4*2);
      }
      for (j = 0; j <glsgd.nlevels; ++j) {
	W(&glsgd.lev[j].flg, 4*5);
	W(&glsgd.lev[j].sldy, 4*3);
      }
      for (j = 0; j < glsgd.nbands; ++j) {
	W(&glsgd.bnd[j].sldy, 4*3);
      }
      WR;
      for (j = 0; j < glsgd.ntlabels; ++j) {
	W(glsgd.txt[j].l, 40);
	W(&glsgd.txt[j].nc, 4*5);
      }
      WR;
    }
#undef W
#undef WR
    strcpy(glsgd.gls_file_name, filnam);
    rename_input_file_with_tilde = 0;

  } else if (!strcmp(command, "OPEN_READ")||
	     !strcmp(command, "READ")) {
    if (!strcmp(command, "OPEN_READ")) {
      /* ask for and open level scheme file */
      while (!file) {
	if (!askfn(filnam, 80, "", ".gls",
		   "Name of GLS level scheme file = ?\n"
		   "   (rtn to abort)")) return 1;
	file = open_readonly(filnam);
      }
      strcpy(glsgd.gls_file_name, filnam);
    } else {
      /* open open level scheme file; name in glsgd.gls_file_name */
      jext = setext(glsgd.gls_file_name, ".gls", 80);
      if (!(file = open_readonly(glsgd.gls_file_name))) return 1;
    }
    jext = setext(glsgd.gls_file_name, "", 80);
    if (!strcmp(glsgd.gls_file_name + jext, ".AGS") ||
	!strcmp(glsgd.gls_file_name + jext, ".ags")) {
      read_ascii_gls(file);
      return 0;
    }
#define RR { rlen = get_file_rec(file, buf, BUFSIZE, -1); rl = 0; }
#define R(a,b) { memcpy(a, buf + rl, b); rl += b; }
    RR;
    if (rlen != 13*4) goto ERR;
    R(&glsgd.atomic_no, 4);
    R(&glsgd.nlevels, 4);
    R(&glsgd.ngammas, 4);
    R(&glsgd.nbands, 4);
    R(&glsgd.csx, 4);
    R(&glsgd.csy, 4);
    R(&glsgd.aspect_ratio, 4);
    R(&glsgd.max_tangent, 4);
    R(&glsgd.max_dist, 4);
    R(&glsgd.default_width, 4);
    R(&glsgd.default_sep, 4);
    R(&glsgd.arrow_width, 4);
    R(&glsgd.arrow_length, 4);
    RR;
    if (rlen <= 0) goto ERR;
    for (j = 0; j < glsgd.nlevels; ++j) {
      R(&glsgd.lev[j].e, 4*5);
      R(&glsgd.lev[j].band, 4);
      glsgd.lev[j].band--;
    }
    for (j = 0; j < glsgd.ngammas; ++j) {
      R(&glsgd.gam[j].li, 4*2);
      R(&glsgd.gam[j].em, 1);
      R(&glsgd.gam[j].e, 4*13);
      glsgd.gam[j].li--;
      glsgd.gam[j].lf--;
    }
    for (j = 0; j < glsgd.nbands; ++j) {
      R(glsgd.bnd[j].name, 8);
      R(&glsgd.bnd[j].x0, 4*2);
    }
    RR;
    if (rlen <= 0) goto ERR;
    for (j = 0; j < glsgd.nbands; ++j) {
      R(&glsgd.bnd[j].sldx, 4);
    }
    for (j = 0; j < glsgd.ngammas; ++j) {
      R(&glsgd.gam[j].eldx, 4*2);
    }
    for (j = 0; j < glsgd.nlevels; ++j) {
      R(&glsgd.lev[j].sldx, 4);
    }
    RR;
    if (rlen <= 0) {
      /* no more data in file; must be an old-type gls file.
	 set the following items to default values */
      glsgd.arrow_break = glsgd.arrow_width * .75f;
      glsgd.level_break = glsgd.arrow_width;
      glsgd.lsl_csx = glsgd.csx;
      glsgd.lsl_csy = glsgd.csy;
      glsgd.lel_csx = glsgd.csx;
      glsgd.lel_csy = glsgd.csy;
      glsgd.gel_csx = glsgd.csx;
      glsgd.gel_csy = glsgd.csy;
      glsgd.ntlabels = 0;
      for (j = 0; j < glsgd.ngammas; ++j) {
	glsgd.gam[j].flg = glsgd.gam[j].elflg = 0;
      }
      for (j = 0; j < glsgd.nlevels; ++j) {
	glsgd.lev[j].flg = glsgd.lev[j].slflg = glsgd.lev[j].elflg = 0;
	glsgd.lev[j].dxl = glsgd.lev[j].dxr = glsgd.lev[j].sldy = 0.f;
	glsgd.lev[j].eldx = glsgd.lev[j].eldy = 0.f;
      }
      for (j = 0; j < glsgd.nbands; ++j) {
	glsgd.bnd[j].sldy = glsgd.bnd[j].eldx = glsgd.bnd[j].eldy = 0.f;
      }
    } else {
      R(&glsgd.arrow_break, 4);
      R(&glsgd.level_break, 4);
      R(&glsgd.lsl_csx, 4);
      R(&glsgd.lsl_csy, 4);
      R(&glsgd.lel_csx, 4);
      R(&glsgd.lel_csy, 4);
      R(&glsgd.gel_csx, 4);
      R(&glsgd.gel_csy, 4);
      R(&glsgd.ntlabels, 4);
      RR;
      if (rlen <= 0) goto ERR;
      for (j = 0; j < glsgd.ngammas; ++j) {
	R(&glsgd.gam[j].flg, 4*2);
      }
      for (j = 0; j < glsgd.nlevels; ++j) {
	R(&glsgd.lev[j].flg, 4*5);
	R(&glsgd.lev[j].sldy, 4*3);
      }
      for (j = 0; j < glsgd.nbands; ++j) {
	R(&glsgd.bnd[j].sldy, 4*3);
      }
      if (glsgd.ntlabels > 0) {
	RR;
	if (rlen <= 0) goto ERR;
	for (j = 0; j < glsgd.ntlabels; ++j) {
	  R(glsgd.txt[j].l, 40);
	  R(&glsgd.txt[j].nc, 4*5);
	  if (glsgd.txt[j].nc == 0 || glsgd.txt[j].csx == 0.0f) {
	    j--;
	    glsgd.ntlabels--;
	  }
	}
      }
    }
    rename_input_file_with_tilde = 1;
  }
#undef R
#undef RR
  fclose(file);
  return 0;
 ERR:
  warn("Cannot read file %s\n"
       "   -- perhaps you need to run vms2unix, unix2unix\n"
       "      or unix2vms on the file.\n", glsgd.gls_file_name);
  fclose(file);
  return 1;
} /* read_write_gls */

/* ======================================================================= */
int testint(FILE *outfile)
{
  float r1, sums, a, resid, sumin, sumsin, sumout, sumsout;
  int   i, j;
  char  l[40];

  /* Intensity test on gamma data.
     This subroutine goes through each level adding up all gamma ray
     intesities which feed that level as well as all gamma ray intensities
     that exit.  The error is also calculated, if the difference in
     intensities is greater than two times the error the appropriate
     data is output. */


  if (outfile) {
    fprintf(outfile, "\f\n"
	    "Test of Gamma Ray Intensity Sums\n\n"
	    "     Level    Energy     SumIn    SumOut    Diff      Diff/Err\n"
	    "--------------------------------------------------------------\n");
  } else {
    tell("\f\n"
	 "Test of Gamma Ray Intensity Sums\n\n"
	 "     Level    Energy     SumIn    SumOut    Diff      Diff/Err\n"
	 "--------------------------------------------------------------\n");
  }
  a = 0.0f;
  for (j = 0; j < glsgd.nlevels; ++j) {
    if (glsgd.lev[j].e > 0.0f) {
      sumin = sumout = sumsin = sumsout = 0.0f;
      /* check gammas entering or leaving level j */
      for (i = 0; i < glsgd.ngammas; ++i) {
	if (glsgd.gam[i].lf == j) {
	  a = glsgd.gam[i].a;
	  sumin += (a + 1.0f) * glsgd.gam[i].i;
	  r1 = (a + 1.0f) * glsgd.gam[i].di;
	  sumsin += r1 * r1;
	} else if (glsgd.gam[i].li == j) {
	  a = glsgd.gam[i].a;
	  sumout += (a + 1.0f) * glsgd.gam[i].i;
	  r1 = (a + 1.0f) * glsgd.gam[i].di;
	  sumsout += r1 * r1;
	}
      }
      sums = sqrt(sumsin + sumsout);
      resid = 0.0f;
      if (sums > 0.0f) resid = (sumin - sumout) / sums;
      if (sumin - sumout > sums * 2.0f) {
	*l = '\0';
	wrresult(l, sumin - sumout, sums, 12);
	if (outfile) {
	  fprintf(outfile, "%s %9.2f %9.2f %9.2f   %s %6.1f\n",
		  glsgd.lev[j].name, glsgd.lev[j].e,
		  sumin, sumout, l, resid);
	} else {
	  tell("%s %9.2f %9.2f %9.2f   %s %6.1f\n",
	       glsgd.lev[j].name, glsgd.lev[j].e,
	       sumin, sumout, l, resid);
	}
      }
    }
  }
  return 0;
} /* testint */

/* ================================================================== */
int checksum(int i, int f, int a, int b, int c, int d, FILE *outfile)
{
  float diff, e1, e2, e3, e4, de1, de2, de3, de4, err;
  char  l[40] = "";

  e1 = e2 = e3 = e4 = 0.0f;
  de1 = de2 = de3 = de4 = 0.0f;
  if (a >= 0) {
    e1 = glsgd.gam[a].e;
    de1 = glsgd.gam[a].de;
  }
  if (b >= 0) {
    e2 = glsgd.gam[b].e;
    de2 = glsgd.gam[b].de;
  }
  if (c >= 0) {
    e3 = glsgd.gam[c].e;
    de3 = glsgd.gam[c].de;
  }
  if (d >= 0) {
    e4 = glsgd.gam[d].e;
    de4 = glsgd.gam[d].de;
  }
  diff = fabs(e1 + e2 - e3 - e4);
  err = sqrt(de1*de1 + de2*de2 + de3*de3 + de4*de4);
  if (err > 0.0f && (diff/err) > 3.0f) {
    wrresult(l, diff, err, 12);
    if (outfile) {
      fprintf(outfile,
	      "%s %7.1f %7.1f %7.1f %7.1f %8.1f %7.1f  %s %5.1f\n",
	      glsgd.lev[i].name,
	      e1, e2, e3, e4, e1 + e2, e3 + e4, l, diff/err);
    } else {
      tell("%s %7.1f %7.1f %7.1f %7.1f %8.1f %7.1f  %s %5.1f\n",
	   glsgd.lev[i].name,
	   e1, e2, e3, e4, e1 + e2, e3 + e4, l, diff/err);
    }
  }
  return 0;
} /* checksum */

/* ====================================================================== */
int testene1(FILE *outfile)
{
  float a, resid;
  int   j;
  char  l[40];

  /* check the difference between gamma energies and initial-final energies */
  /* if the difference is > 3*dE output message */
  if (outfile) {
    fprintf(outfile,
	    "Test of Gamma-ray and Level Energies\n\n"
	    "        Li ->      Lf    Egamma     Ei-Ef   Diff     Diff/Err\n"
	    "-------------------------------------------------------------\n");
  } else {
    tell("Test of Gamma-ray and Level Energies\n\n"
	 "        Li ->      Lf    Egamma     Ei-Ef   Diff     Diff/Err\n"
	 "-------------------------------------------------------------\n");
  }
  for (j = 0; j < glsgd.ngammas; ++j) {
    a = glsgd.lev[glsgd.gam[j].li].e - glsgd.lev[glsgd.gam[j].lf].e;
    resid = 0.0f;
    if (glsgd.gam[j].de > 0.0f)
      resid = fabs(glsgd.gam[j].e - a) / glsgd.gam[j].de;
    if (resid > 3.0f) {
      *l = '\0';
      wrresult(l, fabs(glsgd.gam[j].e - a), glsgd.gam[j].de, 10);
      if (outfile) {
	fprintf(outfile, "%s %s %9.1f %9.1f  %s %7.1f\n",
		glsgd.lev[glsgd.gam[j].li].name,
		glsgd.lev[glsgd.gam[j].lf].name,
		glsgd.gam[j].e, a, l, resid);
      } else {
	tell("%s %s %9.1f %9.1f  %s %7.1f\n",
	     glsgd.lev[glsgd.gam[j].li].name,
	     glsgd.lev[glsgd.gam[j].lf].name,
	     glsgd.gam[j].e, a, l, resid);
      }
    }
  }
  return 0;
} /* testene1 */

/* ====================================================================== */
int testene2(FILE *outfile)
{
  int li, ig1, ig2, ig3, ig4, lf1, lf2;

  if (outfile) {
    fprintf(outfile,
	    "\f\n"
	    "Test of Gamma-ray Energy Sums\n\n"
	    "        Li     Eg1     Eg2     Eg3     Eg4"
	    "     Sum1    Sum2   Diff     Diff/Err\n"
	    "------------------------------------------"
	    "-------------------------------------\n");
  } else {
    tell("\f\n"
	 "Test of Gamma-ray Energy Sums\n\n"
	 "        Li     Eg1     Eg2     Eg3     Eg4"
	 "     Sum1    Sum2   Diff     Diff/Err\n"
	 "------------------------------------------"
	 "-------------------------------------\n");
  }
  for (ig1 = 0; ig1 < glsgd.ngammas; ++ig1) {
    li = glsgd.gam[ig1].li;
    lf1 = glsgd.gam[ig1].lf;
    for (ig2 = ig1 + 1; ig2 < glsgd.ngammas; ++ig2) {
      if (li != glsgd.gam[ig2].li) continue;
      lf2 = glsgd.gam[ig2].lf;
      for (ig3 = 0; ig3 < glsgd.ngammas; ++ig3) {
	if (lf1 == glsgd.gam[ig3].li) {
	  if (lf2 == glsgd.gam[ig3].lf) {
	    checksum(li, lf2, ig1, ig3, ig2, -1, outfile);
	  } else {
	    for (ig4 = 0; ig4 < glsgd.ngammas; ++ig4) {
	      if (lf2 == glsgd.gam[ig4].li &&
		  glsgd.gam[ig4].lf == glsgd.gam[ig3].lf)
		checksum(li, glsgd.gam[ig4].lf, ig1, ig3, ig2, ig4, outfile);
	    }
	  }
	} else if (lf2 == glsgd.gam[ig3].li &&
		   lf1 == glsgd.gam[ig3].lf)
	    checksum(li, lf1, ig1, -1, ig2, ig3, outfile);
      }
    }
  }
  return 0;
} /* testene2 */

/* ====================================================================== */
int testsums(void)
{
  FILE *outfile;
  char filnam[80] = "sums.dat";

  /* test energy and intensity sums */
  calc_band();
  testene1(NULL);
  testene2(NULL);
  testint(NULL);
  if (askyn("Print these results? (Y/N)")) {
    outfile = open_new_file(filnam, 0);
    testene1(outfile);
    testene2(outfile);
    testint(outfile);
    fclose(outfile);
    pr_and_del_file(filnam);
  }
  return 0;
} /* testsums */
