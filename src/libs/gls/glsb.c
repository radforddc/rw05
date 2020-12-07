#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "gls.h"
#include "util.h"
#include "minig.h"

/*  set of routines used only by graphical gls programs,
    i.e. gls, xmgls, escl8r... etc., but not gls_conv */

/* ==== other extern routines ==== */
extern int drawstring(char *, int, char, float, float, float, float);
extern int retic2(float *xout, float *yout, char *cout);
extern int initg2(int *nx, int *ny);

int glsundo_ready = 0;

/* all figure distance units are in keV */
/* gamma arrow widths = intensity * aspect_ratio (in keV units) */

/* ======================================================================= */
int add_gamma(void)
{
  float x1, y1, x2, y2, ds, dp, energy;
  int   j, iband, lf, li, ids = 1;
  char  ch, ans[80];

  /* get initial level for new gamma */
 START:
  if (glsgd.ngammas >= MAXGAM) {
    tell("You now have the maximum of %d gammas in the scheme!\n"
	 " Before you can add more, you need to change MAXGAM in\n"
	 " the file gls.h, and recompile radware.\n", MAXGAM);
    *ans = 'X';
  } else {    
    tell("Select initial level for new gamma"
	 " (N for new level, X or button 3 to exit)...\n");
    retic2(&x1, &y1, ans);
  }
  if (*ans == 'X' || *ans == 'x') return 0;

  if (*ans == 'N' || *ans == 'n') {
    tell("...Select band for new initial level (A to abort, N for new band)...\n");
  GET_BI:
    retic2(&x1, &y1, ans);
    if (*ans == 'A' || *ans == 'a') goto START;
    if (*ans == 'N' || *ans == 'n') {
      iband = glsgd.nbands;
      tell("   ... Give position for new band (A to abort)...\n");
      retic2(&x2, &y2, ans);
      if (*ans == 'A' || *ans == 'a') goto START;
      glsgd.bnd[iband].x0 = x2 - glsgd.bnd[iband].nx / 2.f;
    } else {
      iband = nearest_band(x1, y1);
    }
    if (iband < 0) {
      tell("  ... no band selected, try again...\n");
      goto GET_BI;
    }
    if (iband >= glsgd.nbands) {
      if (insert_band(&iband)) goto START;
    } else {
      tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    }
    li = glsgd.nlevels;
  } else {
    li = nearest_level(x1, y1);
    if (li < 0) {
      tell("  ... no level selected, try again...\n");
      goto START;
    } else {
      tell(" Level %s selected...\n", glsgd.lev[li].name);
    }
  }
  /* get final level for new gamma */
 GET_LF:
  if (li < glsgd.nlevels) {
    tell("...Select final level (N for new level, A to abort)...\n");
  } else {
    tell("...Select final level (A to abort)...\n");
  }
  retic2(&x1, &y1, ans);
  if (*ans == 'A' || *ans == 'a') goto START;
  if (*ans == 'N' || *ans == 'n') {
    if (li >= glsgd.nlevels) {
      warn("ERROR - cannot create two new levels at once.\n");
      goto START;
    }
    tell("...Select band for new final level\n"
	 "    (A to abort, N for new band)...\n");
  GET_BF:
    retic2(&x1, &y1, ans);
    if (*ans == 'A' || *ans == 'a') goto START;
    if (*ans == 'N' || *ans == 'n') {
      iband = glsgd.nbands;
      tell("   ... Give position for new band (A to abort)...\n");
      retic2(&x2, &y2, ans);
      if (*ans == 'A' || *ans == 'a') goto START;
      glsgd.bnd[iband].x0 = x2 - glsgd.bnd[iband].nx / 2.f;
    } else {
      iband = nearest_band(x1, y1);
    }
    if (iband < 0) {
      tell("  ... no band selected, try again...\n");
      goto GET_BF;
    }
    if (iband >= glsgd.nbands) {
      if (insert_band(&iband)) goto START;
    } else {
      tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    }
    lf = glsgd.nlevels;
  } else {
    lf = nearest_level(x1, y1);
    if (lf < 0) {
      tell("  ... no level selected, try again...\n");
      goto GET_LF;
    } else if (li == lf) {
      warn("ERROR - final level = initial level.\n");
      goto START;
    } else {
      tell(" Level %s selected...\n", glsgd.lev[lf].name);
    }
  }
  if (li < glsgd.nlevels && lf < glsgd.nlevels) {
    energy = glsgd.lev[li].e - glsgd.lev[lf].e;
    if (energy < 0.f) {
      warn("ERROR - initial level lower than final level.\n");
      goto START;
    }
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (glsgd.gam[j].li == li && glsgd.gam[j].lf == lf) {
	warn("ERROR - already have such a gamma.\n");
	goto START;
      }
    }
    ds = fabs(glsgd.lev[li].j - glsgd.lev[lf].j);
    ids = (int) (ds + 0.5f);
    if (fabs(ds - (float) ids) > .1f) {
      warn("ERROR - one level has half-int spin,\n"
	   "      other level has int spin.\n");
      goto START;
    }
    if (ids > 9) ids = 9;
    dp = glsgd.lev[li].pi * glsgd.lev[lf].pi;
    if (dp > 0.f) {
      if (ids % 2 == 0) ch = 'E';
      else ch = 'M';
    } else {
      if (ids % 2 == 0) ch = 'M';
      else ch = 'E';
    }
    if (ids == 0) {
      ids = 1;
      if (ch == 'M') ch = 'E';
      else ch = 'M';
    }
    tell("Gamma energy = %.1f, Multipolarity = %c%i\n", energy, ch, ids);
  } else {
    ch = ' ';
    if ((energy = get_energy()) <= 0.f) goto START;
  }
  save_gls_now(0);
  insert_gamma(&li, &lf, &energy, &ch, ids, iband);
  indices_gls();
  calc_band();
  path(1);
  calc_peaks();
  calc_gls_fig();
  display_gls(-1);
  goto START;
} /* add_gamma */

/* ======================================================================= */
int add_gls(void)
{
  float b1, b2, dx;
  int   duplicate_name, jext, i, j, k, old_ngammas, new_ngammas;
  int   old_nlevels, new_nlevels, old_nbands, new_nbands;
  int   old_ntlabels, new_ntlabels, rl, rlen;
#define BUFSIZE (24*MAXLEV + 61*MAXGAM + 16*MAXBAND + 60*MAXTXT)
  char  buf[BUFSIZE];
  char  filnam[80];
  FILE  *file;

  /* combine two .gls level scheme files */
  old_nlevels = glsgd.nlevels;
  old_ngammas = glsgd.ngammas;
  old_nbands = glsgd.nbands;
  old_ntlabels = glsgd.ntlabels;

  while (1) {
    /* ask for and open second level scheme file */
    if (!askfn(filnam, 80, "", ".gls", "Second GLS file = ?")) return 0;
    jext = setext(filnam, "", 80);
    if (!strcmp(filnam+jext, ".AGS") || !strcmp(filnam+jext, ".ags")) {
      warn("ASCII gls format not supported here,"
	   " use a standard .gls file.\n");
    } else if (!(file = open_readonly(filnam))) {
      file_error("open", filnam);
    } else {
      break;
    }
  }

  /* read second file */
#define RR { rlen = get_file_rec(file, buf, BUFSIZE, -1); rl = 0; }
#define R(a,b) { memcpy(a, buf + rl, b); rl += b; }
  RR;
  if (rlen != 13*4) goto ERR;
  rl = 4;
  R(&new_nlevels, 4);
  R(&new_ngammas, 4);
  R(&new_nbands, 4);
  if (new_nlevels + old_nlevels > MAXLEV) {
    warn("*** ERROR: too many levels; total = %i, max. =%i\n",
	 glsgd.nlevels, MAXLEV);
    fclose(file);
    return 1;
  }
  if (new_ngammas + old_ngammas > MAXGAM) {
    warn("*** ERROR: too many gammas; total = %i, max. =%i\n",
	 glsgd.ngammas, MAXGAM);
    fclose(file);
    return 1;
  }
  if (new_nbands + old_nbands > MAXBAND) {
    warn("*** ERROR: too many bands; total = %i, max. =%i\n",
	 glsgd.nbands, MAXBAND);
    fclose(file);
    return 1;
  }
  save_gls_now(0);
  glsgd.nlevels = new_nlevels + old_nlevels;
  glsgd.ngammas = new_ngammas + old_ngammas;
  glsgd.nbands = new_nbands + old_nbands;
  RR;
  for (j = old_nlevels; j < glsgd.nlevels; ++j) {
    R(&glsgd.lev[j].e, 4*5);
    R(&glsgd.lev[j].band, 4);
    glsgd.lev[j].band--;
  }
  for (j = old_ngammas; j < glsgd.ngammas; ++j) {
    R(&glsgd.gam[j].li, 4*2);
    R(&glsgd.gam[j].em, 1);
    R(&glsgd.gam[j].e, 4*13);
    glsgd.gam[j].li--;
    glsgd.gam[j].lf--;
  }
  for (j = old_nbands; j < glsgd.nbands; ++j) {
    R(glsgd.bnd[j].name, 8);
    R(&glsgd.bnd[j].x0, 4*2);
  }
  RR;
  for (j = old_nbands; j < glsgd.nbands; ++j) {
    R(&glsgd.bnd[j].sldx, 4);
  }
  for (j = old_ngammas; j < glsgd.ngammas; ++j) {
    R(&glsgd.gam[j].eldx, 4*2);
  }
  for (j = old_nlevels; j < glsgd.nlevels; ++j) {
    R(&glsgd.lev[j].sldx, 4);
  }
  RR;
  if (rlen <= 0) {
      /* no more data in file; must be an old-type gls file.
	 set the following items to default values */
    for (j = old_ngammas; j < glsgd.ngammas; ++j) {
      glsgd.gam[j].flg = glsgd.gam[j].elflg = 0;
    }
    for (j = old_nlevels; j < glsgd.nlevels; ++j) {
      glsgd.lev[j].flg = glsgd.lev[j].slflg = glsgd.lev[j].elflg = 0;
      glsgd.lev[j].dxl = glsgd.lev[j].dxr = glsgd.lev[j].sldy = 0.f;
      glsgd.lev[j].eldx = glsgd.lev[j].eldy = 0.f;
    }
    for (j = old_nbands; j < glsgd.nbands; ++j) {
      glsgd.bnd[j].sldy = glsgd.bnd[j].eldx = glsgd.bnd[j].eldy = 0.f;
    }
  } else {
    rl = 32;
    R(&new_ntlabels, 4);
    glsgd.ntlabels = new_ntlabels + old_ntlabels;
    if (glsgd.ntlabels > MAXTXT) glsgd.ntlabels = MAXTXT;
    RR;
    for (j = old_ngammas; j < glsgd.ngammas; ++j) {
      R(&glsgd.gam[j].flg, 4*2);
    }
    for (j = old_nlevels; j < glsgd.nlevels; ++j) {
      R(&glsgd.lev[j].flg, 4*5);
      R(&glsgd.lev[j].sldy, 4*3);
    }
    for (j = old_nbands; j < glsgd.nbands; ++j) {
      R(&glsgd.bnd[j].sldy, 4*3);
    }
    RR;
    for (j = old_ntlabels; j < glsgd.ntlabels; ++j) {
      R(glsgd.txt[j].l, 40);
      R(&glsgd.txt[j].nc, 4*5);
    }
  }
#undef R
#undef RR
  fclose(file);

  b1 = glsgd.bnd[old_nbands].x0;
  b2 = glsgd.bnd[0].x0 + glsgd.bnd[0].nx;
  for (i = 1; i < old_nbands; ++i) {
    if (b2 < glsgd.bnd[i].x0 + glsgd.bnd[i].nx)
      b2 = glsgd.bnd[i].x0 + glsgd.bnd[i].nx;
  }
  for (i = old_nbands + 1; i < glsgd.nbands; ++i) {
    if (b1 > glsgd.bnd[i].x0) b1 = glsgd.bnd[i].x0;
  }
  dx = b2 + glsgd.default_sep - b1;
  for (j = old_ngammas; j < glsgd.ngammas; ++j) {
    glsgd.gam[j].li += old_nlevels;
    glsgd.gam[j].lf += old_nlevels;
    if (glsgd.gam[j].x1 != 0.f) {
      glsgd.gam[j].x1 += dx;
      glsgd.gam[j].x2 += dx;
    }
  }
  for (j = old_nlevels; j < glsgd.nlevels; ++j) {
    glsgd.lev[j].band += old_nbands;
  }
  duplicate_name = 0;
  for (j = old_nbands; j < glsgd.nbands; ++j) {
    glsgd.bnd[j].x0 += dx;
    for (k = 0; k < old_nbands; ++k) {
      if (!strncmp(glsgd.bnd[j].name, glsgd.bnd[k].name, 8)) {
	duplicate_name = 1;
      }
    }
  }
  for (j = old_ntlabels; j < glsgd.ntlabels; ++j) {
    glsgd.txt[j].x += dx;
  }
  if (duplicate_name)
    warn("\n"
	 "**** WARNING -- Some bands now have identical names. ****\n"
	 "****         -- Be sure to delete or rename them.    ****\n\n");
  glsgd.x0  = glsgd.tx0;
  glsgd.hix = glsgd.thix;
  glsgd.y0  = glsgd.ty0;
  glsgd.hiy = glsgd.thiy;
  indices_gls();
  calc_band();
  calc_gls_fig();
  display_gls(1);
  path(1);
  calc_peaks();
  return 0;

 ERR:
  warn("Cannot read file %s\n"
       "   -- perhaps you need to run vms2unix, unix2unix\n"
       "      or unix2vms on the file.\n", filnam);
  fclose(file);
  return 1;
} /* add_gls */

/* ======================================================================= */
int add_level(void)
{
  float spin, x1, y1, x2, y2, energy, fj1, fj2;
  int   iband, nc, parity;
  char  ans[80];

 START:
  if (glsgd.nlevels >= MAXLEV) {
    warn("You now have the maximum of %d levels in the scheme!\n"
	 " Before you can add more, you need to change MAXLEV in\n"
	 " the file gls.h, and recompile radware.\n", MAXLEV);
    *ans = 'X';
  } else {    
    tell("...Select band for new level"
	 " (X or button 3 to exit, N for new band)...\n");
    retic2(&x1, &y1, ans);
  }
  if (*ans == 'X' || *ans == 'x') return 0;

  x2 = 0.f;
  if (*ans == 'N' || *ans == 'n') {
    iband = glsgd.nbands;
    tell("   ... Give position for new band (A to abort)...\n");
    retic2(&x2, &y2, ans);
    if (*ans == 'A' || *ans == 'a') goto START;
  } else {
    iband = nearest_band(x1, y1);
  }
  if (iband < 0) {
    tell("  ... no band selected, try again...\n");
    goto START;
  }
  if (iband < glsgd.nbands) {
    tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
  }
  while (1) {
    if (!(nc = ask(ans, 80, "Level energy = ? (rtn to quit)"))) goto START;
    if (!ffin(ans, nc, &energy, &fj1, &fj2)) break;
  }
  while (1) {
    nc = ask(ans, 80, " ...Spin = ?");
    if (!ffin(ans, nc, &spin, &fj1, &fj2)) break;
  }
  spin = ((float) ((int) (spin*2.0f + 0.5f))) * 0.5f;
  parity = -1;
  if (askyn(" ...Parity = even? (Y/N)")) parity = 1;
  save_gls_now(0);
  if (iband >= glsgd.nbands) {
    if (x2 != 0.f) {
      glsgd.bnd[iband].x0 = x2 - glsgd.bnd[iband].nx / 2.f;
    }
    if (insert_band(&iband)) goto START;
  }
  insert_level(&energy, &spin, &parity, iband);
  indices_gls();
  calc_band();
  calc_gls_fig();
  display_gls(-1);
  goto START;
} /* add_level */

/* ======================================================================= */
int add_text(void)
{
  float x1, y1, fj;
  int   nc, it, nsx, nsy;
  char  ans[80];

 START:
  if (glsgd.ntlabels >= MAXTXT) {
    warn("You now have maximum number of %d text labels.\n"
	 " Before you can add more, you need to change MAXTXT in\n"
	 " the file gls.h, and recompile radware.\n", MAXTXT);
    return 0;
  }
  it = glsgd.ntlabels;
  tell("...Position for new text label = ? (X or button 3 to exit)\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  if (!(nc = ask(ans, 40, "...Text for new label = ? (rtn to quit)")))
    goto START;
  save_gls_now(0);
  strncpy(glsgd.txt[it].l, ans, 40);
  glsgd.txt[it].nc = nc;
  glsgd.txt[it].x = x1;
  glsgd.txt[it].y = y1;
  while (1) {
    glsgd.txt[it].csx = glsgd.csx;
    glsgd.txt[it].csy = glsgd.csy;
    if ((nc = ask(ans, 80, "...Character size = ? (rtn for %.0f)",
		  glsgd.csx))) {
      if (ffin(ans, nc, &x1, &fj, &fj)) continue;
      if (x1 > 0.f) {
	glsgd.txt[it].csx = x1;
	glsgd.txt[it].csy = x1*CSY;
      }
    }
    break;
  }
  initg2(&nsx, &nsy);
  drawstring(glsgd.txt[it].l, glsgd.txt[it].nc, 'H',
	     glsgd.txt[it].x, glsgd.txt[it].y,
	     glsgd.txt[it].csx, glsgd.txt[it].csy);
  finig();
  glsgd.ntlabels++;
  goto START;
} /* add_text */

/* ======================================================================= */
int add_to_band(char *command)
{
  float spin, x1, y1, energy, fj1, fj2, e;
  int   ilev, j, iband, nc, parity;
  char  ans[80];

 START:
  tell("Select band for new gammas (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  save_gls_now(0);
  iband = nearest_band(x1, y1);
  if (iband < 0) {
    tell("  ... no band selected, try again...\n");
    goto START;
  } else if (iband >= glsgd.nbands) {
    while (1) {
      if (!(nc = ask(ans, 80, "New bandhead energy = ? (rtn to quit)")))
	goto START;
      if (!ffin(ans, nc, &energy, &fj1, &fj2)) break;
    }
    while (1) {
      nc = ask(ans, 80, " ...Spin = ?");
      if (!ffin(ans, nc, &spin, &fj1, &fj2)) break;
    }
    spin = ((float) ((int) (spin*2.0f + 0.5f))) * 0.5f;
    parity = -1;
    if (askyn(" ...Parity = even? (Y/N)")) parity = 1;
    if (insert_band(&iband)) goto START;
    if (insert_level(&energy, &spin, &parity, iband)) goto START;
    indices_gls();
    calc_band();
    calc_gls_fig();
    display_gls(-1);
  } else {
    tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
  }
  e = -1.0f;
  for (j = 0; j < glsgd.nlevels; ++j) {
    if (glsgd.lev[j].band == iband && glsgd.lev[j].e > e) {
      e = glsgd.lev[j].e;
      ilev = j;
    }
  }
  while ((energy = get_energy()) > 0.f) {
    j = glsgd.nlevels;
    if (!strcmp(command, "AB1") || !strcmp(command, "AB 1")) {
      if (insert_gamma(&j, &ilev, &energy, "M", 1, iband)) break;
    } else {
      if (insert_gamma(&j, &ilev, &energy, "E", 2, iband)) break;
    }
    ilev = glsgd.nlevels - 1;
  }
  indices_gls();
  calc_band();
  path(1);
  calc_peaks();
  return 0;
} /* add_to_band */

/* ======================================================================= */
int change_gamma_label(char *command)
{
  float f1, f2, f3, x1, y1;
  int   igam, i, nc, option, nsx, nsy;
  char  ans[80];
  static char *choices[7] =
    {"gamma label =  123",
     "gamma label = (123)",
     "no gamma label",
     "gamma label =  123.4",
     "gamma label = (123.4)",
     "toggle through gamma labels",
     "change size of gamma labels"};
  /* gam.elflg = 0 for 123
                 1 for (123)
		 2 for no label
		 3 for 123.4
		 4 for (123.4) */

  if (command[2] >= '0' && command[2] <= '6') {
    option = (int) (command[2] - '0');
  } else {
    option = ask_selection(7, -1, (const char **) choices, "12345ts",
			   "Please select a gamma label option:", "");
  }
  if (option < 0) return 0;
  if (option == 6) {
    while (1) {
      nc = ask(ans, 80,
	       "Default label character size is %.0f\n"
	       "   ...New size for gamma labels = ? (rtn for %.0f)\n"
	       "                                    (D for default)",
	       glsgd.csx, glsgd.gel_csx);
      if (nc == 0) return 0;
      save_gls_now(0);
      if (*ans == 'D' || *ans == 'd') {
	glsgd.gel_csx = glsgd.csx;
	glsgd.gel_csy = glsgd.csy;
      } else {
	if (ffin(ans, nc, &f1, &f2, &f3)) continue;
	if (f1 > 0.f) {
	  glsgd.gel_csx = f1;
	  glsgd.gel_csy = f1*CSY;
	}
      }
      break;
    }
    calc_gls_fig();
    display_gls(-1);
    return 0;
  }
  while (1) {
    tell("Select gamma to change (X or button 3 to exit)...\n");
    if (option <= 2) {
      tell("             (A for all gammas)...\n");
    }
    retic2(&x1, &y1, ans);
    if (*ans == 'X' || *ans == 'x') {
      display_gls(-1);
      return 0;
    } else if ((*ans == 'A' || *ans == 'a') 
	       && option <= 4) {
      save_gls_now(2);
      for (i = 0; i < glsgd.ngammas; ++i) {
	glsgd.gam[i].elflg = option;
      }
      calc_gls_fig();
      display_gls(-1);
      return 0;
    }
    igam = nearest_gamma(x1, y1);
    if (igam < 0) {
      tell("  ... no gamma selected, try again...\n");
    } else {
      tell(" Gamma %.1f  selected...\n", glsgd.gam[igam].e);
      save_gls_now(2);
      initg2(&nsx, &nsy);
      setcolor(0);
      drawstring(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H',
		 glsgd.gam[igam].elx, glsgd.gam[igam].ely,
		 glsgd.gel_csx, glsgd.gel_csy);
      if (option <= 4) {
	glsgd.gam[igam].elflg = option;
      } else {
	++glsgd.gam[igam].elflg;
	if (glsgd.gam[igam].elflg > 4) glsgd.gam[igam].elflg = 0;
      }
      calc_gam(igam);
      setcolor(1);
      drawstring(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H',
		 glsgd.gam[igam].elx, glsgd.gam[igam].ely,
		 glsgd.gel_csx, glsgd.gel_csy);
      finig();
    }
  }
} /* change_gamma_label */

/* ======================================================================= */
int change_level_energy_label(char *command)
{
  float f1, f2, f3, x1, y1;
  int   ilev, i, iband, nc, nx, ny, option;
  char  ans[80];
  static char *choices[7] =
    {"no energy label",
     "energy label =  123",
     "energy label = (123)",
     "energy label =  123.4",
     "energy label = (123.4)",
     "toggle through energy labels",
     "change size of energy labels"};
  /* leflg = 0 for no label
             1 for 123
             2 for (123)
             3 for 123.4
             4 for (123.4) */

  if (command[2] >= '0' && command[2] <= '6') {
    option = (int) (command[2] - '0');
  } else {
    option = ask_selection(7, -1, (const char **) choices, "12345ts",
			   "Please select a level energy label option:", "");
  }
  if (option < 0) return 0;
  if (option == 6) {
    while (1) {
      nc = ask(ans, 80,
	       "Default label character size is %.0f\n"
	       "   ...New size for level energy labels = ? (rtn for %.0f)\n"
	       "                                           (D for default)",
	       glsgd.csx, glsgd.lel_csx);
      if (nc == 0) return 0;
      save_gls_now(0);
      if (*ans == 'D' || *ans == 'd') {
	glsgd.lel_csx = glsgd.csx;
	glsgd.lel_csy = glsgd.csy;
      } else {
	if (ffin(ans, nc, &f1, &f2, &f3)) continue;
	if (f1 > 0.f) {
	  glsgd.lel_csx = f1;
	  glsgd.lel_csy = f1*CSY;
	}
      }
      break;
    }
    calc_gls_fig();
    display_gls(-1);
    return 0;
  }
  while (1) {
    tell("Select level or band (B) to change (X or button 3 to exit)...\n");
    if (option < 5)
      tell("                         (A for all levels)...\n");
    retic2(&x1, &y1, ans);
    if (*ans == 'X' || *ans == 'x') {
      display_gls(-1);
      return 0;
    } else if ((*ans == 'A' || *ans == 'a') && option < 5) {
      save_gls_now(1);
      for (i = 0; i < glsgd.nlevels; ++i) {
	glsgd.lev[i].elflg = option;
      }
      calc_gls_fig();
      display_gls(-1);
      return 0;
    } else if (*ans == 'B' || *ans == 'b') {
      iband = nearest_band(x1, y1);
      if (iband < 0 || iband >= glsgd.nbands) {
	tell("  ... no band selected, try again...\n");
	continue;
      }
      tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
      save_gls_now(1);
      initg2(&nx, &ny);
      for (i = 0; i < glsgd.nlevels; ++i) {
	if (glsgd.lev[i].band == iband) {
	  setcolor(0);
	  drawstring(glsgd.lev[i].el, glsgd.lev[i].elnc, 'H',
		     glsgd.lev[i].elx, glsgd.lev[i].ely,
		     glsgd.lel_csx, glsgd.lel_csy);
	  if (option < 5) {
	    glsgd.lev[i].elflg = option;
	  } else {
	    ++glsgd.lev[i].elflg;
	    if (glsgd.lev[i].elflg > 4) glsgd.lev[i].elflg = 0;
	  }
	  calc_lev(i);
	  setcolor(4);
	  drawstring(glsgd.lev[i].el, glsgd.lev[i].elnc, 'H',
		     glsgd.lev[i].elx, glsgd.lev[i].ely,
		     glsgd.lel_csx, glsgd.lel_csy);
	}
      }
      setcolor(1);
      finig();
    } else {
      ilev = nearest_level(x1, y1);
      if (ilev < 0) {
	tell("  ... no level selected, try again...\n");
	continue;
      }
      tell(" Level %s selected...\n", glsgd.lev[ilev].name);
      save_gls_now(1);
      initg2(&nx, &ny);
      setcolor(0);
      drawstring(glsgd.lev[ilev].el, glsgd.lev[ilev].elnc, 'H',
		 glsgd.lev[ilev].elx, glsgd.lev[ilev].ely,
		 glsgd.lel_csx, glsgd.lel_csy);
      if (option < 5) {
	glsgd.lev[ilev].elflg = option;
      } else {
	++glsgd.lev[ilev].elflg;
	if (glsgd.lev[ilev].elflg > 4) glsgd.lev[ilev].elflg = 0;
      }
      calc_lev(ilev);
      setcolor(4);
      drawstring(glsgd.lev[ilev].el, glsgd.lev[ilev].elnc, 'H',
		 glsgd.lev[ilev].elx, glsgd.lev[ilev].ely,
		 glsgd.lel_csx, glsgd.lel_csy);
      setcolor(1);
      finig();
    }
  }
} /* change_level_energy_label */

/* ======================================================================= */
int change_level_spin_label(char *command)
{
  float f1, f2, f3, x1, y1;
  int   ilev, i, iband, nc, nx, ny, option;
  char  ans[80];
  static char *choices[9] =
    {"spin label =  Jpi",
     "spin label = (J)pi",
     "spin label = J(pi)",
     "spin label = (Jpi)",
     "spin label =   J",
     "spin label =  (J)",
     "no spin label",
     "toggle through spin labels",
     "change size of spin labels"};
  /* slflg = 0 for spin label =  Jpi
             1 for spin label = (J)pi
             2 for spin label = J(pi)
             3 for spin label = (Jpi)
             4 for spin label =   J
             5 for spin label =  (J)
             6 for no spin label */

  if (command[2] >= '0' && command[2] <= '8') {
    option = (int) (command[2] - '0');
  } else {
    option = ask_selection(9, -1, (const char **) choices, "1234567ts",
			   "Please select a level spin label option:", "");
  }
  if (option < 0) return 0;
  if (option == 8) {
    while (1) {
      nc = ask(ans, 80,
	       "Default label character size is %.0f\n"
	       "   ...New size for level spin labels = ? (rtn for %.0f)\n"
	       "                                         (D for default)",
	       glsgd.csx, glsgd.lsl_csx);
      if (nc == 0) return 0;
      save_gls_now(0);
      if (*ans == 'D' || *ans == 'd') {
	glsgd.lsl_csx = glsgd.csx;
	glsgd.lsl_csy = glsgd.csy;
      } else {
	if (ffin(ans, nc, &f1, &f2, &f3)) continue;
	if (f1 > 0.f) {
	  glsgd.lsl_csx = f1;
	  glsgd.lsl_csy = f1*CSY;
	}
      }
      break;
    }
    calc_gls_fig();
    display_gls(-1);
    return 0;
  }
  while (1) {
    tell("Select level or band (B) to change (X or button 3 to exit)...\n");
    if (option < 7)
      tell("                         (A for all levels)...\n");
    retic2(&x1, &y1, ans);
    if (*ans == 'X' || *ans == 'x') {
      display_gls(-1);
      return 0;
    } else if ((*ans == 'A' || *ans == 'a') && option < 7) {
      save_gls_now(1);
      for (i = 0; i < glsgd.nlevels; ++i) {
	glsgd.lev[i].slflg = option;
      }
      calc_gls_fig();
      display_gls(-1);
      return 0;
    } else if (*ans == 'B' || *ans == 'b') {
      iband = nearest_band(x1, y1);
      if (iband < 0 || iband >= glsgd.nbands) {
	tell("  ... no band selected, try again...\n");
	continue;
      }
      tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
      save_gls_now(1);
      initg2(&nx, &ny);
      for (i = 0; i < glsgd.nlevels; ++i) {
	if (glsgd.lev[i].band == iband) {
	  setcolor(0);
	  drawstring(glsgd.lev[i].sl, glsgd.lev[i].slnc, 'H',
		     glsgd.lev[i].slx, glsgd.lev[i].sly,
		     glsgd.lsl_csx, glsgd.lsl_csy);
	  if (option < 7) {
	    glsgd.lev[i].slflg = option;
	  } else {
	    ++glsgd.lev[i].slflg;
	    if (glsgd.lev[i].slflg > 6) glsgd.lev[i].slflg = 0;
	  }
	  calc_lev(i);
	  setcolor(3);
	  drawstring(glsgd.lev[i].sl, glsgd.lev[i].slnc, 'H',
		     glsgd.lev[i].slx, glsgd.lev[i].sly,
		     glsgd.lsl_csx, glsgd.lsl_csy);
	}
      }
      setcolor(1);
      finig();
    } else {
      ilev = nearest_level(x1, y1);
      if (ilev < 0) {
	tell("  ... no level selected, try again...\n");
	continue;
      }
      tell(" Level %s selected...\n", glsgd.lev[ilev].name);
      save_gls_now(1);
      initg2(&nx, &ny);
      setcolor(0);
      drawstring(glsgd.lev[ilev].sl, glsgd.lev[ilev].slnc, 'H',
		 glsgd.lev[ilev].slx, glsgd.lev[ilev].sly,
		 glsgd.lsl_csx, glsgd.lsl_csy);
      if (option < 7) {
	glsgd.lev[ilev].slflg = option;
      } else {
	++glsgd.lev[ilev].slflg;
	if (glsgd.lev[ilev].slflg > 6) glsgd.lev[ilev].slflg = 0;
      }
      calc_lev(ilev);
      setcolor(3);
      drawstring(glsgd.lev[ilev].sl, glsgd.lev[ilev].slnc, 'H',
		 glsgd.lev[ilev].slx, glsgd.lev[ilev].sly,
		 glsgd.lsl_csx, glsgd.lsl_csy);
      setcolor(1);
      finig();
    }
  }
} /* change_level_spin_label */

/* ======================================================================= */
int delete_band(void)
{
  float x1, y1;
  int   iband;
  char  ans[80];

 START:
  if (glsgd.nbands == 1) {
    warn("Cannot remove more bands -\n"
	 "      must have at least one band in scheme.\n");
    display_gls(-1);
    return 0;
  }
  tell("Select band to delete (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  }
  iband = nearest_band(x1, y1);
  if (iband < 0 || iband >= glsgd.nbands) {
    tell("  ... no band selected, try again...\n");
  } else {
    tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    save_gls_now(7);
    remove_band(iband);
    indices_gls();
    path(1);
    calc_peaks();
  }
  goto START;
} /* delete_band */

/* ======================================================================= */
int delete_gamma(void)
{
  float x1, y1;
  int   igam;
  char  ans[80];

 START:
  tell("Select gamma to delete (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  }
  igam = nearest_gamma(x1, y1);
  if (igam < 0) {
    tell("  ... no gamma selected, try again...\n");
  } else {
    tell(" Gamma %.1f  selected...\n", glsgd.gam[igam].e);
    save_gls_now(2);
    remove_gamma(igam);
    indices_gls();
    path(1);
    calc_peaks();
  }
  goto START;
} /* delete_gamma */

/* ======================================================================= */
int delete_level(void)
{
  float x1, y1;
  int   del_band, ilev, j, jband;
  char  ans[80];

 START:
  if (glsgd.nlevels == 1) {
    warn("Cannot remove more levels - \n"
	 "      must have at least one level in scheme.\n");
    display_gls(-1);
    return 0;
  }
  tell("Select level to delete (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  }
  ilev = nearest_level(x1, y1);
  if (ilev < 0) {
    tell("  ... no level selected, try again...\n");
  } else {
    tell(" Level %s selected...\n", glsgd.lev[ilev].name);
    save_gls_now(7);
    del_band = 1;
    for (j = 0; j < glsgd.nlevels; ++j) {
      if (j != ilev && glsgd.lev[j].band == glsgd.lev[ilev].band) del_band = 0;
    }
    if (del_band) {
      jband = glsgd.lev[ilev].band;
      remove_band(jband);
    } else {
      remove_level(ilev);
      calc_gls_fig();
    }
    indices_gls();
    path(1);
    calc_peaks();
  }
  goto START;
} /* delete_level */

/* ======================================================================= */
int delete_text(void)
{
  float x1, y1;
  int   i, it, nsx, nsy;
  char  ans[80];

 START:
  if (glsgd.ntlabels == 0) {
    warn("No more text labels to delete.\n");
    return 0;
  }
  tell("...Select text label to delete (X or button 3 to exit)...\n");
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
    tell(" ...No text label selected, try again...\n");
    goto START;
  }
  tell("...Selected label %.40s\n", glsgd.txt[it].l);
  save_gls_now(8);
  initg2(&nsx, &nsy);
  setcolor(0);
  drawstring(glsgd.txt[it].l, glsgd.txt[it].nc, 'H',
	     glsgd.txt[it].x, glsgd.txt[it].y,
	     glsgd.txt[it].csx, glsgd.txt[it].csy);
  setcolor(1);
  finig();
  --glsgd.ntlabels;
  for (i = it; i < glsgd.ntlabels; ++i) {
    memcpy(glsgd.txt[i].l, glsgd.txt[i+1].l, sizeof(txtdat));
  }
  if (glsgd.ntlabels == 0) {
    warn("Have now deleted all text labels.\n");
    display_gls(-1);
    return 0;
  }
  goto START;
} /* delete_text */

/* ======================================================================= */
int display_gls(int iflag)
{
  float d, r1, r, x1, x2, y1, y2, dd, x02, y02;
  float xx[6], yy[6], dx1, dx2, dy2, dy1, ddx, ddy;
  int   igam, ilev, i, j, n, iband, iyaxis, nsx, nsy;
  char  cbuf[8];

  /* subroutine to display level scheme on graphics window */
  initg2(&nsx, &nsy);
  erase();
  limg(nsx, 0, nsy, 0);
  if (iflag == 1) {
    glsgd.dx = glsgd.thix - glsgd.tx0;
    glsgd.dy = glsgd.thiy - glsgd.ty0;
    glsgd.x00 = glsgd.tx0;
    glsgd.y00 = glsgd.ty0;
  } else if (iflag == 0) {
    glsgd.dx = glsgd.hix - glsgd.x0;
    glsgd.dy = glsgd.hiy - glsgd.y0;
    glsgd.x00 = glsgd.x0;
    glsgd.y00 = glsgd.y0;
  }
  ddx = glsgd.dx / (float) nsx;
  ddy = glsgd.dy / (float) nsy;
  if (ddy > ddx) {
    dx2 = ddy * (float) nsx;
    x02 = glsgd.x00 - (dx2 - glsgd.dx) / 2.f;
    dy2 = glsgd.dy;
    y02 = glsgd.y00;
  } else {
    dy2 = ddx * (float) nsy;
    y02 = glsgd.y00 - (dy2 - glsgd.dy) / 2.f;
    dx2 = glsgd.dx;
    x02 = glsgd.x00;
  }
  iyaxis = 0;
  trax(dx2, x02, dy2, y02, iyaxis);
  /* draw levels */
  setcolor(13);
  for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
    pspot(glsgd.lev[ilev].x[2], glsgd.lev[ilev].e);
    vect( glsgd.lev[ilev].x[3], glsgd.lev[ilev].e);
  }
  setcolor(1);
  for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
    pspot(glsgd.lev[ilev].x[0], glsgd.lev[ilev].e);
    vect( glsgd.lev[ilev].x[1], glsgd.lev[ilev].e);
    if (glsgd.lev[ilev].flg == 2) {
      /* thick level */
      pspot(glsgd.lev[ilev].x[0], glsgd.lev[ilev].e + glsgd.lsl_csy * .15f);
      vect (glsgd.lev[ilev].x[1], glsgd.lev[ilev].e + glsgd.lsl_csy * .15f);
      vect (glsgd.lev[ilev].x[1], glsgd.lev[ilev].e - glsgd.lsl_csy * .15f);
      vect (glsgd.lev[ilev].x[0], glsgd.lev[ilev].e - glsgd.lsl_csy * .15f);
      vect (glsgd.lev[ilev].x[0], glsgd.lev[ilev].e + glsgd.lsl_csy * .15f);
    }
  }
  setcolor(0);
  for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
    if (glsgd.lev[ilev].flg == 1) {
      /* tentative level */
      n = (glsgd.lev[ilev].x[1] - glsgd.lev[ilev].x[0] -
	   glsgd.level_break * 2.f) / (glsgd.level_break * 3.f);
      if (n < 1) n = 1;
      d = (glsgd.lev[ilev].x[1] - glsgd.lev[ilev].x[0]) / (n * 3 + 2);
      for (i = 1; i <= n; ++i) {
	x1 = glsgd.lev[ilev].x[0] + d * (float) (i*3);
	pspot(x1 - d, glsgd.lev[ilev].e);
	vect (x1,     glsgd.lev[ilev].e);
      }
    }
  }
  /* draw gammas */
  setcolor(2);
  for (igam = 0; igam < glsgd.ngammas; ++igam) {
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
      n = (int) (dd / (glsgd.arrow_break * 3.f) * 0.5f);
      if (n < 1) n = 1;
      d = dd / (float) (n * 3);
      dx2 = d / dd * dx1;
      dy2 = d / dd * dy1;
      if (glsgd.gam[igam].np <= 6) {
	pspot(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	for (i = 0; i < n; ++i) {
	  vect (glsgd.gam[igam].x[0] - (float) (i * 3 + 1) * dx2,
		glsgd.gam[igam].y[0] - (float) (i * 3 + 1) * dy2);
	  pspot(glsgd.gam[igam].x[0] - (float) (i * 3 + 2) * dx2,
		glsgd.gam[igam].y[0] - (float) (i * 3 + 2) * dy2);
	}
	for (i = 1; i < glsgd.gam[igam].np; ++i) {
	  vect(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
      } else if (glsgd.gam[igam].np <= 8) {
	xx[0] = glsgd.gam[igam].x[1] - dx2;
	yy[0] = glsgd.gam[igam].y[1] - dy2;
	xx[1] = glsgd.gam[igam].x[5] - dx2;
	yy[1] = glsgd.gam[igam].y[5] - dy2;
	xx[2] = glsgd.gam[igam].x[5] + dx2;
	yy[2] = glsgd.gam[igam].y[5] + dy2;
	xx[3] = glsgd.gam[igam].x[1] + dx2;
	yy[3] = glsgd.gam[igam].y[1] + dy2;
	pspot(xx[3], yy[3]);
	for (i = 1; i < 6; ++i) {
	  vect(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	vect(xx[2], yy[2]);
	vect(xx[3], yy[3]);
	for (i = 1; i < n; ++i) {
	  pspot(xx[3] + dx2 * 3.f, yy[3] + dy2 * 3.f);
	  for (j = 0; j < 4; ++j) {
	    xx[j] += dx2 * 3.f;
	    yy[j] += dy2 * 3.f;
	    vect(xx[j], yy[j]);
	  }
	}
	pspot(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	vect(xx[0] + dx2 * 3.f, yy[0] + dy2 * 3.f);
	vect(xx[1] + dx2 * 3.f, yy[1] + dy2 * 3.f);
	vect(glsgd.gam[igam].x[6], glsgd.gam[igam].y[6]);
	vect(glsgd.gam[igam].x[7], glsgd.gam[igam].y[7]);
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
	pspot(xx[3], yy[3]);
	for (i = 1; i < 6; ++i) {
	  vect(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
	}
	for (i = 2; i < 5; ++i) {
	  vect(xx[i], yy[i]);
	}
	vect(glsgd.gam[igam].x[9], glsgd.gam[igam].y[9]);
	for (i = 1; i < n; ++i) {
	  pspot(xx[3] + dx2 * 3.f, yy[3] + dy2 * 3.f);
	  for (j = 0; j < 6; ++j) {
	    xx[j] += dx2 * 3.f;
	    yy[j] += dy2 * 3.f;
	    vect(xx[j], yy[j]);
	  }
	}
	pspot(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
	vect(xx[0] + dx2 * 3.f, yy[0] + dy2 * 3.f);
	vect(xx[1] + dx2 * 3.f, yy[1] + dy2 * 3.f);
	vect(glsgd.gam[igam].x[6], glsgd.gam[igam].y[6]);
	vect(glsgd.gam[igam].x[7], glsgd.gam[igam].y[7]);
	vect(glsgd.gam[igam].x[8], glsgd.gam[igam].y[8]);
	vect(xx[5] + dx2 * 3.f, yy[5] + dy2 * 3.f);
      }
    } else {
      pspot(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
      for (i = 1; i < glsgd.gam[igam].np; ++i) {
	vect(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
      }
    }
  }
  /* white space for gamma energy labels */
  setcolor(0);
  for (igam = 0; igam < glsgd.ngammas; ++igam) {
    if (glsgd.gam[igam].elnc > 0) {
      x1 = glsgd.gam[igam].elx -
	(float) glsgd.gam[igam].elnc * .5f * glsgd.gel_csx - glsgd.gel_csy * .2f;
      x2 = glsgd.gam[igam].elx +
	(float) glsgd.gam[igam].elnc * .5f * glsgd.gel_csx + glsgd.gel_csy * .2f;
      y1 = glsgd.gam[igam].ely - glsgd.gel_csy * .2f;
      y2 = glsgd.gam[igam].ely + glsgd.gel_csy * 1.2f;
      pspot(x1, y1);
      vect(x1, y2);
      vect(x2, y2);
      vect(x2, y1);
      minig_fill();
    }
  }
  /* gamma energy labels */
  setcolor(1);
  for (igam = 0; igam < glsgd.ngammas; ++igam) {
    if (glsgd.gam[igam].elnc > 0) {
      drawstring(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H',
		 glsgd.gam[igam].elx, glsgd.gam[igam].ely,
		 glsgd.gel_csx, glsgd.gel_csy);
    }
  }
  /* level spin and energy labels */
  setcolor(3);
  for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
    if (glsgd.lev[ilev].slnc > 0) {
      drawstring(glsgd.lev[ilev].sl, glsgd.lev[ilev].slnc, 'H',
		 glsgd.lev[ilev].slx, glsgd.lev[ilev].sly,
		 glsgd.lsl_csx, glsgd.lsl_csy);
    }
  }
  setcolor(4);
  for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
    if (glsgd.lev[ilev].elnc > 0) {
      drawstring(glsgd.lev[ilev].el, glsgd.lev[ilev].elnc, 'H',
		 glsgd.lev[ilev].elx, glsgd.lev[ilev].ely,
		 glsgd.lel_csx, glsgd.lel_csy);
    }
  }
  /* general purpose text labels */
  setcolor(1);
  for (i = 0; i < glsgd.ntlabels; ++i) {
    drawstring(glsgd.txt[i].l, glsgd.txt[i].nc, 'H',
	       glsgd.txt[i].x, glsgd.txt[i].y,
	       glsgd.txt[i].csx, glsgd.txt[i].csy);
  }
  /* band labels */
  for (iband = 0; iband < glsgd.nbands + 2; ++iband) {
    strncpy(cbuf, glsgd.bnd[iband].name, 8);
    if (iband >= glsgd.nbands) strncpy(cbuf, "NEW BAND", 8);
    r1 = glsgd.bnd[iband].x0 + glsgd.bnd[iband].nx / 2.f;
    drawstring(cbuf, 8, 'H', r1, -200.0f, glsgd.csx,  glsgd.csy);
  }
  finig();
  hilite(-2);
  return 0;
} /* display_gls */

/* ======================================================================= */
int gls_exec2(char *ans)
{
  float x1, y1;

  /* decode and execute the gls level scheme commands */
  if (!strncmp(ans, "AB", 2)) {
    add_to_band(ans);
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "AD", 2)) {
    add_gls();
  } else if (!strncmp(ans, "AG", 2)) {
    add_gamma();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "AL", 2)) {
    add_level();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "AT", 2)) {
    add_text();
  } else if (!strncmp(ans, "CE", 2)) {
    change_level_energy_label(ans);
  } else if (!strncmp(ans, "CG", 2)) {
    change_gamma_label(ans);
  } else if (!strncmp(ans, "CL", 2)) {
    change_level_spin_label(ans);
  } else if (!strncmp(ans, "DB", 2)) {
    hilite(-1);
    delete_band();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "DG", 2)) {
    hilite(-1);
    delete_gamma();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "DL", 2)) {
    hilite(-1);
    delete_level();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "DT", 2)) {
    delete_text();
  } else if (!strncmp(ans, "EB", 2)) {
    edit_band();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "EG", 2)) {
    edit_gamma();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "EL", 2)) {
    edit_level();
    path(1);
    calc_peaks();
  } else if (!strncmp(ans, "ET", 2)) {
    edit_text();
  } else if (!strncmp(ans, "FL", 2)) {
    fit_lvls();
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FP", 2)) {
    figure_pars();
    calc_band();
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "GB", 2)) {
    move_band_gamma();
  } else if (!strncmp(ans, "HC", 2)) {
    gls_ps();
  } else if (!strncmp(ans, "LB", 2)) {
    move_band_spin_label();
  } else if (!strncmp(ans, "LE", 2)) {
    move_energy_label();
  } else if (!strncmp(ans, "LG", 2)) {
    move_gamma_label();
  } else if (!strncmp(ans, "LL", 2)) {
    move_level_spin_label();
  } else if (!strncmp(ans, "LW", 2)) {
    level_width(ans);
  } else if (!strncmp(ans, "MB", 2)) {
    move_band();
  } else if (!strncmp(ans, "MG", 2)) {
    move_gamma();
  } else if (!strncmp(ans, "ML", 2)) {
    move_level();
  } else if (!strncmp(ans, "MM", 2)) {
    move_many_bands();
  } else if (!strncmp(ans, "MT", 2)) {
    move_text();
  } else if (!strncmp(ans, "MO", 2)) {
    retic2(&x1, &y1, ans);
    glsgd.hix = glsgd.hix - glsgd.x0 + x1;
    glsgd.hiy = glsgd.hiy - glsgd.y0 + y1;
    glsgd.x0 = x1;
    glsgd.y0 = y1;
    display_gls(0);
  } else if (!strncmp(ans, "OC", 2)) {
    open_close_gap();
    /* RD, RD1 */
  } else if (!strncmp(ans, "RD", 2)) {
    calc_gls_fig();
    if (ans[2] == '1') {
      calc_band();
      display_gls(1);
    } else {
      display_gls(0);
    }
  } else if (!strncmp(ans, "RE", 2)) {
    undo_gls(1);  /* redo last undone gls edit */
    path(1);
  } else if (!strncmp(ans, "RG", 2)) {
    read_write_gls("OPEN_READ");
    indices_gls();
    calc_band();
    glsgd.x0 = glsgd.tx0;
    glsgd.hix = glsgd.thix;
    glsgd.y0 = glsgd.ty0;
    glsgd.hiy = glsgd.thiy;
    calc_gls_fig();
    display_gls(1);
    path(1);
    calc_peaks();
    save_gls_now(-1);
  } else if (!strncmp(ans, "SG", 2)) {
    swap_gamma();
    path(1);
  } else if (!strncmp(ans, "WG", 2)) {
    if (!read_write_gls("WRITE")) undo_gls_mark();
  } else if (!strncmp(ans, "SL", 2)) {
    if (!read_write_gls("SAVE")) undo_gls_mark();
  } else if (!strncmp(ans, "TG", 2)) {
    tentative_gamma();
  } else if (!strncmp(ans, "TL", 2)) {
    tentative_level();
  } else if (!strncmp(ans, "TS", 2)) {
    testsums();
  } else if (!strncmp(ans, "UD", 2)) {
    up_down_levels();
    path(1);
  } else if (!strncmp(ans, "UE", 2)) {
    undo_gls(-1);  /* undo previous gls edit */
    path(1);
  } else if (!strncmp(ans, "XG", 2)) {
    examine_gamma();
  } else {
    return 1;    /* command not recognized */
  }
  return 0;
} /* gls_exec2 */

/* ======================================================================= */
int gls_help(void)
{

  warn(
     "\n"
     "  GLS commands:\n"
     "AB       add E2 gammas to top of band in level scheme\n"
     "AB1      add M1 gammas to top of band in level scheme\n"
     "AD       add (combine) a second .gls level scheme file\n"
     "AG       add gamma to level scheme\n"
     "AL       add level to level scheme\n"
     "AT       add general-pupose text labels\n"
     "CE       change energy labels of levels\n"
     "CG       change labels of gammas\n"
     "CL       change spin labels of levels\n"
     "DB       delete band from level scheme\n"
     "DG       delete gamma from level scheme\n"
     "DL       delete level from level scheme\n"
     "DT       delete general-pupose text labels\n"
     "EB       edit parameters of band(s) in level scheme\n"
     "EG       edit parameters of gamma(s) in level scheme\n"
     "EL       edit parameters of level(s) in level scheme\n"
     "ET       edit general-pupose text labels\n");
  warn(
     "EX       expand level scheme display using cursor\n"
     "FL       fit energies of level scheme levels\n"
     "FP       modify default figure parameters\n"
     "GB       move in-band gammas of bands using cursor\n"
     "HC       generate postscript level scheme figure\n"
     "LB       move spin labels of all levels in bands using cursor\n"
     "LG       move labels of gammas\n"
     "LE       move energy labels of levels using cursor\n"
     "LL       move spin labels of individual levels\n"
     "LW       change widths of levels using cursor\n"
     "MB       move band in level scheme\n"
     "MG       move gamma in level scheme\n"
     "ML       move level between bands\n"
     "MM       move multiple bands in level scheme\n"
     "MO       move level scheme display using cursor\n"
     "MT       move general-pupose text labels\n"
     "OC       open/close gap in level scheme using cursor\n");
  warn(
     "RD       redisplay level scheme\n"
     "RD1      redisplay entire level scheme\n"
     "RG       read new .gls level scheme file\n"
     "WG       write level scheme to new .gls file\n"
     "SG       swap ordering of gammas feeding/deexciting levels\n"
     "SL       save level scheme in current file\n"
     "TG       toggle tentative/normal gammas\n"
     "TL       toggle tentative/thick/normal levels\n"
     "TS       test level scheme gamma-ray energy and intensity sums\n"
     "UD       move a bunch of connected levels Up or Down in energy\n"
     "UE       undo the most recent edits/changes to the level scheme\n"
     "RE       redo the most recently undone edits to the level scheme\n"
     "XG       examine gammas in level scheme\n"
     "ST       stop and exit program\n");
  return 0;
} /* gls_help */

/* ======================================================================= */
int gls_init(char *infilnam)
{
  int gls_get_newgls_details(int *z, float *spin, int *parity, char*name);

  float spin;
  int   i, nc, nx, ny, parity;
  char  name[8];

  set_xwg("gls_std_");
  initg2(&nx, &ny);
  erase();
  finig();

  if ((nc = strlen(infilnam)) > 0) {
    strcpy(glsgd.gls_file_name, infilnam);
  } else {
    /* ask for and open level scheme file */
  GETFN:
    nc = askfn(glsgd.gls_file_name, 80, "", ".gls",
	       " GLS level scheme file = ?\n"
	       " (rtn to create a new level scheme)");
  }

  if (nc == 0) {
    /* initialise some default values
       DATA     CSX /75.0/, CSY/85.0/, ASPECT_RATIO /3.0/
       DATA     G_CSX, L_CSX, L_E_CSX /3*75.0/
       DATA     G_CSY, L_CSY, L_E_CSY /3*85.0/
       DATA     MAX_TANGENT /0.2679/, MAX_DIST /999.0/
       DATA     ARROW_WIDTH /40.0/, ARROW_LENGTH /80.0/
       DATA     DEFAULT_WIDTH /600.0/, DEFAULT_SEP /150.0/
       DATA     ARROW_BREAK /30.0/, LEVEL_BREAK /40.0/
    */
    glsgd.csx = glsgd.lsl_csx = glsgd.lel_csx = glsgd.gel_csx = 75.0f;
    glsgd.csy = glsgd.lsl_csy = glsgd.lel_csy = glsgd.gel_csy = 85.0f;
    glsgd.aspect_ratio = 3.0f;
    glsgd.max_tangent = 0.2679f;
    glsgd.max_dist = 999.0f;
    glsgd.arrow_width = 40.0f;
    glsgd.arrow_length = 80.0f;
    glsgd.default_width = 600.0f;
    glsgd.default_sep = 150.0f;
    glsgd.arrow_break = 30.0f;
    glsgd.level_break = 40.0f;

    while (gls_get_newgls_details(&glsgd.atomic_no, &spin, &parity, name));

    for (i = 0; i < MAXGAM; ++i) {
      memset(&glsgd.gam[i].li, 0, sizeof(gamdat));
      glsgd.gam[i].i = 10.f;
    }
    for (i = 0; i < MAXLEV; ++i) {
      memset(&glsgd.lev[i].e, 0, sizeof(levdat));
     glsgd.lev[i].pi = 1.f;
    }
    for (i = 0; i < MAXBAND; ++i) {
      glsgd.bnd[i].nx = glsgd.default_width;
    }
    glsgd.ngammas = 0;
    glsgd.nlevels = 1;
    glsgd.lev[0].e = 0.f;
    glsgd.lev[0].j = spin;
    glsgd.lev[0].pi = (float) parity;
    glsgd.lev[0].band = 0;
    glsgd.nbands = 1;
    glsgd.bnd[0].x0 = glsgd.default_width * 50.f;
    strncpy(glsgd.bnd[0].name, name, 8);
    /* if (askyn("Save this as a gls level scheme file? (Y/N)"))
       read_write_gls("WRITE"); */
    save_gls_now(-1);
    glsundo.mark = -2;
    undo_gls_notify(-3);
  } else {
    /* read level scheme file */
    setext(glsgd.gls_file_name, ".gls", 80);
    if (read_write_gls("READ")) goto GETFN;
  }
  indices_gls();
  calc_band();
  glsgd.x0 = glsgd.tx0;
  glsgd.hix = glsgd.thix;
  glsgd.y0 = glsgd.ty0;
  glsgd.hiy = glsgd.thiy;
  calc_gls_fig();
  display_gls(1);
  tell("*** Hint: To pan and zoom the level scheme when in cursor mode,\n"
       "     try holding down the shift keys and clicking the mouse buttons.\n"
       "     The left button will pan, depending on the pointer position,\n"
       "     and the middle and right buttons will zoom in and out.\n");
  return 0;
} /* gls_init */

/* ======================================================================= */
int hilitemod(int n, int igam)
{
  static int igam_save[MAXGAM], nsave = 0;
  float x1, x2, y1, y2;
  int   j, k, iy, ix1, iy1, ix2, iy2, nsx, nsy;

  if (n > 0) goto REMOVE_HILITE;

  if (igam >= 0) {
    for (j = 0; j < nsave; ++j) {
      if (igam == igam_save[j]) return 0;
    }
  } else if (nsave == 0) {
    return 0;
  }
  initg2(&nsx, &nsy);
  if (igam < 0) {
    setcolor(-1);
    for (j = 0; j < nsave; ++j) {
      k  = igam_save[j];
      x1 = glsgd.gam[k].elx -
	(float) (glsgd.gam[k].elnc + 1) * glsgd.gel_csx / 2.f;
      x2 = glsgd.gam[k].elx +
	(float) (glsgd.gam[k].elnc + 1) * glsgd.gel_csx / 2.f;
      y1 = glsgd.gam[k].ely - glsgd.gel_csy * 0.5f;
      y2 = glsgd.gam[k].ely + glsgd.gel_csy * 1.5f;
      cvxy(&x1, &y1, &ix1, &iy1, 1);
      cvxy(&x2, &y2, &ix2, &iy2, 1);
      for (iy = iy1; iy <= iy2; ++iy) {
	mspot(ix1, iy);
	ivect(ix2, iy);
      }
    }
    setcolor(-1);
    if (igam == -1) {
      setcolor(1);
    } else {
      setcolor(0);
    }
    for (j = 0; j < nsave; ++j) {
      k = igam_save[j];
      drawstring(glsgd.gam[k].el, glsgd.gam[k].elnc, 'H',
		 glsgd.gam[k].elx, glsgd.gam[k].ely,
		 glsgd.gel_csx, glsgd.gel_csy);
    }
    if (igam == -1) nsave = 0;
  } else {
    setcolor(-1);
    igam_save[nsave++] = igam;
    k = igam;
    x1 = glsgd.gam[k].elx -
      (float) (glsgd.gam[k].elnc + 1) * glsgd.gel_csx / 2.f;
    x2 = glsgd.gam[k].elx +
      (float) (glsgd.gam[k].elnc + 1) * glsgd.gel_csx / 2.f;
    y1 = glsgd.gam[k].ely - glsgd.gel_csy * 0.5f;
    y2 = glsgd.gam[k].ely + glsgd.gel_csy * 1.5f;
    cvxy(&x1, &y1, &ix1, &iy1, 1);
    cvxy(&x2, &y2, &ix2, &iy2, 1);
    for (iy = iy1; iy <= iy2; ++iy) {
      mspot(ix1, iy);
      ivect(ix2, iy);
    }
    setcolor(-1);
    setcolor(0);
    drawstring(glsgd.gam[k].el, glsgd.gam[k].elnc, 'H',
	       glsgd.gam[k].elx, glsgd.gam[k].ely,
	       glsgd.gel_csx,  glsgd.gel_csy);
  }
  setcolor(1);
  finig();
  return 0;

 REMOVE_HILITE:
  if (igam < 0) return 0;
  for (j = 0; j < nsave; ++j) {
    if (igam == igam_save[j]) break;
  }
  if (j >= nsave) return 0;
  nsave--;
  for (k = j; k < nsave; ++k) {
    igam_save[k] = igam_save[k+1];
  }
  initg2(&nsx, &nsy);
  setcolor(-1);
  k = igam;
  x1 = glsgd.gam[k].elx -
    (float) (glsgd.gam[k].elnc + 1) * glsgd.gel_csx / 2.f;
  x2 = glsgd.gam[k].elx +
    (float) (glsgd.gam[k].elnc + 1) * glsgd.gel_csx / 2.f;
  y1 = glsgd.gam[k].ely - glsgd.gel_csy * 0.5f;
  y2 = glsgd.gam[k].ely + glsgd.gel_csy * 1.5f;
  cvxy(&x1, &y1, &ix1, &iy1, 1);
  cvxy(&x2, &y2, &ix2, &iy2, 1);
  for (iy = iy1; iy <= iy2; ++iy) {
    mspot(ix1, iy);
    ivect(ix2, iy);
  }
  setcolor(-1);
  setcolor(1);
  drawstring(glsgd.gam[k].el, glsgd.gam[k].elnc, 'H',
	     glsgd.gam[k].elx, glsgd.gam[k].ely,
	     glsgd.gel_csx, glsgd.gel_csy);
  setcolor(1);
  finig();
  return 0;
} /* hilite */

int hilite(int igam)
{
  return hilitemod(0, igam);
}

int remove_hilite(int igam)
{
  return hilitemod(1, igam);
}

/* ======================================================================= */
int insert_band(int *iband)
{
  int  ib, i, j, nc;
  char name[8], ans[80];

  if (glsgd.nbands >= MAXBAND) {
    warn("You now have the maximum of %d bands in the scheme!\n"
	 " Before you can add more, you need to change MAXBAND in\n"
	 " the file gls.h, and recompile radware.\n", MAXBAND);
    return 1;
  }
  while (1) {
    if ((nc = ask(ans, 80, "Name for new band = ? (1-8 characters)")) < 1 ||
	nc > 8) continue;
    strncpy(name, "         ", 8);
    strncpy(name + 8 - nc, ans, nc);
    j = 0;
    for (i = 0; i < glsgd.nbands; ++i) {
      if (!strncmp(glsgd.bnd[i].name, name, 8)) j = 1;
    }
    if (!j) break;
    warn("ERROR - band name already in use...\n");
  }
  ib = glsgd.nbands++;
  strncpy(glsgd.bnd[ib].name, name, 8);
  glsgd.bnd[ib].x0 = glsgd.bnd[*iband].x0;
  glsgd.bnd[ib].nx = glsgd.default_width;
  glsgd.bnd[ib].sldx = 0.f;
  glsgd.bnd[ib].sldy = 0.f;
  glsgd.bnd[ib].eldx = 0.f;
  glsgd.bnd[ib].eldy = 0.f;
  *iband = ib;
  return 0;
} /* insert_band */

/* ======================================================================= */
int insert_gamma(int *li, int *lf, float *energy, char *ch, int ids, int iband)
{
  float spin, el;
  int   i, ig, j1, j2, ii, parity, nsx, nsy;
  char  ans[80], c;

  if (glsgd.ngammas >= MAXGAM) {
    warn("You now have the maximum of %d gammas in the scheme!\n"
	 " Before you can add more, you need to change MAXGAM in\n"
	 " the file gls.h, and recompile radware.\n", MAXGAM);
    return 1;
  }
  c = *ch;
  ii = ids;
  while ((*li >= glsgd.nlevels || *lf >= glsgd.nlevels) && *ch == ' ') {
    if (ask(ans, 80, " Gamma multipolarity = ?") != 2) continue;
    c = *ans;
    if (c == 'e') c = 'E';
    if (c == 'm') c = 'M';
    if (c != 'E' && c != 'M') continue;
    if (!inin(ans+1, 1, &ii, &j1, &j2) && ii > 0 && ii <= 9) break;
    warn("ERROR - illegal multipolarity:\n"
	 "    Number must be between 1 and 9.\n");
  }
  if (ii > 4) {
    warn("*************************************************\n"
	 "WARNING - gammas with multipolarity higher than 4\n"
	 "      do not get correct conversion coefficients.\n"
	   "*************************************************\n");
  }
  if (*li >= glsgd.nlevels) {
    el = glsgd.lev[*lf].e + *energy;
    spin = glsgd.lev[*lf].j + (float) ii;
    parity = rint(glsgd.lev[*lf].pi);
    if (ii%2) parity = - parity;
    if (c == 'M') parity = -parity;
    if (insert_level(&el, &spin, &parity, iband)) return 1;
  } else if (*lf >= glsgd.nlevels) {
    if ((el = glsgd.lev[*li].e - *energy) < 0.f) {
      warn("ERROR - energy of final level < 0\n");
      return 1;
    }
    spin = glsgd.lev[*li].j - (float) ii;
    parity = rint(glsgd.lev[*li].pi);
    if (ii%2) parity = - parity;
    if (c == 'M') parity = -parity;
    if (insert_level(&el, &spin, &parity, iband)) return 1;
  }
  ig = glsgd.ngammas++;
  glsgd.gam[ig].e = *energy;
  glsgd.gam[ig].de = 1.f;
  glsgd.gam[ig].i = 10.f;
  glsgd.gam[ig].di = 1.f;
  glsgd.gam[ig].a = 0.f;
  glsgd.gam[ig].d = 0.f;
  glsgd.gam[ig].li = *li;
  glsgd.gam[ig].lf = *lf;
  glsgd.gam[ig].em = c;
  glsgd.gam[ig].n = (float) ii;
  glsgd.gam[ig].eldx = 0.f;
  glsgd.gam[ig].eldy = 0.f;
  glsgd.gam[ig].flg = 0;
  glsgd.gam[ig].elflg = 0;
  calc_alpha(ig);
  glsgd.gam[ig].x1 = 0.f;
  calc_gam(ig);
  initg2(&nsx, &nsy);
  setcolor(2);
  pspot(glsgd.gam[ig].x[0], glsgd.gam[ig].y[0]);
  for (i = 1; i < glsgd.gam[ig].np; ++i) {
    vect(glsgd.gam[ig].x[i], glsgd.gam[ig].y[i]);
  }
  setcolor(1);
  drawstring(glsgd.gam[ig].el, glsgd.gam[ig].elnc, 'H',
	     glsgd.gam[ig].elx, glsgd.gam[ig].ely,
	     glsgd.gel_csx, glsgd.gel_csy);
  finig();
  return 0;
} /* insert_gamma */

/* ======================================================================= */
int insert_level(float *energy, float *spin, int *parity, int iband)
{
  int i, il, nsx, nsy;

  if (glsgd.nlevels >= MAXLEV) {
    warn("You now have the maximum of %d levels in the scheme!\n"
	 " Before you can add more, you need to change MAXLEV in\n"
	 " the file gls.h, and recompile radware.\n", MAXLEV);
    return 1;
  }
  il = glsgd.nlevels++;
  glsgd.lev[il].e = *energy;
  glsgd.lev[il].j = *spin;
  glsgd.lev[il].pi = (float) (*parity);
  strncpy(glsgd.lev[il].name, glsgd.bnd[iband].name, 8);
  sprintf(glsgd.lev[il].name + 8, "%.2i", (int) (*spin + 0.1f));
  glsgd.lev[il].band = iband;
  glsgd.lev[il].dxl = 0.f;
  glsgd.lev[il].dxr = 0.f;
  glsgd.lev[il].sldx = 0.f;
  glsgd.lev[il].sldy = 0.f;
  glsgd.lev[il].eldx = 0.f;
  glsgd.lev[il].eldy = 0.f;
  glsgd.lev[il].flg = 0;
  glsgd.lev[il].slflg = 0;
  glsgd.lev[il].elflg = 0;
  for (i = 0; i < il; ++i) {
    if (glsgd.lev[i].band == iband &&
	fabs(glsgd.lev[i].j - glsgd.lev[il].j) < 0.7) {
      if (askyn("WARNING: That band already has a state of that spin.\n"
		"...Proceed anyway? (Y/N)")) break;
      glsgd.nlevels--;
      return 1;
    }
  }
  calc_lev(i);
  initg2(&nsx, &nsy);
  setcolor(1);
  pspot(glsgd.lev[il].x[0], glsgd.lev[il].e);
  vect( glsgd.lev[il].x[1], glsgd.lev[il].e);
  setcolor(3);
  drawstring(glsgd.lev[il].sl, glsgd.lev[il].slnc, 'H',
	     glsgd.lev[il].slx, glsgd.lev[il].sly,
	     glsgd.lsl_csx, glsgd.lsl_csy);
  setcolor(1);
  finig();
  return 0;
} /* insert_level */

/* ======================================================================= */
int level_width(char *command)
{
  float d, x1, y1, x2, y2;
  int   ilev, left = 0;
  char  ans[80];

  if (command[2] == 'l' || command[2] == 'L') {
    left = 1;
  } else if (command[2] == 'r' || command[2] == 'R') {
    left = 0;
  } else {
    while (1) {
      ask(ans, 1, "Move left (L) or right (R) edge of levels?");
      if (*ans == 'L' || *ans == 'l') {
	left = 1;
	break;
      } else if (*ans == 'R' || *ans == 'r') {
	left = 0;
	break;
      }
    }
  }

 START:
  tell("Select level to change (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  ilev = nearest_level(x1, y1);
  if (ilev < 0) {
    tell("  ... no level selected, try again...\n");
  } else {
    tell(" Level %s selected...\n", glsgd.lev[ilev].name);
    save_gls_now(1);
    tell("   ...new position, D for default...\n");
    retic2(&x2, &y2, ans);
    if (*ans == 'D' || *ans == 'd') {
      if (left) {
	glsgd.lev[ilev].dxl = 0.f;
      } else {
	glsgd.lev[ilev].dxr = 0.f;
      }
      if (glsgd.lev[ilev].dxl - glsgd.lev[ilev].dxr >=
	  glsgd.bnd[glsgd.lev[ilev].band].nx * .8f) {
	glsgd.lev[ilev].dxl = 0.f;
	glsgd.lev[ilev].dxr = 0.f;
      }
    } else {
      d = x1 - x2;
      if (left) d = -d;
      if (glsgd.lev[ilev].dxl - glsgd.lev[ilev].dxr + d >=
	  glsgd.bnd[glsgd.lev[ilev].band].nx * .8f) {
	warn("ERROR -- that makes level too short.\n");
	goto START;
      }
      if (left) {
	glsgd.lev[ilev].dxl += x2 - x1;
      } else {
	glsgd.lev[ilev].dxr += x2 - x1;
      }
    }
    calc_band();
    calc_gls_fig();
    display_gls(-1);
  }
  goto START;
} /* level_width */

/* ======================================================================= */
int move_band(void)
{
  float x1, y1, x2, y2;
  int   igam, iband;
  char  ans[80];

 START:
  tell("Select band to move (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  iband = nearest_band(x1, y1);
  if (iband < 0 || iband >= glsgd.nbands) {
    tell("  ... no band selected, try again...\n");
  } else {
    tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    save_gls_now(6);
    tell("...New position? (A to abort)...\n");
    retic2(&x2, &y2, ans);
    if (*ans != 'A' && *ans != 'a') {
      glsgd.bnd[iband].x0 = glsgd.bnd[iband].x0 + x2 - x1;
      for (igam = 0; igam < glsgd.ngammas; ++igam) {
	if (glsgd.gam[igam].x1 != 0.f &&
	    glsgd.lev[glsgd.gam[igam].li].band == iband) {
	  glsgd.gam[igam].x1 = glsgd.gam[igam].x1 + x2 - x1;
	  glsgd.gam[igam].x2 = glsgd.gam[igam].x2 + x2 - x1;
	}
      }
      calc_band();
      calc_gls_fig();
      display_gls(-1);
    }
  }
  goto START;
} /* move_band */

/* ======================================================================= */
int move_band_gamma(void)
{
  float x1, y1, x2, y2, x3, x4;
  int   igam, i, iband, nx, ny;
  char  ans[80];

 START:
  tell("Select band to change (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  }
  iband = nearest_band(x1, y1);
  if (iband < 0 || iband >= glsgd.nbands) {
    tell("  ... no band selected, try again...\n");
  } else {
    tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    save_gls_now(2);
    tell("   ...new position, D for default...\n");
    retic2(&x2, &y2, ans);
    initg2(&nx, &ny);
    for (igam = 0; igam < glsgd.ngammas; ++igam) {
      if (glsgd.lev[glsgd.gam[igam].li].band == iband && 
	  glsgd.lev[glsgd.gam[igam].lf].band == iband) {
	if (*ans == 'D' || *ans == 'd') {
	  glsgd.gam[igam].x1 = 0.f;
	} else {
	  if (glsgd.gam[igam].np >= 8) {
	    x3 = (glsgd.gam[igam].x[0] + 
		  glsgd.gam[igam].x[6]) / 2.f;
	  } else {
	    x3 = glsgd.gam[igam].x[0];
	  }
	  x4 = glsgd.gam[igam].x[3];
	  glsgd.gam[igam].x1 = x3 + x2 - x1;
	  glsgd.gam[igam].x2 = x4 + x2 - x1;
	}
	setcolor(0);
	pspot(glsgd.gam[igam].x[0],
	      glsgd.gam[igam].y[0]);
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
      }
    }
    finig();
  }
  goto START;
} /* move_band_gamma */

/* ======================================================================= */
int move_band_spin_label(void)
{
  float x1, y1, x2, y2;
  int   vert, i, iband, nx, ny;
  char  ans[80];

  vert = 0;
  if (askyn("Allow vertical movement of labels? (Y/N)")) vert = 1;
  while (1) {
    tell("Select band to change (X or button 3 to exit)...\n");
    retic2(&x1, &y1, ans);
    if (*ans == 'X' || *ans == 'x') {
      display_gls(-1);
      return 0;
    }
    iband = nearest_band(x1, y1);
    if (iband < 0 || iband >= glsgd.nbands) {
      tell("  ... no band selected, try again...\n");
    } else {
      tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
      save_gls_now(4);
      tell("   ...new position, D for default...\n");
      retic2(&x2, &y2, ans);
      if (*ans == 'D' || *ans == 'd') {
        glsgd.bnd[iband].sldx = 0.f;
        if (vert) glsgd.bnd[iband].sldy = 0.f;
      } else {
        glsgd.bnd[iband].sldx = glsgd.bnd[iband].sldx + x2 - x1;
        if (vert) glsgd.bnd[iband].sldy = glsgd.bnd[iband].sldy + y2 - y1;
      }
      initg2(&nx, &ny);
      for (i = 0; i < glsgd.nlevels; ++i) {
        if (glsgd.lev[i].band == iband) {
          setcolor(0);
          drawstring(glsgd.lev[i].sl, glsgd.lev[i].slnc, 'H',
                     glsgd.lev[i].slx, glsgd.lev[i].sly,
                     glsgd.lsl_csx, glsgd.lsl_csy);
          calc_lev(i);
          setcolor(3);
          drawstring(glsgd.lev[i].sl, glsgd.lev[i].slnc, 'H',
                     glsgd.lev[i].slx, glsgd.lev[i].sly,
                     glsgd.lsl_csx, glsgd.lsl_csy);
        }
      }
      setcolor(1);
      finig();
    }
  }
} /* move_band_spin_label */

/* ======================================================================= */
int move_energy_label(void)
{
  float x1, y1, x2, y2;
  int   ilev, vert, i, iband, nx, ny;
  char  ans[80];

  vert = 0;
  if (askyn("Allow vertical movement of labels? (Y/N)")) vert = 1;
 START:
  tell("Select level or band (B) to change (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  } else if (*ans == 'B' || *ans == 'b') {
    iband = nearest_band(x1, y1);
    if (iband < 0 || iband >= glsgd.nbands) {
      tell("  ... no band selected, try again...\n");
      goto START;
    }
    tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    save_gls_now(4);
    tell("   ...new position, D for default...\n");
    retic2(&x2, &y2, ans);
    if (*ans == 'D' || *ans == 'd') {
      glsgd.bnd[iband].eldx = 0.f;
      if (vert) glsgd.bnd[iband].eldy = 0.f;
    } else {
      glsgd.bnd[iband].eldx = glsgd.bnd[iband].eldx + x2 - x1;
      if (vert) glsgd.bnd[iband].eldy = glsgd.bnd[iband].eldy + y2 - y1;
    }
    initg2(&nx, &ny);
    for (i = 0; i < glsgd.nlevels; ++i) {
      if (glsgd.lev[i].band == iband && glsgd.lev[i].elflg > 0) {
	setcolor(0);
	drawstring(glsgd.lev[i].el, glsgd.lev[i].elnc, 'H',
		   glsgd.lev[i].elx, glsgd.lev[i].ely,
		   glsgd.lel_csx, glsgd.lel_csy);
	calc_lev(i);
	setcolor(4);
	drawstring(glsgd.lev[i].el, glsgd.lev[i].elnc, 'H',
		   glsgd.lev[i].elx, glsgd.lev[i].ely,
		   glsgd.lel_csx, glsgd.lel_csy);
      }
    }
    setcolor(1);
    finig();
  } else {
    ilev = nearest_level(x1, y1);
    if (ilev < 0) {
      tell("  ... no level selected, try again...\n");
      goto START;
    } else if (glsgd.lev[ilev].elflg == 0) {
      tell("Level has no energy displayed, try again...\n" );
      goto START;
    }
    tell(" Level %s selected...\n", glsgd.lev[ilev].name);
    save_gls_now(1);
    tell("   ...new position, D for default...\n");
    retic2(&x2, &y2, ans);
    if (*ans == 'D' || *ans == 'd') {
      glsgd.lev[ilev].eldx = 0.f;
      if (vert) glsgd.lev[ilev].eldy = 0.f;
    } else {
      if (glsgd.lev[ilev].eldx == 0.f) {
	glsgd.lev[ilev].eldx = glsgd.lev[ilev].elx -
	  glsgd.bnd[glsgd.lev[ilev].band].x0;
      }
      glsgd.lev[ilev].eldx = glsgd.lev[ilev].eldx + x2 - x1;
      if (vert) {
	if (glsgd.lev[ilev].eldy == 0.f) {
	  glsgd.lev[ilev].eldy = glsgd.lev[ilev].ely - glsgd.lev[ilev].e;
	}
	glsgd.lev[ilev].eldy = glsgd.lev[ilev].eldy + y2 - y1;
      }
    }
    initg2(&nx, &ny);
    setcolor(0);
    drawstring(glsgd.lev[ilev].el, glsgd.lev[ilev].elnc, 'H',
	       glsgd.lev[ilev].elx, glsgd.lev[ilev].ely,
	       glsgd.lel_csx, glsgd.lel_csy);
    calc_lev(ilev);
    setcolor(4);
    drawstring(glsgd.lev[ilev].el,  glsgd.lev[ilev].elnc, 'H',
	       glsgd.lev[ilev].elx, glsgd.lev[ilev].ely,
	       glsgd.lel_csx, glsgd.lel_csy);
    setcolor(1);
    finig();
  }
  goto START;
} /* move_energy_label */

/* ======================================================================= */
int move_gamma(void)
{
  float x1, y1, x2, y2, x3, x4;
  int   igam, i, nsx, nsy;
  char  ans[80];

 START:
  tell("Select gamma to move (X or button 3 to exit)...\n");
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
    tell(" Gamma %.1f  selected...\n", glsgd.gam[igam].e);
    save_gls_now(2);
    tell("...New position? (A to abort, D for default)...\n" );
    retic2(&x2, &y2, ans);
    if (*ans == 'A' || *ans == 'a') goto START;
    if (*ans == 'D' || *ans == 'd') {
      glsgd.gam[igam].x1 = 0.f;
    } else {
      if (glsgd.gam[igam].np >= 8) {
	x3 = (glsgd.gam[igam].x[0] + 
	      glsgd.gam[igam].x[6]) / 2.f;
      } else {
	x3 = glsgd.gam[igam].x[0];
      }
      x4 = glsgd.gam[igam].x[3];
      glsgd.gam[igam].x1 = x3 + x2 - x1;
      glsgd.gam[igam].x2 = x4 + x2 - x1;
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
  }
  goto START;
} /* move_gamma */

/* ======================================================================= */
int move_gamma_label(void)
{
  float x1, y1, x2, y2;
  int   igam, nx, ny;
  char  ans[80];

 START:
  tell("Select gamma to change (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  }
  igam = nearest_gamma(x1, y1);
  if (igam < 0) {
    tell("  ... no gamma selected, try again...\n");
  } else if (glsgd.gam[igam].elflg == 2) {
    tell("That gamma has no label displayed, try again...\n" );
  } else {
    tell(" Gamma %.1f  selected...\n", glsgd.gam[igam].e);
    save_gls_now(2);
    tell("   ...new position, D for default...\n");
    retic2(&x2, &y2, ans);
    if (*ans == 'D' || *ans == 'd') {
      glsgd.gam[igam].eldx = 0.f;
      glsgd.gam[igam].eldy = 0.f;
    } else {
      glsgd.gam[igam].eldx = glsgd.gam[igam].eldx + x2 - x1;
      glsgd.gam[igam].eldy = glsgd.gam[igam].eldy + y2 - y1;
    }
    initg2(&nx, &ny);
    setcolor(0);
    drawstring(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H', 
	       glsgd.gam[igam].elx, glsgd.gam[igam].ely,
	       glsgd.gel_csx, glsgd.gel_csy);
    calc_gam(igam);
    setcolor(1);
    drawstring(glsgd.gam[igam].el, glsgd.gam[igam].elnc, 'H',
	       glsgd.gam[igam].elx, glsgd.gam[igam].ely,
	       glsgd.gel_csx, glsgd.gel_csy);
    finig();
  }
  goto START;
} /* move_gamma_label */

/* ======================================================================= */
int move_level(void)
{
  float x1, y1, x2, y2;
  int   igam, ilev, jlev, j, iband, jband;
  char  ans[80];

 START:
  tell("Select level to move (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  ilev = nearest_level(x1, y1);
  if (ilev < 0) {
    tell("  ... no level selected, try again...\n");
  } else {
    tell(" Level %s selected...\n"
	   " ...Move level to band = ?\n"
	   "    (A to abort, N for new band)...\n", glsgd.lev[ilev].name);
    retic2(&x2, &y2, ans);
    while (*ans != 'A' && *ans != 'a') {
      if (*ans == 'N' || *ans == 'n') {
	iband = glsgd.nbands;
	tell("   ... Give position for new band (A to abort)...\n");
	retic2(&x2, &y2, ans);
	if (*ans == 'A' || *ans == 'a') goto START;
	glsgd.bnd[iband].x0 = x2 - glsgd.bnd[iband].nx / 2.f;
      } else {
	iband = nearest_band(x2, y2);
      }
      if (iband < 0) {
	tell("  ... no band selected, try again...\n");
	retic2(&x2, &y2, ans);
	continue;
      } else if (iband == glsgd.lev[ilev].band) {
	tell("  ... same band selected, try again...\n");
	retic2(&x2, &y2, ans);
	continue;
      } else if (iband >= glsgd.nbands) {
	save_gls_now(5);
	if (insert_band(&iband)) goto START;
      } else {
	tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
	for (jlev = 0; jlev < glsgd.nlevels; ++jlev) {
	  if (jlev != ilev &&
	      glsgd.lev[jlev].band == iband && 
	      fabs(glsgd.lev[jlev].j - glsgd.lev[ilev].j) < 0.7) {
	    if (askyn("WARNING: That band already has a state"
		      " of the same spin.\n"
		      "...Proceed anyway? (Y/N)")) break;
	    goto START;
	  }
	}
	save_gls_now(1);
      }
      jband = glsgd.lev[ilev].band;
      glsgd.lev[ilev].band = iband;
      for (igam = 0; igam < glsgd.ngammas; ++igam) {
	if (glsgd.gam[igam].x1 != 0.f &&
	    glsgd.gam[igam].li == ilev) {
	  glsgd.gam[igam].x1 += glsgd.bnd[iband].x0 - glsgd.bnd[jband].x0;
	  glsgd.gam[igam].x2 += glsgd.bnd[iband].x0 - glsgd.bnd[jband].x0;
	}
      }
      for (j = 0; j < glsgd.nlevels; ++j) {
	if (glsgd.lev[j].band == jband) break;
      }
      if (j >= glsgd.nlevels) remove_band(jband);
      calc_band();
      calc_gls_fig();
      display_gls(-1);
      goto START;
     }
  }
  goto START;
} /* move_level */

/* ======================================================================= */
int move_level_spin_label(void)
{
  float x1, y1, x2, y2;
  int   ilev, vert, nx, ny;
  char  ans[80];

  vert = 0;
  if (askyn("Allow vertical movement of labels? (Y/N)")) vert = 1;
 START:
  tell("Select level to change (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') {
    display_gls(-1);
    return 0;
  }
  ilev = nearest_level(x1, y1);
  if (ilev < 0) {
    tell("  ... no level selected, try again...\n");
  } else if (glsgd.lev[ilev].slflg >= 6) {
    tell("Level has no spin label displayed, try again...\n" );
  } else {
    tell(" Level %s selected...\n", glsgd.lev[ilev].name);
    save_gls_now(1);
    tell("   ...new position, D for default...\n");
    retic2(&x2, &y2, ans);
    if (*ans == 'D' || *ans == 'd') {
      glsgd.lev[ilev].sldx = 0.f;
      if (vert) glsgd.lev[ilev].sldy = 0.f;
    } else {
      if (glsgd.lev[ilev].sldx == 0.f) {
	glsgd.lev[ilev].sldx = glsgd.lev[ilev].slx -
	  glsgd.bnd[glsgd.lev[ilev].band].x0;
      }
      glsgd.lev[ilev].sldx = glsgd.lev[ilev].sldx + x2 - x1;
      if (vert) {
	if (glsgd.lev[ilev].sldy == 0.f) {
	  glsgd.lev[ilev].sldy = glsgd.lev[ilev].sly - glsgd.lev[ilev].e;
	}
	glsgd.lev[ilev].sldy = glsgd.lev[ilev].sldy + y2 - y1;
      }
    }
    initg2(&nx, &ny);
    setcolor(0);
    drawstring(glsgd.lev[ilev].sl, glsgd.lev[ilev].slnc, 'H',
	       glsgd.lev[ilev].slx, glsgd.lev[ilev].sly,
	       glsgd.lsl_csx, glsgd.lsl_csy);
    calc_lev(ilev);
    setcolor(3);
    drawstring(glsgd.lev[ilev].sl, glsgd.lev[ilev].slnc, 'H',
	       glsgd.lev[ilev].slx, glsgd.lev[ilev].sly,
	       glsgd.lsl_csx, glsgd.lsl_csy);
    setcolor(1);
    finig();
  }
  goto START;
} /* move_level_spin_label */

/* ======================================================================= */
int move_many_bands(void)
{
  float x1, y1, x2, y2, x3;
  int   igam, i, j, iband, nmbands, nextband[MAXBAND];
  char  ans[80];

  nmbands = 0;
  tell("Click to select/deselect bands to be moved...\n"
       "      (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  while (*ans != 'X' && *ans != 'x') {
    iband = nearest_band(x1, y1);
    if (iband < 0 || iband >= glsgd.nbands) {
      tell("  ... no band selected, try again...\n");
    } else {
      for (i = 0; i < nmbands; ++i) {
	if (iband == nextband[i]) {
	  tell(" Band %.8s deselected...\n", glsgd.bnd[iband].name);
	  --nmbands;
	  for (j = 0; j < nmbands; ++j) {
	    nextband[j] = nextband[j+1];
	  }
	  goto SKIP;
	}
      }
      tell(" Band %.8s selected...\n", glsgd.bnd[iband].name);
      nextband[nmbands++] = iband;
    }
  SKIP:
    retic2(&x1, &y1, ans);
  }
  /* now move the selected bands */
  if (nmbands == 0) {
    tell("No bands to be moved.\n");
    return 0;
  }
  tell(" %i bands selected...\n"
       "  ...Click to give new position...\n"
       "  ...A to abort, X to exit when satisfied...\n", nmbands);
  save_gls_now(4);
  x1 = 0.f;
  for (i = 0; i < nmbands; ++i) {
    x1 += glsgd.bnd[nextband[i]].x0;
  }
  x1 /= (float) nmbands;
  x3 = x1;
  retic2(&x2, &y2, ans);
  while (*ans != 'X' && *ans != 'x') {
    if (*ans == 'A' || *ans == 'a') {
      /*
	for (i = 0; i < nmbands; ++i) {
	iband = nextband[i];
	glsgd.bnd[iband].x0 += x1 - x3;
	for (igam = 0; igam < glsgd.ngammas; ++igam) {
	if (glsgd.gam[igam].x1 != 0.f &&
	glsgd.lev[glsgd.gam[igam].li].band == iband) {
	glsgd.gam[igam].x1 += x1 - x3;
	glsgd.gam[igam].x2 += x1 - x3;
	}
	}
	}
	calc_band();
	calc_gls_fig();
	display_gls(-1);
      */
      undo_gls(-1);
      return 0;
    }
    for (i = 0; i < nmbands; ++i) {
      iband = nextband[i];
      glsgd.bnd[iband].x0 += x2 - x3;
      for (igam = 0; igam < glsgd.ngammas; ++igam) {
	if (glsgd.gam[igam].x1 != 0.f &&
	    glsgd.lev[glsgd.gam[igam].li].band == iband) {
	  glsgd.gam[igam].x1 += x2 - x3;
	  glsgd.gam[igam].x2 += x2 - x3;
	}
      }
    }
    calc_band();
    calc_gls_fig();
    display_gls(-1);
    x3 = x2;
    retic2(&x2, &y2, ans);
  }
  return 0;
} /* move_many_bands */

/* ======================================================================= */
int move_text(void)
{
  float x1, y1, x2, y2;
  int   i, it, nsx, nsy;
  char  ans[80];

  if (glsgd.ntlabels == 0) {
    warn("Have no text labels to move.\n");
    return 0;
  }
 START:
  tell("Select text label to move (X or button 3 to exit)...\n");
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
	y1 < glsgd.txt[i].y + glsgd.txt[i].csy) it = i;
  }
  if (it < 0) {
    tell(" ... No text label selected, try again ...\n");
    goto START;
  }
  tell("...Selected label %.40s\n"
       "   ...New position? (A to abort)\n", glsgd.txt[it].l);
  retic2(&x2, &y2, ans);
  if (*ans == 'A' || *ans == 'a') goto START;
  save_gls_now(8);
  initg2(&nsx, &nsy);
  setcolor(0);
  drawstring(glsgd.txt[it].l, glsgd.txt[it].nc, 'H',
	     glsgd.txt[it].x, glsgd.txt[it].y,
	     glsgd.txt[it].csx, glsgd.txt[it].csy);
  setcolor(1);
  glsgd.txt[it].x = glsgd.txt[it].x + x2 - x1;
  glsgd.txt[it].y = glsgd.txt[it].y + y2 - y1;
  drawstring(glsgd.txt[it].l, glsgd.txt[it].nc, 'H',
	     glsgd.txt[it].x, glsgd.txt[it].y,
	     glsgd.txt[it].csx, glsgd.txt[it].csy);
  finig();
  goto START;
} /* move_text */

/* ======================================================================= */
int nearest_band(float x, float y)
{
  int ret_val = -1, i, j;

  for (i = 0; i < glsgd.nbands + 2; ++i) {
    if (glsgd.bnd[i].x0 <= x &&
	glsgd.bnd[i].x0 + glsgd.bnd[i].nx >= x) ret_val = i;
  }
  if ((j = nearest_level(x, y)) >= 0) ret_val = glsgd.lev[j].band;
  return ret_val;
} /* nearest_band */

/* ======================================================================= */
int nearest_gamma(float x, float y)
{
  float r1, r2, d, d1;
  int   ret_val = -1, i, lf, li;

  d = 999.9f;
  for (i = 0; i < glsgd.ngammas; ++i) {
    li = glsgd.gam[i].li;
    lf = glsgd.gam[i].lf;
    r1 = (glsgd.gam[i].x1 + glsgd.gam[i].x2) / 2.f - x;
    r2 = (glsgd.lev[li].e + glsgd.lev[lf].e) / 2.f - y;
    d1 = sqrt(r1*r1 + r2*r2);
    if (d1 < d) {
      d = d1;
      ret_val = i;
    }
  }
  return ret_val;
} /* nearest_gamma */

/* ======================================================================= */
int nearest_level(float x, float y)
{
  float d = 99999.9f, d1;
  int   ret_val = -1, j;

  for (j = 0; j < glsgd.nlevels; ++j) {
    d1 = fabs(glsgd.lev[j].e - y);
    if (glsgd.lev[j].x[0] <= x &&
	glsgd.lev[j].x[1] >= x && d1 < d) {
      d = d1;
      ret_val = j;
    }
  }
  return ret_val;
} /* nearest_level */

/* ======================================================================= */
int open_close_gap(void)
{
  float x1, y1, x2, y2;
  int   igam, i, iband;
  char  ans[80];

 START:
  tell("Gap position? (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  tell("...New position? (A to abort)...\n");
  retic2(&x2, &y2, ans);
  if (*ans != 'A' && *ans != 'a') {
    save_gls_now(12);
    for (iband = 0; iband < glsgd.nbands; ++iband) {
      if (glsgd.bnd[iband].x0 >= x1) {
	glsgd.bnd[iband].x0 += x2 - x1;
	for (igam = 0; igam < glsgd.ngammas; ++igam) {
	  if (glsgd.gam[igam].x1 != 0.f &&
	      glsgd.lev[glsgd.gam[igam].li].band == iband) {
	    glsgd.gam[igam].x1 += x2 - x1;
	    glsgd.gam[igam].x2 += x2 - x1;
	  }
	}
      }
    }
    for (i = 0; i < glsgd.ntlabels; ++i) {
      if (glsgd.txt[i].x >= x1) {
	glsgd.txt[i].x = glsgd.txt[i].x + x2 - x1;
      }
    }
    calc_band();
    calc_gls_fig();
    display_gls(-1);
  }
  goto START;
} /* open_close_gap */

/* ======================================================================= */
int pan_gls(float x, float y, int iflag)
{
  /* shift-button pressed;  pan/zoom gls display */

  if (iflag == 2) {
    /* button 2 zoom in */
    glsgd.x0 = glsgd.x00 + (glsgd.dx - glsgd.dx / 1.3f) / 2.f;
    glsgd.y0 = glsgd.y00 + (glsgd.dy - glsgd.dy / 1.3f) / 2.f;
    glsgd.dx /= 1.3f;
    glsgd.dy /= 1.3f;
  } else if (iflag == 3) {
    /* button 3 zoom out */
    glsgd.x0 = glsgd.x00 + (glsgd.dx - glsgd.dx * 1.3f) / 2.f;
    glsgd.y0 = glsgd.y00 + (glsgd.dy - glsgd.dy * 1.3f) / 2.f;
    glsgd.dx *= 1.3f;
    glsgd.dy *= 1.3f;
  } else {
    /* button 1 center display on x,y */
    glsgd.x0 = x - glsgd.dx / 2.f;
    glsgd.y0 = y - glsgd.dy / 2.f;
  }
  glsgd.hix = glsgd.x0 + glsgd.dx;
  glsgd.hiy = glsgd.y0 + glsgd.dy;
  display_gls(0);
  return 0;
} /* pan_gls */

/* ======================================================================= */
int remove_band(int iband)
{
  int j;

  for (j = glsgd.nlevels-1; j >= 0; --j) {
    if (glsgd.lev[j].band == iband) remove_level(j);
  }
  for (j = 0; j < glsgd.nlevels; ++j) {
    if (glsgd.lev[j].band > iband) --glsgd.lev[j].band;
  }
  --glsgd.nbands;
  for (j = iband; j < glsgd.nbands; ++j) {
    memcpy(glsgd.bnd[j].name, glsgd.bnd[j+1].name, sizeof(bnddat));
  }
  calc_band();
  calc_gls_fig();
  return 0;
} /* remove_band */

/* ======================================================================= */
int remove_gamma(int igam)
{
  int i, j, nsx, nsy;

  initg2(&nsx, &nsy);
  setcolor(0);
  pspot(glsgd.gam[igam].x[0], glsgd.gam[igam].y[0]);
  for (i = 1; i < glsgd.gam[igam].np; ++i) {
    vect(glsgd.gam[igam].x[i], glsgd.gam[igam].y[i]);
  }
  setcolor(1);
  finig();
  --glsgd.ngammas;
  for (j = igam; j < glsgd.ngammas; ++j) {
    memcpy(&glsgd.gam[j].li, &glsgd.gam[j+1].li, sizeof(gamdat));
  }
  return 0;
} /* remove_gamma */

/* ======================================================================= */
int remove_level(int ilev)
{
  int j, nsx, nsy;

  initg2(&nsx, &nsy);
  setcolor(0);
  pspot(glsgd.lev[ilev].x[0], glsgd.lev[ilev].e);
  vect( glsgd.lev[ilev].x[1], glsgd.lev[ilev].e);
  setcolor(1);
  finig();
  for (j = glsgd.ngammas-1; j >= 0; --j) {
    if (glsgd.gam[j].li == ilev ||
	glsgd.gam[j].lf == ilev ) remove_gamma(j);
  }
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].li > ilev ) --glsgd.gam[j].li;
    if (glsgd.gam[j].lf > ilev ) --glsgd.gam[j].lf;
  }
  --glsgd.nlevels;
  for (j = ilev; j < glsgd.nlevels; ++j) {
    memcpy(&glsgd.lev[j].e, &glsgd.lev[j+1].e, sizeof(levdat));
  }
  calc_band();
  return 0;
} /* remove_level */

/* ======================================================================= */
int swap_gamma(void)
{
  float x1, y1;
  int   jgam, ilev, nfeed, ideex, ifeed = 0;
  char  ans[80];

  /* swap ordering of gammas feeding/deexciting levels */
 START:
  tell("Select level to swap gammas (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  ilev = nearest_level(x1, y1);
  if (ilev < 0) {
    tell("  ... no level selected, try again...\n");
  } else {
    tell(" Level %s selected...\n", glsgd.lev[ilev].name);
    nfeed = 0;
    for (jgam = 0; jgam < glsgd.ngammas; ++jgam) {
      if (glsgd.gam[jgam].lf == ilev) {
	++nfeed;
	ifeed = jgam;
      }
    }
    if (levemit.n[ilev] != 1 || nfeed != 1) {
      tell("ERROR - level must have one feeding and one deexciting gamma.\n"
	   "        Try again...\n");
    } else {
      save_gls_now(3);
      ideex = levemit.l[ilev][0];
      glsgd.gam[ifeed].lf = glsgd.gam[ideex].lf;
      glsgd.gam[ideex].lf = ilev;
      glsgd.gam[ideex].li = glsgd.gam[ifeed].li;
      glsgd.gam[ifeed].li = ilev;
      /* fix level energy */
      glsgd.lev[ilev].e = glsgd.lev[glsgd.gam[ifeed].lf].e + glsgd.gam[ifeed].e;
      /* fix spin */
      glsgd.lev[ilev].j = glsgd.lev[glsgd.gam[ifeed].lf].j +
	                  glsgd.lev[glsgd.gam[ideex].li].j - glsgd.lev[ilev].j;
      /* fix parity */
      glsgd.lev[ilev].pi *= glsgd.lev[glsgd.gam[ifeed].lf].pi *
	                    glsgd.lev[glsgd.gam[ideex].li].pi;
      indices_gls();
      path(1);
      calc_peaks();
      calc_gls_fig();
      display_gls(-1);
    }
  }
  goto START;
} /* swap_gamma */

/* ======================================================================= */
int tentative_gamma(void)
{
  float x1, y1;
  int   igam;
  char  ans[80];

 START:
  tell("Select gamma to change (X or button 3 to exit)...\n");
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  igam = nearest_gamma(x1, y1);
  if (igam < 0) {
    tell("  ... no gamma selected, try again...\n");
  } else {
    tell(" Gamma %.1f  selected...\n", glsgd.gam[igam].e);
    save_gls_now(2);
    if (glsgd.gam[igam].flg == 0) {
      glsgd.gam[igam].flg = 1;
      if (glsgd.gam[igam].elflg == 0) glsgd.gam[igam].elflg = 1;
      if (glsgd.gam[igam].elflg == 2) glsgd.gam[igam].elflg = 3;
    } else {
      glsgd.gam[igam].flg = 0;
      if (glsgd.gam[igam].elflg == 1) glsgd.gam[igam].elflg = 0;
      if (glsgd.gam[igam].elflg == 3) glsgd.gam[igam].elflg = 2;
    }
    calc_gam(igam);
    display_gls(-1);
  }
  goto START;
} /* tentative_gamma */

/* ======================================================================= */
int tentative_level(void)
{
  float x1, y1;
  int   ilev;
  char  ans[80];

  while (1) {
    tell("Select level to change (X or button 3 to exit)...\n");
    retic2(&x1, &y1, ans);
    if (*ans == 'X' || *ans == 'x') return 0;
    ilev = nearest_level(x1, y1);
    if (ilev < 0) {
      tell("  ... no level selected, try again...\n");
    } else {
      tell(" Level %s selected...\n", glsgd.lev[ilev].name);
      save_gls_now(1);
      if (glsgd.lev[ilev].flg == 2) {
        glsgd.lev[ilev].flg = 0;
      } else if (++glsgd.lev[ilev].flg == 1) {
        if (glsgd.lev[ilev].slflg == 0) glsgd.lev[ilev].slflg = 3;
        if (glsgd.lev[ilev].elflg == 1) glsgd.lev[ilev].elflg = 2;
      } else {
        if (glsgd.lev[ilev].slflg == 3) glsgd.lev[ilev].slflg = 0;
        if (glsgd.lev[ilev].elflg == 2) glsgd.lev[ilev].elflg = 1;
      }
      calc_lev(ilev);
      display_gls(-1);
    }
  }
} /* tentative_level */

/* ====================================================================== */
int up_down_levels(void)
{
  float elow, x1, y1, y2, fj1, fj2;
  int   i, l, nlvls, jf, nc, ji, repeat;
  short ilev[MAXLEV];
  char  ans[80];

  /* change energies of a group of levels */
  while (1) {
    tell("Select a level in the bunch of connected levels\n"
	 "    that you want to move, or press x to exit\n");
    retic2(&x1, &y1, ans);
    if (*ans == 'X' || *ans == 'x') return 0;
    if ((l = nearest_level(x1, y1)) < 0) {
      tell("No level selected, try again...\n");
      continue;
    }
    tell(" Level %s selected...\n", glsgd.lev[l].name);
    for (i = 0; i < glsgd.nlevels; ++i) {
      ilev[i] = 0;
    }
    ilev[l] = 1;
    elow = glsgd.lev[l].e;
    nlvls = 1;
    repeat = 1;
    while (repeat) {
      repeat = 0;
      for (i = 0; i < glsgd.ngammas; ++i) {
	ji = glsgd.gam[i].li;
	jf = glsgd.gam[i].lf;
	if (ilev[ji] != ilev[jf]) {
	  repeat = 1;
	  ilev[ji] = 1;
	  ilev[jf] = 1;
	  ++nlvls;
	  if (elow > glsgd.lev[jf].e) elow = glsgd.lev[jf].e;
	}
      }
    }
    tell("%d levels included.\n"
	 " Lowest level energy = %f\n"
	 " Enter new position (a to abort,\n"
	 "     t to type energy of lowest included level)\n",
	 nlvls, elow);
    retic2(&x1, &y2, ans);
    if (*ans == 'A' || *ans == 'a') continue;
    if (*ans == 'T' || *ans == 't') {
      nc = ask(ans, 80, "Okay, new lowest energy = ?");
      if (ffin(ans, nc, &y2, &fj1, &fj2)) continue;
      y1 = elow;
    }
    if (elow + y2 - y1 < 0.0) {
      tell("Limiting change to %.1f keV to prevent levels having E < 0.\n",
	   elow);
      y2 = y1 - elow;
    }
    save_gls_now(1);
    for (i = 0; i < glsgd.nlevels; ++i) {
      if (ilev[i] == 1) {
	glsgd.lev[i].e = glsgd.lev[i].e + y2 - y1;
      }
    }
    calc_gls_fig();
    display_gls(-1);
  }
} /* up_down_levels */

/* ======================================================================= */
int save_gls_now(int mode)
{
  /*
    save current level scheme info to temp file
    mode =  -1 to reset (e.g. when new level scheme read)
    mode =   0 to save general level scheme and display info,
           + 1 to save levels
           + 2 to save gammas
           + 4 to save bands
           + 8 to save text labels
  */

  int j, m, nb;

  if (!glsundo_ready) {
    glsundo.prev_pos = -1;
    glsundo.lastmode = glsundo.next_pos = glsundo.file_pos = glsundo.eof = 0;
    if (!(glsundo.file = tmpfile())) {
      warn("ERROR - cannot open gls undo temp file!\n");
      exit(-1);
    }
    glsundo.mark = glsundo.next_pos;
    glsundo_ready = 1;
  }

  /* undo_gls_notify(flag):
     flag = 1: undo_edits now available
            2: redo_edits now available
            3: undo_edits back at flagged position
           -1: undo_edits no longer available
           -2: redo_edits no longer available
           -3: undo_edits no longer at flagged position */
  if (mode < 0) {
    glsundo.prev_pos = -1;
    glsundo.lastmode = glsundo.next_pos = glsundo.file_pos = glsundo.eof = 0;
    if (glsundo.undo_available) undo_gls_notify(-1);
    if (glsundo.redo_available) undo_gls_notify(-2);
    undo_gls_notify(3);
    glsundo.undo_available = glsundo.redo_available = 0;
    glsundo.mark = glsundo.next_pos;
    return 0;
  }

  if (glsundo.mark == glsundo.next_pos) undo_gls_notify(-3);
  if (glsundo.file_pos > MAXSAVESIZE) {
    rewind(glsundo.file);
    glsundo.prev_pos = -1;
    glsundo.lastmode = glsundo.next_pos = glsundo.file_pos = glsundo.eof = 0;
    glsundo.mark = -2;
  } else if (glsundo.mark > glsundo.next_pos) {
    glsundo.mark = -2;
  }
  if (!glsundo.undo_available) undo_gls_notify(1);
  if (glsundo.redo_available) undo_gls_notify(-2);
  glsundo.undo_available = 1;
  glsundo.redo_available = 0;

#define W(a,b) fwrite(a, b, 1, glsundo.file)
  m = (mode | glsundo.lastmode);

  fseek(glsundo.file, glsundo.next_pos, SEEK_SET);
  W(&glsundo.prev_pos, 4);
  W(&m, 4);
  nb = (long) &glsgd.x0 - (long) &glsgd.gls_file_name[0];
  W(glsgd.gls_file_name, nb);
  if (m&1) {
    nb = (long) glsgd.lev[0].x - (long) &glsgd.lev[0].e;
    for (j = 0; j < glsgd.nlevels; ++j) {
      W(&glsgd.lev[j].e, 60);
    }
  }
  if (m&2) {
    nb = (long) glsgd.gam[0].x - (long) &glsgd.gam[0].li;
    for (j = 0; j < glsgd.ngammas; ++j) {
      W(&glsgd.gam[j].li, nb);
    }
  }
  if (m&4) {
    nb = sizeof(bnddat)*glsgd.nbands;
    W(glsgd.bnd[0].name, nb);
  }
  if (m&8) {
    nb = sizeof(txtdat)*glsgd.nbands;
    W(glsgd.txt[0].l, nb);
  }
#undef W

  glsundo.lastmode = mode;
  glsundo.prev_pos = glsundo.next_pos;
  glsundo.next_pos = glsundo.file_pos = glsundo.eof = ftell(glsundo.file);
  return 0;
} /* save_gls_now */

/* ======================================================================= */
int undo_gls(int step)
{
  /* undo or redo gls edits */
  int j, pos, nb, mode;

  if (!glsundo_ready || step == 0) return 1;

  while (step != 0) {
    if (step < 0 && !glsundo.undo_available) {
      bell();
      tell("No more saved edits to undo...\n");
      return 1;
    } else if (step > 0 && !glsundo.redo_available) {
      bell();
      tell("No more undone edits to redo...\n");
      return 1;
    }

    /* undo_gls_notify(flag):
     flag = 1: undo_edits now available
            2: redo_edits now available
            3: undo_edits back at flagged position
           -1: undo_edits no longer available
           -2: redo_edits no longer available
           -3: undo_edits no longer at flagged position */
    if (glsundo.mark == glsundo.next_pos) undo_gls_notify(-3);
    if (step < 0) {
      /* undo previous edits */
      pos = glsundo.prev_pos;
      if (glsundo.file_pos == glsundo.eof) {
	/* Since we are at the end of the file,
	   we must save current info for a possible later redo.
	   This is the source of much possible confusion */
	save_gls_now(glsundo.lastmode);
      }
      if (!glsundo.redo_available) undo_gls_notify(2);
      glsundo.redo_available = 1;
    } else {
      /* redo previously undone edits */
      pos = glsundo.file_pos;
      if (pos > MAXSAVESIZE) pos = 0;
      if (!glsundo.undo_available) undo_gls_notify(1);
      glsundo.undo_available = 1;
    }

    fseek(glsundo.file, pos, SEEK_SET);
 
#define R(a,b) fread(a, b, 1, glsundo.file)
    R(&glsundo.prev_pos, 4);
    R(&mode, 4);
    nb = (long) &glsgd.x0 - (long) &glsgd.gls_file_name[0];
    R(glsgd.gls_file_name, nb);
    if (mode&1) {
      nb = (long) glsgd.lev[0].x - (long) &glsgd.lev[0].e;
      for (j = 0; j < glsgd.nlevels; ++j) {
	R(&glsgd.lev[j].e, 60);
      }
    }
    if (mode&2) {
      nb = (long) glsgd.gam[0].x - (long) &glsgd.gam[0].li;
      for (j = 0; j < glsgd.ngammas; ++j) {
	R(&glsgd.gam[j].li, nb);
      }
    }
    if (mode&4) {
      nb = sizeof(bnddat)*glsgd.nbands;
      R(glsgd.bnd[0].name, nb);
    }
    if (mode&8) {
      nb = sizeof(txtdat)*glsgd.nbands;
      R(glsgd.txt[0].l, nb);
    }
#undef R
    indices_gls();
    calc_band();
    calc_gls_fig();
    display_gls(-1);
    calc_peaks();

    glsundo.file_pos = ftell(glsundo.file);
    glsundo.next_pos = pos;
    if (glsundo.mark == pos) undo_gls_notify(3);

    if (step > 0 && glsundo.file_pos == glsundo.eof) {
      /* the edit we have just redone is the last one */
      undo_gls_notify(-2);
      glsundo.redo_available = 0;
    } else if (step < 0 && glsundo.prev_pos < 0) {
      undo_gls_notify(-1);
      glsundo.undo_available = 0;
    }
    if (step < 0) {
      step++;
    } else {
      step--;
    }
  }
  return 0;
} /* undo_gls */
/* ======================================================================= */
int undo_gls_mark(void)
{

  if (!glsundo_ready) save_gls_now(-1);
  glsundo.mark = glsundo.next_pos;
  undo_gls_notify(3);
  return glsundo.next_pos;
}
