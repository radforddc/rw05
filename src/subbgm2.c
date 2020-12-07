/* program to process 4k x 4k gamma-gamma matrices
   it will multiply by a constant
   ...subtract background
   ...and divide by efficiency
   D.C. Radford      July 1987
   translated to C   Sept 1999
   added 4-byte (.spn/.m4k) support  Dec 2002
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

/* ======================================================================= */
int main(int argc, char **argv)
{
  static char *stitle[5] =
  {" 1st projection","  E2 projection"," 1st background",
   "  E2 background","     Efficiency"};

  float fact, fmax, test, fsum1, fsum2, fsum3;
  float fj1, fj2, factor1, factor2, bspec[10][4096];
  int   ishi, islo, i, j, k, iymax, i0, ii, nch, fourbytes = 0;
  short mat[8][4096];
  int   mat4b[8][4096];
  char  q[256], filnam[80], ans[80], namesp[8];
  FILE  *file, *winfile;

  printf("\n\n"
	 "Subbgm2 - a program to process 4k x 4k gamma-gamma matrices.\n\n"
	 " It will multiply by a constant\n"
	 "    ...subtract background\n"
	 "    ...and divide by efficiency.\n\n"
	 " C version   D. C. Radford    Sept 1999\n\n");
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      cask("Matrix file = ? (default .ext = .mat)\n"
	   "                (specify .spn or .m4k for 4 bytes/ch)", filnam, 80);
    }
    j = setext(filnam, ".mat", 80);
    if ((file = fopen(filnam, "r+"))) break;
    file_error("open", filnam);
  }

  if (strstr(filnam+j, "spn") || strstr(filnam+j, "SPN") ||
      strstr(filnam+j, "m4k") || strstr(filnam+j, "M4K")) fourbytes = 1;
  for (i = 0; i < 5; ++i) {
    sprintf(q, "%s spectrum file name =?", stitle[i]);
    while (1) {
      cask(q, filnam, 80);
      if (readsp(filnam, bspec[i], namesp, &nch, 4096)) continue;
      if (nch == 4096) break;
      printf("ERROR - spectrum has wrong length.");
    }
  }
  fsum1 = 0.0f;
  fsum2 = 0.0f;
  fmax = 0.0f;
  iymax = 0;
  for (i = 0; i < 4096; ++i) {
    fsum1 += bspec[0][i];
    fsum2 += bspec[1][i];
    bspec[6][i] = bspec[0][i] - bspec[2][i];
    test = bspec[6][i] / bspec[4][i];
    if (test > fmax) {
      fmax = test;
      iymax = i;
    }
  }
  while (1) {
    factor1 = fsum1 / fsum2;
    sprintf(q, "Default value for factor 1 is %f\n"
	       " Factor 1 = ? (rtn for default)", factor1);
    if (!(k = cask(q, ans, 80)) ||
	!ffin(ans, k, &factor1, &fj1, &fj2)) break;
  }
  for (j = 0; j < 4096; ++j) {
    bspec[7][j] = bspec[1][j] - bspec[0][j] / factor1;
    bspec[8][j] = bspec[3][j] - bspec[2][j] / factor1;
    bspec[9][j] = bspec[7][j] - bspec[8][j];
    bspec[2][j] /= fsum1;
  }

  while (1) {
    k = cask(" E2 gate file = ? (default .EXT = .win)", ans, 80);
    setext(ans, ".win", 80);
    if ((winfile = open_readonly(ans))) break;
  }
  fsum3 = 0.0f;
  while (fgets(q, 120, winfile) &&
	 sscanf(q, "%*5c%d%*3c%d", &islo, &ishi) == 2) {
    if (islo < 0) islo = 0;
    if (ishi > 4095) ishi = 4095;
    if (islo > ishi) {
      i = ishi;
      ishi = islo;
      islo = i;
    }
    if (islo < 0) islo = 0;
    if (ishi > 4095) ishi = 4095;
    for (i = islo; i <= ishi; ++i) {
      fsum3 += bspec[7][i];
    }
  }
  fclose(winfile);

  while (1) {
    factor2 = fsum3;
    sprintf(q, "Default value for factor 2 is %f\n"
	       " Factor 2 = ? (rtn for default)", factor2);
    if (!(k = cask(q, ans, 80)) ||
	!ffin(ans, k, &factor2, &fj1, &fj2)) break;
  }
  for (j = 0; j < 4096; ++j) {
    bspec[8][j] /= factor2;
  }

  /* find largest element of new matrix */
  if (fourbytes) {
    rmat4b(file, iymax, 1, mat4b[0]);
    ii = iymax + 1;
    fmax = 0.f;
    for (j = 1; j <= 4096; ++j) {
      test = ((float) mat4b[0][j] -
	      bspec[0][j] * bspec[2][ii] - bspec[6][ii] * bspec[2][j] -
	      bspec[7][j] * bspec[8][ii] - bspec[9][ii] * bspec[8][j]) /
	      (bspec[4][j] * bspec[4][ii]);
      if (test > fmax) fmax = test;
    }
  } else {
    rmat(file, iymax, 1, mat[0]);
    ii = iymax + 1;
    fmax = 0.f;
    for (j = 1; j <= 4096; ++j) {
      test = ((float) mat[0][j] -
	      bspec[0][j] * bspec[2][ii] - bspec[6][ii] * bspec[2][j] -
	      bspec[7][j] * bspec[8][ii] - bspec[9][ii] * bspec[8][j]) /
	      (bspec[4][j] * bspec[4][ii]);
      if (test > fmax) fmax = test;
    }
  }

  /* subtract background and divide by efficiency
     set largest element of matrix to 25000 counts */
  fact = 2.5e4f / fmax;
  printf("\nScaling factor = %.3e\n", fact);
  for (i0 = 0; i0 < 4096; i0 += 8) {
    printf("Row %4i\r", i0); fflush(stdout);
    if (fourbytes) {
      /* read matrix segment */
      rmat4b(file, i0, 8, mat4b[0]);
      /* subtract background and divide */
      for (i = 0; i < 8; ++i) {
	ii = i0 + i;
	for (j = 0; j < 4096; ++j) {
	  test = ((float) mat4b[i][j] -
		  bspec[0][j] * bspec[2][ii] - bspec[6][ii] * bspec[2][j] -
		  bspec[7][j] * bspec[8][ii] - bspec[9][ii] * bspec[8][j]) /
	          (bspec[4][j] * bspec[4][ii]);
	  mat4b[i][j] = rint(test * fact);
	}
      }
      /* rewrite matrix segment */
      wmat4b(file, i0, 8, mat4b[0]);
    } else {
      /* read matrix segment */
      rmat(file, i0, 8, mat[0]);
      /* subtract background and divide */
      for (i = 0; i < 8; ++i) {
	ii = i0 + i;
	for (j = 0; j < 4096; ++j) {
	  test = ((float) mat[i][j] -
		  bspec[0][j] * bspec[2][ii] - bspec[6][ii] * bspec[2][j] -
		  bspec[7][j] * bspec[8][ii] - bspec[9][ii] * bspec[8][j]) /
	          (bspec[4][j] * bspec[4][ii]);
	  mat[i][j] = (short) rint(test * fact);
	}
      }
      /* rewrite matrix segment */
      wmat(file, i0, 8, mat[0]);
    }
  }
  fclose(file);
  return 0;
} /* main */
