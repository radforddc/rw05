#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int main(void)
{
  double d1, d2;
  double enc, enc2, enc4, enc6, enc8;
  float  r1, r2, a[3][7], b[10][10];
  float  e[10], g[7][7], p[10][10], r, angle[10], w[10], y[3][10];
  float  thick[10], trans, wnorm[10], x2[3], fk[7];
  float  egamma, pq[7], ewnorm[10], deg, err[3][7], att;
  int    iout1, i, j, jj, nt, nr, ir, irr, nnt, nrs, nrt;
  char   nf[80], of2[80] = "legft.prt", of3[80] = "legft.out";
  char   line[120], *nd;
  FILE   *file1, *file2, *file3;
  /* ************************************
     program legft
     program to fit angular distributions with legendre polynomials
     by the least squares method
     author:           D.C. Radford
     first written:    Feb 1989
     latest revision:  Jan 8 1992
     description:
     This program fits angular distributions using legendre polynomials
     It does two fits to each data set; the first fit is with two
     (P0+P2), the second is with three terms (P0+P2+P4). The
     input file is a modified .eng file, created by running the program
     ENERGY and editing its output file.
     The 1st line must contain the number of angles
     The 2nd line must contain the angles in degrees
     The 3rd line must contain normalisation values
     The 4th line must contain the errors on these values
     The fifth line is used for an optional correction for gamma-ray
     absorbtion in the target backing. If you do not want this correction,
     leave a blank line. Otherwise put the target backing thickness seen
     at each angle (in arbitrary units). At the end of
     each data set, at the end of the line beginning EGAMMA =, you
     must then add the absorbtion per unit thickness for that gamma ray
     (Format F10.5).
     Output is to 2 files; legft.prt has a brief summary of the results
     and is printed automatically, and legft.out has more detail and can
     be printed by the user.
     **************************************** */

  printf(
    "\n\n"
    "Welcome to legft.\n\n"
    "This program fits angular distributions using legendre polynomials.\n"
    "It does two fits to each data set;\n"
    " the first fit is with two terms (P0+P2),\n"
    " the second is with three terms (P0+P2+P4).\n"
    "The expected input file is a modified .eng file, created by running\n"
    " the program ENERGY and editing its output file.\n"
    "The first line must contain the number of angles        (INTEGER)\n"
    "The second line must contain the angles in degrees      (up to 10 REALs)\n"
    "The third line must contain normalisation values        (up to 10 REALs)\n"
    "The fourth line must contain the errors on these values (up to 10 REALs)\n"
    "The fifth line is used for an optional correction for\n"
    " gamma-ray absorbtion in the target backing.  If you do not want\n"
    " this correction, leave a blank line.  Otherwise put the target\n"
    " backing thickness seen at each angle (in arbitrary units,\n"
    " up to 10 REALs). At the end of each data set, at the end of the line\n"
    " beginning EGAMMA =...., you must then add the absorbtion per unit\n"
    " thickness for that gamma ray      (REAL)\n\n"
    "        D. C. Radford    Feb. 1989.\n");
  file2 = open_new_file(of2, 1);
  file3 = open_new_file(of3, 1);

  /* ask for data file name */
 START:
  while (1) {
    cask("Input file name = ? (default .ext = .eng)", nf, 80);
    setext(nf, ".eng", 80);
    if ((file1 = open_readonly(nf))) break;
  }
  if (!fgets(line, 120, file1)) goto NEXTFIL;
  nt = strtol(line, &nd, 0);
  printf("%d angles...\n", nt);
  if (nt < 3 || nt > 10) {
    printf("No. of angles must be > 3  and < 11\n");
    goto NEXTFIL;
  }
  for (i = 0; i < nt; i++) {
    angle[i] = ewnorm[i] = thick[i] = 0.0;
    wnorm[i] = 1.0;
  }

#define R(data) fgets(line, 120, file1); nd = line; if (strlen(line)>=2*nt) { \
                for (i = 0; i < nt; i++) {data[i] = strtod(nd, &nd); nd++;}}
  R(angle);
  R(wnorm);   /* wnorm = normalisation intensity */
  R(ewnorm);  /* ewnorm = normalisation error */
  R(thick);   /* thick = thickness of target backing */
#undef R

  fprintf(file2,
	  "\f\n File = %s\n\n"
	  "   Egamma +- error          a0 +- error    a2/a0 +- error"
	  "    a4/a0 +- error     Chisq\n", nf);
  iout1 = 0;
 NEXTDAT:
  for (nnt = 0; nnt < nt; ++nnt) {
    fgets(line, 120, file1);
    if (sscanf(line, "%*d%*f%*f%f%f", &w[nnt], &e[nnt]) != 2) goto NEXTFIL;
    /* w,e = area,error */
    if (w[nnt] == 0.0f && e[nnt] == 0.0f) nnt--;
  }
  fgets(line, 120, file1);
  att = 0.0f;
  if (sscanf(line, "%*14c%f%*3c%f%*21c%f", &egamma, &deg, &att) < 2)
    goto NEXTFIL;
  fgets(line, 120, file1);
  
  /* att = attenuation of gamma per unit thickness (c.f. thick) */
  trans = 1.0f - att;
  for (nnt = 0; nnt < nt; ++nnt) {
    r1 = e[nnt] / w[nnt];
    r2 = ewnorm[nnt] / wnorm[nnt];
    e[nnt] = sqrt(r1 * r1 + r2 * r2);
    d1 = (double) trans;
    d2 = (double) thick[nnt];
    w[nnt] /= wnorm[nnt] * pow(d1, d2);
    e[nnt] *= w[nnt];
  }
  /* calculate legendre polynomials */
  for (nnt = 0; nnt < nt; ++nnt) {
    enc  = cos(angle[nnt] * 0.0174533f);
    enc2 = enc*enc;
    enc4 = enc2*enc2;
    enc6 = enc4*enc2;
    enc8 = enc4*enc4;
    p[nnt][0] = 1.0f;
    p[nnt][2] = (enc2 * 3.0 - 1.0) / 2.0;
    p[nnt][4] = (enc4 * 35.0 - enc2 * 30.0 + 3.0) / 8.0;
    p[nnt][6] = (enc6 * 231.0 - enc4 * 315.0f + enc2 * 105.0 - 5.0) / 16.0;
    p[nnt][8] = (enc8 * 6435.0 - enc6 * 12012.0 + enc4 * 6930.0 -
		 enc2 * 1260.0 + 35.0) / 128.0;
  }

  for (nr = 3; nr <= 5; nr += 2) {
    jj = nr/2 - 1;
    for (irr = 0; irr < nr; irr += 2) {
      fk[irr] = 0.f;
      for (ir = 0; ir < nr; ir += 2) {
	b[irr][ir] = 0.f;
      }
    }
    for (nnt = 0; nnt < nt; ++nnt) {
      for (irr = 0; irr < nr; irr += 2) {
	for (ir = 0; ir < nr; ir += 2) {
	  b[irr][ir] += p[nnt][ir]*p[nnt][irr] / (e[nnt]*e[nnt]);
	}
	fk[irr] += w[nnt] * p[nnt][irr] / (e[nnt]*e[nnt]);
      }
    }
    g[0][0] = 1.f / b[0][0];
    for (ir = 2; ir < nr; ir += 2) {
      r = b[ir][ir];
      for (i = 0; i < ir; i += 2) {
	pq[i] = 0.f;
	for (j = 0; j < ir; j += 2) {
	  pq[i] += g[j][i] * b[ir][j];
	}
	r -= pq[i] * b[ir][i];
      }
      g[ir][ir] = 1.f / r;
      for (i = 0; i < ir; i += 2) {
	g[ir][i] = g[i][ir] = -pq[i] / r;
	for (j = 0; j < ir; j += 2) {
	  g[j][i] += pq[i] * pq[j] / r;
	}
      }
    }
    for (nrt = 0; nrt < nr; nrt += 2) {
      err[jj][nrt] = sqrt(g[nrt][nrt]);
      a[jj][nrt] = 0.f;
      for (nrs = 0; nrs < nr; nrs += 2) {
	a[jj][nrt] += g[nrs][nrt] * fk[nrs];
      }
    }
    for (nnt = 0; nnt < nt; ++nnt) {
      y[jj][nnt] = 0.f;
    }
    for (nnt = 0; nnt < nt; ++nnt) {
      for (nrt = 0; nrt < nr; nrt += 2) {
	y[jj][nnt] += a[jj][nrt] * p[nnt][nrt];
      }
    }
    x2[jj] = 0.f;
    for (nnt = 0; nnt < nt; ++nnt) {
      r1 = (y[jj][nnt] - w[nnt]) / e[nnt];
      x2[jj] += r1 * r1;
    }
    x2[jj] /= (float) (nt - (nr + 1) / 2);
    for (nrt = 2; nrt < nr; nrt += 2) {
      a[jj][nrt] /= a[jj][0];
      err[jj][nrt] /= a[jj][0];
      r1 = err[jj][1] * a[jj][nrt] / a[jj][0];
      err[jj][nrt] = sqrt(err[jj][nrt]*err[jj][nrt] + r1*r1);
    }
  }
  fprintf(file3, "    Angle  cos**2         Data +- error"
	  "      Fit(p0+p2)   Fit(p0+p2+p4)\n");
  for (nnt = 0; nnt < nt; ++nnt) {
    enc = cos(angle[nnt] * .0174533f);
    enc2 = enc*enc;
    fprintf(file3, "%8.2f %7.4f %12.2f %8.2f %15.2f %15.2f\n",
    angle[nnt], enc2, w[nnt], e[nnt], y[0][nnt], y[1][nnt]);
  }
  fprintf(file3, "   Chisq = %8.3f %8.3f\n", x2[0], x2[1]);
  fprintf(file3,
	  "    File = %s  a0 = %.2f +- %.2f, [%.2f +- %.2f]\n"
	  "  Egamma = %.4f +- %.4f   a2/a0 = %.4f +- %.4f, [%.4f +- %.4f]\n"
	  "  [a4/a0 = %.4f +- %.4f]\n\n",
	  nf, a[0][0], err[0][0], a[1][0], err[1][0], egamma, deg,
	      a[0][2], err[0][2], a[1][2], err[1][2], a[1][4], err[1][4]);
  fprintf(file2,
	  "%10.3f %7.3f %12.2f %7.2f %9.3f %7.3f %27.3f\n"
	  "%31.2f %7.2f %9.3f %7.3f %9.3f %7.3f %9.3f\n",
	  egamma, deg,
	  a[0][0], err[0][0], a[0][2], err[0][2], x2[0],
	  a[1][0], err[1][0], a[1][2], err[1][2], a[1][4], err[1][4], x2[1]);
  ++iout1;
  if (iout1 >= 28) {
    printf("\f\n File = %s\n\n"
	   "   Egamma +- error          a0 +- error    a2/a0 +- error"
	   "    a4/a0 +- error     Chisq\n", nf);
    iout1 = 0;
  }
  goto NEXTDAT;

 NEXTFIL:
  fclose(file1);
  if (caskyn("Would you like to process more files? (Y/N)")) goto START;
  pr_and_del_file(of2);
  fclose(file3);
  printf("Condensed output sent to printer.\n"
	 "For more detail, print file legft.out.\n");
  return 0;
} /* legft */
