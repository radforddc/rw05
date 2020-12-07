#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

/* ====================================================================== */
int read_spe_file(char *filnam, float *sp, char *namesp, int *numch,
		  int idimsp)
{
  /* read spectrum from .spe file = filnam
     into array sp of dimension idimsp
     numch = number of channels read
     namesp = name of spectrum (character*8)
     returns 1 for open/read error
     default file extension = .spe */

  char cbuf[128];
  int  idim1, idim2, j, rl, swap = -1;
  FILE *file;

  setext(filnam, ".spe", 80);

  /* read spectrum in standard gf3 format
     OPEN(ILU,FILE=FILNAM,FORM='UNFORMATTED',STATUS='OLD')
     READ(ILU) NAMESP, IDIM1, IDIM2, IRED1, IRED2
     READ(ILU,ERR=90) (SP(I),I=1,NUMCH) */

  if (!(file = open_readonly(filnam))) return 1;
  rl = get_file_rec(file, cbuf, 128, 0);
  if (rl != 24  && rl != -24) {
    file_error("read", filnam);
    fclose(file);
    return 1;
  }
  if (rl < 0) {
    /* file has wrong byte order - swap bytes */
    tell("*** Swapping bytes read from file %s\n", filnam);
    swap = 1;
    for (j = 2; j < 4; ++j) {
      swapb4(cbuf + 4*j);
    }
  }
  strncpy(namesp, cbuf, 8);
  memcpy(&idim1, cbuf + 8, 4);
  memcpy(&idim2, cbuf + 12, 4);
  *numch = idim1 * idim2;

  if (*numch > idimsp) {
    *numch = idimsp;
    tell("First %d chs only taken.", idimsp);
  }
  rl = get_file_rec(file, sp, 4*idimsp, swap);
  fclose(file);
  if (rl != 4*(*numch) && rl != -4*(*numch)) {
    file_error("read spectrum from", filnam);
    return 1;
  }
  if (rl < 0) {
    /* file has wrong byte order - swap bytes */
    for (j = 0; j < *numch; ++j) {
      swapb4((char *) (sp + j));
    }
  }
  return 0;
} /* read_spe_file */

/* ======================================================================= */
int rmat(FILE *lu, int ich0, int nchs, short *matseg)
{
  /* rmat: subroutine to read matrix segment from disc
     lu = file descr. of matrix file
     ich0 = no. of first row to be read (0 to 4095)
     nchs = no. of rows to be read
     matseg = int*2 or int*4 buffer, 4096 by at least nchs in size */

  fseek(lu, ich0*8192, SEEK_SET);
  return (fread(matseg, 8192, nchs, lu) != nchs);
}

int rmat4b(FILE *lu, int ich0, int nchs, int *matseg)
{

  fseek(lu, ich0*16384, SEEK_SET);
  return (fread(matseg, 16384, nchs, lu) != nchs);
} /* rmat, rmat4b */

/* ======================================================================= */
int wmat(FILE *lu, int ich0, int nchs, short *matseg)
{
  /* wmat: subroutine to write matrix segment to disc
     lu = file descr. of matrix file
     ich0 = no. of first row to be written (0 to 4095)
     nchs = no. of rows to be written
     matseg = int*2 or int*4 buffer, 4096 by at least nchs in size */

  fseek(lu, ich0*8192, SEEK_SET);
  return (fwrite(matseg, 8192, nchs, lu) != nchs);
}

int wmat4b(FILE *lu, int ich0, int nchs, int *matseg)
{

  fseek(lu, ich0*16384, SEEK_SET);
  return (fwrite(matseg, 16384, nchs, lu) != nchs);
} /* wmat, wmat4b */

/* ======================================================================= */
int wspec(char *filnam, float *spec, int idim)
{
  /* write spectra in gf3 format
     filnam = name of file to be created and written
     spec = spectrum of length idim */

  char buf[32];
  int  j, c1 = 1, rl = 0;
  char namesp[8];
  FILE *file;

  j = setext(filnam, ".spe", 80);
  if (!(file = open_new_file(filnam, 0))) return 1;
  strncpy(namesp, filnam, 8);
  if (j < 8) memset(&namesp[j], ' ', 8-j);

  /* WRITE(1) NAMESP,IDIM,1,1,1 */
  /* WRITE(1) SPEC */
#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(namesp,8); W(&idim,4); W(&c1,4); W(&c1,4); W(&c1,4);
#undef W
  if (put_file_rec(file, buf, rl) ||
      put_file_rec(file, spec, 4*idim)) {
    file_error("write to", filnam);
    fclose(file);
    return 1;
  }
  fclose(file);
  return 0;
} /* wspec */
