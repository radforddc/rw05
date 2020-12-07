#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"
#include "minig.h"
#include "gls.h"
#include "esclev.h"

extern FILE *infile, *cffile;    /* command file flag and file descriptor*/
extern int  cflog;               /* command logging flag */

/* This file contains subroutines that are common to all versions */
/* of escl8r and levit8r. */
/* D.C. Radford            Sept 1999 */

/* function defined in drawstring.c */
int drawstring(char *, int, char, float, float, float, float);
/* functions defined in escl8ra.c or lev4d.c */
int combine(char *ans, int nc); /* returns 1 on error */
int disp_dat(int iflag);
int energy(float x, float dx, float *eg, float *deg);
int examine(char *ans, int nc); /* returns 1 on error */
int num_gate(char *ans, int nc); /* returns 1 on error */

char listnam[55] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|";

/* ======================================================================= */
float get_energy(void)
{
  float eg, x, y, dx, fj1, deg;
  int   nc;
  char  ans[80];

  eg = -1.0f;
  tell("Enter gamma energy using cursor on spectrum window.\n"
       " ...Hit any character;\n"
       " T to type energy, X or button 3 to exit.\n");
  retic(&x, &y, ans);
  if (*ans == 'X' || *ans == 'x') return 0;
  if (*ans == 'T' || *ans == 't') {
    nc = ask(ans, 80, "Gamma energy = ? (rtn to quit)");
    if (nc > 0) ffin(ans, nc, &eg, &fj1, &fj1);
  } else {
    dx = 0.0f;
    energy(x - 0.5f, dx, &eg, &deg);
  }
  return eg;
} /* get_energy */

/* ======================================================================= */
int get_fit_gam(int *jng, short *jgam)
{
  float x1, y1;
  int   igam, ilev;
  int   i, k, iband, askit, nc, ki;
  char  li[80], ans[80], new_ans[80];

/* get gamma indices to be fitted in intensity and/or energy */
/* ask for gammas to be fitted */

  askit = 0;
START:
  if (*jng <= 0 || askit ||
      (*ans != 'L' && *ans != 'l' && *ans != 'B' && * ans != 'b')) {
    tell(" Gammas to be fitted = ?\n"
	 "  ...X to end,\n"
	 "  ...button 2 or L to specify initial level(s),\n"
	 "  ...button 3 or B to specify initial band(s),\n"
	 "  ...A for all gammas in level scheme.\n");
    retic2(&x1, &y1, ans);
    if (!strcmp(ans, "X") || !strcmp(ans, "x")) {
      *jng = 0;
      return 0;
    }
  }
  askit = 1;
  *jng = 0;
  if (*ans == 'A' || *ans == 'a') {
    for (i = 0; i < glsgd.ngammas; ++i) {
      jgam[(*jng)++] = (short) i;
    }
  } else if (*ans == 'L' || *ans == 'l') {
    while (1) {
      if (!(nc = ask(new_ans, 80,
		     " ...Name of initial level = ? (rtn to end)"))) goto START;
      if (nc >= 3 && nc <= 10) {
	strcpy(li, "         ");
	strcpy(li + 10 - nc, new_ans);
	for (ki = 0; ki < glsgd.nlevels; ++ki) {
	  if (!strcmp(glsgd.lev[ki].name, li)) break;
	}
	if (ki < glsgd.nlevels) break;
      }
      warn1("*** Bad level name. ***\n");
    }
    for (k = 0; k < levemit.n[ki]; ++k) {
      jgam[(*jng)++] = (short) levemit.l[ki][k];
    }
  } else if (*ans == 'B' || *ans == 'b') {
    while (1) {
      nc = ask(new_ans, 80, " ...Name of initial band = ? (rtn to end)");
      if (nc == 0) goto START;
      if (nc < 9) break;
      warn1("*** Bad band name. ***\n");
    }
    strcpy(li, "         ");
    strcpy(li + 8 - nc, new_ans);
    for (ki = 0; ki < glsgd.nlevels; ++ki) {
      if (!strncmp(glsgd.lev[ki].name, li, 8)) {
	for (k = 0; k < levemit.n[ki]; ++k) {
	  jgam[(*jng)++] = (short) levemit.l[ki][k];
	}
      }
    }
  } else if (*ans == 'G') {
    igam = nearest_gamma(x1, y1);
    if (igam < 0) {
      tell("No gamma selected, try again...\n");
      goto START;
    }
    *jng = 1;
    jgam[0] = (short) igam;
  } else if (!strncmp(ans, "X2", 2)) {
    ilev = nearest_level(x1, y1);
    if (ilev < 0) {
      tell("No level selected, try again...\n");
      goto START;
    }
    for (k = 0; k < levemit.n[ilev]; ++k) {
      jgam[(*jng)++] = (short) levemit.l[ilev][k];
    }
  } else if (!strncmp(ans, "X3", 2)) {
    iband = nearest_band(x1, y1);
    if (iband < 0 || iband >= glsgd.nbands) {
      tell("No band selected, try again...\n");
      goto START;
    }
    for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
      if (glsgd.lev[ilev].band == iband) {
	for (k = 0; k < levemit.n[ilev]; ++k) {
	  jgam[(*jng)++] = (short) levemit.l[ilev][k];
	}
      }
    }
  } else {
    tell(" *** Bad entry, try again... ***\n");
    goto START;
  }
  return 0;
} /* get_fit_gam */

/* ======================================================================= */
int listgam(void)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */

  float x, y, eg, dx, fj1, fj2, deg;
  int   igam, iwin, graphics = 0, i, k, ni, nl;
  char  reply, ans[80];

  /* ....define/edit list of gammas */
  while (1) {
    if (!ask(ans, 80, "List name = ? (A to Z, a to z)")) return 0;
    reply = *ans;
    for (nl = 0; nl < 52; ++nl) {
      if (reply == listnam[nl]) break;
    }
    if (nl < 52) break;
  }

  ni = elgd.nlist[nl];
  i = 0;
  while (i < 40) {
    if (graphics) {
      /* get list energies from level scheme or spectrum window */
      if (i < ni) {
	tell("Gamma %c%2.2i? (P for %.1f, T to type energies, X to end)\n",
	     reply, i+1, elgd.list[nl][i]);
      } else {
	tell("Gamma %c%2.2i? (T to type energies, X to end)\n",
	     reply, i+1);
      }
      retic3(&x, &y, ans, &iwin);
      if (*ans == 'X' || *ans == 'x') {
	elgd.nlist[nl] = i;
	return 0;
      }
      if (*ans == 'T' || *ans == 't') {
	graphics = 0;
      } else if (i >= ni || (*ans != 'P' && *ans != 'p')) {
	if (iwin == 2) {
	  if ((igam = nearest_gamma(x, y)) < 0) {
	    tell("  ... no gamma selected, try again...\n");
	    continue;
	  }
	  eg = glsgd.gam[igam].e;
	} else {
	  dx = 0.0f;
	  energy(x - 0.5f, dx, &eg, &deg);
	}
	elgd.list[nl][i++] = eg;
      } else {
	i++;
      }

    } else {
      /* get list energies from text window */
      if (i < ni) {
	k = ask(ans, 80, "Gamma %c%2.2i = ? (1 for %.1f,\n"
		"   G to enter energies from graphics, rtn to end)",
		reply, i+1, elgd.list[nl][i]);
      } else {
	k = ask(ans, 80, "Gamma %c%2.2i = ?\n"
		"  (G to enter energies from graphics, rtn to end)",
		reply, i+1);
      }
      if (k < 1) {
	elgd.nlist[nl] = i;
	return 0;
      }
      if (*ans == 'G' || *ans == 'g') {
	graphics = 1;
      } else if (ffin(ans, k, &eg, &fj1, &fj2)) {
	tell("  ... bad entry, try again...\n");
      } else if (i >= ni || eg != 1.0f) {
	elgd.list[nl][i++] = eg;
      } else {
	i++;
      }
    }
  }
  elgd.nlist[nl] = 40;
  return 0;
} /* listgam */

/* ======================================================================= */
int examine_gamma(void)
{
  float x1, y1, egamma, dx, deg;
  int   igam, iwin, nc;
  char  ans[80], command[80];

  tell("Button 1: set gate on gamma\n"
       "Button 2: add gate on gamma\n"
       "Button 3: show gamma\n"
       "      - : subtract gamma\n"
       "      . : logical-type combine gamma\n"
       "    b/f : back/forward in gate history\n"
       " ...hit X on keyboard to exit...\n");
START:
  igam = 0;
  retic3(&x1, &y1, ans, &iwin);
  if (!strcmp(ans, "X") || !strcmp(ans, "x")) {
    return 0;
  } else if (!strcmp(ans, "b") || !strcmp(ans, "B") ||
	     !strcmp(ans, "u") || !strcmp(ans, "U")) {
    undo_esclev(-1, 4);
    goto START;
  } else if (!strcmp(ans, "f") || !strcmp(ans, "F") ||
	     !strcmp(ans, "r") || !strcmp(ans, "R")) {
    undo_esclev(1, 4);
    goto START;
  }
  if (iwin == 2) {
    igam = nearest_gamma(x1, y1);
    if (igam < 0) {
      tell("  ... no gamma selected, try again...\n");
      goto START;
    }
    egamma = glsgd.gam[igam].e;
    tell("\n*** Egamma = %7.2f +- %4.2f     Igamma = %6.2f +- %4.2f\n",
	 glsgd.gam[igam].e, glsgd.gam[igam].de,
	 glsgd.gam[igam].i, glsgd.gam[igam].di);
  } else if (iwin == 1) {
    dx = 0.0f;
    energy(x1 - 0.5f, dx, &egamma, &deg);
  } else {
    tell("Click in spectrum window for gates on peaks,\n"
	 "   or in level scheme window for gates on gammas.\n");
    goto START;
  }
  sprintf(command, " %.2f", egamma);
  nc = strlen(command);
  if (ans[1] == '1') {
    tell("%s\n", command);
    hilite(-1);
    if (num_gate(command, nc)) goto ERR;
    disp_dat(0);
  } else if (ans[1] == '2') {
    *command = '+';
    tell("%s\n", command);
    if (combine(command, nc)) goto ERR;
    disp_dat(0);
  } else if (ans[1] == '3') {
    *command = 'X';
    tell("%s\n", command);
    if (examine(command, nc)) goto ERR;
  } else if (!strcmp(ans, "minus") || !strncmp(ans, "-", 1)) {
    *command = '-';
    tell("%s\n", command);
    if (combine(command, nc)) goto ERR;
    disp_dat(0);
  } else if (!strcmp(ans, "period") || !strncmp(ans, ".", 1)) {
    *command = '.';
    tell("%s\n", command);
    if (combine(command, nc)) goto ERR;
    disp_dat(0);
  }
  goto START;
 ERR:
  tell("Bad energy...\n");
  goto START;
} /* examine_gamma */

/* ======================================================================= */
int comfil(char *filnam, int nc)
{
  static int cfopen = 0;

  strncpy(filnam, "  ", 2);
  if (nc < 3) {
    /* ask for command file name */
  GETFILNAM:
    infile = 0;
    nc = askfn(filnam, 80, "", ".cmd", "Command file name = ?");
    if (nc == 0) return 0;
  }
  
  setext(filnam, ".cmd", 80);
  if (!strncmp(filnam, "END", 3) || !strncmp(filnam, "end", 3)) {
    /* close command file, lu IR = console */
    if (cfopen || cflog) {
      fclose(cffile);
    } else {
      warn1("Command file not open.\n");
    }
    cfopen = 0;
    cflog = 0;
    infile = 0;
  } else if (!strncmp(filnam, "CON", 3) || !strncmp(filnam, "con", 3)) {
    if (cfopen) {
      infile = cffile;
    } else {
      warn1("Command file not open.\n");
    }
  } else if (!strncmp(filnam, "CHK", 3) || !strncmp(filnam, "chk", 3)) {
    if (cfopen) {
      infile = 0;
      if (!askyn("Proceed? (Y/N)")) return 0;
      infile = cffile;
    } else if (!cflog) {
      warn1("Command file not open.\n");
    }
  } else if (!strncmp(filnam, "ERA", 3) || !strncmp(filnam, "era", 3)) {
    erase();
  } else if (!strncmp(filnam, "LOG", 3) || !strncmp(filnam, "log", 3)) {
    if (cfopen || cflog) fclose(cffile);
    while (1) {
      cfopen = 0;
      cflog = 0;
      infile = 0;
      nc = askfn(filnam, 80, "", ".cmd", "File name for command logging = ?");
      if (nc == 0) return 0;
      if (strncmp(filnam, "END", 3) && strncmp(filnam, "end", 3) &&
	  strncmp(filnam, "CON", 3) && strncmp(filnam, "con", 3) &&
	  strncmp(filnam, "CHK", 3) && strncmp(filnam, "chk", 3) &&
	  strncmp(filnam, "ERA", 3) && strncmp(filnam, "era", 3) &&
	  strncmp(filnam, "LOG", 3) && strncmp(filnam, "log", 3)) break;
      warn1("*** That is an illegal command file name. ***\n");
    }
    /* open log command file for output
       all input from lu IR to be copied to lu ICF */
    cffile = open_new_file(filnam, 1);
    cflog = 1;
  } else {
    /* CF filename
       open command file for input on lu IR */
    if (cfopen || cflog) fclose(cffile);
    cflog = 0;
    if (!(cffile = fopen(filnam, "r+"))) {
      file_error("open", filnam);
      cfopen = 0;
      goto GETFILNAM;
    }
    cfopen = 1;
    infile = cffile;
  }
  return 0;
} /* comfil */
/* ======================================================================= */
int path(int mode)
{
  static int ordd[MAXLEV];
  float toti, a;
  int   flag = 1, temp, i, j, ig, lf, jj, li;

/* This subroutine calculates all gamma gamma coincidence intensities and all
   branching ratios for gamma rays defined in the level scheme file.  A bubble
   sort is performed on the level energies so that the coincidence intensities
   need only be calculated once per level. ordd is an pointer array into 
   levdata ordered (increasing) by energy.  Branching ratios are calculated
   and stored in levelbr. */

  for (i = 0; i < glsgd.nlevels; ++i) {
    for (j = 0; j < glsgd.ngammas; ++j) {
      elgd.levelbr[j][i] = 0.0f;
    }
  }
  if (mode > 0) {
    /* bubble sort on level energies */
    for (i = 0; i < glsgd.nlevels; ++i) ordd[i] = i;
    while (flag) {
      flag = 0;
      for (i = 0; i < glsgd.nlevels - 1; ++i) {
	if (glsgd.lev[ordd[i]].e > glsgd.lev[ordd[i+1]].e) {
	  temp = ordd[i];
	  ordd[i] = ordd[i+1];
	  ordd[i+1] = temp;
	  flag = 1;
	}
      }
    }
  }
  for (i = 1; i < glsgd.nlevels; ++i) {
/* -----------------------------------find total intensity emitted by level */
    li = ordd[i];
    if (levemit.n[li] > 0) {
      toti = 0.0f;
      for (j = 0; j < levemit.n[li]; ++j) {
	toti += glsgd.gam[levemit.l[li][j]].i * 
	       (glsgd.gam[levemit.l[li][j]].a + 1.0f);
      }
/* -----------------------------------calculate branching ratios */
      if (toti != 0.0f) {
	for (j = 0; j < levemit.n[li]; ++j) {
	  ig = levemit.l[li][j];
	  lf = glsgd.gam[ig].lf;
	  elgd.levelbr[ig][li] = glsgd.gam[ig].i / toti;
/* Since levels are ordered by energy all previous levels are up to date with
   regards to branching ratios, simply add them into new levels and multiply by
   original branching ratio. */
	  a = glsgd.gam[ig].i * (glsgd.gam[ig].a + 1.0f) / toti;
	  for (jj = 0; jj < glsgd.ngammas; ++jj) {
	    elgd.levelbr[jj][li] += elgd.levelbr[jj][lf] * a;
	  }
	}
      }
    }
  }
  return 0;
} /* path */

/* ====================================================================== */
int wrlists(char *ans)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */
  float eg;
  int   i, j, nl;
  char  filnam[80];

/* ....write/read gamma-ray lists to/from disk file */
  if (strlen(ans) > 2) {
    strcpy(filnam, ans + 2);
  } else {
  START:
   if (!askfn(filnam, 80, "", ".lis", "List file name = ?")) return 0;
  }
  setext(filnam, ".lis", 80);
  if (*ans == 'W') {
    filez[1] = open_new_file(filnam, 0);
    if (!filez[1]) return 0;
    for (i = 0; i < 52; ++i) {
      if (elgd.nlist[i] > 0) {
	fprintf(filez[1], "%c\n", listnam[i]);
	for (j = 0; j < elgd.nlist[i]; ++j) {
	  fprintf(filez[1], "%7.2f\n", elgd.list[i][j]);
	}
	fprintf(filez[1], "0.0\n");
      }
    }
  } else {
    if (!(filez[1] = open_readonly(filnam))) goto START;
    while (fgets(lx, 120, filez[1])) {
      for (i = 0; i < 52; ++i) {
	if (*lx == listnam[i]) break;
      }
      if ((nl = i) >= 52) continue;
      elgd.nlist[nl] = 0;
      for (i = 0; i < 40 && fgets(lx, 120, filez[1]); ++i) {
	if (1 != sscanf(lx, "%f", &eg)) {
	  warn1("Cannot read file. WARNING: Lists may contain errors\n.");
	  fclose(filez[1]);
	  goto START;
	}
	if (eg == 0.0f) break;
	elgd.list[nl][i] = eg;
	elgd.nlist[nl] = i+1;
      }
    }
  }
  fclose(filez[1]);
  return 0;
} /* wrlists */

/* ======================================================================= */
int report_curs3(int ix, int iy, int iwin)
{
  float  x, y, e, de;
  int    g, l;

  cvxy(&x, &y, &ix, &iy, 2);
  if (iwin == 1) {
    tell(" y = %.0f", y);
    if ((g = nearest_gamma(x, y)) >= 0) {
      tell(" Gamma %.1f, I = %.2f", glsgd.gam[g].e, glsgd.gam[g].i);
    } else {
      tell("                     ");
    }
    if ((l = nearest_level(x, y)) >= 0) {
      tell(" Level %s, E = %.1f   \r", glsgd.lev[l].name, glsgd.lev[l].e);
    } else {
      tell("                               \r");
    }
  } else {
    energy(x - 0.5f, 0.0f, &e, &de);
    tell(" E = %.1f, y = %.0f                                    \r", e, y);
  }
  fflush(stdout);
  return 0;
}

/* ======================================================================= */
int done_report_curs(void)
{
  tell("                                      "
       "                                    \r");
  fflush(stdout);
  return 0;
}
