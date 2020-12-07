#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"
int rmsread(char *fn, float *sp, char *namesp, int *numch, int idimsp, int id);


int main(void)
{
  float spec[16384];
  int   i, j, lo, hi, numch, nc;
  char  asciifil[80], spe_file[80], namesp[8];
  FILE  *file2;

  if (!(nc = cask("Name of .rms file to be converted = ?\n"
                  "  (default .ext = .rms, return to end)",
                  asciifil, 80))) return 0;
  strcpy(spe_file, asciifil);
  if (!(nc = cask("Range of sp IDs to be converted = ? ",
                  asciifil, 80))) return 0;
  sscanf(asciifil, "%d %d", &lo, &hi);

  /* convert .rms to ..asc */
  j = setext(spe_file, ".rms", 80);
  strcpy(asciifil, spe_file);
  strcpy(asciifil + j, ".asc");
  /* open ASCII file */
  file2 = fopen(asciifil, "w");
  for (j=lo; j<=hi; j++) {
    if (rmsread(spe_file, spec, namesp, &numch, 16384, j)) {
      printf("Error: Cannot open or read %s, id %d\n", spe_file, j);
      return 0;
    }
    for (i = 0; i < numch; ++i) {
      fprintf(file2, "%10i,%10i\n", i, (int) rint(spec[i]));
    }
    fprintf(file2, "\n");
  }
  fclose(file2);
  /* tell user that the file has been converted */
  printf("  %s ==> %s, %i chs.\n", spe_file, asciifil, numch);

  return 0;
} /* spec_ascii */


#define ERR {tell("ERROR reading file %s at %d!\n", fn, ftell(file)); return 1;}
/* ======================================================================= */
int rmsread(char *fn, float *sp, char *namesp, int *numch, int idimsp, int id)
{
  /* subroutine to read spectrum from radware multi-spectrum file
     into array sp, of dimension idimsp
     fn = file name, or contains range of channels to read
     numch = number of channels read
     namesp = name of spectrum (char*8, set to sp. ID or sp. name)
     file extension must be .rms */


  float fsp[16384];
  int   *isp;
  short *ssp;

  int  i, ilo, ihi, iy, nsp, dptr, spdir[2];
  int  spmode, xlen, expand, nextptr=0;
  int  x0, nx;
  char txt[64];
  FILE *file;
  char *modes[5] = {"shorts", "ints", "floats", "doubles", "bytes"};


  *numch = 0;
  isp = (int *) fsp;
  ssp = (short *) fsp;

  ihi = ilo = id;

  printf("opening file %s\n", fn);
  if (!(file = open_readonly(fn))) return 1;

  /* read spectrum ID directory */
  /* directory(ies):   2*(n+1) ints
     int n = number_of_IDs_in_header; int pointer_to_next_directory
     int sp_ID_1    int pointer_to_sp_data_1
     int sp_ID_2    int pointer_to_sp_data_2
     int sp_ID_3    int pointer_to_sp_data_3
     . . .
     int sp_ID_n    int pointer_to_sp_data_n
     [-1            0]  if no more spectra
  */
  for (iy = ilo; iy <= ihi; ++iy) {
    // tell(" >> Looking for ID %d in %s\n", iy, fn);
    if (iy > ilo && nextptr > 1) {
      /* have already read one spectrum; see if the ID of the next one matches */
      fseek(file, nextptr, SEEK_SET);
      if (2 != fread(spdir, sizeof(int), 2, file) || spdir[0] != iy) nextptr = 0;
    }
    if (nextptr == 0) {
      rewind(file);
      while (1) {
        if (1 != fread(&nsp,  sizeof(int), 1, file) ||
            1 != fread(&dptr, sizeof(int), 1, file)) ERR;
        for (i=0; i<nsp; i++) {  // read through directory entries
          if (2 != fread(spdir, sizeof(int), 2, file)) ERR;
          if (spdir[0] == iy) break;  // found it!
        }
        if (spdir[0] == iy || dptr <= 0) break;
        fseek(file, dptr, SEEK_SET);
      }
      if (spdir[0] != iy) {
        tell("Spectrum ID %d not found in file %s!\n", iy, fn);
        return 1;
      }
    }

    /* if we are here, then we have found the spectrum we want */
    /* initialize output spectrum to zero */
    if (iy == ilo) memset(sp, 0, sizeof(float) * idimsp);
    if (spdir[1] < 1) return 1; // spectrum has zero length / does not exist

    nextptr = ftell(file);  // save this to try for next spectrum ID
    fseek(file, spdir[1], SEEK_SET);
    /* now read the spectrum header */
    if (1 != fread(&id,      sizeof(int), 1, file) || // sp ID again
        // 1 != fread(&nextptr, sizeof(int), 1, file) || // pointer to next directory entry
        1 != fread(&spmode,  sizeof(int), 1, file) || // storage mode (short/int/float...)
        1 != fread(&xlen,    sizeof(int), 1, file) || // x dimension
        // 1 != fread(&ylen,    sizeof(int), 1, file) || // y dimension
        1 != fread(&expand,  sizeof(int), 1, file) || // extra space for expansion
        1 != fread(txt,      sizeof(txt), 1, file)) ERR;  // description/name text
    /* if (id != iy) { */
    /*   warn("Spectrum ID mismatch (%d, %d) in file %s!\n", iy, id, fn); */
    /*   return 1; */
    /* } */
    if (spmode > 3) ERR;  // limit to simple short/int/float for now

    tell("Sp ID %d in file %s; %d %s; %s\n", iy, fn, xlen, modes[spmode-1], txt);
    if ((*numch = xlen) > idimsp) {
      tell("First %d (of %d) chs only taken.", idimsp, *numch);
      *numch = idimsp;
    }

    i = 0;
    /* initialize input spectrum to zero */
    memset(fsp, 0, sizeof(fsp));

    /* now read the actual spectrum data */
    while (i < *numch) {
      if (1 != fread(&x0, sizeof(int), 1, file) ||    // starting ch for this chunk of spectrum
          1 != fread(&nx, sizeof(int), 1, file)) ERR; // number of chs in this chunk of spectrum
      if (x0 < 0 || nx < 1) break;   // no more non-zero bins in the spectrum
      if (nx > *numch - x0) nx = *numch - x0;

      if (spmode == 1) {         // shorts
        if (nx != fread(ssp+x0, sizeof(short), nx, file)) ERR;
      } else if (spmode == 2) {  // ints
        if (nx != fread(isp+x0, sizeof(int),   nx, file)) ERR;
      } else if (spmode == 3) {  // floats
        if (nx != fread(fsp+x0, sizeof(float), nx, file)) ERR;
      }
      i = x0 + nx;
    }
    /* and add it to the output spectrum */
    if (spmode == 1) {         // shorts
      for (i=0; i<*numch; i++) sp[i] += (float) ssp[i];
    } else if (spmode == 2) {  // ints
      for (i=0; i<*numch; i++) sp[i] += (float) isp[i];
    } else if (spmode == 3) {  // floats
      for (i=0; i<*numch; i++) sp[i] += fsp[i];
    }
  }
    
  fclose(file);
  if (ihi > ilo) {
    sprintf(namesp, "%4d%4d", ilo, ihi);
  } else {
    strncpy(namesp, txt, 8);
  }
  return 0;
#undef ERR
} /* rmsread */
