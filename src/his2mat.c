#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

  /* program to extract 4kx4k matrices from .his files (ornl format)
        into .mat or .spn files (radware format).
     D.C. Radford   Dec 2002 */

int main(void)
{
  FILE  *file;
  
  short ihed[64];
  int   spec[16384];
  int   iy, id, nc, in2, in3, ierr = 0, fourbytes = 1;
  char  ans[80], mser[80];
  static char hisfilnam[80] = "", matfilnam[80] = "";
  static int  ndx[4] = { 1,1,0,0 }, lud = 1, luh = 2, idimsp = 4096;

  extern void hisman_(char *mode, char *namf, int *lud, int *luh,
		      char *acp, int *ierr);
  extern void hisio_(char *mode, int *lud, int *luh, int *ihn, int *ndx,
		     int *nch, short *ihed, int *idat, int *ierr, char *mser);
  extern struct {
    char mssg[112], namprog[8];
    int  logut, logup;
    char lisflg[4], msgf[4];
  } lll_;


  memset(lll_.mssg, ' ', 112);
  strncpy(lll_.namprog, "HIS2MAT ", 8);
  lll_.logut = 6;
  lll_.logup = 0;
  strncpy(lll_.lisflg,  "LOF ", 4);
  strncpy(lll_.msgf,    "    ", 4);

  printf("\n\n"
	 "HIS2MAT - A program to extract 4kx4k matrices\n"
	 "          from .his files (ornl format)\n"
	 "          into .mat or .spn files (radware format)\n\n");

  /* get input file name */
  while (1) {
    cask("His file name = ? (default .ext = .his)", hisfilnam, 80);
    setext(hisfilnam, ".his", 80);
    hisman_("OPEN", hisfilnam, &lud, &luh, "RO  ", &ierr);
    if (!ierr) break;
    file_error("open", hisfilnam);
  }

  hisio_("INIT", &lud, &luh, &id, ndx, &idimsp, ihed, spec, &ierr, mser);
  if (ierr) {
    mser[40] = 0;
    printf("%s\n", mser);
    goto READERR;
  }

  /* ask for spectrum id */
  while ((nc = cask("Spectrum ID of 4kx4k matrix = ?", ans, 80)) &&
	 inin(ans, nc, &id, &in2, &in3));
  if (nc == 0) return 1;
  if (id < 1) goto READERR;

  fourbytes = caskyn("Is the spectrum 4 bytes per channel? (Y/N)");

  if (fourbytes) {
    /* get output file name */
    cask("Spn file name = ? (default .ext = .spn)", matfilnam, 80);
    setext(matfilnam, ".spn", 80);
    file = open_new_file(matfilnam, 1);

    printf("Working...\n\n");
    for (iy = 0; iy < 4096; ++iy) {
      printf("\ry = %d", iy); fflush(stdout);
      /* read 4k chs of data */
      idimsp = 4096;
      hisio_("READ", &lud, &luh, &id, ndx, &idimsp, ihed, spec, &ierr, mser);
      if (ierr) {
	mser[40] = 0;
	printf("%s\n", mser);
	goto READERR;
      }
      ++ndx[1];
      /* write 4k chs of data */
      fwrite(spec,4096,4,file);
    }
  } else {
    /* get output file name */
    cask("Mat file name = ? (default .ext = .mat)", matfilnam, 80);
    setext(matfilnam, ".mat", 80);
    file = open_new_file(matfilnam, 1);

    printf("Working...\n\n");
    for (iy = 0; iy < 4096; ++iy) {
      printf("\ry = %d", iy); fflush(stdout);
      /* read 4k chs of data */
      idimsp = 4096;
      hisio_("READ", &lud, &luh, &id, ndx, &idimsp, ihed, spec, &ierr, mser);
      if (ierr) {
	mser[40] = 0;
	printf("%s\n", mser);
	goto READERR;
      }
      ++ndx[1];
      /* write 4k chs of data */
      fwrite(spec,4096,2,file);
    }
  }

  fclose(file);
  printf("\nFinished...\n"
	 " 4kx4k channels have been written to %s\n", matfilnam);
  return 0;

 READERR:
  file_error("read", hisfilnam);
  return 1;
} /* his2mat */
