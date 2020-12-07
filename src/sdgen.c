#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  char list[52] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  float e, s, e0, s1, s2, s3, sj1, sj2;
  int   i, i1, i2, j, j1, j2, nc;
  char  ans[80];
  FILE  *file1;

  while (1) {
    cask("Output list file = ? (default .ext = .lis)", ans, 80);
    setext(ans, ".lis", 80);
    file1 = open_new_file(ans, 1);

    while (!(nc = cask("First gamma energy (keV) = ?", ans, 80)) ||
	   ffin(ans, nc, &e0, &sj1, &sj2));
    while (!(nc = cask("Starting spacing (keV) = ?", ans, 80)) ||
	   ffin(ans, nc, &s1, &sj1, &sj2));
    while (!(nc = cask("Spacing increment (keV) = ?", ans, 80)) ||
	   ffin(ans, nc, &s2, &sj1, &sj2));
    while (!(nc = cask("Stretch per transition (keV) = ?", ans, 80)) ||
	   ffin(ans, nc, &s3, &sj1, &sj2));

    while (1) {
      nc = cask("Number of steps = ?", ans, 80);
      if (inin(ans, nc, &i1, &j1, &j2)) continue;
      if (i1 <= 52) break;
      printf("ERROR - max is 52.\n");
    }

    while (!(nc = cask("Number of gammas in test band = ?", ans, 80)) ||
	   inin(ans, nc, &i2, &j1, &j2));

    for (i = 0; i < i1; ++i) {
      s = s1 + (float) (i) * s2;
      fprintf(file1, "%c\n", list[i]);
      e = e0;
      for (j = 0; j < i2; ++j) {
	fprintf(file1, "%.1f\n", e);
	e += s;
	s += s3;
      }
      fprintf(file1, "0.0\n");
    }
    fclose(file1);
    if (!caskyn("...More? (Y/N)")) return 0;
  }
} /* sdgen */
