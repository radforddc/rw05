#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

#define  MAXCHS 4096    /* max number of channels per dimension */

int main(int argc, char **argv)
{
  int    numchs;
  int    nterms;
  int    esc_file_flags1[10], esc_file_flags2[10];
  float  spec[MAXCHS], rdata1[MAXCHS], rdata2[MAXCHS];
  int    reclen;
  FILE   *file1, *file2;

  float a, b, a2, b2, fj1, fj2, fsum1, fsum2, fsum3;
  int   ix, iy, jext1, jext2, nc, fflags2 = 1;
  char  filenm1[80], filenm2[80], dattim[20], ans[80];
  char  *infile1 = "", *infile2 = "";

  printf("\n\n\n"
	 "  ADDESC Version 1.0    D. C. Radford   Nov 1999\n\n"
	 "     Welcome....\n\n");

  /* initialise */
  if (argc > 1) infile1 = argv[1];
  if (argc > 2) infile2 = argv[2];

  /* esc_file_flags[0]   : 0/1 for matrix compressed / not compressed
     esc_file_flags[1]   : 1 for new versions with systematic errors
                              on background and energy, effic calibs
     esc_file_flags[2]   : 0 for standard type file,
                           1 for file with extra data on statistical errors
                             (allows use on difference-matrices,
                              2d-unfolded matrices etc.)
     esc_file_flags[3-9] : for future use */

  /* ask for and open first escl8r data file */
  if ((nc = strlen(infile1)) > 0) {
    strcpy(filenm1, infile1);
  } else {
  START:
    nc = cask("First escl8r data file = ? (default .ext = .esc)",
	      filenm1, 80);
  }
  jext1 = setext(filenm1, ".esc", 80);
  if (!strcmp(filenm1 + jext1, ".e4k") ||
      !strcmp(filenm1 + jext1, ".E4K")) {
    numchs = 4096;
  } else {
    numchs = 2048;
  }
  reclen = numchs*4;
  if (!(file1 = fopen(filenm1, "r+"))) {
    file_error("open", filenm1);
    goto START;
  }

  fseek(file1, (numchs + 6)*reclen + 48, SEEK_SET);
  fread(&nterms, 4, 1, file1);
  if (nterms > 6 || nterms < 2) {
    fclose(file1);
    printf("*** ERROR -- File %s\n"
	   "     seems to have the wrong byte ordering.\n"
	   "    -- Perhaps you need to run unix2unix,\n"
	   "       vms2unix or unix2vms on it.\n", filenm1);
    goto START;
  }
  fseek(file1, (numchs + 6)*reclen + 124, SEEK_SET);
  fread(esc_file_flags1, 4, 10, file1);

  /* ask for and open second escl8r data file */
  if ((nc = strlen(infile2)) > 0) {
    strcpy(filenm2, infile2);
    infile2 = "";
  } else {
  START2:
    printf("Second escl8r data file = ? (default .ext = .%s)",
	   filenm1 + jext1);
    nc = cask("", filenm2, 80);
  }
  jext2 = setext(filenm2, filenm1 + jext1, 80);
  if (strcmp(filenm1 + jext1, filenm2 + jext2)) {
    fclose(file1);
    printf("*** ERROR -- File %s\n"
	   "         and file %s\n"
	   "           have different record lengths!",
	   filenm1, filenm2);
    goto START;
  }
  if (!(file2 = fopen(filenm2, "r+"))) {
    file_error("open", filenm2);
    goto START2;
  }
  fseek(file2, (numchs + 6)*reclen + 48, SEEK_SET);
  fread(&nterms, 4, 1, file2);
  if (nterms > 6 || nterms < 2) {
    fclose(file2);
    printf("*** ERROR -- File %s\n"
	   "     seems to have the wrong byte ordering.\n"
	   "    -- Perhaps you need to run unix2unix,\n"
	   "       vms2unix or unix2vms on it.\n", filenm2);
    goto START2;
  }
  fseek(file2, (numchs + 6)*reclen + 124, SEEK_SET);
  fread(esc_file_flags2, 4, 10, file2);

  printf(" File %s will be replaced by:\n"
	 "     a * %s + b * %s\n", filenm1, filenm1, filenm2);
  while ((nc = cask("   ... a = ? (rtn for 1.0)", ans, 80)) &&
	 ffin(ans, nc, &a, &fj1, &fj2));
  if (!nc) a = 1.0f;
  while ((nc = cask("   ... b = ? (rtn for 1.0)", ans, 80)) &&
	 ffin(ans, nc, &b, &fj1, &fj2));
  if (!nc) b = 1.0f;
  printf("\nWorking....\n\n");
  if (a == 1.0f && b == 1.0f &&
      !esc_file_flags1[2] && !esc_file_flags2[2]) fflags2 = 0;

  a2 = a * a;
  b2 = b * b;
  for (iy = 0; iy < numchs; ++iy) {
    if (iy%10 == 0) {
      printf("\rRow %i...", iy);
      fflush(stdout);
    }
    /* read rows from matrices, subtract and rewrite */
    fseek(file1, iy*reclen, SEEK_SET);
    fread(rdata1, 4, numchs, file1);
    fseek(file2, iy*reclen, SEEK_SET);
    fread(rdata2, 4, numchs, file2);
    for (ix = 0; ix < numchs; ++ix) {
      spec[ix] = a * rdata1[ix] + b * rdata2[ix];
    }
    fseek(file1, iy*reclen, SEEK_SET);
    fwrite(spec, 4, numchs, file1);
    fflush(file1);
    if (!fflags2) continue;

    if (esc_file_flags1[2]) {
      fseek(file1, (iy + numchs + 7)*reclen, SEEK_SET);
      fread(rdata1, 4, numchs, file1);
    }
    if (esc_file_flags2[2]) {
      fseek(file2, (iy + numchs + 7)*reclen, SEEK_SET);
      fread(rdata2, 4, numchs, file2);
    }
    for (ix = 0; ix < numchs; ++ix) {
      spec[ix] = a2 * rdata1[ix] + b2 * rdata2[ix];
    }
    fseek(file1, (iy + numchs + 7)*reclen, SEEK_SET);
    fwrite(spec, 4, numchs, file1);
    fflush(file1);
  }

  printf("\rProjections...\n");
  fflush(stdout);

  fsum1 = 0.0f;
  fsum2 = 0.0f;
  iy = numchs;
  fseek(file1, iy*reclen, SEEK_SET);
  fread(rdata1, 4, numchs, file1);
  fseek(file2, iy*reclen, SEEK_SET);
  fread(rdata2, 4, numchs, file2);
  for (ix = 0; ix < numchs; ++ix) {
    spec[ix] = a * rdata1[ix] + b * rdata2[ix];
    fsum1 += rdata1[ix];
    fsum2 += rdata2[ix];
  }
  fseek(file1, iy*reclen, SEEK_SET);
  fwrite(spec, 4, numchs, file1);
  fflush(file1);

  iy = numchs + 1;
  a2 = a * fsum1;
  b2 = b * fsum2;
  fsum3 = (a2 + b2);
  fseek(file1, iy*reclen, SEEK_SET);
  fread(rdata1, 4, numchs, file1);
  fseek(file2, iy*reclen, SEEK_SET);
  fread(rdata2, 4, numchs, file2);
  for (ix = 0; ix < numchs; ++ix) {
    spec[ix] = (a2 * rdata1[ix] + b2 * rdata2[ix]) / fsum3;
  }
  fseek(file1, iy*reclen, SEEK_SET);
  fwrite(spec, 4, numchs, file1);
  fflush(file1);

  iy = numchs + 2;
  fseek(file1, iy*reclen, SEEK_SET);
  fread(rdata1, 4, numchs, file1);
  fseek(file2, iy*reclen, SEEK_SET);
  fread(rdata2, 4, numchs, file2);
  for (ix = 0; ix < numchs; ++ix) {
    spec[ix] = a * rdata1[ix] + b * rdata2[ix];
  }
  fseek(file1, iy*reclen, SEEK_SET);
  fwrite(spec, 4, numchs, file1);
  fflush(file1);

  memset(spec, 0, reclen);
  for (iy = numchs + 3; iy < numchs + 6; iy++) {
    fseek(file1, iy*reclen, SEEK_SET);
    fwrite(spec, 4, numchs, file1);
    fflush(file1);
  }

  printf(" ....done.\n"
	 "If you had used the E2-enhanced background subtraction,\n"
	 "  it is now disabled.  Sorry.\n");
  fflush(stdout);

  /* make sure esc_file_flags[2] is set for file 1 */
  if (fflags2) {
    esc_file_flags1[2] = 1;
    fseek(file1, (numchs + 6)*reclen + 124, SEEK_SET);
    fwrite(esc_file_flags1, 4, 10, file1);
  }
  fclose(file1);
  fclose(file2);

  /* open .esc_log file for append */
  strcpy(filenm1 + jext1 + 4, "_log");
  if (!(file1 = fopen(filenm1, "a+"))) {
    file_error("open", filenm1);
    return -1;
  }
  datetime(dattim);
  fprintf(file1, "%s: program addesc ran:\n",dattim);
  fprintf(file1,
	  "      File %s replaced by:\n"
	  "        %f * %s + %f * %s\n"
	  "   If you had used the E2-enhanced background subtraction,\n"
	  "      it is now disabled.  Sorry.\n",
	  filenm1, a, filenm1, b, filenm2);
  fclose(file1);

  return 0;
}
