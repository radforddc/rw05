/* program to create a look-up table mapping </= 16384-ch. ADC channels
   to contracted channels which are proportional in width to the
   resolution (FWHM)
   i.e. constant-FWHM nonlinear gains
   intended to be used in the replay of triples data to ggg cubes
   D.C. Radford        May 1992
   Modified for better ease of use with contraction factors != 2
                       Aug. 1993
   translated to C     Sept 1999
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"
/* int read_tab_file(char *infilnam, int *outnclook, int *outlookmin,
   int *outlookmax, short *outlooktab, int inmaxchs);
   returns 1 for error */

/* ======================================================================= */
int main(void)
{
  float wid_spec[16384];
  float wa = 3.0f, wb = 1.0f, wc = 4.0f, cfactor = 2.0f, chs = 2.0f;
  float w, x, f1, f2, f3,  wa2, wb2, wc2;
  int   iclo = 200, ichi = 5000;
  int   i1, i2, i3, hi, lo, nc, i, j, ich2;
  int   nclook, lookmin, lookmax, rl = 0;
  short looktab[16384];
  char  ans[80], tabnam[80], buf[32];
  FILE  *tabfile;

  tell("\n\n"
       "LUFWHM - a program to create lookup tables for incub8r and 4play\n"
       "    replay tasks. Incub8r and 4play allow non-linear gains; this\n"
       "    program creates a lookup table to convert the linear ADC\n"
       "    channels to the nonlinear cube channels, in such a way as to\n"
       "    keep a fixed FWHM for peaks in the cube.\n\n"
       "See documentation on gf2/gf3 for information on the parameters\n"
       "    specifying the FWHM as a function of energy.\n\n"
       "This program now allows lookup-tables to 16384 channels.\n\n"
       "D.C. Radford     Sept 1999\n\n");

  /* open old, or create new, look-up table file */
  /* ask for output file name */
  if (!askfn(tabnam, 80, "", ".tab", "Name of lookup file = ?")) return 0;

  /* try to open OLD output file */
  if ((tabfile = fopen(tabnam, "r+"))) {
    fclose(tabfile);
    if (read_tab_file(tabnam, &nclook,&lookmin, &lookmax, looktab, 16384) ||
	!askyn("It seems that this existing look-up table is for an ADC with %d channels,\n"
	       "     and will produce a cube with %d channels on a side.\n"
	       "...Modify this existing file? (Y/N)", nclook, lookmax)) {
      tabfile = 0;
    } else {
      tabfile = fopen(tabnam, "r+");
    }
  }

  if (!tabfile) {
    /* open NEW output file */
    tabfile = open_new_file(tabnam, 1);
    /* ask for dimension of look-up table (default = 8192) */
    while (1) {
      nclook = 8192;
      nc = ask(ans, 80, "Dimension of look-up table = ? (rtn for %d)", nclook );
      if (nc == 0 ||
	  (!inin(ans, nc, &nclook, &j, &j) &&
	   nclook > 1 && nclook <= 16384)) break;
      warn1("Please enter a value between 2 and 16384.\n");
    }
  }
  if (ichi >= nclook) ichi = nclook - 1;

 START:
  while (1) {
    nc = ask(ans, 80, "FWHM parameters = ? (rtn for %f %f %f)", wa, wb, wc);
    if (nc == 0 || !ffin(ans, nc, &wa, &wb, &wc)) break;
  }
  wa2 = wa * wa;
  wb2 = wb * wb;
  wc2 = wc * wc;

  while (1) {
    nc = ask(ans, 80,
	     "\n"
	     "These parameters were presumably derived from a projection\n"
	     "   spectrum. Now I need to know the compression factor\n"
	     "   between this spectrum and the ADC data on tape\n"
	     "   (e.g. if the ADC is 16384 chs, but your calibration\n"
	     "    spectrum is 4096 chs, then you should enter 4 here.)\n"
	     "... Compression factor = ? (rtn for %f)", cfactor);
    if (nc == 0) break;
    if (!ffin(ans, nc, &f1, &f2, &f3) && f1 >= 1.0f) {
      cfactor = f1;
      break;
    }
    warn1("ERROR -- illegal value, try again.\n");
  }

  while (1) {
    nc = ask(ans, 80, "No. of chs (in cube) per FWHM = ? (rtn for %f)", chs);
    if (nc == 0) break;
    if (!ffin(ans, nc, &f1, &f2, &f3) && f1 > 0.0f) {
      chs = f1;
      break;
    }
    warn1("ERROR -- illegal value, try again.\n");
  }

  while (1) {
    nc = ask(ans, 80,
	     "ADC-channel limits for the cube (lo,hi) = ? (rtn for %d, %d)",
	     iclo, ichi);
    if (nc == 0) break;
    if (!inin(ans, nc, &i1, &i2, &i3) &&
	i1 >= 0 && i2 < nclook && i1 < i2) {
      iclo = i1;
      ichi = i2;
      break;
    }
    warn1("ERROR -- illegal values, try again.\n");
  }

  lo = iclo;
  ich2 = 0;
  while (lo <= ichi) {
    ++ich2;
    x = ((float) lo / cfactor + 0.5f) / 1e3f;
    w = sqrt(wa2 + wb2 * x + wc2 * x * x) * cfactor;
    lo += (int) ((w / chs) + 0.5);
  }
  if (!askyn("No. of chs. in cube = %d.\n"
	     "--- Okay? (Y/N)", ich2)) goto START;

  for (j = 0; j < nclook; ++j) {
    looktab[j] = 0;
  }
  lo = iclo;
  for (i = 1; i < ich2+1; ++i) {
    x = ((float) lo / cfactor + 0.5f) / 1e3f;
    w = sqrt(wa2 + wb2 * x + wc2 * x * x) * cfactor;
    hi = lo + (int) ((w / chs) + 0.5);
    if (hi > nclook) hi = nclook;
    for (j = lo; j < hi; ++j) {
      looktab[j] = i;
    }
    wid_spec[i - 1] = (float) (hi - lo);
    if ((lo = hi) == nclook) break;
  }

  /* write out look-up table to disk file */
  lookmin = 1;
  lookmax = ich2;
  rewind(tabfile);
#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(&nclook, 4);
  W(&lookmin, 4);
  W(&lookmax, 4);
#undef W
  if (put_file_rec(tabfile, buf, rl) ||
      put_file_rec(tabfile, looktab, 2*nclook)) {
    file_error("write to", tabnam);
    return 1;
  }
  fclose(tabfile);

  tell("The \"Width Spectrum\" has %d channels, and contains\n"
	 " the width (in ADC-chs) for each cube channel.\n", ich2);
  askfn(ans, 80, "", ".spe", "File name for width spectrum = ?");
  wspec(ans, wid_spec, ich2);
  tell(" Width Spectrum written as file %s\n", ans);

  return 0;
} /* main */
