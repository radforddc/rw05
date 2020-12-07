#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  float a[8];
  int   i, k;
  char  title[256], filnam[80], line[120];
  FILE  *file1, *file2, *file3;
  
  /* D. C. Radford     Slightly revised version     Feb. 1989 */
  /*                   C version                    Sept 1999 */
  printf(
    "\n\n"
    "This program combines .sto files with .sou files to give .sin files.\n"
    "The .sto file is created by the SA command during the gf2/gf3 analysis\n"
    "  of a source calibration spectrum. You must have exactly one line\n"
    "  for eachgamma-ray listed in the corresponding .sou file. If you\n"
    "  choose not to fit a peak, put a blank line in the file.\n"
    "The .sou file contains the energies and intensities of the source\n"
    "  lines. As of Nov. 1985, the following .sou files are available:\n\n"
    "    Am241, Am243, Ba133, Co56, Eu152, Na24, Se75, Ta182 and Y88.\n\n"
    "The .sin file may that this program produces may be used as input\n"
    "  for such programs as encal and/or effit.\n\n"
    "D. C. Radford    Feb. 1989.\n"
    "C version        Sept 1999.\n\n");

  /* ask for input file names */
  while (1) {
    if (!cask("gf2/gf3 .sto file name = ? (default .ext = .sto)", filnam, 80))
      continue;
    setext(filnam, ".sto", 80);
    if (!(file1 = open_readonly(filnam))) continue;
  
    strcpy(title, filnam);
    strcat(title, "                    ");
    while (1) {
      if (!cask("Source data file name = ? (default .EXT = .sou)", filnam, 80))
	continue;
      setext(filnam, ".sou", 80);
      if ((file2 = open_readonly(filnam))) break; 
    }
  
    strcpy(title + 20, filnam);
    strcat(title, "                    ");
    /* open output file */
    while (!cask("Output file name = ? (default .EXT = .sin)", filnam, 80));
    setext(filnam, ".sin", 80);
    file3 = open_new_file(filnam, 1);

    /* ask for and write title */
    k = cask("Title for output file = ? (40 chs max.)", title+40, 40);
    memset(title+40+k, ' ', 40-k);
    fprintf(file3, " %.79s\n", title);

    while (1) {
      a[0] = a[2] = 0.0f;
      if (!fgets(line, 120, file1)) break;
      if (strstr(line, "Centroid")) continue;
      sscanf(line, "%d%f%f%f%f", &i, &a[0], &a[1], &a[2], &a[3]);
      if (!fgets(line, 120, file2)) break;
      sscanf(line, "%f%f%f%f", &a[4], &a[5], &a[6], &a[7]);

      /* if centroid and area are zero, ignore line */
      if (a[0] != 0.0f || a[2] != 0.0f)
	fprintf(file3,
		"%4i %11.4f %11.4f %11.0f %11.0f %11.4f %11.4f %11.0f %11.0f\n",
		i, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    }

    fclose(file1);
    fclose(file2);
    fclose(file3);
    if (!caskyn("Would you like to process more files? (Y/N)")) break;
  }
  return 0;
} /* source */
