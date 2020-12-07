/* ***********************************
   program algndiag

   program to manipulate large gamma-gamma matrices
   author:           D.C. Radford
   first written:    April 1991
   latest revision:  Sept 1999
   description:      Prompt for the filenames of a gamma-gamma matrix.
                     Shift the rows up/down and re-store the result
		     back in the matrix file.
   restrictions:     matrices stored as integer*2 numbers

   *********************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(int argc, char **argv)
{
  int   i, j, i0, iy, ix1, ix2;
  short mat[8][4096], mat2[8][4096];
  char  filnam[80];
  FILE  *file;

  printf("\n\n"
	 " ALGNDIAG - a program to align the main diagonal channels\n"
	 "    (Ex=Ey) onto x-channel 2048, by shifting the rows of the\n"
	 "    matrix up or down.\n"
	 "    Useful for 2D quasicontinuum analysis etc.\n"
	 "   D. C. Radford    Sept 1999\n\n");
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      if (!cask("Matrix file = ? (default .ext = .mat)", filnam, 80))
	return 0;
    }
    setext(filnam, ".mat", 80);
    if ((file = fopen(filnam, "r+"))) break;
    file_error("open", filnam);
  }
  for (i0 = 0; i0 < 4089; i0 += 8) {
    /* read matrix segment */
    printf("Row %4i\r", i0); fflush(stdout);
    rmat(file, i0, 8, mat[0]);
    for (i = 0; i < 8; ++i) {
      iy = i0 + i;
      ix1 = 2048 - iy;
      ix2 = ix1 + 4096;
      if (ix1 < 0) ix1 = 0;
      if (ix2 > 4096) ix2 = 4096;
      for (j = 0; j < ix1; ++j) {
	mat2[i][j] = 0;
      }
      for (j = ix1; j < ix2; ++j) {
	mat2[i][j] = mat[i][j+iy-2048];
      }
      for (j = ix2; j < 4096; ++j) {
	mat2[i][j] = 0;
      }
    }
    /* rewrite matrix segment */
    wmat(file, i0, 8, mat2[0]);
  }
  fclose(file);
  return 0;
} /* MAIN */
