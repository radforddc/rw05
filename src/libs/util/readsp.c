#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

int datread(char *fn, float *sp, char *namesp, int *numch, int idimsp);
int rmsread(char *fn, float *sp, char *namesp, int *numch, int idimsp);
int sptread(char *fn, float *sp, char *namesp, int *numch, int idimsp);

/* ====================================================================== */
int readsp(char *filnam, float *sp, char *namesp, int *numch, int idimsp)
{
  /* subroutine to read spectrum from file (or id) = filnam
     into array sp, of dimension idimsp
     numch = number of channels read
     namesp = name of spectrum (character*8)
     default file extension = .spe
     spmode = 0/1/2 for .spe/.spk/.mat files
     spmode = 3 for .spn files */

  /* 2000/03/08:  added support for ORNL .his files
     spmode = 4 for .his files */
  /* 2000/10/31:  added support for .sec files (like 8192-ch .spn files)
     spmode = 5 for .sec files */
  /* 2006/04/10:  added support for .dat files (simple float files)
     spmode = 6 for .dat files */
  /* 2016/12/16:  added .rms files (complex radware multi-spectrum files)
     spmode = 7 for .rms files */
  /* 2017/09/29:  added .spt files (spectrum text)
     spmode = 8 for .spt files */

  static int spmode = 0;
  int i, nc, in1, in2, in3;
#ifdef HISFILES
  int hisread(char *fn, float *sp, char *namesp, int *numch, int idimsp);
#endif

  /* if in .spk, .mat or .spn mode, and filnam has ints,
     call spkread or matread */
  if (!strcmp(filnam+2, "/C") || !strcmp(filnam+2, "/c")) {
    return matread(filnam, sp, namesp, numch, idimsp, &spmode);
  }
  nc = strlen(filnam);
  if (spmode != 0 &&
      !inin(filnam, nc, &in1, &in2, &in3)) {
    if (spmode == 1) {
      return spkread(filnam, sp, namesp, numch, idimsp);
    } else if (spmode == 2 || spmode == 3 || spmode == 5) {
      return matread(filnam, sp, namesp, numch, idimsp, &spmode);
    } else if (spmode == 4) {
#ifdef HISFILES
      return hisread(filnam, sp, namesp, numch, idimsp);
#else
      warn(".his files not supported;"
	   " recompile with HISFILES flag defined.\n");
#endif
    } else if (spmode == 6) {
      return datread(filnam, sp, namesp, numch, idimsp);
    } else if (spmode == 7) {
      return rmsread(filnam, sp, namesp, numch, idimsp);
    }
  }
  /* remove leading spaces from filnam
     look for file extension in filnam
     if there is none, put it to .spe */

  // test to see if .spe file exist; if not, try .rms
  char test[80];
  FILE *fp;
  strncpy(test, filnam, 80);
  i = setext(test, ".spe", 80);
  if (!(fp = fopen(test, "r"))) {
    strncpy(test+i, ".rms", 4);
    if ((fp = fopen(test, "r"))) strncpy(filnam, test, 80);  // if .rms file exists, use it instead
  }
  if (fp) fclose(fp);
    
  i = setext(filnam, ".spe", 80);
  /* if it is .spk, .mat, .spn or .m4b, call spkread or matread */
  if (!strcmp(filnam + i, ".SPK") ||
      !strcmp(filnam + i, ".spk")) {
    spmode = 1;
    return spkread(filnam, sp, namesp, numch, idimsp);
  } else if (!strcmp(filnam + i, ".MAT") ||
	     !strcmp(filnam + i, ".mat")) {
    spmode = 2;
    return matread(filnam, sp, namesp, numch, idimsp, &spmode);
  } else if (!strcmp(filnam + i, ".SPN") ||
	     !strcmp(filnam + i, ".spn") ||
	     !strcmp(filnam + i, ".M4B") ||
	     !strcmp(filnam + i, ".m4b")) {
    spmode = 3;
    return matread(filnam, sp, namesp, numch, idimsp, &spmode);
  } else if (!strcmp(filnam + i, ".HIS") ||
	     !strcmp(filnam + i, ".his")) {
#ifdef HISFILES
    spmode = 4;
    return hisread(filnam, sp, namesp, numch, idimsp);
#else
    warn(".his files not supported;"
	 " recompile with HISFILES flag defined.\n");
#endif
  } else if (!strcmp(filnam + i, ".SEC") ||
	     !strcmp(filnam + i, ".sec")) {
    spmode = 5;
    return matread(filnam, sp, namesp, numch, idimsp, &spmode);
  } else if (!strcmp(filnam + i, ".DAT") ||
	     !strcmp(filnam + i, ".dat")) {
    spmode = 6;
    return datread(filnam, sp, namesp, numch, idimsp);
  } else if (!strcmp(filnam + i, ".RMS") ||
	     !strcmp(filnam + i, ".rms")) {
    spmode = 7;
    return rmsread(filnam, sp, namesp, numch, idimsp);
  } else if (!strcmp(filnam + i, ".SPT") ||
	     !strcmp(filnam + i, ".spt")) {
    spmode = 8;
    return sptread(filnam, sp, namesp, numch, idimsp);
  }
  /* read spectrum in standard gf3 format */
  spmode = 0;
  return read_spe_file(filnam, sp, namesp, numch, idimsp);
} /* readsp */

#define lrecl 2048  /* logical record length in bytes */

/* ================================================ */
int locateID(int *idir, int ilo, int ihi, int idn)
{
  /* search spkio directory for id numbers */

  int n;

  if (ihi < ilo) return 0;
  for (n = 2*ilo + 2; n <= 2*ihi + 2; n += 2) {
    if (idir[n] == idn) return n;
  }
  return 0;
} /* locateID */

/* ================================================ */
int dospkio(int mode, FILE *spkfile, int *idat, int ia, int nw)
{
  /* mode  = 1,2 for read, write
     ia    = first half-word on file to xfer
     nw    = # of half-words to xfer */

  fseek(spkfile, 2*ia - 2, SEEK_SET);

  if (mode == 1) {
    if (fread(idat, 2*nw, 1, spkfile) != 1) {
      warn("ERROR - cannot read from spk file.\n");
      return 1;
    }
  } else {
    if (fwrite(idat, 2*nw, 1, spkfile) != 1) {
      warn("ERROR - cannot write to spk file.\n");
      return 1;
    }
  }
  return 0;
} /* dospkio */

/* ================================================ */
int spkio(int mode, FILE *spkfile, int idn, int *ihed, int maxh,
	  int *idat, int *ndx, int nch)
{
/* mode = 0,1,2 says initialize, read, write
   mode = 3 says list the directory        (no longer supported)
   mode = 4 says delete idn from directory (no longer supported)
   mode = 5 says display the directory     (no longer supported)
   mode = 6 says return id-list in idat (idat[0] = # of id's)

   spkfile = file descr. of open file
   idn  = requested id #
   ihed - array to contain header
   maxh = maximum length of ihed in half-words
   idat - array to contain data
   ndx  - array to contain indices of 1st channel to xfer
   nch  = # of channels to xfer

   returned values:
   0 says all ok
   1 says requested id not found
   2 says invalid value of "ndx"
   3 says directory overflow
   4 says i/o error of some sort
   5 says illegal request mode
   6 says id to be deleted not in directory (not used)
   7 says id to be stored already in directory
   8 says file not open

   idir - contains the directory
   idir[0], idir[1] contains 8-character "data type"
   idir[2] contains "nid" (# of spectra on file)
   idir[3] contains "nxwd" (next half-word # on file to use)
   idir[4] contains id #
   idir[5] contains half-word # where header starts */

  static int idir[512];
#define cdir ((char *)idir)
#define nid  idir[2]
#define nxwd idir[3]

  /* header structure:
   word#    contents
       0    id #
   1 - 3    parameter label (from .his-file)
   4 - 6    reserved for (date - time)
       7    bytes/channel = -4  (no longer needed but set to -4)
                                (so that old spkio will get err)
       8    header length (half words)
       9    data length (half words)          (no longer used)
      10    data dimensionality (# parms = 1) (no longer used)
      11    hist length (max # chans to be returned)
      12    length of raw    parameter (pwr of 2)
      13    length of scaled parameter (pwr of 2)
      14    data record blksize (bytes) for mag tapes
            (0 implies one contiguous data record)
      15    min non-zero channel #
      16    max non-zero channel #
   17-19    calibration constants (up to 3)
   20-21    reserved
   22-31    title (40 bytes) */

  int ihed2[32];
#define id   ihed2[0]
#define nwdh ihed2[8]
#define lenc ihed2[11]
#define lraw ihed2[12]
#define lsca ihed2[13]
#define minc ihed2[15]
#define maxc ihed2[16]

  int loch, mind, maxd, nwdd, minr, maxr, stat, i, n;
  int ncmax, ia, nc, ii, ih8, ih16, ih17, idx, nwd, nwn;

  if (!spkfile) return 8;   /* no open file */
  if (mode < 0 || (mode > 2 && mode != 6)) return 5; /* invalid mode */

  if (mode == 0) {   /* initialize */
    /* reset the directory */
    memset(idir, 0, lrecl);
    strncpy(cdir, "HHIRFSPK", 8);
    nxwd = 1025;
    rewind(spkfile);
    if (fwrite(idir, lrecl, 1, spkfile) != 1) {
      warn("SPKIO ERROR - write failure for directory.\n");
      return 4;
    }
    return 0;
  }

  /* read in directory for all modes but init */
  if (dospkio(1, spkfile, idir, 1, 1024)) {
    warn("SPKIO ERROR - read failure for directory.\n");
    return 4;
  }

  if (mode == 6) {   /* return id-list in idat (idat[0] = # of id's) */
    idat[0] = nid;
    i = 4;
    for (n = 1; n <= nid; n++) {
      idat[n] = idir[i];
      i += 2;
    }

  } else if (mode == 1) {  /* mode = input (read) */
    /* test for requested id */
    if ((n = locateID(idir, 1, nid, idn)) <= 0) {
      warn("SPKIO ERROR - no such spectrum ID!\n");
      return 1;
    }
    nwn = idir[n+1];
    loch = nwn;

    /* read first 64 half-words of header into ihed2 */
    if (dospkio(1, spkfile, ihed2, nwn, 64)) {
      warn("SPKIO ERROR - cannot read header(1)\n"
	   "%d %d\n", nwn, 64);
     return 4;
    }
    /* read header into ihed */
    nwd = maxh;
    if (nwd > nwdh) nwd = nwdh;
    if (dospkio(1, spkfile, ihed, nwn, nwd)) {
      warn("SPKIO ERROR - cannot read header(2)\n"
	   "%d %d\n", nwn, nwd);
      return 4;
    }

    /* compute nwn = 1st half-word # on disk to xfer
               nc  = # of channels to xfer
               idx = starting index in idat */

    if (ihed[9] <= 0) ihed[9] = 2*(maxc + 1);      /* # half-wds data */
    if (ihed[13] > ihed[11]) ihed[11] = ihed[13];  /* modify # chans */
    if (nch <= 0) return 0;      /* no histogram data requested */

    if (ihed[7] >= 0) {  /*+++ old spk-file form */

      if (*ndx <= 0 || *ndx > lenc) {  /* illegal ndx */
      warn("SPKIO ERROR - illegal ndx = %d   lenc :%d\n", *ndx, lenc);
      return 2;
    }
      nwn   = loch + nwdh + 2*(*ndx - 1);      /* 1st half-wd# on disk */
      nc    = nch;               /* # channels requested */
      ncmax = lenc - *ndx + 1;   /* max # chans available */
      idx   = 0;                 /* 1st index in idat */

    } else {             /*+++ new spk-file form */

      memset(idat, 0, nch * 4);  /* zero requested # chans */
      mind = minc + 1;	         /* min data index */
      maxd = maxc + 1;           /* max data index */
      minr = *ndx;               /* min index requested */
      maxr = *ndx + nch - 1;     /* max index requested */
      if (minr > maxd || maxr < mind) return 0;  /* test for no overlap */
      if (minr <= mind) {
	nwn = loch + nwdh;       /* start at 1st element of data */
	idx = mind - minr;       /* 1st index in idat */
	nc = nch - idx + 1;      /* # chans to read */
	ncmax = maxd - mind + 1; /* max # chans available */
      } else {                   /* all above min */
	nwn = loch + nwdh + 2*(minr - mind);  /* 1st half-wd# on disk */
	idx = 0;                 /* 1st index in idat */
	nc = nch;                /* # chans to read */
	ncmax = maxd - minr + 1; /* max # chans available */
      }
    }
    
    if (nc > ncmax) nc = ncmax;
    if (dospkio(1, spkfile, &idat[idx], nwn, 2*nc)) {
      warn("SPKIO ERROR - cannot read spectrum\n"
	   "%d %d \n", nwn, 2*nc);
      return 4;
    }
    return 0;

  } else if (mode == 2) {       /* mode = output (write) */
    if (nid >= 254) return 3;    /* directory full */
    /* see if id already exists */
    id = ihed[0];                /* id number */
    if (nid > 0 &&               /* test for directory not empty */
	(n = locateID(idir, 1, nid, id)) != 0) { /* id exists */
      warn("SPKIO ERROR - spectrum ID already exists!\n");
      return 7;
    }

    nc = ihed[11];               /* # chans from header */
    nwdh = ((ihed[8] + 1) / 2) << 1;    /* # half-wds of header to store */

    /* add to file and update directory */
    ++nid;                       /* inc # of id's stored */
    ii = (nid << 1) + 2;         /* directory index for id */
    idir[ii] = id;               /* store id in directory */
    idir[ii+1] = nxwd;           /* store disk loc in dir */
    for (i = 0; i < nc; i++) {   /* loop on # chans to */
      if (idat[i] != 0) break;   /*   find 1st non-zero data */
    }
    if (i >= nc) {   /* !if all 0, store 1 chan */
      minc = 0;      /* min chan # */
      maxc = 0;      /* max chan # */
      ia    = 0;     /* idat index */
      nwdd  = 2;     /* # half-wds to store */
    } else {
      minc = i;      /* set min chan# */
      ia = i;        /* init loop to find last non-zero data */
      for (n = nc - 1; n > ia; n--) { /* reverse loop on idat */
	if (idat[n] != 0) break;      /* test for non-zero */
      }
      maxc = n;                       /* set max chan# */
      nwdd = 2*(maxc - minc + 1);     /* # half-words of data to store */
    }

    /* now store data */
    idx  = minc + 1;   /* start index in idat */
    ih8  = ihed[7];    /* save ihed(8) */
    ih16 = ihed[15];   /* save ihed(16) */
    ih17 = ihed[16];   /* save ihed(17) */
    ihed[7]  = -4;     /* so old spkio gets error */
    ihed[15] = minc;   /* store minc in ihed(16) */
    ihed[16] = maxc;   /* store maxc in ihed(17) */
    /* store header */
    stat = dospkio(2, spkfile, ihed, nxwd, nwdh);
    ihed[7]  = ih8;    /* restore ihed(8) */
    ihed[15] = ih16;   /* restore ihed(16) */
    ihed[16] = ih17;   /* restore ihed(17) */
    if (stat != 0) return 4;  /* test for store error */
    nxwd += nwdh;      /* next disk address */
    /* store data */
    if (dospkio(2, spkfile, &idat[idx], nxwd, nwdd)) return 4;
    /* store directory */
    if (dospkio(2, spkfile, idir, 1, 1024)) return 4;
  }

  return 0;     
} /* spkio */
#undef cdir
#undef nid
#undef nxwd
#undef id
#undef nwdh
#undef lenc
#undef lraw
#undef lsca
#undef minc
#undef maxc

/* ================================================ */
FILE *spkman(char *mode, char *namf, char *iacp)
{
  /* open/create spk-files for input/output
     mode = "OPEN", "CREA"
     namf = filename
     iacp = "RO", "RW" = access mode
     returns valid file descriptor on success, NULL on failure */

  int  iext;
  char cnam[120];
  FILE *file = NULL;

  strncpy(cnam, namf, 120);          /* copy filename */
  iext = setext(cnam, ".spk", 120);  /* locate .spk extension */
  if (strcmp(cnam+iext, ".SPK") &&   /* test for .SPK ext */
      strcmp(cnam+iext, ".spk")) {   /* or  for .spk ext */
    warn("SPKMAN ERROR - filename extension must be .spk!\n");

  } else if (!strncmp(mode, "CREA", 4)) {
    /* create a new spk-file and open for output */
    if (!(file = fopen(cnam, "w+"))) {
      warn("SPKMAN ERROR - cannot create new file %s\n", cnam);
    }
    /* init spk-file */
    else if (spkio(0, file, 0, 0, 0, 0, 0, 0)) {
      warn("SPKMAN ERROR - cannot initialize file %s\n", cnam);
      fclose(file);
      return NULL;
    }

  } else if (!strncmp(iacp, "RW", 2)) {
    if (!(file = fopen(cnam, "r+")))
      warn("SPKMAN ERROR - cannot open (read/write) file %s\n", cnam);

  } else {
    if (!(file = fopen(cnam, "r")))
      warn("SPKMAN ERROR - cannot open (readonly) file %s\n", cnam);
  }
  return file;
} /* spkman */
#undef lrecl

/* ======================================================================= */
int spkread(char *fn, float *sp, char *namesp, int *numch, int idimsp)
{
  /* subroutine to read spectrum from .spk file
     into array sp, of dimension idimsp
     fn = file name, or id of spectrum to be read
     numch = number of channels read
     namesp = name of spectrum (char*8, set to id)
     file extension must be .spk (ornl format) */

  int  ihed[32], isav[16384];
  int  i, i1, i2, id, nc, ns, in2, in3;
  char ans[80];
  static char filnam[80] = "";
  static int ndx[4] = { 1,0,0,0 };
  int  maxh = 64;
  FILE *spkfile;

  if (!inin(fn, strlen(fn), &id, &in2, &in3)) {
    if (!(spkfile = spkman("OPEN", filnam, "RO  "))) return 1;
  } else {
    /* fn = file name
       open .spk file and ask "display directory?" */
    if (!(spkfile = spkman("OPEN", fn, "RO  "))) return 1;
    strncpy(filnam, fn, 80);
    if (askyn("Display directory? (Y/N)")) {
      /* list file directory */
      if (spkio(6, spkfile, id, ihed, maxh, isav, ndx, 0)) goto READERR;
      ns = isav[0];
      if (isav[ns] - isav[1] == ns - 1) {
	tell("%d spectra, numbers %d through %d\n", ns, isav[1], isav[ns]);
      } else {
	tell("%d spectra, numbers:\n", ns);
	for (i1 = 1; i1 < ns; i1 += 12) {
	  i2 = i1 + 11;
	  if (i2 > ns) i2 = ns;
	  for (i = i1; i <= i2; i++) {
	    tell(" %5d", isav[i]);
	  }
	  tell("\n");
	}
      }
    }
   /* ask for spectrum id */
    while ((nc = ask(ans, 80, "Spectrum ID = ?")) &&
	   inin(ans, nc, &id, &in2, &in3));
    if (nc == 0) {
      fclose(spkfile);
      return 1;
    }
  }

  if (id < 1) goto READERR;
  /* read spectrum & get number of chs in spectrum */
  if (spkio(1, spkfile, id, ihed, maxh, isav, ndx, idimsp)) goto READERR;
  *numch = ihed[11];
  if (*numch > idimsp) {
    *numch = idimsp;
    tell(" First %d chs only taken.\n", idimsp);
  }
  fclose(spkfile);
  /* convert to float format */
  for (i = 0; i < *numch; ++i) {
    sp[i] = (float) isav[i];
  }
  strncpy(fn, filnam, 80);
  sprintf(namesp, "%8d", id);
  return 0;

 READERR:
  /* file_error("read", filnam); */
  fclose(spkfile);
  return 1;
} /* spkread */

#ifdef HISFILES
/* ======================================================================= */
int hisread(char *fn, float *sp, char *namesp, int *numch, int idimsp)
{
  /* subroutine to read spectrum from .his file
     into array sp, of dimension idimsp
     fn = file name, or id of spectrum to be read
     numch = number of channels read
     namesp = name of spectrum (char*8, set to id)
     file extension must be .his (ornl format) */

  short ihed[64];
  int   isav[16384];
  int   i, id, nc, in2, in3, ierr = 0;
  char  ans[80], mser[80];
  static char filnam[80] = "";
  static int  ndx[4] = { 1,1,0,0 }, lud = 1, luh = 2, first = 1, fileopen = 0;

  extern void hisman_(char *mode, char *namf, int *lud, int *luh,
		      char *acp, int *ierr);
  extern void hisio_(char *mode, int *lud, int *luh, int *ihn, int *ndx,
		     int *nch, short *ihed, int *idat, int *ierr, char *mser);
  extern struct {
    char mssg[112], namprog[8];
    int  logut, logup;
    char lisflg[4], msgf[4];
  } lll_;


  if (first) {
    first = 0;
    memset(lll_.mssg, ' ', 112);
    strncpy(lll_.namprog, "gf3     ", 8);
    lll_.logut = 6;
    lll_.logup = 0;
    strncpy(lll_.lisflg,  "LOF ", 4);
    strncpy(lll_.msgf,    "    ", 4);
  }

  if (!inin(fn, strlen(fn), &id, &in2, &in3)) {
    if (!fileopen) {
      hisman_("OPEN", filnam, &lud, &luh, "RO  ", &ierr);
      if (ierr) {
	file_error("open", filnam);
	return 1;
      }
      fileopen = 1;
      hisio_("INIT", &lud, &luh, &id, ndx, &idimsp, ihed, isav, &ierr, mser);
      if (ierr) {
	mser[40] = 0;
	warn("%s\n", mser);
	goto READERR;
      }
    }
  } else {
    /* fn = file name
       open .his file */
    if (fileopen && (strcmp(fn, filnam))) {
      hisman_("CLOS", filnam, &lud, &luh, "RO  ", &ierr);
      fileopen = 0;
    }
    if (!fileopen) {
      hisman_("OPEN", fn, &lud, &luh, "RO  ", &ierr);
      if (ierr) {
	file_error("open", fn);
	return 1;
      }
      fileopen = 1;
      hisio_("INIT", &lud, &luh, &id, ndx, &idimsp, ihed, isav, &ierr, mser);
      if (ierr) {
	mser[40] = 0;
	warn("%s\n", mser);
	goto READERR;
      }
    }
    strncpy(filnam, fn, 80);
   /* ask for spectrum id */
    while ((nc = ask(ans, 80, "Spectrum ID = ?")) &&
	   inin(ans, nc, &id, &in2, &in3));
    if (nc == 0) return 1;
  }

  if (id < 1) goto READERR;
  /* read spectrum & get number of chs in spectrum */
  hisio_("READ", &lud, &luh, &id, ndx, &idimsp, ihed, isav, &ierr, mser);
  if (ierr) {
    mser[40] = 0;
    warn("%s\n", mser);
    goto READERR;
  }
  tell(" %d %d %d %d\n", ihed[0], ihed[1], ihed[10], ihed[11]);
  *numch = (int) ihed[10];
  if (ihed[0] > 1) *numch *= (int) ihed[11];
  if (*numch > idimsp) {
    *numch = idimsp;
    tell(" First %d chs only taken.\n", idimsp);
  }
  /* convert to float format */
  if (ihed[1] == 1) {
    for (i = 0; i < *numch; ++i) {
      sp[i] = (float) *((unsigned short *)isav + i);
    }
  } else {
    for (i = 0; i < *numch; ++i) {
      sp[i] = (float) isav[i];
    }
  }
  strncpy(fn, filnam, 80);
  sprintf(namesp, "%8d", id);
  return 0;

 READERR:
  file_error("read", filnam);
  return 1;
} /* hisread */
#endif

/* ======================================================================= */
int datread(char *fn, float *sp, char *namesp, int *numch, int idimsp)
{
  /* subroutine to read spectrum from .dat file (simple string of floats)
     into array sp, of dimension idimsp
     fn = file name to be read
     numch = number of channels read
     namesp = name of spectrum (char*8, set to id)
     file extension must be .dat */

  FILE *file;

  if (!(file = open_readonly(fn))) return 1;
  *numch = fread(sp, 4, idimsp, file);
  fclose(file);

  strncpy(namesp, fn, 8);
  if (*numch < 1) return 1;
  return 0;
} /* datread */


/* ======================================================================= */
int sptread(char *fn, float *sp, char *namesp, int *numch, int idimsp)
{
  /* subroutine to read spectrum from .spt file (special text format)
     into array sp, of dimension idimsp
     fn = file name to be read
     numch = number of channels read
     namesp = name of spectrum (char*8, set to id)
     file extension must be .dat */

  FILE  *file;
  char  line[256];
  int   k = 0;
  float f;

  if (!(file = open_readonly(fn))) return 1;

  // skip to line with "[DATA]"
  while (fgets(line, sizeof(line), file) && !strstr(line, "[DATA]")) ;
  // read integers, one per line
  while (fgets(line, sizeof(line), file) &&
         sscanf(line, "%f", &f) && k < idimsp-1) sp[k++] = f+0.5;

  *numch = k;
  fclose(file);
  strncpy(namesp, fn, 8);
  if (*numch < 1) return 1;
  return 0;
} /* sptread */

#define ERR {tell("ERROR reading file %s at %d!\n", filnam, ftell(file)); return 1;}
/* ======================================================================= */
int rmsread(char *fn, float *sp, char *namesp, int *numch, int idimsp)
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

  int  i, jrecl, in3, ilo, ihi, nc, iy, nsp, dptr, spdir[2];
  int  id, spmode, xlen, expand, nextptr=0;
  int  x0, nx;
  char ans[80], txt[64], *c;
  static char filnam[80];
  FILE *file;
  char *modes[5] = {"shorts", "ints", "floats", "doubles", "bytes"};


  *numch = 0;
  isp = (int *) fsp;
  ssp = (short *) fsp;

  /* look for filename followed by integer chan. no. limits */
  if ((c = strstr(fn, "\n"))) *c = 0;
  if ((c = strstr(fn, "\r"))) *c = 0;
  if ((c = strstr(fn, ".rms")) &&
      (nc = strlen(c)-5) > 0 &&
      inin(c+5, nc, &ilo, &ihi, &in3) == 0) {
    *(c+4) = 0; // terminate file name
    strncpy(filnam, fn, 80);
  } else {
    /* look for int chan. no. limits */
    nc = strlen(fn);
    if (inin(fn, nc, &ilo, &ihi, &in3)) {
      /* not ints; fn = file name, check .rms file exists*/
      if (!inq_file(fn, &jrecl)) {
        file_error("open", fn);
        tell("...File does not exist.\n");
        return 1;
      }
      strncpy(filnam, fn, 80);
      /* ask for range of spectrum IDs to be added together */
      while ((nc = ask(ans, 80, "Type range of spectrum IDs (lo,hi) ?")) &&
             inin(ans, nc, &ilo, &ihi, &in3));
      if (nc == 0) return 0;
    }
  }
  if (ihi == 0) ihi = ilo;
  if (ihi < ilo) {
    i = ilo;
    ilo = ihi;
    ihi = i;
  }
  if (ilo < 0) {
    tell("Bad y-gate channel limits: %d %d", ilo, ihi);
    return 1;
  }
  if (!(file = open_readonly(filnam))) return 1;

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
    // tell(" >> Looking for ID %d in %s\n", iy, filnam);
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
        tell("Spectrum ID %d not found in file %s!\n", iy, filnam);
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
    /*   warn("Spectrum ID mismatch (%d, %d) in file %s!\n", iy, id, filnam); */
    /*   return 1; */
    /* } */
    if (spmode > 3) ERR;  // limit to simple short/int/float for now

    tell("Sp ID %d in file %s; %d %s; %s\n", iy, filnam, xlen, modes[spmode-1], txt);
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
  strncpy(fn, filnam, 80);
  if (ihi > ilo) {
    sprintf(namesp, "%4d%4d", ilo, ihi);
  } else {
    strncpy(namesp, txt, 8);
  }
  return 0;
#undef ERR
} /* rmsread */
