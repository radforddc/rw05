#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "gls.h"
#include "util.h"
#include "minig.h"

/* This file has functions which are used by gls, escl8r, levit8r and 4dg8r.
   However, gtkgls and the versions of escl8r etc. using the GTK GUI
   have replacement versions of these functions. */

/* ==== other extern routines ==== */
extern int drawstring(char *, int, char, float, float, float, float);
extern int retic2(float *xout, float *yout, char *cout);
extern int initg2(int *nx, int *ny);

/* ======================================================================= */
int edit_band(void)
{
  float f, x1, y1, de, dp, ds, fj1, fj2;
  int   ilev, i, iband, bf, bi, nc, ids, iip, iis, iis2, errflag;
  char  name[8], ans[80], ch;

 START:
  tell("Select band to edit (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  iband = nearest_band(x1, y1);
  if (iband < 0 || iband > glsgd.nbands) {
    tell("  ... no band selected, try again...\n");
  } else {
    tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    save_gls_now(7);

    while (askyn("Change name of band? (Y/N)")) {
      while ((nc = ask(ans, 80,
		       "New name for band = ? (1-8 characters)")) < 1 || nc > 8);
      strncpy(name, "         ", 8);
      strncpy(name + 8 - nc, ans, nc);
      errflag = 0;
      for (i = 0; i < glsgd.nbands; ++i) {
	if (!strncmp(glsgd.bnd[i].name, name, 8)) {
	  warn("ERROR - band name already in use...\n");
	  errflag = 1;
	  break;
	}
      }
      if (!errflag) {
	strncpy(glsgd.bnd[iband].name, name, 8);
	break;
      }
    }

    while (askyn("Change width of band? (Y/N)")) {
      if (!(nc = ask(ans, 80,
		     "Present value of width = %.0f\n"
		     "New width = ? (rtn for present value)",
		     glsgd.bnd[iband].nx)))
	break;
      if (!ffin(ans, nc, &f, &fj1, &fj2) && f > 0.f) {
	glsgd.bnd[iband].nx = f;
	for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
	  if (glsgd.lev[ilev].band == iband &&
	      glsgd.lev[ilev].dxl - glsgd.lev[ilev].dxr >=
	      glsgd.bnd[iband].nx * .8f) {
	    glsgd.lev[ilev].dxl = 0.f;
	    glsgd.lev[ilev].dxr = 0.f;
	  }
	}
	break;
      }
    }

    while (1) {
      ds = 0.0f;
      if (!askyn("Change spin of band? (Y/N)")) break;
      while ((nc = ask(ans, 80, "Spin increment = ?")) &&
	     ffin(ans, nc, &ds, &fj1, &fj2)) ds = 0.0f;
      iis = (int) rint(ds);
      iis2 = (int) rint(ds*2.0f);
      if (ds != (float) iis2 / 2.0f) {
	warn("ERROR -- spin change must be integer or half-integer.\n");
	continue;
      }
      errflag = 0;
      for (i = 0; i < glsgd.ngammas; ++i) {
	bi = glsgd.lev[glsgd.gam[i].li].band;
	bf = glsgd.lev[glsgd.gam[i].lf].band;
	if ((bi == iband || bf == iband) && bi != bf &&
	    ds != (float) iis) {
	  warn("ERROR -- spin change must be integer\n"
	       "    since there are interband transitions.\n");
	  errflag = 1;
	  break;
	}
      }
      for (i = 0; i < glsgd.nlevels; ++i) {
	if (glsgd.lev[i].band == iband &&
	    (glsgd.lev[i].j + ds) < -.1f) {
	  warn("ERROR -- cannot have spins < 0\n");
	  errflag = 1;
	  break;
	}
      }
      if (!errflag) break;
    }

    iip = 1;
    if (askyn("Change parity of band? (Y/N)")) iip = -1;

    if (ds != 0.0f || iip == -1) {
      for (i = 0; i < glsgd.nlevels; ++i) {
	if (glsgd.lev[i].band == iband) {
	  glsgd.lev[i].j += ds;
	  glsgd.lev[i].pi *= (float) iip;
	}
      }
      for (i = 0; i < glsgd.ngammas; ++i) {
	bi = glsgd.lev[glsgd.gam[i].li].band;
	bf = glsgd.lev[glsgd.gam[i].lf].band;
	if ((bi == iband || bf == iband) && bi != bf) {
	  ids = abs((int) (glsgd.lev[glsgd.gam[i].li].j -
			   glsgd.lev[glsgd.gam[i].lf].j + 0.5f));
	  if (ids > 9) ids = 9;
	  if (ids == 0) ids = 1;
	  dp = glsgd.lev[glsgd.gam[i].li].pi *
	    glsgd.lev[glsgd.gam[i].lf].pi;
	  if (dp > 0.f) {
	    if (ids % 2 == 0) ch = 'E';
	    else ch = 'M';
	  } else {
	    if (ids % 2 == 0) ch = 'M';
	    else ch = 'E';
	  }
	  glsgd.gam[i].em = ch;
	  glsgd.gam[i].n = (float) ids;
	  tell("Gamma %.1f now has multipolarity %c%i\n",
	       glsgd.gam[i].e, ch, ids);
	}
      }
    }
    while (1) {
      de = 0.0f;
      if (!askyn("Change energy of band? (Y/N)")) break;
      while ((nc = ask(ans, 80, "Energy increment = ?")) &&
	     ffin(ans, nc, &de, &fj1, &fj2)) de = 0.f;
      if (de == 0.f) break;
      errflag = 0;
      for (i = 0; i < glsgd.ngammas; ++i) {
	if (glsgd.lev[glsgd.gam[i].li].band == iband &&
	    glsgd.lev[glsgd.gam[i].lf].band != iband) {
	  f = glsgd.lev[glsgd.gam[i].li].e -
	      glsgd.lev[glsgd.gam[i].lf].e + de;
	} else if (glsgd.lev[glsgd.gam[i].li].band != iband && 
		   glsgd.lev[glsgd.gam[i].lf].band == iband) {
	  f = glsgd.lev[glsgd.gam[i].li].e -
	      glsgd.lev[glsgd.gam[i].lf].e - de;
	} else {
	  continue;
	}
	if (f < 0.f) {
	  warn("ERROR -- cannot have gammas with energy < 0\n");
	  errflag = 1;
	  break;
	}
      }
      if (errflag) continue;
      for (i = 0; i < glsgd.nlevels; ++i) {
	if (glsgd.lev[i].band == iband &&
	    (glsgd.lev[i].e + de) < -0.0001f) {
	  warn("ERROR -- cannot have levels with energy < 0\n");
	  errflag = 1;
	  break;
	}
      }
      if (errflag) continue;

      for (i = 0; i < glsgd.nlevels; ++i) {
	if (glsgd.lev[i].band == iband) glsgd.lev[i].e += de;
      }
      for (i = 0; i < glsgd.ngammas; ++i) {
	if ((glsgd.lev[glsgd.gam[i].li].band == iband ||
	     glsgd.lev[glsgd.gam[i].lf].band == iband) &&
	    glsgd.lev[glsgd.gam[i].li].band != 
	    glsgd.lev[glsgd.gam[i].lf].band) {
	  f = glsgd.lev[glsgd.gam[i].li].e -
	    glsgd.lev[glsgd.gam[i].lf].e;
	  tell("Gamma %.1f now has energy %.1f\n", glsgd.gam[i].e, f);
	  glsgd.gam[i].e = f;
	}
      }
      break;
    }

    if (ds != 0.f || iip != 1 || de != 0.f) {
      tell("Conversion coefficients may be recalculated\n"
	   "   for some transitions...\n");
      for (i = 0; i < glsgd.ngammas; ++i) {
	if ((glsgd.lev[glsgd.gam[i].li].band == iband || 
	     glsgd.lev[glsgd.gam[i].lf].band == iband) && 
	    glsgd.lev[glsgd.gam[i].li].band !=
	    glsgd.lev[glsgd.gam[i].lf].band) {
	  calc_alpha(i);
	}
      }
    }
    calc_band();
    calc_gls_fig();
    display_gls(-1);
    path(1);
    calc_peaks();
  }
  goto START;
} /* edit_band */

/* ======================================================================= */
int edit_gamma(void)
{
  float f1, f2, f3, x1, y1;
  int   igam, i, nc, nsx, nsy;
  char  ans[80];

 START:
  tell("Select gamma to edit (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    calc_gls_fig();
    display_gls(-1);
    return 0;
  }
  igam = nearest_gamma(x1, y1);
  if (igam < 0) {
    tell("  ... no gamma selected, try again...\n");
  } else {
    tell(" Gamma %.1f  selected...\n"
	 "           Energy = %.3f +- %.3f\n"
	 "        Intensity = %.3f +- %.3f\n"
	 "     Mixing ratio = %.5g\n"
	 " Conversion coeff = %.5g\n"
	 "  (rtn to keep present values...) \n",
	 glsgd.gam[igam].e, glsgd.gam[igam].e, glsgd.gam[igam].de,
	 glsgd.gam[igam].i, glsgd.gam[igam].di,
	 glsgd.gam[igam].d, glsgd.gam[igam].a);
    save_gls_now(2);

    while ((nc = ask(ans, 80, "... Energy, error = ? (rtn for %.3f, %.3f)",
		     glsgd.gam[igam].e, glsgd.gam[igam].de))) {
      if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 0.f && f2 >= 0.f) {
	if (f1 > 0.f) glsgd.gam[igam].e = f1;
	if (f2 > 0.f) glsgd.gam[igam].de = f2;
	break;
      }
      warn("ERROR -- illegal value...\n");
    }
    while ((nc = ask(ans, 80, "... Intensity, error = ? (rtn for %.3f, %.3f)",
		     glsgd.gam[igam].i, glsgd.gam[igam].di))) {
      if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 0.f && f2 >= 0.f) {
	if (f1 > 0.f) glsgd.gam[igam].i = f1;
	if (f2 > 0.f) glsgd.gam[igam].di = f2;
	break;
      }
      warn("ERROR -- illegal value...\n");
    }
    while ((nc = ask(ans, 80, "... Mixing ratio = ? (rtn for %.5g)",
		    glsgd.gam[igam].d ))) {
      if (!ffin(ans, nc, &f1, &f2, &f3)) {
	glsgd.gam[igam].d = f1;
	break;
      }
    }
    while ((nc = ask(ans, 80, "... Conversion coeff. = ? (rtn for %.5g) (D to recalculate)",
		     glsgd.gam[igam].a))) {
      if (*ans == 'D' || *ans == 'd') {
	calc_alpha(igam);
	tell(" Conversion coeff = %.5g\n", glsgd.gam[igam].a);
	break;
      } else if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 0.f) {
	if (f1 > 0.f) glsgd.gam[igam].a = f1;
	break;
      }
      warn("ERROR -- illegal value...\n");
    }

    initg2(&nsx, &nsy);
    setcolor(0);
    pspot(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
    for (i = 1; i < glsgd.gam[igam].np; ++i) {
      vect(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
    }
    drawstring(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H',
	       glsgd.gam[igam].elx, glsgd.gam[igam].ely,
	       glsgd.gel_csx, glsgd.gel_csy);
    calc_gam(igam);
    setcolor(2);
    pspot(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
    for (i = 1; i < glsgd.gam[igam].np; ++i) {
      vect(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
    }
    setcolor(1);
    drawstring(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H',
	       glsgd.gam[igam].elx, glsgd.gam[igam].ely,
	       glsgd.gel_csx, glsgd.gel_csy);
    finig();
    path(1);
    calc_peaks();
  }
  goto START;
} /* edit_gamma */

/* ======================================================================= */
int edit_level(void)
{
  float f, x1, y1, de, dp, fj1, fj2;
  int   ilev, i, j1, j2, nc, ids, iip, iis, errflag;
  char  ans[80], ch;

 START:
  tell("Select level to edit (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  ilev = nearest_level(x1, y1);
  if (ilev < 0) {
    tell("  ... no level selected, try again...\n");
  } else {
    tell(" Level %s selected...\n", glsgd.lev[ilev].name);
    save_gls_now(3);

    while (1) {
      iis = 0;
      if (!askyn("Change spin of level? (Y/N)")) break;
      while ((nc = ask(ans, 80, "Spin increment = ?")) &&
	     inin(ans, nc, &iis, &j1, &j2)) iis = 0;
      errflag = 0;
      for (i = 0; i < glsgd.nlevels; ++i) {
	if (i != ilev &&
	    glsgd.lev[i].band == glsgd.lev[ilev].band &&
	    fabs(glsgd.lev[i].j - (glsgd.lev[ilev].j + iis)) < 0.7) {
	  errflag = 1 - askyn("Warning: That band already has a state"
			      " of the same spin...\n"
			      "   ...Continue anyway? (Y/N)");
	  break;
	}
      }
      if (errflag) continue;
      if ((glsgd.lev[ilev].j + (float) iis) > -.1f) break;
      warn("ERROR -- cannot have spins < 0\n");
    }
    glsgd.lev[ilev].j += (float) iis;

    iip = 1;
    if (askyn("Change parity of level? (Y/N)")) {
      iip = -1;
      glsgd.lev[ilev].pi *= (float) iip;
    }

    if (iis != 0 || iip < 0) {
      for (i = 0; i < glsgd.ngammas; ++i) {
	if (glsgd.gam[i].li == ilev || glsgd.gam[i].lf == ilev) {
	  ids = abs((int) (glsgd.lev[glsgd.gam[i].li].j -
			   glsgd.lev[glsgd.gam[i].lf].j + 0.5f));
	  if (ids > 9) ids = 9;
	  if (ids == 0) ids = 1;
	  dp = glsgd.lev[glsgd.gam[i].li].pi *
	    glsgd.lev[glsgd.gam[i].lf].pi;
	  if (dp > 0.f) {
	    if (ids % 2 == 0) ch = 'E';
	    else ch = 'M';
	  } else {
	    if (ids % 2 == 0) ch = 'M';
	    else ch = 'E';
	  }
	  glsgd.gam[i].em = ch;
	  glsgd.gam[i].n = (float) ids;
	  tell("Gamma %.1f now has multipolarity %c%i\n",
	       glsgd.gam[i].e, ch, ids);
	}
      }
    }

    while (1) {
      de = 0.0f;
      if (!askyn("Change energy of level? (Y/N)")) break;
      while ((nc = ask(ans, 80, "Energy increment = ?")) &&
	     ffin(ans, nc, &de, &fj1, &fj2)) de = 0.f;
      if (de == 0.f) break;
      errflag = 0;
      for (i = 0; i < glsgd.ngammas; ++i) {
	if (glsgd.gam[i].li == ilev) {
	  f = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e + de;
	} else if (glsgd.gam[i].lf == ilev) {
	  f = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e - de;
	} else {
	  continue;
	}
	if (f < 0.f) {
	  warn("ERROR -- cannot have gammas with energy < 0\n");
	  errflag = 1;
	  break;
	}
      }
      if (errflag) continue;
      if ((glsgd.lev[ilev].e + de) < -0.0001f) {
	warn("ERROR -- cannot have levels with energy < 0\n");
	continue;
      }
      glsgd.lev[ilev].e += de;
      for (i = 0; i < glsgd.ngammas; ++i) {
	if (glsgd.gam[i].li == ilev || glsgd.gam[i].lf == ilev) {
	  f = glsgd.lev[glsgd.gam[i].li].e - glsgd.lev[glsgd.gam[i].lf].e;
	  tell("Gamma %.1f now has energy %.1f\n", glsgd.gam[i].e, f);
	  glsgd.gam[i].e = f;
	}
      }
    }

    if (iis != 0 || iip != 1 || de != 0.f) {
      tell("Conversion coefficients may be recalculated\n"
	   "   for some transitions...\n");
      for (i = 0; i < glsgd.ngammas; ++i) {
	if (glsgd.gam[i].li == ilev || glsgd.gam[i].lf == ilev) calc_alpha(i);
      }
    }
    calc_band();
    calc_gls_fig();
    path(1);
    calc_peaks();
    display_gls(-1);
  }
  goto START;
} /* edit_level */

/* ======================================================================= */
int edit_text(void)
{
  float x1, y1, fj;
  int   i, nc, it, new_nc, nsx, nsy;
  char  ans[80], newlab[80];

  if (glsgd.ntlabels == 0) {
    warn("Have no text labels to edit.\n");
    return 0;
  }
 START:
  tell("...Select text label to edit (X or button 3 to exit)\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  }
  it = -1;
  for (i = 0; i < glsgd.ntlabels; ++i) {
    if (x1 > glsgd.txt[i].x - (float) glsgd.txt[i].nc * glsgd.txt[i].csx / 2.f &&
	x1 < glsgd.txt[i].x + (float) glsgd.txt[i].nc * glsgd.txt[i].csx / 2.f &&
	y1 > glsgd.txt[i].y &&
	y1 < glsgd.txt[i].y + glsgd.txt[i].csy) {
      it = i;
    }
  }
  if (it < 0) {
    tell(" ... No text label selected, try again ...\n");
    goto START;
  }
  tell("...Selected label %.40s\n", glsgd.txt[it].l);
  save_gls_now(8);
  new_nc = ask(newlab, 40, "...New text for label = ? (rtn to leave unchanged)");
  while (1) {
    x1 = 0.f;
    y1 = 0.f;
    if (!(nc = ask(ans, 80,
		   " ...Current character size is %.0f\n"
		   " ...New character size = ? (rtn for %.0f)",
		   glsgd.txt[it].csx, glsgd.txt[it].csx)) ||
	!ffin(ans, nc, &x1, &fj, &fj)) break;
  }
  initg2(&nsx, &nsy);
  setcolor(0);
  drawstring(glsgd.txt[it].l, glsgd.txt[it].nc, 'H',
	     glsgd.txt[it].x, glsgd.txt[it].y,
	     glsgd.txt[it].csx, glsgd.txt[it].csy);
  setcolor(1);
  if (new_nc > 0) {
    strcpy(glsgd.txt[it].l, newlab);
    glsgd.txt[it].nc = new_nc;
  }
  if (x1 > 0.f) {
    glsgd.txt[it].csx = x1;
    glsgd.txt[it].csy = x1*CSY;
  }
  drawstring(glsgd.txt[it].l, glsgd.txt[it].nc, 'H',
	     glsgd.txt[it].x, glsgd.txt[it].y,
	     glsgd.txt[it].csx, glsgd.txt[it].csy);
  finig();
  goto START;
} /* edit_text */

/* ======================================================================= */
int figure_pars(void)
{
  float angle, f1, f2, f3;
  int   i, nc;
  char  ans[80];

  save_gls_now(0);
  /* all figure distance units are in keV */
  /* gamma arrow widths = intensity*aspect_ratio (keV) */
  angle = atan(glsgd.max_tangent) * 57.29578f;
  tell("\n"
       "Current values are: Label character sizes (x,y): %.0f, %.0f\n"
       "                          Max. angle for gammas: %.0f\n"
       "              Max. sideways distance for gammas: %.0f\n"
       "              Default band width and separation: %.0f, %.0f\n"
       "         Gamma arrow width \"keV\"/intensity unit: %.2f\n"
       "               Gamma arrowhead width and length: %.0f, %.0f\n"
       "         Tentative gamma and level break length: %.0f, %.0f\n"
       "  (rtn to keep present values...) \n",
       glsgd.csx, glsgd.csy, angle, glsgd.max_dist,
       glsgd.default_width, glsgd.default_sep, glsgd.aspect_ratio,
       glsgd.arrow_width, glsgd.arrow_length,
       glsgd.arrow_break, glsgd.level_break);

  while ((nc = ask(ans, 80, " ...Label sizes = ?"))) {
    if (ffin(ans, nc, &f1, &f2, &f3)) continue;
    if (f1 > 0.f) {
      if (glsgd.gel_csx == glsgd.csx) glsgd.gel_csx = f1;
      if (glsgd.lsl_csx == glsgd.csx) glsgd.lsl_csx = f1;
      if (glsgd.lel_csx == glsgd.csx) glsgd.lel_csx = f1;
      glsgd.csx = f1;
      glsgd.gel_csy = glsgd.gel_csx*CSY;
      glsgd.lsl_csy = glsgd.lsl_csx*CSY;
      glsgd.lel_csy = glsgd.lel_csx*CSY;
      for (i=0; i<glsgd.ntlabels; i++) glsgd.txt[i].csy = glsgd.txt[i].csx*CSY;
    }
    if (f2 > 0.f) {
      glsgd.csy = f1;
      glsgd.gel_csy = glsgd.gel_csx*CSY;
      glsgd.lsl_csy = glsgd.lsl_csx*CSY;
      glsgd.lel_csy = glsgd.lel_csx*CSY;
      for (i=0; i<glsgd.ntlabels; i++) glsgd.txt[i].csy = glsgd.txt[i].csx*CSY;
    }
    break;
  }
  while ((nc = ask(ans, 80, " ...Max. angle = ?")) &&
	 ffin(ans, nc, &angle, &f2, &f3));
  glsgd.max_tangent = tan(fabs(angle) / 57.29578f);
  while ((nc = ask(ans, 80, " ...Max. sideways distance = ?"))) {
    if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 0.f) {
      glsgd.max_dist = f1;
      break;
    }
    warn("ERROR -- illegal value...\n");
  }
  while ((nc = ask(ans, 80, " ...Band width and separation = ?"))) {
    if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 0.f && f2 >= 0.f) {
      if (f1 > 0.f) glsgd.default_width = f1;
      if (f2 > 0.f) glsgd.default_sep = f2;
      break;
    }
    warn("ERROR -- illegal value...\n");
  }
  while ((nc = ask(ans, 80, " ...Arrow width factor = ?")) &&
	 ffin(ans, nc, &glsgd.aspect_ratio, &f2, &f3));
  while ((nc = ask(ans, 80, " ...Arrowhead width and length = ?"))) {
    if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 0.f && f2 >= 0.f) {
      if (f1 > 0.f) glsgd.arrow_width = f1;
      if (f2 > 0.f) glsgd.arrow_length = f2;
      break;
    }
    warn("ERROR -- illegal value...\n");
  }
  while ((nc = ask(ans, 80, " ...Tentative gamma and level break length = ?"))) {
    if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 0.f && f2 >= 0.f) {
      if (f1 > 0.f) glsgd.arrow_break = f1;
      if (f2 > 0.f) glsgd.level_break = f2;
      break;
    }
    warn("ERROR -- illegal value...\n");
  }
  return 0;
} /* figure_pars */

/* ======================================================================= */
int gls_get_ps_details(char *title, float *htitle, char *titpos,
		       float *hyaxis, char *scalepos,
		       char *filnam, char *printcmd)
{
  float fj1, fj2;
  int   nc, jext;
  char  ans[80];

  *title = '\0';
  if (askyn("Add title? (Y/N)")) {
    while ((nc = ask(ans, 80, "Enter character height (rtn for 200) ?")) &&
	   ffin(ans, nc, htitle, &fj1, &fj2));
    if (!nc) *htitle = 200.0f;
    while (1) {
      ask(ans, 80, "Put title at Top(T) or Bottom(B) ?");
      if (*ans == 'T' || *ans == 't' || *ans == 'B' || *ans == 'b') break;
    }
    *titpos = *ans;
    if (!ask(title, 80,
	     "Default title is: From %.72s\n"
	     " Use {u...} for superscripts...\n"
	     "...Enter title (max. 80 chars, rtn for default) ?",
	     glsgd.gls_file_name)) {
      strcpy(title, "From ");
      strncat(title, glsgd.gls_file_name, 72);
    }
  }

  *scalepos = '\0';
  if (askyn("Add y-axis (excitation energy in MeV)? (Y/N)")) {
    while ((nc = ask(ans, 80, "Enter character height (rtn for 200) ?")) &&
	   ffin(ans, nc, hyaxis, &fj1, &fj2));
    if (!nc) *hyaxis = 200.0f;
    while (1) {
      ask(ans, 80, "Put y-axis at Left(L) or Right(R) ?");
      if (*ans == 'L' || *ans == 'l' || *ans == 'R' || *ans == 'r') break;
    }
    *scalepos = *ans;
  }

  /* open output file */
  strcpy(filnam, glsgd.gls_file_name);
  jext = setext(filnam, ".ps", 80);
  filnam[jext] = '\0';
  jext = setext(filnam, ".ps", 80);
  nc = askfn(ans, 80, filnam, ".ps", "Output postscript file = ?\n");
  strcpy(filnam, ans);

  *printcmd = '\0';
  if (askyn("Print and delete the postscript file now? (Y/N)\n"
	    "                (If No, the file will be saved)")) *printcmd = '1';
  return 0;
} /* gls_get_ps_details */

/* ======================================================================= */
int gls_get_newgls_details(int *z, float *spin, int *parity, char *name)
{
  float fj1, fj2;
  int   nc, j1, j2;
  char  ans[80];

  while (!(nc = ask(ans, 80, "Atomic number (Z) of nucleus to be studied = ?")) ||
	 inin(ans, nc, z, &j1, &j2) ||
	 *z < 3 || *z > 110) {
    tell("??? Bad value, must be between 3 and 110...\n");
  }
  while (!(nc = ask(ans, 80, "Ground state spin = ?")) ||
	 ffin(ans, nc, spin, &fj1, &fj2) || *spin < 0);
  *spin = ((float) ((int) (*spin*2.0f + 0.5f))) * 0.5f;
  *parity = -1;
  if (askyn(" ...Parity = even? (Y/N)")) *parity = 1;

  while ((nc = ask(ans, 80, "Name for ground band = ? (1-8 characters)")) < 1 ||
	 nc > 8);
  strncpy(name, "        ", 8);
  strncpy(name + 8 - nc, ans, nc);

  return 0;
} /* gls_get_newgls_details */

/* ======================================================================= */
void undo_gls_notify(int flag)
{
  /* flag = 1: undo_edits now available
            2: redo_edits now available
            3: undo_edits back at flagged position
           -1: undo_edits no longer available
           -2: redo_edits no longer available
           -3: undo_edits no longer at flagged position */
  /* this is a dummy version of the routine,
     for replacement in GUI versions of gls */
}
