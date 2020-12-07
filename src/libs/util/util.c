#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>

#include "util.h"

/* ======================================================================= */
void swapb8(char *buf)
{
  char c;
  c = buf[7]; buf[7] = buf[0]; buf[0] = c;
  c = buf[6]; buf[6] = buf[1]; buf[1] = c;
  c = buf[5]; buf[5] = buf[2]; buf[2] = c;
  c = buf[4]; buf[4] = buf[3]; buf[3] = c;
} /* swapb8 */
void swapb4(char *buf)
{
  char c;
  c = buf[3]; buf[3] = buf[0]; buf[0] = c;
  c = buf[2]; buf[2] = buf[1]; buf[1] = c;
} /* swapb4 */
void swapb2(char *buf)
{
  char c;
  c = buf[1]; buf[1] = buf[0]; buf[0] = c;
} /* swapb2 */

/* ====================================================================== */
int datetime(char *dattim)
{
  /* encode current date and time into character string dattim
     desired format 22-Mar-94 HH:MM:SS
     dattim must be at least 20 chars long */
  time_t   now;

  now = time((time_t *)NULL);
  strftime(dattim, 19, "%d-%b-%y %H:%M:%S", localtime(&now));
  dattim[18] = '\0';
  return 0;
} /* datetime */

/* ====================================================================== */
int ffin(char *cin, int nc, float *out1, float *out2, float *out3)
{
  /* free format floating point input routine
     up to three real numbers (out1, out2, out3) decoded from cin
     input fields separated by commas or spaces
     cin = input character string
     nc  = no. of characters to decode in cin
     out1, out2, out3 = decoded numbers
     returns 0 for no error, 1 for invalid character in cin */

  int n, ihi, ilo;
  /* int i, test; */
  float out[3] = {0.f, 0.f, 0.f};
  char  *endptr, csave;

#define QUIT { *out1 = out[0]; *out2 = out[1]; *out3 = out[2]; cin[nc] = csave; return 0; }

  csave = cin[nc];
  cin[nc] = 0;
  if (nc <= 0) QUIT;
  ilo = 0;
  for (n = 0; n < 3; ++n) {
    while (ilo < nc && cin[ilo] == ' ') ilo++;
    if (ilo >= nc) QUIT;
    ihi = ilo;
    while (ihi < nc  && cin[ihi] != ' ' && cin[ihi] != ',') ihi++;
    if (ihi == ilo) continue;
    /* test = 0;
       for (i = ilo; i <= ihi; i++) {
       if ((cin[i] < '0' || cin[i] > '9') && cin[i] != '-') {
       if (cin[i] != '.' || test) {
       *out1 = out[0];
       *out2 = out[1];
       *out3 = out[2];
       return 1;
       }
       test = 1;
       }
       } */
    endptr = &cin[ihi];
    out[n] = strtod(&cin[ilo], &endptr);
    if (endptr < &cin[ihi]) { /* invalid char at endptr */
      out[n] = 0;
      *out1 = out[0];
      *out2 = out[1];
      *out3 = out[2];
      return 1;
    }
    ilo = ihi + 1;
    if (ilo >= nc) QUIT;
  }
  QUIT;
#undef QUIT
} /* ffin */

/* ======================================================================= */
int file_error(char *error_type, char *filename)
{
  /* write error message */
  /* cannot perform operation error_type on file filename */

  if (strlen(error_type) + strlen(filename) > 58) {
    warn1("ERROR - cannot %s file\n%s\n", error_type, filename);
  } else {
    warn1("ERROR - cannot %s file %s\n", error_type, filename);
  }
  return 0;
} /* file_error */

/* ======================================================================= */
int get_directory(char *env_name, char *dir_name, int maxchar)
{
  /* translate environment variable / logical name to give directory name
     maxchar = length of dir_name
     returns number of valid chars stored
     used for vms - unix portability */
  int  nc;
  char *en;

  if ((en = getenv(env_name))) {
    nc = strlen(en);
    strncpy(dir_name, en, maxchar-2);
    if (nc > maxchar-2) return -1;
#ifndef VMS
    strcat(dir_name,"/");
    nc++;
#endif
  } else {
    nc = 0;
    *dir_name = '\0';
  }
  return nc;
} /* get_directory */

/* ======================================================================= */
int get_file_rec(FILE *fd, void *data, int maxbytes, int swap_bytes)
{
  /* read one fortran-unformatted style binary record into data */
  /* for unix systems, swap_bytes controls how get_file_rec deals with
     swapping the bytes of the record length tags at the start and end
     of the records.  Set swap_bytes to
       0 to try to automatically sense if the bytes need swapping
       1 to force the byte swap, or
      -1 to force no byte swap */
  /* returns number of bytes read in record,
     or number of bytes * -1 if bytes need swapping,
     or 0 for error */
  unsigned int  reclen, j1, j2;

#ifdef VMS  /* vms */
  short rh[2];
  char  *buf;

  buf = data;
  reclen = rh[1] = 0;
  while (rh[1] < 2) {
    if (fread(rh, 2, 2, fd) != 2) return reclen;
    reclen += rh[0] - 2;
    /* tell("get_file_rec: reclen rh: %d %d %d\n", reclen, rh[0], rh[1]); */
    if (rh[1] < 0 || rh[1] > 3 ||
	rh[0] < 0 || rh[0] > 2044 ||
	(rh[1] < 2 && rh[0] != 2044)) goto ERR2;
    if (reclen > maxbytes)  goto ERR1;
    if (fread(buf, (int) (rh[0] - 2), 1, fd) != 1) goto ERR2;
    buf += 2042;
    /* if reclen is odd, skip over extra (padding) byte */
    if (2*(reclen>>1) != reclen) fread(&j2, 1, 1, fd);
  }
  return reclen;

#else /* unix */

  if (fread(&reclen, 4, 1, fd) != 1) return 0;
  if (reclen == 0) return 0;
  j1 = reclen;
  if ((swap_bytes == 1) ||
      (swap_bytes == 0 && reclen >= 65536)) swapb4((char *) &reclen);
  if (reclen > maxbytes) goto ERR1;
  if (fread(data, reclen, 1, fd) != 1 ||
      fread(&j2, 4, 1, fd) != 1) goto ERR2;
  /* if (j1 != j2) goto ERR2; */
  if (reclen == j1) return reclen;
  return (-reclen);
#endif

 ERR1:
  warn("ERROR: record is too big for get_file_rec\n"
       "       max size = %d, record size = %d.\n",
       maxbytes, reclen);
  return 0;
 ERR2:
  warn("ERROR during read in get_file_rec.\n");
  return 0;
} /* get_file_rec */

/* ======================================================================= */
int put_file_rec(FILE *fd, void *data, int numbytes)
{
  /* write one fortran-unformatted style binary record into data */
  /* returns 1 for error */

#ifdef VMS  /* vms */
  int   j1;
  short rh[2];
  char  *buf;

  buf = data;
  j1 = numbytes;
  if (numbytes <= 2042) {
    rh[0] = numbytes + 2; rh[1] = 3;
  } else {
    rh[0] = 2044; rh[1] = 1;
    while (j1 > 2042) {
      if (fwrite(rh, 2, 2, fd) != 2 ||
	  fwrite(buf, 2042, 1, fd) != 1) return 1;
       rh[1] = 0; j1 -= 2042; buf += 2042;
    }
    rh[0] = j1 + 2; rh[1] = 2;
  }
  if (fwrite(rh, 2, 2, fd) != 2 ||
      fwrite(buf, j1, 1, fd) != 1) return 1;
  /* if numbytes is odd, write an extra (padding) byte */
  if (2*(numbytes>>1) != numbytes) {
    j1 = 0;
    fwrite(&j1, 1, 1, fd);
  }
    
#else /* unix */

  if (fwrite(&numbytes, 4, 1, fd) != 1 ||
      fwrite(data, numbytes, 1, fd) != 1 ||
      fwrite(&numbytes, 4, 1, fd) != 1) return 1;
#endif
  return 0;
} /*put_file_rec */

/* ====================================================================== */
int inin(char *cin, int nc, int *out1, int *out2, int *out3)
{
  /* free format int input routine
     up to three intgers (out1, out2, out3) decoded from cin
     input fields separated by commas or spaces
     cin = input character string
     nc  = no. of characters to decode in cin
     out1, out2, out3 = decoded numbers
     returns 0 for no error, 1 for invalid character in cin */

  int n, ihi, ilo;
  int out[3] = {0, 0, 0};
  char *endptr;

#define QUIT { *out1 = out[0]; *out2 = out[1]; *out3 = out[2]; return 0; }

  if (nc <= 0) QUIT;
  ilo = 0;
  for (n = 0; n < 3; ++n) {
    while (ilo < nc && cin[ilo] == ' ') ilo++;
    if (ilo >= nc) QUIT;
    ihi = ilo;
    while (ihi < nc  && cin[ihi] != ' ' && cin[ihi] != ',') ihi++;
    if (ihi == ilo) continue;
    endptr = &cin[ihi];
    out[n] = strtol(&cin[ilo], &endptr, 0);
    if (endptr < &cin[ihi]) { /* invalid char at endptr */
      *out1 = out[0];
      *out2 = out[1];
      *out3 = out[2];
      return 1;
    }
    ilo = ihi + 1;
    if (ilo >= nc) QUIT;
  }
  QUIT;
#undef QUIT
} /* inin */

/* ======================================================================= */
int inq_file(char *filename, int *reclen)
{
  /* inquire for file existence and record length in longwords
     returns 0 for file not exists, 1 for file exists */

  int  ext;
  char jfile[80];
  struct stat statbuf;

  *reclen = 0;
  if (stat(filename, &statbuf)) return 0;

  ext = 0;
  strncpy(jfile, filename, 80);
  ext = setext(jfile, "    ", 80);
  if (!strcmp(&jfile[ext], ".mat") ||
      !strcmp(&jfile[ext], ".MAT") ||
      !strcmp(&jfile[ext], ".esc") ||
      !strcmp(&jfile[ext], ".ESC")) {
    *reclen = 2048;
  } else if (!strcmp(&jfile[ext], ".spn") ||
	     !strcmp(&jfile[ext], ".SPN") ||
	     !strcmp(&jfile[ext], ".m4b") ||
	     !strcmp(&jfile[ext], ".M4B") ||
	     !strcmp(&jfile[ext], ".e4k") ||
	     !strcmp(&jfile[ext], ".E4K")) {
    *reclen = 4096;
  } else if (!strcmp(&jfile[ext], ".cub") ||
	     !strcmp(&jfile[ext], ".CUB")) {
    *reclen = 256;
  } else if (!strcmp(&jfile[ext], ".2dp") ||
	     !strcmp(&jfile[ext], ".2DP")) {
    if (statbuf.st_size <= 0) {
      *reclen = 0;
    } else {
      *reclen = (int) (0.5 + sqrt((float) (statbuf.st_size/4)));
    }
  }
  return 1;
} /* inq_file */

/* ======================================================================= */
FILE *open_new_file(char *filename, int force_open)
{
  /* safely open a new file
     filename: name of file to be opened
     force_open = 0 : allow return value NULL for no file opened
     force_open = 1 : require that a file be opened
     force_open = 2 : allow overwrite of an existing file */

  int  j, nc, jext, fn_error = 0;
  char tfn[80], *owf;
  FILE *file = NULL;

  strncpy(tfn, filename, 80);
  jext = setext(tfn, "", 80);

  while (1) {

#ifdef VMS
    fn_error = 0;
#else
    if (force_open < 2 && (fn_error = inq_file(filename, &j))) {
      /* file exists */
      owf = getenv("RADWARE_OVERWRITE_FILE");
      if (owf && (*owf == 'Y' || *owf == 'y')) {
	tell("Overwriting file %s\n", filename);
      } else if (owf && (*owf == 'N' || *owf == 'n')) {
	tell("File %s already exists.\n", filename);
	fn_error = 1;
      } else {
	fn_error = !askyn("File %s already exists - overwrite? (Y/N)", filename);
      }
    }
#endif

    /* open file w+ */
    if (!fn_error) {
      if ((file = fopen(filename, "w+"))) return file;
      file_error("open or overwrite", filename);
    }

    while (1) {
      nc = askfn(filename, 72, "", &tfn[jext], "New file name = ?");
      if (nc == 0 && !force_open) return NULL;
      if ((nc > 0 && filename[nc-1] != '/') ||
	  askyn("Are you sure you want file %s? (Y/N)", filename)) break;
    }
  }
} /* open_new_file */

/* ======================================================================= */
FILE *open_new_unf(char *filename)
{
  return open_new_file(filename, 1);
} /* open_new_unf */

/* ======================================================================= */
FILE *open_pr_file(char *filename)
{
  return open_new_file(filename, 1);
} /* open_pr_file */

/* ======================================================================= */
FILE *open_save_file(char *filename, int save_rename)
{
  /* safely open a new file, with the option to rename 
        an existing file filename to filename~
     filename: name of file to be opened
     save_rename = 0/1 : 1 to rename existing file */

  char newfn[256];
  int  j;
  FILE *file;

  if (save_rename && inq_file(filename, &j)) {
    strncpy(newfn, filename, 254);
    strcat(newfn, "~"); 
    j = rename(filename, newfn);
  }
  if ((file = fopen(filename, "w+"))) return file;
  return open_new_file(filename, 1);
} /* open_save_file */

/* ======================================================================= */
FILE *open_readonly(char *filename)
{
  /* open old file for READONLY (if possible)
     filename  = file name
     provided for VMS compatibility */

  FILE *file;

  if (!(file = fopen(filename, "r"))) file_error("open", filename);
  return file;
} /* open_readonly */

/* ======================================================================= */
int pr_and_del_file(char *filename)
{
/* print and delete file filename */

  static char printer[80] = "";
  int  i;
  char message[256], ans[80];

  if (inq_file(filename, &i)) {
#ifdef VMS
#define STR1 "Enter any required PRINT command flags\n"\
	     "     (e.g.: \"/Queue = xxx\", rtn for \"%s\")"
#define STR2 "print %s %s"
#else
#define STR1 "Enter any required lpr flags\n"\
	     "     (e.g.: \"-Pps02 -h\", rtn for \"%s\")"
#define STR2 "lpr %s %s"
#endif
    if (ask(ans, 80, STR1, printer)) strcpy(printer, ans);
    sprintf(message, STR2, printer, filename);
    tell("Printing with system command: %s\n", message);
#undef STR1
#undef STR2
    if (system(message) || remove(filename)) {
      warn("*** Printing failed; your file was saved. ***\n");
      return 1;
    }
  } else {
    warn("There is no file to print.\n");
    return 1;
  }
  return 0;
} /* pr_and_del_file */

/* ======================================================================= */
int read_cal_file(char *filnam, char *title, int *iorder, double *gain)
{
  /* read energy calib parameters from .aca/.cal file = filnam
     into title, iorder, gain
     returns 1 for open/read error
     default file extension = .aca, .cal */

  char cbuf[128];
  int  j, rl;
  char fn1[80], fn2[80], line[120], *c;
  FILE *file;
  struct stat buf;

  strncpy(fn1, filnam, 80);
  strncpy(fn2, filnam, 80);
  j = setext(fn1, ".aca", 80);
  j = setext(fn2, ".cal", 80);
  if (strcmp(fn1+j, ".aca") && strcmp(fn2+j, ".cal")) {
    tell("ERROR - filename .ext must be .aca or .cal!\n");
    return 1;
  } else if (strcmp(fn1+j, ".aca") && stat(fn2, &buf)) {
    tell("ERROR - file %s does not exist!\n", fn2);
    return 1;
  } else if (strcmp(fn2+j, ".cal") && stat(fn1, &buf)) {
    tell("ERROR - file %s does not exist!\n", fn1);
    return 1;
  } else if (stat(fn1, &buf) && stat(fn2, &buf)) {
    tell("ERROR - neither file %s\n"
	   "            nor file %s exists!\n", fn1, fn2);
    return 1;
  }

#define ERR { file_error("read", fn1); fclose(file); return 1; }
  /* first try ascii cal file (.aca) */
  if (!strcmp(fn1+j, ".aca") &&
      (file = fopen(fn1, "r"))) {
    if (!(fgets(line, 120, file)) ||
	!strstr(line, "ENCAL OUTPUT FILE") ||
	!(fgets(line, 120, file)) ||
	!strncpy(title, line, 80) ||
	!(fgets(line, 120, file))) ERR;
    while ((c = strchr(line, 'D'))) *c = 'E'; /* convert fortran-stye format */
    if (sscanf(line, "%d %lE %lE %lE", iorder, gain, gain+1, gain+2) != 4 ||
	!(fgets(line, 120, file))) ERR;
    while ((c = strchr(line, 'D'))) *c = 'E'; /* convert fortran-stye format */
    if (sscanf(line, "%lE %lE %lE", gain+3, gain+4, gain+5) != 3) ERR;
    fclose(file);
#undef ERR

  /* next try binary cal file (.cal) */
  } else {
    /* read parameters from disk file
       OPEN(ILU,FILE=FILNAM,FORM='UNFORMATTED',STATUS='OLD',ERR=200)
       READ(ILU) TEST, TITLE
       READ(ILU) IORDER, GAIN */
    if (!(file = open_readonly(fn2))) return 1;
    rl = get_file_rec(file, cbuf, 128, 0);
    if ((rl != 98 && rl != -98) ||
	strncmp(cbuf, " ENCAL OUTPUT FILE", 18)) {
      file_error("read", fn2);
      tell("...File is not an ENCAL output file.\n");
      fclose(file);
      return 1;
    }
    strncpy(title, &cbuf[18], 80);
    rl = get_file_rec(file, cbuf, 128, 0);
    fclose(file);
    if (rl != 52 && rl != -52) {
      file_error("read", fn2);
      return 1;
    }
    if (rl < 0) {
      /* file has wrong byte order - swap bytes */
      tell("*** Swapping bytes read from file %s\n", fn2);
      swapb4(cbuf);
      for (j = 0; j < 6; ++j) {
	swapb8(cbuf + 4 + 8*j);
      }
    }
    memcpy(iorder, cbuf, 4);
    for (j = 0; j < 6; ++j) {
      memcpy(&gain[j], cbuf + 4 + 8*j, 8);
    }
  }
  if (strlen(title) > 79) title[79] = '\0';
  if ((c = strchr(title,'\n'))) *c = '\0';
  tell("%s\n", title);
  return 0;
} /* read_cal_file */

/* ======================================================================= */
int read_eff_file(char *filnam, char *title, float *pars)
{
  /* read efficiency parameters from .aef/.eff file = filnam
     into title, pars
     returns 1 for open/read erro
     default file extension = .aef, .eff */

  char cbuf[128];
  int  j, rl;
  char fn1[80], fn2[80], line[120], *c;
  FILE *file;
  struct stat buf;

  strncpy(fn1, filnam, 80);
  strncpy(fn2, filnam, 80);
  j = setext(fn1, ".aef", 80);
  j = setext(fn2, ".eff", 80);
  if (strcmp(fn1+j, ".aef") && strcmp(fn2+j, ".eff")) {
    tell("ERROR - filename .ext must be .aef or .eff!\n");
    return 1;
  } else if (strcmp(fn1+j, ".aef") && stat(fn2, &buf)) {
    tell("ERROR - file %s does not exist!\n", fn2);
    return 1;
  } else if (strcmp(fn2+j, ".eff") && stat(fn1, &buf)) {
    tell("ERROR - file %s does not exist!\n", fn1);
    return 1;
  } else if (stat(fn1, &buf) && stat(fn2, &buf)) {
    tell("ERROR - neither file %s\n"
	   "            nor file %s exists!\n", fn1, fn2);
    return 1;
  }

  /* first try ascii eff file (.aef) */
  if (!strcmp(fn1+j, ".aef") &&
      (file = fopen(fn1, "r"))) {
    if (!(fgets(line, 120, file)) ||
	!strstr(line, "EFFIT PARAMETER FILE") ||
	!strncpy(title, line+1, 40) ||
	!(fgets(line, 120, file)) ||
	(sscanf(line, "%f %f %f %f %f",
		pars, pars+1, pars+2, pars+3, pars+4) != 5) ||
	!(fgets(line, 120, file)) ||
	(sscanf(line, "%f %f %f %f %f",
		pars+5, pars+6, pars+7, pars+8, pars+9) != 5)) {
      file_error("read", fn1);
      fclose(file);
      return 1;
    }
    fclose(file);

  /* next try binary eff file (.eff) */
  } else {
    /* read parameters from disk file
       OPEN(ILU,FILE=FILNAM,FORM='UNFORMATTED',STATUS='OLD')
       READ(ILU) TITLE
       READ(ILU) PARS */
    /* first record has 40 or 42 bytes. */

    if (!(file = open_readonly(fn2))) return 1;
    rl = get_file_rec(file, cbuf, 128, 0);
    if (rl != 40  && rl != 42 &&
	rl != -40 && rl != -42) {
      file_error("read", fn2);
      fclose(file);
      return 1;
    }
    strncpy(title, cbuf, 40);

    rl = get_file_rec(file, cbuf, 128, 0);
    fclose(file);
    if (rl != 40 && rl != -40) {
      file_error("read", fn2);
      return 1;
    }
    if (rl < 0) {
      /* file has wrong byte order - swap bytes */
      tell("*** Swapping bytes read from file %s\n", fn2);
      for (j = 0; j < 10; ++j) {
	swapb4(cbuf + 4*j);
      }
    }
    for (j = 0; j < 10; ++j) {
      memcpy(&pars[j], cbuf + 4*j, 4);
    }
  }
  if (strlen(title) > 39) title[39] = '\0';
  if ((c = strchr(title,'\n'))) *c = '\0';
  tell("%s\n", title);
  return 0;
} /* read_eff_file */

/* ======================================================================= */
int read_tab_file(char *filnam, int *nclook, int *lookmin, int *lookmax,
		  short *looktab, int dimlook)
{
  /* read lookup table from .tab file = filnam
     into array looktab of dimension dimlook
     Nclook = number of channels read
     lookmin, lookmax = min/max values in lookup table
     returns 1 for open/read error
     default file extension = .tab */

  char cbuf[128];
  int  j, rl, swap = -1;
  FILE *file;

  setext(filnam, ".tab", 80);

  /* OPEN (ILU,FILE=FILNAM,FORM='UNFORMATTED',STATUS='OLD')
     READ (ILU) NCLOOK,LOOKMIN,LOOKMAX
     READ (ILU) (LOOKTAB(I),I=1,NCLOOK) */

  if (!(file = open_readonly(filnam))) return 1;
  rl = get_file_rec(file, cbuf, 128, 0);
  if (rl != 12  && rl != -12) {
    file_error("read lookup-table from", filnam);
    fclose(file);
    return 1;
  }
  if (rl < 0) {
    /* file has wrong byte order - swap bytes */
    tell("*** Swapping bytes read from file %s\n", filnam);
    swap = 1;
    for (j = 0; j < 3; ++j) {
      swapb4(cbuf + 4*j);
    }
  }
  memcpy(nclook,  cbuf,     4);
  memcpy(lookmin, cbuf + 4, 4);
  memcpy(lookmax, cbuf + 8, 4);
  tell(" NCLook, LookMin, LookMax = %d, %d, %d\n",
	 *nclook, *lookmin, *lookmax);
  if (*nclook < 2 || *nclook > dimlook) {
    file_error("read lookup-table from", filnam);
    fclose(file);
    return 1;
  }

  rl = get_file_rec(file, looktab, 2*dimlook, swap);
  fclose(file);
  if (rl != 2*(*nclook) && rl != -2*(*nclook)) {
    file_error("read lookup-table from", filnam);
    return 1;
  }
  if (rl < 0) {
    /* file has wrong byte order - swap bytes */
    for (j = 0; j < *nclook; ++j) {
      swapb2((char *) (looktab + j));
    }
  }
  return 0;
} /* read_tab_file */

/* ====================================================================== */
int setext(char *filnam, const char *cext, int filnam_len)
{
  /* set default extension of filename filnam to cext
     leading spaces are first removed from filnam
     if extension is present, it is left unchanged
     if no extension is present, cext is used
     returned value pointer to the dot of the .ext
     cext should include the dot plus a three-letter extension */

  int nc, iext;

  /* remove leading spaces from filnam */
  nc = strlen(filnam);
  if (nc > filnam_len) nc = filnam_len;
  while (nc > 0 && filnam[0] == ' ') {
    memmove(filnam, filnam+1, nc--);
    filnam[nc] = '\0';
  }
  /* remove trailing spaces from filnam */
  while (nc > 0 && filnam[nc-1] == ' ') {
    filnam[--nc] = '\0';
  }
  /* look for file extension in filnam
     if there is none, put it to cext */
  iext = 0;
  if (nc > 0) {
    for (iext = nc-1;
	 (iext > 0 &&
	  filnam[iext] != ']' &&
	  filnam[iext] != '/' &&
	  filnam[iext] != ':');
	 iext--) {
      if (filnam[iext] == '.') return iext;
    }
    iext = nc;
  }
  strncpy(&filnam[iext], cext, filnam_len - iext - 1);
  return iext;
} /* setext */

/* ======================================================================= */
int wrresult(char *out, float value, float err, int minlen)
{
  int   n;
  char  *c;

  /* append nicely-formatted " value(error)" to string out
     minlen = requested minimum length of out at completion
     returns new length of out string */

  c = out + strlen(out);
  if (err < 0.00005f) {
    sprintf(c, " %.4f", value);
    c = out + strlen(out);
    while (*(c-1) == '0' && *(c-2) == '0') c--;
    strcpy(c, "(0)");
  } else if (err < 0.0025f) {
    sprintf(c, " %.4f(%.0f)", value, err * 10000.0f);
  } else if (err < 0.025f) {
    sprintf(c, " %.3f(%.0f)", value, err * 1000.0f);
  } else if (err < 0.25f) {
    sprintf(c, " %.2f(%.0f)", value, err * 100.0f);
  } else if (err < 2.5f) {
    sprintf(c, " %.1f(%.0f)", value, err * 10.0f);
  } else {
    sprintf(c, " %.0f(%.0f)", value, err);
  }
  for (n = strlen(out); n < minlen; n++) {
    strcat(out," ");
  }
  return n;
} /* wrresult */
