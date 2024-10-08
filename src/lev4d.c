#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "minig.h"
#include "levit8r.h"
Xxgd xxgd;

/* declared in esclev.c */
extern char listnam[55];

int gate_label_h_offset = 0;

#define BG2D \
 (xxgd.bspec[0][iy] * xxgd.bspec[1][ix] +\
  xxgd.bspec[1][iy] * xxgd.bspec[2][ix] +\
  xxgd.bspec[3][iy] * xxgd.bspec[4][ix] +\
  xxgd.bspec[4][iy] * xxgd.bspec[5][ix])
  
#define SUBV \
 if ((float) abs(ix - iy) < xxgd.v_width) { \
   r1 = (float) (ix - iy) / xxgd.v_width; \
   y += xxgd.v_depth[iy] * (1.0f - r1 * r1); \
 }
  
#define BG3D \
 (xxgd.pro2d[ichyz] * xxgd.bspec[1][ix] +\
  xxgd.pro2d[ichxz] * xxgd.bspec[1][iy] +\
  xxgd.pro2d[ichxy] * xxgd.bspec[1][iz] +\
  xxgd.e2pro2d[ichyz] * xxgd.bspec[4][ix] +\
  xxgd.e2pro2d[ichxz] * xxgd.bspec[4][iy] +\
  xxgd.e2pro2d[ichxy] * xxgd.bspec[4][iz] -\
  xxgd.bspec[1][ix] * xxgd.bspec[1][iy] * xxgd.bspec[0][iz] -\
  xxgd.bspec[1][ix] * xxgd.bspec[0][iy] * xxgd.bspec[1][iz] -\
  xxgd.bspec[2][ix] * xxgd.bspec[1][iy] * xxgd.bspec[1][iz] -\
  xxgd.bspec[4][ix] * xxgd.bspec[3][iy] * xxgd.bspec[1][iz] -\
  xxgd.bspec[5][ix] * xxgd.bspec[4][iy] * xxgd.bspec[1][iz] -\
  xxgd.bspec[1][ix] * xxgd.bspec[4][iy] * xxgd.bspec[3][iz] -\
  xxgd.bspec[4][ix] * xxgd.bspec[1][iy] * xxgd.bspec[3][iz] -\
  xxgd.bspec[1][ix] * xxgd.bspec[5][iy] * xxgd.bspec[4][iz] -\
  xxgd.bspec[5][ix] * xxgd.bspec[1][iy] * xxgd.bspec[4][iz] -\
  xxgd.e2e2spec[ix] * xxgd.bspec[4][iy] * xxgd.bspec[4][iz] -\
  xxgd.bspec[4][ix] * xxgd.e2e2spec[iy] * xxgd.bspec[4][iz] -\
  xxgd.bspec[4][ix] * xxgd.bspec[4][iy] * xxgd.e2e2spec[iz] -\
  xxgd.bspec[4][ix] * xxgd.bspec[4][iy] * xxgd.bspec[4][iz] * xxgd.e2e2e2sum)

#define GETDAT \
 ichy = xxgd.luch[iy];

#define SUBDAT \
 y = xxgd.pro2d[ichy + ix] - BG2D;\
 SUBV;\
 r1 = elgd.bg_err * (xxgd.pro2d[ichy + ix] - y);\
 dat = xxgd.pro2d[ichy + ix] + r1 * r1;

#include "esclevb.c"

/* ======================================================================= */
void add_cubes(void)
{
  float rj1, rj2;
  int   j, nc, ix, iy, reclen, ich, icubenum;
  char  filnam[80], ans[80];

  /* add cube files in linear combinations */
  warn1("In this program, you can now specify up to five more cubes\n"
	"  to add or subtract in linear combination with the first cube.\n"
	"  These cubes MUST have been created using the cube-channel\n"
	"  lookup-table etc. as the first cube.\n\n");
  for (icubenum = 0; icubenum < 5; icubenum++) {
    xxgd.cubefact[icubenum] = 0.0f;
    while (askfn(xxgd.cubenam1[icubenum], 80, "", ".cub",
		 "Cube data file number %d = ? (rtn for no more cubes)",
		 icubenum + 1)) {
      setcubenum(icubenum + 1);
      if (open3dfile(xxgd.cubenam1[icubenum], &j)) continue;
      if (j != xxgd.numchs) {
	printf(" ERROR: That cube file has different number of channels!\n");
	close3dfile();
	continue;
      }
      xxgd.many_cubes = 1;

      while (1) {
	nc = ask(ans, 80,
		 "Gates from this cube will be combined with the gates\n"
		 "  from the other cube[s], after multiplying by factor%1d.\n"
		 "  ...factor%1d = ?", icubenum + 1, icubenum + 1);
	if (!ffin(ans, nc, &xxgd.cubefact[icubenum], &rj1, &rj2) &&
	    xxgd.cubefact[icubenum]) break;
	warn1("ERROR: Factor must be non-zero.\n");
      }

      /* get 2D projection file */
      while (1) {
	askfn(filnam, 80, "", ".2dp", "2D projection file for this cube = ?");
	if (!inq_file(filnam, &reclen)) {
	  warn1("File %s does not exist.\n", filnam);
	} else if (reclen != xxgd.numchs) {
	  warn1("ERROR: record length of 2D projection file\n"
	       "       is inconsistent with number of channels!\n"
	       "       Record length = %d\n", reclen);
	} else {
	  break;
	}
      }
      filez[1] = fopen(filnam, "r");
      /* read 2D data */
      tell("\nReading 2D projection...\n");
      reclen *= 4;
      for (iy = 0; iy < xxgd.numchs; ++iy) {
	fseek(filez[1], iy*reclen, SEEK_SET);
	fread(xxgd.spec[0], reclen, 1, filez[1]);
	for (ix = 0; ix <= iy; ++ix) {
	  ich = xxgd.luch[ix] + iy;
	  xxgd.pro2d[ich] += xxgd.cubefact[icubenum] * xxgd.spec[0][ix];
	  xxgd.dpro2d[ich] += xxgd.cubefact[icubenum] *
	                      xxgd.cubefact[icubenum] * xxgd.spec[0][ix];
	}
      }
      fclose(filez[1]);
      break;
    }
    if (!xxgd.cubefact[icubenum]) break;
  }
  read_write_l4d_file("WRITE");

} /* add_cubes */

/* ======================================================================= */
int l_gls_exec(char *ans, int nc)
{
  float x1, y1, x2, y2;
  int   iwin1, iwin2;

  /* this subroutine decodes and executes the commands */
  /* convert lower case to upper case characters */
  ans[0] = toupper(ans[0]);
  if (*ans != 'T' || !strstr(ans, "/")) ans[1] = toupper(ans[1]);

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
    lvt8r_help();
    /* SB: Search for Band  */
  } else if (!strncmp(ans, "SB", 2)) {
    check_band(ans, nc);
  } else if (!strncmp(ans, "XT", 2)) {
    examine_trip();
  } else {
    lvt8r_exec(ans, nc);
  }
  return 0;
} /* l_gls_exec */

/* ======================================================================= */
int lvt8r_exec(char *ans, int nc)
{
  float h, x, y, fj1, fj2;
  int   i1, i2, idata, j1, j2, in, in2;
  char  dattim[20], message[256];

  /* this subroutine decodes and executes the commands */
  /* convert lower case to upper case characters */
  ans[0] = toupper(ans[0]);
  if (*ans != 'T') ans[1] = toupper(ans[1]);

  if (!strncmp(ans, "AC", 2) && !strstr(xxgd.progname, "lev")) {
    add_cubes();  /* add more cube files in linear combinations */
  } else if (!strncmp(ans, "BG", 2)) {
    undo_esclev(-1, 4);  /* undo last gate changes */
  } else if (!strncmp(ans, "CA", 2)) {
    if (!askyn("Change energy and efficiency calibrations? (Y/N)")) return 0;
    /* get new calibrations */
    get_cal();
    calc_peaks();
    read_write_l4d_file("WRITE");
  } else if (!strncmp(ans, "CB", 2)) {
    if (!askyn("Change background? (Y/N)")) return 0;
    get_bkgnd();
    read_write_l4d_file("WRITE");
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
  } else if (!strncmp(ans, "FB", 2)) {
    hilite(-1);
    fit_both(0);
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FE", 2)) {
    hilite(-1);
    fit_egam(0);
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FG", 2)) {
    undo_esclev(1, 4);  /* redo last gate changes */
  } else if (!strncmp(ans, "FI", 2)) {
    hilite(-1);
    fit_igam(0);
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FTI", 3) || !strncmp(ans, "FTi", 3)) {
    hilite(-1);
    fit_igam(1);
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FTE", 3) || !strncmp(ans, "FTe", 3)) {
    hilite(-1);
    fit_egam(1);
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
  } else if (!strncmp(ans, "FTB", 3) || !strncmp(ans, "FTb", 3)) {
    hilite(-1);
    fit_both(1);
    hilite(-1);
    calc_gls_fig();
    display_gls(-1);
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
    if (elgd.lox > (i1 = (int) (0.5f + xxgd.ehi_sp[xxgd.numchs - 1] - elgd.numx)))
	elgd.lox = i1;
    disp_dat(1);
  } else if (!strncmp(ans, "MD", 2)) {
    tell("Upper channel limit = ?\n");
    retic(&x, &y, ans);
    i1 = (int) x - elgd.numx;
    i2 = (int) (0.5f + xxgd.elo_sp[0]);
    elgd.lox = (i1 > i2 ? i1 : i2);
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
    tell("Peak find activated; SIGMA, %% = %i %i\n",
	 elgd.isigma, elgd.ipercent);
    return 0;
  } else if (!strncmp(ans, "PR", 2)) {
    hilite(-1);
    project();
  } else if (!strncmp(ans, "PS", 2)) {
    if (!askyn("Change peak shape and FWHM parameters? (Y/N)")) return 0;
    /* get new calibrations */
    get_shapes();
    calc_peaks();
    read_write_l4d_file("WRITE");
  } else if (!strncmp(ans, "RC", 2)) {
    undo_esclev(1, 3);  /* redo last undone .lev/.4dg changes */
  } else if (!strncmp(ans, "RL", 2)) {
    wrlists(ans);
  } else if (!strncmp(ans, "RN", 2)) {
    save_esclev_now(3);
    while ((nc = ask(ans, 80,
		     "Height normalization factor = ? (rtn for %g)",
		     elgd.h_norm)) &&
	   ffin(ans, nc, &h, &fj1, &fj2));
    if (nc && h > 1.0e-9) elgd.h_norm = h;
    while ((nc = ask(ans, 80,
		     "Triples normalization factor = ? (rtn for %g)",
		     elgd.t_norm)) &&
	   ffin(ans, nc, &h, &fj1, &fj2));
    if (nc && h > 1.0e-9) elgd.t_norm = h;
    calc_peaks();
    read_write_l4d_file("WRITE");
    datetime(dattim);
    fprintf(filez[26], "%s:  2D Scaling factor = %g\n", dattim, elgd.h_norm);
    fprintf(filez[26], "%s:  3D Scaling factor = %g\n", dattim, elgd.t_norm);
    fflush(filez[26]);
  } else if (!strncmp(ans, "RS", 2)) {
    rw_saved_spectra(ans);
    hilite(-1);
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
	  system(message);
	} else if (*ans == 'P' || *ans == 'p') {
	  pr_and_del_file(prfilnam);
	} else if (*ans != 'S' && *ans != 's') {
	  continue;
	}
	break;
      }
    }
    save_xwg(xxgd.progname);
    exit(0);
  } else if (!strncmp(ans, "SU", 2)) {
    sum_cur();
  } else if (*ans == 'T') {
    /* T: take triples gate */
    hilite(-1);
    if (trip_gate(ans, nc)) goto ERR;
    disp_dat(0);
  } else if (!strncmp(ans, "UC", 2)) {
    undo_esclev(-1, 3);  /* undo .lev/.4dg changes */
  } else if (!strncmp(ans, "WL", 2)) {
    wrlists(ans);
  } else if (!strncmp(ans, "WS", 2)) {
    write_sp(ans, nc);
  } else if (!strncmp(ans, "XA", 2)) {
    /* XA: modify X0,NX */
    while ((nc = ask(ans, 80, "X0 = ? (return for %i)", elgd.lox)) &&
	   (inin(ans, nc, &idata, &j1, &j2) ||
	    (float) (idata + 20) > xxgd.ehi_sp[xxgd.numchs - 1]));
    if (nc) elgd.lox = idata;
    while ((nc = ask(ans, 80, "NX = ? (return for %i)", elgd.numx)) &&
	   (inin(ans, nc, &idata, &j1, &j2) || idata < 0));
    if (nc) elgd.numx = idata;
  } else if (!strncmp(ans, "X0", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2) ||
	(float) (idata + 20) > xxgd.ehi_sp[xxgd.numchs - 1]) goto ERR;
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
} /* lvt8r_exec */

/* ======================================================================= */
int lvt8r_help(void)
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
  static char fmt_15[] = "\n"
    " SB g      Search cube for (superdef) band with spacing of gate-list g.\n"
    " T                    set double-gate(s) on triples\n"
    "                       (program will prompt for gate(s)\n"
    " T g1 [w1] / g2 [w2]  set double gate on triples at energies\n"
    "                       \"g1\",\"g2\"  [widths \"w1\"*FWHM,\"w2\"*FWHM]\n"
    " T g1 [w1] / l2 [w2]  sum double gates on triples for\n"
    "                       \"g1\"       [width  \"w1\"*FWHM]\n"
    "                  "
    "     and all peaks listed in list \"l2\" [width \"w2\"*FWHM]\n"
    " T l1 [w1] / l2 [w2]  sum double gates on triples for all \n"
    "                       combinations of gates in lists \"l1\" and \"l2\"\n"
    "                                  [widths \"w1\"*FWHM,\"w2\"*FWHM]\n"
    " +T...     add double-gate(s) to current result\n"
    " -T...     subtract double-gate(s) from current result\n"
    " .T...     \"and\" double-gate(s) with current result\n\n";
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
    " NG1    display only one gate on graphics screen\n"
    " NG2    display two gates on graphics screen\n"
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
    " EL     edit level scheme\n"
    " FB     fit both energies and intensities of transitions\n"
    " FE     fit energies of level scheme transitions\n"
    " FI     fit intensities of level scheme transitions\n"
    " FTB    fit energies and intensities of transitions in Triples\n"
    " FTE    fit energies of level scheme transitions in Triples\n"
    " FTI    fit intensities of level scheme transitions in Triples\n"
    " FW     fit peak width parameters to current gate spectrum\n"
     "UC     undo the most recent gates/changes to the levit8r file\n"
     "RC     redo the most recently undone gates/changes to the levit8r file\n"
    " ST     stop and exit program\n\n";

  warn(fmt_10);
  warn(fmt_15);
  warn(fmt_20);
  warn(fmt_30);
  return 0;
} /* lvt8r_help */

/* ======================================================================= */
int check_band(char *ans, int nc)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */
  static char fmt[] =
    "List %c E1 =%7.1f %5i 3-gates,"
    " Mean cnts = %5.1f +- %5.1f %5.1f, Merit = %4.2f\n";
  float r1, calc, area, finc, cnts[455], x, dcnts[455], dx;
  float fj1, fj2, ehi, elo, sum, ehi1, ehi2, elo1, elo2, sum1, sum2, sum3;
  int   numl = 0, i, j, j1, j2, i3gate, nl, ix, iy, iz, nsteps, ihi, ilo;

  /* check gate list for presence of a rotational band */

  if (!prfile) {
    if (askyn("*** WARNING: The results of this search will not be logged since\n"
	      " you did not run the program with the -l command line flag.\n"
	      " ...Would you like to start logging now? (Y/N)")) {
      askfn(prfilnam, 80, "sdsearch.log", ".log", "Log file name = ?");
      prfile = open_new_file(prfilnam, 0);
    }
  }

  if (nc <= 2) {
    nc = ask(ans, 80, "List = ?");
    if (nc == 0) return 0;
  } else {
    nc -= 2;
    memmove(ans, ans + 2, nc);
  }
  while (*ans == ' ') {
    memmove(ans, ans + 1, nc--);
    if (nc == 0) return 0;
  }
  nl = -1;
  for (i = 0; i < 55; ++i) {
    if (*ans == listnam[i]) {
      nl = i;
      numl = elgd.nlist[nl];
    }
  }
  if (nl < 0) {
    warn1(" Bad list name...\n");
    return 0;
  }
  if (numl <= 4) {
    warn1("Not enough gates in list.\n");
    return 0;
  }
  if (numl > 15) {
    numl = 15;
    warn1("Number of transitions limited to 15...\n"
	 "   Only the first 15 energies were taken from the list.\n");
  }

  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = 0.0f;
    }
  }
  nsteps = 0;
 DOSEARCH:

  i3gate = 0;
  for (ix = 0; ix < numl - 2; ++ix) {
    x = elgd.list[nl][ix];
    dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x + elgd.swpars[2] * x * x) / 
      2.0f;
    elo1 = x - dx;
    ehi1 = x + dx;
    for (iy = ix+1; iy < numl - 1; ++iy) {
      xxgd.bf1 = 0.0f;
      xxgd.bf2 = 0.0f;
      xxgd.bf4 = 0.0f;
      xxgd.bf5 = 0.0f;
      for (j = 0; j < 6; ++j) {
	for (iz = 0; iz < xxgd.numchs; ++iz) {
	  xxgd.spec[j][iz] = 0.0f;
	}
      }
      x = elgd.list[nl][iy];
      dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		elgd.swpars[2] * x * x) / 2.0f;
      elo2 = x - dx;
      ehi2 = x + dx;
      add_trip_gate(elo1, ehi1, elo2, ehi2);
      /* subtract remaining background */
      /* correct for width of channel in keV */
      for (i = 0; i < xxgd.numchs; ++i) {
	xxgd.spec[0][i] = xxgd.spec[0][i] - xxgd.bf1 * xxgd.bspec[0][i]  - 
	  xxgd.bf2 * xxgd.bspec[1][i] - xxgd.bf4 * xxgd.bspec[3][i] -
	  xxgd.bf5 * xxgd.bspec[4][i];
	for (j = 0; j < 4; ++j) {
	  xxgd.spec[j][i] /= xxgd.ewid_sp[i];
	}
	if (!xxgd.many_cubes)
	  xxgd.spec[4][i] = xxgd.spec[3][i] / xxgd.ewid_sp[i];
	r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
	xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
	if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
      }
      /* check contents of 3-gates */
      for (iz = iy+1; iz < numl; ++iz) {
	x = elgd.list[nl][iz];
	dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		  elgd.swpars[2] * x * x) / 2.0f;
	elo = x - dx;
	ehi = x + dx;
	ilo = ichanno(elo);
	ihi = ichanno(ehi);
	/* calculate area etc */
	area = 0.0f;
	calc = 0.0f;
	sum = 0.0f;
	for (i = ilo; i <= ihi; ++i) {
	  area += xxgd.spec[0][i];
	  calc += xxgd.spec[2][i];
	  sum += xxgd.spec[5][i];
	}
	cnts[i3gate] = area - calc;
	dcnts[i3gate++] = sqrt(sum);
      }
    }
  }
  sum1 = 0.0f;
  sum2 = 0.0f;
  sum3 = 0.0f;
  for (i = 0; i < i3gate; ++i) {
    sum1 += cnts[i];
    sum2 += dcnts[i];
  }
  sum1 /= (float) i3gate;
  sum2 /= (float) i3gate;
  for (i = 0; i < i3gate; ++i) {
    r1 = cnts[i] - sum1;
    sum3 += r1 * r1;
  }
  sum3 = sqrt(sum3 / (float) (i3gate - 1));
  if (sum3 < 0.001) sum3 = 0.001;
  tell(fmt, listnam[nl], elgd.list[nl][0],
       i3gate, sum1, sum2, sum3, sum1 / sum3);
  if (prfile) fprintf(prfile, fmt, listnam[nl], elgd.list[nl][0],
		      i3gate,  sum1, sum2, sum3, sum1 / sum3);
  if (nsteps <= 0) {
    while ((nc = ask(ans, 80, "Energy increment = ? (rtn to exit)")) &&
	   ffin(ans, nc, &finc, &fj1, &fj2));
    if (nc) {
      while ((nc = ask(ans, 80, " Number of steps = ? (rtn for 1)")) &&
	     inin(ans, nc, &nsteps, &j1, &j2));
      if (!nc) nsteps = 1;
    }
  }
  if (nsteps > 0) {
    for (ix = 0; ix < numl; ++ix) {
      elgd.list[nl][ix] += finc;
    }
    --nsteps;
    goto DOSEARCH;
  }
  strcpy(xxgd.name_gat, xxgd.old_name_gat);
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.spec[j][ix] = xxgd.old_spec[j][ix];
    }
  }
  return 0;
} /* check_band */

/* ======================================================================= */
int energy(float x, float dx, float *eg, float *deg)
{
  *eg = x + 0.5f;
  *deg = dx;
  return 0;
} /* energy */

/* ======================================================================= */
int examine_trip(void)
{
  float x1, y1, egamma;
  int   igam, iwin, nc;
  char  ans[80], command[80], tmp[80];
#define NGX elgd.nlist[52] /* # of gammas set on x */
#define NGY elgd.nlist[53] /* # of gammas set on y */

  NGX = 0;
  NGY = 0;
  tell("Button 1: set or add X-gate\n"
       "Button 2: set or add Y-gate\n"
       "Button 3: show gamma\n"
       "    b/f : back/forward in gate history\n"
       " ...hit X on keyboard to exit...\n");
 START:
  igam = 0;
  retic3(&x1, &y1, ans, &iwin);
  if (!strcmp(ans, "X") || !strcmp(ans, "x")) {
    return 0;
  } else if (!strcmp(ans, "b") || !strcmp(ans, "B") ||
	     !strcmp(ans, "u") || !strcmp(ans, "U")) {
    undo_esclev(-1, 4);
    goto START;
  } else if (!strcmp(ans, "f") || !strcmp(ans, "F") ||
	     !strcmp(ans, "r") || !strcmp(ans, "R")) {
    undo_esclev(1, 4);
    goto START;
  }
  if (iwin == 2) {
    igam = nearest_gamma(x1, y1);
    if (igam < 0) {
      tell("  ... no gamma selected, try again...\n");
      goto START;
    }
    egamma = glsgd.gam[igam].e;
    tell("\n *** Egamma = %.2f +- %.2f     Igamma = %.2f +- %.2f\n",
	   glsgd.gam[igam].e, glsgd.gam[igam].de,
	   glsgd.gam[igam].i, glsgd.gam[igam].di);
  } else if (iwin == 1) {
    egamma = x1;
  } else {
    tell("Click in spectrum window for gates on peaks,\n"
	 "   or in level scheme window for gates on gammas.\n");
    goto START;
  }
  sprintf(tmp, "%.2f", egamma);
  nc = strlen(tmp);
  if (ans[1] == '1') {
    if (NGX == 0 || NGY != 0) {
      strcpy(command, tmp);
      tell("%s\n", command);
      hilite(-1);
      if (num_gate(command, nc)) goto ERR;
      NGX = 1;
      NGY = 0;
    } else {
      sprintf(command, "+%s", tmp);
      tell("%s\n", command);
      if (combine(command, nc + 1)) goto ERR;
      ++NGX;
    }
    elgd.list[52][NGX - 1] = egamma;
    disp_dat(0);
  } else if (ans[1] == '2') {
    if (NGX == 0) {
      tell("First set X-gate...\n");
    } else if (NGY == 0) {
      sprintf(command, "T [/%s", tmp);
      tell("%s\n", command);
      if (trip_gate(command, nc + 4)) goto ERR;
      disp_dat(0);
      ++NGY;
    } else {
      sprintf(command, "+T [/%s", tmp);
      tell("%s\n", command);
      if (combine(command, nc + 5)) goto ERR;
      disp_dat(0);
      ++NGY;
    }
  } else if (ans[1] == '3') {
    sprintf(command, "X%s", tmp);
    tell("%s\n", command);
    if (examine(command, nc + 1)) goto ERR;
  }
  goto START;
 ERR:
  warn1("Bad energy...\n");
  goto START;
#undef NGX
#undef NGY
} /* examine_trip */

/* ======================================================================= */
int add_gate(float elo, float ehi)
{
  float r1, a1, eg1, eg2;
  int   i, j, i1, ii, jj, ix, iy, ich, ihi, ilo;

  /* read gate of energy elo to ehi and calculate expected spectrum */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  /* spec[3][] = non-background-subtracted gate */
  /* spec[4][] = square of statistical uncertainty */
  /* spec[5][] = square of statistical plus systematic uncertainties */
  ilo = ichanno(elo);
  ihi = ichanno(ehi);
  eg1 = xxgd.elo_sp[ilo];
  eg2 = xxgd.ehi_sp[ihi];
  tell("Gate is chs %i to %i,  %.1f\n"
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
  /* copy data from 2D projection */
  for (iy = ilo; iy <= ihi; ++iy) {
    for (ix = 0; ix < iy; ++ix) {
      ich = xxgd.luch[ix] + iy;
      xxgd.spec[0][ix] += xxgd.pro2d[ich];
      xxgd.spec[3][ix] += xxgd.pro2d[ich];
      if (!xxgd.many_cubes) xxgd.spec[4][ix] += xxgd.dpro2d[ich] /
			 (xxgd.ewid_sp[ix] * xxgd.ewid_sp[ix]);
    }
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      ich = xxgd.luch[iy] + ix;
      xxgd.spec[0][ix] += xxgd.pro2d[ich];
      xxgd.spec[3][ix] += xxgd.pro2d[ich];
      if (!xxgd.many_cubes) xxgd.spec[4][ix] += xxgd.dpro2d[ich] /
			 (xxgd.ewid_sp[ix] * xxgd.ewid_sp[ix]);
    }
  }
  for (iy = ilo; iy <= ihi; ++iy) {
    /* subtract background */
    xxgd.bf1 += xxgd.bspec[1][iy];
    xxgd.bf2 += xxgd.bspec[2][iy];
    xxgd.bf4 += xxgd.bspec[4][iy];
    xxgd.bf5 += xxgd.bspec[5][iy];
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      if ((float) abs(ix - iy) < xxgd.v_width) {
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
  return 0;
} /* add_gate */

/* ======================================================================= */
int add_trip_gate(float elo1, float ehi1, float elo2, float ehi2)
{
  float save, a, br, eg1, eg2, eg3, eg4;
  int   i, j, ichxy, ichxz, ichyz, i1, i2, ix, iy, iz, jx, jy, jz;
  int   jjx, jjy, jjz, ihi1, ihi2, ilo1, ilo2;
  int   lx[MAXGAM], ly[MAXGAM], nx = 0, ny = 0;

  /* read gate of energy elo to ehi and calculate expected spectrum */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  /* spec[3][] = non-background-subtracted gate */
  /* spec[4][] = square of statistical uncertainty */
  /* spec[5][] = square of statistical plus systematic uncertainties */
  if (elo1 > ehi1) {
    save = elo1;
    elo1 = ehi1;
    ehi1 = save;
  }
  ilo1 = ichanno(elo1);
  ihi1 = ichanno(ehi1);
  eg1 = xxgd.elo_sp[ilo1];
  eg2 = xxgd.ehi_sp[ihi1];
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].e >= eg1 && glsgd.gam[j].e <= eg2) {
      hilite(j);
    }
  }
  if (elo2 > ehi2) {
    save = elo2;
    elo2 = ehi2;
    ehi2 = save;
  }
  ilo2 = ichanno(elo2);
  ihi2 = ichanno(ehi2);
  eg3 = xxgd.elo_sp[ilo2];
  eg4 = xxgd.ehi_sp[ihi2];
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].e >= eg3 && glsgd.gam[j].e <= eg4) {
      hilite(j);
    }
  }
  read_cube(ilo1, ihi1, ilo2, ihi2);
  for (iy = ilo2; iy <= ihi2; ++iy) {
    for (ix = ilo1; ix <= ihi1; ++ix) {
      if (ix <= iy) {
	ichxy = xxgd.luch[ix] + iy;
      } else {
	ichxy = xxgd.luch[iy] + ix;
      }
      xxgd.bf1 -= xxgd.bspec[1][ix] * xxgd.bspec[1][iy];
      xxgd.bf2 = xxgd.bf2 + xxgd.pro2d[ichxy] - xxgd.bspec[1][ix] * 
        xxgd.bspec[0][iy] - xxgd.bspec[2][ix] * xxgd.bspec[1][iy];
      xxgd.bf2 = xxgd.bf2 - xxgd.bspec[4][ix] * xxgd.bspec[3][iy] - 
        xxgd.bspec[5][ix] * xxgd.bspec[4][iy];
      xxgd.bf4 = xxgd.bf4 - xxgd.bspec[1][ix] * xxgd.bspec[4][iy] - 
        xxgd.bspec[4][ix] * xxgd.bspec[1][iy];
      xxgd.bf5 = xxgd.bf5 + xxgd.e2pro2d[ichxy] - xxgd.bspec[1][ix] * 
        xxgd.bspec[5][iy] - xxgd.bspec[5][ix] * xxgd.bspec[1][iy] - 
        xxgd.e2e2spec[ix] * xxgd.bspec[4][iy] - xxgd.bspec[4][ix] * 
        xxgd.e2e2spec[iy] + xxgd.bspec[4][ix] * xxgd.bspec[4][iy] * 
        xxgd.e2e2e2sum;
      for (iz = 0; iz < xxgd.numchs; ++iz) {
	/* subtract part of the background */
	if (ix < iz) {
	  ichxz = xxgd.luch[ix] + iz;
	} else {
	  ichxz = xxgd.luch[iz] + ix;
	}
	if (iy < iz) {
	  ichyz = xxgd.luch[iy] + iz;
	} else {
	  ichyz = xxgd.luch[iz] + iy;
	}
	xxgd.spec[0][iz] = xxgd.spec[0][iz] - xxgd.pro2d[ichxz] * 
	  xxgd.bspec[1][iy] - xxgd.pro2d[ichyz] * xxgd.bspec[1][ix] - 
	  xxgd.e2pro2d[ichxz] * xxgd.bspec[4][iy] - xxgd.e2pro2d[ichyz] * 
	  xxgd.bspec[4][ix] + xxgd.bspec[4][ix] * xxgd.bspec[4][iy] * 
	  xxgd.e2e2spec[iz];
      }
    }
  }

  /* calculate expected spectrum */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  for (i=0; i<glsgd.ngammas; ++i) {
    if (xxgd.lo_ch[i] <= ihi1 && xxgd.hi_ch[i] >= ilo1) lx[nx++] = i;
    if (xxgd.lo_ch[i] <= ihi2 && xxgd.hi_ch[i] >= ilo2) ly[ny++] = i;
  }

  for (i1 = 0; i1 < nx; ++i1) {
    jx = lx[i1];
    for (i2 = 0; i2 < ny; ++i2) {
      jy = ly[i2];
      if (elgd.levelbr[jy][glsgd.gam[jx].lf] == 0.0f && 
	  elgd.levelbr[jx][glsgd.gam[jy].lf] == 0.0f) continue;
      for (jz = 0; jz < glsgd.ngammas; ++jz) {
	if ((elgd.levelbr[jz][glsgd.gam[jx].lf] == 0.0f && 
	     elgd.levelbr[jx][glsgd.gam[jz].lf] == 0.0f) ||
	    (elgd.levelbr[jz][glsgd.gam[jy].lf] == 0.0f && 
	     elgd.levelbr[jy][glsgd.gam[jz].lf] == 0.0f)) continue;
	br = 0.0f;
	if (elgd.levelbr[jy][glsgd.gam[jx].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f) {
	    br = glsgd.gam[jx].i *
	      elgd.levelbr[jy][glsgd.gam[jx].lf] *
	      elgd.levelbr[jz][glsgd.gam[jy].lf];
	  } else if (elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jz].i *
	      elgd.levelbr[jx][glsgd.gam[jz].lf] *
	      elgd.levelbr[jy][glsgd.gam[jx].lf];
	  } else if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f && 
		     elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jx].i *
	      elgd.levelbr[jz][glsgd.gam[jx].lf] *
	      elgd.levelbr[jy][glsgd.gam[jz].lf];
	  }
	} else if (elgd.levelbr[jx][glsgd.gam[jy].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f) {
	    br = glsgd.gam[jy].i *
	      elgd.levelbr[jx][glsgd.gam[jy].lf] *
	      elgd.levelbr[jz][glsgd.gam[jx].lf];
	  } else if (elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jz].i *
	      elgd.levelbr[jy][glsgd.gam[jz].lf] *
	      elgd.levelbr[jx][glsgd.gam[jy].lf];
	  } else if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f && 
		     elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jy].i *
	      elgd.levelbr[jz][glsgd.gam[jy].lf] *
	      elgd.levelbr[jx][glsgd.gam[jz].lf];
	  }
	}
	for (ix = (ilo1 > xxgd.lo_ch[jx] ? ilo1 : xxgd.lo_ch[jx]);
	     ix <= ihi1 && ix <= xxgd.hi_ch[jx]; ++ix) {
	  jjx = ix - xxgd.lo_ch[jx];
	  for (iy = (ilo2 > xxgd.lo_ch[jy] ? ilo2 : xxgd.lo_ch[jy]);
	       iy <= ihi2 && iy <= xxgd.hi_ch[jy]; ++iy) {
	    jjy = iy - xxgd.lo_ch[jy];
	    a = br * xxgd.pk_shape[jx][jjx] *
	             xxgd.pk_shape[jy][jjy] * sqrt(elgd.h_norm) / elgd.t_norm;
	    elgd.hpk[0][jz] += a;
	    for (iz = xxgd.lo_ch[jz]; iz <= xxgd.hi_ch[jz]; ++iz) {
	      jjz = iz - xxgd.lo_ch[jz];
	      xxgd.spec[2][iz] += a * xxgd.pk_shape[jz][jjz];
	      if (glsgd.gam[jx].e < eg1 || glsgd.gam[jx].e > eg2 || 
		  glsgd.gam[jy].e < eg3 || glsgd.gam[jy].e > eg4) {
		xxgd.spec[1][iz] += a * xxgd.pk_shape[jz][jjz];
	      }
	    }
	  }
	}
      }
    }
  }
  return 0;
} /* add_trip_gate */

/* ======================================================================= */
int calc_peaks(void)
{
  float beta, fwhm, a, h, r, u, w, x, z, width, h2;
  float r1, u1, u7, u5, u6, eg, pos, y = 0.0f, y1 = 1.0f;
  int   i, j, ix, jx, notail, ipk;

  /* calculate peak shapes and derivatives for gammas in level scheme */
  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.npks_ch[i] = 0;
  }
  for (ipk = 0; ipk < glsgd.ngammas; ++ipk) {
    eg = glsgd.gam[ipk].e;
    x = eg;
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
      a = exp(-y * y) / y1 * 2.0f * r * beta;
      pos = x + beta * a / (a + fwhm * 1.06446705f * r1);
    }
    width = fwhm * 1.41421356f / 2.35482f;
    h = effic(eg) / sqrt(elgd.h_norm) / (a + fwhm * 1.06446705f * r1);
    /* find channel region over which peak must be calculated */
    xxgd.lo_ch[ipk] = ichanno(pos - width * 3.1f);
    xxgd.hi_ch[ipk] = ichanno(pos + width * 3.1f);
    if (!notail) {
      j = ichanno(pos - beta * 10.0f);
      if (xxgd.lo_ch[ipk] > j) xxgd.lo_ch[ipk] = j;
    }
    if (xxgd.lo_ch[ipk] < 1) xxgd.lo_ch[ipk] = 1;
    if (xxgd.hi_ch[ipk] > xxgd.numchs) xxgd.hi_ch[ipk] = xxgd.numchs;
    if (xxgd.hi_ch[ipk] - xxgd.lo_ch[ipk] > 14) {
      j = ichanno(0.75f * x + 0.25f * pos) + 7.0f;
      if (xxgd.hi_ch[ipk] > j) xxgd.hi_ch[ipk] = j;
      xxgd.lo_ch[ipk] = xxgd.hi_ch[ipk] - 14;
    }
    /* calculate peak and derivative */
    for (ix = xxgd.lo_ch[ipk]; ix <= xxgd.hi_ch[ipk]; ++ix) {
      /* put this peak into look-up table for this channel */
      if (++xxgd.npks_ch[ix] > 60) {
	warn("Severe error: no. of peaks in ch. %d is greater than 60.\n", ix);
	exit(1);
      }
      xxgd.pks_ch[ix][xxgd.npks_ch[ix] - 1] = (short) ipk;
      /* jx is index for this channel in pk_shape and pk_deriv */
      jx = ix - xxgd.lo_ch[ipk];
      x = xxgd.energy_sp[ix] - pos;
      w = x / width;
      h2 = h * xxgd.ewid_sp[ix];
      if (fabs(w) > 3.1f) {
	u1 = 0.0f;
      } else {
	u1 = r1 * exp(-w * w);
      }
      u = u1;
      a = u1 * w * 2.0f / width;
      if (notail) {
	xxgd.pk_deriv[ipk][jx] = h2 * a;
      } else {
	z = w + y;
	if (fabs(x / beta) > 12.0f) {
	  xxgd.pk_deriv[ipk][jx] = h2 * a;
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
	  xxgd.pk_deriv[ipk][jx] = h2 * (a + r * u7 *
					 (u6 - u5 * 2.0f * y) / width);
	}
      }
      xxgd.pk_shape[ipk][jx] = h2 * u;
      xxgd.w_deriv[ipk][jx] = h2 * a * w * 1.41421356f / 2.35482f -
	h2 * u / fwhm;
    }
  }
  return 0;
} /* calc_peaks */

/* ====================================================================== */
int chk_fit_igam(int i, int *igam, int *npars, float *saveint,
		 float *saveerr, int fit_trip)
{
  int j, k, l, kf, kg, lf, ki, lg, fit, fit2;

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
  fit = 0;
  if (glsgd.gam[i].e <= xxgd.elo_sp[xxgd.numchs - 1] &&
      glsgd.gam[i].e >= xxgd.ehi_sp[0] &&
      levemit.n[glsgd.gam[i].lf] > 0) {
    if (fit_trip) {
      kf = glsgd.gam[i].lf;
      for (k = 0; k < levemit.n[kf]; ++k) {
	kg = levemit.l[kf][k];
	if (glsgd.gam[kg].e <= xxgd.elo_sp[xxgd.numchs - 1] &&
	    glsgd.gam[kg].e >= xxgd.ehi_sp[0] &&
	    levemit.n[glsgd.gam[kg].lf] > 0) {
	  fit = 1;
	}
      }
    } else {
      fit = 1;
    }
  }
  if (!fit) {
    /* energy of gamma is too high or low, or there is */
    /* no decay out of gamma's final level */
    ki = glsgd.gam[i].li;
    if (levemit.n[ki] > 1) {
      /* this is not the only gamma out of initial level */
      for (k = 0; k < levemit.n[ki]; ++k) {
	kg = levemit.l[ki][k];
	if (kg != i) {
	  if (glsgd.gam[kg].e <= xxgd.elo_sp[xxgd.numchs - 1] &&
	      glsgd.gam[kg].e >= xxgd.ehi_sp[0] &&
	      levemit.n[glsgd.gam[kg].lf] > 0) {
	    if (fit_trip) {
	      lf = glsgd.gam[kg].lf;
	      for (l = 0; l < levemit.n[lf]; ++l) {
		lg = levemit.l[lf][l];
		if (glsgd.gam[lg].e <= xxgd.elo_sp[xxgd.numchs - 1] && 
		  glsgd.gam[lg].e >= xxgd.ehi_sp[0] && 
		  levemit.n[glsgd.gam[lg].lf] > 0) {
		  fit = 1;
		}
	      }
	    } else {
	      fit = 1;
	    }
	  }
	  if (!fit) {
	    fit2 = 0;
	    for (j = 0; j < *npars; ++j) {
	      if (igam[j] == kg) fit2 = 1;
	    }
	    if (!fit2) fit = 1;
	  }
	}
      }
      if (fit) {
	/* check that gamma has feeding coincidences */
	for (j = 0; j < glsgd.ngammas; j++) {
	  if (glsgd.gam[j].lf == glsgd.gam[i].li &&
	      glsgd.gam[j].e <= xxgd.elo_sp[xxgd.numchs - 1] &&
	      glsgd.gam[j].e >= xxgd.ehi_sp[0]) break;
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
    tell("Intensity of gamma %7.1f cannot be fitted.\n", glsgd.gam[i].e);
  }
  return 0;
} /* chk_fit_igam */

/* ======================================================================= */
int disp_dat(int iflag)
{
  float r1, dy_s, x, hicnt, x0, y0, dx, dy, xhi, xlo, y0_s;
  int   icol, i, nx, ny, ny1, ihi, ilo, loy;

  /* display data (gate and calculated gate) */
  /* iflag = 0 to display new data */
  /* 1 to redraw data */
  /* 2 to redraw data for all channels */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  xlo = xxgd.elo_sp[0];
  xhi = xxgd.ehi_sp[xxgd.numchs - 1];

  if (iflag == 2 || elgd.numx == 0) {
    elgd.loch = xlo;
    elgd.nchs = xhi - xlo + 1.0f;
  } else {
    elgd.loch = elgd.lox;
    elgd.nchs = elgd.numx;
  }
  elgd.hich = elgd.loch + elgd.nchs;
  if ((float) elgd.hich > xhi) elgd.hich = xhi;
  if ((float) elgd.loch < xlo) elgd.loch = xlo;
  elgd.nchs = elgd.hich - elgd.loch + 1;
  if (elgd.nchs < 10) {
    elgd.nchs = 10;
    elgd.hich = elgd.loch + 9;
  }

  /* separate subroutine for displaying more than 2 stacked gates */
  if (elgd.ngd > 2) return disp_many_sp();

  initg(&nx, &ny);
  if (elgd.pkfind && (elgd.disp_calc != 1)) ny += -30;
  erase();
  ilo = ichanno((float) elgd.loch);
  ihi = ichanno((float) elgd.hich);
  hicnt = (float) (elgd.locnt + elgd.ncnts);
  if (elgd.ncnts <= 0) {
    for (i = ilo; i <= ihi; ++i) {
      if (hicnt < xxgd.spec[0][i]) hicnt = xxgd.spec[0][i];
      if (elgd.disp_calc && hicnt < xxgd.spec[2][i]) {
	hicnt = xxgd.spec[2][i];
      }
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
  pspot(x, xxgd.spec[0][ilo]);
  for (i = ilo; i <= ihi; ++i) {
    vect(x, xxgd.spec[0][i]);
    x = xxgd.ehi_sp[i];
    vect(x, xxgd.spec[0][i]);
  }
  if (elgd.disp_calc) {
    icol = elgd.colormap[2];
    setcolor(icol);
    x = xxgd.energy_sp[ilo];
    pspot(x, xxgd.spec[2][ilo]);
    for (i = ilo; i < ihi; ++i) {
      x = xxgd.energy_sp[i];
      vect(x, xxgd.spec[2][i]);
    }
    icol = elgd.colormap[3];
    setcolor(icol);
    x = xxgd.energy_sp[ilo];
    pspot(x, xxgd.spec[1][ilo]);
    for (i = ilo; i < ihi; ++i) {
      x = xxgd.energy_sp[i];
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
    pspot(x, xxgd.spec[0][ilo] - xxgd.spec[2][ilo]);
    for (i = ilo; i <= ihi; ++i) {
      vect(x, xxgd.spec[0][i] - xxgd.spec[2][i]);
      x = xxgd.ehi_sp[i];
      vect(x, xxgd.spec[0][i] - xxgd.spec[2][i]);
    }
    /* display residuals */
    loy += ny1 << 1;
    y0 = 0.0f;
    dy = 5.0f;
    for (i = ilo; i <= ihi; ++i) {
      if (dy < (r1 = fabs(xxgd.spec[0][i] - xxgd.spec[2][i]) / 
		sqrt(xxgd.spec[5][i]))) dy = r1;
    }
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    icol = elgd.colormap[2];
    setcolor(icol);
    x = (float) elgd.loch;
    r1 = (xxgd.spec[0][ilo] - xxgd.spec[2][ilo]) / sqrt(xxgd.spec[5][ilo]);
    pspot(x, r1);
    for (i = ilo; i <= ihi; ++i) {
      r1 = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
      vect(x, r1);
      x = xxgd.ehi_sp[i];
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
    for (i = ilo; i <= ihi; ++i) {
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
  pspot(x, xxgd.old_spec[0][ilo]);
  for (i = ilo; i <= ihi; ++i) {
    vect(x, xxgd.old_spec[0][i]);
    x = xxgd.ehi_sp[i];
    vect(x, xxgd.old_spec[0][i]);
  }
  if (elgd.disp_calc) {
    icol = elgd.colormap[2];
    setcolor(icol);
    x = xxgd.energy_sp[ilo];
    pspot(x, xxgd.old_spec[2][ilo]);
    for (i = ilo; i < ihi; ++i) {
      x = xxgd.energy_sp[i];
      vect(x, xxgd.old_spec[2][i]);
    }
    icol = elgd.colormap[3];
    setcolor(icol);
    x = xxgd.energy_sp[ilo];
    pspot(x, xxgd.old_spec[1][ilo]);
    for (i = ilo; i < ihi; ++i) {
      x = xxgd.energy_sp[i];
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
    pspot(x, xxgd.old_spec[0][ilo] - xxgd.old_spec[2][ilo]);
    for (i = ilo; i <= ihi; ++i) {
      vect(x, xxgd.old_spec[0][i] - xxgd.old_spec[2][i]);
      x = xxgd.ehi_sp[i];
      vect(x, xxgd.old_spec[0][i] - xxgd.old_spec[2][i]);
    }
    /* display residuals */
    loy += ny1 << 1;
    y0 = 0.0f;
    dy = 5.0f;
    for (i = ilo; i <= ihi; ++i) {
      if (dy < (r1 = fabs(xxgd.old_spec[0][i] - xxgd.old_spec[2][i]) / 
		sqrt(xxgd.old_spec[5][i]))) dy = r1;
    }
    limg(nx, 0, ny1, loy);
    setcolor(1);
    trax(dx, x0, dy, y0, elgd.iyaxis);
    icol = elgd.colormap[2];
    setcolor(icol);
    x = (float) elgd.loch;
    r1 = (xxgd.old_spec[0][ilo] - xxgd.old_spec[2][ilo]) / 
      sqrt(xxgd.old_spec[5][ilo]);
    pspot(x, r1);
    for (i = ilo; i <= ihi; ++i) {
      r1 = (xxgd.old_spec[0][i] - xxgd.old_spec[2][i]) / 
	sqrt(xxgd.old_spec[5][i]);
      vect(x, r1);
      x = xxgd.ehi_sp[i];
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
  float x, hicnt, x0, y0, dx, dy;
  int   icol, i, j, k, nx, ny, ny1, ihi, nnd, ilo, loy, store = 0;

  /* display data (gates and calculated gates) */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  if (elgd.n_stored_sp > elgd.ngd - 1) {
    /* have too many stored spectra */
    for (i = 0; i < elgd.ngd - 1; ++i) {
      strcpy(clabel[i], clabel[i+1 + elgd.n_stored_sp - elgd.ngd]);
      for (j = 0; j < 2; ++j) {
	for (k = 0; k < MAXCHS; ++k) {
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
      for (k = 0; k < MAXCHS; ++k) {
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
	for (j = 0; j < 2; ++j) {
	  for (k = 0; k < MAXCHS; ++k) {
	    dspec[i][j][k] = dspec[i+1][j][k];
	  }
	}
      }
    } else {
      ++elgd.n_stored_sp;
    }
  }

  if (elgd.n_stored_sp == 0) elgd.n_stored_sp = 1;
  strcpy(clabel[elgd.n_stored_sp - 1], xxgd.old_name_gat);
  strcpy(clabel[elgd.n_stored_sp], xxgd.name_gat);
  for (k = 0; k < MAXCHS; ++k) {
    dspec[elgd.n_stored_sp - 1][0][k] = xxgd.old_spec[0][k];
    dspec[elgd.n_stored_sp - 1][1][k] = xxgd.old_spec[2][k];
    dspec[elgd.n_stored_sp][0][k] = xxgd.spec[0][k];
    dspec[elgd.n_stored_sp][1][k] = xxgd.spec[2][k];
  }
  /* can now finally display all the data */
  initg(&nx, &ny);
  if (elgd.pkfind) ny += -60;
  erase();
  ilo = ichanno((float) elgd.loch);
  ihi = ichanno((float) elgd.hich);
  for (nnd = 0; nnd <= elgd.n_stored_sp; ++nnd) {
    hicnt = (float) (elgd.locnt + elgd.ncnts);
    if (elgd.ncnts <= 0) {
      for (i = ilo; i <= ihi; ++i) {
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
    icol = elgd.colormap[1];
    setcolor(icol);
    x = (float) elgd.loch;
    pspot(x, dspec[nnd][0][ilo]);
    for (i = ilo; i <= ihi; ++i) {
      vect(x, dspec[nnd][0][i]);
      x = xxgd.ehi_sp[i];
      vect(x, dspec[nnd][0][i]);
    }
    if (elgd.disp_calc) {
      icol = elgd.colormap[2];
      setcolor(icol);
      x = xxgd.energy_sp[ilo];
      pspot(x, dspec[nnd][1][ilo]);
      for (i = ilo; i < ihi; ++i) {
	x = xxgd.energy_sp[i];
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
int energy_nonlin(float x, float dx, float *eg, float *deg)
{
  int  ich;

  ich = (int) (x + 0.5f);
  if (ich < 0 || ich >= xxgd.numchs) {
    *eg = *deg = 0.0f;
  } else {
    *eg = xxgd.energy_sp[ich] + (x - (float) ich) * xxgd.ewid_sp[ich];
    *deg = dx * xxgd.ewid_sp[ich];
  }
  return 0;
} /* energy_nonlin */

/* ====================================================================== */
int eval0_t(int iz, int iy, int ix, float *fit)
{
  float a, br;
  int   i1, i2, i3, jx, jy, jz, jjx, jjy, jjz;

  /* calculate fit only, using present values of the pars */

  *fit = 0.0f;
  for (i1 = 0; i1 < xxgd.npks_ch[ix]; ++i1) {
    jx = xxgd.pks_ch[ix][i1];
    jjx = ix - xxgd.lo_ch[jx];
    for (i2 = 0; i2 < xxgd.npks_ch[iy]; ++i2) {
      jy = xxgd.pks_ch[iy][i2];
      if (elgd.levelbr[jy][glsgd.gam[jx].lf] == 0.0f && 
	  elgd.levelbr[jx][glsgd.gam[jy].lf] == 0.0f) continue;
      jjy = iy - xxgd.lo_ch[jy];
      for (i3 = 0; i3 < xxgd.npks_ch[iz]; ++i3) {
	jz = xxgd.pks_ch[iz][i3];
	if ((elgd.levelbr[jz][glsgd.gam[jx].lf] == 0.0f && 
	     elgd.levelbr[jx][glsgd.gam[jz].lf] == 0.0f) ||
	     (elgd.levelbr[jz][glsgd.gam[jy].lf] == 0.0f && 
	      elgd.levelbr[jy][glsgd.gam[jz].lf] == 0.0f)) continue;
	br = 0.0f;
	if (elgd.levelbr[jy][glsgd.gam[jx].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f) {
	    br = glsgd.gam[jx].i *
	      elgd.levelbr[jy][glsgd.gam[jx].lf] *
	      elgd.levelbr[jz][glsgd.gam[jy].lf];
	  } else if (elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jz].i *
	      elgd.levelbr[jx][glsgd.gam[jz].lf] *
	      elgd.levelbr[jy][glsgd.gam[jx].lf];
	  } else if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f && 
	    elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jx].i *
	      elgd.levelbr[jz][glsgd.gam[jx].lf] *
	      elgd.levelbr[jy][glsgd.gam[jz].lf];
	  }
	} else if (elgd.levelbr[jx][glsgd.gam[jy].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f) {
	    br = glsgd.gam[jy].i *
	      elgd.levelbr[jx][glsgd.gam[jy].lf] *
	      elgd.levelbr[jz][glsgd.gam[jx].lf];
	  } else if (elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jz].i *
	      elgd.levelbr[jy][glsgd.gam[jz].lf] *
	      elgd.levelbr[jx][glsgd.gam[jy].lf];
	  } else if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f && 
	    elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jy].i *
	      elgd.levelbr[jz][glsgd.gam[jy].lf] *
	      elgd.levelbr[jx][glsgd.gam[jz].lf];
	  }
	}
	br = br * xxgd.pk_shape[jx][jjx] * xxgd.pk_shape[jy][jjy] 
	  * sqrt(elgd.h_norm) / elgd.t_norm;
	jjz = iz - xxgd.lo_ch[jz];
	a = br * xxgd.pk_shape[jz][jjz];
	*fit += a;
      }
    }
  }
  return 0;
} /* eval0_t */

/* ====================================================================== */
int eval_fdb_t(int iz, int iy, int ix, float *fit, float *derivs)
{
  float a, br, ppp;
  int   i, k, npars, i1, i2, i3, hi;
  int   kk, jx, jy, jz, jjx, jjy, jjz, lo = 0, mid = 0;

  /* calculate fit and derivatives w.r.to BOTH Egamma and Igamma */
  /* using present values of the pars */

  npars = fgd.nepars + fgd.nipars;
  *fit = 0.0f;
  for (i = 0; i < npars; ++i) {
    derivs[i] = 0.0f;
  }
  for (i1 = 0; i1 < xxgd.npks_ch[ix]; ++i1) {
    jx = xxgd.pks_ch[ix][i1];
    jjx = ix - xxgd.lo_ch[jx];
    for (i2 = 0; i2 < xxgd.npks_ch[iy]; ++i2) {
      jy = xxgd.pks_ch[iy][i2];
      if (elgd.levelbr[jy][glsgd.gam[jx].lf] == 0.0f && 
	  elgd.levelbr[jx][glsgd.gam[jy].lf] == 0.0f) continue;
      jjy = iy - xxgd.lo_ch[jy];
      for (i3 = 0; i3 < xxgd.npks_ch[iz]; ++i3) {
	jz = xxgd.pks_ch[iz][i3];
	if ((elgd.levelbr[jz][glsgd.gam[jx].lf] == 0.0f && 
	     elgd.levelbr[jx][glsgd.gam[jz].lf] == 0.0f) ||
	    (elgd.levelbr[jz][glsgd.gam[jy].lf] == 0.0f && 
	     elgd.levelbr[jy][glsgd.gam[jz].lf] == 0.0f)) continue;
	hi = -1;
	if (elgd.levelbr[jy][glsgd.gam[jx].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f) {
	    hi = jx;
	    mid = jy;
	    lo = jz;
	  } else if (elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jz;
	    mid = jx;
	    lo = jy;
	  } else if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f && 
		     elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jx;
	    mid = jz;
	    lo = jy;
	  }
	} else if (elgd.levelbr[jx][glsgd.gam[jy].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f) {
	    hi = jy;
	    mid = jx;
	    lo = jz;
	  } else if (elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jz;
	    mid = jy;
	    lo = jx;
	  } else if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f && 
		     elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jy;
	    mid = jz;
	    lo = jx;
	  }
	}
	if (hi < 0) continue;
	br = sqrt(elgd.h_norm) / elgd.t_norm *
	  elgd.levelbr[mid][glsgd.gam[hi].lf] *
	  elgd.levelbr[lo][glsgd.gam[mid].lf];
	jjz = iz - xxgd.lo_ch[jz];
	ppp = xxgd.pk_shape[jx][jjx] * xxgd.pk_shape[jy][jjy] * 
	      xxgd.pk_shape[jz][jjz];
	a = br * glsgd.gam[hi].i;
	*fit += a * ppp;
	for (kk = 0; kk < fgd.nepars; ++kk) {
	  k = fgd.idat[kk];
	  if (k == jx) {
	    derivs[kk] += a * xxgd.pk_deriv[jx][jjx] *
	      xxgd.pk_shape[jy][jjy] * xxgd.pk_shape[jz][jjz];
	  } else if (k == jy) {
	    derivs[kk] += a * xxgd.pk_shape[jx][jjx] *
	      xxgd.pk_deriv[jy][jjy] * xxgd.pk_shape[jz][jjz];
	  } else if (k == jz) {
	    derivs[kk] += a * xxgd.pk_shape[jx][jjx] *
	      xxgd.pk_shape[jy][jjy] * xxgd.pk_deriv[jz][jjz];
	  }
	}
	for (kk = 0; kk < fgd.nipars; ++kk) {
	  k = fgd.idat2[kk];
	  if (k == hi) {
	    derivs[kk + fgd.nepars] += br * ppp;
	  } else if (levemit.n[glsgd.gam[k].li] > 1) {
	    if (k == mid || k == lo) {
	      derivs[kk + fgd.nepars] += a * ppp *
		(1.0f - elgd.levelbr[k][glsgd.gam[k].li] *
		 (glsgd.gam[k].a + 1.0f)) / glsgd.gam[k].i;
	    } else if (elgd.levelbr[k][glsgd.gam[hi].lf] != 0.0f && 
		       elgd.levelbr[mid][glsgd.gam[k].li] != 0.0f) {
	      derivs[kk + fgd.nepars] += ppp *
		elgd.levelbr[k][glsgd.gam[hi].lf] * (glsgd.gam[k].a + 1.0f) *
		(elgd.levelbr[mid][glsgd.gam[k].lf] - 
		 elgd.levelbr[mid][glsgd.gam[k].li]) /
		glsgd.gam[k].i * glsgd.gam[hi].i *
		elgd.levelbr[lo][glsgd.gam[mid].lf];
	    } else if (elgd.levelbr[k][glsgd.gam[mid].lf] != 0.0f && 
		       elgd.levelbr[lo][glsgd.gam[k].li] != 0.0f) {
	      derivs[kk + fgd.nepars] += ppp *
		elgd.levelbr[k][glsgd.gam[mid].lf] * (glsgd.gam[k].a + 1.0f) *
		(elgd.levelbr[lo][glsgd.gam[k].lf] - 
		 elgd.levelbr[lo][glsgd.gam[k].li]) /
		glsgd.gam[k].i * glsgd.gam[hi].i *
		elgd.levelbr[mid][glsgd.gam[hi].lf];
	    }
	  }
	}
      }
    }
  }
  return 0;
} /* eval_fdb_t */

/* ======================================================================= */
int eval_fde_t(int iz, int iy, int ix, float*fit, float *derivs)
{
  float a, br;
  int   i, k, i1, i2, i3, kk, jx, jy, jz, jjx, jjy, jjz;

  /* calculate fit and derivatives w.r.to Egamma, */
  /* using present values of the pars */

  *fit = 0.0f;
  for (i = 0; i < fgd.npars; ++i) {
    derivs[i] = 0.0f;
  }
  for (i1 = 0; i1 < xxgd.npks_ch[ix]; ++i1) {
    jx = xxgd.pks_ch[ix][i1];
    jjx = ix - xxgd.lo_ch[jx];
    for (i2 = 0; i2 < xxgd.npks_ch[iy]; ++i2) {
      jy = xxgd.pks_ch[iy][i2];
      if (elgd.levelbr[jy][glsgd.gam[jx].lf] == 0.0f && 
	  elgd.levelbr[jx][glsgd.gam[jy].lf] == 0.0f) continue;
      jjy = iy - xxgd.lo_ch[jy];
      for (i3 = 0; i3 < xxgd.npks_ch[iz]; ++i3) {
	jz = xxgd.pks_ch[iz][i3];
	if ((elgd.levelbr[jz][glsgd.gam[jx].lf] == 0.0f && 
	     elgd.levelbr[jx][glsgd.gam[jz].lf] == 0.0f) ||
	    (elgd.levelbr[jz][glsgd.gam[jy].lf] == 0.0f && 
	     elgd.levelbr[jy][glsgd.gam[jz].lf] == 0.0f)) continue;
	br = 0.0f;
	if (elgd.levelbr[jy][glsgd.gam[jx].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f) {
	    br = glsgd.gam[jx].i *
	      elgd.levelbr[jy][glsgd.gam[jx].lf] *
	      elgd.levelbr[jz][glsgd.gam[jy].lf];
	  } else if (elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jz].i *
	      elgd.levelbr[jx][glsgd.gam[jz].lf] *
	      elgd.levelbr[jy][glsgd.gam[jx].lf];
	  } else if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f && 
		     elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jx].i *
	      elgd.levelbr[jz][glsgd.gam[jx].lf] *
	      elgd.levelbr[jy][glsgd.gam[jz].lf];
	  }
	} else if (elgd.levelbr[jx][glsgd.gam[jy].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f) {
	    br = glsgd.gam[jy].i *
	      elgd.levelbr[jx][glsgd.gam[jy].lf] *
	      elgd.levelbr[jz][glsgd.gam[jx].lf];
	  } else if (elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jz].i *
	      elgd.levelbr[jy][glsgd.gam[jz].lf] *
	      elgd.levelbr[jx][glsgd.gam[jy].lf];
	  } else if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f && 
		     elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    br = glsgd.gam[jy].i *
	      elgd.levelbr[jz][glsgd.gam[jy].lf] *
	      elgd.levelbr[jx][glsgd.gam[jz].lf];
	  }
	}
	if (br == 0.0f) continue;
	br = br * sqrt(elgd.h_norm) / elgd.t_norm;
	jjz = iz - xxgd.lo_ch[jz];
	a = br * xxgd.pk_shape[jx][jjx] * xxgd.pk_shape[jy][jjy] * 
	         xxgd.pk_shape[jz][jjz];
	*fit += a;
	for (kk = 0; kk < fgd.npars; ++kk) {
	  k = fgd.idat[kk];
	  if (k == jx) {
	    derivs[kk] += br * xxgd.pk_deriv[jx][jjx] *
	      xxgd.pk_shape[jy][jjy] * xxgd.pk_shape[jz][jjz];
	  } else if (k == jy) {
	    derivs[kk] += br * xxgd.pk_shape[jx][jjx] *
	      xxgd.pk_deriv[jy][jjy] * xxgd.pk_shape[jz][jjz];
	  } else if (k == jz) {
	    derivs[kk] += br * xxgd.pk_shape[jx][jjx] *
	      xxgd.pk_shape[jy][jjy] * xxgd.pk_deriv[jz][jjz];
	  }
	}
      }
    }
  }
  return 0;
} /* eval_fde_t */

/* ======================================================================= */
int eval_fdi_t(int iz, int iy, int ix, float *fit, float *derivs)
{
  float a, br, ppp;
  int   i, k, i1, i2, i3, hi, kk, jx, jy, jz, jjx, jjy, jjz, lo = 0, mid = 0;

  /* calculate fit and derivatives w.r.to Igamma, */
  /* using present values of the pars */

  *fit = 0.0f;
  for (i = 0; i < fgd.npars; ++i) {
    derivs[i] = 0.0f;
  }
  for (i1 = 0; i1 < xxgd.npks_ch[ix]; ++i1) {
    jx = xxgd.pks_ch[ix][i1];
    jjx = ix - xxgd.lo_ch[jx];
    for (i2 = 0; i2 < xxgd.npks_ch[iy]; ++i2) {
      jy = xxgd.pks_ch[iy][i2];
      if (elgd.levelbr[jy][glsgd.gam[jx].lf] == 0.0f && 
	  elgd.levelbr[jx][glsgd.gam[jy].lf] == 0.0f) continue;
      jjy = iy - xxgd.lo_ch[jy];
      for (i3 = 0; i3 < xxgd.npks_ch[iz]; ++i3) {
	jz = xxgd.pks_ch[iz][i3];
	if ((elgd.levelbr[jz][glsgd.gam[jx].lf] == 0.0f && 
	     elgd.levelbr[jx][glsgd.gam[jz].lf] == 0.0f) ||
	    (elgd.levelbr[jz][glsgd.gam[jy].lf] == 0.0f && 
	     elgd.levelbr[jy][glsgd.gam[jz].lf] == 0.0f)) continue;
	hi = -1;
	if (elgd.levelbr[jy][glsgd.gam[jx].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f) {
	    hi = jx;
	    mid = jy;
	    lo = jz;
	  } else if (elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jz;
	    mid = jx;
	    lo = jy;
	  } else if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f && 
	    elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jx;
	    mid = jz;
	    lo = jy;
	  }
	} else if (elgd.levelbr[jx][glsgd.gam[jy].lf] != 0.0f) {
	  if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f) {
	    hi = jy;
	    mid = jx;
	    lo = jz;
	  } else if (elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jz;
	    mid = jy;
	    lo = jx;
	  } else if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f && 
	    elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    hi = jy;
	    mid = jz;
	    lo = jx;
	  }
	}
	if (hi < 0) continue;
	br = sqrt(elgd.h_norm) / elgd.t_norm *
	  elgd.levelbr[mid][glsgd.gam[hi].lf] *
	  elgd.levelbr[lo][glsgd.gam[mid].lf];
	jjz = iz - xxgd.lo_ch[jz];
	ppp = xxgd.pk_shape[jx][jjx] * xxgd.pk_shape[jy][jjy] * 
	  xxgd.pk_shape[jz][jjz];
	a = br * ppp * glsgd.gam[hi].i;
	*fit += a;
	for (kk = 0; kk < fgd.npars; ++kk) {
	  k = fgd.idat[kk];
	  if (k == hi) {
	    derivs[kk] += br * ppp;
	  } else if (levemit.n[glsgd.gam[k].li] > 1) {
	    if (k == mid || k == lo) {
	      derivs[kk] += a *
		(1.0f - elgd.levelbr[k][glsgd.gam[k].li] *
		 (glsgd.gam[k].a + 1.0f)) / glsgd.gam[k].i;
	    } else if (elgd.levelbr[k][glsgd.gam[hi].lf] != 0.0f && 
		       elgd.levelbr[mid][glsgd.gam[k].li] != 0.0f) {
	      derivs[kk] += ppp *
		elgd.levelbr[k][glsgd.gam[hi].lf] * (glsgd.gam[k].a + 1.0f) *
	        (elgd.levelbr[mid][glsgd.gam[k].lf] - 
		 elgd.levelbr[mid][glsgd.gam[k].li]) /
		glsgd.gam[k].i * glsgd.gam[hi].i *
		elgd.levelbr[lo][glsgd.gam[mid].lf];
            } else if (elgd.levelbr[k][glsgd.gam[mid].lf] != 0.0f && 
		       elgd.levelbr[lo][glsgd.gam[k].li] != 0.0f) {
	      derivs[kk] += ppp *
		elgd.levelbr[k][glsgd.gam[mid].lf] * (glsgd.gam[k].a + 1.0f) *
	        (elgd.levelbr[lo][glsgd.gam[k].lf] -
		 elgd.levelbr[lo][glsgd.gam[k].li]) /
		glsgd.gam[k].i * glsgd.gam[hi].i *
		elgd.levelbr[mid][glsgd.gam[hi].lf];
	    }
	  }
	}
      }
    }
  }
  return 0;
} /* eval_fdi_t */

/* ======================================================================= */
int findpks(float spec[6][MAXCHS])
{
  float fwhm, pmax, x, y, chanx[300], psize[300], eg, dx, rsigma, deg, ref;
  int   maxpk, ifwhm1, ix, iy, nx, ny, ihi, ilo, ipk, npk;
  char  cchar[16];

  /* do peak search */
  rsigma = (float) elgd.isigma;
  maxpk = 300;
  energy_nonlin(1.0f, 1.0f, &x, &dx);
  fwhm = sqrt(elgd.swpars[0] + elgd.swpars[1] * x + elgd.swpars[2] * x * x);
  ifwhm1 = (int) (0.5f + fwhm / dx);
  ilo = ichanno((float) elgd.loch) + 1;
  ihi = ichanno((float) elgd.hich) - 1;
  pfind(chanx, psize, ilo, ihi, ifwhm1, rsigma, maxpk, &npk, spec);
  pmax = 0.0f;
  dx = 0.0f;
  for (ipk = 0; ipk < npk; ++ipk) {
    psize[ipk] /= xxgd.eff_sp[(int) (0.5f + chanx[ipk]) - 1];
    if (psize[ipk] > pmax) pmax = psize[ipk];
    chanx[ipk] += -1.0f;
  }
  ref = (float) elgd.ipercent * pmax / 100.0f;
  /* display markers and energies at peak positions */
  initg(&nx, &ny);
  for (ipk = 0; ipk < npk; ++ipk) {
    if (psize[ipk] > ref) {
      energy_nonlin(chanx[ipk], dx, &eg, &deg);
      x = eg;
      y = spec[0][(int) (0.5f + chanx[ipk])];
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
int fitter3d(char mode, int maxits)
{
  /* 3D fitter subroutine for levit8r / 4dg8r */
  /* call with mode = E to fit Energies,
                      I to fit Intensities,
                   or B to fit Both energies and intensities */
  static char fmt[] =
    " ****** Cannot - diag. element eq. to 0.0 ******\n"
    " Gamma energy = %.3f\n";

  float r1, r2, diff, y, chisq = 0.0f;
  float save_spec[2][MAXCHS], chisq1, flamda, dat, fit;
  int   conv = 0, i7, i8, nits, i, j, k, l, m, ichxy, ichxz, ichyz;
  int   ii, jj, kk, ll, iy, ix, iz, fitchz[MAXCHS], ich, ndf;
  int   get_cube_data = 0, isave = 0, save_cube[20000];

  /* this subroutine is a modified version of 'CURFIT', in Bevington */
  /* see page 237 */

  if (mode == 'B') fgd.npars = fgd.nepars + fgd.nipars;
  path(1);
  calc_peaks();
  for (i = 0; i < xxgd.numchs; ++i) {
    save_spec[0][i] = xxgd.spec[0][i];
    save_spec[1][i] = xxgd.spec[3][i];
  }
 START:

  if (mode == 'I') {
    if (!(filez[1] = tmpfile())) {
      warn1("ERROR - cannot open temp file!\n");
      return -1;
    }
    get_cube_data = 1;
  }

  flamda = 1.0f;
  nits = 0;

  /* turn on trapping of control-C / interupt */
  set_signal_alert(1, "Busy fitting...");
  /* evaluate fit, alpha & beta matrices, & chisq */
 NEXT_IT:
  for (j = 0; j < fgd.npars; ++j) {
    if (mode == 'B') {
      if (j < fgd.nepars) {
	fgd.pars[j] = glsgd.gam[fgd.idat[j]].e;
      } else {
	fgd.pars[j] = glsgd.gam[fgd.idat2[j - fgd.nepars]].i;
      }
    } else if (mode == 'E') {
      fgd.pars[j] = glsgd.gam[fgd.idat[j]].e;
    } else {
      fgd.pars[j] = glsgd.gam[fgd.idat[j]].i;
    }
    fgd.beta[j] = 0.0f;
    for (k = 0; k <= j; ++k) {
      fgd.alpha[k][j] = 0.0f;
    }
  }
  ndf = 0;
  chisq1 = 0.0f;
  if (mode == 'I') {
    isave = 0;
    if (!get_cube_data) {
      rewind(filez[1]);
      fread(save_cube, 80000, 1, filez[1]);
    }
  }
  for (iy = 0; iy < xxgd.numchs; ++iy) {
    if (check_signal_alert()) goto DONE;
    for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
      i = xxgd.pks_ch[iy][ii];
      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
	j = elgd.ifitdat[i][jj];
	if (iy <= xxgd.hi_ch[j]) goto L96;
      }
    }
    continue;
  L96:
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      xxgd.fitchx[ix] = 0;
      for (kk = 0; kk < xxgd.npks_ch[ix]; ++kk) {
	k = xxgd.pks_ch[ix][kk];
	for (ll = 0; ll < elgd.nfitdat[k]; ++ll) {
	  l = elgd.ifitdat[k][ll];
	  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
	    i = xxgd.pks_ch[iy][ii];
	    if (k != i) {
	      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
		j = elgd.ifitdat[i][jj];
		if (l == j && ix <= xxgd.hi_ch[l]) {
		  xxgd.fitchx[ix] = 1;
		  goto L150;
		}
	      }
	    }
	  }
	}
      }
    L150:
      ;
    }
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      if (!xxgd.fitchx[ix]) continue;
      if (mode != 'I' || get_cube_data) {
	for (i = ix; i < xxgd.numchs; ++i) {
	  xxgd.spec[0][i] = 0.0f;
	}
	read_cube(ix, ix, iy, iy);
      }
      ichxy = xxgd.luch[iy] + ix;
      for (iz = ix; iz < xxgd.numchs; ++iz) {
	fitchz[iz] = 0;
      }
      for (kk = 0; kk < xxgd.npks_ch[ix]; ++kk) {
	k = xxgd.pks_ch[ix][kk];
	for (ll = 0; ll < elgd.nfitdat[k]; ++ll) {
	  l = elgd.ifitdat[k][ll];
	  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
	    i = xxgd.pks_ch[iy][ii];
	    if (k != i) {
	      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
		j = elgd.ifitdat[i][jj];
		if (l == j) {
		  i8 = xxgd.lo_ch[l];
		  for (ich = (ix > i8 ? ix : i8); ich <= xxgd.hi_ch[l]; ++ich) {
		    fitchz[ich] = 1;
		  }
		}
	      }
	    }
	  }
	}
      }
      for (iz = ix; iz <= xxgd.numchs; ++iz) {
	if (!fitchz[iz]) continue;
	++ndf;
	ichxz = xxgd.luch[ix] + iz;
	ichyz = xxgd.luch[iy] + iz;
	if (mode == 'B') {
	  eval_fdb_t(iz, iy, ix, &fit, fgd.derivs);
	  dat = xxgd.spec[0][iz];
	} else if (mode == 'E') {
	  eval_fde_t(iz, iy, ix, &fit, fgd.derivs);
	  dat = xxgd.spec[0][iz];
	} else {
	  eval_fdi_t(iz, iy, ix, &fit, fgd.derivs);
	  if (++isave >= 20000) {
	    if (get_cube_data) {
	      fwrite(save_cube, 80000, 1, filez[1]);
	    } else {
	      fread(save_cube, 80000, 1, filez[1]);
	    }
	    isave = 0;
	  }
	  if (get_cube_data) save_cube[isave] = (int) (xxgd.spec[0][iz] + 0.5f);
	  dat = (float) save_cube[isave];
	}
	y = dat - BG3D;
	r1 = elgd.bg_err * (dat - y);
	dat += r1 * r1;
	if (dat < 1.0f) dat = 1.0f;
	diff = y - fit;
	chisq1 += diff * diff / dat;
	k = 0;
	for (l = 0; l < fgd.npars; ++l) {
	  if (fgd.derivs[l] != 0.0f) {
	    fgd.nextp[k++] = l;
	    fgd.beta[l] += diff * fgd.derivs[l] / dat;
	    for (m = 0; m < k; ++m) {
	      fgd.alpha[fgd.nextp[m]][l] += fgd.derivs[l] * 
	        fgd.derivs[fgd.nextp[m]] / dat;
	    }
	  }
	}
      }
    }
  }

  if (mode == 'I') {
    if (get_cube_data) fwrite(save_cube, 80000, 1, filez[1]);
    get_cube_data = 0;
  }
  ndf -= fgd.npars;
  chisq1 /= (float) ndf;
  /* invert modified curvature matrix to find new parameters */
  for (j = 0; j < fgd.npars; ++j) {
    if (fgd.alpha[j][j] == 0.0f) {

      if (mode == 'B') {
	if (j < fgd.nepars) {
	  tell(fmt, glsgd.gam[fgd.idat[j]].e);
	  if (prfile) fprintf(prfile, fmt, glsgd.gam[fgd.idat[j]].e);
	} else {
	  tell(fmt, glsgd.gam[fgd.idat2[j - fgd.nepars]].e);
	  if (prfile) fprintf(prfile, fmt,
			      glsgd.gam[fgd.idat2[j - fgd.nepars]].e);
	}
      } else {
	tell(fmt, glsgd.gam[fgd.idat[j]].e);
	if (prfile) fprintf(prfile, fmt, glsgd.gam[fgd.idat[j]].e);
      }

      if (fgd.npars <= 1 ||
	  !askyn("...Delete from fitted parameters? (Y/N)")) goto L490;
      --fgd.npars;

      if (mode == 'B') {
	if (j < fgd.nepars) {
	  fgd.nepars--;
	  for (i = j; i < fgd.nepars; ++i) {
	    fgd.idat[i] = fgd.idat[i+1];
	    fgd.savedat[i] = fgd.savedat[i+1];
	    fgd.saveerr[i] = fgd.saveerr[i+1];
	  }
	} else {
	  fgd.nipars--;
	  for (i = j - fgd.nepars; i < fgd.nipars; ++i) {
	    fgd.idat2[i] = fgd.idat2[i+1];
	    fgd.savedat2[i] = fgd.savedat2[i+1];
	    fgd.saveerr2[i] = fgd.saveerr2[i+1];
	  }
	}
      } else {
	for (i = j; i < fgd.npars; ++i) {
	  fgd.idat[i] = fgd.idat[i+1];
	  fgd.savedat[i] = fgd.savedat[i+1];
	  fgd.saveerr[i] = fgd.saveerr[i+1];
	}
      }

      goto START;
    }
    for (k = 0; k < j; ++k) {
      fgd.alpha[j][k] = fgd.alpha[k][j];
    }
  }
  matinv_float(fgd.alpha[0], fgd.npars, MAXFIT);
  if (check_signal_alert()) goto DONE;
  for (j = 0; j < fgd.npars; ++j) {
    fgd.delta[j] = 0.0f;
    for (k = 0; k < fgd.npars; ++k) {
      fgd.delta[j] += fgd.beta[k] * fgd.alpha[k][j];
    }
  }

  /* if chisq increased, decrease flamda & try again */
 L255:
  if (mode == 'B') {
    for (l = 0; l < fgd.npars; ++l) {
      if (l < fgd.nepars) {
	glsgd.gam[fgd.idat[l]].e = fgd.pars[l] + flamda * fgd.delta[l];
      } else {
	glsgd.gam[fgd.idat2[l - fgd.nepars]].i = fgd.pars[l] +
	  flamda * fgd.delta[l];
      }
    }
    path(0);
    calc_peaks();
    ndf = 0;
  } else if (mode == 'E') {
    for (l = 0; l < fgd.npars; ++l) {
      glsgd.gam[fgd.idat[l]].e = fgd.pars[l] + flamda * fgd.delta[l];
    }
    calc_peaks();
    ndf = 0;
  } else {
    for (l = 0; l < fgd.npars; ++l) {
      glsgd.gam[fgd.idat[l]].i = fgd.pars[l] + flamda * fgd.delta[l];
    }
    path(0);
    isave = 0;
    rewind(filez[1]);
    fread(save_cube, 80000, 1, filez[1]);
  }

  chisq = 0.0f;
  for (iy = 0; iy < xxgd.numchs; ++iy) {
    if (check_signal_alert()) goto DONE;
    for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
      i = xxgd.pks_ch[iy][ii];
      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
	j = elgd.ifitdat[i][jj];
	if (iy <= xxgd.hi_ch[j]) goto L276;
      }
    }
    continue;
  L276:
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      xxgd.fitchx[ix] = 0;
      for (kk = 0; kk < xxgd.npks_ch[ix]; ++kk) {
	k = xxgd.pks_ch[ix][kk];
	for (ll = 0; ll < elgd.nfitdat[k]; ++ll) {
	  l = elgd.ifitdat[k][ll];
	  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
	    i = xxgd.pks_ch[iy][ii];
	    if (k != i) {
	      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
		j = elgd.ifitdat[i][jj];
		if (l == j && ix <= xxgd.hi_ch[l]) {
		  xxgd.fitchx[ix] = 1;
		  goto L300;
		}
	      }
	    }
	  }
	}
      }
    L300:
      ;
    }
    for (ix = iy; ix < xxgd.numchs; ++ix) {
      if (!xxgd.fitchx[ix]) continue;
      if (mode != 'I') {
	for (i = ix; i < xxgd.numchs; ++i) {
	  xxgd.spec[0][i] = 0.0f;
	}
	read_cube(ix, ix, iy, iy);
      }
      ichxy = xxgd.luch[iy] + ix;
      for (iz = ix; iz < xxgd.numchs; ++iz) {
	fitchz[iz] = 0;
      }
      for (kk = 0; kk < xxgd.npks_ch[ix]; ++kk) {
	k = xxgd.pks_ch[ix][kk];
	for (ll = 0; ll < elgd.nfitdat[k]; ++ll) {
	  l = elgd.ifitdat[k][ll];
	  for (ii = 0; ii < xxgd.npks_ch[iy]; ++ii) {
	    i = xxgd.pks_ch[iy][ii];
	    if (k != i) {
	      for (jj = 0; jj < elgd.nfitdat[i]; ++jj) {
		j = elgd.ifitdat[i][jj];
		if (l == j) {
		  i7 = xxgd.lo_ch[l];
		  for (ich = (ix > i7 ? ix : i7); ich <= xxgd.hi_ch[l]; ++ich) {
		    fitchz[ich] = 1;
		  }
		}
	      }
	    }
	  }
	}
      }
      for (iz = ix; iz <= xxgd.numchs; ++iz) {
	if (!fitchz[iz]) continue;
	eval0_t(iz, iy, ix, &fit);
	ichxz = xxgd.luch[ix] + iz;
	ichyz = xxgd.luch[iy] + iz;
	if (mode == 'I') {
	  if (++isave >= 20000) {
	    fread(save_cube, 80000, 1, filez[1]);
	    isave = 0;
	  }
	  dat = (float) save_cube[isave];
	} else {
	  ++ndf;
	  dat = xxgd.spec[0][iz];
	}
	y = dat - BG3D;
	r1 = elgd.bg_err * (dat - y);
	dat += r1 * r1;
	if (dat < 1.0f) dat = 1.0f;
	diff = y - fit;
	chisq += diff * diff / dat;
      }
    }
  }
  if (mode != 'I') ndf -= fgd.npars;
  chisq /= (float) ndf;
  tell("*** nits, flamda, chisq, chisq1 = %i %.2e %.4f %.4f\n",
       nits, flamda, chisq, chisq1);
  if (chisq > chisq1 * 1.0001f && flamda > 0.01f) {
    flamda /= 4.0f;
    goto L255;
  }
  /* evaluate parameters and errors */
  /* test for convergence */
  conv = 1;
  for (j = 0; j < fgd.npars; ++j) {
    if (fgd.alpha[j][j] < 0.0f) fgd.alpha[j][j] = 0.0f;
    fgd.ers[j] = sqrt(fgd.alpha[j][j]);
    if (flamda * fabs(fgd.delta[j]) >= fgd.ers[j] / 100.0f) conv = 0;
  }
  if (flamda < 1.0f) flamda *= 2.0f;
  if (!conv && ++nits < maxits) goto NEXT_IT;

 DONE:
  if (check_signal_alert()) {
    tell("**** Fit Interrupted (Control-C) ****\n"
	 "**** WARNING: Parameter values and errors may be undefined!\n");
    if (prfile)
      fprintf(prfile, "**** Fit Interrupted (Control-C) ****\n"
	      "**** WARNING: Parameter values and errors may be undefined!\n");
  }
  /* turn off trapping of control-C / interupt */
  set_signal_alert(0, "");
  /* list data and exit */
  r1 = chisq;
  if (chisq < 1.0f) r1 = 1.0f;
  for (l = 0; l < fgd.npars; ++l) {

    if (mode == 'B') {
      if (l < fgd.nepars) {
	r2 = elgd.encal_err;
	glsgd.gam[fgd.idat[l]].de =
	  sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
      } else {
	r2 = elgd.effcal_err * glsgd.gam[fgd.idat2[l - fgd.nepars]].i;
	glsgd.gam[fgd.idat2[l - fgd.nepars]].di =
	  sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
      }
    } else if (mode == 'E') {
      r2 = elgd.encal_err;
      glsgd.gam[fgd.idat[l]].de =
	sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
    } else {
      r2 = elgd.effcal_err * glsgd.gam[fgd.idat[l]].i;
      glsgd.gam[fgd.idat[l]].di = sqrt(fgd.ers[l] * fgd.ers[l] * r1 + r2 * r2);
    }

  }
  tell("\n%i indept. pars   %i degrees of freedom.\n", fgd.npars, ndf);
  if (prfile) fprintf(prfile,
	  "\n%i indept. pars   %i degrees of freedom.\n", fgd.npars, ndf);
  if (conv) {
    tell("%i iterations,    Chisq/D.O.F.= %.4f\n", nits, chisq);
    if (prfile) fprintf(prfile,
			"%i iterations,    Chisq/D.O.F.= %.4f\n", nits, chisq);
    for (i = 0; i < xxgd.numchs; ++i) {
      xxgd.spec[0][i] = save_spec[0][i];
      xxgd.spec[3][i] = save_spec[1][i];
    }
    if (mode == 'I') fclose(filez[1]);
    return 0;
  }
 L490:
  tell("***** Failed to converge after %i iterations.\n"
       "      Chisq/D.O.F.= %.4f\n", nits, chisq);
  if (prfile) fprintf(prfile,
		      "***** Failed to converge after %i iterations.\n"
		      "      Chisq/D.O.F.= %.4f\n", nits, chisq);
  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.spec[0][i] = save_spec[0][i];
    xxgd.spec[3][i] = save_spec[1][i];
  }
  if (mode == 'I') fclose(filez[1]);
  return 1;
} /* fitter3d */

/* ====================================================================== */
int gate_sum_list(char *ans)
{
  float r1, x, listx[40], wfact1, dx, ms_err, ehi1, elo1;
  int   numx, i, j, nc, ix;

  /* sum gates on list of gammas */
  if (((nc = strlen(ans) - 2) <= 0 ||
      get_list_of_gates(ans + 2, nc, &numx, listx, &wfact1)) &&
      (!(nc = ask(ans, 80, "Gate list = ?")) ||
       get_list_of_gates(ans, nc, &numx, listx, &wfact1))) return 1;
  if (numx == 0) return 1;
  save_esclev_now(4);
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  sprintf(xxgd.name_gat, "LIST %.9s", ans);
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = 0.0f;
    }
  }
  xxgd.bf1 = 0.0f;
  xxgd.bf2 = 0.0f;
  xxgd.bf4 = 0.0f;
  xxgd.bf5 = 0.0f;
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }
  for (ix = 0; ix < numx; ++ix) {
    x = listx[ix];
    dx = wfact1 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		       elgd.swpars[2] * x * x) / 2.0f;
    elo1 = x - dx;
    ehi1 = x + dx;
    add_gate(elo1, ehi1);
  }
  /* subtract background */
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    xxgd.spec[0][ix] = xxgd.spec[0][ix] - xxgd.bf1 * xxgd.bspec[0][ix] -
      xxgd.bf2 * xxgd.bspec[1][ix] - xxgd.bf4 * xxgd.bspec[3][ix] -
      xxgd.bf5 * xxgd.bspec[4][ix];
  }
  /* correct for width of channel in keV */
  for (i = 0; i < xxgd.numchs; ++i) {
    for (j = 0; j < 4; ++j) {
      xxgd.spec[j][i] /= xxgd.ewid_sp[i];
    }
    if (!xxgd.many_cubes) xxgd.spec[4][i] = xxgd.spec[3][i] / xxgd.ewid_sp[i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
  }
  ms_err /= (float) xxgd.numchs;
  tell("Mean square error on calculation = %.2f\n", ms_err);
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

  float fsum1, fsum2, fsum3, f, fj1, fj2, factor1, factor2;
  float save_spec[2][MAXCHS], bspec2[4][MAXCHS];
  int   ishi, islo, i, j, k, nc, ix, iy, ich, nch;
  char  filnam[80], dattim[20], namesp[16], ans[80];

  save_esclev_now(3);
  xxgd.le2pro2d = 0;
  /* get projections and background spectra */
  datetime(dattim);
  if (!askyn("Use enhanced background from E2-bump gates? (Y/N)")) {
    fprintf(filez[26], "%s: New background spectra, no E2-enhancement.\n",
	    dattim);
    fflush(filez[26]);
    for (i = 0; i < 4; i += 2) {
      while (1) {
	askfn(filnam, 80, "", ".spe", stitle[i]);
	if (read_spe_file(filnam, &xxgd.bspec[i][0], namesp, &nch, MAXCHS))
	  continue;
	if (nch == xxgd.numchs) break;
	warn1(" ERROR - spectrum has wrong length.\n");
      }
      fprintf(filez[26], "       ...%.38s %s\n", stitle[i], filnam);
    }
    fsum1 = 0.0f;
    for (i = 0; i < xxgd.numchs; ++i) {
      fsum1 += xxgd.bspec[0][i];
      bspec2[0][i] = xxgd.bspec[0][i] - xxgd.bspec[2][i];
    }
    for (j = 0; j < xxgd.numchs; ++j) {
      bspec2[1][j] = bspec2[2][j] = bspec2[3][j] = 0.0f;
      xxgd.bspec[2][j] /= fsum1;
    }

  } else {
    fprintf(filez[26], "%s: New background spectra, E2-enhanced.\n", dattim);
    fflush(filez[26]);
    for (i = 0; i < 4; ++i) {
      while (1) {
	askfn(filnam, 80, "", ".spe", stitle[i]);
	if (read_spe_file(filnam, &xxgd.bspec[i][0], namesp, &nch, MAXCHS))
	  continue;
	if (nch == xxgd.numchs) break;
	warn1(" ERROR - spectrum has wrong length.\n");
      }
      fprintf(filez[26], "       ...%.38s %s\n", stitle[i], filnam);
    }
    fsum1 = 0.0f;
    fsum2 = 0.0f;
    for (i = 0; i < xxgd.numchs; ++i) {
      fsum1 += xxgd.bspec[0][i];
      fsum2 += xxgd.bspec[1][i];
      bspec2[0][i] = xxgd.bspec[0][i] - xxgd.bspec[2][i];
    }
    if (fsum2 == 0.0f) {
      for (j = 0; j < xxgd.numchs; ++j) {
	bspec2[1][j] = bspec2[2][j] = bspec2[3][j] = 0.0f;
	xxgd.bspec[2][j] /= fsum1;
      }
    } else {
      while (1) {
	factor1 = fsum1 / fsum2;
	if (!(k = ask(ans, 80, " Factor 1 = ? (rtn for %f)", factor1)) ||
	    !ffin(ans, k, &factor1, &fj1, &fj2)) break;
      }
      fprintf(filez[26], "       ...Factor1 = %f\n", factor1);
      for (j = 0; j < xxgd.numchs; ++j) {
	bspec2[1][j] = xxgd.bspec[1][j] - xxgd.bspec[0][j] / factor1;
	bspec2[2][j] = xxgd.bspec[3][j] - xxgd.bspec[2][j] / factor1;
	bspec2[3][j] = bspec2[1][j] - bspec2[2][j];
	xxgd.bspec[2][j] /= fsum1;
      }
      while (1) {
	askfn(ans, 80, "", ".win", " E2 gate file = ?");
	if ((filez[1] = open_readonly(ans))) break;
      }
      fprintf(filez[26], "       ...E2 gate file = %s\n", ans);
      fsum3 = 0.0f;
      for (i = 0; i < xxgd.matchs; ++i) {
	xxgd.e2pro2d[i] = 0.0f;
      }
      for (i = 0; i < xxgd.numchs; ++i) {
	save_spec[0][i] = xxgd.spec[0][i];
	save_spec[1][i] = xxgd.spec[3][i];
	xxgd.e2e2spec[i] = 0.0f;
      }
      xxgd.e2e2e2sum = 0.0f;
      while (fgets(lx, 120, filez[1]) &&
	     sscanf(lx, "%*5c%d%*3c%d", &islo, &ishi) == 2) {
	if (islo < 0) islo = 0;
	if (ishi > xxgd.numchs - 1) ishi = xxgd.numchs - 1;
	if (islo > ishi) {
	  i = ishi;
	  ishi = islo;
	  islo = i;
	}
	if (islo < 0) islo = 0;
	if (ishi > xxgd.numchs - 1) ishi = xxgd.numchs - 1;
	for (i = islo; i <= ishi; ++i) {
	  fsum3 += bspec2[1][i];
	}
	tell("Reading E2 2D matrix from cube....\n"
	     "  ...It may take a while...\n\n");
	for (ix = 0; ix < xxgd.numchs; ++ix) {
	  tell("Ch. %i\r", ix+1);
	  for (i = ix; i < xxgd.numchs; ++i) {
	    xxgd.spec[0][i] = 0.0f;
	  }
	  read_cube(islo, ishi, ix, ix);
	  for (iy = ix; iy < xxgd.numchs; ++iy) {
	    ich = xxgd.luch[ix] + iy;
	    xxgd.e2pro2d[ich] += xxgd.spec[0][iy];
	  }
	}
	for (ix = 0; ix < xxgd.numchs; ++ix) {
	  for (iy = islo; iy <= ishi; ++iy) {
	    if (ix < iy) {
	      ich = xxgd.luch[ix] + iy;
	    } else {
	      ich = xxgd.luch[iy] + ix;
	    }
	    xxgd.e2e2spec[ix] += xxgd.e2pro2d[ich];
	  }
	}
	for (i = islo; i <= ishi; ++i) {
	  xxgd.e2e2e2sum += xxgd.e2e2spec[i];
	}
      }
      fclose(filez[1]);
      for (i = 0; i < xxgd.matchs; ++i) {
	xxgd.e2pro2d[i] -= xxgd.pro2d[i] / factor1;
      }
      for (i = 0; i < xxgd.numchs; ++i) {
	xxgd.spec[0][i] = save_spec[0][i];
	xxgd.spec[3][i] = save_spec[1][i];
	xxgd.e2e2spec[i] = xxgd.e2e2spec[i] - xxgd.bspec[0][i] /
	  (factor1 * factor1) - bspec2[1][i] * 2.0f / factor1;
      }
      xxgd.le2pro2d = 1;
      while (1) {
	factor2 = fsum3;
	if (!(k = ask(ans, 80, " Factor 2 = ? (rtn for %f)", factor2)) ||
	    !ffin(ans, k, &factor2, &fj1, &fj2)) break;
      }
      fprintf(filez[26], "       ...Factor2 = %f\n", factor2);
      for (j = 0; j < xxgd.numchs; ++j) {
	bspec2[2][j] /= factor2;
      }
      xxgd.e2e2e2sum -= (factor2 * 3 + fsum2 / factor1) / factor1;
    }
  }

  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.bspec[1][i] = xxgd.bspec[2][i];
    xxgd.bspec[2][i] = bspec2[0][i];
    xxgd.bspec[3][i] = bspec2[1][i];
    xxgd.bspec[4][i] = bspec2[2][i];
    xxgd.bspec[5][i] = bspec2[3][i];
  }
  while ((nc = ask(ans, 80,
		   "This program adds systematic errors for the background subtraction.\n"
		   "Percentage error on background = ? (rtn for %.1f)",
		   elgd.bg_err * 100.0f)) &&
	 ffin(ans, nc, &f, &fj1, &fj2));
  if (nc) elgd.bg_err = f / 100.0f;
  datetime(dattim);
  fprintf(filez[26], "%s: Percentage error on background = %f\n",
	  dattim, elgd.bg_err * 100.0f);
  fflush(filez[26]);
  return 0;
} /* get_bkgnd */

/* ======================================================================= */
int get_cal(void)
{
  double gain[6],f1, f2, x1, x2, x3;
  float  eff_pars[10], x, eg, f, fj1, fj2, eff;
  int    contract, i, j, j1, j2, nc, jj, iorder, nterms;
  char   title[80], filnam[80], dattim[20], ans[80];

  save_esclev_now(1);
  /* get new energy and efficiency calibrations */
  /* get new energy calibration */
  while (1) {
    askfn(filnam, 80, "", "",
	  "Energy calibration file = ? (default .ext = .aca, .cal)");
    if (!read_cal_file(filnam, title, &iorder, gain)) break;
  }
  nterms = iorder + 1;
  datetime(dattim);
  fprintf(filez[26], "%s: Energy calibration file = %s\n", dattim, filnam);
  fflush(filez[26]);
  while (1) {
    contract = 2;
    if ((nc = ask(ans, 80, "Contraction factor between this calibration\n"
		  "  and the ADC data on tape = ?\n"
		  "   (e.g. if the ADC is 16384 chs, but your calibration\n"
		  "    spectrum is 4096 chs, then you should enter 4 here.)\n"
		  "   (rtn for 2)")))
      inin(ans, nc, &contract, &j1, &j2);
    if (contract > 0) break;
    warn1("ERROR -- Illegal value, try again...\n");
  }
  fprintf(filez[26], "     ...Contraction factor = %i\n", contract);

  /* get efficiency calibration */
  while (1) {
    askfn(filnam, 80, "", "",
	  "Efficiency parameter file = ? (default .ext = .aef, .eff)");
    if (!read_eff_file(filnam, title, eff_pars)) break;
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Efficiency calibration file = %s\n", dattim, filnam);
  fflush(filez[26]);

  /* calculate energy and efficiency spectra */
  jj = 0;
  for (i = 0; i < xxgd.nclook; ++i) {
    if (xxgd.looktab[i] != jj) {
      x = ((float) i - 0.5f) / (float) contract -
	  ((float) contract / 2.0f - 0.5f);
      eg = gain[nterms - 1];
      for (j = nterms - 1; j >= 1; --j) {
	eg = gain[j - 1] + eg * x;
      }
      if (jj != 0) xxgd.ehi_sp[jj - 1] = eg;
      jj = xxgd.looktab[i];
      if (jj != 0) xxgd.elo_sp[jj - 1] = eg;
    }
  }
  if (xxgd.looktab[xxgd.nclook - 1] != 0) {
    x = ((float) xxgd.nclook - 1.0f);
    eg = gain[nterms - 1];
    for (j = nterms - 1; j >= 1; --j) {
      eg = gain[j - 1] + eg * x;
    }
    xxgd.ehi_sp[xxgd.looktab[xxgd.nclook - 1] - 1] = eg;
  }
  for (i = 0; i < xxgd.numchs; ++i) {
    eg = (xxgd.elo_sp[i] + xxgd.ehi_sp[i]) / 2.0f;
    x1 = log(eg / eff_pars[7]);
    x2 = log(eg / eff_pars[8]);
    f1 = eff_pars[0] + eff_pars[1] * x1 + eff_pars[2] * x1 * x1;
    f2 = eff_pars[3] + eff_pars[4] * x2 + eff_pars[5] * x2 * x2;
    if (f1 <= 0. || f2 <= 0.) {
      eff = 1.0f;
    } else {
      x3 = exp(-eff_pars[6] * log(f1)) + exp(-eff_pars[6] * log(f2));
      if (x3 <= 0.) {
	eff = 1.0f;
      } else {
	eff = exp(exp(-log(x3) / eff_pars[6]));
      }
    }
    xxgd.eff_sp[i] = eff;
    xxgd.energy_sp[i] = eg;
    xxgd.ewid_sp[i] = xxgd.ehi_sp[i] - xxgd.elo_sp[i];
  }
  while ((nc = ask(ans, 80,
		   "This program adds systematic errors for\n"
		   " the energy and efficiency calibrations.\n"
		   "Error for energy calibration in keV = ? (rtn for %.3f)",
		   elgd.encal_err)) &&
	 ffin(ans, nc, &elgd.encal_err, &fj1, &fj2));
  while ((nc = ask(ans, 80,
		   "Percentage error on efficiency = ? (rtn for %.1f)",
		   elgd.effcal_err * 100.0f)) &&
	 ffin(ans, nc, &f, &fj1, &fj2));
  if (nc) elgd.effcal_err = f / 100.0f;
  datetime(dattim);
  fprintf(filez[26], "%s: Error for energy calibration = %f\n",
	  dattim, elgd.encal_err);
  fprintf(filez[26], "%s: Percentage error on efficiency = %f\n",
	  dattim, elgd.effcal_err * 100.0f);
  fflush(filez[26]);
  return 0;
} /* get_cal */

/* ======================================================================= */
int get_gate(float elo, float ehi)
{
  float r1, save, ms_err, eg1, eg2;
  int   i, j, ix, ihi, ilo;

  /* read gate of energy elo to ehi and calculate expected spectrum */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  /* spec[3][] = non-background-subtracted gate */
  /* spec[4][] = square of statistical uncertainty */
  /* spec[5][] = square of statistical plus systematic uncertainties */
  if (elo > ehi) {
    save = elo;
    elo = ehi;
    ehi = save;
  }
  ilo = ichanno(elo);
  ihi = ichanno(ehi);
  eg1 = xxgd.elo_sp[ilo];
  eg2 = xxgd.ehi_sp[ihi];
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  sprintf(xxgd.name_gat, " %i to %i, %.1f\n", ilo, ihi, (eg1 + eg2) / 2.0f);
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = 0.0f;
    }
  }
  /* copy data from 2D projection */
  xxgd.bf1 = 0.0f;
  xxgd.bf2 = 0.0f;
  xxgd.bf4 = 0.0f;
  xxgd.bf5 = 0.0f;
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }
  add_gate(elo, ehi);
  /* subtract background */
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    xxgd.spec[0][ix] = xxgd.spec[0][ix] - xxgd.bf1 * xxgd.bspec[0][ix] - xxgd.bf2 
      * xxgd.bspec[1][ix] - xxgd.bf4 * xxgd.bspec[3][ix] - xxgd.bf5 * 
      xxgd.bspec[4][ix];
  }
  /* correct for width of channel in keV */
  for (i = 0; i < xxgd.numchs; ++i) {
    for (j = 0; j < 4; ++j) {
      xxgd.spec[j][i] /= xxgd.ewid_sp[i];
    }
    if (!xxgd.many_cubes) xxgd.spec[4][i] = xxgd.spec[3][i] / xxgd.ewid_sp[i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
  }
  ms_err /= (float) xxgd.numchs;
  tell("Mean square error on calculation = %.2f\n", ms_err);
  return 0;
} /* get_gate */

/* ======================================================================= */
int get_list_of_gates(char *ans, int nc, int *outnum, float *outlist,
		      float *wfact)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */
  float eg, fj1, fj2, tmp;
  int   i, j, k, nl;

  /* read Egamma / list_name and width factor from ANS */
  /* return list of energies OUTLIST with OUTNUM energies */

  *outnum = 0;
  if (nc == 0) return 0;
  while (*ans == ' ') {
    memmove(ans, ans + 1, nc--);
    ans[nc]=' ';
    if (nc == 0) return 0;
  }
  for (i = 0; i < 55; ++i) {
    if (*ans == listnam[i]) {
      /* list name found */
      nl = i;
      *wfact = 1.0f;
      if (nc >= 3 &&
	  ffin(ans + 2, nc - 2, wfact, &fj1, &fj2)) return 1;
      if (*wfact <= 0.0f) *wfact = 1.0f;
      for (j = 0; j < elgd.nlist[nl]; ++j) {
	eg = elgd.list[nl][j];
	if (eg >= xxgd.energy_sp[0] &&
	    eg <= xxgd.energy_sp[xxgd.numchs - 1]) outlist[(*outnum)++] = eg;
      }
      /* order the energies in the list */
      for (j = 0; j < *outnum - 1; ++j) {
	if (outlist[j] > outlist[j + 1]) {
	  tmp = outlist[j];
	  outlist[j] = outlist[j + 1];
	  outlist[j + 1] = tmp;
	  for (k = j - 1; k >= 0 && outlist[k] > outlist[k + 1]; k--) {
	    tmp = outlist[k];
	    outlist[k] = outlist[k + 1];
	    outlist[k + 1] = tmp;
	  }
	}
      }
      return 0;
    }
  }
  if (ffin(ans, nc, &eg, wfact, &fj2) ||
      eg < xxgd.energy_sp[0] ||
      eg > xxgd.energy_sp[xxgd.numchs - 1]) return 1;
  if (*wfact <= 0.0f) *wfact = 1.0f;
  *outnum = 1;
  outlist[0] = eg;
  return 0;
} /* get_list_of_gates */

/* ======================================================================= */
int get_lookup_table(void)
{
  char  filnam[80], dattim[20];

  /* get lookup table */
  save_esclev_now(1);
  while (1) {
    askfn(filnam, 80, "", ".tab",
	 "The replay program used a lookup table to convert\n"
	 "      ADC channels to nonlinear channels...\n"
	 "Lookup table file = ?");
    if (read_tab_file(filnam, &xxgd.nclook, &xxgd.lookmin, &xxgd.lookmax, 
		      xxgd.looktab, 16384)) continue;
    if (xxgd.lookmax == xxgd.numchs) break;
    warn1("No. of values in lookup table is not %d\n", xxgd.numchs);
  }
  datetime(dattim);
  fprintf(filez[26], "%s: ADC-to-Cube lookup table file = %s\n",
	  dattim, filnam);
  fflush(filez[26]);
  return 0;
} /* get_lookup_table */

/* ======================================================================= */
int get_shapes(void)
{
  float rj2, sw1, sw2, sw3, newp[4];
  int   i, k, nc;
  char  dattim[20], ans[80];

  save_esclev_now(1);
  /* get new peak shape and FWHM parameters */
  elgd.finest[0] *= 2.0f;
  elgd.finest[2] *= 2.0f;
  elgd.swpars[0] *= 4.0f;
  elgd.swpars[1] *= 2.0f;
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
		    " In the the following, the energy dispersion is assumed"
		    " to be 0.5 keV/ch\n\n"
		    " The parameters R and BETA define the shapes of the peaks.\n"
		    "        The peak is the sum of a gaussian of height H*(1-R/100)\n"
		    "        and a skew gaussian of height H*R/100, where BETA is \n"
		    "        the decay constant of the skew gaussian (in channels).\n\n"
		    " R is taken as R = A + B*x    (x = ch. no.)\n"
		    /* " The default values for A and B are %.1f and %.5f.\n", */
		    "Enter A,B (rtn for %.1f, %.5f)",
		    elgd.finest[0], elgd.finest[1])) &&
	   ffin(ans, k, &newp[0], &newp[1], &rj2));
    if (!k) {
      newp[0] = elgd.finest[0];
      newp[1] = elgd.finest[1];
    }
    while ((k = ask(ans, 80,
		    " BETA is taken as BETA = C + D*x (x = ch. no.)\n"
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
		   "     FWHM = SQRT(F*F + G*G*x + H*H*x*x) (x = ch.no./1000)\n"
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
  /* contract gain */
  elgd.finest[0] *= 0.5f;
  elgd.finest[2] *= 0.5f;
  elgd.swpars[0] *= 0.25f;
  elgd.swpars[1] *= 0.5f;
  return 0;
} /* get_shapes */

/* ======================================================================= */
int num_gate(char *ans, int nc)
{
  float x, wfact, eg, dx, fj2, ehi, elo;

  /* calculate gate channels ilo, ihi from specified Egamma
     and width_factor * parameterized_FWHM */
  if (ffin(ans, nc, &eg, &wfact, &fj2) ||
      eg < xxgd.energy_sp[0] ||
      eg > xxgd.energy_sp[xxgd.numchs - 1]) return 1;
  save_esclev_now(4);
  if (wfact <= 0.0f) wfact = 1.0f;
  x = eg;
  dx = wfact * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		    elgd.swpars[2] * x * x) / 2.0f;
  elo = x - dx;
  ehi = x + dx;
  get_gate(elo, ehi);
  return 0;
} /* num_gate */

/* ======================================================================= */
int project(void)
{
  float r1, fact, a1, a2, s1, s2, ms_err;
  int   i, j, ii, ix;

  save_esclev_now(4);
  /* set displayed spectrum to background-subtracted projection */
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
  /* correct for width of channel in keV */
  for (i = 0; i < xxgd.numchs; ++i) {
    for (j = 0; j < 4; ++j) {
      xxgd.spec[j][i] /= xxgd.ewid_sp[i];
    }
    xxgd.spec[4][i] = xxgd.spec[3][i] / xxgd.ewid_sp[i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
  }
  ms_err /= (float) xxgd.numchs;
  tell("Mean square error on calculation = %.2f\n", ms_err);
  disp_dat(0);
  return 0;
} /* project */

/* ======================================================================= */
int read_cube(int ilo1, int ihi1, int ilo2, int ihi2)
{
  int   i, icubenum, ispec[MAXCHS];

  if (!xxgd.many_cubes) {
    read3dcube(ilo1, ihi1, ilo2, ihi2, ispec);
    for (i = 0; i < xxgd.numchs; ++i) {
      xxgd.spec[0][i] += (float) ispec[i];
      xxgd.spec[3][i] += (float) ispec[i];
    }

  } else {
    /* stuff added for linear combinations of cubes: */
    setcubenum(0);
    read3dcube(ilo1, ihi1, ilo2, ihi2, ispec);
    for (i = 0; i < xxgd.numchs; ++i) {
      xxgd.spec[0][i] += (float) ispec[i];
      xxgd.spec[3][i] += (float) ispec[i];
      xxgd.spec[4][i] += (float) ispec[i] /
	(xxgd.ewid_sp[i] * xxgd.ewid_sp[i]);
    }

    for (icubenum = 0; icubenum < 5 && xxgd.cubefact[icubenum]; icubenum++) {
      setcubenum(icubenum + 1);
      read3dcube(ilo1, ihi1, ilo2, ihi2, ispec);
      for (i = 0; i < xxgd.numchs; ++i) {
	xxgd.spec[0][i] += xxgd.cubefact[icubenum] * (float) ispec[i];
	xxgd.spec[3][i] += xxgd.cubefact[icubenum] * (float) ispec[i];
	xxgd.spec[4][i] += xxgd.cubefact[icubenum] *
                           xxgd.cubefact[icubenum] * (float) ispec[i] /
	  (xxgd.ewid_sp[i]*xxgd.ewid_sp[i]);
      }
    }
  }
  return 0;
} /* read_cube */

/* ======================================================================= */
int sum_eng(float elo, float ehi)
{
  float r1, calc, area, cent, save, x, dc, eg, eg1, eg2, deg, cts, sum;
  int   i, j, nx, ny, ihi, ilo;

  /* sum results over energy elo to ehi */
  if (elo > ehi) {
    save = elo;
    elo = ehi;
    ehi = save;
  }
  ilo = ichanno(elo);
  ihi = ichanno(ehi);
  eg1 = xxgd.elo_sp[ilo];
  eg2 = xxgd.ehi_sp[ihi];
  tell("Sum over energy %.1f to %.1f,  includes level scheme gammas:\n",
	 eg1, eg2);
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
  if (eg1 >= (float) elgd.loch && eg1 <= (float) elgd.hich) {
    x = eg1;
    pspot(x, fy0);
    vect(x, fy0 + fdy / 2.0f);
  }
  if (eg2 >= (float) elgd.loch && eg2 <= (float) elgd.hich) {
    x = eg2;
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
    area += xxgd.spec[0][i] * xxgd.ewid_sp[i];
    calc += xxgd.spec[2][i] * xxgd.ewid_sp[i];
    cent += xxgd.spec[0][i] * (float) (i - ilo) * xxgd.ewid_sp[i];
  }
  if (area == 0.0f) {
    eg = 0.0f;
    deg = 0.0f;
  } else {
    cent = cent / area + (float) ilo;
    for (i = ilo; i <= ihi; ++i) {
      cts = xxgd.spec[5][i] * xxgd.ewid_sp[i] * xxgd.ewid_sp[i];
      sum += cts;
      r1 = ((float) i - cent) / area;
      dc += cts * (r1 * r1);
    }
    dc = sqrt(dc);
    energy_nonlin(cent, dc, &eg, &deg);
  }
  /* write results */
  tell(" Chs %i to %i   Area: %.0f +- %.0f, Calc: %.0f\n"
       "     Energy: %.3f +- %.3f\n",
       ilo, ihi, area, sqrt(sum), calc, eg, deg);
  return 0;
} /* sum_eng */

/* ======================================================================= */
int trip_gate(char *ans, int nc)
{
  float r1, x, listx[40], listy[40], wfact1, wfact2, dx;
  float ms_err, ehi1, ehi2, elo1, elo2;
  int   nsum, numx, numy, i, j, ix, iy, iz, jnc, jjj;
  char  save_name[80], *c1, *c2;

  /* calculate gate channels ilo, ihi from specified Egamma
     and width_factor * parameterized_FWHM */
  strcpy(save_name, "");
  if (!(c1 = strchr(ans, '/'))) {
    if (!(nc = ask(ans, 80, "X-gate energy or list = ?")) ||
	get_list_of_gates(ans, nc, &numx, listx, &wfact1)) return 1;
    strcpy(save_name, ans);
    if (!(nc = ask(ans, 80, "Y-gate energy or list = ?")) ||
	get_list_of_gates(ans, nc, &numy, listy, &wfact2)) return 1;
    strcpy(save_name + 9, ans);
  } else {
    *c1 = '\0';
    c2 = ans+1;
    while (*c2 == ' ') c2++;
    jnc = strlen(c2);
    if (get_list_of_gates(c2, jnc, &numx, listx, &wfact1)) return 1;
    strcpy(save_name, c2);

    c1++;
    jnc = strlen(c1);
    if (get_list_of_gates(c1, jnc, &numy, listy, &wfact2)) return 1;
    strcpy(save_name + 9, c1);
  }
  if (numx == 0 || numy == 0) return 1;
  save_esclev_now(4);
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  sprintf(xxgd.name_gat, "T %.9s/%.9s", save_name, save_name + 9);
  for (j = 0; j < 6; ++j) {
    for (ix = 0; ix < xxgd.numchs; ++ix) {
      xxgd.old_spec[j][ix] = xxgd.spec[j][ix];
      xxgd.spec[j][ix] = 0.0f;
    }
  }
  xxgd.bf1 = xxgd.bf2 = xxgd.bf4 = xxgd.bf5 = 0.0f;
  for (i = 0; i < glsgd.ngammas; ++i) {
    elgd.hpk[1][i] = elgd.hpk[0][i];
    elgd.hpk[0][i] = 0.0f;
  }
  nsum = 0;
  jjj = -99;
  if (numx > 1 && numy > 1 &&
      !strncmp(save_name, save_name + 9, 9))
    jjj = (1 - askyn(" ...Include diagonal gates? (Y/N)"));
  set_signal_alert(1, "Busy reading gates...");
  for (ix = 0; ix < numx; ++ix) {
    if (check_signal_alert()) break;
    x = listx[ix];
    dx = wfact1 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		       elgd.swpars[2] * x * x) / 2.0f;
    elo1 = x - dx;
    ehi1 = x + dx;
    for (iy = (jjj < 0 ? 0 : ix + jjj); iy < numy; ++iy) {
      if (check_signal_alert()) break;
      x = listy[iy];
      dx = wfact2 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
			 elgd.swpars[2] * x * x) / 2.0f;
      elo2 = x - dx;
      ehi2 = x + dx;
      ++nsum;
      add_trip_gate(elo1, ehi1, elo2, ehi2);
    }
  }
  if (check_signal_alert()) tell("**** Gating interrupted.\n");
  set_signal_alert(0, "");
  if (nsum > 1) tell("Sum of %d double-gates...\n", nsum);
  /* subtract remaining background */
  for (iz = 0; iz < xxgd.numchs; ++iz) {
    xxgd.spec[0][iz] = xxgd.spec[0][iz] - xxgd.bf1 * xxgd.bspec[0][iz] -
      xxgd.bf2 * xxgd.bspec[1][iz] - xxgd.bf4 * xxgd.bspec[3][iz] -
      xxgd.bf5 * xxgd.bspec[4][iz];
  }
  /* correct for width of channel in keV */
  for (i = 0; i < xxgd.numchs; ++i) {
    for (j = 0; j < 4; ++j) {
      xxgd.spec[j][i] /= xxgd.ewid_sp[i];
    }
    if (!xxgd.many_cubes) xxgd.spec[4][i] = xxgd.spec[3][i] / xxgd.ewid_sp[i];
    r1 = elgd.bg_err * (xxgd.spec[3][i] - xxgd.spec[0][i]);
    xxgd.spec[5][i] = xxgd.spec[4][i] + r1 * r1;
    if (xxgd.spec[5][i] < 1.0f) xxgd.spec[5][i] = 1.0f;
  }
  /* calculate mean square error between calc. and exp. spectra */
  ms_err = 0.0f;
  for (ix = 0; ix < xxgd.numchs; ++ix) {
    r1 = xxgd.spec[0][ix] - xxgd.spec[2][ix];
    ms_err += r1 * r1 / xxgd.spec[5][ix];
  }
  ms_err /= (float) xxgd.numchs;
  tell("Mean square error on calculation = %.2f\n", ms_err);
  return 0;
} /* trip_gate */

/* ======================================================================= */
int wspec2(float *spec, char *filnam, char sp_nam_mod, int expand)
{
  float spec2[16384], *sp;
  int   i, jj, jext, rl = 0, c1 = 1;
  char  namesp[8], buf[32];

  /* write spectrum to disk file */
  /* default file extension = .spe */
  jext = setext(filnam, ".spe", 80);
  filnam[jext-1] = sp_nam_mod;
  if (jext > 8) {
    strncpy(namesp, filnam, 8);
  } else {
    memset(namesp, ' ', 8);
    strncpy(namesp, filnam, jext);
  }
  filez[1] = open_new_file(filnam, 0);

  if (expand) {
    for (i = 0; i < xxgd.nclook; ++i) {
      jj = xxgd.looktab[i];
      if (jj != 0) {
	spec2[i] = spec[jj - 1];
      } else {
	spec2[i] = 0.0f;
      }
    }
    sp = spec2;
    jj = xxgd.nclook;
  } else {
    sp = spec;
    jj = xxgd.numchs;
  }

#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(namesp,8); W(&jj,4); W(&c1,4); W(&c1,4); W(&c1,4);
#undef W
  if (put_file_rec(filez[1], buf, rl) ||
      put_file_rec(filez[1], sp, 4*jj)) {
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
  int   i, expand;
  char  filnam[80];

  expand = askyn("Expand spectra to linear ADC channels? (Y/N)");
  strcpy(filnam, ans+2);
  nc -= 2;
  /* ask for output file name */
  if (nc < 1 &&
      (nc = ask(filnam, 70, "Output spectrum name = ?")) == 0) return 0;
  strcpy(filnam + nc, "e");
  if (askyn("Write observed spectrum? (Y/N)"))
    wspec2(xxgd.spec[0], filnam, 'e', expand);
  if (askyn("Write unsubtracted spectrum? (Y/N)"))
    wspec2(xxgd.spec[3], filnam, 'u', expand);
  if (askyn("Write calculated spectrum? (Y/N)"))
    wspec2(xxgd.spec[2], filnam, 'c', expand);
  if (askyn("Write difference spectrum? (Y/N)")) {
    for (i = 0; i < xxgd.numchs; ++i) {
      spec[i] = xxgd.spec[0][i] - xxgd.spec[2][i];
    }
    wspec2(spec, filnam, 'd', expand);
  }
  if (askyn("Write residual spectrum? (Y/N)")) {
    for (i = 0; i < xxgd.numchs; ++i) {
      spec[i] = (xxgd.spec[0][i] - xxgd.spec[2][i]) / sqrt(xxgd.spec[5][i]);
    }
    wspec2(spec, filnam, 'r', expand);
  }
  return 0;
} /* write_sp */

/* ======================================================================= */
int ichanno(float egamma)
{
  int ret_val, nc;

  if (xxgd.numchs < 2) return 0;

  if (egamma < xxgd.ehi_sp[0]) {
    ret_val = 0;
  } else if (egamma > xxgd.elo_sp[xxgd.numchs - 1]) {
    ret_val = xxgd.numchs - 1;
  } else {
    ret_val = 0;
    nc = (xxgd.numchs + 1) / 2;
    while (nc != 1 || egamma > xxgd.ehi_sp[ret_val - 1]) {
      ret_val += nc;
      if (ret_val > xxgd.numchs) ret_val = xxgd.numchs;
      if (ret_val < 1) ret_val = 1;
      nc = abs(nc);
      if (egamma <= xxgd.elo_sp[ret_val - 1]) {
	nc = -(nc + 1) / 2;
      } else {
	nc = (nc + 1) / 2;
      }
    }
    --ret_val;
  }
  return ret_val;
} /* ichanno */

/* ======================================================================= */
float effic(float eg)
{
  return xxgd.eff_sp[ichanno(eg)];
} /* effic */

/* ====================================================================== */

void set_gate_label_h_offset(int offset)
{
  gate_label_h_offset = offset;
} /* set_gate_label_h_offset */
