#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"
#include "gls.h"

float x00, dx, y00, dy;
char  gls_file_name[80];
FILE  *outfile;

/* ============================================================== */
int main(void)
{
  char ans[80], filnam[80];
  int output(void);
  int branch(void);
  int ex_diff(void);
  FILE *infile;

  /* Version 2.0  D.C. Radford  Sept 1999 */
  printf("\n\n\n"
	 "  GLS_conv Version 2.0    D. C. Radford   Sept 1999\n\n"
	 "     Welcome....\n\n");
  while (1) {
    printf(
      " Type 1 to create a level scheme figure in postscript\n"
      "        from a .gls file,\n"
      "      2 to output level and gamma data (.dat file) from a .gls file,\n"
      "      3 to convert a .gls file to an ASCII (.ags) file,\n"
      "      4 to convert an ASCII .ags file to a .gls file,\n"
      "      5 to calculate B(M1)/B(E2), B(E2)/B(E2) and B(E1)/B(E2) ratios,\n"
      "   or 6 to calculate E(band1) - E(band2) or E - RigidRotor\n"
      "           energy differences.\n\n");
    if (!cask(" ...Response = ?", ans, 1)) return 0;
    if (*ans == '1') {
      if (read_write_gls("OPEN_READ")) return 0;
      calc_band();
      calc_gls_fig();
      glsgd.x00 = glsgd.tx0;
      glsgd.dx = glsgd.thix - glsgd.tx0;
      glsgd.y00 = glsgd.ty0;
      glsgd.dy = glsgd.thiy - glsgd.ty0;
      gls_ps();
    } else if (*ans == '2') {
      if (read_write_gls("OPEN_READ")) return 0;
      cask("Output data file = ? (default .ext = .dat)", filnam, 80);
      setext(filnam, ".dat", 80);
      if ((outfile = open_new_file(filnam, 0))) {
	calc_band();
	calc_gls_fig();
	output();
      }
    } else if (*ans == '3') {
      if (read_write_gls("OPEN_READ")) return 0;
      if (glsgd.nlevels == 0) return 0;
      read_write_gls("WRITE_AGS");
    } else if (*ans == '4') {
      while (1) {
	cask("Input level scheme file = ? (default .ext = .ags)", filnam, 80);
	setext(filnam, ".ags", 80);
	if ((infile = open_readonly(filnam))) break;
      }
      read_ascii_gls(infile);
      if (glsgd.nlevels > 0) {
	strcpy(glsgd.gls_file_name, filnam);
	read_write_gls("WRITE");
      }
    } else if (*ans == '5') {
      branch();
    } else if (*ans == '6') {
      ex_diff();
    } else {
      continue;
    }
    return 0;
  }
} /* main */

/* ============================================================== */
int output(void)
{
  float a, ee, ie;
  int   i, j, k, k0, ln[MAXLEV], gn[MAXGAM], temp, flag, nc, lb, lk;
  char  l[120], *c;

  /* This subroutine writes the level scheme data to the file outfile in a
     user-friendly format. It performs three tests, two on the gamma-ray
     energies and the other on the gamma-ray intensities, and outputs the
     results. */

  if (caskyn("\nNormalise gamma intensities to <= 100? (Y/N)")) {
    a = 0.0;
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (a < glsgd.gam[j].i) a = glsgd.gam[j].i;
    }
    a /= 100.0f;
    for (j = 0; j < glsgd.ngammas; ++j) {
      glsgd.gam[j].i  /= a;
      glsgd.gam[j].di /= a;
    }
  }

  while ((nc = cask("\nThe Gamma-ray energy and level energy uncertainties\n"
		   "  will be multiplied by a constant factor, efactor.\n"
		   " ...efactor = ? (rtn for 1.0)", l, 80)) &&
	 ffin(l, nc, &ee, &a, &a));
  if (nc == 0 || ee <= 0.0f) ee = 1.0f;

  while ((nc = cask("\nThe Gamma-ray intensity  uncertainties\n"
		   "  will be multiplied by a constant factor, ifactor\n"
		   " ...ifactor = ? (rtn for 1.0)", l, 80)) &&
	 ffin(l, nc, &ie, &a, &a));
  if (nc == 0 || ie <= 0.0f) ie = 1.0f;

  for (j = 0; j < glsgd.nlevels; ++j) {
    glsgd.lev[j].de *= ee;
  }
  for (j = 0; j < glsgd.ngammas; ++j) {
    glsgd.gam[j].de *= ee;
    glsgd.gam[j].di *= ie;
  }

  if (caskyn("\nThe list of levels will be sorted by level energy,\n"
	     " or by band, sub-sorted by level energy.\n"
	     " ...Sort levels by band? (Y/N)")) {
    k = 0;
    for (i = 0; i < glsgd.nbands; ++i) {
      k0 = k;
      for  (j = 0; j < glsgd.nlevels; ++j) {
	if (glsgd.lev[j].band == i) ln[k++] = j;
      }
      if (k < k0 + 2) continue;
      /* bubble sort on level energies */
      flag = 1;
      while (flag) {
	flag = 0;
	for (j = k0; j < k - 1; ++j) {
	  if (glsgd.lev[ln[j]].e > glsgd.lev[ln[j+1]].e) {
	    temp = ln[j];
	    ln[j] = ln[j+1];
	    ln[j+1] = temp;
	    flag = 1;
	  }
	}
      }
    }
  } else {
    for (j = 0; j < glsgd.nlevels; ++j) ln[j] = j;
    /* bubble sort on level energies */
    flag = 1;
    while (flag) {
      flag = 0;
      for (j = 0; j < glsgd.nlevels - 1; ++j) {
	if (glsgd.lev[ln[j]].e > glsgd.lev[ln[j+1]].e) {
	  temp = ln[j];
	  ln[j] = ln[j+1];
	  ln[j+1] = temp;
	  flag = 1;
	}
      }
    }
  }

  if (caskyn("\nThe list of gammas will be sorted by gamma energy,\n"
	     " or by initial level, sorted as above.\n"
	     " ...Sort gammas by initial level? (Y/N)")) {
    k = 0;
    for (i = 0; i < glsgd.nlevels; ++i) {
      k0 = k;
      for  (j = 0; j < glsgd.ngammas; ++j) {
	if (glsgd.gam[j].li == ln[i]) gn[k++] = j;
      }
      if (k < k0 + 2) continue;
      /* bubble sort on gamma energies */
      flag = 1;
      while (flag) {
	flag = 0;
	for (j = k0; j < k - 1; ++j) {
	  if (glsgd.gam[gn[j]].e > glsgd.gam[gn[j+1]].e) {
	    temp = gn[j];
	    gn[j] = gn[j+1];
	    gn[j+1] = temp;
	    flag = 1;
	  }
	}
      }
    }
  } else {
    for (j = 0; j < glsgd.ngammas; ++j) gn[j] = j;
    /* bubble sort on gamma energies */
    flag = 1;
    while (flag) {
      flag = 0;
      for (j = 0; j < glsgd.ngammas - 1; ++j) {
	if (glsgd.gam[gn[j]].e > glsgd.gam[gn[j+1]].e) {
	  temp = gn[j];
	  gn[j] = gn[j+1];
	  gn[j+1] = temp;
	  flag = 1;
	}
      }
    }
  }

  if (caskyn("\nThe tables of levels and gammas can be in LaTex format,\n"
	     " or in standard text format.\n"
	     " ...Use LaTex format? (Y/N)")) {
    for (j = 0; j < glsgd.nlevels; ++j) {
      if ((c = strstr(glsgd.lev[ln[j]].sl, "{u"))) {
	*c = '^';
	*(c+1) = '{';
      }
      if ((c = strstr(glsgd.lev[ln[j]].sl, "{g"))) {
        for (; *(c+1)!=0; c++) *c = *(c+2);
	if ((c = strstr(glsgd.lev[ln[j]].sl, "}")))
          for (; *c!=0; c++) *c = *(c+1);
      }
    }
    lb = caskyn("\nList band names in table of levels? (Y/N)");
    lk = caskyn("List K-values in table of levels? (Y/N)");
    fprintf(outfile,
	    "List of levels:\n\n"
	    "\\begin{tabular}{|c|c|");
    if (lb) fprintf(outfile,"c|");
    if (lk) fprintf(outfile,"c|");
    fprintf(outfile,"}\n"
	    "\\hline \\hline\n"
	    " Level Energy &$ J^{\\pi} $");
    if (lb) fprintf(outfile,"& Band ");
    if (lk) fprintf(outfile,"&$ K $");
    fprintf(outfile, "\\\\\n\\hline\n");

    for (j = 0; j < glsgd.nlevels; ++j) {
      *l = '\0';
      wrresult(l, glsgd.lev[ln[j]].e, glsgd.lev[ln[j]].de, 14);
      fprintf(outfile, " %s &$ %10s $", l, glsgd.lev[ln[j]].sl);
      if (lb) fprintf(outfile,"& %.8s ", glsgd.lev[ln[j]].name);
      if (lk) fprintf(outfile,"& %5.1f", glsgd.lev[ln[j]].k);
      fprintf(outfile, "\\\\\n");
    }
    
    lb = caskyn("\nList band names in table of gammas? (Y/N)");
    lk = caskyn("List multipolarities in table of gammas? (Y/N)");
    fprintf(outfile,
	    "\\hline \\hline\n"
	    "\\end{tabular}\n\n"
	    "\f\n"
	    "   List of transitions:\n\n"
	    "\\begin{tabular}{|c|ccc|");
    if (lb) fprintf(outfile,"ccc|");
    if (lk) fprintf(outfile,"c|");
    fprintf(outfile,
	    "c|c|}\n"
	    "\\hline \\hline\n"
	    "     $E_i$   &");
    if (lb) fprintf(outfile,"$ Band_i $&$ \\rightarrow $&$ Band_f $&");
    fprintf(outfile,
	    "$ J^{\\pi}_i $&$ \\rightarrow $&$ J^{\\pi}_f $&"
	    "$ E_{\\gamma} $&$ I_{\\gamma} $");
    if (lk) fprintf(outfile,"& Mult. ");
    fprintf(outfile, "\\\\\n\\hline\n");
    for (j = 0; j < glsgd.ngammas; ++j) {
      *l = '\0';
      wrresult(l, glsgd.gam[gn[j]].e, glsgd.gam[gn[j]].de, 16);
      strcat(l, " &");
      wrresult(l, glsgd.gam[gn[j]].i, glsgd.gam[gn[j]].di, 31);
      fprintf(outfile,"%7.0f &", glsgd.lev[glsgd.gam[gn[j]].li].e);
      if (lb) fprintf(outfile,
		      "$ %.8s $&$ \\rightarrow $&$ %.8s $&",
		      glsgd.lev[glsgd.gam[gn[j]].li].name,
		      glsgd.lev[glsgd.gam[gn[j]].lf].name);
      fprintf(outfile,
	      "$ %10s $&$ \\rightarrow $&$ %10s $& %s ",
	      glsgd.lev[glsgd.gam[gn[j]].li].sl,
	      glsgd.lev[glsgd.gam[gn[j]].lf].sl, l);
      if (lk) fprintf(outfile, "& %c%.0f ",
		      glsgd.gam[gn[j]].em, glsgd.gam[gn[j]].n);
      fprintf(outfile, "\\\\\n");
    }
    fprintf(outfile,
	    "\\hline \\hline\n"
	    "\\end{tabular}\n\n");

  } else {
    fprintf(outfile,
	    "   List of levels:\n\n"
	    "     Level   Energy             Jpi    K\n"
	    "------------------------------------------\n");
    for (j = 0; j < glsgd.nlevels; ++j) {
      strcpy(l, glsgd.lev[ln[j]].name);
      strcat(l, "  ");
      wrresult(l, glsgd.lev[ln[j]].e, glsgd.lev[ln[j]].de, 26);
      if ((c = strstr(glsgd.lev[ln[j]].sl, "{u"))) for (; *(c+1)!=0; c++) *c = *(c+2);
      if ((c = strstr(glsgd.lev[ln[j]].sl, "{g"))) for (; *(c+1)!=0; c++) *c = *(c+2);
      while ((c = strstr(glsgd.lev[ln[j]].sl, "}"))) for (; *c!=0; c++) *c = *(c+1);
      fprintf(outfile, "%s %8s %5.1f\n",
	      l, glsgd.lev[ln[j]].sl, glsgd.lev[ln[j]].k);
    }
    
    fprintf(outfile,
	    "\f\n"
	    "   List of transitions:\n\n"
	    "     Ei    Jpi_i ->  Jpi_f   Energy          Intensity   "
	    " Alpha     Delta Mult\n"
	    "---------------------------------------------------------"
	    "---------------------\n");
    for (j = 0; j < glsgd.ngammas; ++j) {
      *l = '\0';
      wrresult(l, glsgd.gam[gn[j]].e, glsgd.gam[gn[j]].de, 16);
      wrresult(l, glsgd.gam[gn[j]].i, glsgd.gam[gn[j]].di, 29);
      fprintf(outfile,
	      "%7.0f %8s  %8s  %s %8.2e %6.2f  %c%.0f\n",
	      glsgd.lev[glsgd.gam[gn[j]].li].e,
	      glsgd.lev[glsgd.gam[gn[j]].li].sl,
	      glsgd.lev[glsgd.gam[gn[j]].lf].sl, l,
	      glsgd.gam[gn[j]].a, glsgd.gam[gn[j]].d,
	      glsgd.gam[gn[j]].em, glsgd.gam[gn[j]].n);
    }
  }

  /* test energy and intensity sums */
  fprintf(outfile, "\f\n");
  testene1(outfile);
  testene2(outfile);
  testint(outfile);
  return 0;
} /* output */

/* ============================================================== */
int branch(void)
{
  float a, r1, r2, r3;
  float brat, brat1, brat2, ebrat, ebrat1, ebrat2;
  float flamda, elamda, flamda1, elamda1, flamda2, elamda2;
  int   j, ig1, ig2, ili, ige1, ige2, igm1;
  char  outfile[80];
  FILE *file;

  if (read_write_gls("OPEN_READ")) return 0;
  calc_band();
  cask("Output data file = ? (default .ext = .dat)", outfile, 80);
  setext(outfile, ".dat", 80);
  if (!(file = open_new_file(outfile, 0))) return 0;

  if (caskyn("\nNormalise gamma intensities to <= 100? (Y/N)")) {
    a = 0.0;
    for (j = 0; j < glsgd.ngammas; ++j) {
      if (a < glsgd.gam[j].i) a = glsgd.gam[j].i;
    }
    a /= 100.0f;
    for (j = 0; j < glsgd.ngammas; ++j) {
      glsgd.gam[j].i  /= a;
      glsgd.gam[j].di /= a;
    }
  }
  
  /* calculate B(M1)/B(E2)'s */
  fprintf(file,
	  "* BM1/BE2 in units (mu_N/eb)-squared\n"
	  "*  Initial      Final  Egamma  Igamma   Err"
	  "  Delta    Lamda  Err  BM1/BE2   Err\n");
  for (ili = 0; ili < glsgd.nlevels; ++ili) {
    for (ige2 = 0; ige2 < glsgd.ngammas; ++ige2) {
      if (glsgd.gam[ige2].i > 0.0f &&
	  glsgd.gam[ige2].li == ili && 
	  glsgd.gam[ige2].em == 'E' &&
	  (int) (0.5f + glsgd.gam[ige2].n) == 2) {
	for (igm1 = 0; igm1 < glsgd.ngammas; ++igm1) {
	  if (glsgd.gam[igm1].i > 0.0f &&
	      glsgd.gam[igm1].li == ili && 
	      glsgd.gam[igm1].em == 'M' &&
	      (int) (0.5f + glsgd.gam[igm1].n) == 1) {
	    flamda = glsgd.gam[ige2].i / glsgd.gam[igm1].i;
	    r1 = glsgd.gam[ige2].di / glsgd.gam[ige2].i;
	    r2 = glsgd.gam[igm1].di / glsgd.gam[igm1].i;
	    elamda = flamda * sqrt(r1 * r1 + r2 * r2);
	    r1 = glsgd.gam[ige2].e / 1e3f; r2 = r1 * r1;
	    r3 = glsgd.gam[igm1].e / 1e3f;
	    brat = r2 * r2 * r1 * 0.693f / (r3 * r3 * r3) /
	      flamda / (glsgd.gam[igm1].d * glsgd.gam[igm1].d + 1.0f);
	    ebrat = brat * elamda / flamda;
	    fprintf(file,
		    "%s %s %7.1f %7.2f %5.2f %6.2f\n"
		    "%s %s %7.1f %7.2f %5.2f %6.2f"
		    " %7.3f %5.3f %7.3f %6.3f\n",
		    glsgd.lev[ili].name,
		    glsgd.lev[glsgd.gam[ige2].lf].name, 
		    glsgd.gam[ige2].e, glsgd.gam[ige2].i,
		    glsgd.gam[ige2].di, glsgd.gam[ige2].d,
		    "          ",
		    glsgd.lev[glsgd.gam[igm1].lf].name,
		    glsgd.gam[igm1].e, glsgd.gam[igm1].i,
		    glsgd.gam[igm1].di, glsgd.gam[igm1].d,
		    flamda, elamda, brat, ebrat);
	  }
	}
      }
    }
  }

  /* calculate B(E2)/B(E2)'s */
  fprintf(file,
	  "\n\n\n* BE2/BE2\n"
	  "*   Initial      Final  Egamma   Igamma   Err"
	  "     Lamda    Err   BE2/BE2    Err\n");
  for (ili = 0; ili < glsgd.nlevels; ++ili) {
    for (ig1 = 0; ig1 < glsgd.ngammas - 1; ++ig1) {
      if (glsgd.gam[ig1].i > 0.0f &&
	  glsgd.gam[ig1].li == ili && 
	  glsgd.gam[ig1].em == 'E' &&
	  (int) (0.5f + glsgd.gam[ig1].n) == 2) {
	for (ig2 = ig1 + 1; ig2 < glsgd.ngammas; ++ig2) {
	  if (glsgd.gam[ig2].i > 0.0f &&
	      glsgd.gam[ig2].li == ili && 
	      glsgd.gam[ig2].em == 'E' && 
	      (int) (0.5f + glsgd.gam[ig2].n) == 2) {
	    flamda1 = glsgd.gam[ig1].i / glsgd.gam[ig2].i;
	    r1 = glsgd.gam[ig1].di / glsgd.gam[ig1].i;
	    r2 = glsgd.gam[ig2].di / glsgd.gam[ig2].i;
	    elamda1 = flamda1 * sqrt(r1 * r1 + r2 * r2);
	    flamda2 = 1.0f / flamda1;
	    elamda2 = flamda2 * elamda1 / flamda1;
	    r1 = glsgd.gam[ig2].e / glsgd.gam[ig1].e; r2 = r1 * r1;
	    brat1 = r2 * r2 * r1 * flamda1;
	    ebrat1 = brat1 * elamda1 / flamda1;
	    brat2 = 1.0f / brat1;
	    ebrat2 = brat2 * ebrat1 / brat1;
	    fprintf(file,
		    " %s %s %7.1f %8.2f %5.2f %9.3f %6.3f %9.3f %6.3f\n"
		    " %s %s %7.1f %8.2f %5.2f %9.3f %6.3f %9.3f %6.3f\n",
		    glsgd.lev[ili].name,
		    glsgd.lev[glsgd.gam[ig1].lf].name, 
		    glsgd.gam[ig1].e, glsgd.gam[ig1].i, glsgd.gam[ig1].di,
		    flamda1, elamda1, brat1, ebrat1,
		    "          ",
		    glsgd.lev[glsgd.gam[ig2].lf].name, 
		    glsgd.gam[ig2].e, glsgd.gam[ig2].i, glsgd.gam[ig2].di,
		    flamda2, elamda2, brat2, ebrat2);
	  }
	}
      }
    }
  }

  /* calculate B(E1)/B(E2)'s */
  fprintf(file,
	  "\n\n\n* BE1/BE2 in units (10^-9 fm^-2)\n"
	  "*   Initial      Final  Egamma   Igamma   Err"
	  "     Lamda    Err   BE1/BE2    Err\n");
  for (ili = 0; ili < glsgd.nlevels; ++ili) {
    for (ige2 = 0; ige2 < glsgd.ngammas - 1; ++ige2) {
      if (glsgd.gam[ige2].i > 0.0f &&
	  glsgd.gam[ige2].li == ili && 
	  glsgd.gam[ige2].em == 'E' &&
	  (int) (0.5f + glsgd.gam[ige2].n) == 2) {
	for (ige1 = 0; ige1 < glsgd.ngammas; ++ige1) {
	  if (glsgd.gam[ige1].i > 0.0f &&
	      glsgd.gam[ige1].li == ili && 
	      glsgd.gam[ige1].em == 'E' &&
	      (int) (0.5f + glsgd.gam[ige1].n) == 1) {
	    flamda = glsgd.gam[ige2].i / glsgd.gam[ige1].i;
	    r1 = glsgd.gam[ige2].di / glsgd.gam[ige2].i;
	    r2 = glsgd.gam[ige1].di / glsgd.gam[ige1].i;
	    elamda = flamda * sqrt(r1 * r1 + r2 * r2);
	    r1 = glsgd.gam[ige2].e / 1e3f; r2 = r1 * r1;
	    r3 = glsgd.gam[ige1].e / 1e3f;
	    brat = r2 * r2 * r1 * 767.0f / (r3 * r3 * r3) / flamda;
	    ebrat = brat * elamda / flamda;
	    fprintf(file,
		    " %s %s %7.1f %8.2f %5.2f\n"
		    " %s %s %7.1f %8.2f %5.2f %9.3f %6.3f %9.2f %6.2f\n",
		    glsgd.lev[ili].name,
		    glsgd.lev[glsgd.gam[ige2].lf].name, 
		    glsgd.gam[ige2].e, glsgd.gam[ige2].i, glsgd.gam[ige2].di,
		    "          ",
		    glsgd.lev[glsgd.gam[ige1].lf].name, 
		    glsgd.gam[ige1].e,glsgd.gam[ige1].i, glsgd.gam[ige1].di,
		    flamda, elamda, brat, ebrat);
	  }
	}
      }
    }
  }
  fclose(file);
  return 0;
} /* branch */

/* ======================================================================= */
int ex_diff(void)
{
  float e_rr, e_diff, de_diff;
  int   j, k, nc;
  char  ans[80], ini_lev[16], fin_lev[16], outfile[80];
  FILE *file;

  /* this program reads a standard .gls level scheme file as input */
  /* it produces an output file of differences in excitation energy */
  /* between two different bands, or between bands and rigid rotor energies */

  if (read_write_gls("OPEN_READ")) return 0;
  nc = cask("Output data file = ? (default .ext = .dat)", outfile, 80);
  setext(outfile, ".dat", 80);
  if (!(file = open_new_file(outfile, 0))) return 0;

  /* output all level and gamma information */
  while ((nc = cask("Initial band = ? (* for all, rtn to end)", ans, 80))) {
    if (nc > 8) continue;
    strcpy(ini_lev, "        ");
    strcpy(ini_lev + 8 - nc, ans);
    while (1) {
      if (!(nc = cask("Final band = ? (RR for rigid rotor)", ans, 80)))
	return 0;
      if (nc <= 8) break;
    }
    strcpy(fin_lev, "        ");
    strcpy(fin_lev + 8 - nc, ans);
    fprintf(file, "*Spin   DeltaEx +- err     InitialLev ---> FinalLev\n");
    for (j = 0; j < glsgd.nlevels; ++j) {
      if (!strncmp(ini_lev, "       *", 8) ||
	  !strncmp(ini_lev, glsgd.lev[j].name,8)) {
	if (!strncmp(ini_lev, "       *", 8) && j > 0 &&
	    strncmp(glsgd.lev[j].name, glsgd.lev[j - 1].name, 8))
	  fprintf(file, "\n");
	if (!strncmp(fin_lev, "      RR", 8) ||
	    !strncmp(fin_lev, "      rr", 8)) {
	  e_rr = glsgd.lev[j].j * 7.3f * (glsgd.lev[j].j + 1.0f);
	  e_diff = glsgd.lev[j].e - e_rr;
	  fprintf(file, "%5.1f  %8.2f%8.2f     %s ---> %s\n",
		  glsgd.lev[j].j, e_diff, glsgd.lev[j].de,
		  glsgd.lev[j].name, "RR");
	} else {
	  for (k = 0; k < glsgd.nlevels; ++k) {
	    if (!strncmp(fin_lev, glsgd.lev[k].name, 8) &&
		glsgd.lev[j].j == glsgd.lev[k].j) {
	      e_diff = glsgd.lev[j].e - glsgd.lev[k].e;
	      de_diff = sqrt(glsgd.lev[j].de * glsgd.lev[j].de + 
			     glsgd.lev[k].de * glsgd.lev[k].de);
	      fprintf(file, "%5.1f  %8.2f%8.2f     %s ---> %s\n",
		      glsgd.lev[j].j, e_diff, de_diff, 
		      glsgd.lev[j].name, glsgd.lev[k].name);
	    }
	  }
	}
      }
    }
    fprintf(file, "\n");
  }
  fclose(file);
  return 0;
} /* ex_diff */

/* ======================================================================= */
int gls_get_ps_details(char *title, float *htitle, char *titpos,
		       float *hyaxis, char *scalepos,
		       char *filnam, char *printcmd)
{
  float fj1, fj2;
  int   nc, jext;
  char  ans[80];

  *title = '\0';
  if (askyn("Add title? (Y/N)")) {
    while ((nc = ask(ans, 80, "Enter character height (rtn for 200) ?")) &&
	   ffin(ans, nc, htitle, &fj1, &fj2));
    if (!nc) *htitle = 200.0f;
    while (1) {
      ask(ans, 80, "Put title at Top(T) or Bottom(B) ?");
      if (*ans == 'T' || *ans == 't' || *ans == 'B' || *ans == 'b') break;
    }
    *titpos = *ans;
    if (!ask(title, 80,
	     "Default title is: From %.72s\n"
	     " Use {u...} for superscripts...\n"
	     "...Enter title (max. 80 chars, rtn for default) ?",
	     glsgd.gls_file_name)) {
      strcpy(title, "From ");
      strncat(title, glsgd.gls_file_name, 72);
    }
  }

  *scalepos = '\0';
  if (askyn("Add y-axis (excitation energy in MeV)? (Y/N)")) {
    while ((nc = ask(ans, 80, "Enter character height (rtn for 200) ?")) &&
	   ffin(ans, nc, hyaxis, &fj1, &fj2));
    if (!nc) *hyaxis = 200.0f;
    while (1) {
      ask(ans, 80, "Put y-axis at Left(L) or Right(R) ?");
      if (*ans == 'L' || *ans == 'l' || *ans == 'R' || *ans == 'r') break;
    }
    *scalepos = *ans;
  }

  /* open output file */
  strcpy(filnam, glsgd.gls_file_name);
  jext = setext(filnam, ".ps", 80);
  filnam[jext] = '\0';
  jext = setext(filnam, ".ps", 80);
  nc = askfn(ans, 80, filnam, ".ps", "Output postscript file = ?\n");
  strcpy(filnam, ans);

  *printcmd = '\0';
  if (askyn("Print and delete the postscript file now? (Y/N)\n"
	    "                (If No, the file will be saved)")) *printcmd = '1';
  return 0;
} /* gls_get_ps_details */
