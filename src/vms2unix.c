#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

int byte1, file_size, pointer;
/* CHARACTER*1  BUF(30000 + MAXCHS*(MAXCHS+25)*2) */
/* MAX_BYTES = 30000 + MAXCHS*(MAXCHS+25)*2 */
int max_bytes = 4020000;
char buf[4020000];
FILE *ofile, *file;
char filename[80];

/* ==== routines defined here ==== */
int spe_format(void);
int rsp_format(void);
int dmp_format(void);
int eff_format(void);
int cal_format(void);
int tab_format(void);
int gls_format(void);
int lev_format(void);
int twod_format(void);
int esc_format(void);
int e4k_format(void);
int mat_format(void);
int spn_format(void);
int cub_format(void);

/* ======================================================================= */
int main(void)
{
  char *ext;
  short i2;

  /* figure out if system is big-endian or little-endian */
  i2 = 1;
  byte1 = 0;
  if ((*(char *)(&i2)) == '\1') byte1 = 1;

  printf("\n\n"
	 "This program translates VMS unformatted RadWare files,\n"
	 "    transferred for example by FTP from a VMS machine,\n"
	 "    to the equivalent unix-compatible unformatted files.\n\n"
	 " At present, it accepts the following filename extensions:\n"
	 "      .spe,  .err,  .dmp,  .eff,  .cal,  .tab,\n"
	 "      .gls,  .lev,  .2dp,  .2d,   .esc,\n"
	 "      .e4k,  .mat,  .spn,  .rsp  and  .cub.\n\n"
	 "C version    D.C. Radford    Sept 1999\n\n");

  while (cask("Input file = ? (rtn to end)", filename, 80)) {
    ext =  filename + setext(filename, "    ", 80) + 1;

    if (!strcmp(ext, "spe") || !strcmp(ext, "SPE") ||
	!strcmp(ext, "err") || !strcmp(ext, "ERR")) {
      spe_format();
    } else if (!strcmp(ext, "rsp") || !strcmp(ext, "RSP")) {
      rsp_format();
    } else if (!strcmp(ext, "dmp") || !strcmp(ext, "DMP")) {
      dmp_format();
    } else if (!strcmp(ext, "eff") || !strcmp(ext, "EFF")) {
      eff_format();
    } else if (!strcmp(ext, "cal") || !strcmp(ext, "CAL")) {
      cal_format();
    } else if (!strcmp(ext, "tab") || !strcmp(ext, "TAB")) {
      tab_format();
    } else if (!strcmp(ext, "gls") || !strcmp(ext, "GLS")) {
      gls_format();
    } else if (!strcmp(ext, "lev") || !strcmp(ext, "LEV")) {
      lev_format();
    } else if (!strcmp(ext, "2dp") || !strcmp(ext, "2DP")) {
      twod_format();
    } else if (!strcmp(ext, "2d" ) || !strcmp(ext, "2D" )) {
      twod_format();
    } else if (!strcmp(ext, "esc") || !strcmp(ext, "ESC")) {
      esc_format();
    } else if (!strcmp(ext, "e4k") || !strcmp(ext, "E4K")) {
      e4k_format();
    } else if (!strcmp(ext, "mat") || !strcmp(ext, "MAT")) {
      mat_format();
    } else if (!strcmp(ext, "spn") || !strcmp(ext, "SPN")) {
      spn_format();
    } else if (!strcmp(ext, "cub") || !strcmp(ext, "CUB")) {
      printf("As of rw97 and later, .cub files no longer require"
	     "   byte-swapping, which is done automatically as needed:\n");
    } else {
      printf("*** ERROR -- Unrecognised file extension...\n");
    }
  }
  return 0;
} /* main */

/* ======================================================================= */
int get_rec(int rlen)
{
  int   jrecl, ip = pointer + 4;
  short srecl[2];

  pointer += 8;
  jrecl = rlen;
  if (fread(srecl, 2, 1, file) != 1) goto ERR;
  memcpy(buf + ip, &rlen, 4);
  ip += 4;
  while (jrecl > 2042) {
    if (fread(buf + ip, 2044, 1, file) != 1) goto ERR;
    ip += 2042;
    jrecl -= 2042;
  }
  if (fread(buf + ip, jrecl, 1, file) != 1) goto ERR;
  ip += jrecl;
  memcpy(buf + ip, &rlen, 4);
  return 0;

 ERR:
  printf("Error during read, file %s.\n", filename);
  printf("This may be normal; if this error can be ignored,\n"
	 "   the output file will still be created.\n");
  return 1;
} /* get_vms_file */

/* ======================================================================= */
int get_vms_file(int rlen)
{
  struct stat sbuf;

  /* does file exist? */
  if (stat (filename, &sbuf)) {
    printf("Error: file does not exist: %s\n", filename);
    return 1;
  }
  /* will the file fit into memory block? */
  if ((file_size = sbuf.st_size) > max_bytes) {
    printf("Error: File %s is too big for buffer"
	   "       max size = %d, file size = %d.\n",
	   filename, max_bytes, file_size);
    return 1;
  }
  if (!(file = open_readonly(filename))) return 1;

  pointer = -4;
  return get_rec(rlen);

} /* get_vms_file */

/* ======================================================================= */
int open_new_ofile(void)
{
  int  iext, nc;
  char ans[80], ext[20];

  /* open new unformatted file, filename = default file name */
  iext = setext(filename, "", 80);
  strncpy(ext, filename+iext, 20);
  nc = askfn(ans, 80, filename, ext, "Name for new file = ?");
  strcpy(filename, ans);
  if (nc == 0) {
    ofile = open_save_file(filename, 1);
  } else {
    ofile = open_new_file(filename, 1);
  }
  return 0;
} /* open_new_ofile */

/* ======================================================================= */
int get_char(char *out, int nchar)
{
    memcpy(out, buf+pointer, nchar);
    pointer += nchar;
    return 0;
} /* get_char */

/* ======================================================================= */
int fixr4(void)
{
  float out;
  int   j;
  static int equiv_0[1];
#define bb ((char *)equiv_0)
#define ii (*equiv_0)

  if (byte1) {
    bb[0] = buf[pointer + 2];
    bb[1] = buf[pointer + 3];
    bb[2] = buf[pointer];
    bb[3] = buf[pointer + 1];
  } else {
    bb[0] = buf[pointer + 1];
    bb[1] = buf[pointer];
    bb[2] = buf[pointer + 3];
    bb[3] = buf[pointer + 2];
  }
  if (ii == 0) {
    out = 0.0f;
  } else {
    /* J = IOR('800000'X, IAND(II,'7FFFFF'X)) */
    j = 8388608 | (ii & 8388607);
    /* IF (IAND(II,'80000000'X).NE.0) J = -J */
    if (ii < 0) j = -j;
    out = (float) j / 8388608.f;
    /* J = IAND(II,'7F800000'X)/'800000'X */
    j = (ii & 2139095040) / 8388608;
    /* OUT = OUT * 2.0**(J - '81'X) */
    out *= pow(2.0f, (j - 129));
  }
  memcpy(buf+pointer, &out, 4);
  pointer += 4;
  return 0;
} /* fixr4 */
#undef ii
#undef bb

/* ======================================================================= */
int fixr8(void)
{
  double out;
  int    j;
  static int equiv_0[1];
#define bb ((char *)equiv_0)
#define ii (*equiv_0)

  if (byte1) {
    bb[0] = buf[pointer + 6];
    bb[1] = buf[pointer + 7];
    bb[2] = buf[pointer + 4];
    bb[3] = buf[pointer + 5];
  } else {
    bb[0] = buf[pointer + 5];
    bb[1] = buf[pointer + 4];
    bb[2] = buf[pointer + 7];
    bb[3] = buf[pointer + 6];
    }
  /* OUT = DBLE(IAND(II,'7FFFFFFF'X)) / DBLE(2)**32 */
  out = (double) (ii & 2147483647) / 4294967296.;
  if (ii < 0) out += 0.5;
  if (byte1) {
    bb[0] = buf[pointer + 2];
    bb[1] = buf[pointer + 3];
    bb[2] = buf[pointer];
    bb[3] = buf[pointer + 1];
  } else {
    bb[0] = buf[pointer + 1];
    bb[1] = buf[pointer];
    bb[2] = buf[pointer + 3];
    bb[3] = buf[pointer + 2];
  }
  if (ii == 0) {
    out = 0.0f;
  } else {
    /* J = IOR('800000'X, IAND(II,'7FFFFF'X)) */
    j = 8388608 | (ii & 8388607);
    out = (out + (double) j) / 8388608.;
    /* J = IAND(II,'7F800000'X)/'800000'X */
    j = (ii & 2139095040) / 8388608;
    out *= pow(2.0, (j - 129));
    if (ii < 0) out = -out;
  }
  memcpy(buf+pointer, &out, 8);
  pointer += 8;
  return 0;
} /* fixr8 */
#undef ii
#undef bb

/* ======================================================================= */
int swap4(void)
{
  char c;
 
  if (!byte1) {
    c = buf[pointer + 3];
    buf[pointer + 3] = buf[pointer];
    buf[pointer] = c;
    c = buf[pointer + 2];
    buf[pointer + 2] = buf[pointer + 1];
    buf[pointer + 1] = c;
  }
  pointer += 4;
  return 0;
} /* swap4 */

/* ======================================================================= */
int swap2(void)
{
  char c;

  if (!byte1) {
    c = buf[pointer + 1];
    buf[pointer + 1] = buf[pointer];
    buf[pointer] = c;
  }
  pointer += 2;
  return 0;
} /* swap2 */

/* ======================================================================= */
int get_i4(int *out)
{
  swap4();
  memcpy(out, buf+pointer-4, 4);
  return 0;
} /* get_i4 */

/* ======================================================================= */
int spe_format(void)
{
  int i, numch, idim1, idim2;

  if (get_vms_file(24)) return 0;
  pointer += 8;
  get_i4(&idim1);
  get_i4(&idim2);
  swap4();
  swap4();
  numch = idim1 * idim2;
  printf("%i channels...\n", numch);
  if (numch > 16384) {
    numch = 16384;
    printf("First 16384 chs only taken....\n");
  }
  if (get_rec(numch*4)) return 0;
  for (i = 0; i < numch; ++i) fixr4();

  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* spe_format */

/* ======================================================================= */
int rsp_format(void)
{
  char head[20] = "RESPONSE FILE VER. 1";
  int  i;
  char ans[80];

  if (get_vms_file(40)) return 0;

  get_char(ans, 40);
  if (strncmp(ans, head, 20)) {
    printf("ERROR - File cannot be read or is not\n"
	   "       a good response function file.\n");
    return 0;
  }

  if (get_rec(51404)) return 0;
  fixr4();
  for (i = 0; i < 514*25; ++i) fixr4();

  /* write response functions to new file  */
  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* rsp_format */

/* ======================================================================= */
int dmp_format(void)
{
  char head3[20] = "GELIFT ver. 6.1 dump";
  char title[40];
  int  i;

  if (get_vms_file(40)) return 0;

  get_char(title, 40);
  if (strncmp(title, head3, 20)) {
    printf("ERROR -- file is not a supported gf2/gf3 dump file\n");
    return 0;
  }

  if (get_rec(8932)) return 0;
  swap4();
  swap4();
  for (i = 0; i < 15; ++i) fixr4();
  swap4();
  for (i = 0; i < 51; ++i) fixr4();
  for (i = 0; i < 53; ++i) swap4();
  for (i = 0; i < 51; ++i) fixr4();
  swap4();
  swap4();
  for (i = 0; i < 45; ++i) fixr4();
  swap4();
  swap4();
  for (i = 0; i < 5; ++i) fixr4();
  for (i = 0; i < 3; ++i) swap4();
  for (i = 0; i < 6; ++i) fixr8();
  for (i = 0; i < 7; ++i) swap4();
  for (i = 0; i < 3; ++i) fixr4();
  for (i = 0; i < 21; ++i) swap4();
  for (i = 0; i < 1800; ++i) fixr4();
  for (i = 0; i < 20; ++i) swap4();
  pointer += 20;
  
  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* dmp_format */

/* ======================================================================= */
int eff_format(void)
{
  int i;

  if (get_vms_file(40)) return 0;

  pointer += 40;
  /* skip over 2 bytes at end of a possible 42-byte record */
  if (file_size == 86) fread(&i, 2, 1, file);

  if (get_rec(40)) return 0;
  for (i = 0; i < 10; ++i) fixr4();

  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* eff_format */

/* ======================================================================= */
int cal_format(void)
{
  int i;

  if (get_vms_file(98)) return 0;
  pointer += 98;
  if (get_rec(52)) return 0;
  swap4();
  for (i = 0; i < 6; ++i) fixr8();

  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* cal_format */

/* ======================================================================= */
int tab_format(void)
{
  int i, nclook;

  if (get_vms_file(12)) return 0;
  get_i4(&nclook);
  if (nclook < 2 || nclook > 16384) {
    file_error("read", filename);
    printf("          -- Bad NCLOOK = %i", nclook);
    return 0;
  }
  swap4();
  swap4();

  if (get_rec(2*nclook)) return 0;
  for (i = 0; i < nclook; ++i) swap2();

  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* tab_format */

/* ======================================================================= */
int gls_format(void)
{
  int level, gamma, nbands, ntlabel;
  int i, j, k;

  if (get_vms_file(52)) return 0;
  swap4();
  get_i4(&level);
  get_i4(&gamma);
  get_i4(&nbands);
  for (i = 0; i < 9; ++i) fixr4();

  printf("%d levels, %d gammas, %d bands.\n", level, gamma, nbands);
  if (get_rec(24*level + 61*gamma + 16*nbands)) return 0;
  for (j = 0; j < level; ++j) {
    for (k = 0; k < 5; ++k) fixr4();
    swap4();
  }
  for (j = 0; j < gamma; ++j) {
    swap4();
    swap4();
    pointer++;
    for (k = 0; k < 13; ++k) fixr4();
  }
  for (j = 0; j < nbands; ++j) {
    pointer += 8;
    fixr4();
    fixr4();
  }
  if (get_rec(4*level + 8*gamma + 4*nbands)) return 0;
  for (j = 0; j < nbands + 2*gamma + level; ++j) fixr4();

  if (!get_rec(36)) {
    for (j = 0; j < 8; ++j) fixr4();
    get_i4(&ntlabel);
    printf("%d text labels.\n", ntlabel);

    if (get_rec(32*level + 8*gamma + 12*nbands)) return 0;
    for (j = 0; j < 2*gamma; ++j) swap4();
    for (i = 0; i < level; ++i) {
      for (j = 0; j < 3; ++j) swap4();
      for (j = 0; j < 5; ++j) fixr4();
    }
    for (j = 0; j < 3*nbands; ++j) fixr4();

    if (ntlabel > 0) {
      if (get_rec(60*ntlabel)) return 0;
      for (i = 0; i < ntlabel; ++i) {
	pointer += 40;
	swap4();
	for (j = 0; j < 4; ++j) fixr4();
      }
    } else {
      memset(buf+pointer+4, 0, 8);
      pointer += 8;
    }
  }

  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* gls_format */

/* ======================================================================= */
int lev_format(void)
{
  int i, j, numchs, matchs, nclook;

  if (get_vms_file(88)) return 0;
  get_i4(&numchs);
  get_i4(&matchs);
  pointer += 80;

  if (get_rec(numchs*4)) return 0;
  for (j = 0; j < numchs; ++j) swap4();
  if (get_rec(matchs*4)) return 0;
  for (j = 0; j < matchs; ++j) fixr4();
  if (get_rec(numchs*27)) return 0;
  for (j = 0; j < 7*numchs; ++j) fixr4();
  if (get_rec(numchs*8)) return 0;
  for (j = 0; j < 2*numchs; ++j) fixr4();
  if (get_rec(numchs*8)) return 0;
  for (j = 0; j < 2*numchs; ++j) fixr4();
  if (get_rec(40)) return 0;
  for (j = 0; j < 10; ++j) fixr4();
  if (get_rec(12)) return 0;
  get_i4(&nclook);
  swap4();
  swap4();
  if (get_rec(nclook*2)) return 0;
  for (i = 0; i < nclook; ++i) swap2();

  if (!get_rec(4)) {
    get_i4(&j);
    if (j) {
      if (get_rec(matchs*4)) return 0;
      for (j = 0; j < matchs; ++j) fixr4();
      if (!get_rec(4 + numchs*4)) for (j = 0; j < numchs + 1; ++j) fixr4();
    }
    if (!get_rec(12)) for (j = 0; j < 3; ++j) fixr4();
  }
 
  fclose(file);
  open_new_ofile();
  fwrite(buf, pointer+4, 1, ofile);
  fclose(ofile);
  return 0;
} /* lev_format */

/* ======================================================================= */
int twod_format(void)
{
  int j, iy, reclen, numchs;

  if (!inq_file(filename, &reclen)) {
    printf("File %s does not exist!\n", filename);
    return 0;
  }
  printf("Record length = %i\n", reclen);
  if (!caskyn("The existing direct access file will be overwritten.\n"
	      "   ...Proceed? (Y/N)")) return 0;
  file = fopen(filename, "r+");

  numchs = reclen;
  reclen *= 4;
  for (iy = 0; iy < numchs; ++iy) {
    fseek(file, iy*reclen, SEEK_SET);
    fread(buf, reclen, 1, file);
    pointer = 0;
    for (j = 0; j < numchs; ++j) fixr4();
    fseek(file, iy*reclen, SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  fclose(file);
  return 0;
} /* twod_format */

/* ======================================================================= */
int esc_format(void)
{
  int i, iy, reclen;

  if (!inq_file(filename, &reclen)) {
    printf("File %s does not exist!\n", filename);
    return 0;
  }
  reclen = 2048*4;
  if (!caskyn("The existing direct access file will be overwritten.\n"
	      "   ...Proceed? (Y/N)")) return 0;
  file = fopen(filename, "r+");

  for (iy = 0; iy < 2054; ++iy) {
    fseek(file, iy*reclen, SEEK_SET);
    fread(buf, reclen, 1, file);
    pointer = 0;
    for (i = 0; i < 2048; ++i) fixr4();
    fseek(file, iy*reclen, SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  iy = 2054;
  fseek(file, iy*reclen, SEEK_SET);
  fread(buf, reclen, 1, file);
  pointer = 0;
  for (i = 0; i < 6; ++i) fixr8();
  swap4();
  for (i = 0; i < 18; ++i) fixr4();
  for (i = 0; i < 10; ++i) swap4();
  for (i = 0; i < 3; ++i) fixr4();
  fseek(file, iy*reclen, SEEK_SET);
  fwrite(buf, reclen, 1, file);

  memcpy(&i, buf+132, 4);  /* esc_file_flags[2] */
  if (i) {
    for (iy = 2055; iy < 4103; ++iy) {
      fseek(file, iy*reclen, SEEK_SET);
      fread(buf, reclen, 1, file);
      pointer = 0;
      for (i = 0; i < 2048; ++i) fixr4();
      fseek(file, iy*reclen, SEEK_SET);
      fwrite(buf, reclen, 1, file);
    }
  }

  fclose(file);
  return 0;
} /* esc_format */

/* ======================================================================= */
int e4k_format(void)
{
  int i, iy, reclen;

  if (!inq_file(filename, &reclen)) {
    printf("File %s does not exist!\n", filename);
    return 0;
  }
  reclen = 4096*4;
  if (!caskyn("The existing direct access file will be overwritten.\n"
	      "   ...Proceed? (Y/N)")) return 0;
  file = fopen(filename, "r+");
  for (iy = 0; iy < 4102; ++iy) {
    fseek(file, iy*reclen, SEEK_SET);
    fread(buf, reclen, 1, file);
    pointer = 0;
    for (i = 0; i < 4096; ++i) fixr4();
    fseek(file, iy*reclen, SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  iy = 4102;
  fseek(file, iy*reclen, SEEK_SET);
  fread(buf, reclen, 1, file);
  pointer = 0;
  for (i = 0; i < 6; ++i) fixr8();
  swap4();
  for (i = 0; i < 18; ++i) fixr4();
  for (i = 0; i < 10; ++i) swap4();
  for (i = 0; i < 3; ++i) fixr4();
  fseek(file, iy*reclen, SEEK_SET);
  fwrite(buf, reclen, 1, file);
 
  memcpy(&i, buf+132, 4);  /* esc_file_flags[2] */
  if (i) {
    for (iy = 4103; iy < 8199; ++iy) {
      fseek(file, iy*reclen, SEEK_SET);
      fread(buf, reclen, 1, file);
      pointer = 0;
      for (i = 0; i < 4096; ++i) fixr4();
      fseek(file, iy*reclen, SEEK_SET);
      fwrite(buf, reclen, 1, file);
    }
  }

  fclose(file);
  return 0;
} /* e4k_format */

/* ======================================================================= */
int mat_format(void)
{
  int j, iy, reclen;

  if (byte1) {
    printf("No modification necessary to this file...\n");
    return 0;
  }
  if (!inq_file(filename, &reclen)) {
    printf("File %s does not exist!\n", filename);
    return 0;
  }
  reclen = 4096*2;
  if (!caskyn("The existing direct access file will be overwritten.\n"
	      "   ...Proceed? (Y/N)")) return 0;
  file = fopen(filename, "r+");

  for (iy = 0; iy < 4096; ++iy) {
    fseek(file, iy*reclen, SEEK_SET);
    fread(buf, reclen, 1, file);
    pointer = 0;
    for (j = 0; j < 4096; ++j) swap2();
    fseek(file, iy*reclen, SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  fclose(file);
  return 0;
} /* mat_format */

/* ======================================================================= */
int spn_format(void)
{
  int j, iy = 0, reclen;

  if (byte1) {
    printf("No modification necessary to this file...\n");
    return 0;
  }
  if (!inq_file(filename, &reclen)) {
    printf("File %s does not exist!\n", filename);
    return 0;
  }
  reclen = 4096*4;
  if (!caskyn("The existing direct access file will be overwritten.\n"
	      "   ...Proceed? (Y/N)")) return 0;
  file = fopen(filename, "r+");

  while (fread(buf, reclen, 1, file) == 1) {
    pointer = 0;
    for (j = 0; j < 4096; ++j) swap4();
    fseek(file, reclen*(iy++), SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  fclose(file);
  return 0;
} /* spn_format */
