/***************************************************************************
   FOLDOUT:  program to fold 1/6 of triples cube to 1/2 cube, using
       compressed-format RadWare triple coincidence cubes.

   D.C Radford      Oct 1997
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "pro3d.h"
#include "rwutils.c"

#define VERSION "2 Oct 1997  Alpha"

int lumx3d[RW_MAXCH+16]; /* look-up table, maps 3d ch to linear minicube */
int lumy3d[RW_MAXCH+16];
int lumz3d[RW_MAXCH+16];

  /* look up minicube addr */
#define LUMC(x,y,z) lumx3d[x] + lumy3d[y] + lumz3d[z]

#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))

/* Some global data */
struct GlobalData {
  FILE *CubeFileIn;
  int length;     /* length of axis on cube */
  int *mcnum;     /* SuperRecord minicube number table */
  int numrecs;    /* number of 4kB data records in the file */
  int cubetype;   /* 0 for compressed 1/6 cube,
		     1 for compressed 1/2 cube,
                     2 for uncompressed 1/6 cube, or
		     3 for uncompressed 1/2 cube  */
  int numsr;
  int swapbytes;
} gd;

#include "compress3d.c"

void
swap4(int *in)
{
  char *c, tmp;

  c = (char *) in;
  tmp = *c;
  *c = *(c+3);
  *(c+3) = tmp;
  tmp = *(c+1);
  *(c+1) = *(c+2);
  *(c+2) = tmp;
}

void
swap2(unsigned short *in)
{
  char *c, tmp;

  c = (char *) in;
  tmp = *c;
  *c = *(c+1);
  *(c+1) = tmp;
}

void
getmc(int num, unsigned int mc[256])
{
  int i, skip, mcl, nbytes;
  static Record3D rec;
  static int recloaded=0;
  static int recnum=0;      /* number of any currently loaded record */
  static int oldnum=0;      /* number of last minicube decompressed, if any */
  int srnum;
  static unsigned char *mcptr;
  unsigned char cbuf[1024];

  if ((!recloaded) ||
      num < rec.h.minmc ||
      num > rec.h.minmc+rec.h.nummc-1) {
    /* no record is loaded, or it does not have the required minicube */
    /* first check to see that the required minicube does not */
    /* lie just ahead of any currently loaded record */
    /* if it does, we can just read ahead in the file, rather than fseeking */
    srnum = recnum/16;
    if ((!recloaded) ||
	num < rec.h.minmc ||
	num > rec.h.minmc+4*rec.h.nummc-1) {

      /* next look to see if it lies outside any current SuperRecord */
      if ((!recloaded) ||
	  num < gd.mcnum[srnum] ||
	  num > gd.mcnum[srnum+1]) {

	/* search for the record that contains minicube # num */
	/* first search in units of 64MB */
	for (srnum=1024; srnum<=gd.numsr; srnum+=1024)
	  if (gd.mcnum[srnum] > num)
	    break;
	
	/* now search in units of 2MB */
	for (srnum-=1024; srnum<=gd.numsr; srnum+=32)
	  if (gd.mcnum[srnum] > num)
	    break;

	/* now search in units of 1/16 MB */
	for (srnum-=32; srnum<=gd.numsr; srnum++)
	  if (gd.mcnum[srnum] > num)
	    break;
	srnum--;

	if (num < gd.mcnum[srnum] || num > gd.mcnum[srnum+1]) {
	  printf("\007  ERROR -- SR search failed!! ...aborting.\n"
		 "  mc minmc maxmc SRnum: %d %d %d %d\n",
		 num, gd.mcnum[srnum], gd.mcnum[srnum+1], srnum);
	  exit(-1);
	}
      }

      /* we know that the minicube lies in SuperRecord srnum */
      /* take a conservative guess at the correct record number */
      if ((num-gd.mcnum[srnum]) < (gd.mcnum[srnum+1]-num)) {
	recnum = 16*srnum +
	  15*(num - gd.mcnum[srnum])/(gd.mcnum[srnum+1]-gd.mcnum[srnum]+1);
      }
      else {
	recnum = 16*(srnum+1) -
	  17*(gd.mcnum[srnum+1] - num)/(gd.mcnum[srnum+1]-gd.mcnum[srnum]+1);
      }
      if (recnum>=gd.numrecs)
	recnum = gd.numrecs - 1;

      while (1) {
	fseek(gd.CubeFileIn, ((long)recnum)*4096+1024, SEEK_SET);
	if (!fread(&rec,4096,1,gd.CubeFileIn)) {
	  printf("\007  ERROR -- Could not read record %d... "
		 "aborting.\n", recnum);
	  exit(-1);
	}
	if (gd.swapbytes) {
	  swap4(&rec.h.minmc);
	  swap2(&rec.h.nummc);
	  swap2(&rec.h.offset);
	}
	/* printf("record number %d seeked and read...\n",recnum); */
	if (num >= rec.h.minmc)
	  break;
	recnum--;
/* printf(" Ooops, seeking backwards, record number %d...\n",recnum); */
      }
    }

    /* if we're not quite there yet, read forward in the file */
    while (num > rec.h.minmc+rec.h.nummc-1) {
      recnum++;
      if (!fread(&rec,4096,1,gd.CubeFileIn)) {
	printf("\007  ERROR -- Could not read record %d... "
	       "aborting.\n", recnum);
	exit(-1);
      }
      if (gd.swapbytes) {
	swap4(&rec.h.minmc);
	swap2(&rec.h.nummc);
	swap2(&rec.h.offset);
      }
      /* printf("record number %d read...\n",recnum); */
    }

    recloaded=1;
    mcptr = (unsigned char *)(&rec) + rec.h.offset;
    oldnum = rec.h.minmc;
  }

  /* ok, by now we've loaded the rec with our mc */
  if (num < oldnum) {
    mcptr = (unsigned char *)(&rec) + rec.h.offset;
    oldnum = rec.h.minmc;
  }
  skip = num - oldnum;
  for (i=0; i<skip; i++)
    mcptr += MCLEN(mcptr);
  if (mcptr > (unsigned char *)(&rec)+4096 ) {
    printf("\007  ERROR -- mcptr outside record!! ...aborting.\n"
	   "  mc minmc nummc recnum mcptr(-rec): %d %d %d %d %ld\n",
	   num, rec.h.minmc, rec.h.nummc, recnum,
	   (long int) (mcptr-(unsigned char *)(&rec)) );
    exit(-1);
  }

  oldnum = num;
  mcl=MCLEN(mcptr);
  if (mcl<1 || mcl>1024) {
    printf("Ack!! mclength = %d!\n"
	   "  mc minmc nummc recnum mcptr(-rec): %d %d %d %d %ld\n",
	   mcl, num, rec.h.minmc, rec.h.nummc, recnum,
	   (long int) (mcptr-(unsigned char *)(&rec)) );
    printf("  rec.h.offset = %d\n",rec.h.offset);
    mcptr = (unsigned char *)(&rec) + rec.h.offset;
    for (i=rec.h.minmc; i<=num; i++) {
      printf("mc, mcl, head: %d %d %d %d\n",i,MCLEN(mcptr),*mcptr,*(mcptr+1));
      mcptr += MCLEN(mcptr);
    }
    exit(-1);
  }

  if (mcptr + mcl > (unsigned char *)(&rec)+4096 ) {
    /* minicube spills over to next record */
    /* copy first part of minicube into cbuf */
    nbytes = (unsigned char *)(&rec) + 4096 - mcptr;
    memcpy (cbuf, mcptr, nbytes);

    /* now read in next record */
    recnum++;
    if (!fread(&rec,4096,1,gd.CubeFileIn)) {
      printf("\007  ERROR -- Could not read record %d... "
	     "aborting.\n", recnum);
      exit(-1);
    }
    if (gd.swapbytes) {
      swap4(&rec.h.minmc);
      swap2(&rec.h.nummc);
      swap2(&rec.h.offset);
    }
    /* printf("record number %d read...\n",recnum); */

    /* now copy second part of minicube to cbuf */
    memcpy ((cbuf+nbytes), (char *)(&rec)+8, mcl-nbytes);
    decompress3d (cbuf, mc);

    /* lastly fix up mcptr in case next call is for a mc in the same record */
    mcptr = (unsigned char *)(&rec) + rec.h.offset;
    oldnum = rec.h.minmc;
  }
  else
    decompress3d (mcptr,mc);

}

FILE * 
open_3cube (char *name)
{
  FILE *file;
  int length, i, nummc, sr;
  FHead3D head;
  Record3D rec;


  if (!name)
    return NULL;
  if (!(file=fopen(name,"r"))) {
    printf("\007  ERROR -- Cannot open file %s for reading.\n", name);
    return NULL;
  }
  if (!fread(&head,1024,1,file)) {
    printf("\007  ERROR -- Cannot read file %s... aborting.\n",name);
    fclose(file);
    return NULL;
  }
  if (head.cps > 6) {
    gd.swapbytes = 1;
    swap4(&head.numch);
    swap4(&head.bpc);
    swap4(&head.cps);
    swap4(&head.numrecs);
  }
  else
    gd.swapbytes = 0;

  if (!(strncmp(head.id,"Incub8r3/Pro4d  ",16)) && head.cps == 6)
    gd.cubetype = 0;
  else if (!(strncmp(head.id,"INCUB8R2        ",16)) && head.cps == 6)
    gd.cubetype = 2;
  else {
    printf("\007  ERROR -- Invalid header in file %s... aborting.\n",name);
    fclose(file);
    return NULL;
  }

  length = head.numch;
  gd.length = length;
  nummc = (length+7)/8;  /* minicubes per side */
  nummc = nummc*(nummc+1)*(nummc+2)/3; /* minicubes in cube */

  /* calculate look-up tables to convert (x,y,z) 3d cube address 
        to linearized 3d minicube address.  x<y<z
     lum{x,y,z}3d[ch] returns the number of subcubes to skip over
        to get to the subcube with ch in it.
     note that a 3d subcube on disk is 4x8x8 channels, but we always read them
        in pairs, i.e. we always read 8x8x8 channels.
     */
  for (i=0;i<8;i++) {
    lumx3d[i] = 0;
    lumy3d[i] = 0;
    lumz3d[i] = 0;
  }
  for (i=8;i<gd.length+16;i++) {
    lumx3d[i] = lumx3d[i-8] + 2;
    lumy3d[i] = lumy3d[i-8] + lumx3d[i];
    lumz3d[i] = lumz3d[i-8] + lumy3d[i];
  }

  printf("Axis length of cube is %d.\n", length);
  if (gd.cubetype>1)
    return file;

  gd.numrecs = head.numrecs;
  printf("Cube has %d minicubes and %d records.\n", nummc, gd.numrecs);
  gd.numsr = (head.numrecs + 15)/16;

  /* read SR list from end of file */
  while (!(gd.mcnum = malloc(4*(1+gd.numsr))))
    printf("trying to malloc gd.mcnum...\n");
  if ((fseek(file, (((long) head.numrecs)*4+1)*1024, SEEK_SET)) ||
	(!fread(gd.mcnum,4*(1+gd.numsr),1,file))) {

    /* build SuperRecord physical # and minicube # lookup table */
    printf("  Building SuperRecord list...\n");
    for (sr=0; sr<gd.numsr; sr++) {
      if ((fseek(file, (((long) sr)*64+1)*1024, SEEK_SET)) ||
	  (!fread(&rec,4096,1,file))) {
	printf("\007  ERROR -- Could not read Record %d... "
	       "aborting.\n", sr*16);
	fclose(file);
	return NULL;
      }
      gd.mcnum[sr] = rec.h.minmc;
    }
    gd.mcnum[gd.numsr] = nummc;
    if (gd.swapbytes)
      swap4(&gd.mcnum[gd.numsr]);
    printf("  ...Done building SuperRecord list,\n"
	   "       will now save it at end of cube file.\n");

    /* save SR list to end of file */
    fclose(file);
    if (!(file=fopen(name,"a+"))) {
      printf("\007  ERROR -- Cannot open file %s for writing.\n", name);
      return NULL;
    }
    if (!fwrite(gd.mcnum,4*(1+gd.numsr),1,file)) {
      printf("\007  ERROR -- Could not write SuperRecord list... aborting.\n");
      fclose(file);
      return NULL;
    }
    fflush(file);
  }
  if (gd.swapbytes) {
    for (sr=0; sr<=gd.numsr; sr++)
      swap4(&gd.mcnum[sr]);
  }

 return file;
}

int
main (int argc, char **argv)
{
  int length;  /* number of channels per side of cube */
  char innam[256], outnam[256], cbuf[256];
  unsigned int mc[8][8][8], dataOut[RW_MAXCH+7][8][8];
  unsigned short mc2[8][8][8];
  int i, j, x, y, z, ix, iy, iz;
  FILE *outfile;
  FHead3D head;
  Record3D *rec;
  int mcOut=0, numrecsOut=0;
  unsigned char *outptr;

  printf("\n\n"
	 "  Foldout  --  a program to fold out 1/6 triples cubes to 1/2 cubes,"
	 "               using the compressed-format RadWare cubes.\n\n"
	 "  Author     D.C Radford   Oct 1997\n\n"
	 "  Version    "VERSION"\n\n");

  while (1) {
    if (argc > 1) {
      strncpy(innam, argv[1], 70);
      argc = 0;
    } else {
      printf("Enter input cube file name (%s): ",RW_3DFILE);
      if (!fgets(innam, 250, stdin)) continue;
      innam[strlen(innam)-1] = 0;  /* remove \n */
      if (innam[0] == 0) {
	strncpy(innam, RW_3DFILE, 256);
      }
    }

    /* open input cube file */
    setfnamext(innam,".cub",0);
    if ((gd.CubeFileIn = open_3cube(innam)))
      break;
  }
  length = gd.length;

  while (1) {
    setfnamext(innam,"_foldout.cub",1);
    printf("Enter output cube file name (%s): ",innam);
    if (!fgets(outnam, 250, stdin)) continue;
    outnam[strlen(outnam)-1] = 0;  /* remove \n */
    if (outnam[0] == 0)
      strncpy(outnam, innam, 256);

    /* open output cube file */
    setfnamext(outnam,".cub",0);
    if ((outfile=fopen(outnam,"r+"))) {
      printf("\007  WARNING -- File %s exists. Overwrite it? (N/Y) ", outnam);
      if (!fgets(cbuf,200,stdin) ||
	  (cbuf[0] != 'y' && cbuf[0] != 'Y')) {
	fclose(outfile);
	continue;
      }
    }
    else if (!(outfile=fopen(outnam,"w+"))) {
      printf("\007  ERROR -- Cannot open file %s for writing.\n", outnam);
      continue;
    }

    /* write new output file header */
    strncpy(head.id, "Foldout         ", 16);
    head.numch = gd.length;
    head.bpc = 4;
    head.cps = 2;
    head.numrecs = 0;
    memset (head.resv, 0, 992);
    if (!fwrite(&head,1024,1,outfile)) {
      printf("\007  ERROR -- Could not write output cube file header..."
	     " aborting.\n");
      fclose(outfile);
      continue;
    }

    break;
  }

  /* malloc and prepare the output record */
  while (!(rec = malloc(5*1024)))
    printf("trying to malloc 5kB...\n");
  outptr = rec->d;
  rec->h.minmc = 0;
  rec->h.offset=8;

  printf("Working...\n");

  for (iz=0; iz<length; iz+=8) {
    for (iy=0; iy<=iz; iy+=8) {
      printf("\ry z: %4d %4d ",iy,iz);
      fflush(stdout);

      if (gd.cubetype==0) {  /* compressed 1/6 cube */

	/* read cube for x <= y <= z */
	for (ix=0; ix<=iy; ix+=8) {
	  j = LUMC(ix,iy,iz);
	  getmc(j,mc[0][0]);
	  getmc(j+1,mc[4][0]);
	  for (x=0; x<8; x++) {
	    for (y=0; y<8; y++) {
	      for (z=0; z<8; z++)
		dataOut[ix+x][z][y] = mc[z][y][x];
	    }
	  }
	}

	/* read cube for y <= x <= z */
	for (x=0; x<8; x++) {
	  for (y=0; y<8; y++) {
	    for (z=0; z<8; z++)
	      dataOut[iy+y][z][x] += mc[z][y][x];
	  }
	}
	for (ix=iy+8; ix<=iz; ix+=8) {
	  j = LUMC(iy,ix,iz);
	  getmc(j,mc[0][0]);
	  getmc(j+1,mc[4][0]);
	  for (x=0; x<8; x++) {
	    for (y=0; y<8; y++) {
	      for (z=0; z<8; z++)
		dataOut[ix+y][z][x] = mc[z][y][x];
	    }
	  }
	}

	/* read cube for y <= z <= x */
	for (x=0; x<8; x++) {
	  for (y=0; y<8; y++) {
	    for (z=0; z<8; z++)
	      dataOut[iz+z][y][x] += mc[z][y][x];
	  }
	}
	for (ix=iz+8; ix<length; ix+=8) {
	  j = LUMC(iy,iz,ix);
	  getmc(j,mc[0][0]);
	  getmc(j+1,mc[4][0]);
	  for (x=0; x<8; x++) {
	    for (y=0; y<8; y++) {
	      for (z=0; z<8; z++)
		dataOut[ix+z][y][x] = mc[z][y][x];
	    }
	  }
	}

      }
      else {  /* uncompressed 1/6 cube */

#define GETMC2() \
  if ((fseek(gd.CubeFileIn, (long)(j+2)*512, SEEK_SET)) || \
      (!fread(mc2[0][0],1024,1,gd.CubeFileIn))) { \
    printf("\007  ERROR -- Could not read record %d... " \
	   "aborting.\n", j/2); \
    exit(-1); \
  } \
  if (gd.swapbytes) { \
    for (i=0; i<512; i++) \
      swap2(mc2[0][0]+i); \
  }

	/* read cube for x <= y <= z */
	for (ix=0; ix<=iy; ix+=8) {
	  j = LUMC(ix,iy,iz);
	  GETMC2()
	  for (x=0; x<8; x++) {
	    for (y=0; y<8; y++) {
	      for (z=0; z<8; z++)
		dataOut[ix+x][z][y] = mc2[z][y][x];
	    }
	  }
	}

	/* read cube for y <= x <= z */
	for (x=0; x<8; x++) {
	  for (y=0; y<8; y++) {
	    for (z=0; z<8; z++)
	      dataOut[iy+y][z][x] += mc2[z][y][x];
	  }
	}
	for (ix=iy+8; ix<=iz; ix+=8) {
	  j = LUMC(iy,ix,iz);
	  GETMC2()
	  for (x=0; x<8; x++) {
	    for (y=0; y<8; y++) {
	      for (z=0; z<8; z++)
		dataOut[ix+y][z][x] = mc2[z][y][x];
	    }
	  }
	}

	/* read cube for y <= z <= x */
	for (x=0; x<8; x++) {
	  for (y=0; y<8; y++) {
	    for (z=0; z<8; z++)
	      dataOut[iz+z][y][x] += mc2[z][y][x];
	  }
	}
	for (ix=iz+8; ix<length; ix+=8) {
	  j = LUMC(iy,iz,ix);
	  GETMC2()
	  for (x=0; x<8; x++) {
	    for (y=0; y<8; y++) {
	      for (z=0; z<8; z++)
		dataOut[ix+z][y][x] = mc2[z][y][x];
	    }
	  }
	}
      }

      if (iy==iz) {
	for (j=0; j<length; j++) {
	  for (i=0; i<8; i++)
	    dataOut[j][i][i] *= 2;
	}
      }

      /* compress and write out foldout data */
      for (ix=0; ix<length; ix+=4) {
	j = compress3d(&dataOut[ix][0][0],outptr);
	outptr += j;
	mcOut++;
	if (outptr > &(rec->d[4086])) {
	  /* record is full, write it out */
	  rec->h.nummc = mcOut - rec->h.minmc;
	  if (!(fwrite(rec, 4096, 1, outfile))) {
	    printf("\007  ERROR -- Cannot write output cube, aborting...\n");
	    exit(-1);
	  }
	  numrecsOut++;
	  rec->h.minmc = mcOut;
	  rec->h.offset = 8 + outptr - &(rec->d[4088]);
	  if (rec->h.offset<8)
	    rec->h.offset = 8;
	  outptr = (unsigned char *)rec + rec->h.offset;
	  if (rec->h.offset>8)
	    memcpy(rec->d, &rec->d[4088], rec->h.offset-8);
	}
      }

    }
  }

  /* write out the last record */
  rec->h.nummc = mcOut - rec->h.minmc;
  if (!(fwrite(rec, 4096, 1, outfile))) {
    printf("\007  ERROR -- Cannot write output cube, aborting...\n");
    exit(-1);
  }
  numrecsOut++;

  rewind(outfile);
  if (!(fread(&head, 1024, 1, outfile))) {
    printf("\007  ERROR -- Cannot reread output cube header, aborting...\n");
    exit(-1);
  }
  head.numrecs = numrecsOut;
  rewind(outfile);
  if (!(fwrite(&head, 1024, 1, outfile))) {
    printf("\007  ERROR -- Cannot rewrite output cube header, aborting...\n");
    exit(-1);
  }

  fclose(outfile);
  printf("\n Congratulations! All seems to have finished okay.\n\n");
  return 0;
}
