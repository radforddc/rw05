#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  float spec[16384], fspec[8], fj=0, fj2=0;
  int   jdum, idim1, idim2, ired1, ired2, compress=1;
  int   i, j, numch, nc, list;
  char  asciifil[80], spe_file[80], lst_fils[80], namesp[8], ans[80] = "";
  char  line[120];
  FILE  *file1, *file2;

  /* **************************************
     PROGRAM SPEC_ASCII
     a file conversion program: .txt to .spe and .spe to .txt files
     author:           D.C. Radford, R.W. MacLeod
     first written:    Oct 1991
     latest revision:  Jan 7 1992
     description:      a file conversion program: .txt to .spe files
     and .spe to .txt files
     file spec_ascii.dat contains list of VALID files
     to be converted
     text files will/must be written as 80-char. records
     restrictions:     input text files must be written as 80-char. records
     format 1X,6I10/8I10/8I10

     July 2005:  DCR:  added support for .asc files
     lines 0 to num_channels-1:
     <channel>,  <counts>  (int, int)
     *************************************** */
  
  /* ask which direction files are to be converted */
  file1 = NULL;
  *ans = '\0';
  while (*ans < '1' || *ans > '4') {
    cask("\n"
	 "Type 1 to convert .spe files to .txt files,\n"
	 "     2 to convert .txt files to .spe files,\n"
	 "     3 to convert .spe files to .asc files,\n"
	 "     4 to convert .asc files to .spe files?", ans, 1);
  }  
  list = caskyn("\n"
		"The files to be converted can be listed in a LIST_OF_FILES\n"
		" file, or listed individually.\n"
		"Do you want to use a LIST_OF_FILES file? (Y/N)");

  if (list) {
    while (1) {
      /* open the "list of files to be converted" file */
      nc = cask("The default .ext is .dat for the file list,\n"
		"   .spe and .txt for spectra.\n"
		"LIST_OF_FILES file = ? (<rtn> for spec_ascii.dat)", lst_fils, 80);
      if (nc == 0) strcpy(lst_fils, "spec_ascii.dat");
      setext(lst_fils, ".dat", 80);
      if ((file1 = open_readonly(lst_fils))) break;
    }
    printf("Using list_of_files in file: %s\n", lst_fils);
  }
  /* get name of file to be converted */
  while (1) {
    if (list) {
      if (!fgets(asciifil, 80, file1)) break;
      nc = strlen(asciifil) - 1;
      if (asciifil[nc] == '\n') asciifil[nc] = '\0';
    } else if (*ans == '1' || *ans == '3') {
      if (!(nc = cask("Name of .spe file to be converted = ?\n"
		      "  (default .ext = .spe, return to end)",
		      asciifil, 80))) break;
    } else if (*ans == '2') {
      if (!(nc = cask("Name of .txt file to be converted = ?\n"
			   "  (default .ext = .txt, return to end)",
			   asciifil, 80))) break;
    } else {
      if (!(nc = cask("Name of .asc file to be converted = ?\n"
			   "  (default .ext = .asc, return to end)",
			   asciifil, 80))) break;
    }
    strcpy(spe_file, asciifil);
    printf("Converting file %s\n", asciifil);

    if (*ans == '1' || *ans == '3') {
      /* convert .spe to .txt or .asc */
      j = setext(spe_file, ".spe", 80);
      if (*ans == '1') {   /* .txt output file */
	strcpy(asciifil + j, ".txt");
      } else {             /* .asc output file */
	strcpy(asciifil + j, ".asc");
      }
      if (readsp(spe_file, spec, namesp, &numch, 16384)) {
	file_error("open or read", spe_file);
	if (list) printf("   ...skipping to next file.\n");
	continue;
      }
      /* open ASCII file */
      file2 = open_new_file(asciifil, 1);
      if (*ans == '1') {   /* .txt output file */
	/* write header info */
	fprintf(file2, " %.8s%10i%10i%10i%10i\n", namesp, numch, 1, 1, 1);
	/* write out spectrum in eight columns */
	for (jdum = 0; jdum < numch; jdum += 8) {
	  for (i = 0; i < 8; ++i) {
	    fprintf(file2, " %9f", spec[i + jdum]);
	  }
	  fprintf(file2, "\n");
	}
      } else {             /* .asc output file */
	for (i = 0; i < numch; ++i) {
          if (fabs(spec[i]) > 100000) {
            fprintf(file2, "%10i,%10i\n", i, (int) rint(spec[i]));
          } else {
            fprintf(file2, "%10i, %9f\n", i, spec[i]);
          }
	}
      }
      fclose(file2);
      /* tell user that the file has been converted */
      printf("  %s ==> %s, %i chs.\n", spe_file, asciifil, numch);

    } else {
      /* convert .txt or .asc to .spe */
      if (*ans == '2') {   /* .txt input file */
	j = setext(asciifil, ".txt", 80);
      } else {             /* .asc input file */
	j = setext(asciifil, ".asc", 80);
      }
      strcpy(spe_file + j, ".spe");
      if (!(file2 = open_readonly(asciifil))) {
	file_error("open", asciifil);
	if (list) printf("   ...skipping to next file.\n");
	continue;
      }
      if (*ans == '2') {   /* .txt input file */
	/* read header info */
	if (!fgets(line, 120, file2) ||
	    sscanf(line, " %8s%i%i%i%i", namesp,
		   &idim1, &idim2,  &ired1,  &ired2) != 5) {
	  file_error("read", asciifil);
	  if (list) printf("   ...skipping to next file.\n");
	  continue;
	}
	numch = idim1 * idim2;
	printf(" %i channels..\n", numch);
	compress = 1;
	if (numch > 16384) {
	  compress = (numch+16383) / 16384;
	  numch /= compress;
	  printf("   ...compressing by factor %d from %d to %d chs.\n",
		 compress, numch*compress, numch);
	}
	for (jdum = 0; jdum < numch; jdum++) spec[jdum] = 0;
	
	/* read counts */
	for (jdum = 0; jdum < numch*compress; jdum += 8) {
	  if (!fgets(line, 120, file2) ||
	      sscanf(line, "%f%f%f%f%f%f%f%f",
		     &fspec[0], &fspec[1], &fspec[2], &fspec[3],
		     &fspec[4], &fspec[5], &fspec[6], &fspec[7]) != 8) {
	    file_error("read", asciifil);
	    if (list) printf("   ...skipping to next file.\n");
	    fclose(file2);
	    continue;
	  }
	  for (i = 0; i < 8; ++i) {
	    spec[(i + jdum)/compress] += fspec[i];
	  }
	}
      } else {             /* .asc input file */
/* 	/\* read header info *\/ */
/* 	if (!fgets(line, 120, file2) || */
/* 	    sscanf(line, "%5s %i", namesp, &idim1) != 2) { */
/* 	  file_error("read", asciifil); */
/* 	  if (list) printf("   ...skipping to next file.\n"); */
/* 	  fclose(file2); */
/* 	  continue; */
/* 	} */
	strncpy(namesp, asciifil, 8);
	/* read channels and counts */
	i = 0;
	while (fgets(line, 120, file2) &&
	       ((sscanf(line, "%i,%f", &j, &fj2) == 2 && j < 16384 && i == j) ||
                (sscanf(line, "%f %f", &fj, &fj2) == 2 && i < 16383))) {
	  spec[i++] = fj2;
          
	}
	tell("%i lines read...\n", i);
	if (i < 4) {
	  file_error("read", asciifil);
	  if (list) printf("   ...skipping to next file.\n");
	  continue;
	}
	numch = idim1 = i;
	idim2 = ired1 = ired2 = 1;
	printf(" %i channels..\n", numch);

      }
      fclose(file2);
      /* write .spe file */
      wspec(spe_file, spec, numch);
      /* tell user that the file has been converted */
      printf("  %s ==> %s, %i chs.\n", asciifil, spe_file, numch);

    }
  }
  if (list) fclose(file1);
  return 0;
} /* spec_ascii */
