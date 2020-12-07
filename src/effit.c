  /* program to fit Ge efficiency curves with */
  /* a seven-parameter parameterisation. */
  /* Version 4.0    D. C. Radford    Sept 1999 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "minig.h"

struct {
  float pars[9], errs[7], y[100], dy[100], x[100];
  int   freepars[7], nfp, ndp;
  int   disp, lox, hix, loy, nx, ny, iyaxis, lowx, numx;
  char  namef[80];
} efgd;

/* ==== routines defined here ==== */
int dspdat(void);
int dspfit(void);
int efalt(char *, int);
int efhelp(char *);
int efinit(char *);
int eval(float *, float, float *, float *, int);
int fitter(int);
int getdat(char *, int, int);
int parset(void);
int list_params(void);

int main(int argc, char **argv)
{
  /* program to fit Ge efficiency curves with */
  /* a seven-parameter parameterisation. */
  /* Version 4.0    D. C. Radford    Sept 1999 */
  static float a = 0.0f;
  static float b = 0.5f;

  float r1, eff_spec[16384], yeff, jpars[10];
  float x1, y1, x2, ch, rj1, rj2;
  float egamma, factor, derivs[7], fj2;
  int   j1, j2, idel[10], nchs, n;
  int   i, j, k, idata, nc, in, in2, ich, maxits;
  char  dattim[20], title[80], ans[80], line[80], *input_file_name;
  FILE  *file;

  /* initialise some parameters */
  strncpy(efgd.namef, "JUNK", 80);
  efgd.pars[7] = 100.0f;
  efgd.pars[8] = 1e3f;
  efgd.disp = 0;
  efgd.lox = 0;
  efgd.nx = 0;
  efgd.loy = 0;
  efgd.ny = 0;
  efgd.iyaxis = -3;
  efgd.ndp = 0;

  input_file_name = "";
  if (argc > 1) input_file_name = argv[1];
  /* welcome and get data file */
  efinit(input_file_name);

  while (1) {
    /* get new command */
    if ((nc = cask("?", ans, 80)) < 2) continue;
    /* convert lower case to upper case characters */
    for (i = 0; i < 2; ++i) {
      ans[i] = toupper(ans[i]);
    }
    if (!strncmp(ans, "FT", 2)) {
      /* do fit */
      if (inin(ans+2, nc-2, &idata, &in, &in2)) goto BADCMD;
      if (idata < 0) parset();  /* reset initial parameter estimates */
      while (1) {
	maxits = 100;
	if (!(k = cask("Max. no. of iterations = ? (rtn for 100)", ans, 80)) ||
	    (!inin(ans, k, &maxits, &j1, &j2) && maxits > 0)) break;
      }
      fitter(maxits);
      /* display fit and list parameters */
      if (efgd.disp) dspfit();
      list_params();

    } else if (!strncmp(ans, "LP", 2)) {
      /* list pars */
      list_params();
    } else if (!strncmp(ans, "FX", 2) ||
	       !strncmp(ans, "FR", 2)) {
      /* change fixed pars */
      if (inin(ans+2, nc-2, &idata, &in, &in2)) goto BADCMD;
      efalt(ans, idata);
    } else if (!strncmp(ans, "ND", 2)) {
      /* get new data file */
      getdat(ans, nc, 1);
    } else if (!strncmp(ans, "DD", 2)) {
      /* display data */
      dspdat();
    } else if (!strncmp(ans, "HC", 2)) {
      /* HC; generate hardcopy of graphics screen */
      hcopy();
    } else if (!strncmp(ans, "X0", 2)) {
      /* change X0, NX, Y0, NY or yaxis scale for display */
      if (inin(ans+2, nc-2, &idata, &in, &in2)) goto BADCMD;
      efgd.lox = idata;
    } else if (!strncmp(ans, "NX", 2)) {
      if (inin(ans+2, nc-2, &idata, &in, &in2)) goto BADCMD;
      efgd.nx = idata;
    } else if (!strncmp(ans, "Y0", 2)) {
      if (inin(ans+2, nc-2, &idata, &in, &in2)) goto BADCMD;
      efgd.loy = idata;
    } else if (!strncmp(ans, "NY", 2)) {
      if (inin(ans+2, nc-2, &idata, &in, &in2)) goto BADCMD;
      if (idata < -3) goto BADCMD;
      if (idata < 0) {
	efgd.iyaxis = idata;
      } else {
	efgd.ny = idata;
      }
    } else if (!strncmp(ans, "DF", 2)) {
      /* display fit */
      if (!efgd.disp) goto BADCMD;
      dspfit();
    } else if (!strncmp(ans, "WP", 2)) {
      if (!efgd.freepars[6] && /* G fixed */
	  caskyn("Before you save the pararameters, would you like\n"
		 "   to refit with parameter G free? (Y/N)")) {
	efgd.freepars[6] = 1;
	efgd.nfp -= 1;
	fitter(100);
	if (efgd.disp) dspfit();
	list_params();
	if (!caskyn(" ...Proceed to save the pararameters? (Y/N)")) continue;
      }
      /* store parameters in disk file */
      strncpy(ans, "  ", 2);
      if (nc < 3) {
	/* ask for output file name */
	k= cask("Name of output file=? (default .ext = .aef)", ans, 80);
	if (k == 0) continue;
      }
      j = setext(ans, ".aef", 80);
      if (!(file = open_new_file(ans, 0))) continue;
      datetime(dattim);
      fprintf(file, 
	      " EFFIT PARAMETER FILE  %.18s\n"
	      " %14.7E %14.7E %14.7E %14.7E %14.7E\n"
	      " %14.7E %14.7E %14.7E %14.7E %14.7E\n",
	      dattim, efgd.pars[0], efgd.pars[1], efgd.pars[2],
	      efgd.pars[3], efgd.pars[4], efgd.pars[5], efgd.pars[6],
	      efgd.pars[7], efgd.pars[8], efgd.errs[3]);
      fclose(file);
    } else if (!strncmp(ans, "HE", 2)) {
      efhelp(ans);
    } else if (!strncmp(ans, "MD", 2)) {
      /* multiply data by factor */
      if (nc < 3 || ffin(ans+2, nc-2, &factor, &rj1, &rj2)) {
	while (1) {
	  nc = cask("Mult. factor=?", ans, 80);
	  if (!ffin(ans, nc, &factor, &rj1, &rj2)) break;
	}
      }
      if (factor > 0.0f) {
	for (i = 0; i < efgd.ndp; ++i) {
	  efgd.y[i] *= factor;
	  efgd.dy[i] *= factor;
	}
	efgd.disp = 0;
      }
    } else if (!strncmp(ans, "AD", 2)) {
      /* add new data points from second file */
      if (efgd.ndp >= 100) goto BADCMD;
      getdat(ans, nc, 2);
    } else if (!strncmp(ans, "DE", 2)) {
      /* delete data points */
      /* clear graphics screen */
      erase();
      printf(" ID   Energy  Efficiency\n");
      for (i = 0; i < efgd.ndp; ++i) {
	*line = '\0';
	wrresult(line, efgd.y[i], efgd.dy[i], 0);
	printf("%3i %8.2f %s\n", i+1, efgd.x[i],  line);
      }
      while (1) {
	nc = cask(" ...ID of points to be deleted = ? (rtn to end)", ans, 80);
	if (nc == 0) break;
	for (i = 0; i < nc; ++i) {
	  if (ans[i] == ' ') ans[i] = ',';
	}
	n = sscanf(ans, "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i",
		   &idel[0], &idel[1], &idel[2], &idel[3], &idel[4],
		   &idel[5], &idel[6], &idel[7], &idel[8], &idel[9]); 
	for (i = 0; i < n; ++i) {
	  if (idel[i] <= 0) continue;
	  if (idel[i] > efgd.ndp) continue;
	  efgd.x[idel[i]-1] = 0.0;
	}
      }
      for (i = 0; i < efgd.ndp; ++i) {
	while (efgd.x [i] == 0.0 && i < efgd.ndp) {
	  --efgd.ndp;
	  for (j = i; j < efgd.ndp; ++j) {
	    efgd.x [j] = efgd.x [j+1];
	    efgd.y [j] = efgd.y [j+1];
	    efgd.dy[j] = efgd.dy[j+1];
	  }
	}
      }
    } else if (!strncmp(ans, "CR", 2)) {
      /* call cursor */
      if (!efgd.disp) goto BADCMD;
      printf("Type any character; X to exit\n");
      while (1) {
	retic(&x1, &y1, ans);
	if (*ans == 'X' || *ans == 'x') break;
	printf("  X = %.0f, Y = %.2f\n", x1, y1);
      }
    } else if (!strncmp(ans, "EX", 2)) {
      /* expand display using cursor */
      if (!efgd.disp) goto BADCMD;
      retic(&x1, &y1, ans);
      retic(&x2, &y1, ans);
      efgd.nx = (r1 = x2 - x1, fabs(r1));
      efgd.lox = x1;
      if (x1 > x2) efgd.lox = x2;
      erase();
      dspdat();
    } else if (!strncmp(ans, "RP", 2)) {
      /* read parameters from disk file */
      strncpy(ans, "  ", 2);
      i = (nc < 3);
      while (nc > 0 && (i || read_eff_file(ans, title, jpars))) {
	nc = cask("Name of input file = ? (default .ext = .aef, .eff)",
		  ans, 80);
	i = 0;
      }
      if (nc == 0) continue;
      for (i = 0; i < 9; ++i) {
	efgd.pars[i] = jpars[i];
      }
      efgd.errs[3] = jpars[9];
    } else if (!strncmp(ans, "WS", 2)) {
      /* write out calculated efficiency as a spectrum file */
      while (1) {
	nc = cask("No. of channels in output spectrum = ? (rtn for 4096)",
		  ans, 80);
	nchs = 4096;
	if (nc > 0 && inin(ans, nc, &nchs, &j1, &j2)) continue;
	if (nchs >= 8 && nchs <= 16384) break;
	printf(" Bad no. of channels - must be between 8 and 16384\n");
      }
      /* ask for energy calibration */
      while (1) {
	printf("Energy calibration is E = A + B * ch.\n"
	       "Default coefficients are: A,B = %f %f\n", a, b);
	if ((nc = cask(" ... Enter A,B ?", ans, 80)) == 0 ||
	    !ffin(ans, nc, &a, &b, &fj2)) break;
      }
      /* ask for output file name */
      nc = cask("Name for output spectrum file = ? (default .ext = .spe)",
		ans, 80);
      if (nc == 0) continue;
      for (ich = 0; ich < nchs; ++ich) {
	eff_spec[i] = 0.f;
	ch = (float) ich;
	egamma = a + b * ch;
	if (egamma > 0.f) {
	  eval(efgd.pars, egamma, &yeff, derivs, 0);
	  eff_spec[ich] = yeff;
	}
      }
      setext(ans, ".spe", 80);
      wspec(ans, eff_spec, nchs);
    } else if (!strncmp(ans, "ST", 2)) {
      /* stop */
      if (!caskyn("Are you sure you want to exit? (Y/N)")) continue;
      save_xwg("effit___");
      return 0;
    } else {
    BADCMD:
      printf("Bad command.\n");
    }
  }
} /* main */

/* ====================================================================== */
int dspdat(void)
{
  float y1, y2, y3, dx, loy=0.0f, hiy, dy1;
  int   i, i1, i2, nsx, nsy;
  int   ix, ix1, iy1, ix2, iy2;
  char  heading[60];

  initg(&nsx, &nsy);
  limg(nsx, 0, nsy, 0);
  erase();

  efgd.hix = efgd.lox + efgd.nx - 1;
  if (efgd.nx <= 0) {
    for (i = 0; i < efgd.ndp; ++i) {
      if (efgd.hix < (int) (efgd.x[i] * 1.2f))
	efgd.hix = efgd.x[i] * 1.2f;
    }
  }
  dx  = (float) (efgd.hix - efgd.lox + 1);
  hiy = (float) (efgd.loy + efgd.ny);
  loy = (float) efgd.loy;
  if (efgd.ny <= 0) {
    loy = hiy = efgd.y[0];
    for (i = 1; i < efgd.ndp; ++i) {
      if (hiy < efgd.y[i]) hiy = efgd.y[i];
      if (loy > efgd.y[i]) loy = efgd.y[i];
    }
    hiy *= 1.1f;
    loy /= 1.3f;
    if (efgd.loy != 0 || efgd.iyaxis == -1) loy = (float) efgd.loy;
  }

  dy1 = hiy - loy;
  trax(dx, (float) efgd.lox, dy1, loy, efgd.iyaxis);
  i1 = efgd.ndp;
  for (i = 0; i < i1; ++i) {
    y3 = efgd.y[i];
    y1 = y3 + efgd.dy[i];
    if (y1 > hiy) y1 = hiy;
    y2 = y3 - efgd.dy[i];
    if (y2 < loy) y2 = loy;
    if (efgd.x[i] < (float) efgd.lox ||
	efgd.x[i] > (float) efgd.hix) continue;
    if (y1 < loy || y2 > hiy) continue;
    if (y3 >= loy && y3 <= hiy) symbg(1, efgd.x[i], y3, 7);
    cvxy(&efgd.x[i], &y1, &ix, &iy1, 1);
    cvxy(&efgd.x[i], &y2, &ix, &iy2, 1);
    ix1 = ix - 3;
    ix2 = ix + 3;
    mspot(ix1, iy1);
    ivect(ix2, iy1);
    mspot(ix1, iy2);
    ivect(ix2, iy2);
    mspot(ix,  iy1);
    ivect(ix,  iy2);
  }
  datetime(heading);
  strcpy(heading+18, "  ");
  strcat(heading, efgd.namef);
  i1 = nsx - 90;
  i2 = nsy - 10;
  mspot(i1, i2);
  putg(heading, strlen(heading), 9);
  finig();
  efgd.disp = 1;
  return 0;
} /* dspdat */

/* ====================================================================== */
int dspfit(void)
{
  float x1, x2, y1, dx, derivs[7];
  int   i1, i, nsx, nsy;

  x1 = (float) efgd.hix;
  x2 = (float) efgd.lox;
  i1 = efgd.ndp;
  for (i = 0; i < i1; ++i) {
    if (x1 > efgd.x[i] * 0.8f) x1 = efgd.x[i] * 0.8f;
    if (x2 < efgd.x[i] * 1.2f) x2 = efgd.x[i] * 1.2f;
  }
  if (x1 < (float) efgd.lox) x1 = (float) efgd.lox;
  if (x2 > (float) efgd.hix) x2 = (float) efgd.hix;
  dx = x2 - x1;
  if (dx <= 0.0f) return 0;
  dx /= 250.f;
  /* display fit */
  initg(&nsx, &nsy);
  setcolor(2);
  eval(efgd.pars, x1, &y1, derivs, 0);
  pspot(x1, y1);
  for (i = 0; i < 250; ++i) {
    x1 += dx;
    eval(efgd.pars, x1, &y1, derivs, 0);
    vect(x1, y1);
  }
  setcolor(1);
  finig();
  return 0;
} /* dspfit */

/* ====================================================================== */
int efalt(char *com, int idata)
{
  /* fix or free parameters */

  static char *parc = " A   B   C   D   E   F   G";
  static int  npars = 7;

  float val, rj1, rj2;
  int   i, j, k, n, nc;
  char  ans[80];

  n = idata - 1;
  if (n >= 0 && n < npars) {
    if (!strncmp(com, "FR", 2)) {
      efgd.nfp -= 1 - efgd.freepars[n];
      efgd.freepars[n] = 1;
    } else if (efgd.nfp + efgd.freepars[n] >= npars - 1) {
      printf("Cannot - too many fixed pars.\n");
    } else {
      efgd.nfp += efgd.freepars[n];
      efgd.freepars[n] = 0;
      while (1) {
	k = cask("Value = ? (rtn for present value)", ans, 80);
	if (k > 0) {
	  if (ffin(ans, k, &val, &rj1, &rj2)) continue;
	  efgd.pars[n] = val;
	}
	break;
      }
    }
    return 0;
  }

  for (i = 0; i < npars; ++i) {
    if (efgd.freepars[i] == 0) {
      printf("%2i* ", i+1);
    } else {
      printf("%2i  ", i+1);
    }
  }
  printf("\n%s\n", parc);
  if (!strncmp(com, "FX", 2)) {
    while (efgd.nfp < npars-1 &&
	   (nc = cask("Param number to fix = ? (rtn to end)", ans, 80))) {
      if (inin(ans, nc, &n, &j, &j) || n < 1 || n > npars) {
	printf("Try again...\n");
	continue;
      }
      efgd.nfp += efgd.freepars[n-1];
      efgd.freepars[n-1] = 0;
      while (1) {
	k = cask("Value = ? (rtn for present value)", ans, 80);
	if (k != 0) {
	  if (ffin(ans, k, &val, &rj1, &rj2)) continue;
	  efgd.pars[n-1] = val;
	}
	break;
      }
    }

  } else {
    while (efgd.nfp > 0 &&
	   (nc = cask("Param number to free = ? (rtn to end)", ans, 80))) {
      if (inin(ans, nc, &n, &j, &j) || n < 1 || n > npars) {
	printf("Try again...\n");
	continue;
      }
      efgd.nfp -= 1 - efgd.freepars[n-1];
      efgd.freepars[n-1] = 1;
    }
  }
  return 0;
} /* efalt */

/* ====================================================================== */
int efhelp(char *ans)
{
  FILE *file;
  static char outfn[80] = "effithelp.txt";
  static char *hstr =
    "  Commands available:\n"
    "FT       fit using present initial parameter values\n"
    "FT -1    recalculate initial estimates and restart fit\n"
    "FX [N]   fix additional parameters [no. N] or change fixed value(s)\n"
    "FR [N]   free additional pars. [no. N]\n"
    "DF       display fit for present par. values\n"
    "LP       list parameter values\n"
    "ND FN    get new data  (FN = file name)\n"
    "AD FN    get additional data points from file FN\n"
    "DE       delete data points from data set\n"
    "DD       display data points\n"
    "X0 N     X-axis lower limit (energy) = N\n"
    "NX N     range of X-axis (energy) = N\n"
    "Y0 N     Y-axis lower limit (efficiency) = N\n"
    "NY N     N > 0 : range of Y-axis (efficiency) = N\n"
    "         N = -1/-2/-3 : linear / square-root / logarithmic Y-axis\n"
    "EX       expand display using cursor\n"
    "CR       call cursor  (hit X to exit)\n"
    "WP FN    write parameter values to disk (FN = file name )\n"
    "MD X     multiply efficiency values by factor X\n"
    "HC       generate hardcopy of graphics screen\n"
    "RP       read parameters from .eff file\n"
    "WS       write out calculated efficiency as a spectrum file\n"
    "HE[/P]   send this output to the terminal [ lineprinter ]\n"
    "ST       stop and exit from program\n";

  if (strncmp(ans, "HE/P", 4) &&
      strncmp(ans, "HE/p", 4)) {
    puts(hstr);
    return 0;
  }

  /* open output (print) file */
  if (!(file = open_new_file(outfn, 0))) return 1;
  fputs(hstr, file);
  fclose(file);
  pr_and_del_file(outfn);
  return 0;
} /* efhelp */

/* ====================================================================== */
int efinit(char *input_file_name)
{
  int  nsx, nsy;
  char ans[80];

  set_xwg("effit___");
  initg(&nsx, &nsy);
  finig();
  printf("  WELCOME TO EFFIT\n\n"
	 "This is a program to fit Ge detector efficiency curves.\n"
	 "The data are taken from .sin files, which may be  generated\n"
	 "     by running the program SOURCE.\n\n"
	 "The fitted parameters are :\n"
	 "A,B and C :  Log(Eff.) at low energies = A + B*X + C*X*X\n"
	 "    where X = log(Egamma/100).\n"
	 "    C is normally left fixed to zero.\n"
	 "D,E and F :  Log(Eff.) at high energies = D + E*Y + F*Y*Y\n"
	 "    where Y = log(Egamma/1000).\n"
	 "G :  An interaction parameter between the two regions.\n"
	 "    This parameter determines the efficiency in the turnover\n"
	 "    region; the larger G is, the sharper the turnover.\n"
	 " <Return> or 0 as an answer to any (Y/N) question is"
	 " equivalent to N\n"
	 "             1 as an answer to any (Y/N) question is"
	 " equivalent to Y\n"
	 "Type HE   to type a list of available commands\n"
	 "     HE/P to print a list of available commands.\n");

  cask("Hit any key to continue...", ans, 1);
  printf("\n\n Version 4.0    D. C. Radford    Sept 1999\n\n");
  /* ask for data file name and read data from disk */
  strcpy(ans+2, input_file_name);
  getdat(ans, strlen(input_file_name) + 2, 1);
  return 0;
} /* efinit */

/* ====================================================================== */
int eval(float *pars, float x, float *fit, float *derivs, int mode)
{
  double f, g, r, f1, f2, f3, x1, x2, y1, y2, y3;
  double dg = 0.0, d1, df1 = 0.0, df2 = 0.0;
 
  /* this eval is for use with effit */
  /* calculate the fit using present values of the pars */

  x1 = log(x / pars[7]);
  x2 = log(x / pars[8]);
  g  = pars[6];
  f1 = pars[0] + pars[1]*x1 + pars[2]*x1*x1;
  f2 = pars[3] + pars[4]*x2 + pars[5]*x2*x2;
  if (f1 <= f2) {
    f = f1;
    r = f1 / f2;
  } else {
    f = f2;
    r = f2 / f1;
  }
  if (r <= 1e-6) {
    *fit = exp(f);
    df1 = *fit;
  } else {
    y1 = pow(r, g);
    y2 = y1 + 1.0;
    d1 = -1.0 / g;
    y3 = pow(y2, d1);
    *fit = f3 = exp(f * y3);
    if (mode >= 1) {
      df1 = f3 * y3 * (1.0 - y1 / y2);
      df2 = f3 * y3 * r * y1 / y2;
      dg  = f3 * y3 * f * (log(y2)/g - y1*log(r)/y2) / g;
    }
  }
  /* calculate derivs only for mode.ge.1 */
  if (mode >= 1) {
    if (f1 <= f2) {
      derivs[0] = df1;
      derivs[3] = df2;
    } else {
      derivs[0] = df2;
      derivs[3] = df1;
    }
    derivs[1] = derivs[0] * x1;
    derivs[2] = derivs[1] * x1;
    derivs[4] = derivs[3] * x2;
    derivs[5] = derivs[4] * x2;
    derivs[6] = dg;
  }
  return 0;
} /* eval */

/* ====================================================================== */
int fitter(int maxits)
{
  static int npars = 7;

  double ddat, alpha[7][7], array[7][7];
  float  r1, diff, beta[7], b[9], delta[7], chisq=0.0f;
  float  chisq1, flamda, derivs[7], dat, fit, ers[7];
  int    conv, nits, test, i, j, k, l, m, nextp[7], ndf, nip;

  /* this subroutine is a modified version of 'CURFIT', in Bevington */
  /* see page 237 */
  nip = npars - efgd.nfp;
  ndf = efgd.ndp - nip;
  if (ndf < 1) {
    printf("No D.O.F.\n");
    return 1;
  }
  if (nip < 2) {
    printf("Too many fixed pars.\n");
    return 1;
  }
  k = 0;
  for (j = 0; j < npars; ++j) {
    if (efgd.freepars[j]) nextp[k++] = j;
  }
  if (k != nip) {
    printf("nip != sum(freepars)!!!!\n");
    return 1;
  }
  flamda = 0.001f;
  nits = 0;
  test = 0;
  derivs[0] = 1.0f;
  for (i = 0; i < npars; ++i) {
    efgd.errs[i] = 0.0f;
    b[i] = efgd.pars[i];
  }
  b[7] = efgd.pars[7];
  b[8] = efgd.pars[8];
  /* evaluate fit, alpha & beta matrices, & chisq */
 NEXT_ITERATION:
  for (j = 0; j < nip; ++j) {
    beta[j] = 0.0f;
    for (k = 0; k <= j; ++k) {
      alpha[k][j] = 0.0;
    }
  }
  chisq1 = 0.0f;
  for (i = 0; i < efgd.ndp; ++i) {
    eval(efgd.pars, efgd.x[i], &fit, derivs, 1);
    diff = efgd.y[i] - fit;
    dat = efgd.dy[i] * efgd.dy[i];
    if (dat < 1.0f) dat = 1.0f;
    ddat = (double) dat;
    chisq1 += diff * diff / dat;
    for (l = 0; l < nip; ++l) {
      j = nextp[l];
      beta[l] += diff * derivs[j] / dat;
      for (m = 0; m <= l; ++m) {
	alpha[m][l] += (double) derivs[j] * (double) derivs[nextp[m]] / ddat;
      }
    }
  }
  chisq1 /= (float) ndf;
  /* invert modified curvature matrix to find new parameters */
 INVERT_MATRIX:
  array[0][0] = flamda + 1.0f;
  for (j = 1; j < nip; ++j) {
    for (k = 0; k < j; ++k) {
      if (alpha[j][j] * alpha[k][k] == 0.0) {
	printf("Cannot - diag. element %i or %i eq. to 0.0\n", j, k);
	return 1;
      }
      array[k][j] = alpha[k][j] / sqrt(alpha[j][j] * alpha[k][k]);
      array[j][k] = array[k][j];
    }
    array[j][j] = flamda + 1.0f;
  }
  matinv(array[0], nip, 7);
  if (!test) {
    for (j = 0; j < nip; ++j) {
      if (alpha[j][j] * alpha[j][j] == 0.0) {
	printf("Cannot - diag. element %i eq. to 0.0\n", j);
	return 1;
      }
      delta[j] = 0.f;
      for (k = 0; k < nip; ++k) {
	delta[j] += beta[k] * array[k][j] / sqrt(alpha[j][j] * alpha[k][k]);
      }
    }
    /* if chisq increased, increase flamda & try again */
    chisq = 0.0f;
    for (l = 0; l < nip; ++l) {
      j = nextp[l];
      b[j] = efgd.pars[j] + delta[l];
    }
    for (i = 0; i < efgd.ndp; ++i) {
      eval(b, efgd.x[i], &fit, derivs, 0);
      diff = efgd.y[i] - fit;
      dat = efgd.dy[i] * efgd.dy[i];
      if (dat == 0.f) dat = 1.f;
      chisq += diff * diff / dat;
    }
    chisq /= (float) ndf;
    if (chisq > chisq1 && flamda < 2.f) {
      flamda *= 10.f;
      goto INVERT_MATRIX;
    }
  }

  /* evaluate parameters and errors */
  /* test for convergence */
  conv = 1;
  for (j = 0; j < nip; ++j) {
    if (array[j][j] < 0.0) array[j][j] = 0.0;
    ers[j] = sqrt(array[j][j] /  alpha[j][j]) * sqrt(flamda + 1.0f);
    if ((r1 = delta[j], fabs(r1)) >= ers[j] / 1e3f) conv = 0;
  }
  if (!test) {
    for (j = 0; j < npars; ++j) {
      efgd.pars[j] = b[j];
    }
    flamda /= 10.f;
    ++nits;
    if (! conv && nits < maxits) goto NEXT_ITERATION;

    /* re-do matrix inversion with flamda=0 to calculate errors */
    flamda = 0.f;
    test = 1;
    goto INVERT_MATRIX;
  }

  /* list data and exit */
  for (l = 0; l < nip; ++l) {
    efgd.errs[nextp[l]] = ers[l];
  }
  printf(" %s\n %i indept. pars    %i degrees of freedom\n",
	 efgd.namef, nip, ndf);
  if (conv) {
    printf(" %i iterations,  Chisq/D.O.F. = %.3f\n", nits, chisq);
    return 0;
  }
  printf(" Failed to converge after %i iterations,  Chisq/D.O.F. = %.3f\n"
	 "  WARNING - do not believe quoted errors.\n", nits, chisq);
  return 2;
} /* fitter */

/* ====================================================================== */
int getdat(char *ans, int nc, int mode)
{
  static int npars = 7;

  float erat, e, de, eff, rat, err, c, dc, trat=0.0f, tes=0.0f;
  float y1, a, da, factor, derivs[7], intens, dint, rj1, rj2;
  int   i, j, idata, n, ndp1=0;
  char  title[120], line[120];
  FILE *file;

  /* mode = 1: get new data */
  /* mode = 2: add data from second file with norm. factor */
  strncpy(ans, "  ", 2);
  if (nc < 3) {
    /* ask for data file name and read data from disk */
  GETFN:
    nc = cask("Data file = ? (default .ext = .sin)", ans, 80);
    if (nc == 0 && mode >= 2) return 0;
  }
  setext(ans, ".sin", 80);
  if (!(file = open_readonly(ans))) goto GETFN;
  erase();
  finig();
  efgd.disp = 0;

  /* calculate relative efficiencies from source areas and intensities */
  if (!fgets(title, 120, file)) goto GETFN;
  if (mode <= 1) {
    strncpy(efgd.namef, ans, 80);
    i = 0;
    printf("%s\n   Energy      Efficiency\n", title);
  } else {
    i = ndp1 = efgd.ndp;
    printf("%s\n   Energy      Efficiency      Ratio\n",
	   title);
  }
  while (fgets(line, 120, file) && i < 99) {
    if ((j=sscanf(line, "%d%f%f%f%f%f%f%f%f",
		  &n, &c, &dc, &a, &da, &e, &de, &intens, &dint)) != 9) {
      file_error("read", ans);
      goto GETFN;
    }
    if (a == 0.0f) continue;
    if (intens == 0.0f) break;
    eff = a / intens;
    err = eff * sqrt(da*da/(a*a) + dint*dint/(intens*intens));
    if (mode <= 1) {
      *line = '\0';
      wrresult(line, eff, err, 0);
      printf("%9.2f     %s\n", e, line);
    } else {
      rat = 0.f;
      erat = 0.f;
      eval(efgd.pars, e, &y1, derivs, 0);
      rat = y1 / eff;
      erat = rat * err / eff;
      trat += rat  / (erat*erat);
      tes  += 1.0f / (erat*erat);
      *line = '\0';
      wrresult(line, eff, err, 15);
      wrresult(line, rat, erat, 0);
      printf("%9.2f     %s\n", e, line);
    }
    efgd.x[i] = e;
    efgd.y[i] = eff;
    efgd.dy[i++] = err;
  }
  if (i >= 100) printf("First 100 points only taken\n");
  efgd.ndp = i;
  fclose(file);
  if (efgd.ndp < 2) {
    printf("Too few data points.\n");
    goto GETFN;
  }
  if (mode <= 1) {
   for (i = 0; i < npars; ++i) {
      efgd.freepars[i] = 1;
    }
    parset();
    efgd.freepars[2] = 0; /* C = 0 */
    efgd.freepars[6] = 0; /* G = 15 */
    efgd.nfp = 2;
    idata = 0;
    printf("\n\n");
    strcpy(ans, "FX");
    efalt(ans, idata);
  } else {
    if (tes > 0.0f) {
      trat /= tes;
      tes = 1.0f / sqrt(tes);
    }
    *line = '\0';
    wrresult(line, trat, tes, 0);
    printf("   Mean ratio = %s\n", line);
    while (1) {
      nc = cask("     Norm. factor = ?", ans, 80);
      if (!ffin(ans, nc, &factor, &rj1, &rj2) &&
	  factor > 0.0f) {
	/* multiply data by factor */
	for (i = ndp1; i < efgd.ndp; ++i) {
	  efgd.y[i] *= factor;
	  efgd.dy[i] *= factor;
	}
	break;
      }
    }
  }
  dspdat();
  return 0;
} /* getdat */

/* ====================================================================== */
int parset(void)
{
  static int npars = 7;

  float r1, r2;
  int   i, ix1, ix2;

  for (i = 0; i < npars; ++i) efgd.errs[i] = 0.f;

  ix1 = 0;
  ix2 = 0;
  for (i = 1; i < efgd.ndp; ++i) {
    if ((r1 = efgd.x[ix1] - efgd.pars[7], fabs(r1)) >
	(r2 = efgd.x[i] - efgd.pars[7], fabs(r2))) ix1 = i;
    if ((r1 = efgd.x[ix2] - efgd.pars[8], fabs(r1)) >
	(r2 = efgd.x[i] - efgd.pars[8], fabs(r2))) ix2 = i;
  }
  if (efgd.freepars[1] == 1) efgd.pars[1] = 1.5f;
  if (efgd.freepars[0] == 1) efgd.pars[0] = log(efgd.y[ix1]) +
      efgd.pars[1]*(log(efgd.pars[7]) - log(efgd.x[ix1]));
  if (efgd.freepars[2] == 1) efgd.pars[2] = 0.0f;
  if (efgd.freepars[4] == 1) efgd.pars[4] = -0.9f;
  if (efgd.freepars[3] == 1)efgd.pars[3] = log(efgd.y[ix2]) +
      efgd.pars[4] * (log(efgd.pars[8]) - log(efgd.x[ix2]));
  if (efgd.freepars[5] == 1) efgd.pars[5] = 0.0f;
  if (efgd.freepars[6] == 1) efgd.pars[6] = 15.0f;
  return 0;
} /* parset */

/* ====================================================================== */
int list_params(void)
{
  int  j;
  char l[200];

  strcpy(l, " A =");
  wrresult(l, efgd.pars[0], efgd.errs[0], 15);
  strcat(l, "   B =");
  wrresult(l, efgd.pars[1], efgd.errs[1], 32);
  strcat(l, "   C =");
  j = wrresult(l, efgd.pars[2], efgd.errs[2], 0);
  strcat(l, "\n D =");
  wrresult(l, efgd.pars[3], efgd.errs[3], j + 16);
  strcat(l, "   E =");
  wrresult(l, efgd.pars[4], efgd.errs[4], j + 33);
  strcat(l, "   F =");
  wrresult(l, efgd.pars[5], efgd.errs[5], 0);
  strcat(l, "\n G =");
  wrresult(l, efgd.pars[6], efgd.errs[6], 0);
  printf("%s\n", l);
  return 0;
} /* list_params */
