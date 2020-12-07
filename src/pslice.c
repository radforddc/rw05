/* program to slice matrices perpendicular and parallel to diagonal
   D.C. Radford                   Jan. 1987
   translated from fortran to C   Sept 1999
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

union {
  short matseg[16][4096];
  int   lmatseg[16][4096];
} mat;
int   longmat = 0;
FILE  *matfile;

/* ======================================================================= */
int main(int argc, char **argv)
{
  int  takpar(void), takper(void);
  int  j;
  char matnam[80], filnam[80], line[256];

  strcpy(matnam, "mat1.mat");
  printf("\n\n"
	 " PSLICE - a program to project 4k by 4k gamma-gamma matrices\n"
	 "   perpendicular and/or parallel to the diagonal\n\n"
	 " This program now accepts matrices in both .mat (2 Bytes/ch)\n"
	 "    and .spn or .m4b (4 Bytes/ch) formats.\n"
	 " Specify .ext = .spn or .m4b for 4 Bytes/ch.\n\n"
	 "    The default extension for all spectrum files is .spe\n\n"
	 "    D. C. Radford    Sept 1999\n\n");

  /* open matrix file */
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      sprintf(line,
	      "Matrix file name = ? (return for %s,\n"
	      "                      E to end,\n"
	      "                      default .ext = .mat)", matnam);
      if (!cask(line, filnam, 80)) strcpy(filnam, matnam);
      if (!strcmp(filnam, "E") || !strcmp(filnam, "e")) return 0;
    }

    j = setext(filnam, ".mat", 80);
    longmat = 0;
    if (!strcmp(filnam + j, ".spn") || !strcmp(filnam + j, ".SPN") ||
	!strcmp(filnam + j, ".m4b") || !strcmp(filnam + j, ".M4B"))
      longmat = 1;
    if (!(matfile = open_readonly(filnam))) continue;
    strcpy(matnam, filnam);

    /* ask if perpendicular projections required */
    if (caskyn("Take perpendicular projections? (Y/N)")) takper();
    /* ask if parallel projections required */
    if (caskyn("Take parallel projections? (Y/N)")) takpar();
    /* close matrix file and ask for next one */
    fclose(matfile);
  }
} /* main */

/* ======================================================================= */
int takper(void)
{
  float spec[4096];
  int   proj[4096];
  int   idlo, i, j, k, j1, j2, jf, ji, iy, icf, ich, ici;
  char  pronam[80], ans[80];

  while (1) {
    while (1) {
      k = cask("Initial channel (X=Y) = ? (rtn to end)", ans, 80);
      if (k < 1) return 0;
      if (!inin(ans, k, &ici, &j1, &j2) &&
	  ici >= 0 &&
	  ici < 4096) break;
      printf("...Sorry, must be a number between 0 and 4095, try gain...\n");
    }
    while (1) {
      k = cask("Final channel (X=Y) = ? (rtn to end)", ans, 80);
      if (k < 1) return 0;
      if (!inin(ans, k, &icf, &j1, &j2) &&
	  icf >= ici &&
	  icf < 4096) break;
      printf("...Sorry, must be a number between %d and 4095, try again...\n",
	     ici);
    }
    ici *= 2;
    icf *= 2;
    for (i = 0; i < 4096; ++i) {
      proj[i] = 0;
    }
    /* read matrix and compute projection */
    printf("Working...\n\n");
    for (idlo = 0; idlo < 4080; idlo += 16) {
      if (idlo*2 + 2030 < ici) continue;
      printf("Row %4i\r", idlo); fflush(stdout);
      if (longmat) {
	rmat4b(matfile, idlo, 16, mat.lmatseg[0]);
      } else {
	rmat(matfile, idlo, 16, mat.matseg[0]);
      }
      for (i = 0; i < 16; ++i) {
	iy = idlo + i;
	if (iy*2 + 2000 < ici) continue;
	if (iy*2 - 2000 > icf) goto DONE;
	ji = ici - iy;
	if (ji < iy - 2000) ji = iy - 2000;
	if (ji < 0) ji = 0;
	jf = icf - iy;
	if (jf > iy + 2000) jf = iy + 2000;
	if (jf > 4096) jf = 4096;
	if (longmat) {
	  for (j = ji; j < jf; ++j) {
	    ich = j + 2048 - iy;
	    proj[ich] += mat.lmatseg[i][j];
	  }
	} else {
	  for (j = ji; j < jf; ++j) {
	    ich = j + 2048 - iy;
	    proj[ich] += mat.matseg[i][j];
	  }
	}
      }
    }
    /* write out projection */
  DONE:
    cask("Spectrum file name = ?", pronam, 80);
    for (i = 0; i < 4096; ++i) {
      spec[i] = (float) proj[i];
    }
    wspec(pronam, spec, 4096);
  }
} /* takper */

/* ======================================================================= */
int takpar(void)
{
  float spec[4096];
  int   proj[4096];
  int   idlo, i, k, j1, j2, iy, ilo, ips, isw;
  int   ponm=0, ponx=0, pony=0;
  char  pronam[80], ans[80];

  while (1) {
    k = cask("Type X/Y/M to project on Ex>Ey / Ey<Ex / (Ex+Ey)/2", ans, 1);
    if (*ans == 'X' || *ans == 'x') {
	ponx = 1;
	break;
    } else if (*ans == 'Y' || *ans == 'y') {
	pony = 1;
	break;
    } else if (*ans == 'M' || *ans == 'm') {
	ponm = 1;
	break;
    }
  }

  while (1) {
    while (1) {
      k = cask("Peak separation in channels = ? (rtn to end)", ans, 80);
      if (k < 1) return 0;
      if (!inin(ans, k, &ips, &j1, &j2) &&
	  ips >= 0 &&
	  ips < 2048) break;
      printf("...Sorry, must be a number between 0 and 2047, try gain...\n");
    }
    while (1) {
      k = cask("Slice width (chs) = ? (rtn to end)", ans, 80);
      if (k < 1) return 0;
      if (!inin(ans, k, &isw, &j1, &j2) &&
	  isw > 0) break;
      printf("...Sorry, must be a number greater than 0, try gain...\n");
    }
    for (i = 0; i < 4096; ++i) {
      proj[i] = 0;
    }
    /* read matrix and compute projection */
    printf("Working...\n\n");
    for (idlo = 0; idlo < 4080; idlo += 16) {
      printf("Row %4i\r", idlo); fflush(stdout);
      if (longmat) {
	rmat4b(matfile, idlo, 16, mat.lmatseg[0]);
      } else {
	rmat(matfile, idlo, 16, mat.matseg[0]);
      }
      for (i = 0; i < 16; ++i) {
	iy = idlo + i;
	if (iy + ips > 4095 - isw) continue;
	ilo = iy + ips - isw/2;
	if (longmat) {
	  for (k = ilo; k < ilo + isw; ++k) {
	    if (ponx) proj[k]  += mat.lmatseg[i][k];
	    if (pony) proj[iy] += mat.lmatseg[i][k];
	    if (ponm) proj[(iy + k)/2] += mat.lmatseg[i][k];
	  }
	} else {
	  for (k = ilo; k < ilo + isw; ++k) {
	    if (ponx) proj[k]  += mat.matseg[i][k];
	    if (pony) proj[iy] += mat.matseg[i][k];
	    if (ponm) proj[(iy + k)/2] += mat.matseg[i][k];
	  }
	}
      }
    }
    /* write out projection */
    cask("Spectrum file name = ?", pronam, 80);
    for (i = 0; i < 4096; ++i) {
      spec[i] = (float) proj[i];
    }
    wspec(pronam, spec, 4096);
  }
} /* takpar */
