/* ***********************************
   program slice

   program to slice and/or project 4kx4k matrices
   author:           D.C. Radford
   first written:    Oct  1986
   latest revision:  May  2000
   description:      Prompt for the filename of a 4k x 4k matrix.
                     Ask if matrix is to be sliced and/or projected.
                     Ask for new matrix filename; exit if response is E.

   *********************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

float bgspec[4096];
union {
  short matseg[16][4096];
  int   lmatseg[16][4096];
} mat;
int   proj[2][4096], longmat = 0;
int   inam, xsli, ysli, ssli, addsli, subbg, wr_err_sp, iblo, ibhi;
char  matnam[80], slinam[80], winnam[80];
FILE  *matfile, *slifile, *logfile;


int opnwin(void), dosli(void);

/* ======================================================================= */
int main(int argc, char **argv)
{
  float spec[4096];
  int   idlo, i, j, iy;
  char  filnam[80], pronam[80], line[256];

  strcpy(matnam, "mat1.mat");
  printf(" SLICE - a program to slice and/or project 4k by 4k\n"
	 "    gamma-gamma matrices.\n\n"
	 " This program now accepts matrices in both .mat (2 Bytes/ch)\n"
	 "    and .spn or .m4b (4 Bytes/ch) formats.\n"
	 " Specify .ext = .spn or .m4b for 4 Bytes/ch.\n\n"
	 "    The default extension for all spectrum files is .spe\n\n"
	 "    D. C. Radford    May 2000\n\n");

  /* open matrix file */
  while (1) {
    if (argc > 1) {
      strncpy(filnam, argv[1], 70);
      argc = 0;
    } else {
      sprintf(line,
	      "Matrix file name = ? (return for %s,\n"
	      "                      E to end,\n"
	      "                      default .ext = .mat)", matnam);
      if (!cask(line, filnam, 80)) strcpy(filnam, matnam);
      if (!strncmp(filnam, "E", 80) || !strncmp(filnam, "e", 80)) return 0;
    }

    j = setext(filnam, ".mat", 80);
    longmat = 0;
    if (!strcmp(filnam + j, ".spn") || !strcmp(filnam + j, ".SPN") ||
	!strcmp(filnam + j, ".m4b") || !strcmp(filnam + j, ".M4B"))
      longmat = 1;
    if (!(matfile = open_readonly(filnam))) continue;
    strcpy(matnam, filnam);

    /* ask if slices required
       if so, open window and log files
       then slice matrix and write slice spectra to disk */
    opnwin();

    /* ask if projections required */
    if (caskyn("Take projections? (Y/N)")) {
      for (i = 0; i < 4096; ++i) {
	proj[0][i] = 0;
	proj[1][i] = 0;
      }
      /* read matrix and compute projections */
      printf("Working...\n\n");
      for (idlo = 0; idlo < 4096; idlo += 16) {
	printf("Row %4i\r", idlo); fflush(stdout);
	if (longmat) {
	  rmat4b(matfile, idlo, 16, mat.lmatseg[0]);
	  for (i = 0; i < 16; ++i) {
	    iy = idlo + i;
	    for (j = 0; j < 4096; ++j) {
	      proj[0][j]  += mat.lmatseg[i][j];
	      proj[1][iy] += mat.lmatseg[i][j];
	    }
	  }
	} else {
	  rmat(matfile, idlo, 16, mat.matseg[0]);
	  for (i = 0; i < 16; ++i) {
	    iy = idlo + i;
	    for (j = 0; j < 4096; ++j) {
	      proj[0][j]  += mat.matseg[i][j];
	      proj[1][iy] += mat.matseg[i][j];
	    }
	  }
	}
      }

      /* write out projections */
      cask(" X-Projection spectrum file name = ?", pronam, 80);
      for (i = 0; i < 4096; ++i)
	spec[i] = (float) proj[0][i];
      wspec(pronam, spec, 4096);

      cask(" Y-Projection spectrum file name = ?", pronam, 80);
      for (i = 0; i < 4096; ++i)
	spec[i] = (float) proj[1][i];
      wspec(pronam, spec, 4096);
    }
    /* close matrix file and ask for next one */
    fclose(matfile);
  }
} /* main */

/* ======================================================================= */
int opnwin(void)
{
  int  j, k, j2, nc, nch;
  char bgnam[80], lognam[80], namesp[8], ans[80], xys[80], line[120];

  /* Ask if slices required.
     If so, open window and log files,
     get background subtraction, then call dosli. */

  xsli = 0;
  ysli = 0;
  ssli = 0;

  while (1) {
    if (!caskyn("Take slices ? (Y/N)")) return 0;

    cask("Window file name = ? (default .ext = .win)", winnam, 80);
    setext(winnam, ".win",80);
    if ((slifile = open_readonly(winnam))) break;
  }
  /* ask for direction of slices */
  while (1) {
    cask("Take slices projected onto X,Y or Sum ? (X/Y/S)", xys, 1);
    xys[1] = '\0';
    if (*xys == 'X' || *xys == 'x') {
      xsli = 1;
      break;
    } else if (*xys == 'Y' || *xys == 'y') {
      ysli = 1;
      break;
    } else if (*xys == 'S' || *xys == 's') {
      ssli = 1;
      break;
    }
  }
  /* ask if slices are to be added all together */
  addsli = caskyn("Would you like to add all these slices together\n"
		  "            to form one spectrum ? (Y/N)\n"
		  "WARNING -- if yes, you will NOT get the individual\n"
		  "             spectra, but only the summed spectrum.");
  if (addsli) {
    /* slices are to be added */
    /* ask for total spectrum file name */
    cask("Total spectrum output file name = ?", slinam, 80);
    j = setext(slinam, ".spe", 80);
    strcpy(lognam, slinam);
    strcpy(lognam+j, ".log");
    inam = j - 3;
  } else {
    /* slices are to be written individually */
    /* ask for invariant piece of slice file name */
    sprintf(line,
	    "Output file names will be  NNN%s001.spe etc...\n"
	    "                        ...NNN = ?", xys);
    k = cask(line, slinam, 60);
    strcat(slinam, xys);
    strcpy(lognam, slinam);
    strcat(slinam, "001.spe");
    strcat(lognam, "sli.log");
    inam = k + 1;
  }
  printf("Log file name = %s\n\n", lognam);

  /* ask if background to be subtracted */
  while (1) {
    wr_err_sp = 0;
    if (!(subbg = caskyn("Would you like to subtract background? (Y/N)")))
      break;
    /* ask for background spectrum file name */
    /* try to read spectrum */
    cask("Background spectrum file name = ?", bgnam, 80);
    if (readsp(bgnam, bgspec, namesp, &nch, 4096)) continue;
    if (nch == 4096) break;
    printf("ERROR - background spectrum has wrong length!\n");
  }

  iblo = 0;
  ibhi = 4095;
  while (subbg && !ssli) {
    iblo = 0;
    ibhi = 4095;
    nc = cask("LO,HI channels for background normalization = ?\n"
	      "                            (return for 0, 4095)", ans, 80);
    if (nc == 0) break;
    if (inin(ans, nc, &iblo, &ibhi, &j2)) continue;
    if (iblo == 0 && ibhi == 0) ibhi = 4095;
    if (iblo < 0) iblo = 0;
    if (ibhi > 4095)  ibhi = 4095;
    if (iblo > ibhi) continue;
    printf(" Limits are %i to %i.\n", iblo, ibhi);
    break;
  }
  if (subbg)
    wr_err_sp = caskyn("The program can write out error spectrum files\n"
		       "   with the extension .err...\n"
		       "   Would you like to have these files written? (Y/N)");

  /* open slice log file */
  logfile = open_new_file(lognam, 1);
  fprintf(logfile,
	  "Program SLICE log file\n"
	  "   Matrix file name = %s\n"
	  "   Window file name = %s\n", matnam, winnam);
  if (ssli) {
    fprintf(logfile,"    Summing slices on both X- and Y-axes.\n");
  } else {
    fprintf(logfile,"    Slices projected onto %s-axis.\n", xys);
  }
  if (subbg) {
    fprintf(logfile,"    Background spectrum %s subtracted.\n", bgnam);
  } else {
    fprintf(logfile,"    No background subtracted.\n");
  }
  dosli();
  return 0;
} /* opnwin */

/* ======================================================================= */
int dosli(void)
{
  float spec[4096], spec2[4096], ptot[32], egamma[32];
  float fact1, fact2, bgsum = 0.0f, bgcnts = 0.0f;
  int   idhi, idlo, nchs, ishi[32], islo[32];
  int   i, j, nslice, is1, ihi, ilo, is, iy;
  int   ssum[32], islice[32][4096];
  char  errnam[80], line[120];

  /* Read window file and compute slices.
     Subtract bkgnd and write slice spectra to disk.
     if slices to be added, initialize sum of slice spectra
     if bkgnd to be subtracted, calculate sum of bkgnd spectrum */

  printf("Working...\n\n");
  if (addsli) {
    for (i = 0; i < 4096; ++i) {
      proj[0][i] = 0;
    }
  }
  if (subbg) {
    for (i = iblo; i <= ibhi; ++i) {
      bgsum += bgspec[i];
    }
  }

  /* read slice limits etc. from window file */
  is1 = 0;
 GETGATES:
  for (is = 0; is < 32; ++is) {
    if (!fgets(line, 120, slifile)) break;
    if (sscanf(line, "%*5c%d%*3c%d%*8c%f%*11c%f",
	       &islo[is], &ishi[is], &ptot[is], &egamma[is]) != 4) {
      file_error("read", winnam);
      /* close files and return */
      fclose(slifile);
      fclose(logfile);
      return 1;
    }
    if (islo[is] < 0) islo[is] = 0;
    if (ishi[is] > 4095) ishi[is] = 4095;
    if (islo[is] > ishi[is]) {
      i = ishi[is];
      ishi[is] = islo[is];
      islo[is] = i;
    }
    if (islo[is] < 0) islo[is] = 0;
    if (ishi[is] > 4095) ishi[is] = 4095;
    for (i = 0; i < 4096; ++i) {
      islice[is][i] = 0;
    }
  }

  nslice = is;
  if (nslice == 0 && (is1 == 0 || !addsli)) {
    fclose(slifile);
    fclose(logfile);
    return 0;
  }

  /* read matrix and compute slices */
  if (xsli && nslice != 0) {
    /* X slices only - read only relevant rows of matrix */
    for (is = 0; is < nslice; ++is) {
      for (ilo = islo[is]; ilo <= ishi[is]; ilo += nchs) {
	nchs = ishi[is] - ilo + 1;
	if (nchs > 16) nchs = 16;
	if (longmat) {
	  rmat4b(matfile, ilo, nchs, mat.lmatseg[0]);
	  for (i = 0; i < nchs; ++i) {
	    for (j = 0; j < 4096; ++j) {
	      islice[is][j] += mat.lmatseg[i][j];
	    }
	  }
	} else {
	  rmat(matfile, ilo, nchs, mat.matseg[0]);
	  for (i = 0; i < nchs; ++i) {
	    for (j = 0; j < 4096; ++j) {
	      islice[is][j] += mat.matseg[i][j];
	    }
	  }
	}
      }
    }
  } else if (nslice != 0) {
    /* Y or SUM slices - read complete matrix */
    for (idlo = 0; idlo <= 4080; idlo += 16) {
      idhi = idlo + 15;
      /* Y slice */
      if (longmat) {
	rmat4b(matfile, idlo, 16, mat.lmatseg[0]);
	for (is = 0; is < nslice; ++is) {
	  for (i = 0; i < 16; ++i) {
	    iy = idlo + i;
	    for (j = islo[is]; j <= ishi[is]; ++j) {
	      islice[is][iy] += mat.lmatseg[i][j];
	    }
	  }
	}
      } else {
	rmat(matfile, idlo, 16, mat.matseg[0]);
	for (is = 0; is < nslice; ++is) {
	  for (i = 0; i < 16; ++i) {
	    iy = idlo + i;
	    for (j = islo[is]; j <= ishi[is]; ++j) {
	      islice[is][iy] += mat.matseg[i][j];
	    }
	  }
	}
      }
      if (ssli) {
	/* add X slice */
	for (is = 0; is < nslice; ++is) {
	  if (islo[is] <= idhi && ishi[is] >= idlo) {
	    ilo = idlo;
	    if (ilo < islo[is]) ilo = islo[is];
	    ihi = idhi;
	    if (ihi > ishi[is]) ihi = ishi[is];
	    for (i = ilo - idlo; i <= ihi - idlo; ++i) {
	      if (longmat) {
		for (j = 0; j < 4096; ++j) {
		  islice[is][j] += mat.lmatseg[i][j];
		}
	      } else {
		for (j = 0; j < 4096; ++j) {
		  islice[is][j] += mat.matseg[i][j];
		}
	      }
	    }
	  }
	}
      }
    }
  }
  /* if slices to be added, sum slice spectra
     if bkgnd to be subtracted, calculate sums of slice spectra
     and number of counts to be subtracted */
  if (addsli) {
    if (nslice != 0) {
      if (subbg) {
	for (is = 0; is < nslice; ++is) {
	  ssum[is] = 0.0f;
	  for (i = iblo; i <= ibhi; ++i) {
	    ssum[is] += islice[is][i];
	  }
	  bgcnts += (float) ssum[is] * (1.0f - ptot[is]);
	}
      }
      for (is = 0; is < nslice; ++is) {
	for (i = 0; i < 4096; ++i) {
	  proj[0][i] += islice[is][i];
	}
	/* enter slices in log file */
	fprintf(logfile,
		" Added slice for chs %4i to %4i,    Energy = %9.3f\n",
		islo[is], ishi[is], egamma[is]);
      }
      /* if there are more slices to get, go back  */
      is1 += 32;
      if (nslice == 32) goto GETGATES;
    }
    /* if bkgnd to be subtracted, calculate subtracted spectrum
       and error spectrum */
    if (subbg) {
      fact1 = bgcnts / bgsum;
      fact2 = fact1 * fact1;
      if (wr_err_sp) {
	for (i = 0; i < 4096; ++i) {
	  /* error spectrum */
	  spec2[i] = (float) proj[0][i] + fact2 * bgspec[i];
	  /* subtracted spectrum */
	  spec[i] = (float) proj[0][i] - fact1 * bgspec[i];
	}
      } else {
	for (i = 0; i < 4096; ++i) {
	  /* subtracted spectrum */
	  spec[i] = (float) proj[0][i] - fact1 * bgspec[i];
	}
      }
    } else {
      /* if bkgnd NOT to be subtracted, calculate total spectrum*/
      for (i = 0; i < 4096; ++i) {
	/* summed spectrum */
	spec[i] = (float) proj[0][i];
      }
    }
    /* write out spectra to disk files */
    /* enter file names in log file */
    wspec(slinam, spec, 4096);
    fprintf(logfile,"\n    Sum Spectrum File = %s\n", slinam);
    if (wr_err_sp) {
      strcpy(errnam, slinam);
      strcpy(errnam + inam + 3, ".err");
      wspec(errnam, spec2, 4096);
      fprintf(logfile,"    Error Spectrum File = %s\n", errnam);
    }
  } else {
    /* slices to be output individually */
    /* write out slices to disk files */
    /* first encode required file name */
    for (is = 0; is < nslice; ++is) {
      sprintf(slinam + inam, "%03i", is + is1 + 1);
      /* if bkgnd to be subtracted, calculate subtracted spectrum
	 and error spectrum */
      if (subbg) {
	ssum[is] = 0.0f;
	for (i = iblo; i <= ibhi; ++i) {
	  ssum[is] += islice[is][i];
	}
	bgcnts = (float) ssum[is] * (1.0f - ptot[is]);
	fact1 = bgcnts / bgsum;
	fact2 = fact1 * fact1;
	if (wr_err_sp) {
	  for (i = 0; i < 4096; ++i) {
	    /* subtracted spectrum */
	    spec[i] = (float) islice[is][i] - fact1 * bgspec[i];
	    /* error spectrum */
	    spec2[i] = (float) islice[is][i] + fact2 * bgspec[i];
	  }
	} else {
	  for (i = 0; i < 4096; ++i) {
	    /* subtracted spectrum */
	    spec[i] = (float) islice[is][i] - fact1 * bgspec[i];
	  }
	}
      } else {
	/* if bkgnd NOT to be subtracted, calculate standard spectrum*/
	for (i = 0; i < 4096; ++i) {
	  /* standard spectrum */
	  spec[i] = (float) islice[is][i];
	}
      }
      /* write out spectra to disk files */
      /* enter file names in log file */
      wspec(slinam, spec, 4096);
      fprintf(logfile,
	      "    Output File = %20s -- chs %4i to %4i    Energy = %8.3f\n",
	      slinam, islo[is], ishi[is], egamma[is]);
      if (wr_err_sp) {
	strcpy(errnam, slinam);
	strcpy(errnam + inam + 3, ".err");
	wspec(errnam, spec2, 4096);
	fprintf(logfile,"    Error Spectrum File = %s\n", errnam);
      }
    }
    /* if there are more slices to get, go back */
    is1 += 32;
    if (nslice == 32) goto GETGATES;
  }
  /* close files and return */
  fclose(slifile);
  fclose(logfile);
  return 0;
} /* dosli */
