#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

int byte1, pointer;
/* MAX_BYTES = 30000 + MAXCHS*(MAXCHS+25)*2 */
/* CHARACTER*1  BUF(30000 + MAXCHS*(MAXCHS+25)*2) */
int max_bytes = 4020000;
char buf[4020000];
FILE *ofile, *file;
char filename[80];

/* ==== routines defined here ==== */
int get_file_c(char *fn, char *data, int maxbytes);
int rsp_format(void);
int dmp_format(void);
int gls_format(void);
int lev_format(void);
int twod_format(void);
int esc_format(void);
int e4k_format(void);
int mat_format(void);
int spn_format(void);
int sec_format(void);

/* ======================================================================= */
int main(void)
{
  char *ext;

  printf("\n\n"
	 "This program swaps bytes for unformatted RadWare files\n"
	 "    on unix systems, e.g. SUN to DEC or SUN to Linux.\n\n"
	 " At present, it accepts the following filename extensions:\n"
	 "      .dmp,  .gls,  .lev,  .2dp,  .2d,\n"
	 "      .esc,  .e4k,  .mat,  .spn, .sec  and  .rsp.\n"
	 " As of rw97 and later, the following file types no longer require\n"
	 "   byte-swapping, which is done automatically as needed:\n"
	 "      .spe,  .eff,  .cal,  and  .cub\n\n"
	 "C version    D.C. Radford    Sept 1999\n\n");

  if (!caskyn("Are you sure the Bytes need swapping? (Y/N)")) return 0;

  while (cask("Input file = ? (rtn to end)", filename, 80)) {
    ext =  filename + setext(filename, "    ", 80) + 1;

    if (!strcmp(ext, "rsp") || !strcmp(ext, "RSP")) {
      rsp_format();
    } else if (!strcmp(ext, "dmp") || !strcmp(ext, "DMP")) {
      dmp_format();
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
    } else if (!strcmp(ext, "sec") || !strcmp(ext, "SEC")) {
      sec_format();
    } else {
      printf(
	"*** ERROR -- Unrecognised file extension...\n"
	" As of rw97 and later, the following file types no longer require\n"
	"   byte-swapping, which is done automatically as needed:\n"
	"      .spe,  .eff,  .cal,  .tab  and  .cub.\n\n");
    }
  }
  return 0;
} /* main */

/* ======================================================================= */
int get_file_c(char *fn, char *data, int maxbytes)
{
  int  j;
  char *c;
  struct stat buf;
  FILE *fd;

  /* we need to make sure the filename is null terminated */
  if ((c = strchr(fn,' '))) *c = '\0';

  /* does file exist? */
  if (stat (fn, &buf)) {
    printf("Error: file does not exist: %s\n", fn);
    return 1;
  }
  /* will the file fit into memory block? */
  if (buf.st_size > maxbytes) {
    printf("Error: File %s is too big for get_file_c\n"
	   "       max size = %d, file size = %d.\n",
	   fn, maxbytes, (int) buf.st_size);
    return 1;
  }

  if (!(fd = open_readonly(fn))) return 1;
  j = fread(data, buf.st_size, 1, fd);
  fclose(fd);

  if (j != 1) {
    printf("Error during read, file %s.\n", fn);
    return 1;
  }
  return 0;
} /* get_file_c */

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
int swap8(void)
{
  char c;
 
  c = buf[pointer + 7];
  buf[pointer + 7] = buf[pointer];
  buf[pointer] = c;
  c = buf[pointer + 6];
  buf[pointer + 6] = buf[pointer + 1];
  buf[pointer + 1] = c;
  c = buf[pointer + 5];
  buf[pointer + 5] = buf[pointer + 2];
  buf[pointer + 2] = c;
  c = buf[pointer + 4];
  buf[pointer + 4] = buf[pointer + 3];
  buf[pointer + 3] = c;
  pointer += 8;
  return 0;
} /* swap8 */

/* ======================================================================= */
int swap4(void)
{
  char c;
 
  c = buf[pointer + 3];
  buf[pointer + 3] = buf[pointer];
  buf[pointer] = c;
  c = buf[pointer + 2];
  buf[pointer + 2] = buf[pointer + 1];
  buf[pointer + 1] = c;
  pointer += 4;
  return 0;
} /* swap4 */

/* ======================================================================= */
int swap2(void)
{
  char c;
 
  c = buf[pointer + 1];
  buf[pointer + 1] = buf[pointer];
  buf[pointer] = c;
  pointer += 2;
  return 0;
} /* swap2 */

/* ======================================================================= */
int get_i4(int *out)
{
#ifndef TEST_ONLY
  swap4();
#else
  pointer += 4;
#endif
  memcpy(out, buf+pointer-4, 4);
  return 0;
} /* get_i4 */

/* ======================================================================= */
int fix_record(void)
{
  swap4();
  swap4();
  return 0;
} /* fix_record */

/* ======================================================================= */
int rsp_format(void)
{
  char head[20] = "RESPONSE FILE VER. 1";
  int  i;
  char ans[80];

  if (get_file_c(filename, buf, max_bytes)) return 0;
  pointer = 0;

  swap4();
  get_char(ans, 40);
  if (strncmp(ans, head, 20)) {
    printf("ERROR - File cannot be read or is not\n"
	   "       a good response function file.\n");
    return 0;
  }
  /* skip over bytes at the beginning of next record */
  fix_record();
  swap4();
  for (i = 0; i < 514*25; ++i) swap4();
  swap4();

  /* write response functions to new file  */
  open_new_ofile();
  fwrite(buf, pointer, 1, ofile);
  fclose(ofile);
  return 0;
} /* rsp_format */

/* ======================================================================= */
int dmp_format(void)
{
  char head3[20] = "GELIFT ver. 6.1 dump";
  char title[40];
  int  i;

  if (get_file_c(filename, buf, max_bytes)) return 0;
  pointer = 0;
  swap4();
  get_char(title, 40);
  if (strncmp(title, head3, 20)) {
    printf("ERROR -- file is not a supported gf2/gf3 dump file\n");
    return 0;
  }

  fix_record();
  /* R(2, gf3gd.mch, 4) ||
     R(15, gf3gd.ppos, 4) ||
     R(1, &gf3gd.irelw, 4) ||
     R(51, gf3gd.pars, 4) ||
     R(51, gf3gd.freepars, 4) ||     120
     R(1, &gf3gd.npars, 4) ||
     R(1, &gf3gd.nfp, 4) ||
     R(51, gf3gd.errs, 4) ||
     R(1, &gf3gd.npks, 4) ||
     R(1, &gf3gd.irelpos, 4) ||       55
     R(15, gf3gd.areas, 4) ||
     R(15, gf3gd.dareas, 4) ||
     R(15, gf3gd.cents, 4) ||
     R(1, &ready, 4) ||
     R(1, &gf3gd.wtmode, 4) ||
     R(5, gf3gd.finest, 4) ||
     R(3, gf3gd.infix, 4) ||          55    */
  for (i = 0; i < 230; ++i) swap4();
  /* R(6, gf3gd.gain, 8) || */
  for (i = 0; i < 6; ++i) swap8();
  /* R(1, &gf3gd.ical, 4) ||
     R(1, &gf3gd.nterms, 4) ||
     R(1, &gf3gd.lox, 4) ||
     R(1, &gf3gd.numx, 4) ||
     R(1, &gf3gd.locnt, 4) ||
     R(1, &gf3gd.ncnts, 4) ||
     R(1, &gf3gd.iyaxis, 4) ||
     R(3, gf3gd.swpars, 4) ||
     R(1, &gf3gd.infixrw, 4) ||
     R(1, &gf3gd.infixw, 4) ||
     R(15, colormap, 4) ||
     R(1, &gf3gd.pkfind, 4) ||
     R(1, &gf3gd.ifwhm, 4) ||
     R(1, &gf3gd.isigma, 4) ||
     R(1, &gf3gd.ipercent, 4))      31
     R(300, gf3gd.stoc[0], 4) ||
     R(300, gf3gd.stodc[0], 4) ||
     R(300, gf3gd.stoa[0], 4) ||
     R(300, gf3gd.stoda[0], 4) ||
     R(300, gf3gd.stoe[0], 4) ||
     R(300, gf3gd.stode[0], 4) || 1800
     R(20, gf3gd.isto, 4) ||        20 */
  for (i = 0; i < 1851; ++i) swap4();
  /* R(20, gf3gd.namsto, 28)) {  20*28 */
  pointer += 20*28;
  swap4();

  open_new_ofile();
  fwrite(buf, pointer, 1, ofile);
  fclose(ofile);
  return 0;
} /* dmp_format */

/* ======================================================================= */
int gls_format(void)
{
  int level, gamma, nbands, ntlabel;
  int i, j, k;
  char zero[40];

  memset(buf, 0, max_bytes);
  memset(zero, 0, 40);
  if (get_file_c(filename, buf, max_bytes)) return 0;
  pointer = 0;
  swap4();
  swap4();
  get_i4(&level);
  get_i4(&gamma);
  get_i4(&nbands);
  for (i = 0; i < 9; ++i) swap4();

  printf("%d levels, %d gammas, %d bands.\n", level, gamma, nbands);
  /* skip over 8 bytes at the beginning of each unformatted record*/
  fix_record();
  for (j = 0; j < 6*level; ++j) swap4();
  for (j = 0; j < gamma; ++j) {
    swap4();
    swap4();
    pointer++;
    for (k = 0; k < 13; ++k) swap4();
  }
  for (j = 0; j < nbands; ++j) {
    pointer += 8;
    swap4();
    swap4();
  }
  fix_record();
  for (j = 0; j < nbands + 2*gamma + level; ++j) swap4();
  fix_record();

  if (!memcmp(buf+pointer, zero, 40)) {
    pointer -= 4;
  } else {
    for (j = 0; j < 8; ++j) swap4();
    get_i4(&ntlabel);
    printf("%d text labels.\n", ntlabel);

    fix_record();
    for (j = 0; j < 2*gamma + 8*level + 3*nbands; ++j) swap4();
    fix_record();
    for (j = 0; j < ntlabel; ++j) {
      pointer += 40;
      for (i = 0; i < 5; ++i) swap4();
    }
    swap4();
  }

  open_new_ofile();
  fwrite(buf, pointer, 1, ofile);
  fclose(ofile);
  return 0;
} /* gls_format */

/* ======================================================================= */
int lev_format(void)
{
  int i, j, numchs, matchs, nclook;
  char zero[40];

  memset(buf, 0, max_bytes);
  memset(zero, 0, 40);
  if (get_file_c(filename, buf, max_bytes)) return 0;
  pointer = 0;
  swap4();
  get_i4(&numchs);
  get_i4(&matchs);
  pointer += 80;

  fix_record();
  for (j = 0; j < numchs; ++j) swap4();
  fix_record();
  for (j = 0; j < matchs; ++j) swap4();
  fix_record();
  for (j = 0; j < 7*numchs; ++j) swap4();
  fix_record();
  for (j = 0; j < 2*numchs; ++j) swap4();
  fix_record();
  for (j = 0; j < 2*numchs; ++j) swap4();
  fix_record();
  for (j = 0; j < 10; ++j) swap4();
  fix_record();
  get_i4(&nclook);
  swap4();
  swap4();
  fix_record();
  for (i = 0; i < nclook; ++i) swap2();
  fix_record();

  if (memcmp(buf+pointer, zero, 40)) {
    get_i4(&j);
    fix_record();
    if (j) {
      for (j = 0; j < matchs; ++j) swap4();
      fix_record();
      if (memcmp(buf+pointer, zero, 40)) {
	for (j = 0; j < numchs + 1; ++j) swap4();
	fix_record();
      }
    }
    if (memcmp(buf+pointer, zero, 40)) {
      for (j = 0; j < 3; ++j) swap4();
      fix_record();
    }
  }
 
  pointer -= 4;
  open_new_ofile();
  fwrite(buf, pointer, 1, ofile);
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
    for (j = 0; j < numchs; ++j) swap4();
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
    for (i = 0; i < 2048; ++i) swap4();
    fseek(file, iy*reclen, SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  iy = 2054;
  fseek(file, iy*reclen, SEEK_SET);
  fread(buf, reclen, 1, file);
  pointer = 0;
  for (i = 0; i < 6; ++i) swap8();
  for (i = 0; i < 32; ++i) swap4();
  fseek(file, iy*reclen, SEEK_SET);
  fwrite(buf, reclen, 1, file);

  memcpy(&i, buf+132, 4);  /* esc_file_flags[2] */
  if (i) {
    for (iy = 2055; iy < 4103; ++iy) {
      fseek(file, iy*reclen, SEEK_SET);
      fread(buf, reclen, 1, file);
      pointer = 0;
      for (i = 0; i < 2048; ++i) swap4();
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
    for (i = 0; i < 4096; ++i) swap4();
    fseek(file, iy*reclen, SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  iy = 4102;
  fseek(file, iy*reclen, SEEK_SET);
  fread(buf, reclen, 1, file);
  pointer = 0;
  for (i = 0; i < 6; ++i) swap8();
  for (i = 0; i < 32; ++i) swap4();
  fseek(file, iy*reclen, SEEK_SET);
  fwrite(buf, reclen, 1, file);
 
  memcpy(&i, buf+132, 4);  /* esc_file_flags[2] */
  if (i) {
    for (iy = 4103; iy < 8199; ++iy) {
      fseek(file, iy*reclen, SEEK_SET);
      fread(buf, reclen, 1, file);
      pointer = 0;
      for (i = 0; i < 4096; ++i) swap4();
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

/* ======================================================================= */
int sec_format(void)
{
  int j, iy = 0, reclen;

  if (!inq_file(filename, &reclen)) {
    printf("File %s does not exist!\n", filename);
    return 0;
  }
  reclen = 8192*4;
  if (!caskyn("The existing direct access file will be overwritten.\n"
	      "   ...Proceed? (Y/N)")) return 0;
  file = fopen(filename, "r+");

  while (fread(buf, reclen, 1, file) == 1) {
    pointer = 0;
    for (j = 0; j < 8192; ++j) swap4();
    fseek(file, reclen*(iy++), SEEK_SET);
    fwrite(buf, reclen, 1, file);
  }

  fclose(file);
  return 0;
} /* sec_format */
