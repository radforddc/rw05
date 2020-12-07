/* ***********************************
   program adjmat

   program to adjust the gain of one axis of gamma-gamma matrices
   author:           D.C. Radford
   first written:    July 2003

   *********************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "util.h"

int main(void)
{
  int   i, j, k, i0, spnfiletype = 0, newch, ioh, iol, ist, flag;
  int   mat4b[4096];
#define mat2b ((short *)mat4b)
  char  filnam[80], ans[80];
  float a, b, c, ch, cl, oh, ol;
  float oldch1, oldch2, newch1, newch2, newsp[4096];
  static float d = (float) (RAND_MAX) + 1.0;
  FILE  *file;

  /* open matrix files */
  printf("\n\n"
	 " ADJMAT - a program to adjust the gain of one axis of a\n"
	 " 4k x 4k gamma-gamma matrix.\n"
	 " Specify .ext = .spn or .m4b for 4 Bytes/ch.\n\n"
	 "   D. C. Radford    July 2003\n\n");
  while (1) {
    if (!cask("Matrix file = ? (default .ext = .mat)", filnam, 80)) {
      return 0;
    }
    /* if necessary, add default .EXT = .mat */
    j = setext(filnam, ".mat", 80);
    spnfiletype = 0;
    if (!strcmp(filnam+j,".spn") ||
	!strcmp(filnam+j,".SPN") ||
	!strcmp(filnam+j,".m4b") ||
	!strcmp(filnam+j,".M4B")) spnfiletype = 1;

    if ((file = fopen(filnam, "r+"))) break;
    file_error("open", filnam);
  }

  while (1) {
    k = cask("Oldch1,Oldch2,Newch1,Newch2 = ?", ans, 80);
    if (k == 0) return 0;
    for (i = 1; i < k; ++i) {
      if (ans[i] == ' ') {
	ans[i] = ',';
      }
    }
    strncpy(ans + k, ",", 80-k);
    if (sscanf(ans, "%f,%f,%f,%f",
	       &oldch1, &oldch2, &newch1, &newch2) == 4) break;
  }
  if (newch1 == newch2 ||
      (a = (oldch2 - oldch1) / (newch2 - newch1)) <= 0.f) {
    printf("Error - cannot continue - bad values.\n");
    return 1;
  }
  b = oldch2 - a * newch2;
  ist = (-0.5f - b) / a + 0.5f;
  if (ist < 0) ist = 0;
  ol = ((float) ist - 0.5f) * a + b;
  if (ol < -0.5f) ol = -0.5f;

  printf("Working...\n");

  if (spnfiletype) {
    for (i0 = 0; i0 < 4096; i0++) {
      /* read matrix segment */
      if (!(i0&7)) printf("Row %4i\r", i0); fflush(stdout);
      rmat4b(file, i0, 1, mat4b);
      /* adjust gain */
      for (i = 0; i < 4096; ++i) {
	newsp[i] = 0.0f;
      }
      iol = ol + 0.5f;
      cl = (float) mat4b[iol] * ((float) iol + 0.5f - ol);
      flag = 1;
      for (newch = ist; newch < 4096 && flag; ++newch) {
	oh = ((float) newch + 0.5f) * a + b;
	ioh = oh + 0.5f;
	if (ioh >= 4096) {
	  oh = 4095.5f;
	  ioh = 4095;
	  flag = 0;
	}
	ch = (float) mat4b[ioh] * ((float) ioh + 0.5f - oh);
	while (++iol <= ioh) {
	  cl += (float) mat4b[iol];
	}
	newsp[newch] += (cl - ch);
	cl = ch;
	iol = ioh;
      }
      for (i = 0; i < 4096; ++i) {
	mat4b[i] = newsp[i] + 0.5f;
      }
      /* rewrite matrix segment */
      wmat4b(file, i0, 1, mat4b);
    }
  } else {
    for (i0 = 0; i0 < 4096; i0++) {
      /* read matrix segment */
      if (!(i0&7)) printf("Row %4i\r", i0); fflush(stdout);
      rmat(file, i0, 1, mat2b);
      /* adjust gain */
      for (i = 0; i < 4096; ++i) {
	newsp[i] = 0.0f;
      }
      iol = ol + 0.5f;
      cl = (float) mat2b[iol] * ((float) iol + 0.5f - ol);
      flag = 1;
      for (newch = ist; newch < 4096 && flag; ++newch) {
	oh = ((float) newch + 0.5f) * a + b;
	ioh = oh + 0.5f;
	if (ioh >= 4096) {
	  oh = 4095.5f;
	  ioh = 4095;
	  flag = 0;
	} 
	/* if (i0<50) printf("%d %d %d %d %f %f\n",
	   i0, newch, iol, ioh, ol, oh); */
	ch = (float) mat2b[ioh] * ((float) ioh + 0.5f - oh);
	while (++iol <= ioh) {
	  cl += (float) mat2b[iol];
	}
	newsp[newch] += (cl - ch);
	cl = ch;
	iol = ioh;
      }
      for (i = 0; i < 4096; ++i) {
	c = ((float) rand())/d;
	mat2b[i] = newsp[i] + c;
      }
     /* rewrite matrix segment */
      wmat(file, i0, 1, mat2b);
    }
  }

  fclose(file);
  return 0;
} /* main */
