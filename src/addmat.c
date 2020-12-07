/* ***********************************
   program addmat

   program to add two large gamma-gamma matrices
   author:           D.C. Radford
   first written:    July 1987
   latest revision:  July 2001
   description:      Prompt for the filenames of two gamma-gamma matrices.
                     Add the two matrices together and store the result
		     in the first file.
   restrictions:     matrices must both be either integer*2 numbers (.mat)
                     or integer*4 numbers (.spn or .m4b)

   *********************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  int   i, j, i0, spnfiletype = 0;
  int   mat4b1[8*4096], mat4b2[8*4096];
#define mat2b1 ((short *)mat4b1)
#define mat2b2 ((short *)mat4b2)
  char  filnam[80], line[120];
  FILE  *file[2];

  /* open matrix files */
  printf("\n\n"
	 " ADDMAT - a program to add two 4k x 4k gamma-gamma matrices.\n"
	 " Matrix 1 is replaced by the sum of matrix 1 and matrix 2.\n"
	 " Matrices may be either short (.mat) or long (.spn or .m4b)\n"
	 "  but they must both be of the same type.\n"
	 " Specify .ext = .spn or .m4b for 4 Bytes/ch.\n\n"
	 "   D. C. Radford    July 2001\n\n");
  for (i = 0; i < 2; ++i) {
    sprintf(line, "Matrix file # %i = ? (default .ext = .mat)", i+1);
    while (1) {
      if (!cask(line, filnam, 80)) {
	if (file[0]) fclose(file[0]);
	return 0;
      }
      /* if necessary, add default .EXT = .mat */
      j = setext(filnam, ".mat", 80);
      if (i == 0) {
	spnfiletype = 0;
	if (!strcmp(filnam+j,".spn") ||
	    !strcmp(filnam+j,".SPN") ||
	    !strcmp(filnam+j,".m4b") ||
	    !strcmp(filnam+j,".M4B")) spnfiletype = 1;
      } else if (!strcmp(filnam+j,".spn") ||
		 !strcmp(filnam+j,".SPN") ||
		 !strcmp(filnam+j,".m4b") ||
		 !strcmp(filnam+j,".M4B")) {
	if (!spnfiletype) {
	  printf("ERROR : both files must be 2 bytes per ch.,\n"
		 "        or 4 bytes per ch.\n"
		 "        You cannot mix types.\n\n");
	  return 1;
	}
      } else if (spnfiletype) {
	printf("ERROR : both files must be 2 bytes per ch.,\n"
	       "        or 4 bytes per ch.\n"
	       "        You cannot mix types.\n\n");
	return 1;
      }

      if ((file[i] = fopen(filnam, "r+"))) break;
      file_error("open", filnam);
    }
  }
  printf("Working...\n");

  if (spnfiletype) {
    for (i0 = 0; i0 < 4089; i0 += 8) {
      /* read matrix segments */
      printf("Row %4i\r", i0); fflush(stdout);
      rmat4b(file[0], i0, 8, mat4b1);
      rmat4b(file[1], i0, 8, mat4b2);
      /* add matrices */
      for (i = 0; i < 8*4096; ++i) {
	mat4b1[i] += mat4b2[i];
      }
      /* rewrite matrix segment */
      wmat4b(file[0], i0, 8, mat4b1);
    }
  } else {
    for (i0 = 0; i0 < 4089; i0 += 16) {
      /* read matrix segments */
      printf("Row %4i\r", i0); fflush(stdout);
      rmat(file[0], i0, 16, mat2b1);
      rmat(file[1], i0, 16, mat2b2);
      /* add matrices */
      for (i = 0; i < 16*4096; ++i) {
	mat2b1[i] += mat2b2[i];
      }
      /* rewrite matrix segment */
      wmat(file[0], i0, 16, mat2b1);
    }
  }
  fclose(file[0]);
  fclose(file[1]);
  return 0;
} /* main */
