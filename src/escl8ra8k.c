#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "minig.h"
#include "escl8r.h"


/* DCR 2011-4-2 added command NY+, NY- */


/* declared in esclev.c */
extern char listnam[55];

static int reclen;  /* record length of direct-access .esc or .e4k data file */
static int gate_label_h_offset = 0;

#define BG2D \
 (xxgd.bspec[0][iy] * xxgd.bspec[1][ix] +\
  xxgd.bspec[1][iy] * xxgd.bspec[2][ix] +\
  xxgd.bspec[3][iy] * xxgd.bspec[4][ix] +\
  xxgd.bspec[4][iy] * xxgd.bspec[5][ix])

#define SUBV \
 if ((float) abs(ix - iy) < xxgd.v_width) {\
   r1 = (float) (ix - iy) / xxgd.v_width;\
   y += xxgd.v_depth[iy] * (1.0f - r1 * r1);\
 }

#define GETDAT \
 int junk = fseek(filez[2], iy*reclen, SEEK_SET);\
 junk = fread(xxgd.rdata, 4, xxgd.numchs, filez[2]);\
 if (xxgd.esc_file_flags[2]) {\
   junk = fseek(filez[2], (iy + xxgd.numchs + 7)*reclen, SEEK_SET);\
   junk = fread(xxgd.rdata2, 4, xxgd.numchs, filez[2]);\
 } else {\
   memcpy(xxgd.rdata2, xxgd.rdata, reclen);\
 }

#define SUBDAT \
 y = xxgd.rdata[ix] - BG2D;\
 SUBV;\
 r1 = elgd.bg_err * (xxgd.rdata[ix] - y);\
 dat = xxgd.rdata2[ix] + r1 * r1;

#define SAVEPARS \
 fseek(filez[2], (xxgd.numchs + 6)*reclen, SEEK_SET);\
 fwrite(&xxgd.gain[0], 8, 6, filez[2]);\
 fwrite(&xxgd.nterms, 4, 1, filez[2]);\
 fwrite(&xxgd.eff_pars[0], 4, 9, filez[2]);\
 fwrite(&elgd.finest[0], 4, 5, filez[2]);\
 fwrite(&elgd.swpars[0], 4, 3, filez[2]);\
 fwrite(&elgd.h_norm, 4, 1, filez[2]);\
 fwrite(&xxgd.esc_file_flags[0], 4, 10, filez[2]);\
 fwrite(&elgd.bg_err, 4, 1, filez[2]);\
 fwrite(&elgd.encal_err, 4, 1, filez[2]);\
 fwrite(&elgd.effcal_err, 4, 1, filez[2]);\
 memset(xxgd.rdata, 0, reclen);\
 fwrite(xxgd.rdata, reclen-176, 1, filez[2]); /* zeroes to fill reclen */\
 fflush(filez[2]);

#define ESCL8R
#include "esclevb.c"

/* ======================================================================= */
int e_gls_exec(char *ans, int nc)
{
  float x1, y1, x2, y2;
  int   iwin1, iwin2, i;

  /* this subroutine decodes and executes the commands */
  /* convert lower case to upper case characters */
  for (i = 0; i < 2; ++i) {
    ans[i] = toupper(ans[i]);
  }
  if (!gls_exec2(ans)) return 0;
  /* expand spectrum display or level scheme display using cursor */
  if (!strncmp(ans, "EX", 2)) {
    while (1) {
      retic3(&x1, &y1, ans, &iwin1);
      retic3(&x2, &y2, ans, &iwin2);
      if (iwin1 == iwin2) break;
      tell("Click twice in the window you want to expand.\n");
    }
    if (iwin1 == 1) {
      elgd.numx = fabs(x2 - x1);
      if (elgd.numx < 10) elgd.numx = 10;
      elgd.lox = (x1 < x2 ? x1 : x2);
      disp_dat(1);
    } else {
      glsgd.x0 = (x1 < x2 ? x1 : x2);
      glsgd.hix = (x1 > x2 ? x1 : x2);
      glsgd.y0 = (y1 < y2 ? y1 : y2);
      glsgd.hiy = (y1 > y2 ? y1 : y2);
      display_gls(0);
    }
  } else if (!strncmp(ans, "HE", 2)) {
    gls_help();
    escl8r_help();
  } else {
    escl8r_exec(ans, nc);
  }
  return 0;
} /* e_gls_exec */

/* ======================================================================= */
int escl8r_exec(char *ans, int nc)
{
  float h, x, y, fj1, fj2;
  int   i, idata, j1, j2, in, in2;
  char  dattim[20], message[256];

  /* this subroutine decodes and executes the commands */
  /* convert lower case to upper case characters */
  for (i = 0; i < 2; ++i) {
    ans[i] = toupper(ans[i]);
  }
  if (!strncmp(ans, "BG", 2)) {
    undo_esclev(-1, 4);  /* undo last gate changes */
  } else if (!strncmp(ans, "CA", 2)) {
    if (!askyn("Change energy and efficiency calibrations? (Y/N)")) return 0;
    /* get new calibrations */
    get_cal();
    calc_peaks();
    SAVEPARS;
  } else if (!strncmp(ans, "CB", 2)) {
    if (!askyn("Change background? (Y/N)")) return 0;
    get_bkgnd();
    SAVEPARS;
  } else if (!strncmp(ans, "CF", 2)) {
    comfil(ans, nc);
  } else if (!strncmp(ans, "CR", 2)) {
    curse();
  } else if (!strncmp(ans, "DC", 2)) {
    if (elgd.disp_calc == 2) {
      elgd.disp_calc = 0;
      tell("Calculated spectra will now not be displayed.\n");
    } else if (elgd.disp_calc == 0) {
      elgd.disp_calc = 1;
      tell("Calculated spectra and differences will now be displayed.\n");
    } else {
      elgd.disp_calc = 2;
      tell("Calculated spectra (but not differences) will now be displayed.\n");
    }
    disp_dat(1);
  } else if (!strncmp(ans, "DS", 2)) {
    if (ans[2] == '1') {
      disp_dat(2);
    } else {
      disp_dat(1);
    }
  } else if (!strncmp(ans, "ES", 2)) {
    energy_sum();
  } else if (!strncmp(ans, "FE", 2)) {
    hilite(-1);
    fit_egam();
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FI", 2)) {
    hilite(-1);
    fit_igam();
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FB", 2)) {
    hilite(-1);
    fit_both();
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FG", 2)) {
    undo_esclev(1, 4);  /* redo last gate changes */
  } else if (!strncmp(ans, "FW", 2)) {
    fit_width_pars();
  } else if (!strncmp(ans, "G ", 2) || !strcmp(ans, "G")) {
    hilite(-1);
    if (cur_gate()) return 0;
    disp_dat(0);
  } else if (!strncmp(ans, "GL", 2)) {
    hilite(-1);
    if (gate_sum_list(ans)) goto ERR;
    disp_dat(0);
  } else if (*ans == 'L') {
    listgam();
  } else if (!strncmp(ans, "MU", 2)) {
    tell("Lower channel limit = ?\n");
    retic(&x, &y, ans);
    elgd.lox = (int) x;
    if (elgd.lox > xxgd.numchs - elgd.numx) elgd.lox = xxgd.numchs - elgd.numx;
    disp_dat(1);
  } else if (!strncmp(ans, "MD", 2)) {
    tell("Upper channel limit = ?\n");
    retic(&x, &y, ans);
    elgd.lox = (int) x - elgd.numx;
    if (elgd.lox < 0) elgd.lox = 0;
    disp_dat(1);
  } else if (!strncmp(ans, "MY", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    elgd.min_ny = idata;
  } else if (!strncmp(ans, "NG", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    if (idata <= 0) {
      while (1) {
	nc = ask(ans, 80, "Number of gates to be displayed = ? (rtn for 1)");
	idata = 1;
	if (nc == 0 || !inin(ans, nc, &idata, &j1, &j2)) break;
      }
    }
    elgd.ngd = idata;
    if (elgd.ngd < 1) elgd.ngd = 1;
    if (elgd.ngd > 10) elgd.ngd = 10;
    if (elgd.ngd <= 2) elgd.n_stored_sp = 0;
    disp_dat(1);
  } else if (!strncmp(ans, "NX", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    elgd.numx = idata;
  } else if (!strncmp(ans, "NY", 2)) {
    if (strchr(ans, '+')) {  // new command to increase y-scale (counts)
      elgd.multy *= 1.5f;
      elgd.ncnts = elgd.ncnts*3/2;
      disp_dat(3);
    } else if (strchr(ans, '-')) {  // new command to decrease y-scale (counts)
      elgd.multy /= 1.5f;
      elgd.ncnts = elgd.ncnts*2/3;
      disp_dat(3);
    } else if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) {
      goto ERR;
    } else {
      elgd.multy = 1.0f;
      elgd.ncnts = idata;
      disp_dat(3);
    }
  } else if (!strncmp(ans, "PF", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &in, &in2)) goto ERR;
    while (idata <= 0) {
      if (!(elgd.pkfind = askyn("Do you want peak find activated? (Y/N)")))
	return 0;
      while ((nc = ask(ans, 80,
		       "Values for SIGMA, %% = ? (rtn for %i %i)",
		       elgd.isigma, elgd.ipercent)) &&
	     inin(ans, nc, &idata, &in, &in2));
      if (nc == 0) return 0;
    }
    elgd.pkfind = 1;
    elgd.isigma = idata;
    if (in != 0) elgd.ipercent = in;
    tell("Peak find activated; SIGMA, %% = %i %i\n", elgd.isigma, elgd.ipercent);
    return 0;
  } else if (!strncmp(ans, "PR", 2)) {
    hilite(-1);
    project();
    disp_dat(0);
  } else if (!strncmp(ans, "PS", 2)) {
    if (!askyn("Change peak shape and FWHM parameters? (Y/N)")) return 0;
    /* get new calibrations */
    get_shapes();
    calc_peaks();
    SAVEPARS;
  } else if (!strncmp(ans, "RC", 2)) {
    undo_esclev(1, 3);  /* redo last undone .esc changes */
  } else if (!strncmp(ans, "RL", 2)) {
    wrlists(ans);
  } else if (!strncmp(ans, "RN", 2)) {
    while ((nc = ask(ans, 80,
		     "Height normalization factor = ? (rtn for %g)",
		     elgd.h_norm)) &&
	   ffin(ans, nc, &h, &fj1, &fj2));
    if (nc == 0 || h < 1.0e-9) return 0;
    save_esclev_now(3);
    elgd.h_norm = h;
    calc_peaks();
    SAVEPARS;
    datetime(dattim);
    fprintf(filez[26], "%s:  2D Scaling factor = %g\n", dattim, elgd.h_norm);
    fflush(filez[26]);
  } else if (!strncmp(ans, "RS", 2)) {
    rw_saved_spectra(ans);
    disp_dat(0);
  } else if (!strncmp(ans, "SHC", 3) ||
	     !strncmp(ans, "SHc", 3)) {
    /* SHC; generate hardcopy of spectrum window */
    hcopy();
  } else if (!strncmp(ans, "SS", 2)) {
    rw_saved_spectra(ans);
  } else if (!strncmp(ans, "ST", 2)) {
    if (!askyn("Are you sure you want to exit? (Y/N)")) return 0;
    read_write_gls("WRITE");
    if (prfile) {
      fclose(prfile);
      while (1) {
	nc = ask(ans, 1, "Enter P/D/S to Print/Delete/Save log file ?");
	if (nc == 0 || *ans == 'D' || *ans == 'd') {
#ifdef VMS
	  sprintf(message, "delete/noconfirm/nolog %s;0", prfilnam);
#else
	  sprintf(message, "rm -f %s", prfilnam);
#endif
	  if (system(message)) ;
	} else if (*ans == 'P' || *ans == 'p') {
	  pr_and_del_file(prfilnam);
	} else if (*ans != 'S' && *ans != 's') {
	  continue;
	}
	break;
      }
    }
    save_xwg("escl8r__");
    exit(0);
  } else if (!strncmp(ans, "SU", 2)) {
    sum_cur();
  } else if (!strncmp(ans, "UC", 2)) {
    undo_esclev(-1, 3);  /* undo .esc changes */
  } else if (!strncmp(ans, "WL", 2)) {
    wrlists(ans);
  } else if (!strncmp(ans, "WS", 2)) {
    write_sp(ans, nc);
  } else if (!strncmp(ans, "XA", 2)) {
    /* XA: modify X0,NX */
    while ((nc = ask(ans, 80, "X0 = ? (return for %i)", elgd.lox)) &&
	   (inin(ans, nc, &idata, &j1, &j2) ||
	    idata + 2 > xxgd.numchs));
    if (nc) elgd.lox = idata;
    while ((nc = ask(ans, 80, "NX = ? (return for %i)", elgd.numx)) &&
	   (inin(ans, nc, &idata, &j1, &j2) || idata < 0));
    if (nc) elgd.numx = idata;
  } else if (!strncmp(ans, "X0", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2) ||
	idata + 2 > xxgd.numchs) goto ERR;
    elgd.lox = idata;
  } else if (*ans == 'X') {
    if (examine(ans, nc)) goto ERR;
  } else if (!strncmp(ans, "YA", 2)) {
    /* YA: modify Y0,NY */
    while ((nc = ask(ans, 80, "Y0 = ? (return for %i)", elgd.locnt)) &&
	   (inin(ans, nc, &idata, &j1, &j2)));
    if (nc) elgd.locnt = idata;
    while ((nc = ask(ans, 80, "NY = ? (return for %i)", elgd.ncnts)) &&
	   (inin(ans, nc, &idata, &j1, &j2) || idata < 0));
    if (nc) elgd.ncnts = idata;
  } else if (!strncmp(ans, "Y0", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    elgd.locnt = idata;
  } else if (*ans == '+' || *ans == '-' || *ans == '.') {
    if (combine(ans, nc)) goto ERR;
    disp_dat(0);
  } else if (*ans == '*') {
    if (multiply(ans, nc)) goto ERR;
    disp_dat(0);
  } else {
    /* look for gate by gamma energy */
    hilite(-1);
    if (num_gate(ans, nc)) goto ERR;
    disp_dat(0);
  }
  return 0;
  /* command cannot be recognized */
 ERR:
  warn1("Bad energy or command.\n");
  return 1;
} /* escl8r_exec */

/* ======================================================================= */
int escl8r_help(void)
{
  static char fmt_10[] = "\n"
    " g [w]     set gate at energy \"g\",      [width \"w\"*FWHM]\n"
    " +g [w]    add gate at energy \"g\",      [width \"w\"*FWHM]\n"
    " -g [w]    subtract gate at energy \"g\", [width \"w\"*FWHM]\n"
    " .g [w]    \"and\" gate at energy \"g\",    [width \"w\"*FWHM]\n"
    "            (take arithmetic minimum of two spectra for each channel)\n"
    " *x        multiply current gate by factor \"x\"\n"
    " X g [w]   examine gate for peak at energy \"g\", [width \"w\"*FWHM]\n"
    " L         define/edit gamma-ray lists\n"
    " RL,WL     read/write gamma-ray lists from/to a disk file.\n"
    " GL l [w]  sum gates for all peaks listed in list \"l\", [width \"w\"*FWHM]\n"
    " XL l [w]  examine gate for peaks listed in list \"l\", [width \"w\"*FWHM]\n"
    " PR        get one-dimensional projection of data\n"
    " G         select gate using cursor\n"
    " ES        check for energy sums with cursor\n"
    " WS        write spectra to gf3-type disk file\n"
    " SS [file]   save spectra to a special .gat disk file\n"
    " RS [file]   read previously saved spectra from a .gat disk file\n"
    " +RS [file]  add previously saved gate\n"
    " -RS [file]  subtract previously saved gate n"
    " .RS [file]  \"and\" previously saved gate\n\n";
  static char fmt_20[] = "\n"
    " NX,NY  |\n"
    " X0,Y0  | set up display axes, as for gf3\n"
    " XA,YA  |\n"
    " NY+    increase y scale / shrink spectrum vertically"
    " NY-    decrease y scale / expand spectrum vertically"
    " MY n   set Minimum value of Y autoscale (counts) to n\n"
    " EX     expand spectrum display using cursor\n"
    " MU     move display up using cursor\n"
    " MD     move display down using cursor\n"
    " NG n   display n gates on graphics screen, n = 1 to 10\n"
    " PF     set up or disable peak search for display\n"
    " DS     redisplay spectrum\n"
    " DS1    redisplay entire spectrum\n"
    " DC     toggle on/off Display of Calculated spectra\n"
    " CR     call cursor\n"
    " SHC    create HardCopy of Spectrum window\n"
    " SU     sum spectrum over channels using cursor\n\n";
  static char fmt_30[] = "\n"
    " CA     get new energy and efficiency calibrations\n"
    " CB     get new background spectra etc\n"
    " CF     command file (same options as gf3)\n"
    " PS     modify peak shape and width parameters\n"
    " RN     renormalise ratio of level scheme intensities to peak areas\n"
    " FB     fit both energies and intensities of transitions\n"
    " FE     fit energies of level scheme transitions\n"
    " FI     fit intensities of level scheme transitions\n"
    " FW     fit peak width parameters to current gate spectrum\n"
     "UC     undo the most recent gates/changes to the escl8r file\n"
     "RC     redo the most recently undone gates/changes to the escl8r file\n"
    " ST     stop and exit program\n\n";

  warn(fmt_10);
  warn(fmt_20);
  warn(fmt_30);
  return 0;
} /* escl8r_help */

/* ======================================================================= */
int e_gls_init(int argc, char **argv)
{
  float d1, d2, fj1, fj2;
  int   nx, ny, jext, i, j, i1, i2, nc;
  char  filenm[80], dattim[20], ans[80], *infile1 = "", *infile2 = "";

  /* initialise */
  set_xwg("escl8r__");
  initg2(&nx, &ny);
  finig();
  tell("\n\n\n"
       "  ESCL8R_GLS Version 2.0    D. C. Radford   Oct 1999\n\n"
       "     Welcome....\n\n");

  /* parse input parameters */
  prfile = 0;
  *prfilnam = 0;
  if (argc > 1) {
    infile1 = argv[1];
    if (!strcmp(infile1, "-l")) {
      infile1 = "";
      strcpy(prfilnam, "escl8r");
      if (argc > 2) strcpy(prfilnam, argv[2]);
      if (argc > 3) infile1 = argv[3];
      if (argc > 4) infile2 = argv[4];
    } else if (argc > 2) {
      infile2 = argv[2];
      if (!strcmp(infile2, "-l")) {
	infile2 = "";
	strcpy(prfilnam, "escl8r");
	if (argc > 3) strcpy(prfilnam, argv[3]);
	if (argc > 4) infile2 = argv[4];
      } else if (argc > 3) {
	strcpy(prfilnam, "escl8r");
	if (argc > 4) strcpy(prfilnam, argv[4]);
      }
    }
  }
  /* ask for and open level scheme file */
  gls_init(infile1);
  /* open log (print) file */
  if (*prfilnam) {
    setext(prfilnam, ".log", 80);
    prfile = open_new_file(prfilnam, 1);
  }

  /* esc_file_flags[0]   : 0/1 for matrix compressed / not compressed
     esc_file_flags[1]   : 1 for new versions with systematic errors
                              on background and energy, effic calibs
     esc_file_flags[2]   : 0 for standard type file,
                           1 for file with extra data on statistical errors
                             (allows use on difference-matrices,
                              2d-unfolded matrices etc.)
     esc_file_flags[3-9] : for future use */

  /* initial values of some parameters */
  elgd.lox = elgd.hich = elgd.numx = elgd.locnt = elgd.ncnts = 0;
  elgd.iyaxis = -1;
  elgd.ngd = elgd.pkfind = elgd.disp_calc = 1;
  elgd.multy = 1.0f;
  memset(elgd.nlist, 0, 55*4);
  elgd.finest[0] = 10.0f;
  elgd.finest[1] =  elgd.finest[2] =  elgd.finest[3] =  elgd.finest[4] = 0.0f;
  elgd.bg_err = 0.05f;
  elgd.encal_err = elgd.effcal_err = 0.03f;
  elgd.min_ny = 100.0f;
  elgd.n_stored_sp = 0;
  elgd.isigma = 4;
  elgd.ipercent = 5;
  elgd.swpars[0] = 9.0f;
  elgd.swpars[1] = 0.002f;
  elgd.swpars[2] = 0.0f;
  for (j=0; j<15; j++) elgd.colormap[j] = j+1;

  xxgd.v_width = -1.0f;
  for (j = 0; j < 1024; ++j) {
    xxgd.v_depth[j] = 0.0f;
  }

  /* ask for and open matrix data file */
  if ((nc = strlen(infile2)) > 0) {
    strcpy(filenm, infile2);
  } else {
  START:
    nc = askfn(filenm, 80, "", ".esc",
	      "Compressed matrix data file = ? (rtn to create new file)");
  }
  if (nc == 0) {
    prep();
  } else {
    jext = setext(filenm, ".esc", 80);
    if (!strcmp(filenm + jext, ".e8k") ||
	!strcmp(filenm + jext, ".E8K")) {
      if (MAXCHS < 8192) {
	warn1("ERROR; Trying to use a .e8k data set, but MAXCHS is defined as %d.\n"
	      "Edit escl8r.h to modify MAXCHS to be at least 8192, then recompile.\n",
	      MAXCHS);
	goto START;
      }
      xxgd.numchs = 8192;
    } else if (!strcmp(filenm + jext, ".e4k") ||
	       !strcmp(filenm + jext, ".E4K")) {
      xxgd.numchs = 4096;
    } else {
      xxgd.numchs = 2048;
    }
    reclen = xxgd.numchs*4;
    if (!(filez[2] = fopen(filenm, "r+"))) {
      warn1(" File does not exist.\n");
      goto START;
    }
    /* read projections, calibrations etc. from file */
    if (fseek(filez[2], xxgd.numchs*reclen, SEEK_SET)) goto RERR;
#define R(a,b,c) if (fread(b,c,a,filez[2]) != a) goto RERR;
    for (i = 0; i < 6; ++i) {
      R(xxgd.numchs, xxgd.bspec[i], 4);
    }
    R(6, &xxgd.gain[0], 8);
    R(1, &xxgd.nterms, 4);
    R(9, &xxgd.eff_pars[0], 4);
    R(5, &elgd.finest[0], 4);
    R(3, &elgd.swpars[0], 4);
    R(1, &elgd.h_norm, 4);
    R(10, &xxgd.esc_file_flags[0], 4);
    R(1, &elgd.bg_err, 4);
    R(1, &elgd.encal_err, 4);
    R(1, &elgd.effcal_err, 4);
#undef R
    if (xxgd.nterms > 6 || xxgd.nterms < 2) {
      fclose(filez[2]);
      warn1("*** ERROR -- File %s\n"
	    "     seems to have the wrong byte ordering.\n"
	    "    -- Perhaps you need to run unix2unix,\n"
	    "       vms2unix or unix2vms on it.\n", filenm);
      goto START;
    }
    /* open .esc_log file for append */
    strcpy(filenm + jext + 4, "_log");
    if (!(filez[26] = fopen(filenm, "a+"))) {
      file_error("open", filenm);
      exit(1);
    }
    if (xxgd.esc_file_flags[1] == 0) {
      warn1("\n"
	    "ESCL8R now adds systematic errors for the background\n"
	    "   subtraction, and for the energy and efficiency\n"
	    "   calibrations. These are missing from this file.\n");
      while ((nc = ask(ans, 80,
		       "Percentage error on background = ? (rtn for 5.0)")) &&
	     ffin(ans, nc, &elgd.bg_err, &fj1, &fj2));
      if (!nc) elgd.bg_err = 5.0f;
      elgd.bg_err /= 100.0f;
      while ((nc = ask(ans, 80,
		       "Error for energy calibration in keV = ? (rtn for 0.03)")) &&
	     ffin(ans, nc, &elgd.encal_err, &fj1, &fj2));
      if (!nc) elgd.encal_err = 0.03f;
      while ((nc = ask(ans, 80,
		       "Percentage error on efficiency = ? (rtn for 3.0)")) &&
	     ffin(ans, nc, &elgd.effcal_err, &fj1, &fj2));
      if (!nc) elgd.effcal_err = 3.0f;
      elgd.effcal_err /= 100.0f;
      xxgd.esc_file_flags[1] = 1;
      SAVEPARS;
      datetime(dattim);
      fprintf(filez[26], "%s: Percentage error on background = %f\n",
	      dattim, elgd.bg_err * 100.0f);
      fprintf(filez[26], "%s: Error for energy calibration = %f\n",
	      dattim, elgd.encal_err);
      fprintf(filez[26], "%s: Percentage error on efficiency = %f\n",
	      dattim, elgd.effcal_err * 100.0f);
      fflush(filez[26]);
    }
    /* read depth and width of Egamma = Egamma valley from .val file */
    strcpy(filenm + jext, ".val");
    if ((filez[1] = fopen(filenm, "r"))) {
      tell("Valley width and depth read from file %s\n", filenm);
      if (fgets(lx, 120, filez[1]) &&
	  1 == sscanf(lx, "%f", &xxgd.v_width)) {
	xxgd.v_width /= 2.0f;
	i1 = 0;
	d1 = 0.0f;
	for (i = 0; i < 1024; ++i) {
	  if (!fgets(lx, 120, filez[1]) ||
	      2 != sscanf(lx, "%i%f", &i2, &d2)) break;
	  if (i2 <= i1 || i2 >= MAXCHS) {
	    warn("Starting channels out of order\n"
		 " or bad channel number, file: %s\n", filenm);
	    break;
	  }
	  for (j = i1; j <= i2; ++j) {
	    xxgd.v_depth[j] = d1 + (d2 - d1) *
	      (float) (j - i1) / (float) (i2 - i1);
	  }
	  i1 = i2;
	  d1 = d2;
	}
      }
      fclose(filez[1]);
    }
  }
  /* calculate level branches through all gamma rays */
  indices_gls();
  path(1);
  /* calculate peak shapes for level scheme gammas */
  calc_peaks();
  /* display background-subtracted projection */
  /* call project twice to put projection also into old_spec */
  project();
  project();
  disp_dat(0);
  return 0;
 RERR:
  file_error("read", filenm);
  goto START;
} /* e_gls_init */

/* ======================================================================= */
int calc_peaks(void)
{
  float beta, fwhm, a, h, r, u, w, x, z, fjunk, width;
  float r1, u1, y2, u7, u5, u6, eg, dx, deg, pos, y = 0.0f, y1 = 1.0f;
  int   i, j, ix, jx, notail, ipk;

  /* calculate peak shapes and derivatives for gammas in level scheme */
  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.npks_ch[i] = 0;
  }
  dx = 1.0f;
  for (ipk = 0; ipk < glsgd.ngammas; ++ipk) {
    eg = glsgd.gam[ipk].e;
    x = channo(eg);
    energy(x, dx, &fjunk, &deg);
    fwhm = sqrt(elgd.swpars[0] + elgd.swpars[1] * x + elgd.swpars[2] * x * x);
    r = (elgd.finest[0] + elgd.finest[1] * x) / 100.0f;
    r1 = 1.0f - r;
    beta = elgd.finest[2] + elgd.finest[3] * x;
    if (beta == 0.0f) beta = fwhm / 2.0f;
    /* check for low-energy tail and calculate peak position */
    if (r == 0.0f) {
      notail = 1;
      a = 0.0f;
      pos = x;
    } else {
      notail = 0;
      y = fwhm / (beta * 3.33021838f);
      y1 = erfc(y);
      y2 = exp(-y * y) / y1 * 1.12837917f;
      a = exp(-y * y) / y1 * 2.0f * r * beta;
      pos = x + beta * a / (a + fwhm * 1.06446705f * r1);
    }
    width = fwhm * 1.41421356f / 2.35482f;
    h = effic(eg) / sqrt(elgd.h_norm) / (a + fwhm * 1.06446705f * r1);
    /* find channel region over which peak must be calculated */
    xxgd.lo_ch[ipk] = (int) (pos - width * 3.1f + 0.5f);
    xxgd.hi_ch[ipk] = (int) (pos + width * 3.1f + 0.5f);
    if (!notail) {
      j = (int) (pos - beta * 10.0f + 0.5f);
      if (xxgd.lo_ch[ipk] > j) xxgd.lo_ch[ipk] = j;
    }
    if (xxgd.lo_ch[ipk] < 1) xxgd.lo_ch[ipk] = 1;
    if (xxgd.hi_ch[ipk] > xxgd.numchs) xxgd.hi_ch[ipk] = xxgd.numchs;
    if (xxgd.hi_ch[ipk] - xxgd.lo_ch[ipk] > 39) {
      j = (int) (0.75f * x + 0.25f * pos + 19.0f);
      if (xxgd.hi_ch[ipk] > j) xxgd.hi_ch[ipk] = j;
      xxgd.lo_ch[ipk] = xxgd.hi_ch[ipk] - 39;
    }
    /* calculate peak and derivative */
    for (ix = xxgd.lo_ch[ipk]; ix <= xxgd.hi_ch[ipk]; ++ix) {
      /* put this peak into look-up table for this channel */
      xxgd.pks_ch[ix][xxgd.npks_ch[ix]++] = (short) ipk;
      if (xxgd.npks_ch[ix] > 60) {
	warn("Severe error: no. of peaks in ch. %d is greater than 60.\n", ix);
	exit(1);
      }
      /* jx is index for this channel in pk_shape and pk_deriv */
      jx = ix - xxgd.lo_ch[ipk];
      x = (float) ix - pos;
      w = x / width;
      if (fabs(w) > 3.1f) {
	u1 = 0.0f;
      } else {
	u1 = r1 * exp(-w * w);
      }
      u = u1;
      a = u1 * w * 2.0f / width;
      if (notail) {
	xxgd.pk_deriv[ipk][jx] = h / deg * a;
      } else {
	z = w + y;
	if (fabs(x / beta) > 12.0f) {
	  xxgd.pk_deriv[ipk][jx] = h / deg * a;
	} else {
	  u7 = exp(x / beta) / y1;
	  if (fabs(z) > 3.1f) {
	    u5 = 0.0f;
	    if (z < 0.0f) u5 = 2.0f;
	    u6 = 0.0f;
	  } else {
	    u5 = erfc(z);
	    u6 = exp(-z * z) * 1.12837917f;
	  }
	  u += r * u5 * u7;
	  xxgd.pk_deriv[ipk][jx] = h / deg *
	    (a + r * u7 * (u6 - u5 * 2.0f * y) / width);
	}
      }
      xxgd.pk_shape[ipk][jx] = h * u;
      xxgd.w_deriv[ipk][jx] = h * a * w * 1.41421356f / 2.35482f -
	h * u / fwhm;
    }
  }
  return 0;
} /* calc_peaks */

/* ======================================================================= */
int chk_fit_igam(int i, int *igam, int *npars, float *saveint, float *saveerr)
{
  float x, eg, dx, deg;
  int   j, k, kg, ki, fit, fit2;

  /* check that intensity of gamma # i can be fitted */

  for (j = 0; j < *npars; ++j) {
    if (igam[j] == i) {
      /* gamma already selected, deselect it */
      --(*npars);
      for (k = j; k < *npars; ++k) {
	igam[k] = igam[k + 1];
	saveint[k] = saveint[k + 1];
	saveerr[k] = saveerr[k + 1];
      }
      remove_hilite(i);
      return 0;
    }
  }
  x = (float) (xxgd.numchs - 1);
  dx = 0.0f;
  energy(x, dx, &eg, &deg);
  if (glsgd.gam[i].e <= eg &&
      glsgd.gam[i].e >= 50.0f && 
      levemit.n[glsgd.gam[i].lf] > 0) {
    fit = 1;
  } else {
    /* energy of gamma is too high or low, or there is */
    /* no decay out of gamma's final level */
    fit = 0;
    ki = glsgd.gam[i].li;
    if (levemit.n[ki] > 1) {
      /* this is not the only gamma out of initial level */
      for (k = 0; k < levemit.n[ki]; ++k) {
	kg = levemit.l[ki][k];
	if (kg != i) {
	  if (glsgd.gam[kg].e > eg ||
	      glsgd.gam[kg].e < 50.0f || 
	      levemit.n[glsgd.gam[kg].lf] == 0) {
	    fit2 = 0;
	    for (j = 0; j < *npars; ++j) {
	      if (igam[j] == kg) fit2 = 1;
	    }
	    if (!fit2) fit = 1;
	  } else {
	    fit = 1;
	  }
	}
      }
      if (fit) {
	/* check that gamma has feeding coincidences */
	for (j = 0; j < glsgd.ngammas; j++) {
	  if (glsgd.gam[j].lf == glsgd.gam[i].li &&
	      glsgd.gam[j].e <= eg && glsgd.gam[j].e >= 50.0f) break;
	}
	if (j >= glsgd.ngammas)  {
	  fit = 0;
	} else {
	  tell("Intensity of gamma %.1f can be fitted"
	       " only by its branching ratio.\n", glsgd.gam[i].e);
	}
      }
    }
  }
  if (fit) {
    igam[*npars] = i;
    saveint[*npars] = glsgd.gam[i].i;
    saveerr[*npars] = glsgd.gam[i].di;
    ++(*npars);
    hilite(i);
  } else {
    tell("Intensity of gamma %.1f cannot be fitted.\n", glsgd.gam[i].e);
  }
  return 0;
} /* chk_fit_igam */

/* ======================================================================= */
int combine(char *ans, int nc)
{
  float r1;
  int   i, j, ix;
  char  save_ans[80], newname[80], *c;

  /* add / subtract / take minimum of  two gates */
  strcpy(save_ans, ans);
  strcpy(ans, save_ans + 1);
  if (!strncmp(ans, "G ", 2) || !strcmp(ans, "G")) {
    if (cur_gate()) return 1;
  } else if (!strncmp(ans, "GL", 2) || !strncmp(ans, "Gl", 2)) {
    if (gate_sum_list(ans)) return 1;
  } else if (!strncmp(ans, "RS", 2) || !strncmp(ans, "Rs", 2)) {
    if (rw_saved_spectra(ans)) return 1;
  } else {
    if (num_gate(ans, nc-1)) return 1;
  }
  c = xxgd.name_gat;
  if (strlen(c) > 22) c += strlen(c) - 22;
  strcpy(newname, c);
  c = xxgd.old_name_gat;
  if (strlen(c) > 16) c += strlen(c) - 16;
  sprintf(xxgd.name_gat, "...%s %c %s", c, *save_ans, newname);

  if (*save_ans == '+') {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      for (j = 0; j < 5; ++j) {
	xxgd.spec[j][ix] += xxgd.old_spec[j][ix];
      }
    }
    for (i = 0; i < glsgd.ngammas; ++i) {
      elgd.hpk[0][i] += elgd.hpk[1][i];
    }
  } else if (*save_ans == '-') {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      for (j = 0; j < 4; ++j) {
	xxgd.spec[j][ix] = xxgd.old_spec[j][ix] - xxgd.spec[j][ix];
      }
      xxgd.spec[4][ix] += xxgd.old_spec[4][ix];
    }
    for (i = 0; i < glsgd.ngammas; ++i) {
      elgd.hpk[0][i] = elgd.hpk[1][i] - elgd.hpk[0][i];
    }
  } else if (*save_ans == '.') {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      for (j = 0; j < 4; ++j) {
	if (xxgd.spec[j][ix] > xxgd.old_spec[j][ix])
	  xxgd.spec[j][ix] = xxgd.old_spec[j][ix];
      }
      if (xxgd.spec[4][ix] < xxgd.old_spec[4][ix])
	xxgd.spec[4][ix] = xxgd.old_spec[4][ix];
    }
    for (i = 0; i < glsgd.ngammas; ++i) {
      if (elgd.hpk[0][i] > elgd.hpk[1][i]) elgd.hpk[0][i] = elgd.hpk[1][i];
    }
  }
  for (i = 0; i < xxgd.numchs; ++i) {
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  return 0;
} /* combine */

/* ======================================================================= */
int disp_dat(int iflag)
{
  float r1, dy_s, x, hicnt, x0, y0, dx, dy, y0_s;
  int   icol, i, nx, ny, ny1, loy;

  /* display data (gate and calculated gate) */
  /* iflag = 0 to display new data */
  /* 1 to redraw data */
  /* 2 to redraw data for full NUMCHS channels */
  /* 3 to redraw data reusing previous limits */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  if (xxgd.numchs < 1) return 1;
  if (iflag < 3) {
    if (iflag == 2 || elgd.numx == 0) {
      elgd.loch = 0;
      elgd.nchs = xxgd.numchs;
    } else {
      elgd.loch = elgd.lox;
      elgd.nchs = elgd.numx;
    }
    elgd.hich = elgd.loch + elgd.nchs - 1;
    if (elgd.hich > xxgd.numchs - 1) elgd.hich = xxgd.numchs - 1;
    elgd.nchs = elgd.hich - elgd.loch + 1;
  }
  if (elgd.ngd > 2) {
    /* separate subroutine for displaying more than 2 stacked gates */
    disp_many_sp();
    return 0;
  }
  initg(&nx, &ny);
  if (elgd.pkfind && (elgd.disp_calc != 1)) ny -= 30;
  erase();
  hicnt = (float) (elgd.locnt + elgd.ncnts);
  if (elgd.ncnts <= 0) {
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      if (hicnt < xxgd.spec[0][i]) hicnt = xxgd.spec[0][i];
      if (elgd.disp_calc && hicnt < xxgd.spec[2][i])
	hicnt = xxgd.spec[2][i];
    }
    hicnt *= elgd.multy;
    if (elgd.pkfind) {
      if (hicnt < elgd.min_ny / 1.1f) hicnt = elgd.min_ny / 1.1f;
    } else {
      if (hicnt < elgd.min_ny) hicnt = elgd.min_ny;
    }
  }
  x0 = (float) elgd.loch;
  dx = (float) elgd.nchs;
  y0 = (float) elgd.locnt;
  dy = hicnt - (float) (elgd.locnt - 1);
  if (elgd.pkfind) dy *= 1.1f;
  loy = ny / (elgd.ngd * 20) + ny * (elgd.ngd - 1) / elgd.ngd;
  ny1 = ny * 9 / (elgd.ngd * 20);
  if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
  limg(nx, 0, ny1, loy);
  setcolor(1);
  trax(dx, x0, dy, y0, elgd.iyaxis);
  dy_s = dy;
  y0_s = y0;
  icol = elgd.colormap[1];
  setcolor(icol);
  x = (float) elgd.loch;
  pspot(x, xxgd.spec[0][elgd.loch]);
  for (i = elgd.loch; i <= elgd.hich; ++i) {
    vect(x, xxgd.spec[0][i]);
    x += 1.0f;
    vect(x, xxgd.spec[0][i]);
  }
  if (elgd.disp_calc) {
    icol = elgd.colormap[2];
    setcolor(icol);
    x = (float) elgd.loch + 0.5f;
    pspot(x, xxgd.spec[2][elgd.loch]);
    for (i = elgd.loch + 1; i <= elgd.hich; ++i) {
      x += 1.0f;
      vect(x, xxgd.spec[2][i]);
    }
    icol = elgd.colormap[3];
    setcolor(icol);
    x = (float) elgd.loch + 0.5f;
    pspot(x, xxgd.spec[1][elgd.loch]);
    for (i = elgd.loch + 1; i <= elgd.hich; ++i) {
      x += 1.0f;
      vect(x, xxgd.spec[1][i]);
    }
  }
  if (elgd.disp_calc == 1 ) {
    /* display difference (observed - calculated) */
    loy = loy + ny1 + ny / (elgd.ngd << 3);
    ny1 = ny / (elgd.ngd << 3);
    y0 = 0.0f;
    dy = dy * 2.5f / 9.0f;
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, 0);
    pspot(x0, y0);
    vect(x0 + dx, y0);
    icol = elgd.colormap[1];
    setcolor(icol);
    x = (float) elgd.loch;
    pspot(x, xxgd.spec[0][elgd.loch] - xxgd.spec[2][elgd.loch]);
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      vect(x, xxgd.spec[0][i] - xxgd.spec[2][i]);
      x += 1.0f;
      vect(x, xxgd.spec[0][i] - xxgd.spec[2][i]);
    }
    /* display residuals */
    loy += ny1 << 1;
    y0 = 0.0f;
    dy = 5.0f;
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      if (dy < (r1 = fabs(xxgd.spec[0][i] - xxgd.spec[2][i]) /
		sqrt(xxgd.spec[5][i]))) dy = r1;
    }
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    icol = elgd.colormap[2];
    setcolor(icol);
    x = (float) elgd.loch;
    r1 = (xxgd.spec[0][elgd.loch] - xxgd.spec[2][elgd.loch]) / 
      sqrt(xxgd.spec[5][elgd.loch]);
    pspot(x, r1);
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      r1 = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
      vect(x, r1);
      x += 1.0f;
      r1 = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
      vect(x, r1);
    }
  }

  /* display gate label */
  setcolor(1);
  mspot(nx - 120 - gate_label_h_offset, loy + ny1);
  putg(xxgd.name_gat, strlen(xxgd.name_gat), 8);
  /* exit if old data not to be displayed */
  if (elgd.ngd <= 1) goto DONE;
  hicnt = (float) (elgd.locnt + elgd.ncnts);
  if (elgd.ncnts <= 0) {
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      if (hicnt < xxgd.old_spec[0][i]) hicnt = xxgd.old_spec[0][i];
      if (elgd.disp_calc && hicnt < xxgd.old_spec[2][i]) {
	hicnt = xxgd.old_spec[2][i];
      }
    }
    hicnt *= elgd.multy;
    if (hicnt < elgd.min_ny) hicnt = elgd.min_ny;
  }
  y0 = (float) elgd.locnt;
  dy = hicnt - (float) (elgd.locnt - 1);
  if (elgd.pkfind) dy *= 1.1f;
  loy = ny / (elgd.ngd * 20);
  ny1 = ny * 9 / (elgd.ngd * 20);
  if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
  limg(nx, 0, ny1, loy);
  setcolor(1);
  trax(dx, x0, dy, y0, elgd.iyaxis);
  icol = elgd.colormap[1];
  setcolor(icol);
  x = (float) elgd.loch;
  pspot(x, xxgd.old_spec[0][elgd.loch]);
  for (i = elgd.loch; i <= elgd.hich; ++i) {
    vect(x, xxgd.old_spec[0][i]);
    x += 1.0f;
    vect(x, xxgd.old_spec[0][i]);
  }
  if (elgd.disp_calc) {
    icol = elgd.colormap[2];
    setcolor(icol);
    x = (float) elgd.loch + 0.5f;
    pspot(x, xxgd.old_spec[2][elgd.loch]);
    for (i = elgd.loch + 1; i <= elgd.hich; ++i) {
      x += 1.0f;
      vect(x, xxgd.old_spec[2][i]);
    }
    icol = elgd.colormap[3];
    setcolor(icol);
    x = (float) elgd.loch + 0.5f;
    pspot(x, xxgd.old_spec[1][elgd.loch]);
    for (i = elgd.loch + 1; i <= elgd.hich; ++i) {
      x += 1.0f;
      vect(x, xxgd.old_spec[1][i]);
    }
  }
  if (elgd.disp_calc == 1 ) {
    /* display difference (observed - calculated) */
    loy = loy + ny1 + ny / (elgd.ngd << 3);
    ny1 = ny / (elgd.ngd << 3);
    y0 = 0.0f;
    dy = dy * 2.5f / 9.0f;
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, 0);
    pspot(x0, y0);
    vect(x0 + dx, y0);
    icol = elgd.colormap[1];
    setcolor(icol);
    x = (float) elgd.loch;
    pspot(x, xxgd.old_spec[0][elgd.loch] - xxgd.old_spec[2][elgd.loch]);
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      vect(x, xxgd.old_spec[0][i] - xxgd.old_spec[2][i]);
      x += 1.0f;
      vect(x, xxgd.old_spec[0][i] - xxgd.old_spec[2][i]);
    }
    /* display residuals */
    loy += ny1 << 1;
    y0 = 0.0f;
    dy = 5.0f;
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      if (dy < (r1 = fabs(xxgd.old_spec[0][i] - xxgd.old_spec[2][i]) / 
		sqrt(xxgd.old_spec[5][i]))) {
	dy = r1;
      }
    }
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    icol = elgd.colormap[2];
    setcolor(icol);
    x = (float) elgd.loch;
    r1 = (xxgd.old_spec[0][elgd.loch] - xxgd.old_spec[2][elgd.loch]) / 
      sqrt(xxgd.old_spec[5][elgd.loch]);
    pspot(x, r1);
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      r1 = (xxgd.old_spec[0][i] - xxgd.old_spec[2][i]) / 
	sqrt(xxgd.old_spec[5][i]);
      vect(x, r1);
      x += 1.0f;
      r1 = (xxgd.old_spec[0][i] - xxgd.old_spec[2][i]) / 
	sqrt(xxgd.old_spec[5][i]);
      vect(x, r1);
    }
  }

  /* display gate label and find peaks */
  setcolor(1);
  mspot(nx - 120 - gate_label_h_offset, loy + ny1);
  putg(xxgd.old_name_gat, strlen(xxgd.old_name_gat), 8);
  if (elgd.pkfind) {
    loy = ny / (elgd.ngd * 20);
    ny1 = ny * 9 / (elgd.ngd * 20);
    if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
    y0 = (float) elgd.locnt;
    dy = 1.1f * (hicnt - (float) (elgd.locnt - 1));
    limg(nx, 0, ny1, loy);
    trax(dx, x0, dy, y0, 0);
    findpks(xxgd.old_spec);
  }
 DONE:
  loy = ny / (elgd.ngd * 20) + ny * (elgd.ngd - 1) / elgd.ngd;
  ny1 = ny * 9 / (elgd.ngd * 20);
  if (elgd.disp_calc != 1) ny1 = ny * 9 / (elgd.ngd * 10);
  limg(nx, 0, ny1, loy);
  trax(dx, x0, dy_s, y0_s, 0);
  if (elgd.pkfind) findpks(xxgd.spec);
  finig();
  return 0;
} /* disp_dat */

/* ======================================================================= */
int disp_many_sp(void)
{
  static float dspec[10][2][MAXCHS];
  static char  clabel[10][80];
  float dy_s, x, hicnt, x0, y0, dx, dy, y0_s;
  int   icol, i, j, k, nx, ny, ny1, nnd, loy, store = 0;

  /* display data (gates and calculated gates) */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  if (elgd.n_stored_sp > elgd.ngd - 1) {
    /* have too many stored spectra */
    for (i = 0; i < elgd.ngd - 1; ++i) {
      strcpy(clabel[i], clabel[i+1 + elgd.n_stored_sp - elgd.ngd]);
      for (j = 0; j < 2; ++j) {
	for (k = 0; k < xxgd.numchs; ++k) {
	  dspec[i][j][k] = dspec[i+1 + elgd.n_stored_sp - elgd.ngd][j][k];
	}
      }
    }
    elgd.n_stored_sp = elgd.ngd - 1;
  }
  /* first check to see if need to store another spectrum */
  if (elgd.n_stored_sp > 0) {
    if (strcmp(clabel[elgd.n_stored_sp - 1], xxgd.old_name_gat)) {
      store = 1;
    } else {
      for (k = 0; k < xxgd.numchs; ++k) {
	if (dspec[elgd.n_stored_sp - 1][0][k] != xxgd.old_spec[0][k] ||
	    dspec[elgd.n_stored_sp - 1][1][k] != xxgd.old_spec[2][k]) {
	  store = 1;
	  break;
	}
      }
    }
  }
  if (store) {
    /* store another spectrum */
    if (elgd.n_stored_sp == elgd.ngd - 1) {
      /* first overwrite oldest spectrum */
      for (i = 0; i < elgd.n_stored_sp - 1; ++i) {
	strcpy(clabel[i], clabel[i+1]);
	memcpy(dspec[i][0], dspec[i+1][0], 4*xxgd.numchs);
	memcpy(dspec[i][1], dspec[i+1][1], 4*xxgd.numchs);
      }
    } else {
      ++elgd.n_stored_sp;
    }
  }

  if (elgd.n_stored_sp == 0) elgd.n_stored_sp = 1;
  strcpy(clabel[elgd.n_stored_sp - 1], xxgd.old_name_gat);
  strcpy(clabel[elgd.n_stored_sp], xxgd.name_gat);
  for (k = 0; k < xxgd.numchs; ++k) {
    dspec[elgd.n_stored_sp - 1][0][k] = xxgd.old_spec[0][k];
    dspec[elgd.n_stored_sp - 1][1][k] = xxgd.old_spec[2][k];
    dspec[elgd.n_stored_sp][0][k] = xxgd.spec[0][k];
    dspec[elgd.n_stored_sp][1][k] = xxgd.spec[2][k];
  }
  /* can now finally display all the data */
  initg(&nx, &ny);
  if (elgd.pkfind) ny += -60;
  erase();
  for (nnd = 0; nnd < elgd.n_stored_sp + 1; ++nnd) {
    hicnt = (float) (elgd.locnt + elgd.ncnts);
    if (elgd.ncnts <= 0) {
      for (i = elgd.loch; i <= elgd.hich; ++i) {
	if (hicnt < dspec[nnd][0][i]) hicnt = dspec[nnd][0][i];
	if (elgd.disp_calc && hicnt < dspec[nnd][1][i])
	  hicnt = dspec[nnd][1][i];
      }
      hicnt *= elgd.multy;
      if (hicnt < elgd.min_ny) hicnt = elgd.min_ny;
    }
    x0 = (float) elgd.loch;
    dx = (float) elgd.nchs;
    y0 = (float) elgd.locnt;
    dy = hicnt - (float) (elgd.locnt - 1);
    loy = ny / ((elgd.n_stored_sp + 1) * 20) +
      nnd * ny / (elgd.n_stored_sp + 1);
    ny1 = ny * 19 / ((elgd.n_stored_sp + 1) * 20);
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    dy_s = dy;
    y0_s = y0;
    icol = elgd.colormap[1];
    setcolor(icol);
    x = (float) elgd.loch;
    pspot(x, dspec[nnd][0][elgd.loch]);
    for (i = elgd.loch; i <= elgd.hich; ++i) {
      vect(x, dspec[nnd][0][i]);
      x += 1.0f;
      vect(x, dspec[nnd][0][i]);
    }
    if (elgd.disp_calc) {
      icol = elgd.colormap[2];
      setcolor(icol);
      x = (float) elgd.loch + 0.5f;
      pspot(x, dspec[nnd][1][elgd.loch]);
      for (i = elgd.loch + 1; i <= elgd.hich; ++i) {
	x += 1.0f;
	vect(x, dspec[nnd][1][i]);
      }
    }
    /* display gate label */
    setcolor(1);
    mspot(nx - 120 - gate_label_h_offset, loy + ny1);
    putg(clabel[nnd], strlen(clabel[nnd]), 8);
  }
  finig();
  if (elgd.pkfind) findpks(xxgd.spec);
  return 0;
} /* disp_many_sp */

/* ======================================================================= */
int energy(float x, float dx, float *eg, float *deg)
{
  int   jj;

  *deg = 0.0f;
  *eg = xxgd.gain[xxgd.nterms - 1];
  for (jj = xxgd.nterms - 1; jj >= 1; --jj) {
    *deg = (float) jj * xxgd.gain[jj] + *deg * x;
    *eg = xxgd.gain[jj - 1] + *eg * x;
  }
  *deg *= dx;
  return 0;
} /* energy */

/* ======================================================================= */
int findpks(float spec[6][MAXCHS])
{
  float pmax, x, y, chanx[300], xchan, psize[300], eg, rsigma, deg, ref;
  int   hipk, lopk, ich, ihi, jhi, ilo, jlo, ipk, npk;
  int   i, j, maxpk, ifwhm1, ifwhm2, ix, iy, nx, ny;
  char  cchar[16];

  /* do peak search */
  rsigma = (float) elgd.isigma;
  ilo = 0;
  lopk = 0;
  maxpk = 300;
  ifwhm1 = (int) (sqrt(elgd.swpars[0]) + 0.5f);
  for (ich = 1; ich < xxgd.numchs; ++ich) {
    x = (float) ich;
    ifwhm2 = (int) (sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
			 elgd.swpars[2] * x * x) + 0.5f);
    if (ifwhm2 != ifwhm1 || ich == xxgd.numchs - 1) {
      ihi = ich - 1;
      jlo = (0 > ilo - 2*ifwhm1 ? 0 : ilo - 2*ifwhm1);
      jhi = (xxgd.numchs - 1 < ihi + 2*ifwhm1 ?
	     xxgd.numchs - 1 : ihi + 2*ifwhm1);
      pfind(&chanx[lopk], &psize[lopk], jlo, jhi, ifwhm1, rsigma, 
	    maxpk, &npk, spec);
      hipk = lopk + npk - 1;
      for (i = hipk; i >= lopk; --i) {
	if (chanx[i] < (float) ilo || chanx[i] > (float) ich) {
	  --npk;
	  for (j = i; j <= hipk - 1; ++j) {
	    chanx[j] = chanx[j+1];
	    psize[j] = psize[j+1];
	  }
	}
      }
      lopk += npk;
      maxpk -= npk;
      if (maxpk <= 0) break;
      ilo = ich;
      ifwhm1 = ifwhm2;
    }
  }
  npk = lopk - 1;
  pmax = 0.0f;
  for (ipk = 0; ipk < npk; ++ipk) {
    chanx[ipk] -= 1.0f;
    psize[ipk] /= effic(chanx[ipk]);
    if (psize[ipk] > pmax) pmax = psize[ipk];
  }
  ref = (float) elgd.ipercent * pmax / 100.0f;
  /* display markers and energies at peak positions */
  initg(&nx, &ny);
  for (ipk = 0; ipk < npk; ++ipk) {
    if (psize[ipk] > ref &&
	chanx[ipk] >= (float) elgd.loch &&
	chanx[ipk] <= (float) elgd.hich) {
      xchan = chanx[ipk];
      energy(xchan, 0.0f, &eg, &deg);
      x = xchan + 0.5f;
      y = spec[0][(int) (0.5f + xchan)];
      cvxy(&x, &y, &ix, &iy, 1);
      mspot(ix, iy + 10);
      ivect(ix, iy + 30);
      mspot(ix, iy + 35);
      sprintf(cchar, "%.1f", eg);
      putg(cchar, strlen(cchar), 5);
    }
  }
  finig();
  return 0;
} /* findpks */

/* ====================================================================== */
int gate_sum_list(char *ans)
{
  float r1, x, wfact, a1, listx[40], eg, dx, ms_err, eg1, eg2, deg;
  int   isum, numx, i, j, ilist, i1, nc, ii, jj, ix, iy, ihi, ilo;

  /* sum gates on list of gammas */
  if (((nc = strlen(ans) - 2) <= 0 ||
       get_list_of_gates(ans + 2, nc, &numx, listx, &wfact)) &&
      (!(nc = ask(ans, 80, "Gate list = ?")) ||
       get_list_of_gates(ans, nc, &numx, listx, &wfact))) return 1;
  if (numx == 0) return 1;
  save_esclev_now(4);
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  sprintf(xxgd.name_gat, "LIST %.35s", ans);
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = 0.0f;
    }
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }
  for (ilist = 0; ilist < numx; ++ilist) {
    eg = listx[ilist];
    x = channo(eg);
    dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		      elgd.swpars[2] * x * x) / 2.0f;
    ilo = (int) (0.5f + x - dx);
    ihi = (int) (0.5f + x + dx);
    if (ihi < 0 || ilo >= xxgd.numchs) return 1;
    if (ilo < 0) ilo = 0;
    if (ihi >= xxgd.numchs) ihi = xxgd.numchs - 1;
    energy((float) ilo - 0.5f, 0.0f, &eg1, &deg);
    energy((float) ihi + 0.5f, 0.0f, &eg2, &deg);
    tell("Gate is chs %i to %i, %.1f\n"
	 " Gate includes level scheme gammas:\n",
	 ilo, ihi, (eg1 + eg2) / 2.0f);
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (glsgd.gam[j].e >= eg1 && glsgd.gam[j].e <= eg2) {
	hilite(j);
	tell("  Gamma #%4i   E = %7.2f   I = %6.2f,  from %s to %s\n",
	     j+1, glsgd.gam[j].e, glsgd.gam[j].i,
	     glsgd.lev[glsgd.gam[j].li].name,
	     glsgd.lev[glsgd.gam[j].lf].name);
      }
    }
    /* read rows from matrix */
    for (iy = ilo; iy <= ihi; ++iy) {
      int junk = fseek(filez[2], iy*reclen, SEEK_SET);
      junk = fread(xxgd.rdata, 4, xxgd.numchs, filez[2]);
      for (ix = 0; ix < xxgd.numchs; ++ix) {
	xxgd.spec[0][ix] += xxgd.rdata[ix];
	xxgd.spec[3][ix] += xxgd.rdata[ix];
	/* subtract background */
	xxgd.spec[0][ix] -= BG2D;
	if (abs(ix - iy) < xxgd.v_width) {
	  r1 = (float) (ix - iy) / xxgd.v_width;
	  xxgd.spec[0][ix] += xxgd.v_depth[iy] * (1.0f - r1 * r1);
	}
      }
      if (xxgd.esc_file_flags[2]) {
	junk = fseek(filez[2], (iy + xxgd.numchs + 7)*reclen, SEEK_SET);
	junk = fread(xxgd.rdata2, 4, xxgd.numchs, filez[2]);
	for (ix = 0; ix < xxgd.numchs; ++ix) {
	  xxgd.spec[4][ix] += xxgd.rdata2[ix];
	}
      }
      /* calculate expected spectrum */
      /* spec[1][] = calculated spectrum from gammas outside gate */
      /* spec[2][] = calculated spectrum from all gammas */
      for (i1 = 0; i1 < xxgd.npks_ch[iy]; ++i1) {
	i = xxgd.pks_ch[iy][i1];
	ii = iy - xxgd.lo_ch[i];
	for (j = 0; j < glsgd.ngammas; ++j) {
	  if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	    a1 = elgd.levelbr[j][glsgd.gam[i].lf] *
	      xxgd.pk_shape[i][ii] * glsgd.gam[i].i;
	    elgd.hpk[0][j] += a1;
	    for (ix = xxgd.lo_ch[j]; ix <= xxgd.hi_ch[j]; ++ix) {
	      jj = ix - xxgd.lo_ch[j];
	      xxgd.spec[2][ix] += a1 * xxgd.pk_shape[j][jj];
	      if (glsgd.gam[i].e < eg1 || glsgd.gam[i].e > eg2)
		xxgd.spec[1][ix] += a1 * xxgd.pk_shape[j][jj];
	    }
	  } else if (elgd.levelbr[i][glsgd.gam[j].lf] != 0.0f) {
	    a1 = elgd.levelbr[i][glsgd.gam[j].lf] *
	      xxgd.pk_shape[i][ii] * glsgd.gam[j].i;
	    elgd.hpk[0][j] += a1;
	    for (ix = xxgd.lo_ch[j]; ix <= xxgd.hi_ch[j]; ++ix) {
	      jj = ix - xxgd.lo_ch[j];
	      xxgd.spec[2][ix] += a1 * xxgd.pk_shape[j][jj];
	      if (glsgd.gam[i].e < eg1 || glsgd.gam[i].e > eg2)
		xxgd.spec[1][ix] += a1 * xxgd.pk_shape[j][jj];
	    }
	  }
	}
      }
    }
  }
  for (i = 0; i < xxgd.numchs; ++i) {
    if (!xxgd.esc_file_flags[2]) xxgd.spec[4][i] = xxgd.spec[3][i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  isum = 0;
  for (ix = xxgd.numchs / 24; ix < xxgd.numchs * 3 / 4; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
    ++isum;
  }
  ms_err /= (float) isum;
  tell("  Mean square error on calculation = %.2f\n", ms_err);
  return 0;
} /* gate_sum_list */

/* ======================================================================= */
int get_bkgnd(void)
{
  static char  stitle[4][40] =
  { "Total projection spectrum file name = ?",
    "   E2 projection spectrum file name = ?",
    "Total background spectrum file name = ?",
    "   E2 background spectrum file name = ?" };

  float bsp[10][MAXCHS], fsum1, fsum2, fsum3, f, fj1, fj2, factor1, factor2;
  int   ishi, islo, i, j, k, ihich, nc, nch;
  char  filnam[80], dattim[20], namesp[8], ans[80];

  save_esclev_now(3);
  /* get projections and background spectra */
  if (xxgd.esc_file_flags[0] != 1) {
    /* compress 4k to 2k. */
    ihich = 4096;
  } else {
    /* do not compress. */
    ihich = xxgd.numchs;
  }
  datetime(dattim);
  if (!askyn("Use enhanced background from E2-bump gates? (Y/N)")) {
    fprintf(filez[26], "%s: New background spectra, no E2-enhancement.\n",
	    dattim);
    fflush(filez[26]);
    for (i = 0; i < 4; i += 2) {
      while (1) {
	askfn(filnam, 80, "", ".spe", stitle[i]);
	if (read_spe_file(filnam, bsp[i], namesp, &nch, MAXCHS)) continue;
	if (nch >= ihich) break;
	warn1(" ERROR - spectrum has wrong length.\n");
      }
      fprintf(filez[26], "       ...%.38s %s\n", stitle[i], filnam);
    }
    fsum1 = 0.0f;
    for (i = 0; i < ihich; ++i) {
      fsum1 += bsp[0][i];
      bsp[6][i] = bsp[0][i] - bsp[2][i];
    }
    for (j = 0; j < ihich; ++j) {
      bsp[7][j] = bsp[8][j] = bsp[9][j] = 0.0f;
      bsp[2][j] /= fsum1;
    }

  } else {
    fprintf(filez[26], "%s: New background spectra, E2-enhanced.\n", dattim);
    fflush(filez[26]);
    for (i = 0; i < 4; ++i) {
      while (1) {
	askfn(filnam, 80, "", ".spe", stitle[i]);
	if (read_spe_file(filnam, bsp[i], namesp, &nch, MAXCHS)) continue;
	if (nch >= ihich) break;
	warn1(" ERROR - spectrum has wrong length.\n");
      }
      fprintf(filez[26], "       ...%.38s %s\n", stitle[i], filnam);
    }
    fsum1 = 0.0f;
    fsum2 = 0.0f;
    for (i = 0; i < ihich; ++i) {
      fsum1 += bsp[0][i];
      fsum2 += bsp[1][i];
      bsp[6][i] = bsp[0][i] - bsp[2][i];
    }
    if (fsum2 == 0.0f) {
      for (j = 0; j < ihich; ++j) {
	bsp[7][j] = bsp[8][j] = bsp[9][j] = 0.0f;
	bsp[2][j] /= fsum1;
      }
    } else {
      while (1) {
	factor1 = fsum1 / fsum2;
	if (!(k = ask(ans, 80, " Factor 1 = ? (rtn for %f)", factor1)) ||
	    !ffin(ans, k, &factor1, &fj1, &fj2)) break;
      }
      fprintf(filez[26], "       ...Factor1 = %f\n", factor1);
      for (j = 0; j < ihich; ++j) {
	bsp[7][j] = bsp[1][j] - bsp[0][j] / factor1;
	bsp[8][j] = bsp[3][j] - bsp[2][j] / factor1;
	bsp[9][j] = bsp[7][j] - bsp[8][j];
	bsp[2][j] /= fsum1;
      }
      while (1) {
	askfn(ans, 80, "", ".win", " E2 gate file = ?");
	if ((filez[1] = open_readonly(ans))) break;
      }
      fprintf(filez[26], "       ...E2 gate file = %s\n", ans);
      fsum3 = 0.0f;
      while (fgets(lx, 120, filez[1]) &&
	     sscanf(lx, "%*5c%d%*3c%d", &islo, &ishi) == 2) {
	if (islo < 0) islo = 0;
	if (ishi > ihich-1) ishi = ihich-1;
	if (islo > ishi) {
	  i = ishi;
	  ishi = islo;
	  islo = i;
	}
	if (islo < 0) islo = 0;
	if (ishi > ihich-1) ishi = ihich-1;
	for (i = islo; i <= ishi; ++i) {
	  fsum3 += bsp[7][i];
	}
      }
      fclose(filez[1]);
      while (1) {
	factor2 = fsum3;
	if (!(k = ask(ans, 80, " Factor 2 = ? (rtn for %f)", factor2)) ||
	    !ffin(ans, k, &factor2, &fj1, &fj2)) break;
      }
      fprintf(filez[26], "       ...Factor2 = %f\n", factor2);
      for (j = 0; j < ihich; ++j) {
	bsp[8][j] /= factor2;
      }
    }
  }

  /* if necessary, contract projections */
  if (xxgd.esc_file_flags[0] != 1) {
    for (i = 0; i < xxgd.numchs; ++i) {
      xxgd.bspec[0][i] = bsp[0][2*i] + bsp[0][2*i + 1];
      xxgd.bspec[1][i] = bsp[2][2*i] + bsp[2][2*i + 1];
      xxgd.bspec[2][i] = bsp[6][2*i] + bsp[6][2*i + 1];
      xxgd.bspec[3][i] = bsp[7][2*i] + bsp[7][2*i + 1];
      xxgd.bspec[4][i] = bsp[8][2*i] + bsp[8][2*i + 1];
      xxgd.bspec[5][i] = bsp[9][2*i] + bsp[9][2*i + 1];
    }
  } else {
    for (i = 0; i < xxgd.numchs; ++i) {
      xxgd.bspec[0][i] = bsp[0][i];
      xxgd.bspec[1][i] = bsp[2][i];
      xxgd.bspec[2][i] = bsp[6][i];
      xxgd.bspec[3][i] = bsp[7][i];
      xxgd.bspec[4][i] = bsp[8][i];
      xxgd.bspec[5][i] = bsp[9][i];
    }
  }
  while ((nc = ask(ans, 80,
		   "ESCL8R adds systematic errors for the background subtraction.\n"
		   "Percentage error on background = ? (rtn for %.1f)",
		   elgd.bg_err * 100.0f)) &&
	 ffin(ans, nc, &f, &fj1, &fj2));
  if (nc) elgd.bg_err = f / 100.0f;
  datetime(dattim);
  fprintf(filez[26], "%s: Percentage error on background = %f\n",
	  dattim, elgd.bg_err * 100.0f);
  fflush(filez[26]);
  /* write projections etc. to .esc file */
  for (i = 0; i < 6; ++i) {
    fseek(filez[2], (xxgd.numchs + i)*reclen, SEEK_SET);
    fwrite(xxgd.bspec[i], 4, xxgd.numchs, filez[2]);
  }
  return 0;
} /* get_bkgnd */

/* ======================================================================= */
int get_cal(void)
{
  float a, b, c, d, e, f, jpars[10], fj1, fj2;
  int   i, nc, jj, iorder;
  char  title[80], filnam[80], dattim[20], ans[80];

  save_esclev_now(1);
  /* get new energy and efficiency calibrations */
  /* get new energy calibration */
  while (1) {
    askfn(filnam, 80, "", "",
	  "Energy calibration file for original matrix = ?\n"
	  "                     (default .ext = .aca, .cal)");
    if (!read_cal_file(filnam, title, &iorder, xxgd.gain)) break;
  }
  xxgd.nterms = iorder + 1;
  datetime(dattim);
  fprintf(filez[26], "%s: Energy calibration file = %s\n", dattim, filnam);
  fflush(filez[26]);

  /* get efficiency calibration */
  while (1) {
    askfn(filnam, 80, "", "",
	  "Efficiency parameter file = ? (default .ext = .aef, .eff)");
    if (!read_eff_file(filnam, title, jpars)) break;
  }
  for (i = 0; i < 9; ++i) {
    xxgd.eff_pars[i] = jpars[i];
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Efficiency calibration file = %s\n", dattim, filnam);
  fflush(filez[26]);
  /* if necessary, contract gain */
  if (xxgd.esc_file_flags[0] != 1) {
    a = xxgd.gain[xxgd.nterms - 1];
    b = 0.0f;
    c = 0.0f;
    d = 0.0f;
    e = 0.0f;
    f = 0.0f;
    for (jj = xxgd.nterms - 1; jj >= 1; --jj) {
      a = a / 2.0f + xxgd.gain[jj - 1];
      b += (float) jj * 2.0f * xxgd.gain[jj];
    }
    c = xxgd.gain[2] * 4.0f;
    if (xxgd.nterms > 3) {
      c += xxgd.gain[3] * 6.0f;
      d = xxgd.gain[3] * 8.0f;
      if (xxgd.nterms > 4) {
	c += xxgd.gain[4] * 6.0f;
	d += xxgd.gain[4] * 16.0f;
	e = xxgd.gain[4] * 16.0f;
	if (xxgd.nterms > 5) {
	  c += xxgd.gain[5] * 5.0f;
	  d += xxgd.gain[5] * 20.0f;
	  e += xxgd.gain[5] * 40.0f;
	  f = xxgd.gain[5] * 32.0f;
	}
      }
    }
    xxgd.gain[0] = a;
    xxgd.gain[1] = b;
    xxgd.gain[2] = c;
    xxgd.gain[3] = d;
    xxgd.gain[4] = e;
    xxgd.gain[5] = f;
  }
  while ((nc = ask(ans, 80,
		   "ESCL8R adds systematic errors for\n"
		   " the energy and efficiency calibrations.\n"
		   "Error for energy calibration in keV = ? (rtn for %.3f)",
		   elgd.encal_err)) &&
	 ffin(ans, nc, &elgd.encal_err, &fj1, &fj2));
  while ((nc = ask(ans, 80,
		   "Percentage error on efficiency = ? (rtn for %.1f)",
		   elgd.effcal_err * 100.0f)) &&
	 ffin(ans, nc, &elgd.effcal_err, &fj1, &fj2));
  if (nc > 0) elgd.effcal_err /= 100.0f;
  datetime(dattim);
  fprintf(filez[26], "%s: Error for energy calibration = %f\n",
	  dattim, elgd.encal_err);
  fprintf(filez[26], "%s: Percentage error on efficiency = %f\n",
	  dattim, elgd.effcal_err * 100.0f);
  fflush(filez[26]);
  return 0;
} /* get_cal */

/* ======================================================================= */
int get_gate(int ilo, int ihi)
{
  float r1, a1, ms_err, eg1, eg2, deg;
  int   isum, i, j, isave, i1, ii, jj, ix, iy;

  /* read gate of channels ilo to ihi and calculate expected spectrum */
  /* spec[0] = background-subtracted gate */
  /* spec[1] = calculated spectrum from gammas outside gate */
  /* spec[2] = calculated spectrum from all gammas */
  /* spec[3] = non-background-subtracted gate */
  /* spec[4] = square of statistical uncertainty */
  /* spec[5] = square of statistical plus systematic uncertainties */
  if (ilo > ihi) {
    isave = ilo;
    ilo = ihi;
    ihi = isave;
  }
  energy((float) ilo - 0.5f, 0.0f, &eg1, &deg);
  energy((float) ihi + 0.5f, 0.0f, &eg2, &deg);
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  sprintf(xxgd.name_gat, "%i to %i, %.1f\n", ilo, ihi, (eg1 + eg2) / 2.0f);
  tell("Gate is chs %i to %i, %.1f\n"
       " Gate includes level scheme gammas:\n",
       ilo, ihi, (eg1 + eg2) / 2.0f);
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].e >= eg1 && glsgd.gam[j].e <= eg2) {
      hilite(j);
      tell("  Gamma #%4i   E = %7.2f   I = %6.2f,  from %s to %s\n",
	   j+1, glsgd.gam[j].e, glsgd.gam[j].i,
	   glsgd.lev[glsgd.gam[j].li].name,
	   glsgd.lev[glsgd.gam[j].lf].name);
    }
  }
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = 0.0f;
    }
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }
  /* read rows from matrix */
  for (iy = ilo; iy <= ihi; ++iy) {
    int junk = fseek(filez[2], iy*reclen, SEEK_SET);
    junk = fread(xxgd.rdata, xxgd.numchs, 4, filez[2]);
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.spec[0][ix] += xxgd.rdata[ix];
    }
    if (xxgd.esc_file_flags[2]) {
      junk = fseek(filez[2], (iy + xxgd.numchs + 7)*reclen, SEEK_SET);
      junk = fread(xxgd.rdata2, 4, xxgd.numchs, filez[2]);
      for (ix = 0; ix < xxgd.numchs; ++ix) {
	xxgd.spec[4][ix] += xxgd.rdata2[ix];
      }
    }
  }
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    xxgd.spec[3][ix] = xxgd.spec[0][ix];
  }
  for (iy = ilo; iy <= ihi; ++iy) {
    /* subtract background */
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.spec[0][ix] -= BG2D;
      if (abs(ix - iy) < xxgd.v_width) {
	r1 = (float) (ix - iy) / xxgd.v_width;
	xxgd.spec[0][ix] += xxgd.v_depth[iy] * (1.0f - r1 * r1);
      }
    }
    /* calculate expected spectrum */
    /* spec[1][] = calculated spectrum from gammas outside gate */
    /* spec[2][] = calculated spectrum from all gammas */
    for (i1 = 0; i1 < xxgd.npks_ch[iy]; ++i1) {
      i = xxgd.pks_ch[iy][i1];
      ii = iy - xxgd.lo_ch[i];
      for (j = 0; j < glsgd.ngammas; ++j) {
	if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	  a1 = elgd.levelbr[j][glsgd.gam[i].lf] *
	    xxgd.pk_shape[i][ii] * glsgd.gam[i].i;
	  elgd.hpk[0][j] += a1;
	  for (ix = xxgd.lo_ch[j]; ix <= xxgd.hi_ch[j]; ++ix) {
	    jj = ix - xxgd.lo_ch[j];
	    xxgd.spec[2][ix] += a1 * xxgd.pk_shape[j][jj];
	    if (glsgd.gam[i].e < eg1 || glsgd.gam[i].e > eg2) {
	      xxgd.spec[1][ix] += a1 * xxgd.pk_shape[j][jj];
	    }
	  }
	} else if (elgd.levelbr[i][glsgd.gam[j].lf] != 0.0f) {
	  a1 = elgd.levelbr[i][glsgd.gam[j].lf] *
	    xxgd.pk_shape[i][ii] * glsgd.gam[j].i;
	  elgd.hpk[0][j] += a1;
	  for (ix = xxgd.lo_ch[j]; ix <= xxgd.hi_ch[j]; ++ix) {
	    jj = ix - xxgd.lo_ch[j];
	    xxgd.spec[2][ix] += a1 * xxgd.pk_shape[j][jj];
	    if (glsgd.gam[i].e < eg1 || glsgd.gam[i].e > eg2) {
	      xxgd.spec[1][ix] += a1 * xxgd.pk_shape[j][jj];
	    }
	  }
	}
      }
    }
  }
  for (i = 0; i < xxgd.numchs; ++i) {
    if (!xxgd.esc_file_flags[2]) xxgd.spec[4][i] = xxgd.spec[3][i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  isum = 0;
  for (ix = xxgd.numchs / 24; ix < xxgd.numchs * 3 / 4; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
    ++isum;
  }
  ms_err /= (float) isum;
  tell("Mean square error on calculation = %.2f\n", ms_err);
  return 0;
} /* get_gate */

/* ======================================================================= */
int get_list_of_gates(char *ans, int nc, int *outnum, float *outlist,
		      float *wfact)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */
  float eg, fj1, fj2;
  int   i, j, nl;

  /* read Egamma / list_name and width factor from ANS */
  /* return list of energies OUTLIST with OUTNUM energies */

  *outnum = 0;
  if (nc == 0) return 0;
  if (*ans == ' ') {
    memmove(ans, ans + 1, nc);
    if (--nc == 0) return 0;
  }
  for (i = 0; i < 55; ++i) {
    if (*ans == listnam[i]) {
      /* list name found */
      nl = i;
      *wfact = 1.0f;
      if (nc >= 3 &&
	  ffin(ans + 2, nc - 2, wfact, &fj1, &fj2)) return 1;
      for (j = 0; j < elgd.nlist[nl]; ++j) {
	outlist[(*outnum)++] = elgd.list[nl][j];
      }
      if (*wfact <= 0.0f) *wfact = 1.0f;
      return 0;
    }
  }
  /* list name not found */
  if (ffin(ans, nc, &eg, wfact, &fj2)) return 1;
  *outnum = 1;
  outlist[0] = eg;
  if (*wfact <= 0.0f) *wfact = 1.0f;
  return 0;
} /* get_list_of_gates */

/* ======================================================================= */
int get_shapes(void)
{
  float rj2, sw1, sw2, sw3, newp[4];
  int   i, k, nc;
  char  dattim[20], ans[80];

  save_esclev_now(1);
  /* get new peak shape and FWHM parameters */
  if (xxgd.esc_file_flags[0] != 1) {
    elgd.finest[0] *= 2.0f;
    elgd.finest[2] *= 2.0f;
    elgd.swpars[0] *= 4.0f;
    elgd.swpars[1] *= 2.0f;
  }
  newp[0] = 0.0f;
  newp[1] = 0.0f;
  newp[2] = 1.0f;
  newp[3] = 0.0f;
  if (!askyn("Non-Gaussian peak shapes can be used,\n"
	     "   but are not usually necessary.\n"
	     "...Use non-Gaussian peak shapes? (Y/N)")) {
    datetime(dattim);
    fprintf(filez[26], "%s: Gaussian peak shapes selected.\n", dattim);
    fflush(filez[26]);
  } else {
    while ((k = ask(ans, 80,
		    " The parameters R and BETA define the shapes of the peaks.\n"
		    "        The peak is the sum of a gaussian of height H*(1-R/100)\n"
		    "        and a skew gaussian of height H*R/100, where BETA is \n"
		    "        the decay constant of the skew gaussian (in channels).\n\n"
		    " R is taken as R = A + B*x    (x = ch. no. in original matrix)\n"
		    /* " The default values for A and B are %.1f and %.5f.\n", */
		    "Enter A,B (rtn for %.1f, %.5f)",
		    elgd.finest[0], elgd.finest[1])) &&
	   ffin(ans, k, &newp[0], &newp[1], &rj2));
    if (!k) {
      newp[0] = elgd.finest[0];
      newp[1] = elgd.finest[1];
    }
    while ((k = ask(ans, 80,
		    " BETA is taken as BETA = C + D*x"
		    "  (x = ch. no. in original matrix)\n"
		    /* " The default values for C and D are %.1f and %.5f.\n" */
		    "Enter C,D (rtn for %.1f, %.5f)",
		    elgd.finest[2], elgd.finest[3])) &&
	   ffin(ans, k, &newp[2], &newp[3], &rj2));
    if (!k) {
      newp[2] = elgd.finest[2];
      newp[3] = elgd.finest[3];
    }
    datetime(dattim);
    fprintf(filez[26], "%s: Non-gaussian peak shapes: %f %f %f %f\n",
	    dattim, newp[0], newp[1], newp[2], newp[3]);
    fflush(filez[26]);
  }

  for (i = 0; i < 4; ++i) {
    elgd.finest[i] = newp[i];
  }
  while (1) {
    if (!(nc = ask(ans, 80,
		   " The fitted peak widths are taken as:\n"
		   "     FWHM = SQRT(F*F + G*G*x + H*H*x*x)\n"
		   "                (x = ch. no. /1000 in original matrix)\n"
		   /* "  Default values are:  F = %.2f, G = %.2f, H= %.2f\n" */
		   "Enter F,G,H (rtn for %.2f, %.2f, %.2f)",
		   sqrt(elgd.swpars[0]),  sqrt(elgd.swpars[1] * 1e3f),
		   sqrt(elgd.swpars[2]) * 1e3f))) break;
    if (ffin(ans, nc, &sw1, &sw2, &sw3)) continue;
    if (sw1 > 0.0f && sw2 >= 0.0f) {
      elgd.swpars[0] = sw1 * sw1;
      elgd.swpars[1] = sw2 * sw2 / 1e3f;
      elgd.swpars[2] = sw3 * sw3 / 1e6f;
      break;
    }
    warn1("F must be > 0, G must be >= 0.\n");
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Peak width parameters = %f %f %f\n",
	  dattim, sqrt(elgd.swpars[0]),
	  sqrt(elgd.swpars[1] * 1e3f), sqrt(elgd.swpars[2]) * 1e3f);
  fflush(filez[26]);
  /* if necessary, contract gain */
  if (xxgd.esc_file_flags[0] != 1) {
    elgd.finest[0] /= 2.0f;
    elgd.finest[2] /= 2.0f;
    elgd.swpars[0] /= 4.0f;
    elgd.swpars[1] /= 2.0f;
  }
  return 0;
} /* get_shapes */

/* ======================================================================= */
int num_gate(char *ans, int nc)
{
  float x, wfact, eg, dx, fj2;
  int   ihi, ilo;

  /* calculate gate channels ilo, ihi from specified Egamma */
  /* and width_factor * parameterized_FWHM */
  if (ffin(ans, nc, &eg, &wfact, &fj2)) return 1;
  save_esclev_now(4);
  if (wfact <= 0.0f) wfact = 1.0f;
  x = channo(eg);
  dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		    elgd.swpars[2] * x * x) / 2.0f;
  ilo = (int) (0.5f + x - dx);
  ihi = (int) (0.5f + x + dx);
  if (ihi < 0 || ilo >= xxgd.numchs) return 1;
  if (ilo < 0) ilo = 0;
  if (ihi >= xxgd.numchs) ihi = xxgd.numchs - 1;
  get_gate(ilo, ihi);
  return 0;
} /* num_gate */

/* ======================================================================= */
int prep(void)
{
  float fmax, fwhm, test, x, eg, effspec[MAXCHS];
  int   jext, i, j, k, iymax, i0, i4, nc, ii, iy, ix, m4b;
  int   matseg4b[4096*8];
  short matseg[4096*8];
  char  fn[80], filnam[80], dattim[20], ans[80];

  /* equivalence here just to save RAM */
  /* open matrix file */
  for (i = 0; i < 10; ++i) {
    xxgd.esc_file_flags[i] = 0;
  }
  warn1(
    " Compressed matrix data file preparation routine...\n\n"
    " You will need:\n"
    " 4k x 4k channel SYMMETRIZED gamma-gamma matrix\n"
    "     (should NOT be background-subtracted);\n"
    "     (could be 8k x 8k matrix if you are making a .e8k file);\n"
    " Total symmetrized matrix projection and background;\n"
    " A gate (.win) file with gates set on bkgnd chs in the E2-bump region;\n"
    " The summed X-gate spectrum from this gate file (with no bkgnd sub.);\n"
    " A background spectrum for this E2-gated sum spectrum;\n"
    " Efficiency (.aef/.eff) and energy (.aca/.cal) calibration files;\n"
    " and parameters to define the FWHM, and R and BETA peak shapes,\n"
    "    as a function of ch. no. (see gf3 for details).\n\n"
    " This program now accepts matrices in .mat (2B/ch) and\n"
    "   .spn or .m4b (4B/ch) formats, and .sec format for 4B/ch, 8192-ch matrix.\n"
    " Specify .ext = .spn, .m4b, or .sec for 4B/ch.\n\n");

  while (1) {
    askfn(filnam, 80, "", ".mat", "Matrix file = ?");
    j = setext(filnam, ".mat", 80);
    if ((filez[1] = open_readonly(filnam))) break;
  }
  m4b = 0;   /* int*2 data */
  if (!strcmp(filnam + j, ".spn") ||
      !strcmp(filnam + j, ".SPN") ||
      !strcmp(filnam + j, ".m4b") ||
      !strcmp(filnam + j, ".M4B")) m4b = 1;  /* int*4 data */
  if (!strcmp(filnam + j, ".sec") ||
      !strcmp(filnam + j, ".SEC")) m4b = 2;  /* 8192-ch data */
  /* copy matrix to new .esc file */
  strcpy(fn, filnam);
  if (m4b == 2) {
    strcpy(filnam + j, ".e8k");
    nc = askfn(ans, 80, filnam, ".e8k",
	       "\nName for new escl8r data file = ?\n"
	       "       (.e8k indicates 8k x 8k chs)");
    jext = setext(ans, ".e8k", 80);
    xxgd.numchs = 8192;
  } else {
    strcpy(filnam + j, ".esc");
    nc = askfn(ans, 80, filnam, ".esc",
	       "\nName for new escl8r data file = ?\n"
	       "       (specify .ext = .e4k to keep full 4kx4k chs)");
    jext = setext(ans, ".esc", 80);
    if (!strncmp(ans + jext, ".e4k", 4) ||
	!strncmp(ans + jext, ".E4K", 4)) {
      xxgd.numchs = 4096;
    } else {
      xxgd.numchs = 2048;
    }
  }

  reclen = xxgd.numchs*4;
  filez[2] = open_new_file(ans, 1);
  /* open .esc_log file for append */
  strcpy(ans + jext + 4, "_log");
  if (!(filez[26] = fopen(ans, "a+"))) {
    file_error("open", ans);
    exit(-1);
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Created new escl8r data file %s\n", dattim, ans);
  fprintf(filez[26], "      ...from matrix file %s\n", fn);
  fflush(filez[26]);
  xxgd.esc_file_flags[0] = 1;
  if (xxgd.numchs < 4096 &&
      askyn("The matrix will be reduced to 2048 x 2048 channels.\n"
	    "You have a choice of compressing by a factor of two\n"
	    "   or just taking the first 2048 channels.\n\n"
	    " ...Compress matrix by factor of two? (Y/N)")) {
    xxgd.esc_file_flags[0] = 0;
    elgd.swpars[0] /= 4.0f;
    elgd.swpars[1] /= 2.0f;
  }
  if (xxgd.esc_file_flags[0] != 1) {
    fprintf(filez[26], "      ...by compressing from 4k to 2k channels.\n");
    tell("\nCompressing matrix to new file %s\n\n", ans);
    for (i0 = 0; i0 < 4096; i0 += 8) {
      tell("Row %i\r", i0); fflush(stdout);
      /* read matrix segment */
      if (m4b) {
	rmat4b(filez[1], i0, 8, matseg4b);
	/* compress matrix segment */
	for (iy = 0; iy < 4; ++iy) {
	  for (ix = 0; ix < xxgd.numchs; ++ix) {
	    j = (ix << 1) + (iy << 13);
	    i4 = matseg4b[j] + matseg4b[j+1] +
	         matseg4b[j+4096] + matseg4b[j+4097];
	    xxgd.spec[iy][ix] = (float) i4;
	  }
	}
      } else {
	rmat(filez[1], i0, 8, matseg);
	/* compress matrix segment */
	for (iy = 0; iy < 4; ++iy) {
	  for (ix = 0; ix < xxgd.numchs; ++ix) {
	    j = (ix << 1) + (iy << 13);
	    i4 = matseg[j] + matseg[j+1] + matseg[j+4096] + matseg[j+4097];
	    xxgd.spec[iy][ix] = (float) i4;
	  }
	}
      }
      /* rewrite matrix segment */
      for (i = 0; i < 4; ++i) {
	fseek(filez[2], (i0 / 2 + i)*reclen, SEEK_SET);
	fwrite(xxgd.spec[i], 4, xxgd.numchs, filez[2]);
      }
    }
  } else {
    if (xxgd.numchs >= 4096) {
      fprintf(filez[26], "      ...by taking all %dk channels.\n", xxgd.numchs/1024);
      tell("\nCopying matrix to new file %s\n\n", ans);
    } else {
      fprintf(filez[26], "      ...by taking only the first 2k channels.\n");
      tell("\nCopying part of matrix to new file %s\n\n", ans);
    }
    for (i0 = 0; i0 < xxgd.numchs; i0 += 8) {
      tell("Row %i\r", i0); fflush(stdout);
      /* read matrix segment */
      if (m4b == 2) {
	for (k=0; k<2; k++) {
	  fseek(filez[1], (i0 + 4*k)*reclen, SEEK_SET);
	  fread(matseg4b, reclen, 4, filez[1]);
	  /* rewrite matrix segment */
	  for (i = 0; i < 4; ++i) {
	    for (j = 0; j < xxgd.numchs; ++j) {
	      xxgd.spec[0][j] = (float) matseg4b[j + (i << 12)];
	    }
	    fseek(filez[2], (i0 + 4*k + i)*reclen, SEEK_SET);
	    fwrite(xxgd.spec[0], 4, xxgd.numchs, filez[2]);
	  }
	}
      } else if (m4b == 1) {
	rmat4b(filez[1], i0, 8, matseg4b);
	/* rewrite matrix segment */
	for (i = 0; i < 8; ++i) {
	  for (j = 0; j < xxgd.numchs; ++j) {
	    xxgd.spec[0][j] = (float) matseg4b[j + (i << 12)];
	  }
	  fseek(filez[2], (i0 + i)*reclen, SEEK_SET);
	  fwrite(xxgd.spec[0], 4, xxgd.numchs, filez[2]);
	}
      } else {
	rmat(filez[1], i0, 8, matseg);
	/* rewrite matrix segment */
	for (i = 0; i < 8; ++i) {
	  for (j = 0; j < xxgd.numchs; ++j) {
	    xxgd.spec[0][j] = (float) matseg[j + (i << 12)];
	  }
	  fseek(filez[2], (i0 + i)*reclen, SEEK_SET);
	  fwrite(xxgd.spec[0], 4, xxgd.numchs, filez[2]);
	}
      }
    }
  }
  fclose(filez[1]);
  /* get energy and efficiency calibrations */
  get_cal();
  /* get peak shape and FWHM parameters */
  get_shapes();
  /* get projections and background spectra */
  get_bkgnd();
  /* find largest element of new matrix */
  fmax = 1e-6f;
  iymax = 0;
  for (i = 140; i < xxgd.numchs; ++i) {
    x = (float) i;
    eg = xxgd.gain[xxgd.nterms - 1];
    for (j = xxgd.nterms - 2; j >= 0; --j) {
      eg = xxgd.gain[j] + eg * x;
    }
    fwhm = sqrt(elgd.swpars[0] + elgd.swpars[1] * x + elgd.swpars[2] * x * x);
    effspec[i] = effic(eg) / (fwhm * 1.06446705f);
    test = xxgd.bspec[2][i] / effspec[i];
    if (test > fmax) {
      fmax = test;
      iymax = i;
    }
  }
  ii = iymax;
  fmax = 1e-6f;
  int junk = fseek(filez[2], ii*reclen, SEEK_SET);
  junk = fread(xxgd.spec[0], 4, xxgd.numchs, filez[2]);
  for (j = 149; j < xxgd.numchs; ++j) {
    test = (xxgd.spec[0][j] - 
	    xxgd.bspec[0][j] * xxgd.bspec[1][ii] - 
	    xxgd.bspec[2][ii] * xxgd.bspec[1][j] -
	    xxgd.bspec[3][j] * xxgd.bspec[4][ii] -
	    xxgd.bspec[5][ii] * xxgd.bspec[4][j]) / 
      (effspec[j] * effspec[ii]);
    if (test > fmax) fmax = test;
  }
  /* set up height normalization factor to give coinc. int. */
  /* of 100 for most intense peak */
  elgd.h_norm = 100.0f / fmax;
  tell("\n 2D Scaling factor = %.3e\n\n", elgd.h_norm);
  datetime(dattim);
  fprintf(filez[26], "%s:  2D Scaling factor = %f\n", dattim, elgd.h_norm);
  fflush(filez[26]);
  /* write calibrations etc. to file */
  xxgd.esc_file_flags[1] = 1;
  SAVEPARS;
  return 0;
} /* prep */

/* ======================================================================= */
int project(void)
{
  float r1, fact, a1, a2, s1, s2, ms_err;
  int   isum, i, j, ii, ix;

  /* set displayed spectrum to background-subtracted projection */
  save_esclev_now(4);
  s1 = 0.0f;
  s2 = 0.0f;
  for (i = 0; i < xxgd.numchs; ++i) {
    s1 += xxgd.bspec[0][i];
    s2 += xxgd.bspec[2][i];
    for (j = 0; j < 6; ++j) {
      xxgd.old_spec[j][i] = xxgd.spec[j][i];
      xxgd.spec[j][i] = 0.0f;
    }
    xxgd.spec[0][i] = xxgd.bspec[2][i];
    xxgd.spec[3][i] = xxgd.bspec[0][i];
    xxgd.spec[4][i] = xxgd.bspec[0][i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  strcpy(xxgd.name_gat, "Projection");
  /* calculate expected spectrum */
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }
  for (i = 0; i < glsgd.ngammas; ++i) {
    a1 = effic(glsgd.gam[i].e) * glsgd.gam[i].i / sqrt(elgd.h_norm);
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (elgd.levelbr[j][glsgd.gam[i].lf] != 0.0f) {
	elgd.hpk[0][j] += a1 * elgd.levelbr[j][glsgd.gam[i].lf];
	a2 = effic(glsgd.gam[j].e) * glsgd.gam[i].i / sqrt(elgd.h_norm);
	elgd.hpk[0][i] += a2 * elgd.levelbr[j][glsgd.gam[i].lf];
      }
    }
  }
  fact = s1 / s2;
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[0][i] *= fact;
    for (ix = xxgd.lo_ch[i]; ix <= xxgd.hi_ch[i]; ++ix) {
      ii = ix - xxgd.lo_ch[i];
      xxgd.spec[2][ix] += elgd.hpk[0][i] * xxgd.pk_shape[i][ii];
    }
  }
  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  isum = 0;
  for (ix = xxgd.numchs / 24; ix < xxgd.numchs * 3 / 4; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
    ++isum;
  }
  ms_err /= (float) isum;
  tell("Mean square error on calculation = %.2f\n", ms_err);
  return 0;
} /* project */

/* ======================================================================= */
int sum_chs(int ilo, int ihi)
{
  float r1, calc, area, cent, x, dc, eg, eg1, eg2, deg, cts, sum;
  int   i, j, nx, ny;

  /* sum results over chs ilo to ihi */
  if (ilo < 0 || ilo >= xxgd.numchs ||
      ihi < 0 || ihi >= xxgd.numchs) return 0;
  energy((float) ilo - 0.5f, 0.0f, &eg1, &deg);
  energy((float) ihi + 0.5f, 0.0f, &eg2, &deg);
  tell("Sum over chs %i to %i,   includes level scheme gammas:\n", ilo, ihi);
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].e >= eg1 && glsgd.gam[j].e <= eg2) {
      hilite(j);
      tell("  Gamma #%4i   E = %7.2f   I = %6.2f,  from %s to %s\n",
	   j+1, glsgd.gam[j].e, glsgd.gam[j].i,
	   glsgd.lev[glsgd.gam[j].li].name,
	   glsgd.lev[glsgd.gam[j].lf].name);
    }
  }
  /* display limits */
  initg(&nx, &ny);
  if (ilo >= elgd.loch && ilo <= elgd.hich) {
    x = (float) ilo + 0.5f;
    pspot(x, fy0);
    vect(x, fy0 + fdy / 2.0f);
  }
  if (ihi >= elgd.loch && ihi <= elgd.hich) {
    x = (float) ihi + 0.5f;
    pspot(x, fy0);
    vect(x, fy0 + fdy / 2.0f);
  }
  finig();
  /* calculate area, centroid etc */
  area = 0.0f;
  calc = 0.0f;
  cent = 0.0f;
  sum = 0.0f;
  dc = 0.0f;
  for (i = ilo; i <= ihi; ++i) {
    area += xxgd.spec[0][i];
    calc += xxgd.spec[2][i];
    cent += xxgd.spec[0][i] * (float) (i - ilo);
  }
  if (area == 0.0f) {
    eg = 0.0f;
    deg = 0.0f;
  } else {
    cent = cent / area + (float) ilo;
    for (i = ilo; i <= ihi; ++i) {
      cts = xxgd.spec[5][i];
      sum += cts;
      r1 = ((float) i - cent) / area;
      dc += cts * (r1 * r1);
    }
    dc = sqrt(dc);
    energy(cent, dc, &eg, &deg);
  }
  /* write results */
  tell(" Chs %i to %i   Area: %.0f +- %.0f, Calc: %.0f\n"
       "     Energy: %.3f +- %.3f\n",
       ilo, ihi, area, sqrt(sum), calc, eg, deg);
  return 0;
} /* sum_chs */

/* ======================================================================= */
int wspec2(float *spec, char *filnam, char sp_nam_mod, int expand)
{
  int   jext, rl = 0, c1 = 1;
  char  namesp[80], buf[32];

  /* write spectrum to disk file */
  /* default file extension = .spe */
  /* expand is ignored in escl8r;
     provided only for consistency with levit8r / 4dg8r */
  jext = setext(filnam, ".spe", 80);
  filnam[jext-1] = sp_nam_mod;
  if (jext > 8) {
    strncpy(namesp, filnam, 8);
  } else {
    memset(namesp, ' ', 8);
    strncpy(namesp, filnam, jext);
  }
  filez[1] = open_new_file(filnam, 0);
  if (!filez[1]) return 1;

#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(namesp,8); W(&xxgd.numchs,4); W(&c1,4); W(&c1,4); W(&c1,4);
#undef W
  if (put_file_rec(filez[1], buf, rl) ||
      put_file_rec(filez[1], spec, 4*xxgd.numchs)) {
    warn("ERROR: could not write to file %s\n"
	 " ...spectrum not written.\n", filnam);
    fclose(filez[1]);
    return 1;
  }
  fclose(filez[1]);
  tell("Spectrum written as %s\n", filnam);
  return 0;
} /* wspec2 */

/* ======================================================================= */
int write_sp(char *ans, int nc)
{
  float spec[MAXCHS];
  int   i;
  char  filnam[80];

  strcpy(filnam, ans+2);
  nc -= 2;
  /* ask for output file name */
  if (nc < 1 &&
      (nc = ask(filnam, 70, "Output spectrum name = ?")) == 0) return 0;
  strcpy(filnam + nc, "e");
  if (askyn("Write observed spectrum? (Y/N)"))
    wspec2(xxgd.spec[0], filnam, 'e', 0);
  if (askyn("Write unsubtracted spectrum? (Y/N)"))
    wspec2(xxgd.spec[3], filnam, 'u', 0);
  if (askyn("Write calculated spectrum? (Y/N)"))
    wspec2(xxgd.spec[2], filnam, 'c', 0);
  if (askyn("Write difference spectrum? (Y/N)")) {
    for (i = 0; i < xxgd.numchs; ++i) {
      spec[i] = xxgd.spec[0][i] - xxgd.spec[2][i];
    }
    wspec2(spec, filnam, 'd', 0);
  }
  if (askyn("Write residual spectrum? (Y/N)")) {
    for (i = 0; i < xxgd.numchs; ++i) {
      spec[i] = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
    }
    wspec2(spec, filnam, 'r', 0);
  }
  return 0;
} /* write_sp */

/* ======================================================================= */
float channo(float egamma)
{
  static float diff = 0.002f;
  double e, x1, x2;
  float  ret_val;
  int    i;

  /* the value of diff fixes the precision when nterms > 3 */
  if (xxgd.nterms <= 3) {
    if (xxgd.gain[2] < 1e-7) {
      x1 = (egamma - xxgd.gain[0]) / xxgd.gain[1];
      ret_val = (egamma - xxgd.gain[0]) / (xxgd.gain[1] + xxgd.gain[2] * x1);
    } else {
      ret_val = (sqrt(xxgd.gain[1] * xxgd.gain[1] - xxgd.gain[2] * 4.0f *
		      (xxgd.gain[0] - egamma)) - xxgd.gain[1]) /
	(xxgd.gain[2] * 2.0f);
    }
  } else {
    x1 = (egamma - xxgd.gain[0]) / xxgd.gain[1];
    x1 = (egamma - xxgd.gain[0]) / (xxgd.gain[1] + xxgd.gain[2] * x1);
    while (1) {
      e = xxgd.gain[xxgd.nterms - 1];
      for (i = xxgd.nterms - 2; i >= 2; --i) {
	e = xxgd.gain[i] + e * x1;
      }
      x2 = e * x1 * x1;
      x2 = (egamma - xxgd.gain[0] - x2) / xxgd.gain[1];
      if (fabs(x2 - x1) < diff) break;
      x1 = x2;
    }
    ret_val = x2;
  }
  return ret_val;
} /* channo */

/* ======================================================================= */
float effic(float eg)
{
  float  ret_val;
  double f1, f2, x1, x2, x3;

  x1 = log(eg / xxgd.eff_pars[7]);
  x2 = log(eg / xxgd.eff_pars[8]);
  f1 = xxgd.eff_pars[0] + xxgd.eff_pars[1] * x1 + xxgd.eff_pars[2] * x1 * x1;
  f2 = xxgd.eff_pars[3] + xxgd.eff_pars[4] * x2 + xxgd.eff_pars[5] * x2 * x2;
  if (f1 <= 0. || f2 <= 0.) {
    ret_val = 1.0f;
  } else {
    x3 = exp(-xxgd.eff_pars[6] * log(f1)) + exp(-xxgd.eff_pars[6] * log(f2));
    if (x3 <= 0.) {
      ret_val = 1.0f;
    } else {
      ret_val = exp(exp(-log(x3) / xxgd.eff_pars[6]));
    }
  }
  return ret_val;
} /* effic */

/* ====================================================================== */

void set_gate_label_h_offset(int offset)
{
  gate_label_h_offset = offset;
} /* set_gate_label_h_offset */
