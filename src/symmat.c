/* program to symmetrize 4k x 4k gamma-gamma matrices
   D.C. Radford      July 1987
   translated to C   Sept 1999
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
  int mat4b[256*4096], mat4b1[32*4096];
#define mat2b  ((short *)mat4b)
#define mat2b1 ((short *)mat4b1)
#define RERR    { file_error("read", filnam); return 1; }

  int  i, j, i0, i1;
  char filnam[80];
  FILE *file;

  printf("\n\n"
	 "Symmat - a program to symmetrize 4k x 4k  gamma-gamma matrices.\n"
	 " This program now accepts matrices in both .mat (2 Bytes/ch)\n"
	 "    and .spn or .m4b (4 Bytes/ch) formats.\n"
	 " Specify .ext = .spn or .m4b for 4 Bytes/ch.\n\n"
	 " C version   D. C. Radford    Sept 1999\n\n");
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      cask("Matrix file = ? (default .ext = .mat)", filnam, 80);
    }
    j = setext(filnam, ".mat", 80);
    if ((file = fopen(filnam, "r+"))) break;
    file_error("open", filnam);
  }

  if (!strcmp(filnam + j, ".spn") || !strcmp(filnam + j, ".SPN") ||
      !strcmp(filnam + j, ".m4b") || !strcmp(filnam + j, ".M4B")) {
    /* integer*4 data */
    for (i0 = 0; i0 < 4096; i0 += 256) {
      printf("%i/16 done...\r", i0/256); fflush(stdout);
      if (rmat4b(file, i0, 256, mat4b)) RERR;
      for (i = 0; i < 256; ++i) {
	for (j = i; j < 256; ++j) {
	  mat4b[i0 + i + (j<<12)] += mat4b[i0 + j + (i<<12)];
	  mat4b[i0 + j + (i<<12)] =  mat4b[i0 + i + (j<<12)];
	}
      }
      if (i0 > 0) {
	for (i1 = 0; i1 < i0; i1 += 32) {
	  if (rmat4b(file, i1, 32, mat4b1)) RERR;
	  for (i = 0; i < 32; ++i) {
	    for (j = 0; j < 256; ++j) {
	      mat4b[i1 + i + (j<<12)] = mat4b1[i0 + j + (i<<12)];
	    }
	  }
	}
      }
      if (i0 < 3840) {
	for (i1 = i0 + 256; i1 < 4096; i1 += 32) {
	  if (rmat4b(file, i1, 32, mat4b1)) RERR;
	  for (i = 0; i < 32; ++i) {
	    for (j = 0; j < 256; ++j) {
	      mat4b[i1 + i + (j<<12)] += mat4b1[i0 + j + (i<<12)];
	    }
	  }
	}
      }
      wmat4b(file, i0, 256, mat4b);
    }
  } else {
    /* integer*2 data */
    for (i0 = 0; i0 < 4096; i0 += 512) {
      printf("%i/8 done...\r", i0/512); fflush(stdout);
      if (rmat(file, i0, 512, mat2b)) RERR;
      for (i = 0; i < 512; ++i) {
	for (j = i; j < 512; ++j) {
	  mat2b[i0 + i + (j<<12)] += mat2b[i0 + j + (i<<12)];
	  mat2b[i0 + j + (i<<12)] = mat2b[i0 + i + (j<<12)];
	}
      }
      if (i0 > 0) {
	i1 = i0 - 64;
	for (i1 = 0; i1 < i0; i1 += 64) {
	  if (rmat(file, i1, 64, mat2b1)) RERR;
	  for (i = 0; i < 64; ++i) {
	    for (j = 0; j < 512; ++j) {
	      mat2b[i1 + i + (j<<12)] = mat2b1[i0 + j + (i<<12)];
	    }
	  }
	}
      }
      if (i0 < 3584) {
	for (i1 = i0 + 512; i1 < 4096; i1 += 64) {
	  if (rmat(file, i1, 64, mat2b1)) RERR;
	  for (i = 0; i < 64; ++i) {
	    for (j = 0; j < 512; ++j) {
	      mat2b[i1 + i + (j<<12)] += mat2b1[i0 + j + (i<<12)];
	    }
	  }
	}
      }
      wmat(file, i0, 512, mat2b);
    }
  }
  return 0;
} /* main */
