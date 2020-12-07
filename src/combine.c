#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  float area[50], area1[50], darea[50], c1[50], e1[50];
  float a, c[50], e[50], darea1[50], dc[50], de[50], dc1[50], de1[50];
  int   i, j[50], j1[50], j2, nc, nlines;
  char  line[120], nf[80], nam[50][30], nam1[50][30];
  FILE  *file1, *file2;
#define IFMT "%i%f%f%f%f%f%f    %30s"
#define OFMT "%3i %10.4f %8.4f %10.0f %8.0f %10.4f %8.4f    %.30s\n"

  /* COMBINE - a program to combine data for one or more consecutive
     peaks in a gf3 .sto file */
  printf(" \n\n"
	 " Welcome to COMBINE.\n"
	 " This program combines the data for one or more consecutive\n"
	 "  peaks in .sto files from gf3. It does this in such\n"
	 "  a way that they will then appear to be a single peak.\n"
	 " Useful for example for rdm, rdmfit\n"
	 "   where you had to fit wide peaks with multiple gaussians.\n\n"
	 "         D. C. Radford    Feb. 1989.\n");
  /* ask for data file name */
  while (1) {
    cask("gf2/gf3 .sto file = ? (default .ext = .sto)", nf, 80);
    setext(nf, ".sto", 80);
    if (!(file1 = open_readonly(nf))) continue;
  
    /* ask for output file name */
    cask("Output file name = ? (default .ext = .com)", nf, 80);
    setext(nf, ".com", 80);
    file2 = open_new_file(nf, 1);
    fprintf(file2,
	    " No.  Centroid +- error       Area +- error"
	    "     Energy +- error    Sp.name\n");

    while (!(nc = cask("Number of lines per peak = ?", nf, 80)) ||
	   inin(nf, nc, &nlines, j1, &j2));  
    if (nlines < 1) nlines = 1;  
    if (nlines > 50) {
      printf("Maximum is 50.\n");
      goto NEXTFIL;
    }

    i = 0;
    while (i < nlines) {
      if (!fgets(line, 120, file1)) goto NEXTFIL;
      if (strstr(line, "Centroid") ||
	  sscanf(line, IFMT, &j[i], &c[i], &dc[i],
		 &area[i], &darea[i], &e[i], &de[i], nam[i]) != 8 ||
	  c[i] == 0.0f)  continue;      /* if centroid == 0.0 ignore line */
      else if (dc[i] == 0.0f) {
	printf("Errors must be nonzero; zero values set to 1.0 chs\n");
	dc[i] = 1.0f;
      }
      i++;
    }

    while (1) {
      i = 0;
      while (i < nlines) {
	if (!fgets(line, 120, file1)) goto ATEOF;
	if (strstr(line, "Centroid") ||
	    sscanf(line, IFMT, &j1[i], &c1[i], &dc1[i],
		   &area1[i], &darea1[i], &e1[i], &de1[i], nam1[i]) != 8 ||
	    c1[i] == 0.0f)  continue;   /* if centroid == 0.0 ignore line */
	else if (dc1[i] == 0.0f) {
	  printf("Errors must be nonzero; zero values set to 1.0 chs\n");
	  dc1[i] = 1.0f;
	}
	i++;
      }

      printf(" Centroids: %.2f %.2f   Energies: %.2f %.2f\n",
	     c[0], c1[0], e[0], e1[0]);

      if (caskyn(" -- Combine? (Y/N)")) {
	for (i = 0; i < nlines; ++i) {
	  a = area[i] + area1[i];
	  darea[i] = sqrt(darea[i]*darea[i] + darea1[i]*darea1[i]);
	  c[i] = (area[i] * c[i] + area1[i] * c1[i]) / a;
	  e[i] = (area[i] * e[i] + area1[i] * e1[i]) / a;
	  area[i] = a;
	}
      } else {
	for (i = 0; i < nlines; ++i) {
	  fprintf(file2, OFMT, j[i], c[i], dc[i],
		  area[i], darea[i], e[i], de[i], nam[i]);
	  j[i] = j1[i];
	  c[i] = c1[i];
	  dc[i] = dc1[i];
	  area[i] = area1[i];
	  darea[i] = darea1[i];
	  e[i] = e1[i];
	  de[i] = de1[i];
	  strncpy(nam[i], nam1[i], 30);
	}
      }
    }

  ATEOF:
    for (i = 0; i < nlines; ++i) {
      fprintf(file2, OFMT,
	      j[i], c[i], dc[i], area[i], darea[i], e[i], de[i], nam[i]);
    }

  NEXTFIL:
    fclose(file1);
    fclose(file2);
    if (!caskyn("Would you like to process more files? (Y/N)")) break;
  }
  return 0;
} /* combine */
