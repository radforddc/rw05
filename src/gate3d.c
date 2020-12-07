/***************************************************************************
   Gate3D: Double-gating routines for RadWare triple coincidence cubes.
   D.C Radford      Oct 1997
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pro3d.h"
#include "util.h"

int lumx3d[RW_MAXCH+16]; /* look-up table, maps 3d ch to linear minicube */
int lumy3d[RW_MAXCH+16];
int lumz3d[RW_MAXCH+16];

/* look up minicube addr */
#define LUMC3D(x,y,z) lumx3d[x] + lumy3d[y] + lumz3d[z]

#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))

/* Some global data */
typedef struct {
  FILE *CubeFile3d;
  int length;     /* length of axis on cube */
  int nummc;      /* number of minicubes */
  int *mcnum;     /* SuperRecord minicube number table */
  int numrecs;    /* number of 4kB data records in the file */
  int cubetype;   /* 0 for compressed 1/6 cube,
		     1 for compressed 1/2 cube,
		     2 for uncompressed 1/6 cube, or
		     3 for uncompressed 1/2 cube  */
  int numsr;
  int swapbytes;
} CubeData3D;

/* stuff added for linear combinations of cubes: */
CubeData3D gd3dn[6], *gd3d = &gd3dn[0];
static int recloaded = 0;

#include "compress3d.c"

void setcubenum(int cubenum)
{
  gd3d = &gd3dn[cubenum];
  recloaded = 0;
}

void swap4(int *in)
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

void swap2(unsigned short *in)
{
  char *c, tmp;

  c = (char *) in;
  tmp = *c;
  *c = *(c+1);
  *(c+1) = tmp;
}

void getmc3d(int num, unsigned int mc[256])
{
  int i, skip, mcl, nbytes;
  static Record3D rec;
  static int recnum=0;      /* number of any currently loaded record */
  static int oldnum=0;      /* number of last minicube decompressed, if any */
  static int srnum=0;
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
	  num < gd3d->mcnum[srnum] ||
	  num > gd3d->mcnum[srnum+1]) {

	/* search for the record that contains minicube # num */
	/* first search in units of 64MB */
	for (srnum=1024; srnum<=gd3d->numsr; srnum+=1024)
	  if (gd3d->mcnum[srnum] > num) break;
	
	/* now search in units of 2MB */
	for (srnum-=1024; srnum<=gd3d->numsr; srnum+=32)
	  if (gd3d->mcnum[srnum] > num) break;

	/* now search in units of 1/16 MB */
	for (srnum-=32; srnum<=gd3d->numsr; srnum++)
	  if (gd3d->mcnum[srnum] > num) break;
	srnum--;

	if (num < gd3d->mcnum[srnum] || num > gd3d->mcnum[srnum+1]) {
	  warn1("\007  ERROR -- SR search failed!! ...aborting.\n"
	       "  mc minmc maxmc SRnum: %d %d %d %d\n",
	       num, gd3d->mcnum[srnum], gd3d->mcnum[srnum+1], srnum);
	  exit(-1);
	}
      }

      /* we know that the minicube lies in SuperRecord srnum */
      /* take a conservative guess at the correct record number */
      if ((num-gd3d->mcnum[srnum]) < (gd3d->mcnum[srnum+1]-num)) {
	recnum = 16*srnum + 15*(num - gd3d->mcnum[srnum])/
	  (gd3d->mcnum[srnum+1]-gd3d->mcnum[srnum]+1);
      }
      else {
	recnum = 16*(srnum+1) - 17*(gd3d->mcnum[srnum+1] - num)/
	  (gd3d->mcnum[srnum+1]-gd3d->mcnum[srnum]+1);
      }
      if (recnum >= gd3d->numrecs) recnum = gd3d->numrecs - 1;

      while (1) {
	fseek(gd3d->CubeFile3d, ((long)recnum)*4096+1024, SEEK_SET);
	if (!fread(&rec,4096,1,gd3d->CubeFile3d)) {
	  warn1("\007  ERROR -- Could not read record %d... aborting.\n"
	       "  mc minmc maxmc SRnum: %d %d %d %d\n", recnum,
	       num, gd3d->mcnum[srnum], gd3d->mcnum[srnum+1], srnum);
	  exit(-1);
	}
	if (gd3d->swapbytes) {
	  swap4(&rec.h.minmc);
	  swap2(&rec.h.nummc);
	  swap2(&rec.h.offset);
	}
	/* printf("record number %d seeked and read...\n",recnum); */
	if (num >= rec.h.minmc) break;
	recnum--;
/* printf(" Ooops, seeking backwards, record number %d...\n",recnum); */
      }
    }

   /* if we're not quite there yet, read forward in the file */
    while (num > rec.h.minmc+rec.h.nummc-1) {
      recnum++;
      if (!fread(&rec,4096,1,gd3d->CubeFile3d)) {
	warn1("\007  ERROR -- Could not read record %d... "
	     "aborting.\n", recnum);
	exit(-1);
      }
      if (gd3d->swapbytes) {
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
    warn1("\007  ERROR -- mcptr outside record!! ...aborting.\n"
	 "  mc SRnum minmc nummc recnum mcptr(-rec): %d %d %d %d %d %d\n",
	 num, srnum, rec.h.minmc, rec.h.nummc, recnum,
	 mcptr-(unsigned char *)(&rec) );
    exit(-1);
  }

  oldnum = num;
  mcl=MCLEN(mcptr);
  if (mcl<1 || mcl>1024) {
    warn1("Ack!! mclength = %d!\n"
	 "  mc SRnum minmc nummc recnum mcptr(-rec): %d %d %d %d %d %d\n",
	 mcl, num, srnum, rec.h.minmc, rec.h.nummc, recnum,
	 mcptr-(unsigned char *)(&rec) );
    warn1("  rec.h.offset = %d\n",rec.h.offset);
    mcptr = (unsigned char *)(&rec) + rec.h.offset;
    for (i=rec.h.minmc; i<=num; i++) {
      warn1("mc, mcl, head: %d %d %d %d\n",i,MCLEN(mcptr),*mcptr,*(mcptr+1));
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
    if (!fread(&rec,4096,1,gd3d->CubeFile3d)) {
      warn1("\007  ERROR -- Could not read record %d... "
	   "aborting.\n", recnum);
      exit(-1);
    }
    if (gd3d->swapbytes) {
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

int close3dfile(void)
{
  if (gd3d->CubeFile3d) {
    fclose(gd3d->CubeFile3d);
    free(gd3d->mcnum);
    gd3d->CubeFile3d = 0;
  }
  return 0;
}

int open3dfile(char *name, int *numchs_ret)
{
  int length, i, nummc, sr;
  FHead3D head;
  Record3D rec;
  char cname[256], *tmp;

  strcpy(cname, name);
  if ((tmp = strchr(cname,' '))) *tmp = 0;

  if (!gd3d) gd3d = &gd3dn[0];

  if (!cname[0] || !(gd3d->CubeFile3d=fopen(cname,"r"))) {
    warn1("\007  ERROR -- Cannot open file %s for reading.\n", cname);
    return 1;
  }

#define QUIT close3dfile(); return 1;
  if (!fread(&head,1024,1,gd3d->CubeFile3d)) {
    warn1("\007  ERROR -- Cannot read file %s.\n",name);
    QUIT;
  }
  if (head.cps > 6) {
    gd3d->swapbytes = 1;
    swap4(&head.numch);
    swap4(&head.bpc);
    swap4(&head.cps);
    swap4(&head.numrecs);
  }
  else
    gd3d->swapbytes = 0;

  if (!(strncmp(head.id,"Incub8r3/Pro4d  ",16)) && head.cps == 6)
    gd3d->cubetype = 0;
  else if (!(strncmp(head.id,"Foldout         ",16)) && head.cps == 2)
    gd3d->cubetype = 1;
  else if (!(strncmp(head.id,"INCUB8R2        ",16)) && head.cps == 6)
    gd3d->cubetype = 2;
  else if (!(strncmp(head.id,"FOLDOUT         ",16)) && head.cps == 2)
    gd3d->cubetype = 3;
  else {
    warn1("\007  ERROR -- Invalid header in file %s.\n",name);
    QUIT;
  }

  length = head.numch;
  *numchs_ret = length;
  gd3d->length = length;
  if (length > RW_MAXCH) {
    warn1("ERROR: Number of channels in cube (%d)\n"
	 "         is greater than RW_MAXCH (%d) in pro3d.h\n",
	 length, RW_MAXCH);
    exit(-1);
  }
  gd3d->numrecs = head.numrecs;
  nummc = (length+7)/8;  /* minicubes per side */
  if (gd3d->cubetype==0 || gd3d->cubetype==2)
    nummc = nummc*(nummc+1)*(nummc+2)/3; /* minicubes in cube */
  else if (gd3d->cubetype==1 || gd3d->cubetype==3)
    nummc = (nummc*(nummc+1)/2) * ((length+3)/4); /* minicubes in cube */
  gd3d->nummc = nummc;

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
  for (i=8;i<gd3d->length+16;i++) {
    lumx3d[i] = lumx3d[i-8] + 2;
    lumy3d[i] = lumy3d[i-8] + lumx3d[i];
    lumz3d[i] = lumz3d[i-8] + lumy3d[i];
  }

  tell("Axis length of cube is %d.\n", length);
  if (gd3d->cubetype>1) return 0;

  gd3d->numrecs = head.numrecs;
  tell("Cube has %d minicubes and %d records.\n", nummc, gd3d->numrecs);
  gd3d->numsr = (head.numrecs + 15)/16;

  /* read SR list from end of file */
  while (!(gd3d->mcnum = malloc(4*(1+gd3d->numsr))))
    tell("trying to malloc gd3d->mcnum...\n");
  if ((fseek(gd3d->CubeFile3d, (((long) head.numrecs)*4+1)*1024, SEEK_SET)) ||
	(!fread(gd3d->mcnum,4*(1+gd3d->numsr),1,gd3d->CubeFile3d))) {

    /* build SuperRecord physical # and minicube # lookup table */
    tell("  Building 3d SuperRecord list...\n");
    for (sr=0; sr<gd3d->numsr; sr++) {
      if ((fseek(gd3d->CubeFile3d, (((long) sr)*64+1)*1024, SEEK_SET)) ||
	  (!fread(&rec,4096,1,gd3d->CubeFile3d))) {
	warn1("\007  ERROR -- Could not read Record %d... aborting.\n", sr*16);
	QUIT;
      }
      gd3d->mcnum[sr] = rec.h.minmc;
    }
    gd3d->mcnum[gd3d->numsr] = nummc;
    if (gd3d->swapbytes)
      swap4(&gd3d->mcnum[gd3d->numsr]);
    tell("  ...Done building SuperRecord list,\n"
	 "       will now save it at end of cube file.\n");

    /* save SR list to end of file */
    fclose(gd3d->CubeFile3d);
    if (!(gd3d->CubeFile3d=fopen(cname,"a+"))) {
      warn1("\007  ERROR -- Cannot open file %s for writing.\n", name);
      QUIT;

    }
    if (!fwrite(gd3d->mcnum,4*(1+gd3d->numsr),1,gd3d->CubeFile3d)) {
      warn1("\007  ERROR -- Could not write SuperRecord list... aborting.\n");
      QUIT;
    }
    fflush(gd3d->CubeFile3d);
  }
  if (gd3d->swapbytes) {
    for (sr=0; sr<=gd3d->numsr; sr++)
      swap4(&gd3d->mcnum[sr]);
  }
#undef QUIT
  return 0;
}

int read3dcube(int lo1, int hi1, int lo2, int hi2, int *spec_ret)
{
  int lim[2][2];
  int i, j, k, tmp, x, y, z, iy, iz;
  int ymc, zmc, ylo, zlo, yhi, zhi;
  int x1, y1, y2, z1, z2, yy1, yy2, zz1, zz2;
  unsigned int mc[8][8][8];
  unsigned short mc2[8][8][8];
  unsigned char lobyte[RW_MAXCH+6];
  unsigned short overflows[RW_MAXCH][2], numoverflows;
  int overflowaddr;
  int mcnum;
  int length;  /* number of channels per side of cube */


  length = gd3d->length;
  memset(spec_ret, 0, 4*gd3d->length);

  lim[0][0] = lo1; lim[0][1] = hi1;
  lim[1][0] = lo2; lim[1][1] = hi2;
  for (i=0; i<2; i++) {
    if (lim[i][0]>lim[i][1]) {
      tmp = lim[i][0];
      lim[i][0] = lim[i][1];
      lim[i][1] = tmp;
    }
    if (lim[i][0]<0 || lim[i][1] >= gd3d->length) {
      warn1(" OVERFLOW.  The valid range is 0 to %d.\n", gd3d->length-1);
      return 0;
    }
  }

  /* order the lower limits of the gates */
  if (lim[0][0]>lim[1][0]) {
    tmp = lim[1][0];
    lim[1][0] = lim[0][0];
    lim[0][0] = tmp;
    tmp = lim[1][1];
    lim[1][1] = lim[0][1];
    lim[0][1] = tmp;
  }

  /* now go read the cube to get the gate */

  if (gd3d->cubetype==3) {  /* uncompressed 1/2 cube */
    j = 1024;
    k = 1 + ((length*(length+1)/2 * (length+6) + 1023) / 1024);
    k = k * 1024;
    for (iz=lim[1][0]; iz<=lim[1][1]; iz++) {
      for (iy=lim[0][0]; iy<=lim[0][1]; iy++) {
	if (iy<=iz)
	  j = 1024 + (iz*(iz+1)/2 + iy)*(length+6);
	else
	  j = 1024 + (iy*(iy+1)/2 + iz)*(length+6);
	fseek(gd3d->CubeFile3d, ((long)j), SEEK_SET);
	if (!fread(lobyte,length+6,1,gd3d->CubeFile3d)) {
	  warn1("\007  ERROR -- Could not read cube file... aborting.\n");
	  exit(-1);
	}
	for (x=0; x<length; x++) {
	  spec_ret[x] += (int)lobyte[x];
	}

	memcpy(&numoverflows, &lobyte[length], 2);
	if (numoverflows == 0)
	  continue;
	memcpy(&overflowaddr, &lobyte[length+2], 4);
	if (gd3d->swapbytes) {
	  swap2(&numoverflows);
	  swap4(&overflowaddr);
	}
	fseek(gd3d->CubeFile3d, (long)k+(long)(overflowaddr*4), SEEK_SET);
	if (!fread(overflows,4*numoverflows,1,gd3d->CubeFile3d)) {
	  warn1("\007  ERROR -- Could not read cube file... aborting.\n");
	  exit(-1);
	}
	for (x=0; x<numoverflows; x++) {
	  if (gd3d->swapbytes) {
	    swap2(&overflows[x][0]);
	    swap2(&overflows[x][1]);
	  }
	  /*  ACKK!! A bug!     DCR   3-April-1998
           *	  spec_ret[overflows[x][0]] += (int)overflows[x][1];
           */
	  spec_ret[overflows[x][0]-1] += (int)overflows[x][1];
	}
      }
    }
    return 0;
  }

  /* remember the data are organised as minicubes, 8 x 8 x 4 chs */ 
  for (zmc = lim[1][0]>>3; zmc<=lim[1][1]>>3; zmc++) {
    zlo = zmc*8;
    zhi = zlo+7;
    if (zlo<lim[1][0])
      zlo = lim[1][0];
    if (zhi>lim[1][1])
      zhi = lim[1][1];

    for (ymc = lim[0][0]>>3; ymc<=lim[0][1]>>3; ymc++) {
      ylo = ymc*8;
      yhi = ylo+7;
      if (ylo<lim[0][0])
	ylo = lim[0][0];
      if (yhi>lim[0][1])
	yhi = lim[0][1];

      if (ylo>zlo) {
	y1 = zlo;
	y2 = zhi;
	z1 = ylo;
	z2 = yhi;
      }
      else{
	y1 = ylo;
	y2 = yhi;
	z1 = zlo;
	z2 = zhi;
      }
      /* (y,z) gate is now ((y1,y2),(z1,z2)) */
      yy1 = y1&7;
      yy2 = y2&7;
      zz1 = z1&7;
      zz2 = z2&7;

      if (gd3d->cubetype==1) {  /* compressed 1/2 cube */
	/* j = first minicube to read */
	j = ((y1/8) + ((z1/8)*((z1/8)+1))/2) * ((length+3)/4);
	for (x1=0; x1<length; x1+=8) {
	  getmc3d(j++,mc[0][0]);
	  getmc3d(j++,mc[4][0]);
	  for (z=zz1; z<=zz2; z++) {
	    for (y=yy1; y<=yy2; y++) {
	      for (x=0; x<8; x++)
		spec_ret[x1+x] += (int)mc[x][z][y];
	    }
	  }
	  if (y2>=z1) {
	    for (z=yy1; z<=yy2; z++) {
	      for (y=zz1; y<z; y++) {
		for (x=0; x<8; x++)
		  spec_ret[x1+x] += (int)mc[x][z][y];
	      }
	    }
	  }
	}
      }

      else if (gd3d->cubetype==0) { /* compressed 1/6 cube */
	/* read gate for x <= y */
	for (x1=0; x1<=y1; x1+=8) {
	  mcnum = LUMC3D(x1,y1,z1);
	  getmc3d(mcnum, mc[0][0]);
	  getmc3d(mcnum+1, mc[4][0]);
	  for (z=zz1; z<=zz2; z++) {
	    for (y=yy1; y<=yy2; y++) {
	      for (x=0; x<8; x++)
		spec_ret[x1+x] += mc[z][y][x];
	    }
	  }
	  if (y2>=z1) {
	    for (z=yy1; z<=yy2; z++) {
	      for (y=zz1; y<=z; y++) {
		for (x=0; x<8; x++)
		  spec_ret[x1+x] += mc[z][y][x];
	      }
	    }
	  }
	}

	/* read gate for y <= x <= z */
	for (x1=(y1>>3)<<3; x1<=z1; x1+=8) {
	  mcnum = LUMC3D(y1,x1,z1);
	  getmc3d(mcnum, mc[0][0]);
	  getmc3d(mcnum+1, mc[4][0]);
	  for (z=zz1; z<=zz2; z++) {
	    for (y=yy1; y<=yy2; y++) {
	      for (x=0; x<8; x++)
		spec_ret[x1+x] += mc[z][x][y];
	    }
	  }
	  if (y2>=z1) {
	    for (z=yy1; z<=yy2; z++) {
	      for (y=zz1; y<=z; y++) {
		for (x=0; x<8; x++)
		  spec_ret[x1+x] += mc[z][x][y];
	      }
	    }
	  }
	}

	  /* read gate for z <= x */
	for (x1=(z1>>3)<<3; x1<length; x1+=8) {
	  mcnum = LUMC3D(y1,z1,x1);
	  getmc3d(mcnum, mc[0][0]);
	  getmc3d(mcnum+1, mc[4][0]);
	  for (z=zz1; z<=zz2; z++) {
	    for (y=yy1; y<=yy2; y++) {
	      for (x=0; x<8; x++)
		spec_ret[x1+x] += mc[x][z][y];
	    }
	  }
	  if (y2>=z1) {
	    for (z=yy1; z<=yy2; z++) {
	      for (y=zz1; y<=z; y++) {
		for (x=0; x<8; x++)
		  spec_ret[x1+x] += mc[x][z][y];
	      }
	    }
	  }
	}
      }

#define GETMC2() \
   if ((fseek(gd3d->CubeFile3d, (long)(mcnum+2)*512, SEEK_SET)) || \
       (!fread(mc2[0][0],1024,1,gd3d->CubeFile3d))) { \
     warn1("\007 ERROR -- Could not read rec %d... aborting.\n", mcnum/2); \
     exit(-1); \
   } \
   if (gd3d->swapbytes) { \
     for (i=0; i<512; i++) \
       swap2(mc2[0][0]+i); \
   }

      else if (gd3d->cubetype==2) { /* uncompressed 1/6 cube */
	/* read gate for x <= y */
	for (x1=0; x1<=y1; x1+=8) {
	  mcnum = LUMC3D(x1,y1,z1);
	  GETMC2()
	  for (z=zz1; z<=zz2; z++) {
	    for (y=yy1; y<=yy2; y++) {
	      for (x=0; x<8; x++)
		spec_ret[x1+x] += mc2[z][y][x];
	    }
	  }
	  if (y2>=z1) {
	    for (z=yy1; z<=yy2; z++) {
	      for (y=zz1; y<=z; y++) {
		for (x=0; x<8; x++)
		  spec_ret[x1+x] += mc2[z][y][x];
	      }
	    }
	  }
	}

	/* read gate for y <= x <= z */
	for (x1=(y1>>3)<<3; x1<=z1; x1+=8) {
	  mcnum = LUMC3D(y1,x1,z1);
	  GETMC2()
	  for (z=zz1; z<=zz2; z++) {
	    for (y=yy1; y<=yy2; y++) {
	      for (x=0; x<8; x++)
		spec_ret[x1+x] += mc2[z][x][y];
	    }
	  }
	  if (y2>=z1) {
	    for (z=yy1; z<=yy2; z++) {
	      for (y=zz1; y<=z; y++) {
		for (x=0; x<8; x++)
		  spec_ret[x1+x] += mc2[z][x][y];
	      }
	    }
	  }
	}

	/* read gate for z <= x */
	for (x1=(z1>>3)<<3; x1<length; x1+=8) {
	  mcnum = LUMC3D(y1,z1,x1);
	  GETMC2()
	  for (z=zz1; z<=zz2; z++) {
	    for (y=yy1; y<=yy2; y++) {
	      for (x=0; x<8; x++)
		spec_ret[x1+x] += mc2[x][z][y];
	    }
	  }
	  if (y2>=z1) {
	    for (z=yy1; z<=yy2; z++) {
	      for (y=zz1; y<=z; y++) {
		for (x=0; x<8; x++)
		  spec_ret[x1+x] += mc2[x][z][y];
	      }
	    }
	  }
	}
      }

    }
  }
  return 0;
}
