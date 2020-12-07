#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "minig.h"
#include "levit8r.h"

/* declared in esclev.c */
extern char listnam[55];

/* ======================================================================= */
int fdg8r_exec(char *ans, int nc)
{
  float h, fj1, fj2;
  char  dattim[20];

  /* this subroutine decodes and executes the commands */
  /* convert lower case to upper case characters */
  ans[0] = toupper(ans[0]);
  if ((*ans != 'Q' && *ans != 'T') || !strstr(ans, "/")) ans[1] = toupper(ans[1]);

  if (!strncmp(ans, "HE", 2)) {
    fdg8r_help();
  } else if (!strncmp(ans, "RN", 2)) {
    save_esclev_now(3);
    while ((nc = ask(ans, 80,
		     "New 2D height normalization factor = ? (rtn for %g)",  elgd.h_norm)) &&
	   ffin(ans, nc, &h, &fj1, &fj2));
    if (nc && h > 1.0e-9) elgd.h_norm = h;
    while ((nc = ask(ans, 80,
		     "New 3D height normalization factor = ? (rtn for %g)",  elgd.t_norm)) &&
	   ffin(ans, nc, &h, &fj1, &fj2));
    if (nc && h > 1.0e-9) elgd.t_norm = h;
    while ((nc = ask(ans, 80,
		     "New 4D height normalization factor = ? (rtn for %g)",  elgd.q_norm)) &&
	   ffin(ans, nc, &h, &fj1, &fj2));
    if (nc && h > 1.0e-9) elgd.q_norm = h;
    calc_peaks();
    read_write_l4d_file("WRITE");
    datetime(dattim);
    fprintf(filez[26], "%s:  2D Scaling factor = %g\n", dattim, elgd.h_norm);
    fprintf(filez[26], "%s:  3D Scaling factor = %g\n", dattim, elgd.t_norm);
    fprintf(filez[26], "%s:  4D Scaling factor = %g\n", dattim, elgd.q_norm);
    fflush(filez[26]);
  } else if (!strncmp(ans, "SQ", 2)) {
    check_4d_band(ans, nc);
  } else if (!strncmp(ans, "XQ", 2)) {
    examine_quad();
  } else if (*ans == 'Q') {
    hilite(-1);
    if (quad_gate(ans, nc)) {
      tell("Bad energy or command.\n");
      return 0;
    }
    disp_dat(0);
  } else {
    l_gls_exec(ans, nc);
  }
  return 0;
} /* fdg8r_exec */

/* ======================================================================= */
int fdg8r_help(void)
{

  warn("\n"
    " SQ g      Search Quadruples for band with spacing of gate-list g.\n"
    " XQ        eXamine Quadruples with mouse\n"
    " Q         set triple-gate(s) on Quadruples\n"
    "                 (program will prompt for gate(s)\n"
    " Q g1 [w1] / g2 [w2] / g3 [w3]\n"
    "           - set triple-gate on quadruples at energies\n"
    "                 \"g1\",\"g2\",\"g3\"\n"
    "                 [widths \"w1\"*FWHM,\"w2\"*FWHM,\"w3\"*FWHM]\n"
    "           - list letters can be used instead of numbers\n"
    "                 to sum multiple gates.\n"
    " +Q...     add triple-gate(s) to current result\n"
    " -Q...     subtract triple-gate(s) from current result\n"
    " .Q...     \"and\" triple-gate(s) with current result\n\n");
  gls_help();
  lvt8r_help();
  return 0;
} /* fdg8r_help */

/* ======================================================================= */
int fdg8r_init(int argc, char **argv)
{
  float d1, d2;
  int   jext, i, j, i1, i2, nc, nx, ny, matchs, numchs, jnumchs, rl;
  char  buf[256], filnam[80], *infile1 = "", *infile2 = "", *c;

  /* initialise */
  /* ....open output files */
  /* ....open .4dg file */
  /* ....get energy and efficiency calibrations etc */

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
  elgd.min_ny = 20.0f;
  elgd.n_stored_sp = 0;
  elgd.isigma = 2;
  elgd.ipercent = 5;
  elgd.swpars[0] = 2.25f;
  elgd.swpars[1] = 0.001f;
  elgd.swpars[2] = 0.0f;
  for (j=0; j<15; j++) elgd.colormap[j] = j+1;
  elgd.t_norm = 100.0f;
  elgd.q_norm = 1.0f;
  xxgd.numchs = 1;
  xxgd.many_cubes = 0;
  xxgd.cubefact[0] = xxgd.cubefact[1] = xxgd.cubefact[2] = xxgd.cubefact[3] = 0.0;
  setcubenum(0);

  set_xwg("4dg8r___");
  strncpy(xxgd.progname, "4dg8r___", 8);
  initg2(&nx, &ny);
  finig();
  tell("\n\n"
       "  4DG8R Version 1.1    D.C. Radford    Sept. 1999\n\n"
       "     Welcome....\n\n");

  /* parse input parameters */
  prfile = 0;
  *prfilnam = 0;
  if (argc > 1) {
    infile1 = argv[1];
    if (!strcmp(infile1, "-l")) {
      infile1 = "";
      strcpy(prfilnam, "4dg8r");
      if (argc > 2) strcpy(prfilnam, argv[2]);
      if (argc > 3) infile1 = argv[3];
      if (argc > 4) infile2 = argv[4];
    } else if (argc > 2) {
      infile2 = argv[2];
      if (!strcmp(infile2, "-l")) {
	infile2 = "";
	strcpy(prfilnam, "4dg8r");
	if (argc > 3) strcpy(prfilnam, argv[3]);
	if (argc > 4) infile2 = argv[4];
      } else if (argc > 3) {
	strcpy(prfilnam, "4dg8r");
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
  /* ask for and open .4dg data file */
  xxgd.v_width = -1.0f;
  for (j = 0; j < MAXCHS; ++j) {
    xxgd.v_depth[j] = 0.0f;
  }

  if ((nc = strlen(infile2)) > 0) {
    strcpy(filnam, infile2);
  } else {
  START:
    nc = askfn(filnam, 80, "", ".4dg",
	       "4dg8r data file = ? (rtn to create new file)");
  }
  if (nc == 0) {
    prep4dg();
  } else {
    jext = setext(filnam, ".4dg", 80);
    if (!(filez[22] = fopen(filnam, "r+"))) {
      file_error("open", filnam);
      goto START;
    }
    strcpy(xxgd.fdgfile, filnam);
    /* read file */
    if (get_file_rec(filez[22], buf, 256, 0) <= 0) goto RERR;
    rl = 0;
#define R(a,b) { memcpy(a, buf + rl, b); rl += b; }
    R(&numchs, 4);
    R(&matchs, 4);
    R(xxgd.fdname, 80);
    R(xxgd.cubnam, 80);
#undef R
    if (numchs > MAXCHS) {
      warn1("ERROR: Number of channels in cube file %s is > MAXCHS (%d)\n", filnam, MAXCHS);
      fclose(filez[1]);
      goto START;
    }
    read_write_l4d_file("READ");
    strcpy(filnam, xxgd.fdname);
    if (!inq_file(filnam, &j)) {
      /* .4d file does not exist? - try pre-pending path to .4dg file */
      strcpy(filnam, xxgd.fdgfile);
      if ((c = rindex(filnam, '/'))) {
	*(c+1) = '\0';
      } else {
	filnam[0] = '\0';
      }
      strcat(filnam, xxgd.fdname);
    }
    if (open4dfile(filnam, &jnumchs)) {
      fclose(filez[22]);
      goto START;
    }
    if (jnumchs != numchs) {
      warn1("ERROR: 4Cube has wrong number of channels!\n");
      fclose(filez[22]);
      close4dfile();
      goto START;
    }
    strcpy(filnam, xxgd.cubnam);
    if (!inq_file(filnam, &j)) {
      /* cube file does not exist? - try pre-pending path to .lev file */
      strcpy(filnam, xxgd.levfile);
      if ((c = rindex(filnam, '/'))) {
	*(c+1) = '\0';
      } else {
	filnam[0] = '\0';
      }
      strcat(filnam, xxgd.cubnam);
    }
    if (open3dfile(filnam, &jnumchs)) {
      fclose(filez[22]);
      close4dfile();
      goto START;
    }
    if (jnumchs != numchs) {
      warn1("ERROR: Cube file has wrong number of channels\n!");
      fclose(filez[22]);
      close4dfile();
      close3dfile();
      goto START;
    }
    /* open .4dg_log file for append */
    strcpy(filnam, xxgd.fdgfile);
    strcpy(filnam + jext + 4, "_log");
    if (!(filez[26] = fopen(filnam, "a+"))) {
      file_error("open", filnam);
      exit(1);
    }
    /* read depth and width of Egamma = Egamma valley from .val file */
    strcpy(filnam + jext, ".val");
    if ((filez[1] = fopen(filnam, "r"))) {
      tell("Valley width and depth read from file %s\n", filnam);
      if (fgets(lx, 120, filez[1]) &&
	  sscanf(lx, "%f", &xxgd.v_width) == 1) {
	xxgd.v_width /= 2.0f;
	i1 = 0;
	d1 = 0.0f;
	for (i = 0; i < 1024; ++i) {
	  if (!fgets(lx, 120, filez[1]) ||
	      sscanf(lx, "%i%f", &i2, &d2) != 2) break;
	  if (i2 <= i1 || i2 >= MAXCHS) {
	    warn("Starting channels out of order\n"
		   " or bad channel number, file: %s\n", filnam);
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
  return 0;
 RERR:
  warn1("*** ERROR -- cannot read file %s\n"
	"    -- Perhaps you need to run unix2unix,\n"
	"       vms2unix or unix2vms on it.\n", filnam);
  fclose(filez[1]);
  goto START;
} /* fdg8r_init */

/* ======================================================================= */
int examine_quad(void)
{
  static float x1, y1, egamma;
  int   igam, iwin, nc;
  char  ans[80], command[80], tmp[80];
#define NGX elgd.nlist[52] /* # of gammas set on x */
#define NGY elgd.nlist[53] /* # of gammas set on y */
#define NGZ elgd.nlist[54] /* # of gammas set on z */

  NGX = 0;
  NGY = 0;
  NGZ = 0;
  tell("Button 1: set or add X-gate\n"
	 "Button 2: set or add Y-gate\n"
	 "Button 3: set or add Z-gate\n"
	 "  e or s: show gate\n"
	 "    b/f : back/forward in gate history\n"
	 " ...hit X on keyboard to exit...\n");
#define EERR { tell("Bad energy...\n"); goto START; }
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
    tell("\n *** Egamma = %8.2f +- %4.2f    Igamma = %8.2f +- %4.2f\n",
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
  if (*ans == 's' || *ans == 'S' || *ans == 'e' || *ans == 'E') {
    sprintf(command, "X%s", tmp);
    tell("%s\n", command);
    if (examine(command, nc + 1)) EERR;
  } else if (ans[1] == '1') {
    if (NGX == 0 || NGY != 0) {
      strcpy(command, tmp);
      tell("%s\n", command);
      hilite(-1);
      if (num_gate(command, nc)) EERR;
      NGX = 1;
      NGY = 0;
      NGZ = 0;
    } else {
      sprintf(command, "+%s", tmp);
      tell("%s\n", command);
      if (combine(command, nc + 1)) EERR;
      ++NGX;
    }
    elgd.list[52][NGX - 1] = egamma;
    disp_dat(0);
  } else if (ans[1] == '2') {
    if (NGX == 0) {
      tell("First set X-gate...\n");
    } else {
      if (NGY == 0) {
	sprintf(command, "T [/%s", tmp);
	tell("%s\n", command);
	if (trip_gate(command, nc + 4)) EERR;
      } else {
	sprintf(command, "+T [/%s", tmp);
	tell("%s\n", command);
	if (combine(command, nc + 5)) EERR;
      }
      elgd.list[53][NGY++] = egamma;
      disp_dat(0);
    }
  } else if (ans[1] == '3') {
    if (NGX == 0) {
      tell("First set X- and Y-gate...\n");
    } else if (NGY == 0) {
      tell("First set Y-gate...\n");
    } else if (NGZ == 0) {
      sprintf(command, "Q [/]/%s", tmp);
      tell("%s\n", command);
      if (quad_gate(command, nc + 6)) EERR;
      disp_dat(0);
      ++NGZ;
     } else {
      sprintf(command, "+Q [/]/%s", tmp);
      tell("%s\n", command);
      if (combine(command, nc + 7)) EERR;
      disp_dat(0);
      ++NGZ;
    }
  }
  goto START;
#undef EERR
#undef NGX
#undef NGY
#undef NGZ
} /* examine_quad */

/* ======================================================================= */
int add_quad_gate(float elo1, float ehi1, float elo2, float ehi2,
		  float elo3, float ehi3)
{
  float save, sp2dx[MAXCHS], sp2dy[MAXCHS], sp2dz[MAXCHS], sum1, sum2, sum3;
  int   i, j, gdata[MAXCHS], ichxy, ichxz, ichyz;
  int   ix, iz, iy, ihi1, ihi2, ihi3, ilo1, ilo2, ilo3;

  /* read gate of energy elo to ehi and calculate expected spectrum */
  /* spec[0][] = background-subtracted gate */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  /* spec[3][] = non-background-subtracted gate */
  /* spec[4][] = square of statistical uncertainty */
  /* spec[5][] = square of statistical plus systematic uncertainties */
  save_esclev_now(4);

  for (i = 0; i < xxgd.numchs; ++i) {
    sp2dx[i] = 0.0f;
    sp2dy[i] = 0.0f;
    sp2dz[i] = 0.0f;
  }
  if (elo1 > ehi1) {
    save = elo1;
    elo1 = ehi1;
    ehi1 = save;
  }
  ilo1 = ichanno(elo1);
  ihi1 = ichanno(ehi1);
  sum1 = 0.0f;
  for (i = ilo1; i <= ihi1; ++i) {
    sum1 += xxgd.bspec[1][i];
    for (j = 0; j < i; ++j) {
      sp2dx[j] += xxgd.pro2d[xxgd.luch[j] + i];
    }
    for (j = i; j < xxgd.numchs; ++j) {
      sp2dx[j] += xxgd.pro2d[xxgd.luch[i] + j];
    }
  }
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].e >= xxgd.elo_sp[ilo1] &&
	glsgd.gam[j].e <= xxgd.ehi_sp[ihi1]) hilite(j);
  }
  if (elo2 > ehi2) {
    save = elo2;
    elo2 = ehi2;
    ehi2 = save;
  }
  ilo2 = ichanno(elo2);
  ihi2 = ichanno(ehi2);
  sum2 = 0.0f;
  for (i = ilo2; i <= ihi2; ++i) {
    sum2 += xxgd.bspec[1][i];
    for (j = 0; j < i; ++j) {
      sp2dy[j] += xxgd.pro2d[xxgd.luch[j] + i];
    }
    for (j = i; j < xxgd.numchs; ++j) {
      sp2dy[j] += xxgd.pro2d[xxgd.luch[i] + j];
    }
  }
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].e >= xxgd.elo_sp[ilo2] &&
	glsgd.gam[j].e <= xxgd.ehi_sp[ihi2]) {
      hilite(j);
    }
  }
  if (elo3 > ehi3) {
    save = elo3;
    elo3 = ehi3;
    ehi3 = save;
  }
  ilo3 = ichanno(elo3);
  ihi3 = ichanno(ehi3);
  sum3 = 0.0f;
  for (i = ilo3; i <= ihi3; ++i) {
    sum3 += xxgd.bspec[1][i];
    for (j = 0; j < i; ++j) {
      sp2dz[j] += xxgd.pro2d[xxgd.luch[j] + i];
    }
    for (j = i; j < xxgd.numchs; ++j) {
      sp2dz[j] += xxgd.pro2d[xxgd.luch[i] + j];
    }
  }
  for (j = 0; j < glsgd.ngammas; ++j) {
    if (glsgd.gam[j].e >= xxgd.elo_sp[ilo3] &&
	glsgd.gam[j].e <= xxgd.ehi_sp[ihi3]) {
      hilite(j);
    }
  }
  read4dcube(ilo1, ihi1, ilo2, ihi2, ilo3, ihi3, gdata);
  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.spec[0][i] += (float) gdata[i];
    xxgd.spec[3][i] += (float) gdata[i];
  }
  xxgd.bf1 += sum1 * sum2 * sum3;
  for (iz = ilo3; iz <= ihi3; ++iz) {
    for (iy = ilo2; iy <= ihi2; ++iy) {
      if (iy < iz) {
	ichyz = xxgd.luch[iy] + iz;
      } else {
	ichyz = xxgd.luch[iz] + iy;
      }
      xxgd.bf2 -= xxgd.pro2d[ichyz] * sum1;
    }
    xxgd.bf2 += xxgd.bspec[2][iz] * sum1 * sum2;
  }
  for (ix = ilo1; ix <= ihi1; ++ix) {
    for (iz = ilo3; iz <= ihi3; ++iz) {
      if (ix <= iz) {
	ichxz = xxgd.luch[ix] + iz;
      } else {
	ichxz = xxgd.luch[iz] + ix;
      }
      xxgd.bf2 -= xxgd.pro2d[ichxz] * sum2;
    }
    xxgd.bf2 += xxgd.bspec[0][ix] * sum2 * sum3;
  }
  for (iy = ilo2; iy <= ihi2; ++iy) {
    for (ix = ilo1; ix <= ihi1; ++ix) {
      if (ix <= iy) {
	ichxy = xxgd.luch[ix] + iy;
      } else {
	ichxy = xxgd.luch[iy] + ix;
      }
      xxgd.bf2 -= xxgd.pro2d[ichxy] * sum3;
    }
    xxgd.bf2 += xxgd.bspec[0][iy] * sum1 * sum3;
  }
  /* subtract part of the background */
  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.spec[0][i] += (sp2dx[i] * sum2 + sp2dy[i] * sum1) * sum3 + sp2dz[i] * 
      sum1 * sum2;
  }
  /* calculate expected spectrum */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  calc_4d_spec(ilo1, ihi1, ilo2, ihi2, ilo3, ihi3,
	       xxgd.spec[1], xxgd.spec[2]);
  return 0;
} /* add_quad_gate */

/* ======================================================================= */
int calc_4d_spec(int ilo1, int ihi1, int ilo2, int ihi2, int ilo3, int ihi3,
		 float *spec1, float *spec2)
{
  float a, br, norm;
  int   ix, iz, iy, jx, jy, jz, jw, iiw, jjw, jjx, jjy, jjz;
  int   i, i1, i2, i3, k1 = 0, k2 = 0, k3 = 0;
  int   lx[MAXGAM], ly[MAXGAM], lz[MAXGAM], nx = 0, ny = 0, nz = 0;

  /* calculate expected spectrum */
  /* spec[1][] = calculated spectrum from gammas outside gate */
  /* spec[2][] = calculated spectrum from all gammas */
  for (i=0; i<glsgd.ngammas; ++i) {
    if (xxgd.lo_ch[i] <= ihi1 && xxgd.hi_ch[i] >= ilo1) lx[nx++] = i;
    if (xxgd.lo_ch[i] <= ihi2 && xxgd.hi_ch[i] >= ilo2) ly[ny++] = i;
    if (xxgd.lo_ch[i] <= ihi3 && xxgd.hi_ch[i] >= ilo3) lz[nz++] = i;
  }
  norm = sqrt(elgd.h_norm) * elgd.h_norm /
    (elgd.t_norm * elgd.t_norm * elgd.q_norm);

  for (i1 = 0; i1 < nx; ++i1) {
    jx = lx[i1];
    for (i2 = 0; i2 < ny; ++i2) {
      jy = ly[i2];
      if (elgd.levelbr[jy][glsgd.gam[jx].lf] == 0.0f && 
	  elgd.levelbr[jx][glsgd.gam[jy].lf] == 0.0f) continue;
      for (i3 = 0; i3 < nz; ++i3) {
	jz = lz[i3];
	if ((elgd.levelbr[jz][glsgd.gam[jx].lf] == 0.0f && 
	     elgd.levelbr[jx][glsgd.gam[jz].lf] == 0.0f) ||
	    (elgd.levelbr[jz][glsgd.gam[jy].lf] == 0.0f && 
	     elgd.levelbr[jy][glsgd.gam[jz].lf] == 0.0f)) continue;
	if (elgd.levelbr[jy][glsgd.gam[jx].lf] != 0.0f && 
	    elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f) {
	  k1 = jx;
	  if (elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f) {
	    k2 = jy;
	    k3 = jz;
	  } else if (elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	    k2 = jz;
	    k3 = jy;
	  } else {
	    continue;
	  }
	} else if (elgd.levelbr[jx][glsgd.gam[jy].lf] != 0.0f && 
		   elgd.levelbr[jz][glsgd.gam[jy].lf] != 0.0f) {
	  k1 = jy;
	  if (elgd.levelbr[jz][glsgd.gam[jx].lf] != 0.0f) {
	    k2 = jx;
	    k3 = jz;
	  } else if (elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f) {
	    k2 = jz;
	    k3 = jx;
	  } else {
	    continue;
	  }
	} else if (elgd.levelbr[jx][glsgd.gam[jz].lf] != 0.0f && 
		   elgd.levelbr[jy][glsgd.gam[jz].lf] != 0.0f) {
	  k1 = jz;
	  if (elgd.levelbr[jy][glsgd.gam[jx].lf] != 0.0f) {
	    k2 = jx;
	    k3 = jy;
	  } else if (elgd.levelbr[jx][glsgd.gam[jy].lf] != 0.0f) {
	    k2 = jy;
	    k3 = jx;
	  } else {
	    continue;
	  }
	}
	for (jw = 0; jw < glsgd.ngammas; ++jw) {
	  if ((elgd.levelbr[jw][glsgd.gam[jx].lf] == 0.0f && 
	       elgd.levelbr[jx][glsgd.gam[jw].lf] == 0.0f) ||
	      (elgd.levelbr[jw][glsgd.gam[jy].lf] == 0.0f && 
	       elgd.levelbr[jy][glsgd.gam[jw].lf] == 0.0f) ||
	      (elgd.levelbr[jw][glsgd.gam[jz].lf] == 0.0f && 
	       elgd.levelbr[jz][glsgd.gam[jw].lf] == 0.0f)) continue;
	  br = 0.0f;
	  if (elgd.levelbr[k1][glsgd.gam[jw].lf] != 0.0f) {
	    br = glsgd.gam[jw].i *
	      elgd.levelbr[k1][glsgd.gam[jw].lf] *
	      elgd.levelbr[k2][glsgd.gam[k1].lf] *
	      elgd.levelbr[k3][glsgd.gam[k2].lf];
	  } else if (elgd.levelbr[jw][glsgd.gam[k1].lf] != 0.0f && 
		     elgd.levelbr[k2][glsgd.gam[jw].lf] != 0.0f) {
	    br = glsgd.gam[k1].i *
	      elgd.levelbr[jw][glsgd.gam[k1].lf] *
	      elgd.levelbr[k2][glsgd.gam[jw].lf] * 
	      elgd.levelbr[k3][glsgd.gam[k2].lf];
	  } else if (elgd.levelbr[jw][glsgd.gam[k2].lf] != 0.0f && 
		     elgd.levelbr[k3][glsgd.gam[jw].lf] != 0.0f) {
	    br = glsgd.gam[k1].i *
	      elgd.levelbr[k2][glsgd.gam[k1].lf] *
	      elgd.levelbr[jw][glsgd.gam[k2].lf] *
	      elgd.levelbr[k3][glsgd.gam[jw].lf];
	  } else if (elgd.levelbr[jw][glsgd.gam[k3].lf] != 0.0f) {
	    br = glsgd.gam[k1].i *
	      elgd.levelbr[k2][glsgd.gam[k1].lf] *
	      elgd.levelbr[k3][glsgd.gam[k2].lf] *
	      elgd.levelbr[jw][glsgd.gam[k3].lf];
	  } else {
	    continue;
	  }

	  for (ix = (ilo1 > xxgd.lo_ch[jx] ? ilo1 : xxgd.lo_ch[jx]);
	       ix <= ihi1 && ix <= xxgd.hi_ch[jx]; ++ix) {
	    jjx = ix - xxgd.lo_ch[jx];
	    for (iy = (ilo2 > xxgd.lo_ch[jy] ? ilo2 : xxgd.lo_ch[jy]);
		 iy <= ihi2 && iy <= xxgd.hi_ch[jy]; ++iy) {
	      jjy = iy - xxgd.lo_ch[jy];
	      for (iz = (ilo3 > xxgd.lo_ch[jz] ? ilo3 : xxgd.lo_ch[jz]);
		   iz <= ihi3 && iz <= xxgd.hi_ch[jz]; ++iz) {
		jjz = iz - xxgd.lo_ch[jz];
		a = br * xxgd.pk_shape[jx][jjx] * xxgd.pk_shape[jy][jjy] *
		         xxgd.pk_shape[jz][jjz] * norm;
		elgd.hpk[0][jw] += a;
		for (iiw = xxgd.lo_ch[jw]; iiw <= xxgd.hi_ch[jw]; ++iiw) {
		  jjw = iiw - xxgd.lo_ch[jw];
		  spec2[iiw] += a * xxgd.pk_shape[jw][jjw];
		  if (glsgd.gam[jx].e < xxgd.elo_sp[ilo1] ||
		      glsgd.gam[jx].e > xxgd.ehi_sp[ihi1] ||
		      glsgd.gam[jy].e < xxgd.elo_sp[ilo2] ||
		      glsgd.gam[jy].e > xxgd.ehi_sp[ihi2] ||
		      glsgd.gam[jz].e < xxgd.elo_sp[ilo3] ||
		      glsgd.gam[jz].e > xxgd.ehi_sp[ihi3])
		    spec1[iiw] += a * xxgd.pk_shape[jw][jjw];
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return 0;
}

/* ======================================================================= */
int check_4d_band(char *ans, int nc)
{
  /* defined in esclev.h:
     char listnam[55] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]|" */
  static char fmt[] =
    "List %c E1 =%7.1f %5i 4-gates,"
    " Mean cnts = %5.1f +- %5.1f %5.1f, Merit = %4.2f\n";
  float r1, area, bkgnd, err, finc, cnts[1365], x, dcnts[1365], dx;
  float fj1, fj2, sum1, sum2, sum3, spec1[MAXCHS], spec2[MAXCHS];
  float b1sum[6][15], b2sum[15][15], b3sum[15][15][15];
  int   numl = 0, i, j, i3gate, nl, iw, ix, iy, iz, nsteps;
  int   loc[4], hic[4], j1, j2, ich, ispec[MAXCHS];

  /* check gate list for presence of a rotational band */

  if (!prfile &&
      askyn("*** WARNING: The results of this search will not be logged since\n"
	    " you did not run the program with the -l command line flag.\n"
	    " ...Would you like to start logging now? (Y/N)") &&
      askfn(prfilnam, 80, "sdsearch.log", ".log", "Log file name = ?")) {
    prfile = open_new_file(prfilnam, 0);
  }

  if (nc <= 2) {
    nc = ask(ans, 80, "List = ?");
    if (nc == 0) return 0;
  } else {
    memmove(ans, ans + 2, strlen(ans));
    nc -= 2;
  }
  if (*ans == ' ') {
    memmove(ans, ans + 1, strlen(ans));
    nc--;
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
  if (numl <= 5) {
    warn1("Not enough gates in list.\n");
    return 0;
  }
  if (numl > 15) {
    numl = 15;
    warn1("Number of transitions limited to 15...\n"
	 "   Only the first 15 energies were taken from the list.\n");
  }

  nsteps = 0;
 DOSEARCH:

  i3gate = 0;
  /* first collect 1D, 2D and 3D background numbers */
  for (iz = 0; iz < numl; ++iz) {
    x = elgd.list[nl][iz];
    dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
	      elgd.swpars[2] * x * x) / 2.0f;
    loc[3] = ichanno(x - dx);
    hic[3] = ichanno(x + dx);
    for (j = 0; j < 2; ++j) {
      b1sum[j][iz] = 0.0f;
      for (i = loc[3]; i <= hic[3]; ++i) {
	b1sum[j][iz] += xxgd.bspec[j][i];
      }
    }
    if (iz < 1) continue;

    for (iy = 0; iy < iz; ++iy) {
      x = elgd.list[nl][iy];
      dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		elgd.swpars[2] * x * x) / 2.0f;
      loc[2] = ichanno(x - dx);
      hic[2] = ichanno(x + dx);
      b2sum[iy][iz] = 0.0f;
      for (j = loc[3]; j <= hic[3]; ++j) {
	for (i = loc[2]; i <= hic[2]; ++i) {
	  if (i < j) {
	    ich = xxgd.luch[i] + j;
	  } else {
	    ich = xxgd.luch[j] + i;
	  }
	  b2sum[iy][iz] += xxgd.pro2d[ich];
	}
      }
      if (iy < 1) continue;

      read3dcube(loc[2], hic[2], loc[3], hic[3], ispec);
      for (ix = 0; ix < iy; ++ix) {
	x = elgd.list[nl][ix];
	dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		  elgd.swpars[2] * x * x) / 2.0f;
	loc[1] = ichanno(x - dx);
	hic[1] = ichanno(x + dx);
	j = 0;
	for (i = loc[1]; i <= hic[1]; ++i) {
	  j += ispec[i];
	}
	b3sum[ix][iy][iz] = (float) j;
      }
    }
  }

  /* now collect 4D gate numbers */
  for (iz = 3; iz < numl; ++iz) {
    x = elgd.list[nl][iz];
    dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
	      elgd.swpars[2] * x * x) / 2.0f;
    loc[3] = ichanno(x - dx);
    hic[3] = ichanno(x + dx);
    for (iy = 2; iy < iz; ++iy) {
      x = elgd.list[nl][iy];
      dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		elgd.swpars[2] * x * x) / 2.0f;
      loc[2] = ichanno(x - dx);
      hic[2] = ichanno(x + dx);
      for (ix = 1; ix < iy; ++ix) {
	x = elgd.list[nl][ix];
	dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		  elgd.swpars[2] * x * x) / 2.0f;
	loc[1] = ichanno(x - dx);
	hic[1] = ichanno(x + dx);
	/* calculate expected spectrum */
	memset(spec1, 0, MAXCHS*4);
	memset(spec2, 0, MAXCHS*4);
	calc_4d_spec(loc[1], hic[1], loc[2], hic[2], loc[3], hic[3],
		     spec1, spec2);
	for (iw = 0; iw < ix; ++iw) {
	  x = elgd.list[nl][iw];
	  dx = sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		    elgd.swpars[2] * x * x) / 2.0f;
	  loc[0] = ichanno(x - dx);
	  hic[0] = ichanno(x + dx);
	  area = (float) sum4gate(loc, hic);

	  /* subtract background */
	  bkgnd =
	    (b1sum[1][iw] * b1sum[1][ix] * b1sum[1][iy] * b1sum[2][iz] +
	     b1sum[1][iw] * b1sum[1][ix] * b1sum[0][iy] * b1sum[1][iz] +
	     b1sum[1][iw] * b1sum[0][ix] * b1sum[1][iy] * b1sum[1][iz] +
	     b1sum[0][iw] * b1sum[1][ix] * b1sum[1][iy] * b1sum[1][iz]) -
	    (b2sum[iw][ix] * b1sum[1][iy] * b1sum[1][iz] +
	     b2sum[iw][iy] * b1sum[1][ix] * b1sum[1][iz] +
	     b2sum[iw][iz] * b1sum[1][ix] * b1sum[1][iy] +
	     b2sum[ix][iy] * b1sum[1][iw] * b1sum[1][iz] +
	     b2sum[ix][iz] * b1sum[1][iw] * b1sum[1][iy] +
	     b2sum[iy][iz] * b1sum[1][iw] * b1sum[1][ix]) +
	    (b3sum[ix][iy][iz] * b1sum[1][iw] +
	     b3sum[iw][iy][iz] * b1sum[1][ix] +
	     b3sum[iw][ix][iz] * b1sum[1][iy] +
	     b3sum[iw][ix][iy] * b1sum[1][iz]);
	  /* --- */
	  r1 = elgd.bg_err * bkgnd;
	  err = area + r1 * r1;
	  if (err < 1.0f) err = 1.0f;
	  for (i = loc[0]; i <= hic[0]; ++i) {
	    area -= spec2[i];
	  }
	  cnts[i3gate] = area - bkgnd;
	  dcnts[i3gate++] = sqrt(err);
	}
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
      while ((nc = ask(ans, 80, "Number of steps = ? (rtn for 1)")) &&
	     inin(ans, nc, &nsteps, &j1, &j2));
      if (!nc) nsteps = 1;
    }
  }
  if (--nsteps < 0) return 0;
  for (ix = 0; ix < numl; ++ix) {
    elgd.list[nl][ix] += finc;
  }
  goto DOSEARCH;
} /* check_4d_band */

/* ======================================================================= */
int combine(char *ans, int nc)
{
  float r1;
  int   i, j, ix;
  char  save_ans[80], newname[80], *c;

  /* add / subtract / take minimum of  two gates */
  strcpy(save_ans, ans);
  strcpy(ans, save_ans + 1);
  ans[39] = ' ';
  if (!strncmp(ans, "G ", 2) || !strcmp(ans, "G")) {
    if (cur_gate()) return 1;
  } else if (!strncmp(ans, "GL", 2) || !strncmp(ans, "Gl", 2)) {
    if (gate_sum_list(ans)) return 1;
  } else if (*ans == 'T') {
    if (trip_gate(ans, nc-1)) return 1;
  } else if (*ans == 'Q') {
    if (quad_gate(ans, nc-1)) return 1;
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
    for (j = 0; j < 5; ++j) {
      for (ix = 0; ix < xxgd.numchs; ++ix) {
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
	xxgd.spec[j][ix] = (xxgd.old_spec[j][ix] < xxgd.spec[j][ix] ? 
			    xxgd.old_spec[j][ix] : xxgd.spec[j][ix]);
      }
      xxgd.spec[4][ix] = (xxgd.old_spec[4][ix] > xxgd.spec[4][ix] ?
			  xxgd.old_spec[4][ix] : xxgd.spec[4][ix]);
    }
    for (i = 0; i < glsgd.ngammas; ++i) {
      elgd.hpk[0][i] = (elgd.hpk[0][i] < elgd.hpk[1][i] ?
			elgd.hpk[0][i] : elgd.hpk[1][i]);
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
int prep4dg(void)
{
  float fmax, fwhm, test, x, effspec[MAXCHS];
  int   jext, i, j, iymax, ii, ix, iy, reclen, ich, jnumchs;
  char  fn[80], filnam[80], dattim[20], ans[80];

  warn1(" HyperCube data file preparation routine...\n\n"
	" You will need:\n"
	" 4DCube data file from 4play;\n"
	" The look-up table (.tab) file used by 4play;\n"
	" 3D projection from pro4d;\n"
	" 1D and 2D projections from pro3d;\n"
	" 1D background corresponding to the 1D projection\n"
	"    (created with the BG command in gf3);\n"
	" A gate (.win) file with gates set on bkgnd chs in the E2-bump region;\n"
	" The summed X-gate spectrum from this gate file (with no bkgnd sub.);\n"
	" A background spectrum for this E2-gated sum spectrum;\n"
	" Efficiency (.aef/.eff) and energy (.aca/.cal) calibration files;\n"
	" and parameters to define the FWHM, and R and BETA peak shapes,\n"
	"    as a function of ch. no. in a 4k-ch. spectrum\n"
	"    (see gf3 for details).\n\n");

  /* get 4d hypercube data file */
  while (1) {
    askfn(xxgd.fdname, 80, "", ".4d", "4D cube directory file name = ?");
    jext = setext(xxgd.cubnam, ".4d", 80);
    if (open4dfile(xxgd.fdname, &xxgd.numchs)) continue;
    if (xxgd.numchs > 1 && xxgd.numchs <= MAXCHS) break;
    warn1(" ERROR: 4Cube file has too many channels!\n"
	  "...Number of channels in 4cube is %d\n"
	  "       ...maximum is MAXCHS = %d\n", xxgd.numchs, MAXCHS);
    close4dfile();
  }
  tell("%d channels...\n", xxgd.numchs);
  /* get 3d projection data file */
  while (1) {
    askfn(xxgd.cubnam, 80, "", ".cub", "3D projection file = ?");
    if (open3dfile(xxgd.cubnam, &jnumchs)) continue;
    if (jnumchs == xxgd.numchs) break;
    warn1("ERROR: 3D cube has wrong number of channels!\n");
    close3dfile();
  }
  /* get 2D projection file */
  while (1) {
    askfn(filnam, 80, "", ".2dp", "2D projection file = ?");
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
  xxgd.luch[0] = 0;
  for (iy = 0; iy < xxgd.numchs - 1; ++iy) {
    xxgd.luch[iy+1] = xxgd.luch[iy] + xxgd.numchs - iy+1;
  }
  xxgd.matchs = xxgd.luch[xxgd.numchs - 1] + xxgd.numchs;
  reclen *= 4;
  for (iy = 0; iy < xxgd.numchs; ++iy) {
    fseek(filez[1], iy*reclen, SEEK_SET);
    fread(xxgd.spec[0], reclen, 1, filez[1]);
    for (ix = 0; ix <= iy; ++ix) {
      ich = xxgd.luch[ix] + iy;
      xxgd.pro2d[ich] = xxgd.spec[0][ix];
    }
  }
  fclose(filez[1]);
  /* get name of new .4dg file */
  strcpy(fn, filnam);
  strcpy(filnam, xxgd.fdname);
  strcpy(filnam + jext, ".4dg");
  askfn(ans, 80, filnam, ".4dg",
        "\nName for new 4dg8r data file = ?\n");
  jext = setext(ans, ".4dg", 80);
  filez[22] = open_new_file(ans, 1);
  strcpy(xxgd.fdgfile, ans);
  /* open .4dg_log file for append */
  strcpy(ans + jext + 4, "_log");
  if (!(filez[26] = fopen(ans, "a+"))) {
    file_error("open", ans);
    exit(1);
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Created new 4dg8r data file %s\n",
	  dattim, xxgd.fdgfile);
  fprintf(filez[26], "      ...4D cube directory file = %s\n", xxgd.fdname);
  fprintf(filez[26], "      ...3d projection file = %s\n", xxgd.cubnam);
  fprintf(filez[26], "      ...2D projection file = %s\n", fn);
  fflush(filez[26]);
  /* get lookup table used to convert ADC channels to cube channels */
  get_lookup_table();
  /* get energy and efficiency calibrations */
  get_cal();
  /* get peak shape and FWHM parameters */
  get_shapes();
  /* get projections and background spectra */
  get_bkgnd();
  /* find largest element of new matrix */
  fmax = 1e-6f;
  iymax = 0;
  for (i = 0; i < xxgd.numchs; ++i) {
    x = xxgd.energy_sp[i];
    fwhm = sqrt(elgd.swpars[0] + elgd.swpars[1] * x + elgd.swpars[2] * x * x);
    effspec[i] = xxgd.eff_sp[i] / (fwhm * 1.06446705f / xxgd.ewid_sp[i]);
    test = xxgd.bspec[2][i] / effspec[i];
    if (test > fmax) {
      fmax = test;
      iymax = i;
    }
  }
  ii = iymax + 1;
  fmax = 1e-6f;
  for (j = 0; j < ii; ++j) {
    ich = xxgd.luch[j] + ii;
    test = (xxgd.pro2d[ich - 1] - xxgd.bspec[0][j] * xxgd.bspec[1][ii - 1] - 
      xxgd.bspec[2][ii - 1] * xxgd.bspec[1][j] - xxgd.bspec[3][j] * 
      xxgd.bspec[4][ii - 1] - xxgd.bspec[5][ii - 1] * xxgd.bspec[4][j]) / 
      (effspec[j] * effspec[ii - 1]);
    if (test > fmax) fmax = test;
  }
  for (j = ii + 1; j <= xxgd.numchs; ++j) {
    ich = xxgd.luch[ii - 1] + j;
    test = (xxgd.pro2d[ich - 1] -
	    xxgd.bspec[0][j - 1] * xxgd.bspec[1][ii - 1] - 
	    xxgd.bspec[2][ii - 1] * xxgd.bspec[1][j - 1] -
	    xxgd.bspec[3][j - 1] * xxgd.bspec[4][ii - 1] -
	    xxgd.bspec[5][ii - 1] * xxgd.bspec[4][j - 1]) / 
      (effspec[j - 1] * effspec[ii - 1]);
    if (test > fmax) fmax = test;
  }
  /* set up height normalization factor to give coinc. int.
     of 100 for most intense peak */
  elgd.h_norm = 100.0f / fmax;
  tell("\n 2D Scaling factor (H_NORM) = %10.3e\n\n", elgd.h_norm);
  datetime(dattim);
  fprintf(filez[26], "%s: 2D Scaling factor = %f\n", dattim, elgd.h_norm);
  fflush(filez[26]);
  /* write projections, calibrations etc. to file */
  tell("\nWriting .4dg file...\n");
  read_write_l4d_file("WRITE");
  return 0;
} /* prep4dg */

/* ======================================================================= */
int quad_gate(char *ans, int nc)
{
  float r1, sp3db[MAXCHS], x;
  float listx[40], listy[40], listz[40], wfact1, wfact2, wfact3;
  float sp3dxy[MAXCHS], sp3dxz[MAXCHS], sp3dyz[MAXCHS], dx;
  float ms_err, ehi1, ehi2, ehi3, elo1, elo2, elo3;
  float sum1, sum2, sum3;
  int   nsum, numx, numy, numz, i, j, ilo1, ilo2, ilo3;
  int   ix, iy, iz, jnc, jjj = 0, loy, loz, ihi1, ihi2, ihi3;
  char  save_name[80], *c1, *c2;

  /* calculate gate channels ilo, ihi from specified Egamma
     and width_factor * parameterized_FWHM */
  strcpy(save_name, " ");
  if (!(c1 = strchr(ans, '/'))) {
    if (!(nc = ask(ans, 80, "X-gate energy or list = ?")) ||
	get_list_of_gates(ans, nc, &numx, listx, &wfact1)) return 1;
    strcpy(save_name, ans);
    if (!(nc = ask(ans, 80, "Y-gate energy or list = ?")) ||
	get_list_of_gates(ans, nc, &numy, listy, &wfact2)) return 1;
    strcpy(save_name + 9, ans);
    if (!(nc = ask(ans, 80, "Z-gate energy or list = ?")) ||
	get_list_of_gates(ans, nc, &numz, listz, &wfact3)) return 1;
    strcpy(save_name + 19, ans);
  } else {
    *c1 = '\0';
    jnc = strlen(ans) - 1;
    if (get_list_of_gates(ans + 1, jnc, &numx, listx, &wfact1)) return 1;
    i = 1;
    strcpy(save_name, ans + i);
    while (*save_name == ' ') strcpy(save_name, ans + (++i));

    c1++;
    if (!(c2 = strchr(c1, '/'))) return 1;
    *c2 = '\0';
    jnc = strlen(c1);
    if (get_list_of_gates(c1, jnc, &numy, listy, &wfact2)) return 1;
    strcpy(save_name + 9, c1);
    c2++;
    jnc = strlen(c2);
    if (get_list_of_gates(c2, jnc, &numz, listz, &wfact3)) return 1;
    strcpy(save_name + 19, c2);
  }
  if (numx == 0 || numy == 0 || numz == 0) return 1;
  save_esclev_now(4);
  strcpy(xxgd.old_name_gat, xxgd.name_gat);
  sprintf(xxgd.name_gat, "Q %s/%s/%s",
	  save_name, save_name + 9, save_name + 19);
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
  nsum = 0;
  if ((numx > 1 && numy > 1 && *save_name == save_name[9]) ||
      (numx > 1 && numz > 1 && *save_name == save_name[19]) ||
      (numy > 1 && numz > 1 && save_name[9] == save_name[19])) {
    jjj = 1;
    if (askyn(" ...Include diagonal gates? (Y/N)")) jjj = 0;
  }
  set_signal_alert(1, "Busy reading gates...");
  /* first get 3d spectra for the background */
  for (i = 0; i < xxgd.numchs; ++i) {
    sp3db[i] = 0.0f;
  }
  for (ix = 0; ix < numx; ++ix) {
    if (check_signal_alert()) break;
    x = listx[ix];
    dx = wfact1 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		       elgd.swpars[2] * x * x) / 2.0f;
    elo1 = x - dx;
    ehi1 = x + dx;
    ilo1 = ichanno(elo1);
    ihi1 = ichanno(ehi1);
    sum1 = 0.0f;
    for (i = ilo1; i <= ihi1; ++i) {
      sum1 += xxgd.bspec[1][i];
    }
    if (numx > 1 && numy > 1 && *save_name == save_name[9]) {
      loy = ix + jjj;
    } else {
      loy = 0;
    }
    for (iy = loy; iy < numy; ++iy) {
      if (check_signal_alert()) break;
      x = listy[iy];
      dx = wfact2 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
			 elgd.swpars[2] * x * x) / 2.0f;
      elo2 = x - dx;
      ehi2 = x + dx;
      ilo2 = ichanno(elo2);
      ihi2 = ichanno(ehi2);
      sum2 = 0.0f;
      for (i = ilo2; i <= ihi2; ++i) {
	sum2 += xxgd.bspec[1][i];
      }
      read_cube(ilo1, ihi1, ilo2, ihi2);
      for (i = 0; i < xxgd.numchs; ++i) {
	sp3dxy[i] = xxgd.spec[0][i];
	xxgd.spec[0][i] = 0.0f;
      }
      if (numx > 1 && numz > 1 && *save_name == save_name[19]) {
	loz = ix + jjj;
	if (numy > 1 && save_name[9] == save_name[19])
	  loz = iy + jjj;
      } else if (numy > 1 && numz > 1 && save_name[9] == save_name[19]) {
	loz = iy + jjj;
      } else {
	loz = 0;
      }
      for (iz = loz; iz < numz; ++iz) {
	if (check_signal_alert()) break;
	x = listz[iz];
	dx = wfact3 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
			   elgd.swpars[2] * x * x) / 2.0f;
	elo3 = x - dx;
	ehi3 = x + dx;
	ilo3 = ichanno(elo3);
	ihi3 = ichanno(ehi3);
	sum3 = 0.0f;
	for (i = ilo3; i <= ihi3; ++i) {
	  xxgd.bf2 += sp3dxy[i];
	  sum3 += xxgd.bspec[1][i];
	}
	read_cube(ilo1, ihi1, ilo3, ihi3);
	for (i = 0; i < xxgd.numchs; ++i) {
	  sp3dxz[i] = xxgd.spec[0][i];
	  xxgd.spec[0][i] = 0.0f;
	}
	read_cube(ilo2, ihi2, ilo3, ihi3);
	for (i = 0; i < xxgd.numchs; ++i) {
	  sp3dyz[i] = xxgd.spec[0][i];
	  xxgd.spec[0][i] = 0.0f;
	}
	for (i = 0; i < xxgd.numchs; ++i) {
	  sp3db[i] += sum1 * sp3dyz[i] + sum2 * sp3dxz[i] + sum3 * sp3dxy[i];
	}
      }
    }
  }
  for (i = 0; i < xxgd.numchs; ++i) {
    xxgd.spec[3][i] = 0.0f;
  }
  /* now read the triple-gates from the 4cube data */
  for (ix = 0; ix < numx; ++ix) {
    if (check_signal_alert()) break;
    x = listx[ix];
    dx = wfact1 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
		       elgd.swpars[2] * x * x) / 2.0f;
    elo1 = x - dx;
    ehi1 = x + dx;
    if (numx > 1 && numy > 1 && *save_name == save_name[9]) {
      loy = ix+1 + jjj;
    } else {
      loy = 1;
    }
    for (iy = loy; iy <= numy; ++iy) {
      if (check_signal_alert()) break;
      x = listy[iy - 1];
      dx = wfact2 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
			 elgd.swpars[2] * x * x) / 2.0f;
      elo2 = x - dx;
      ehi2 = x + dx;
      if (numx > 1 && numz > 1 && *save_name == save_name[19]) {
	loz = ix+1 + jjj;
	if (numy > 1 && numz > 1 && save_name[9] == save_name[19])
	  loz = iy + jjj;
      } else if (numy > 1 && numz > 1 && save_name[9] == save_name[19]) {
	loz = iy + jjj;
      } else {
	loz = 1;
      }
      for (iz = loz; iz <= numz; ++iz) {
	if (check_signal_alert()) break;
	x = listz[iz - 1];
	dx = wfact3 * sqrt(elgd.swpars[0] + elgd.swpars[1] * x +
			   elgd.swpars[2] * x * x) / 2.0f;
	elo3 = x - dx;
	ehi3 = x + dx;
	++nsum;
	add_quad_gate(elo1, ehi1, elo2, ehi2, elo3, ehi3);
      }
    }
  }
  if (check_signal_alert()) tell("**** Gating interrupted.\n");
  set_signal_alert(0, "");
  if (nsum > 1) tell(" Sum of %d triple-gates...\n", nsum);

  /* subtract remaining background */
  for (iz = 0; iz < xxgd.numchs; ++iz) {
    xxgd.spec[0][iz] = xxgd.spec[0][iz] - xxgd.bf1 * xxgd.bspec[0][iz] -
      xxgd.bf2 * xxgd.bspec[1][iz] - sp3db[iz];
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
  return 0;
} /* quad_gate */

/* ======================================================================= */
int read_write_l4d_file(char *command)
{
  int  k, rl = 0;
#define NBBUF 28*MAXCHS
  char buf[NBBUF];

  /* read/write 4dg8r .4dg file (log. unit 22) */
  rewind(filez[22]);
  if (!strcmp(command, "READ")) {
#define RR(a,b) { get_file_rec(filez[22], a, b, -1); rl = 0; }
#define R(a,b) { memcpy(a, buf + rl, b); rl += b; }
    RR(buf, NBBUF);
    R(&xxgd.numchs, 4);
    R(&xxgd.matchs, 4);
    R(xxgd.fdname, 80);
    R(xxgd.cubnam, 80);
    RR(xxgd.luch, xxgd.numchs*4);
    RR(xxgd.pro2d, xxgd.matchs*4);
    RR(buf, NBBUF);
    for (k = 0; k < 6; ++k) {
      R(xxgd.bspec[k], xxgd.numchs*4);
    }
    R(xxgd.eff_sp, xxgd.numchs*4);
    RR(buf, NBBUF);
    R(xxgd.energy_sp, xxgd.numchs*4);
    R(xxgd.elo_sp, xxgd.numchs*4);
    RR(buf, NBBUF);
    R(xxgd.ehi_sp, xxgd.numchs*4);
    R(xxgd.ewid_sp, xxgd.numchs*4);
    RR(buf, NBBUF);
    R(&elgd.finest[0], 4*5);
    R(&elgd.swpars[0], 4*3);
    R(&elgd.h_norm, 4);
    R(&elgd.t_norm, 4);
    R(&elgd.q_norm, 4);
    RR(buf, NBBUF);
    R(&xxgd.nclook, 4);
    R(&xxgd.lookmin, 4);
    R(&xxgd.lookmax, 4);
    RR(xxgd.looktab, xxgd.nclook*2);
    RR(&xxgd.le2pro2d, 4);
    if (xxgd.le2pro2d) {
      RR(xxgd.e2pro2d, xxgd.matchs*4);
      RR(buf, NBBUF);
      R(xxgd.e2e2spec, xxgd.numchs*4);
      R(&xxgd.e2e2e2sum, 4);
    }
    RR(buf, NBBUF);
    R(&elgd.bg_err, 4);
    R(&elgd.encal_err, 4);
    R(&elgd.effcal_err, 4);
#undef RR
#undef R
    return 0;
  }
  if (strcmp(command, "WRITE")) return 0;
#define WW(a,b) { put_file_rec(filez[22], a, b); rl = 0; }
#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(&xxgd.numchs, 4);
  W(&xxgd.matchs, 4);
  W(xxgd.fdname, 80);
  W(xxgd.cubnam, 80);
  WW(buf, rl);
  WW(xxgd.luch, xxgd.numchs*4);
  WW(xxgd.pro2d, xxgd.matchs*4);
  for (k = 0; k < 6; ++k) {
    W(xxgd.bspec[k], xxgd.numchs*4);
  }
  W(xxgd.eff_sp, xxgd.numchs*4);
  WW(buf, rl);
  W(xxgd.energy_sp, xxgd.numchs*4);
  W(xxgd.elo_sp, xxgd.numchs*4);
  WW(buf, rl);
  W(xxgd.ehi_sp, xxgd.numchs*4);
  W(xxgd.ewid_sp, xxgd.numchs*4);
  WW(buf, rl);
  W(&elgd.finest[0], 4*5);
  W(&elgd.swpars[0], 4*3);
  W(&elgd.h_norm, 4);
  W(&elgd.t_norm, 4);
  W(&elgd.q_norm, 4);
  WW(buf, rl);
  W(&xxgd.nclook, 4);
  W(&xxgd.lookmin, 4);
  W(&xxgd.lookmax, 4);
  WW(buf, rl);
  WW(xxgd.looktab, xxgd.nclook*2);
  WW(&xxgd.le2pro2d, 4);
  if (xxgd.le2pro2d) {
    WW(xxgd.e2pro2d, xxgd.matchs*4);
    W(xxgd.e2e2spec, xxgd.numchs*4);
    W(&xxgd.e2e2e2sum, 4);
    WW(buf, rl);
  }
  W(&elgd.bg_err, 4);
  W(&elgd.encal_err, 4);
  W(&elgd.effcal_err, 4);
  WW(buf, rl);
#undef WW
#undef W
  /* flush data to file */
  fflush(filez[22]);
  return 0;
} /* read_write_l4d_file */
