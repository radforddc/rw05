#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"
#include "minig.h"
#include "levit8r.h"

/* ======================================================================= */
int l_gls_init(int argc, char **argv)
{
  float d1, d2;
  int   jext, i, j, i1, i2, nc, nx, ny, matchs, numchs, jnumchs, rl, icubenum;
  char  buf[256], filnam[80], *infile1 = "", *infile2 = "", *c;

  /* initialise */
  /* ....open output files */
  /* ....open .lev file */
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
  xxgd.numchs = 1;
  xxgd.many_cubes = 0;
  xxgd.cubefact[0] = xxgd.cubefact[1] =
    xxgd.cubefact[2] = xxgd.cubefact[3] = xxgd.cubefact[4] = 0.0;
  setcubenum(0);

  set_xwg("levit8r_");
  strncpy(xxgd.progname, "levit8r_", 8);
  initg2(&nx, &ny);
  finig();
  tell("\n\n"
       "  LEVIT8R Version 4.0    D.C. Radford    Oct. 2005\n\n"
       "     Welcome....\n\n");

  /* parse input parameters */
  prfile = 0;
  *prfilnam = 0;
  if (argc > 1) {
    infile1 = argv[1];
    if (!strcmp(infile1, "-l")) {
      infile1 = "";
      strcpy(prfilnam, "levit8r");
      if (argc > 2) strcpy(prfilnam, argv[2]);
      if (argc > 3) infile1 = argv[3];
      if (argc > 4) infile2 = argv[4];
    } else if (argc > 2) {
      infile2 = argv[2];
      if (!strcmp(infile2, "-l")) {
	infile2 = "";
	strcpy(prfilnam, "levit8r");
	if (argc > 3) strcpy(prfilnam, argv[3]);
	if (argc > 4) infile2 = argv[4];
      } else if (argc > 3) {
	strcpy(prfilnam, "levit8r");
	if (argc > 4) strcpy(prfilnam, argv[4]);
      }
    }
  }
  /* ask for and open level scheme file */
  gls_init(infile1);
  /* open log (print) file */
  if (*prfilnam) {
    setext(prfilnam, ".log", 80);
    prfile = open_new_file(prfilnam, 0);
  }
  /* ask for and open .lev data file */
  xxgd.v_width = -1.0f;
  for (j = 0; j < MAXCHS; ++j) {
    xxgd.v_depth[j] = 0.0f;
  }

  if ((nc = strlen(infile2)) > 0) {
    strcpy(filnam, infile2);
  } else {
  START:
    nc = askfn(filnam, 80, "", ".lev",
	       "Levit8r data file = ? (rtn to create new file)");
  }
  if (nc == 0) {
    prep();
  } else {
    jext = setext(filnam, ".lev", 80);
    if (!(filez[22] = fopen(filnam, "r+"))) {
      file_error("open", filnam);
      goto START;
    }
    strcpy(xxgd.levfile, filnam);
    /* read file */
    if (get_file_rec(filez[22], buf, 256, 0) <= 0) goto RERR;
    rl = 0;
#define R(a,b) { memcpy(a, buf + rl, b); rl += b; }
    R(&numchs, 4);
    R(&matchs, 4);
    R(xxgd.cubnam, 80);
#undef R
    if (numchs > MAXCHS) {
      warn1("ERROR: Number of channels in cube file %s is > MAXCHS (%d)\n", filnam, MAXCHS);
      fclose(filez[1]);
      goto START;
    }
    read_write_l4d_file("READ");

    setcubenum(0);
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
      goto START;
    }
    if (jnumchs != numchs) {
      warn1("ERROR -- Cube file %s has wrong number of channels!\n", filnam);
      fclose(filez[22]);
      close3dfile();
      goto START;
    }

    if (xxgd.many_cubes) {
      for (icubenum = 0; icubenum < 5 && xxgd.cubefact[icubenum]; icubenum++) {
	setcubenum(icubenum + 1);
	strcpy(filnam, xxgd.cubenam1[icubenum]);
	if (!inq_file(filnam, &j)) {
	  /* cube file does not exist? - try pre-pending path to .lev file */
	  strcpy(filnam, xxgd.levfile);
	  if ((c = rindex(filnam, '/'))) {
	    *(c+1) = '\0';
	  } else {
	    filnam[0] = '\0';
	  }
	  strcat(filnam, xxgd.cubenam1[icubenum]);
	}
	if (open3dfile(filnam, &jnumchs)) {
	  fclose(filez[22]);
	  for (i = icubenum - 1; i >= 0; i--) {
	    setcubenum(i);
	    close3dfile();
	  }
	  goto START;
	}
	if (jnumchs != numchs) {
	  warn1("ERROR: Cube file # %d has wrong number of channels!\n",
		icubenum + 2);
	  fclose(filez[22]);
	  close3dfile();
	  for (i = icubenum; i >= 0; i--) {
	    setcubenum(i);
	    close3dfile();
	  }
	  goto START;
	}
      }
    }

    /* open .lev_log file for append */
    strcpy(filnam, xxgd.levfile);
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
	      2 != sscanf(lx, "%i%f", &i2, &d2)) break;
	  if (i2 <= i1 || i2 >= MAXCHS) {
	    warn1("Starting channels out of order\n"
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
} /* l_gls_init */

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
  } else if (*ans == 'T') {
    if (trip_gate(ans, nc-1)) return 1;
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
int prep(void)
{
  float fmax, fwhm, test, x, effspec[MAXCHS];
  int   jext, i, j, iymax, ii, ix, iy, reclen, ich;
  char  fn[80], filnam[80], dattim[20], ans[80];

  warn1(" Cube data file preparation routine...\n\n"
       " You will need:\n"
       " 3D histogram cube file;\n"
       " The look-up table (.tab) file used in the cube replay;\n"
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

  /* get cube data file */
  while (1) {
    askfn(xxgd.cubnam, 80, "", ".cub", "Cube data file = ?");
    jext = setext(xxgd.cubnam, ".cub", 80);
    if (open3dfile(xxgd.cubnam, &xxgd.numchs)) continue;
    if (xxgd.numchs <= MAXCHS) break;
    warn1(" ERROR: Cube file has too many channels!\n"
	 "...Number of channels in 4cube is %d\n"
	 "       ...maximum is MAXCHS = %d\n", xxgd.numchs, MAXCHS);
    close3dfile();
  }
  tell("%d channels...\n", xxgd.numchs);
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
  for (iy = 1; iy < xxgd.numchs; ++iy) {
    xxgd.luch[iy] = xxgd.luch[iy-1] + xxgd.numchs - iy;
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

  /* get name of new .lev file */
  strcpy(fn, filnam);
  strcpy(filnam, xxgd.cubnam);
  strcpy(filnam + jext, ".lev");
  (void) askfn(ans, 80, filnam, ".lev",
               "\nName for new levit8r data file = ?\n");
  jext = setext(ans, ".lev", 80);
  filez[22] = open_new_file(ans, 1);
  strcpy(xxgd.levfile, ans);
  /* open .lev_log file for append */
  strcpy(ans + jext + 4, "_log");
  if (!(filez[26] = fopen(ans, "a+"))) {
    file_error("open", ans);
    exit(1);
  }
  datetime(dattim);
  fprintf(filez[26], "%s: Created new levit8r data file %s\n",
	  dattim, xxgd.levfile);
  fprintf(filez[26], "      ...cube file = %s\n", xxgd.cubnam);
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
  ii = iymax;
  fmax = 1e-6f;
  for (j = 0; j < ii; ++j) {
    ich = xxgd.luch[j] + ii;
    test = (xxgd.pro2d[ich] - xxgd.bspec[0][j] * xxgd.bspec[1][ii] - 
	    xxgd.bspec[2][ii] * xxgd.bspec[1][j] - xxgd.bspec[3][j] * 
	    xxgd.bspec[4][ii] - xxgd.bspec[5][ii] * xxgd.bspec[4][j]) / 
      (effspec[j] * effspec[ii]);
    if (test > fmax) fmax = test;
  }
  for (j = ii; j < xxgd.numchs; ++j) {
    ich = xxgd.luch[ii] + j;
    test = (xxgd.pro2d[ich] - xxgd.bspec[0][j] * xxgd.bspec[1][ii] - 
	    xxgd.bspec[2][ii] * xxgd.bspec[1][j] - xxgd.bspec[3][j] * 
	    xxgd.bspec[4][ii] - xxgd.bspec[5][ii] * xxgd.bspec[4][j]) / 
      (effspec[j] * effspec[ii]);
     if (test > fmax) fmax = test;
  }
  /* set up height normalization factor to give coinc. int.
     of 100 for most intense peak */
  elgd.h_norm = 100.0f / fmax;
  tell("\n H_NORM Scaling factor = %10.3e\n\n", elgd.h_norm);
  datetime(dattim);
  fprintf(filez[26], "%s: 2D Scaling factor = %f\n", dattim, elgd.h_norm);
  fflush(filez[26]);
  /* write projections, calibrations etc. to file */
  tell("\nWriting .lev file...\n");
  read_write_l4d_file("WRITE");
  return 0;
} /* prep */

/* ======================================================================= */
int read_write_l4d_file(char *command)
{
  float fj1, fj2;
  int   j, k, nc, rlen, rl = 0;
#define NBBUF 28*MAXCHS
  char  buf[NBBUF], ans[80];

  /* read/write levit8r .lev file (log. unit 22) */
  rewind(filez[22]);
  if (!strcmp(command, "READ")) {
#define RR(a,b) { rlen = get_file_rec(filez[22], a, b, -1); rl = 0; }
#define R(a,b) { memcpy(a, buf + rl, b); rl += b; }
    RR(buf, NBBUF);
    R(&xxgd.numchs, 4);
    R(&xxgd.matchs, 4);
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
    RR(buf, NBBUF);
    R(&xxgd.nclook, 4);
    R(&xxgd.lookmin, 4);
    R(&xxgd.lookmax, 4);
    RR(xxgd.looktab, xxgd.nclook*2);
    RR(&xxgd.le2pro2d, 4); if (rlen != 4) goto ERR1;
    if (xxgd.le2pro2d) {
      RR(xxgd.e2pro2d, xxgd.matchs*4); if (rlen != xxgd.matchs*4) goto ERR1;
      RR(buf, NBBUF); if (rlen != xxgd.numchs*4 + 4) goto ERR2;
      R(xxgd.e2e2spec, xxgd.numchs*4);
      R(&xxgd.e2e2e2sum, 4);
    }
    RR(buf, NBBUF); if (rlen != 12) goto ERR3;
    R(&elgd.bg_err, 4);
    R(&elgd.encal_err, 4);
    R(&elgd.effcal_err, 4);

    /* stuff added for linear combinations of cubes: */
    RR(buf, NBBUF);
    if (rlen < 4) {
      xxgd.many_cubes = 0;
      xxgd.cubefact[0] = xxgd.cubefact[1] =
	xxgd.cubefact[2] = xxgd.cubefact[3] = xxgd.cubefact[4] = 0.0;
      setcubenum(0);
    } else {
      R(&xxgd.many_cubes, 4);
      if (xxgd.many_cubes) {
	R(xxgd.cubenam1, 80*5);
	R(&xxgd.cubefact, 4*5);
      }
    }
    if (xxgd.many_cubes) RR(xxgd.dpro2d, xxgd.matchs*4);
#undef RR
#undef R
    return 0;

  ERR1:
    xxgd.le2pro2d = 0;
    for (j = 0; j < xxgd.matchs; ++j) {
      xxgd.e2pro2d[j] = 0.0f;
    }
  ERR2:
    for (j = 0; j < xxgd.numchs; ++j) {
      xxgd.e2e2spec[j] = 0.0f;
    }
    xxgd.e2e2e2sum = 0.0f;
    warn1("NOTE: E2 correction will not be used or will be incorrect"
	 " in double-gating...\n"
	 "If this is not satisfactory, redefine your background spectra.\n");
  ERR3:
    warn1("\n"
	 " This program adds systematic errors for the background\n"
	 "   subtraction, and for the energy and efficiency\n"
	 "   calibrations. These are missing from this .lev file.\n");
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
    if (!askyn("Save these values? (Y/N)")) return 0;
    rewind(filez[22]);

  } else if (strcmp(command, "WRITE")) {
    return 0;
  }

#define WW(a,b) { put_file_rec(filez[22], a, b); rl = 0; }
#define W(a,b) { memcpy(buf + rl, a, b); rl += b; }
  W(&xxgd.numchs, 4);
  W(&xxgd.matchs, 4);
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

  /* stuff added for linear combinations of cubes: */
  W(&xxgd.many_cubes, 4);
  if (xxgd.many_cubes) {
    W(xxgd.cubenam1, 80*5);
    W(&xxgd.cubefact, 4*5);
  }
  WW(buf, rl);
  if (xxgd.many_cubes) WW(xxgd.dpro2d, xxgd.matchs*4);
#undef WW
#undef W
  /* flush data to file */
  fflush(filez[22]);
  return 0;
} /* read_write_l4d_file */
