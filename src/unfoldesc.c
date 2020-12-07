#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "util.h"

int adjgain(float oldch1, float oldch2, float newch1, float newch2,
	    float *oldsp, float *newsp, int nco, int ncn, float fact);
int calres(int ch, float *spout, int numchs);
float channo(float egamma);

#define  MAXCHS 4096    /* max number of channels per dimension */

double gain[6];
float  resp[25][514], thresh;
int    nterms;
#define DATA(x,y) data[(x > y ? luch[y] + x : luch[x] + y)]

int main(int argc, char **argv)
{
  int    numchs;
  int    esc_file_flags[10];
  float  spec[MAXCHS], rdata[MAXCHS];
  int    reclen;
  FILE   *file, *file2, *file3;

  float save, c, *data, area, fact, fj1, fj2;
  int   iw, ix, iy, jext, nc, luch[MAXCHS], proj[MAXCHS];
  int   ich, k, i0, isum, i;
  char  fn[80], filenm[80], dattim[20], ans[80], buf[51404];
  char  *infile1 = "", *infile2 = "";
  static char head[20] = "RESPONSE FILE VER. 1";

  printf("\n\n\n"
	 "  unfoldesc Version 1.0    D. C. Radford   Nov 1999\n\n"
	 "     Welcome....\n\n");

  /* initialise */
  if (argc > 1) infile1 = argv[1];
  if (argc > 2) infile2 = argv[2];

  /* esc_file_flags[0]   : 0/1 for matrix compressed / not compressed
     esc_file_flags[1]   : 1 for new versions with systematic errors
                              on background and energy, effic calibs
     esc_file_flags[2]   : 0 for standard type file,
                           1 for file with extra data on statistical errors
                             (allows use on difference-matrices,
                              2d-unfolded matrices etc.)
     esc_file_flags[3-9] : for future use */

  /* ask for and open escl8r data file */
  if ((nc = strlen(infile1)) > 0) {
    strcpy(filenm, infile1);
  } else {
  START:
    nc = cask("Escl8r data file = ? (default .ext = .esc)",
	      filenm, 80);
  }
  jext = setext(filenm, ".esc", 80);
  if (!strcmp(filenm + jext, ".e4k") ||
      !strcmp(filenm + jext, ".E4K")) {
    numchs = 4096;
  } else {
    numchs = 2048;
  }
  if (!(data = malloc(numchs * (numchs+1) * 2))) {
    fprintf(stderr,"\007  ERROR: Could not malloc data area (%d bytes).\n",
	    (numchs * (numchs+1) * 2));
    fprintf(stdout,"terminating...\n");
    return -1;
  }
  reclen = numchs*4;
  if (!(file = fopen(filenm, "r+"))) {
    file_error("open", filenm);
    goto START;
  }

  fseek(file, (numchs + 6)*reclen, SEEK_SET);
  fread(gain, 8, 6, file);
  fread(&nterms, 4, 1, file);
  if (nterms > 6 || nterms < 2) {
    fclose(file);
    printf("*** ERROR -- File %s\n"
	   "     seems to have the wrong byte ordering.\n"
	   "    -- Perhaps you need to run unix2unix,\n"
	   "       vms2unix or unix2vms on it.\n", filenm);
    goto START;
  }
  fseek(file, (numchs + 6)*reclen + 124, SEEK_SET);
  fread(esc_file_flags, 4, 10, file);

  /* read 2D data */
  luch[0] = 0;
  for (iy = 1; iy < numchs; ++iy) {
    luch[iy] = luch[iy-1] + numchs - iy;
  }
  printf("\nReading 2D data...\n");
  if (!esc_file_flags[2])
    printf(" and copying it for the uncertainties....\n");
  for (iy = 0; iy < numchs; ++iy) {
    if (iy%10 == 0) {
      printf("\rRow %i", iy);
      fflush(stdout);
    }
    fseek(file, iy*reclen, SEEK_SET);
    fread(rdata, 4, numchs, file);
    if (!esc_file_flags[2]) {
      fseek(file, (iy + numchs + 7)*reclen, SEEK_SET);
      fwrite(rdata, 4, numchs, file);
    }
    for (ix = 0; ix <= iy; ++ix) {
      ich = luch[ix] + iy;
      data[ich] = rdata[ix];
    }
  }
  fseek(file, numchs*reclen, SEEK_SET);
  fread(proj, 4, numchs, file);
  printf("    ...done.\n");

  /* open .esc_log file for append */
  strcpy(filenm + jext + 4, "_log");
  if (!(file3 = fopen(filenm, "a+"))) {
    file_error("open", filenm);
    return -1;
  }

  /* correct for summing effects */
  while (caskyn("Correct for summing effects? (Y/N)")) {
    while ((k = cask("Coinc. rate/singles rate\n"
		     "    (for a single detector) = ?", ans, 80)) &&
	   ffin(ans, k, &fact, &fj1, &fj2));
    if (k == 0) continue;
    save = fact;
    if (gain[0] < 0.0) {
      i0 = (int) (gain[0] - 0.5);
    } else {
      i0 = (int) (gain[0] + 0.5);
    }
    area = 0.f;
    for (ix = 0; ix <= numchs; ++ix) {
      area += proj[ix];
    }
    fact /= (area * area);

    printf("\nWorking...\n");
    for (iy = 0; iy < numchs; ++iy) {
      printf("\rRow %i", iy);
      fflush(stdout);
      for (ix = 0; ix <= iy; ++ix) {
	if ((isum = ix + iy - i0) < 0) continue;
	if (isum >= numchs) break;
	c = DATA(ix,iy) * fact;
	if (ix == iy) c /= 2.0f;
	for (iw = 0; iw < isum; ++iw) {
	  ich = luch[iw] + isum;
	  data[ich] -= (DATA(ix,iw) + DATA(iy,iw)) * c;
	}
	for (iw = isum; iw < numchs; ++iw) {
	  ich = luch[isum] + iw;
	  data[ich] -= (DATA(ix,iw) + DATA(iy,iw)) * c;
	}
      }
    }
    printf("    ...done.\n");
    datetime(dattim);
    fprintf(file3,
	    "%s: program unfoldesc:\n"
	    "   Corrected for summing in 2D, Coinc/Sing ratio = %f\n",
	    dattim, save);
    fflush(file3);
    break;
  }

  /* unfold in 2D */
  while (caskyn("Correct for Comptons? (Y/N)")) {
    /* ask for and open response function file */
    if (strlen(infile2) > 0) {
      strcpy(fn, infile2);
      infile2 = "";
    } else {
      cask("Response function file = ? (default .ext = .rsp)", fn, 80);
    }
    setext(fn, ".rsp", 80);
    if (!(file2 = fopen(fn, "r"))) {
      file_error("open", fn);
      continue;
    }
    if (get_file_rec(file2, ans, 40, 0) <= 0 ||
	strncmp(ans, head, 20) ||
	get_file_rec(file2, buf, sizeof(buf), -1) <= 0) {
      file_error("read", fn);
      fclose(file2);
      continue;
    }
    memcpy(&thresh, buf, 4);
    memcpy(resp[0], buf + 4, 2056*25);
    fclose(file2);
    printf("File was written at %.18s\n"
	   "  Threshold = %.1f keV\n"
	   "  Stored response functions are:\n"
	   "    #    Egamma\n", ans+20, thresh);
    for (i = 0; i < 25; i++) {
      if (resp[i][513] > 0.0f) printf("%5d %9.2f\n", i+1, resp[i][513]);
    }
    if (resp[23][513] == 0.f) {
      printf("ERROR - not enough response functions to do unfolding.\n");
      continue;
    }

    printf("\nWorking...\n");
    i0 = channo(100.0f);
    if (i0 < 1) i0 = 1;
    if (i0 < channo(thresh)) i0 = channo(thresh);
    for (iy = numchs-1; iy >= i0; --iy) {
      printf("\rRow %4i", iy);
      fflush(stdout);
      if (calres(iy, spec, numchs)) break;
      for (ix = 0; ix < numchs; ++ix) {
	c = DATA(ix,iy);
	for (iw = 0; iw <= iy; ++iw) {
	  DATA(ix,iw) -= spec[iw] * c;
	}
      }
    }
    printf("\n ...done.\n");
    datetime(dattim);
    fprintf(file3,
	    "%s: program unfoldesc:\n"
	    "   2D-unfolded using response functions in file %s\n",
	    dattim, fn);
    fflush(file3);
    break;
  }

  printf("\nRewriting 2D data...\n");
  rewind(file);
  fflush(file);
  for (iy = 0; iy < numchs; ++iy) {
    if (iy%10 == 0) {
      printf("\rRow %i", iy);
      fflush(stdout);
    }
    for (ix = 0; ix < iy; ++ix) {
      ich = luch[ix] + iy;
      rdata[ix] = data[ich];
    }
    for (ix = iy; ix < numchs; ++ix) {
      ich = luch[iy] + ix;
      rdata[ix] = data[ich];
    }
    fwrite(rdata, 4, numchs, file);
  }
  fflush(file);
  printf("    ...done.\n");

  printf("You will need to change the projection and background\n"
	 " spectra in the .esc file...\n");
  fflush(stdout);
  for (iy = 0; iy < numchs; ++iy) {
    spec[iy] = 0.0f;
    for (ix = 0; ix < numchs; ++ix) {
      spec[iy] += DATA(ix,iy);
    }
  }
  if (!esc_file_flags[0]) {
    for (iy = 4095; iy >= 0; iy--) {
      spec[iy] = spec[iy/2] / 2.0f;
    }
  }
  /* write out projections */
  cask(" New projection spectrum file name = ?", ans, 80);
  wspec(ans, spec, MAXCHS);

  memset(spec, 0, reclen);
  for (iy = numchs + 3; iy < numchs + 6; iy++) {
    fseek(file, iy*reclen, SEEK_SET);
    fwrite(spec, 4, numchs, file);
    fflush(file);
  }
  printf(" ....done.\n"
	 "If you had used the E2-enhanced background subtraction,\n"
	 "  it is now disabled.  Sorry.\n");
  fprintf(file3,
	  "   If you had used the E2-enhanced background subtraction,\n"
	  "      it is now disabled.  Sorry.\n");
  fflush(file3);

  fflush(stdout);

  /* make sure esc_file_flags[2] is set for file 1 */
  esc_file_flags[2] = 1;
  fseek(file, (numchs + 6)*reclen + 124, SEEK_SET);
  fwrite(esc_file_flags, 4, 10, file);

  fclose(file);
  fclose(file3);

  return 0;
}

/* ======================================================================= */
int adjgain(float oldch1, float oldch2, float newch1, float newch2,
	    float *oldsp, float *newsp, int nco, int ncn, float fact)
{
  /* adjust gain of old spectrum to get newsp
     mode=1:  add (new values * fact) to newsp
     nco = no. of chs in oldsp
     ncn = no. of chs in newsp */

  float a, b, ch, cl, oh, ol;
  int   flag, newch, ioh, iol, ist;

  if (newch1 == newch2 ||
      (a = (oldch2 - oldch1) / (newch2 - newch1)) <= 0.f) {
    printf("Error - cannot continue.  Present values of\n"
	   "   oldch1,oldch2,newch1,newch2 are: %.3f %.3f %.3f %.3f\n",
	   oldch1, oldch2, newch1, newch2);
    return 0;
  }
  b = oldch2 - a * newch2;
  ist = (-0.5f - b) / a + 0.5f;
  if (ist < 0) ist = 0;
  ol = ((float) ist - 0.5f) * a + b;
  if (ol < -0.5f) ol = -0.5f;
  iol = ol + 0.5f;
  cl = oldsp[iol] * ((float) iol + 0.5f - ol);
  flag = 0;
  for (newch = ist; newch < ncn; ++newch) {
    oh = ((float) newch + 0.5f) * a + b;
    ioh = oh + 0.5f;
    if (ioh >= nco) {
      oh = (float) (nco) - 0.5f;
      ioh = nco - 1;
      flag = 1;
    }
    ch = oldsp[ioh] * ((float) ioh + 0.5f - oh);
    while (++iol <= ioh) {
      cl += oldsp[iol];
    }
    newsp[newch] += (cl - ch) * fact;
    if (flag) return 0;
    cl = ch;
    iol = ioh;
  }
  return 0;
} /* adjgain */

/* ====================================================================== */
int calres(int ch, float *spout, int numchs)
{
  float fact, ecut, xcut, f, fch, x, f1, f2, ee, el, egamma;
  float ebs, esc, xhi, xlo, rnc1;
  float save[512];
  int   i, j, ich;
  static float cch1 = -0.5f, cch2 = 99.5f, cch3 = 249.5f, cch4 = 129.5f;

  memset(spout, 0, 4*numchs);
  fch = (float) ch;
  egamma = gain[nterms - 1];
  for (j = nterms - 2; j >= 0; --j) {
    egamma = gain[j] + egamma * fch;
  }

  for (j = 0; j < 24; ++j) {
    if (egamma < resp[j+1][513]) break;  /* resp[j][513] = egamma */
  }
  if (j > 23) j = 23;
  if (resp[j][513] == 0.0f) j++;
  if (j == 24) return 1;
  el = resp[j][513];
  f2 = (log(egamma) - log(el)) / (log(resp[j+1][513]) - log(el));
  f1 = 1.0f - f2;
  for (i = 0; i < 510; ++i) {
    save[i] = f1*resp[j][i] + f2*resp[j+1][i];
  }
  if (el - (el * 1.2f / (el / 255.503f + 1.0f)) <= thresh) {
    for (i = 0; i < 100; ++i) {
      save[i] = resp[j+1][i];
    }
  }
  /* resp[j][513] = egamma
     resp[j][512] gives e1area
     resp[j][511] gives e2area */
  ebs = egamma / (1.0f + (egamma/255.503f));
  ecut = egamma - ebs * 1.2f;
  xcut = channo(ecut);
  xlo = channo(ebs * 0.8f);
  xhi = channo(ebs * 1.8f);
  /* printf("*** %f %f %f %f %f %f %f\n",
     fch, egamma, ebs, ecut, xcut, xlo, xhi);
  */
  if (xhi > fch * 0.99f) xhi = fch * 0.99f;
  if (thresh < ecut) {
    rnc1 = channo(thresh);
    fact = ecut - thresh;
    adjgain(cch1, cch2, rnc1, xcut, save, spout, 100, numchs, fact);
  }
  fact = egamma - ecut;
  adjgain(cch1, cch3, xcut, fch, save+100, spout, 280, numchs, fact);
  fact = ebs;
  adjgain(cch1, cch4, xlo, xhi, save+380, spout, 130, numchs, fact);

  /* calculate escape peaks */
  if (egamma < 1100.f) return 0;
  ee = log(log(egamma / 1022.0f)) * 2.935f;
  for (i = 1; i <= 2; ++i) {
    if (resp[24][513-i] == 0.0f) continue;
    x = channo(egamma - ((float) i) * 511.006f);
    ich = (int) x;
    f = x - (float) ich;
    esc = exp(resp[24][513-i] + ee);
    spout[ich] += (1.0f - f) * esc;
    spout[ich + 1] += f * esc;
  }
  return 0;
} /* calres */

/* ======================================================================= */
float channo(float egamma)
{
  static float diff = .002f;
  float  e, x1, x2, d1, ret_val;
  int    i;

  /* the value of diff fixes the precision when nterms > 3 */
  if (nterms <= 3) {
    if (gain[2] < 1e-7) {
      x1 = (egamma - gain[0]) / gain[1];
      ret_val = (egamma - gain[0]) / (gain[1] + gain[2] * x1);
    } else {
      ret_val = (sqrt(gain[1] * gain[1] -
		      gain[2] * 4.0f * (gain[0] - egamma)) -
		 gain[1]) / (gain[2] * 2.0f);
    }
  } else {
    x1 = (egamma - gain[0]) / gain[1];
    x1 = (egamma - gain[0]) / (gain[1] + gain[2] * x1);
    
    while (1) {
      e = gain[nterms - 1];
      for (i = nterms - 2; i > 1; --i) {
	e = gain[i] + e * x1;
      }
      x2 = e * x1 * x1;
      x2 = (egamma - gain[0] - x2) / gain[1];
      if ((d1 = x2 - x1, fabs(d1)) <= diff) break;
      x1 = x2;
    }
    ret_val = x2;
  }
  return ret_val;
} /* channo */
