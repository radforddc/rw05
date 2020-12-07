#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  int   ishi[400], islo[400], i, j, is;
  int   nclook, lookmin, lookmax, rl = 0;
  short looktab[16384];
  char  filnam[80], line[120], buf[32];
  FILE  *file;
  
  /* program to convert .win files to .tab files.
     Original     F. Banville    Apr. 1988
     C version    D.C. Radford   Sept 1999 */

  printf("\n\n"
	 "WIN2TAB - a program to convert .win files to .tab files\n\n");
  while (1) {
    cask("Window file name = ? (default .EXT = .win)", filnam, 80);
    setext(filnam, ".win", 80);
    if ((file = open_readonly(filnam))) break;
  }

  /* read in windows */
  is = 0;
  while (fgets(line, 120, file) &&
	 sscanf(line, "%*5c%d%*3c%d", &islo[is], &ishi[is]) == 2 &&
	 is++ < 400);
  fclose(file);
  printf(" %i windows have been read from %s.\n", is, filnam);

  /* output (.tab) file name */
  cask("Lookup table file name = ? (default .EXT = .tab)", filnam, 80);
  setext(filnam, ".tab", 80);
  file = open_new_file(filnam, 1);
  /* construction of the table */
  for (i = 0; i < 16384; ++i) {
    looktab[i] = 0;
  }
  nclook = 0;
  for (i = 0; i < is; ++i) {
    for (j = islo[i]; j <= ishi[i]; ++j) {
      if (j > 0 && j < 16384) {
	looktab[j] = (short) (i+1);
	if (nclook <= j) nclook = j + 1;
      }
    }
  }
  lookmin = 1;
  lookmax = is;
  /* write it out */
#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(&nclook, 4);
  W(&lookmin, 4);
  W(&lookmax, 4);
#undef W
  if (put_file_rec(file, buf, rl) ||
      put_file_rec(file, looktab, 2*nclook)) {
    file_error("write to", filnam);
    fclose(file);
    return 1;
  }
  fclose(file);
  printf(" %i channels have been written to %s\n", nclook, filnam);
  return 0;
} /* win2tab */
