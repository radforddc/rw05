/***************************************************************************
   SPLIT4CUB: program to split quadruple coincidence hybercubes into multiple
              files.
   This version for 1/24 hybercubes.
   D.C Radford, C.J. Beyer    Aug 1997
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "4play.h"
#include "rwutils.c"

#define VERSION "5 Sept 1997   Beta"

/* Some global data */
struct GlobalData {
  FILE *CubeFile[RW_MAX_FILES];
  char CubeFileName[RW_MAX_FILES][256];
  int numCubeFiles;
  int mclo[RW_MAX_FILES], mchi[RW_MAX_FILES];
  int length;     /* length of axis on cube */
  int nummc;      /* number of minicubes */
  int numsr[RW_MAX_FILES];     /* current number of SuperRecords */
  int totnumsr;                /* current total number of SuperRecords */
} gd;
struct GlobalDataIn {
  FILE *CubeFile;
  char CubeFileName[256];
  int mclo, mchi;
  int length;     /* length of axis on cube */
  int nummc;      /* number of minicubes */
  short srvlut[2*RW_MAX_FILE_SIZE];/* SR virtual # lookup table */
  int numsr;     /* current number of SuperRecords */
} gdin;

#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))

#include "compress4d.c"
int
main (int argc, char **argv)
{
  int            numchunks;
  char           *srecIn;
  char           *srecOut;
  SRHead         *srheadIn;
  SRHead         *srheadOut;
  Record         *recIn, *recOut;
  int            i, j, f, mc, mcl;
  unsigned char  *mcptrIn, *mcptrOut, cbuf[1024];
  int            minmc, maxmc;
  int            srecnumIn, srecnumOut, oldsrnum;
  int            recnumIn, recnumOut, nbytes;
  int            nmcperc=RW_CHNK_S*256, numcperf;
  FHead          filehead;
  FILE           *file;
  char           dnam[256];
  int            nummc,numsr,chunknum;
  SRHead         shead;


  printf("\n\n"
	 " Split4cub: a program to split a RadWare quadruple coincidence\n"
	 "       hybercube file into multiple files.\n\n"
	 "    Version "VERSION"\n\n");

  while (!(srecIn=malloc(1024*1024))) {
    printf("Ooops, srecIn malloc failed...\n"
	   "  ... please free up some memory and press return.\n");
    fgets((char *)cbuf,256,stdin);
  }
  while (!(srecOut=malloc(1024*1025))) {
    printf("Ooops, srecOut malloc failed...\n"
	   "  ... please free up some memory and press return.\n");
    fgets((char *)cbuf,256,stdin);
  }

  /* open the input cube file */
  while (1) {
    if (argc > 1) {
      strncpy(dnam, argv[1], 70);
      argc = 0;
    } else {
      printf("Enter input cube file name: ");
      fgets(dnam, 250, stdin);
      dnam[strlen(dnam)-1] = 0;  /* remove \n */
    }
    if (dnam[0] == 0) continue;

    setfnamext(dnam,".4cub",0);
    if (!(file=fopen(dnam,"r"))) {
      fprintf(stderr,"\007  ERROR -- cube file %s does not exist...\n", dnam);
      continue;
    }

    printf("  Checking existing cube file %s...\n", dnam);
    if (!fread(&filehead,1024,1,file)) {
      fprintf(stderr,"\007  ERROR -- cannot read cube file header...\n");
      continue;
    }
    if (strncmp(filehead.id,"4play hypercube ",16) ||
	filehead.cps != 24) {
      fprintf(stderr,"\007  ERROR -- Invalid cube file header...\n");
      fclose(file);
      continue;
    }
    gd.length = filehead.numch;
    gdin.numsr = filehead.snum;
    gdin.mclo = filehead.mclo;
    gdin.mchi = filehead.mchi;
    numsr = filehead.snum;
    if (numsr>2*RW_MAX_FILE_SIZE) {
      fprintf(stderr,"\007  ERROR -- too many SuperRecords in cube...\n");
      fclose(file);
      continue;
    }
    break;
  }
  printf("  ...Okay, %d SuperRecords.\n", numsr);
  gdin.CubeFile = file;

  nummc = gdin.mchi - gdin.mclo + 1;  /* minicubes in this file */
  gd.nummc = nummc;

  /* build SuperRecord virtual number lookup table */
  printf("  Building SuperRecord list...\n");
  memset(gdin.srvlut, -1, 2*RW_MAX_FILE_SIZE*2);
  for (i=0; i<numsr; i++) {
    fseek(file, ((((long)i)*1024)+1)*1024, SEEK_SET);
    if (!fread(&shead,1024,1,file)) {
      fprintf(stderr, "\007  ERROR -- Could not read SRec header %d... "
	      "aborting.\n", i);
      exit(-1);
    }
    gdin.srvlut[i] = shead.snum;
  }
  printf("  ...Done building SuperRecord list.\n");

  /* a chunk is RW_CHNK_S/16 Mchannels, nmcperc minicubes */
  numchunks = (gd.nummc+nmcperc-1)/nmcperc;
  printf("There are %d chunks in this file....\n",numchunks);

  while (1) {
    printf("How many files do you want to split this file into? ");
    fgets((char *)cbuf, 256, stdin);
    if (sscanf((char *)cbuf,"%d",&gd.numCubeFiles) == 1) {
      if (gd.numCubeFiles > 1 && gd.numCubeFiles <= RW_MAX_FILES)
	  break;
      printf("Must be between 2 and %d\n", RW_MAX_FILES);
    }
  }

  numcperf = 1 + (numchunks-1)/gd.numCubeFiles;
  printf("Okay, there will be %d chunks per output file.\n\n", numcperf);

  srheadIn = (void*)srecIn;
  srheadOut = (void*)srecOut;
  srecnumIn = 0;      /* virtual SuperRecord for first chunk */
  recnumIn = 0;       /* very first record starts the cube */
  oldsrnum = 0;
  chunknum = 0;

  /* read in first SuperRecord */
  for (j=0; gdin.srvlut[j]!=srecnumIn; j++);
  fseek(gdin.CubeFile, ((((long)j)*1024)+1)*1024, SEEK_SET);
  if (!fread(srecIn, 1024*1024, 1, gdin.CubeFile)) {
    printf("\007  ERROR -- Corrupted cube, aborting...\n");
    exit(-1);
  }
  recIn = (Record *)&srecIn[1024];
  mcptrIn = (unsigned char *)recIn + recIn->h.offset;
  gdin.srvlut[srecnumIn] = -1;

  /* loop through all the output files */
  for (f=0; f<gd.numCubeFiles; f++) {

    /* open the output cube subfile */
    while (1) {
      printf("Enter name for new output cube file #%d: ", f+1);
      fgets(dnam, 250, stdin);
      dnam[strlen(dnam)-1] = 0;  /* remove \n */
      if (dnam[0] == 0) continue;

      setfnamext(dnam,".4cub",0);
      if ((file=fopen(dnam,"r"))) {
	printf("\007  ERROR -- file already exists, give a new file name.\n");
	fclose(file);
	continue;
      }
      if ((gd.CubeFile[f] = fopen(dnam,"w+"))) break;
      printf("\007  ERROR -- cannot open new file %s.\n", dnam);
    }

    gd.mclo[f] = gdin.mclo + nmcperc*numcperf*f;
    gd.mchi[f] = gdin.mclo + nmcperc*numcperf*(f+1) - 1;
    if (gd.mchi[f] > gdin.mchi) gd.mchi[f] = gdin.mchi;
    printf(" File #%d, %s, has minicubes %d to %d.\n\n",
	   f+1, dnam, gd.mclo[f], gd.mchi[f]);

    filehead.snum = 0;
    filehead.mclo = gd.mclo[f];
    filehead.mchi = gd.mchi[f];
    if (!fwrite (&filehead, 1024, 1, gd.CubeFile[f])) {
      printf("\007  ERROR -- Cannot write new file header, aborting...\n");
      exit(-1);
    }

    /* init the first output SuperRecord */
    srecnumOut = 0;
    srheadOut->snum = 0;
    srheadOut->minmc = gd.mclo[f];
    memcpy (srheadOut->resv, srheadIn->resv, 1012);
    recnumOut = 0;
    recOut = (Record *)&srecOut[1024];
    recOut->h.minmc = gd.mclo[f];
    recOut->h.offset = 8;
    mcptrOut = (unsigned char *)recOut + recOut->h.offset;

    gd.numsr[f] = 0;
  
    /* loop through all the chunks in the new cube subfile */
    for (minmc=gd.mclo[f]; minmc<gd.mchi[f]; minmc+=nmcperc) {
      maxmc = minmc+nmcperc-1;
      if (maxmc>gd.mchi[f]) maxmc = gd.mchi[f];
      printf("\r  ...chunk %d %d, SRs %d %d, recs %d %d   ",
	     chunknum, chunknum-(f*numcperf), oldsrnum,srecnumOut,
	     recnumIn,recnumOut);
      fflush(stdout);
      chunknum++;

      /* loop through all the minicubes in the chunk */
      for (mc=minmc; mc<=maxmc; mc++) {

	if (mc > recIn->h.minmc + recIn->h.nummc - 1) {
	  /* next compressed minicube starts in the next input record */
	  if (++recnumIn == 1023) {
	    /* ooops, we need a new SuperRecord */
	    srecnumIn++;
	    oldsrnum++;
	    for (j=0; gdin.srvlut[j]!=srecnumIn; j++);
	    fseek (gdin.CubeFile, ((((long)j)*1024)+1)*1024, SEEK_SET);
	    if (!fread (srecIn, 1024*1024, 1, gdin.CubeFile)) {
	      printf("\007  ERROR -- Corrupted cube, aborting...\n");
	      exit(-1);
	    }
	    gdin.srvlut[j] = -1;
	    recIn = (Record *)&srecIn[1024];
	    recnumIn = 0;
	  }
	  else {
	    recIn = (Record *)((unsigned char *)recIn + 1024);
	  }

	  mcptrIn = (unsigned char *)recIn + recIn->h.offset;
	  /* at this point our minicube should be at the start of the record */
	  if (recIn->h.minmc != mc) {
	    printf("Severe ERROR - fatal!\n PLEASE send a bug report"
		   " to radfordd@mail.phy,ornl.gov with this information\n");
	    printf("SR in,out: %d,%d  rec: %d  mc: %d  should be %d\n",
		   oldsrnum,srecnumOut, recnumIn, mc, recIn->h.minmc);
	    exit(-1);
	  }
	}
	else if (mcptrIn > (unsigned char *)recIn+1024 ) {
	  printf("Severe ERROR 2 - fatal!\n PLEASE send a bug report"
		 " to radfordd@mail.phy,ornl.gov with this information\n");
	  printf("SR in,out: %d,%d  rec: %d  mc: %d  should be gt %d\n",
		 oldsrnum,srecnumOut, recnumIn, mc,
		 recIn->h.minmc + recIn->h.nummc - 1);
	  exit(-1);
	}

	mcl=MCLEN(mcptrIn);
	memcpy (mcptrOut, mcptrIn, mcl);

	if (mcptrIn + mcl > (unsigned char *)recIn+1024 )
	  memcpy (mcptrOut+(int)((unsigned char *)recIn + 1024 - mcptrIn),
		  (char*)recIn+1024+8, mcl);
	else
	  mcptrIn += mcl;

	if (mcptrOut + mcl > (unsigned char *)recOut+1024 ) {
	  /* printf("SR, rec, mcl: %d %d %d %d %d\n", oldsrnum,srecnumOut,
	     recnumIn,recnumOut,mcl); */
	  /* the minicube spills over the end of the output record */
	  memcpy (cbuf, mcptrOut, mcl);

	  if (recnumOut == 1022) {
	    /* ooops, we are at the end of our output SuperRecord */
	    recOut->h.nummc = mc - recOut->h.minmc;
	    srheadOut->maxmc = mc-1;
	    fseek (gd.CubeFile[f],
		   ((((long)srecnumOut)*1024)+1)*1024, SEEK_SET);
	    if (!fwrite(srecOut, 1024*1024, 1, gd.CubeFile[f])) {
	      printf("\007  ERROR -- Cannot write cube, aborting...\n");
	      exit(-1);
	    }
	    gd.numsr[f]++;

	    /* set up header for new output SuperRecord */
	    srecnumOut++;
	    srheadOut->snum = srecnumOut;
	    srheadOut->minmc = mc;

	    /* now set up first record of next output SuperRecord */
	    recnumOut = 0;
	    recOut = (Record *)&srecOut[1024];
	    recOut->h.minmc = mc;
	    recOut->h.offset = 8;
	    mcptrOut = (unsigned char *)recOut + recOut->h.offset;
	    memcpy (mcptrOut, cbuf, mcl);
	    mcptrOut += mcl;

	  }
	  else if (mcptrOut + 2 > (unsigned char *)recOut+1024 ) {
	    /* need at least first 2 bytes of minicube in current record */
	    /* so move whole minicube to next record */
	    recOut->h.nummc = mc - recOut->h.minmc;
	    recOut = (Record *)((unsigned char *)recOut + 1024);
	    recOut->h.minmc = mc;
	    recOut->h.offset = 8;
	    mcptrOut = (unsigned char *)recOut + recOut->h.offset;
	    memcpy (mcptrOut, cbuf, mcl);
	    mcptrOut += mcl;
	    recnumOut++;

	  }
	  else {
	    /* move only part of minicube to next record */
	    nbytes = mcptrOut + mcl - ((unsigned char *)recOut+1024);
	    recOut->h.nummc = mc - recOut->h.minmc + 1;
	    recOut = (Record *)((unsigned char *)recOut + 1024);
	    recOut->h.minmc = mc+1;
	    recOut->h.offset = nbytes+8;
	    memcpy ((char *)recOut+8, cbuf+mcl-nbytes, nbytes);
	    mcptrOut = (unsigned char *)recOut + recOut->h.offset;
	    recnumOut++;

	  }
	}
	else {
	  mcptrOut += mcl;
	}
      }
    }

    /* write out the last output SuperRecord in this subfile */
    recOut->h.nummc = gd.mchi[f] - recOut->h.minmc + 1;
    srheadOut->maxmc = gd.mchi[f];
    for (j=0; j<1022-recnumOut; j++) {
      recOut = (Record *)((unsigned char *)recOut + 1024);
      recOut->h.minmc = -1;
    }
    fseek (gd.CubeFile[f], ((((long)srecnumOut)*1024)+1)*1024, SEEK_SET);
    if (!fwrite(srecOut, 1024*1024, 1, gd.CubeFile[f])) {
      printf("\007  ERROR -- Cannot write cube, aborting...\n");
      exit(-1);
    }
    gd.numsr[f]++;

    filehead.snum = gd.numsr[f];
    fseek (gd.CubeFile[f], 0, SEEK_SET);
    if (!fwrite (&filehead, 1024, 1, gd.CubeFile[f])) {
      printf("\007  ERROR -- Cannot rewrite cube header, aborting...\n");
      exit(-1);
    }

    printf("\nThere are %d SuperRecords in file #%d.\n", gd.numsr[f], f+1);
    fclose(gd.CubeFile[f]);
  }

  fclose(gdin.CubeFile);
  free(srecIn);
  free(srecOut);

  printf("\nAll is done.\n"
	 "    Do not forget to edit your 4cube directory file\n"
	 "    to reflect the changes in the file structure!\n\n");
  exit(0);

}

