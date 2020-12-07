/***************************************************************************
   MAKE4CUB: program to splitmake new (empty) RadWare quadruple coincidence
             hybercubes in multiple files.
   This version for 1/24 hybercubes.
   D.C Radford, C.J. Beyer    Aug 1997
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "4play.h"
#include "rwutils.c"

#define VERSION "27 August 1997   Beta"

/* Some global data */
struct GlobalData {
  FILE *CubeDirFile;
  char CubeDirFileName[256];
  int numCubeFiles;
  int mclo[RW_MAX_FILES], mchi[RW_MAX_FILES];
  int length;     /* length of axis on cube */
  int nummc;      /* number of minicubes */
  int numsr[RW_MAX_FILES];     /* current number of SuperRecords */
  int totnumsr;                /* current total number of SuperRecords */
} gd;

int
main (int argc, char **argv)
{
  int            numchunks;
  Record         rec;
  int            i, j, f, length, numr;
  char           cbuf[256];
  int            nmcperc=RW_CHNK_S*256, numcperf;
  FHead          filehead;
  FILE           *file;
  char           dnam[256];
  unsigned int   nummc, numsr;
  SRHead         shead;


  printf("\n\n"
	 " Make4cub: a program to make new  RadWare\n"
	 "       quadruple coincidence hybercubes.\n\n"
	 "    Version "VERSION"\n\n");


  while (1) {
    printf("How many channels per side for the new hypercube? ");
    fgets(cbuf, 256, stdin);
    if (sscanf(cbuf,"%d",&length) == 1) {
      if (length > 1 && length <= RW_MAXCH)
	  break;
      printf("Must be no more than %d\n", RW_MAXCH);
    }
  }

  nummc = (length +3)/4;  /* minicubes per side */
  nummc = nummc*(nummc+1)*(nummc+2)/6*(nummc+3)/4; /* minicubes in cube */
  gd.nummc = nummc;

  /* a chunk is RW_CHNK_S/16 Mchannels, nmcperc minicubes */
  numchunks = (gd.nummc+nmcperc-1)/nmcperc;
  printf("There will be %d chunks in the cube....\n",numchunks);

  while (1) {
    printf("How many files do you want to split the cube into? ");
    fgets(cbuf, 256, stdin);
    if (sscanf(cbuf,"%d",&gd.numCubeFiles) == 1) {
      if (gd.numCubeFiles > 0 && gd.numCubeFiles <= RW_MAX_FILES)
	  break;
      printf("Must be between 1 and %d\n", RW_MAX_FILES);
    }
  }

  numcperf = 1 + (numchunks-1)/gd.numCubeFiles;
  printf("Okay, there will be %d chunks per file.\n\n", numcperf);

  /* open the output cube directory file */
  while (1) {
    printf("Enter output cube directory file name: ");
    fgets(dnam, 250, stdin);
    dnam[strlen(dnam)-1] = 0;  /* remove \n */
    if (dnam[0] == 0)
      continue;

    setfnamext(dnam,".4d",0);
    if ((file=fopen(dnam,"r"))) {
      printf("\007  ERROR -- file already exists, give a new file name.\n");
      fclose(file);
      continue;
    }
    if ((gd.CubeDirFile = fopen(dnam,"w+")))
      break;
    printf("\007  ERROR -- cannot open new file %s.\n", dnam);
  }

  /* loop through all the output files */
  gd.totnumsr = 0;
  for (f=0; f<gd.numCubeFiles; f++) {

    /* open the output cube subfile */
    while (1) {
      printf("Enter name for new output cube file #%d: ", f+1);
      fgets(dnam, 250, stdin);
      dnam[strlen(dnam)-1] = 0;  /* remove \n */
      if (dnam[0] == 0)
	continue;

      setfnamext(dnam,".4cub",0);
      if ((file=fopen(dnam,"r"))) {
	printf("\007  ERROR -- file already exists, give a new file name.\n");
	fclose(file);
	continue;
      }
      if ((file = fopen(dnam,"w+")))
	break;
      printf("\007  ERROR -- cannot open new file %s.\n", dnam);
    }
    fprintf(gd.CubeDirFile, "%s\n", dnam);

    gd.mclo[f] = nmcperc*numcperf*f;
    gd.mchi[f] = nmcperc*numcperf*(f+1) - 1;
    if (gd.mchi[f] > gd.nummc-1)
      gd.mchi[f] = gd.nummc - 1;
    printf(" File #%d, %s, has minicubes %d to %d.\n\n",
	   f+1, dnam, gd.mclo[f], gd.mchi[f]);

    numr = 1 + (gd.mchi[f]-gd.mclo[f])/1016; /* an empty minicube is 1 byte */
    numsr = (numr + 1022)/1023; /* number of SuperRecords in empty file */
    gd.numsr[f] = numsr;
    gd.totnumsr += numsr;

    strncpy(filehead.id, "4play hypercube ", 16);
    filehead.numch   = length;
    filehead.cps     = 24;
    filehead.version = 0;
    filehead.snum    = numsr;
    filehead.mclo    = gd.mclo[f];
    filehead.mchi    = gd.mchi[f];
    memset (filehead.resv, 0, 984);
    memset (rec.d, 0, 1016);

    if (!fwrite (&filehead, 1024, 1, file)) {
      printf("\007  ERROR -- Cannot write new cube header, aborting...\n");
      exit(-1);
    }

    for(j=0; j<numsr; j++) { /* loop through SuperRecords */
      shead.snum = j;
      shead.minmc = gd.mclo[f] + j*1016*1023;
      shead.maxmc = gd.mclo[f] + (j+1)*1016*1023 - 1;
      if (shead.maxmc>gd.mchi[f])
	shead.maxmc = gd.mchi[f];
      if(!fwrite(&shead,1024,1,file)) {
	fprintf(stderr, "\007  ERROR -- Could not write header for "
		"SuperRecord number %d... aborting.\n",j+1);
	fclose(file);
	exit(-1);
      }
      for(i=0; i<1023; i++) { /* loop through records in SuperRecord */
	rec.h.minmc = gd.mclo[f] + (j*1023 +i)*1016;
	if (rec.h.minmc>gd.mchi[f])
	  rec.h.minmc = -1;
	rec.h.nummc = 1016;
	if (rec.h.minmc>gd.mchi[f]) {
	  rec.h.minmc = -1;
	  rec.h.nummc = 0;
	}
	if (rec.h.minmc+1015>gd.mchi[f])
	   rec.h.nummc = gd.mchi[f] - rec.h.minmc + 1;
	rec.h.offset = 8;
	if(!fwrite(&rec,1024,1,file)) {
	  fprintf(stderr, "\007  ERROR -- Could not write record %d "
		  "of SuperRecord %d... aborting.\n",i+1,j+1);
	  fclose(file);
	  exit(-1);
	}
      }
    }
    fclose(file);
  }

  printf("Total number of SuperRecords = %d.\n", gd.totnumsr);
  return 0;
}
