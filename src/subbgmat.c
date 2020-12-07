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
  static char *stitle[6] =
  {" X - projection"," Y - projection"," X - background",
   " Y - background"," X - efficiency"," Y - efficiency"};

  float fact, fmax, fsum, test, bspec[6][4096];
  int   i, j, iymax, i0, ii, nch, fourbytes = 0;
  short mat[8][4096];
  int   mat4b[8][4096];
  char  q[80], filnam[80], namesp[8];
  FILE  *file;

  printf("\n\n"
	 "Subbgmat - a program to process 4k x 4k gamma-gamma matrices.\n\n"
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
  for (i = 0; i < 6; ++i) {
    sprintf(q, "%s spectrum file name =?", stitle[i]);
    while (1) {
      cask(q, filnam, 80);
      if (readsp(filnam, bspec[i], namesp, &nch, 4096)) continue;
      if (nch == 4096) break;
      printf("ERROR - spectrum has wrong length.");
    }
  }
  fsum = 0.0f;
  fmax = 0.0f;
  iymax = 0;
  for (i = 0; i < 4096; ++i) {
    fsum += bspec[0][i];
    bspec[1][i] -= bspec[3][i];
    test = bspec[1][i] / bspec[5][i];
    if (test > fmax) {
      fmax = test;
      iymax = i;
    }
  }
  for (i = 2; i < 4; ++i) {
    for (j = 0; j < 4096; ++j) {
      bspec[i][j] /= fsum;
    }
  }
  /* find largest element of new matrix */
  if (fourbytes) {
    rmat4b(file, iymax, 1, mat4b[0]);
    ii = iymax + 1;
    fmax = 0.0f;
    for (j = 0; j < 4096; ++j) {
      test = ((float) mat4b[0][j] -
	      bspec[0][j] * bspec[3][ii] - bspec[1][ii] * bspec[2][j]) /
	      (bspec[4][j] * bspec[5][ii]);
      if (test > fmax) fmax = test;
    }
  } else {
    rmat(file, iymax, 1, mat[0]);
    ii = iymax + 1;
    fmax = 0.0f;
    for (j = 0; j < 4096; ++j) {
      test = ((float) mat[0][j] -
	      bspec[0][j] * bspec[3][ii] - bspec[1][ii] * bspec[2][j]) /
	      (bspec[4][j] * bspec[5][ii]);
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
		  bspec[0][j] * bspec[3][ii] - bspec[1][ii] * bspec[2][j]) /
	          (bspec[4][j] * bspec[5][ii]);
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
		  bspec[0][j] * bspec[3][ii] - bspec[1][ii] * bspec[2][j]) /
	          (bspec[4][j] * bspec[5][ii]);
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
