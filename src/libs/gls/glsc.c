#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "gls.h"
#include "util.h"

/* This file has functions which are used only by the stand-alone gls program.
   escl8r, levit8r and 4dg8r have replacement versions of these functions.      */

extern int glsundo_ready;

/* ==== other extern routines ==== */
extern int save_xwg(char *);
extern int retic2(float *, float *, char *);

/* all figure distance units are in keV */
/* gamma arrow widths = intensity * aspect_ratio (in keV units) */

/* ======================================================================= */
int calc_peaks(void)
{
  /* dummy routine
     included here for compatibilty between gls_exec2 and escl8r_gls etc
     real routine is in escl8ra or lev4d */
  return 0;
} /* calc_peaks */

/* ======================================================================= */
int examine_gamma(void)
{
  float x1, y1;
  int   igam;
  char  ans[80];

  tell(" Select gamma to examine (X or button 3 to exit)...\n");
 START:
  igam = 0;
  retic2(&x1, &y1, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  igam = nearest_gamma(x1, y1);
  if (igam < 0) {
    tell("  ... no gamma selected, try again...\n");
  } else {
    tell(" Energy = %7.2f +- %4.2f,  Intensity = %7.2f +- %4.2f\n",
	 glsgd.gam[igam].e, glsgd.gam[igam].de,
	 glsgd.gam[igam].i, glsgd.gam[igam].di);
  }
  goto START;
} /* examine_gamma */

/* ======================================================================= */
float get_energy(void)
{
  float fj1, e;
  int   nc;
  char  ans[80];

  e = -1.0f;
  if ((nc = ask(ans, 80, "Gamma energy = ? (rtn to quit)")))
     ffin(ans, nc, &e, &fj1, &fj1);
  return e;
} /* get_energy */

/* ======================================================================= */
int gls_exec(char *ans)
{
  float x1, y1, x2, y2;
  int   i, ic;

  /* this subroutine decodes and executes the commands */
  /* convert lower case to upper case characters */
  for (i = 0; i < 2; ++i) {
    ic = ans[i];
    if (ic >= 97 && ic <= 122) ans[i] = (char) (ic - 32);
  }
  if (!gls_exec2(ans)) return 0;

  if (!strncmp(ans, "EX", 2)) {
    retic2(&x1, &y1, ans);
    retic2(&x2, &y2, ans);
    glsgd.x0 =  x1;
    glsgd.hix = x2;
    if (x1 > x2) {
      glsgd.x0 = x2;
      glsgd.hix = x1;
    }
    glsgd.y0 = y1;
    glsgd.hiy = y2;
    if (y1 > y2) {
      glsgd.y0 = y2;
      glsgd.hiy = y1;
    }
    display_gls(0);
  } else if (!strncmp(ans, "HE", 2)) {
    gls_help();
  } else if (!strncmp(ans, "ST", 2)) {
    if (askyn("Are you sure you want to exit? (Y/N)")) {
      if (glsundo_ready &&
	  glsundo.mark != glsundo.next_pos) read_write_gls("WRITE");
      save_xwg("gls_std_");
      exit(0);
    }
  } else {
    /* command cannot be recognized */
    tell("Bad command.\n");
  }
  return 0;
} /* gls_exec */

/* ======================================================================= */
int path(int mode)
{
  /* dummy routine
  included here for compatibilty between gls_exec2 and escl8r_gls etc
  real routine is in esclev */
  return 0;
} /* path */
