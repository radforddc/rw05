#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

int main(int argc, char **argv)
{
  double a[6], x[50], y[50], dy[50], y2[50], dy2[50], dd[2][50];
  double d, e, f, a1, a2, chisq1, chisq2;
  int    npts=0, i;
  char   line[120];
  FILE  *f_in, *f_out;
  
  int polfit(double *, double *, double *,
	     int, int, int, double *, double *);

  /* fwhm_cal  -  a program to fit FWHM values as a function of energy */
  printf(" \n\n"
     " Welcome to fwhm_cal.\n\n"
     " This program does polynomial fits to FWHM values as  afunction of energy.\n"
     " It takes its input data from a file with three columns of numbers: Energy, fwhm, error\n\n"
     " D. C. Radford   Sept. 2019.\n");

  if (argc < 2) {
    printf("Usage: %s input_file_name\n\n", argv[0]);
    return 0;
  }
  if (!(f_in = fopen(argv[1], "r"))) {
    printf("Error: Input file %s does not exist?\n\n", argv[1]);
    return 1;
  }    

  /* get data from input file */
  while (fgets(line, sizeof(line), f_in)) {
    if (line[0] == '#' || strlen(line) < 4) continue;
    if (sscanf(line, "%lf %lf %lf", &x[npts], &y[npts], &dy[npts]) < 3) {
      printf(" ERROR; reading input file failed! line: %s\n", line);
      fclose(f_in);
      return 1;
    }
    if (++npts >= 50) break;
  }
  fclose(f_in);
  
  if (npts < 3) {
    printf("Too few data points.\n");
    return 1;
  }
  for (i=0; i<npts; i++) {
    y2[i] = y[i]*y[i];  // FWHM squared
    dy2[i] = 2.0 * dy[i] * y[i];
  }

  // First fit with sqrt(a + b*x)
  polfit(x, y2, dy2, npts, 2, 1, a, &chisq1);
  chisq2 = 0;
  a1 = a[0];
  a2 = a[1];
  printf("Fit 1: y = SQRT(a*a + b*x)\n"
         "       a = %.3f    b = %.3e\n"
         "      E    fwhm   error    fit     diff     resid\n", sqrt(a1), a2);
    //     1  277.4  0.9160 0.0030  0.9110   0.0050   1.5830
  for (i = 0; i < npts; ++i) {
    f = sqrt(a1 + a2 * x[i]);
    d = y[i] - f;
    printf("%2d %6.1f %7.4f %6.4f %7.4f %8.4f %8.4f\n",
           i+1, x[i], y[i], dy[i], f, d, d/dy[i]);
    chisq2 += (d*d) / (dy[i]*dy[i]);
    dd[0][i] = d;
  }
  printf("\n Chisq/DOF = %.3f (%.3f)\n\n", chisq1, chisq2/(npts - 2.0));

  // Second fit with sqrt(a + b*x)
  polfit(x, y2, dy2, npts, 3, 1, a, &chisq1);
  chisq2 = 0;
  printf("Fit 2: y = SQRT(a*a + b*x + c*x*x)\n"
         "       a = %.3f    b = %.3e  c = %.3e\n"
         "      E    fwhm   error    fit     diff     resid\n", sqrt(a[0]), a[1], a[2]);
    //     1  277.4  0.9160 0.0030  0.9110   0.0050   1.5830
  for (i = 0; i < npts; ++i) {
    f = sqrt(a[0] + a[1] * x[i] + a[2] * x[i] * x[i]);
    d = y[i] - f;
    printf("%2d %6.1f %7.4f %6.4f %7.4f %8.4f %8.4f\n",
           i+1, x[i], y[i], dy[i], f, d, d/dy[i]);
    chisq2 += (d*d) / (dy[i]*dy[i]);
    dd[1][i] = d;
  }
  printf("\n Chisq/DOF = %.3f (%.3f)\n\n", chisq1, chisq2/(npts - 3.0));

  f_out = fopen("fwhm_cal.out", "w");
  fprintf(f_out, "#    E      fwhm  error    delta1   delta2\n");
  //               1  277.4  0.9160 0.0030   0.0050  -0.0020
  for (i = 0; i < npts; ++i) {
    fprintf(f_out, "%2d %6.1f %7.4f %6.4f %8.4f %8.4f\n",
            i+1, x[i], y[i], dy[i], dd[0][i], dd[1][i]);
  }
  fprintf(f_out, "\n#    E   fit1   fit2\n");
  for (e = 0; e < x[npts-1]*1.3; e += x[npts-1]/50.0) {
    fprintf(f_out, " %6.2f %6.3f %6.3f\n",
            e, sqrt(a1 + a2 * e), sqrt(a[0] + a[1] * e + a[2] * e * e));
  }
  fprintf(f_out, "\n %6.2f %6.3f %6.3f\n %6.2f %6.3f %6.3f\n",
            0.0, 0.0, 0.0, x[npts-1]*1.05, 0.0, 0.0);
  e = 2038.0;
  f = sqrt(a1 + a2 * e);
  d =sqrt(a[0] + a[1] * e + a[2] * e * e);
  fprintf(f_out, "\n %6.2f %6.3f %6.3f\n %6.2f %6.3f %6.3f\n %6.2f %6.3f %6.3f\n",
          0.0, f, d, e, f, d, e, 0.5, 0.5);

  fclose(f_out);

  return 0;
} /* encal */

/* ====================================================================== */
int polfit(double *x, double *y, double *sigmay, int npts,
	   int nterms, int mode, double *a, double *chisqr)
{
  double weight, sumx[19], sumy[10];
  double delta, chisq, array[10][10];
  double xterm, yterm, xi, yi;
  int    nmax, i, j, k, l, n;

  double determ(double *, int);

  /* accumulate weighted sums */
  nmax = 2*nterms - 1;
  for (n = 0; n < nmax; ++n) {
    sumx[n] = 0.;
  }
  for (j = 0; j < nterms; ++j) {
    sumy[j] = 0.;
  }
  chisq = 0.;
  for (i = 0; i < npts; ++i) {
    xi = x[i];
    yi = y[i];
    if (mode < 0) {
      if (yi > 0.f) {
	weight = 1.f / yi;
      } else if (yi < 0.f) {
	weight = 1.f / (-yi);
      } else {
	weight = 1.f;
      }
    } else if (mode == 0) {
      weight = 1.f;
    } else {
      weight = 1.f / (sigmay[i] * sigmay[i]);
    }
    xterm = weight;
    for (n = 0; n < nmax; ++n) {
      sumx[n] += xterm;
      xterm *= xi;
    }
    yterm = weight * yi;
    for (n = 0; n < nterms; ++n) {
      sumy[n] += yterm;
      yterm *= xi;
    }
    chisq += weight * yi * yi;
  }
  /* construct matrices and calculate coefficients */
  for (j = 0; j < nterms; ++j) {
    for (k = 0; k < nterms; ++k) {
      n = j + k;
      array[k][j] = sumx[n];
    }
  }
  delta = determ(array[0], nterms);
  if (delta == 0.f) {
    *chisqr = 0.f;
    for (j = 0; j < nterms; ++j) {
      a[j] = 0.f;
    }
  } else {
    for (l = 0; l < nterms; ++l) {
      for (j = 0; j < nterms; ++j) {
	for (k = 0; k < nterms; ++k) {
	  n = j + k;
	  array[k][j] = sumx[n];
	}
	array[l][j] = sumy[j];
      }
      a[l] = determ(array[0], nterms) / delta;
    }
    /* calculate chi square */
    for (j = 0; j < nterms; ++j) {
      chisq -= a[j] * 2.f * sumy[j];
      for (k = 0; k < nterms; ++k) {
	n = j + k;
	chisq += a[j] * a[k] * sumx[n];
      }
    }
    *chisqr = chisq / (npts - nterms);
  }
  return 0;
} /* polfit */

/* ====================================================================== */
double determ(double *array, int norder)
{
  double save, ret_val;
  int    i, j, k;

  ret_val = 1.f;
  for (k = 0; k < norder; ++k) {
    /* interchange columns if diagonal element is zero */
    if (array[k + k*10] == 0.f) {
      for (j = k; j < norder; ++j) {
	if (array[k + j*10] != 0.f) break;;
      }
      if (j >= norder) return 0.f;
      for (i = k; i < norder; ++i) {
	save = array[i + j*10];
	array[i + j*10] = array[i + k*10];
	array[i + k*10] = save;
      }
      ret_val = -ret_val;
    }
    /* subtract row k from lower rows to get diagonal matrix */
    ret_val *= array[k + k*10];
    for (i = k+1; i < norder; ++i) {
      for (j = k+1; j < norder; ++j) {
	array[i + j*10] -= array[i + k*10] * array[k + j*10] / array[k + k*10];
      }
    }
  }
  return ret_val;
} /* determ */
