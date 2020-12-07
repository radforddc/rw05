/***************************************************************************
   PRO3D: 3D-to-2D and 3D-to-1D projection program for
       compressed-format RadWare triple coincidence cubes.

   D.C Radford      Sept 1997
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "rwutils.c"
#include "util.h"

int main (int argc, char **argv)
{
  float *data2d;
  int   length;  /* number of channels per side of cube */
  int   x, y, xx, yy, ix, iy, length2, cfact;
  int   nclook, lookmin, lookmax;          /* lookup table data */
  short looktab[16384];
  char  innam[256], tabnam[256], outnam[256], cbuf[256];
  int   data1d[16384], ch_width[16384];
  FILE  *file;
  struct stat statbuf;


  printf("\n\n"
	 "  2dp2spn -- a 2D - projection -to- .spn radware matrix program.\n\n"
	 "       D.C Radford    Aug 2004\n\n");

  /* open 2d projection file */
  while (1) {
    printf("Enter input 2D file name (%s): ",innam);
    fgets(innam, 250, stdin);
    innam[strlen(innam)-1] = 0;  /* remove \n */
    setfnamext(innam,".2dp",0);
    if ((file=fopen(innam,"r+"))) break;
    printf("\007  ERROR -- Cannot open file %s for reading.\n", innam);
  }

  /* deduce number of channels from file size */
  stat(innam, &statbuf);
  length = (int) (0.5 + sqrt((float) (statbuf.st_size/4)));

  /* malloc the 2D projection space */
  while (!(data2d=malloc(4*length*length))) {
    printf("Ooops, data2d malloc failed...\n"
	   "  ... please free up some memory and press return.\n");
    fgets(cbuf,256,stdin);
  }

  /* read in 2d projection */
  if (!(fread(data2d, 4*length, length, file))) {
    printf("\007  ERROR -- Cannot read 2d file, aborting...\n");
    exit(-1);
  }
  fclose(file);

  /* get the cube channel lookup table */
  while (1) {
    printf("Enter .tab file name used to create the cube: ");
    fgets(tabnam, 250, stdin);
    tabnam[strlen(tabnam)-1] = 0;  /* remove \n */
    setfnamext(tabnam,".tab",0);
    if (read_tab_file(tabnam, &nclook, &lookmin, &lookmax, looktab, 16384)) {
      printf("\007  ERROR -- Cannot read file %s\n", tabnam);
      continue;
    }
    if (lookmax == length) break;
    printf("\007  ERROR -- lookup table max. value = %d;\n"
	   "        -- does not match 2dp size of %d channels.\n", lookmax, length);
  }

  /* we need to correct for the variable number of ADC chs per cube ch */
  for (x=0; x<length; x++) {
    ch_width[x] = 0;
  }
  length2 = 4096;
  for (x=0; x<nclook; x++) {
    if (looktab[x]) {
      length2 = x + 1;           /* length2 = 2dp size in ADC chs */
      ch_width[looktab[x]-1]++;  /* no. of ADC chs for each cube ch. */
    }
  }
  for (x=0; x<length; x++) {
    for (y=0; y<length; y++) {
      *(data2d + y*length + x) /= (float) (ch_width[x]*ch_width[y]);
    }
  }
  cfact = (length2+4095)/4096;      /* we need to contract ADC chs by cfact to get <= 4096 */
  if (cfact > 4) cfact = 4;

  /* open spn file */
  while (1) {
    setfnamext(innam,".spn",1);
    printf("Enter output matrix file name (%s): ",innam);
    fgets(outnam, 250, stdin);
    outnam[strlen(outnam)-1] = 0;  /* remove \n */
    if (outnam[0] == 0) strncpy(outnam, innam, 256);
    setfnamext(outnam,".spn",0);
    if ((file=fopen(outnam,"w+"))) break;
    printf("\007  ERROR -- Cannot open file %s for writing.\n", outnam);
  }

  /* write out spn file */
  for (y=0; y<4096; y++) {
    for (x=0; x<4096; x++) {
      data1d[x] = 0;
    }
    for (yy=cfact*y; yy<cfact*(y+1); yy++) {
      if (!(iy = looktab[yy])) continue;
      for (x=0; x<4096; x++) {
	for (xx=cfact*x; xx<cfact*(x+1); xx++) {
	  if (!(ix = looktab[xx])) continue;
	  data1d[x] += (int) (*(data2d + (iy-1)*length + ix-1) + 0.5f);
	}
      }
    }

    if (!(fwrite(data1d, 4*4096, 1, file))) {
      printf("\007  ERROR -- Cannot write spn file, aborting...\n");
      exit(-1);
    }
  }
  fclose(file);

  printf("\nCongratulations! All seems to have finished okay.\n\n");
  return 0;
}
