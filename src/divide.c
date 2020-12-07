#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  float area, darea, c, dc, eg, deg, a[50], deff, fj1, fj2;
  int   i, j, j1, j2, nc, nt;
  char  nam[30], ans[80], nf[80], line[120];
  FILE  *file1, *file2;
#define IFMT "%i%f%f%f%f%f%f    %30s"
#define OFMT "%3i %10.4f %8.4f %10.0f %8.0f %10.4f %8.4f    %.30s\n"
  
  /* DIVIDE - a program to divide output files from energy
     by total intensities to give normalized intensities */
  printf("\n\n"
	 "This program divides output files from the program energy\n"
	 " by total intensities to give normalized relative intensities\n\n"
	 "D. C. Radford        Sept 1999\n\n");

  /* ask for data file name */
  while (1) {
    cask(" Input file name = ? (default .EXT = .eng)", nf, 80);
    setext(nf, ".eng", 80);
    if (!(file1 = open_readonly(nf))) continue;
  
    /* ask for output file name */
    cask("Output file name = ? (default .EXT = .div)", nf, 80);
    setext(nf, ".div", 80);
    file2 = open_new_file(nf, 1);

    while (!(nc = cask("Number of lines per peak = ?", ans, 80)) ||
	   inin(ans, nc, &nt, &j1, &j2));

    printf("Enter the %i dividing values, one per line...\n", nt);
    i = 0;
    while (i < nt) {
      nc = cask("...value = ?", ans, 80);
      if (ffin(ans, nc, &a[i], &fj1, &fj2) || a[i] <= 0.0f) {
	printf("ERROR - bad entry, try again...\n");
      } else {
	i++;
      }
    }
  
    while (!(nc = cask("Percentage error on dividing values = ?", ans, 80)) ||
	   ffin(ans, nc, &deff, &fj1, &fj2));
    deff /= 100.f;
    if (fgets(line, 120, file1)) fputs(line, file2);

    i = 0;
    while (fgets(line, 120, file1)) {
      if (sscanf(line, IFMT, &j, &c, &dc, &area,
		  &darea, &eg, &deg, nam) != 8 ||
	  c == 0.0f) continue;   /* if centroid == 0.0 ignore line */
      area /= a[i];
      darea /= a[i];
      darea = sqrt(darea*darea + area*deff * area*deff);
      fprintf(file2, OFMT,
	      j, c, dc, area, darea, eg, deg, nam);
      if (++i >= nt) {
	i = 0;
	if (nt > 1) {
	  if (fgets(line, 120, file1)) fputs(line, file2);
	  if (fgets(line, 120, file1)) fputs(line, file2);
	}
      }
    }

    fclose(file1);
    fclose(file2);
    if (!caskyn("Would you like to process more files? (Y/N)")) break;
  }
  return 0;
} /* divide */
