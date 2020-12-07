#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

int matread(char *fn, float *sp, char *namesp, int *numch, int idimsp,
	    int *spmode)
{
  /* subroutine to read spectrum from matrix file
     into array sp, of dimension idimsp
     fn = file name, or contains range of channels to read
     numch = number of channels read
     namesp = name of spectrum (char*8, set to ilo ihi)
     spmode = 2/3 for .mat/.spn files
     file extension must be .mat */

  /* 2000/10/31:  added support for .sec files (like 8192-ch .spn files)
     spmode = 5 for .sec files */

  int i4spn[8192];
#define i2mat ((short *) i4spn)

  int  i4sum[8192], i, jrecl, in3, ilo, ihi, nc, iy, nch;
  char ans[80], filnam[80];
  FILE *file;


  *numch = 0;

  /* look for int chan. no. limits */
  nc = strlen(fn);
  if (inin(fn, nc, &ilo, &ihi, &in3)) {
    /* not ints; fn = file name, check matrix file exists*/
    if (!inq_file(fn, &jrecl)) {
      file_error("open", fn);
      tell("...File does not exist.\n");
      return 1;
    }
    strncpy(filnam, fn, 80);
    /* ask for range of Y-channels to be added together */
    while ((nc = ask(ans, 80, "Type range of y-channels (lo,hi) ?")) &&
           inin(ans, nc, &ilo, &ihi, &in3));
    if (nc == 0) return 0;
  }
  if (ihi == 0) ihi = ilo;
  if (ihi < ilo) {
    i = ilo;
    ilo = ihi;
    ihi = i;
  }
  if (ilo < 0 || ihi >= *numch) {
    tell("Bad y-gate channel limits: %d %d", ilo, ihi);
    return 1;
  }
  if (!(file = open_readonly(filnam))) return 1;
  nch = 4096;
  if (*spmode == 5) nch = 8192;
  *numch = nch;
  if (*numch > idimsp) {
    *numch = idimsp;
    tell("First %d chs only taken.", idimsp);
  }
  /* initialize i4sum */
  for (i = 0; i < *numch; ++i) {
    i4sum[i] = 0;
  }
  if (*spmode == 2) {
    /* read matrix rows into i2mat and add into i4sum */
    fseek(file, ilo*8192, SEEK_SET);
    for (iy = ilo; iy <= ihi; ++iy) {
      if (fread(i2mat, 8192, 1, file) != 1) {
	file_error("read", filnam);
	fclose(file);
	return 1;
      }
      for (i = 0; i < *numch; ++i) {
	i4sum[i] += i2mat[i];
      }
    }
  } else {
    /* read matrix rows into i4spn and add into i4sum */
    fseek(file, ilo*nch*4, SEEK_SET);
    for (iy = ilo; iy <= ihi; ++iy) {
      if (fread(i4spn, nch*4, 1, file) != 1) {
	file_error("read", filnam);
	fclose(file);
	return 1;
      }
      for (i = 0; i < *numch; ++i) {
	i4sum[i] += i4spn[i];
      }
    }
  }
  /* convert to float */
  for (i = 0; i < *numch; ++i) {
    sp[i] = (float) i4sum[i];
  }
  fclose(file);
  strncpy(fn, filnam, 80);
  sprintf(namesp, "%4d%4d", ilo, ihi);
  return 0;
} /* matread */
