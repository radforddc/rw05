/* ***********************************
   program calib_ascii

   author:           D.C. Radford
   first written:    Aug 1991
   latest revision:  Sept 1999
   description:      Utility for efficiency and energy calibration
                     file conversion to/from ASCII
                     Accepts file extensions .eff, .cal, .aef and .aca

   *********************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

/* ======================================================================= */
int main(void)
{
  double gain[6];
  float  pars[10];
  int    iext, iorder;
  char   ans[80], title[80], fn[80], q[256];
  FILE  *file;

  printf("\n\n"
	 "Utility for conversion  of unformatted efficiency and\n"
	 "   energy calibration files to ASCII.\n"
	 "   Accepts input file extensions .eff and .cal.\n\n"
	 "   D.C. Radford   Sept 1999.\n\n");

  /* ask for name of input file */
  while (cask("Input file = ? (rtn to end)", ans, 80)) {
    iext = setext(ans, " ", 80);

    if (!strcmp(ans + iext, ".EFF") || !strcmp(ans + iext, ".eff")) {
      printf("\n");
      /* read parameters from disk file */
      if (read_eff_file(ans, title, pars)) continue;
      printf(" %14.7E %14.7E %14.7E %14.7E %14.7E\n"
	     " %14.7E %14.7E %14.7E %14.7E %14.7E\n\n",
	      pars[0], pars[1], pars[2], pars[3], pars[4],
	      pars[5], pars[6], pars[7], pars[8], pars[9]);

      /* ask for name of output file */
      strcpy(fn, ans);
      strcpy(fn +iext, ".aef");
      sprintf(q,
	      "Output ASCII file = ?\n"
	      "        (default .ext = .aef)\n"
	      "        (return for %s)", fn);
      if (!cask(q, ans, 80)) strcpy(ans, fn);

      /* write parameters to disk file */
      iext = setext(ans, ".aef", 80);
      file = open_new_file(ans, 1);
      fprintf(file,
	      "%.40s\n"
	      " %14.7E %14.7E %14.7E %14.7E %14.7E\n"
	      " %14.7E %14.7E %14.7E %14.7E %14.7E\n",
	      title, pars[0], pars[1], pars[2], pars[3], pars[4],
	      pars[5], pars[6], pars[7], pars[8], pars[9]);
      fclose(file);

    } else if (!strcmp(ans + iext, ".CAL") || !strcmp(ans + iext, ".cal")) {
      printf("\n");
      /* read parameters from disk file */
      if (read_cal_file(ans, title, &iorder, gain)) continue;

      printf("%5d %18.11E %18.11E %18.11E\n"
	     "      %18.11E %18.11E %18.11E\n\n", iorder,
	     gain[0], gain[1], gain[2], gain[3], gain[4], gain[5]);
      /* ask for name of output file */
      strcpy(fn, ans);
      strcpy(fn + iext, ".aca");
      sprintf(q,
	      "Output ASCII file = ?\n"
	      "        (default .ext = .aca)\n"
	      "        (return for %s)", fn);
      if (!cask(q, ans, 80)) strcpy(ans, fn);

      /* write parameters to disk file */
      iext = setext(ans, ".aca", 80);
      file = open_new_file(ans, 1);
      fprintf(file,"  ENCAL OUTPUT FILE\n%.80s\n",title);
      fprintf(file, "%5d %18.11E %18.11E %18.11E\n"
	      "      %18.11E %18.11E %18.11E\n", iorder,
	      gain[0], gain[1], gain[2], gain[3], gain[4], gain[5]);
      fclose(file);
    } else {
      printf("File %s has unrecognized extension.\n", ans);
    }
  }
  return 0;
} /* main */
