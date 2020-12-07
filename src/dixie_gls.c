#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "minig.h"
#include "gls.h"

int initg2(int *, int *);
int retic2(float *, float *, char *);
int retic3(float *, float *, char *, int *);

int d_gls_exec(char *ans, int nc);
int d_gls_help(void);
int d_gls_init(char *);
int examine(int mode);
int disp_dat(int mode);
int wplot(void);
int wrout(void);
int calcul(int iband);
int curse(void);

struct {
  float i0, i1, e0, rr_fact, x[6][100], y[6][100], bk[MAXBAND];
  int   ndb, ndp, data_band[MAXBAND], data_type[MAXBAND];
  int   disp, lox1, hix1, loy1, hiy1, lox, loy, nx, ny, issize;
} dixgd;
/* x[0][*] contains hbar-ohmega
   x[1][*]          initial spin of gamma
   x[2][*]          Egamma
   x[3][*]          mean value of hbar-ohmega
   x[4][*]          spin of level
   x[5][*]          J(J+1)
   y[0][*] contains I_x
   y[1][*]          aligned angular momentum
   y[2][*]          Routhian
   y[3][*]          first moment of inertia
   y[4][*]          second moment of inertia
   y[5][*]          excitation energy minus rigid rotor */

extern float fdx, fx0, fdy, fy0;
extern int   idx, ix0, idy, iy0, iyflag;

int  rlen;
char lx[120], prfilnam[80];
FILE *file, *prfile, *filez[40];

/* ======================================================================= */
int main(int argc, char **argv)
{
  int   nc;
  char  ans[80], *input_file_name;

  /* program to display I_x, aligned angular momentum, and routhians
     for rotation bands in nuclei
     input comes from .gls level scheme files
     Version 1.0      D.C. Radford           Feb 1995 */

  /* initialise */
  input_file_name = "";
  if (argc > 1) {
    input_file_name = argv[1];
    printf("\nInput file name: %s\n", input_file_name);
  }
  d_gls_init(input_file_name);

  while (1) {
    /* get and execute command */
    if ((nc = cask("Next command?", ans, 80))) d_gls_exec(ans, nc);
  }
} /* main */

/* ======================================================================= */
int d_gls_exec(char *ans, int nc)
{
  float x1, y1, x2, y2, fj, fi0, fi1;
  int   iwin1, iwin2, i, idata, j1, j2;

  /* this subroutine decodes and executes the commands */
  /* convert lower case to upper case characters */
  for (i = 0; i < 2; ++i) {
    ans[i] = toupper(ans[i]);
  }
  if (!gls_exec2(ans)) return 0;
  if (!strncmp(ans, "EX", 2)) {
    /* expand data display or level scheme display using cursor */
    while (1) {
      retic3(&x1, &y1, ans, &iwin1);
      retic3(&x2, &y2, ans, &iwin2);
      if (iwin1 == iwin2) break;
      printf("Click twice in the window you want to expand.\n");
    }
    if (iwin1 == 1) {
      dixgd.nx = fabs(x2 - x1);
      dixgd.lox = (x1 < x2 ? x1 : x2);
      dixgd.ny = fabs(y2 - y1);
      dixgd.loy = (y1 < y2 ? y1 : y2);
      disp_dat(0);
    } else {
      glsgd.x0 = (x1 < x2 ? x1 : x2);
      glsgd.hix = (x1 > x2 ? x1 : x2);
      glsgd.y0 = (y1 < y2 ? y1 : y2);
      glsgd.hiy = (y1 > y2 ? y1 : y2);
      display_gls(0);
    }
  } else if (!strncmp(ans, "HE", 2)) {
    gls_help();
    d_gls_help();
  } else if (!strncmp(ans, "CH", 2)) {
    printf("Current Values for I0, I2 = %.2f, %.2f\n", dixgd.i0, dixgd.i1);
    while ((nc = cask("...New values = ? (rtn to end)", ans, 80))) {
      if (ffin(ans, nc, &fi0, &fi1, &fj)) continue;
      dixgd.i0 = fi0;
      dixgd.i1 = fi1;
      disp_dat(1);
    }
  } else if (!strncmp(ans, "CR", 2)) {
    curse();
  } else if (!strncmp(ans, "DD1", 3)) {
    disp_dat(1);
  } else if (!strncmp(ans, "DD", 2)) {
    disp_dat(0);
  } else if (!strncmp(ans, "NX", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    dixgd.nx = idata;
  } else if (!strncmp(ans, "NY", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    dixgd.ny = idata;
  } else if (!strncmp(ans, "ST", 2)) {
    if (caskyn("Are you sure you want to exit? (Y/N)")) {
      read_write_gls("WRITE");
      save_xwg("dixiegls");
      exit(0);
    }
  } else if (!strncmp(ans, "WD", 2)) {
    wrout();
  } else if (!strncmp(ans, "WP", 2)) {
    wplot();
  } else if (!strncmp(ans, "X0", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    dixgd.lox = idata;

  /* examine (calculate and display) data from bands */
  } else if (!strncmp(ans, "XA", 2)) {
    /* alignments */
    examine(1);
  } else if (!strncmp(ans, "XE", 2)) {
    /* E_x - RigidRotor */
    examine(5);
  } else if (!strncmp(ans, "XI", 2)) {
    /* I_x */
    examine(2);
  } else if (!strncmp(ans, "XM", 2)) {
    /* moments of inertia */
    examine(3);
  } else if (!strncmp(ans, "XR", 2)) {
    /* routhians */
    examine(4);
  } else if (!strncmp(ans, "Y0", 2)) {
    if (inin(ans + 2, nc - 2, &idata, &j1, &j2)) goto ERR;
    dixgd.loy = idata;
  } else {
    /* command cannot be recognized */
  ERR:
    printf("Bad energy or command.\n");
    return 1;
  }
  return 0;
} /* d_gls_exec */

/* ======================================================================= */
int d_gls_help(void)
{
 char  ans[4];

  cask(" ...press any key for more...", ans, 1);
  printf("\n"
    " XA       examine alignment data\n"
    " XE       examine (E_x - RigidRotor) data\n"
    " XI       examine I_x data\n"
    " XM       examine moment-of-inertia data\n"
    " XR       examine Routhian data\n"
    " X0,NX    set up data display X-axis\n"
    " Y0,NY    set up data display Y-axis\n"
    " EX       expand display using cursor\n"
    " CH       change Harris parameters\n"
    " CR       call cursor\n"
    " DD       redisplay data\n"
    " DD1      redisplay data with autoscale\n"
    " WD       write displayed data to dixie-type file\n"
    " WP       write displayed plot to plotdata-type file\n"
    " ST       stop and exit program\n\n");
  cask(" ...press any key when done...", ans, 1);
  return 0;
} /* d_gls_help */

/* ======================================================================= */
int d_gls_init(char *infilnam)
{
  float fj;
  int   nc;
  char  ans[80];

/* initialise */
  dixgd.disp = 0;
  dixgd.lox = 0;
  dixgd.nx = 0;
  dixgd.loy = 0;
  dixgd.ny = 0;
  dixgd.issize = 9;
  dixgd.ndb = 0;
  dixgd.rr_fact = 7.0f;
  set_xwg("dixiegls");
  initg2(&dixgd.nx, &dixgd.ny);
  finig();
  printf("\n\n\n"
	 " DIXIE_GLS Version 2.0    D. C. Radford   Oct 1999\n"
	 "     Welcome....\n\n"
	 " This is a program to calculate and display I_x, alignments,\n"
	 " Routhians and moments of inertia for rotational bands\n"
	 " in nuclei. Data is taken from .gls level scheme files.\n\n");

/* ask for and open level scheme file */
  if ((nc = strlen(infilnam)) > 0) {
    strcpy(glsgd.gls_file_name, infilnam);
    setext(glsgd.gls_file_name, ".gls", 80);
    if (read_write_gls("READ")) read_write_gls("OPEN_READ");
  } else {
    read_write_gls("OPEN_READ");
  }
  indices_gls();
  calc_band();
  glsgd.x0 = glsgd.tx0;
  glsgd.hix = glsgd.thix;
  glsgd.y0 = glsgd.ty0;
  glsgd.hiy = glsgd.thiy;
  calc_gls_fig();
  display_gls(1);
  while ((nc = cask("Starting values for Harris parameters I0, I2 = ?",
		    ans, 80)) &&
	 ffin(ans, nc, &dixgd.i0, &dixgd.i1, &fj));
  return 0;
} /* d_gls_init */

/* ======================================================================= */
int examine(int mode)
{
  float bandk, x1, y1, fk, fj1, fj2;
  int   ilev, iband, lev_f, lev_i, nc, igamma;
  char  ans[80];

/* mode = 1/2/3/4/5:
   alignments / I_x / moments of inertia /  routhians / E_x - RigidRotor */
  while (mode == 5) {
    printf("Default Rigid Rotor factor = %f\n", dixgd.rr_fact);
    if (!(nc = cask(" ... enter desired factor, or return for default. ? ",
		    ans, 80)) ||
	!ffin(ans, nc, &dixgd.rr_fact, &fj1, &fj2)) break;
  }

 START:
  printf("Button 1: Display data for selected band\n"
	 "Button 2: Overlay data for selected band\n"
	 "Button 3 or X: Exit\n");
  retic2(&x1, &y1, ans);
  if (!strcmp(ans, "X") || !strcmp(ans, "x") || ans[1] == '3') return 0;
  iband = nearest_band(x1, y1);
  if (iband < 0 || iband >= glsgd.nbands) {
    printf("  ... no band selected, try again...\n");
  } else {
    printf(" Band %.8s selected...\n", glsgd.bnd[iband].name);
    if (ans[1] == '1' || ans[1] == '\0') {
      hilite(-1);
      dixgd.ndb = 0;
    }
    ++dixgd.ndb;
    dixgd.data_band[dixgd.ndb - 1] = iband;
    if (mode == 1) {
      dixgd.data_type[dixgd.ndb - 1] = 1;
    } else if (mode == 2) {
      dixgd.data_type[dixgd.ndb - 1] = 0;
    } else if (mode == 3) {
      *ans = '0';
      while (*ans != '1' && *ans != '2') {
	cask("Enter 1/2 for 1st/2nd Moment of Inertia?", ans, 1);
      }
      if (*ans == '1') {
	dixgd.data_type[dixgd.ndb - 1] = 3;
      } else {
	dixgd.data_type[dixgd.ndb - 1] = 4;
      }
    } else if (mode == 4) {
      dixgd.data_type[dixgd.ndb - 1] = 2;
    } else if (mode == 5) {
      dixgd.data_type[dixgd.ndb - 1] = 5;
      for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
	if (glsgd.lev[ilev].band == iband) {
	  disp_dat(1);
	  goto START;
	}
      }
    }
    bandk = 1e3f;
    for (igamma = 0; igamma < glsgd.ngammas; ++igamma) {
      lev_i = glsgd.gam[igamma].li;
      lev_f = glsgd.gam[igamma].lf;
      if (glsgd.lev[lev_i].band == iband &&
	  glsgd.lev[lev_f].band == iband &&
	  glsgd.lev[lev_i].j == glsgd.lev[lev_f].j + 2.0f && 
	  glsgd.lev[lev_i].pi * glsgd.lev[lev_f].pi > 0.0f) {
	/* have found an in-band stretched E2 gamma */
	hilite(igamma);
	if (bandk > glsgd.lev[lev_f].j) bandk = glsgd.lev[lev_f].j;
      }
    }
    if (bandk == 1e3f) goto START;
    if (bandk > 0.0f) {
      printf("Default value of K for band = %.1f\n", bandk);
      while ((nc = cask("K = ? (rtn for default)", ans, 80))) {
	if (ffin(ans, nc, &fk, &fj1, &fj2)) continue;
	if (fk < 0.0f || fk > bandk) {
	  printf("*** Illegal value ***\n");
	  continue;
	}
	bandk = fk;
	break;
      }
    }
    dixgd.bk[dixgd.ndb - 1] = bandk;
    disp_dat(1);
  }
  goto START;
} /* examine */

/* ======================================================================= */
int disp_dat(int mode)
{
  float xmax = 0.0f, ymax = 0.0f, ymin = 0.0f, dx, xx, yy, dy1;
  int   draw, i, n, isymb, nsx, nsy;

/* display data */
/* mode = 1 for autoscale */

  if (dixgd.ndb <= 0) return 0;
  initg(&nsx, &nsy);
  erase();
  limg(nsx, 0, nsy, 0);
  if (mode == 1 || dixgd.nx <= 0 || dixgd.ny <= 0) {
    xmax = ymax = ymin = 0.0f;
    for (n = 0; n < dixgd.ndb; ++n) {
      calcul(n);
      for (i = 0; i < dixgd.ndp; ++i) {
	xx = dixgd.x[0][i];
	if (dixgd.data_type[n] == 4) xx = dixgd.x[3][i];
	if (dixgd.data_type[n] == 5) xx = dixgd.x[4][i];
	if (xmax < xx) xmax = xx;
	if (ymax < dixgd.y[dixgd.data_type[n]][i]) {
	  ymax = dixgd.y[dixgd.data_type[n]][i];
	}
	if (ymin > dixgd.y[dixgd.data_type[n]][i]) {
	  ymin = dixgd.y[dixgd.data_type[n]][i];
	}
      }
    }
  }
  dixgd.lox1 = dixgd.lox;
  dixgd.hix1 = dixgd.lox + dixgd.nx - 1;
  if (mode == 1 || dixgd.nx <= 0) {
    dixgd.lox1 = 0;
    dixgd.hix1 = xmax * 1.2f;
  }
  glsgd.x0 = (float) dixgd.lox1;
  dx = (float) (dixgd.hix1 - dixgd.lox1 + 1);
  dixgd.loy1 = dixgd.loy;
  dixgd.hiy1 = dixgd.loy1 + dixgd.ny;
  if (mode == 1 || dixgd.ny <= 0) {
    dixgd.loy1 = ymin * 1.2f;
    if (ymin < 0.0f) --dixgd.loy1;
    dixgd.hiy1 = ymax * 1.2f;
  }
  glsgd.y0 = (float) dixgd.loy1;
  dy1 = (float) (dixgd.hiy1 - dixgd.loy1 + 1);
  trax(dx, glsgd.x0, dy1, glsgd.y0, -1);
  isymb = 1;
  for (n = 0; n < dixgd.ndb; ++n) {
    calcul(n);
    draw = 0;
    for (i = 0; i < dixgd.ndp; ++i) {
      if (i == 0 && dixgd.data_type[n] == 4) continue;
      xx = dixgd.x[0][i];
      if (dixgd.data_type[n] == 4) xx = dixgd.x[3][i];
      if (dixgd.data_type[n] == 5) xx = dixgd.x[4][i];
      yy = dixgd.y[dixgd.data_type[n]][i];
      if (draw) vect(xx, yy);
      symbg(isymb, xx, yy, dixgd.issize);
      pspot(xx, yy);
      draw = 1;
    }
    ++isymb;
    if (isymb > 6) isymb = 1;
  }
  finig();
  dixgd.disp = 1;
  return 0;
} /* disp_dat */

/* ======================================================================= */
int wplot(void)
{
  static char heading[6][40] =
    { "I{dx}  ({sq})",
      "Alignment, i{dx} ({sq})",
      "Routhian  (keV)",
      "{sJ}{u(1)}  ({sq}{u2}/MeV)" ,
      "{sJ}{u(2)}  ({sq}{u2}/MeV)",
      "E{dx} - E{dRR}  (keV)" };
  int  i, n, isymb, nc, jjx;
  char dattim[20], outfil[80];

/* write out data to plotdata-type .pdc file */
  if (dixgd.ndb <= 0) {
    printf("No data to write!\n");
    return 0;
  }
  nc = cask("Output file = ? (default .ext = .pdc)", outfil, 80);
  if (nc == 0) return 0;
  setext(outfil, ".pdc", 80);
  if (!(filez[2] = open_new_file(outfil, 0))) return 0;
  datetime(dattim);
  fprintf(filez[2],
	  "c 4 5 2\n\n"
	  "** Dixie_gls plot output %.18s\n"
	  "lin 100 200 100 200\n"
	  "    %i %i %i %i\n"
	  "x,  \"{sq}{gw}  (keV)\" 2\n"
	  "xt, \"\" 1\n"
	  "y,  \"%s\" 2\n"
	  "yr, \"\" 1\n\n",
	  dattim,
	  dixgd.lox1, dixgd.hix1 - dixgd.lox1 + 1,
	  dixgd.loy1, dixgd.hiy1 - dixgd.loy1 + 1,
	  heading[dixgd.data_type[0]]);
  isymb = 1;
  for (i = 0; i < dixgd.ndb; ++i) {
    calcul(i);
    if (dixgd.data_type[i] == 5) {
      fprintf(filez[2],
	      "s %i 3\n"
	      "d 1 0 3 0\n"
	      "* Band = %.8s         Rigid Rotor E = %.2f*J*(J+1)\n"
	      "* Band-head Spin = %.1f    Band-head Energy %.1f\n"
	      "*   Spin  J(J+1) E_x-E_RR\n",
	      isymb, glsgd.bnd[dixgd.data_band[i]].name, 
	      dixgd.rr_fact, dixgd.x[4][0], dixgd.e0);
      for (n = 0; n < dixgd.ndp; ++n) {
	fprintf(filez[2], "%8.1f %7.1f %8.1f\n",
		dixgd.x[4][n], dixgd.x[5][n], dixgd.y[5][n]);
      }
    } else {
      jjx = 3;
      if (dixgd.data_type[i] == 4) jjx = 9;
      fprintf(filez[2],
	"s %i 3\n"
	"d %i 0 %i 0\n"
	"* Band = %.8s  Harris parameters: I0 = %.2f, I2 = %.2f\n"
	"* Band-head Spin = %.1f,  K(band) = %.1f   Band-head Energy = %.1f\n"
	"* Egamma  I(init)  homega     I_x     i_x    "
	"Routh    I(1)    I(2)  Mean_homega\n",
	      isymb, jjx, dixgd.data_type[i] + 4,
	      glsgd.bnd[dixgd.data_band[i]].name, dixgd.i0, dixgd.i1,
	      dixgd.x[1][0] - 2.0f, dixgd.bk[i], dixgd.e0);
      for (n = 0; n < dixgd.ndp; ++n) {
	if (dixgd.data_type[i] == 4 && n == 0) {
	  fprintf(filez[2],
		  "*%8.2f %8.1f %7.2f %7.2f %7.2f %8.1f %7.1f %7.1f %7.2f\n",
		  dixgd.x[2][0], dixgd.x[1][0], dixgd.x[0][0], dixgd.y[0][0],
		  dixgd.y[1][0], dixgd.y[2][0], dixgd.y[3][0], dixgd.y[4][0],
		  dixgd.x[3][0]);
	} else {
	  fprintf(filez[2],
		  "%8.2f %8.1f %7.2f %7.2f %7.2f %8.1f %7.1f %7.1f %7.2f\n",
		  dixgd.x[2][n], dixgd.x[1][n], dixgd.x[0][n], dixgd.y[0][n],
		  dixgd.y[1][n], dixgd.y[2][n], dixgd.y[3][n], dixgd.y[4][n],
		  dixgd.x[3][n]);
	}
      }
    }
    ++isymb;
    if (isymb > 6) isymb = 1;
    fprintf(filez[2], "\n");
  }
  fclose(filez[2]);
  return 0;
} /* wplot */

/* ======================================================================= */
int wrout(void)
{
  static char version[20] = "** DIXIE_GLS OUTPUT ";
  int   i, n, nc;
  char  dattim[20], outfil[80];

/* write out data to dixie-type file */
  if (dixgd.ndb <= 0) {
    printf("No data to write!\n");
    return 0;
  }
  nc = cask("Output file = ? (default .ext = .dix)", outfil, 80);
  if (nc == 0) return 0;
  setext(outfil, ".dix", 80);
  if (!(filez[2] = open_new_file(outfil, 0))) return 0;
  datetime(dattim);
  for (i = 0; i < dixgd.ndb; ++i) {
    calcul(i);
    fprintf(filez[2], "%s%s\n", version, dattim);
    fprintf(filez[2],
      "* Band = %.8s   Harris parameters: I0 = %.2f, I2 = %.2f\n"
      "* Band-head Spin = %.1f,  K(band) = %.1f   Band-head Energy = %.1f\n\n"
      "* Egamma  I(init)     I_x    i_x  homega  Routhian    I(1)    I(2)\n\n",
	    glsgd.bnd[dixgd.data_band[i]].name, dixgd.i0, dixgd.i1,
	    dixgd.x[1][0] - 2.0f, dixgd.bk[i], dixgd.e0);
    for (n = 0; n < dixgd.ndp; ++n) {
      fprintf(filez[2],
	      "%8.2f %8.1f %7.2f %6.2f %7.1f %9.0f %7.1f %7.1f\n",
	      dixgd.x[2][n], dixgd.x[1][n], dixgd.y[0][n], dixgd.y[1][n],
	      dixgd.x[0][n], dixgd.y[2][n], dixgd.y[3][n], dixgd.y[4][n]);
    }
    fprintf(filez[2], "\n");
  }
  fclose(filez[2]);
  return 0;
} /* wrout */

/* ======================================================================= */
int calcul(int iband)
{
  float e_iplus1, iref, spin, e_imoins1, del_e, e_ref;
  float e_mev, e_exp, hw, hw2, i_x, ixf, ixi;
  int   igam[100];
  int   ilev, j, jband, n, k, lev_f, lev_i, k2, igamma;

  jband = dixgd.data_band[iband];
  dixgd.ndp = 0;
  if (dixgd.data_type[iband] == 5) {
    for (ilev = 0; ilev < glsgd.nlevels; ++ilev) {
      if (glsgd.lev[ilev].band == jband) {
	dixgd.x[4][dixgd.ndp] = glsgd.lev[ilev].j;
	dixgd.y[5][dixgd.ndp] = glsgd.lev[ilev].e;
	/* order levels by spin */
	for (j = dixgd.ndp; j > 0 && dixgd.x[4][j - 1] > dixgd.x[4][j]; --j) {
	  dixgd.x[4][j] = dixgd.x[4][j - 1];
	  dixgd.y[5][j] = dixgd.y[5][j - 1];
	  dixgd.x[4][j - 1] = glsgd.lev[ilev].j;
	  dixgd.y[5][j - 1] = glsgd.lev[ilev].e;
	}
	++dixgd.ndp;
      }
    }
    if (dixgd.ndp <= 0) return 0;
    dixgd.e0 = dixgd.y[5][0];
    for (n = 0; n < dixgd.ndp; ++n) {
      spin = dixgd.x[4][n];
      dixgd.x[5][n] = spin * (spin + 1.0f);
      dixgd.y[5][n] -= dixgd.rr_fact * spin * (spin + 1.0f);
    }
    return 0;
  }
  for (igamma = 0; igamma < glsgd.ngammas; ++igamma) {
    lev_i = glsgd.gam[igamma].li;
    lev_f = glsgd.gam[igamma].lf;
    if (glsgd.lev[lev_i].band == jband &&
	glsgd.lev[lev_f].band == jband &&
	glsgd.lev[lev_i].j == glsgd.lev[lev_f].j + 2.0f &&
	glsgd.lev[lev_i].pi * glsgd.lev[lev_f].pi > 0.0f) {
      /* have found an in-band stretched E2 gamma */
      igam[dixgd.ndp] = igamma;
      dixgd.x[1][dixgd.ndp] = glsgd.lev[lev_i].j;
      dixgd.x[2][dixgd.ndp] = glsgd.gam[igamma].e;
      /* order gammas by spin */
      for (j = dixgd.ndp; j > 0 && dixgd.x[1][j - 1] > dixgd.x[1][j]; --j) {
	igam[j] = igam[j - 1];
	dixgd.x[1][j] = dixgd.x[1][j - 1];
	dixgd.x[2][j] = dixgd.x[2][j - 1];
	igam[j - 1] = igamma;
	dixgd.x[1][j - 1] = glsgd.lev[lev_i].j;
	dixgd.x[2][j - 1] = glsgd.gam[igamma].e;
      }
      ++dixgd.ndp;
    }
  }
  if (dixgd.ndp <= 0) return 0;
  /* band-head energy */
  dixgd.e0 = glsgd.lev[glsgd.gam[igam[0]].lf].e;
  k = dixgd.bk[iband];
  k2 = k * k;
  for (n = 0; n < dixgd.ndp; ++n) {
    e_mev = dixgd.x[2][n] / 1e3f;
    spin = dixgd.x[1][n] - 1.0f;
    i_x = sqrt(spin * (spin + 1.0f) - k2);
    ixi = sqrt((spin + 1.0f) * (spin + 2.0f) - k2);
    ixf = sqrt((spin - 1.0f) * spin - k2);
    hw = e_mev / (ixi - ixf);
    hw2 = hw * hw;
    iref = (dixgd.i0 + dixgd.i1 * hw2) * hw;
    e_ref = dixgd.i0 * -.5f * hw2 - dixgd.i1 * 0.25f * hw2 * hw2 + .125f / 
      dixgd.i0;
    e_imoins1 = glsgd.lev[glsgd.gam[igam[n]].lf].e;
    e_iplus1 = glsgd.lev[glsgd.gam[igam[n]].li].e;
    e_exp = (e_iplus1 + e_imoins1) * 5e-4f - i_x * hw;
    dixgd.x[0][n] = hw * 1e3f;
    if (n > 0) {
      dixgd.x[3][n] = (dixgd.x[0][n] + dixgd.x[0][n - 1]) / 2.0f;
    } else {
      dixgd.x[3][n] = 0.0f;
    }
    dixgd.y[0][n] = i_x;
    dixgd.y[1][n] = i_x - iref;
    dixgd.y[2][n] = (e_exp - e_ref) * 1e3f;
    dixgd.y[3][n] = (spin * 2.0f + 1.0f) / e_mev;
    if (n > 0) {
      del_e = (dixgd.x[2][n] - dixgd.x[2][n - 1]) / 1e3f;
      if (del_e < 1e-4f) {
	dixgd.y[4][n] = 0.0f;
      } else {
	dixgd.y[4][n] = 4.0f / del_e;
      }
    } else {
      dixgd.y[4][n] = 0.0f;
    }
  }
  return 0;
} /* calcul */

/* ======================================================================= */
int curse(void)
{
  float x, y;
  char  ans[80];

/* call cursor */
  printf("Type any character; X or right mouse button to exit.\n");
  while (1) {
    retic(&x, &y, ans);
    if (*ans == 'X' || *ans == 'x') return 0;
    printf("  X =%10.2f  Y =%10.2f\n", x, y);
  }
} /* curse */

/* ======================================================================= */
int report_curs3(int ix, int iy, int iwin)
{
  float  x, y;
  int    g, l;

  cvxy(&x, &y, &ix, &iy, 2);
  if (iwin == 1) {
    printf(" y = %.0f", y);
    if ((g = nearest_gamma(x, y)) >= 0) {
      printf(" Gamma %.1f, I = %.2f", glsgd.gam[g].e, glsgd.gam[g].i);
    } else {
      printf("                     ");
    }
    if ((l = nearest_level(x, y)) >= 0) {
      printf(" Level %s, E = %.1f   \r", glsgd.lev[l].name, glsgd.lev[l].e);
    } else {
      printf("                               \r");
    }
  } else {
    printf(" x = %.1f, y = %.0f                                    \r", x, y);
  }
  fflush(stdout);
  return 0;
}

/* ======================================================================= */
int done_report_curs(void)
{
  printf("                                      "
	 "                                    \r");
  fflush(stdout);
  return 0;
}
