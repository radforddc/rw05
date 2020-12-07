#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "gls.h"

/* Program to create and edit a level scheme graphically */
/* Version 2.0                  D.C. Radford  Oct  1996 */
/* Version 3.0  (C version)     D.C. Radford  Sept 1999 */

extern int cask(char *, char *, int);
extern int cvxy(float *, float *, int *, int *, int);
extern int initg(int *, int *);
extern int retic(float *, float *, char *);

/* ======================================================================= */
int main(int argc, char **argv)
{
  int nc;
  char ans[80], *input_file_name;

  /* Program to create and edit a level scheme graphically */
  /* Version 2.0                  D.C. Radford  Oct  1996 */
  /* Version 3.0  (C version)     D.C. Radford  Sept 1999 */
  input_file_name = "";
  if (argc > 1) {
    input_file_name = argv[1];
    printf("\nInput file name: %s\n", input_file_name);
  }

  printf("\n\n\n"
         "GLS Version 3.0    D. C. Radford   Sept 1999\n\n"
         "    Welcome....\n\n");
  /* initialise */
  /* read level scheme from .gls file */
  gls_init(input_file_name);

  while (1) {
    if ((nc = cask("Next command?", ans, 80)))  /* get new command */
      gls_exec(ans);  /* decode and execute command */
  }
} /* main */

/* ======================================================================= */
int initg2(int *nx, int *ny)
{
  
  initg(nx, ny);
  return 0;
} /* initg2 */

/* ======================================================================= */
int retic2(float *x, float *y, char *cout)
{
  int iflag;

  while (1) {
    retic(x, y, cout);
    if (strncmp(cout + 2, "-S", 2)) return 0;
    /* shift-button pressed; pan/zoom gls display */
    iflag = 1;
    if (!strncmp(cout, "X2", 2)) iflag = 2;
    if (!strncmp(cout, "X3", 2)) iflag = 3;
    pan_gls(*x, *y, iflag);
    strcpy(cout, "NOBELL");
  }
} /* retic2 */

/* ======================================================================= */
int report_curs(int ix, int iy)
{
  float  x, y;
  int    g, l;

  cvxy(&x, &y, &ix, &iy, 2);
  printf(" y = %.0f", y);
  if ((g = nearest_gamma(x, y)) >= 0) {
    printf(" Gamma %.1f, I = %.2f", glsgd.gam[g].e, glsgd.gam[g].i);
  } else {
    printf("                     ");
  }
  if ((l = nearest_level(x, y)) >= 0) {
    printf(" Level %s, E = %.1f   \r", glsgd.lev[l].name, glsgd.lev[l].e);
  } else {
    printf("                               \r");
  }
  fflush(stdout);
  return 0;
}

/* ======================================================================= */
int done_report_curs(void)
{
  printf("                                      "
	 "                                    \r");
  fflush(stdout);
  return 0;
}
