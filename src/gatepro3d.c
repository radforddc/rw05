/***************************************************************************
   PRO3D: 3D-to-2D and 3D-to-1D projection program for
       compressed-format RadWare triple coincidence cubes.
       Generates .2dp  and .spe files, optionally
       gated by a .tab look-up gate file (see gf3 help).

   D.C Radford      August 2004
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
#include "util.h"

#define VERSION "August 2004"

int lumx3d[RW_MAXCH+16]; /* look-up table, maps 3d ch to linear minicube */
int lumy3d[RW_MAXCH+16];
int lumz3d[RW_MAXCH+16];

  /* look up minicube addr */
#define LUMC(x,y,z) lumx3d[x] + lumy3d[y] + lumz3d[z]

#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))

/* Some global data */
struct GlobalData {
  FILE *CubeFile3d;
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

  printf("swap4 called..\n");
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

  printf("swap2 called..\n");
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
	fseek(gd.CubeFile3d, ((long)recnum)*4096+1024, SEEK_SET);
	if (!fread(&rec,4096,1,gd.CubeFile3d)) {
	  printf("\007  ERROR -- Could not read record %d... "
		 "aborting.\n", recnum);
	  printf("  mc minmc maxmc SRnum: %d %d %d %d\n",
		 num, gd.mcnum[srnum], gd.mcnum[srnum+1], srnum);
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
      if (!fread(&rec,4096,1,gd.CubeFile3d)) {
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
	   "  mc minmc nummc recnum mcptr(-rec): %d %d %d %d %d\n",
	   num, rec.h.minmc, rec.h.nummc, recnum,
	   mcptr-(unsigned char *)(&rec) );
    exit(-1);
  }

  oldnum = num;
  mcl=MCLEN(mcptr);
  if (mcl<1 || mcl>1024) {
    printf("Ack!! mclength = %d!\n"
	   "  mc minmc nummc recnum mcptr(-rec): %d %d %d %d %d\n",
	   mcl, num, rec.h.minmc, rec.h.nummc, recnum,
	   mcptr-(unsigned char *)(&rec) );
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
    if (!fread(&rec,4096,1,gd.CubeFile3d)) {
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
  } else {
    decompress3d (mcptr,mc);
  }
}

FILE * 
open_3cube (char *name)
{
  FILE *file;
  int length, i, nummc, sr;
  FHead3D head;
  Record3D rec;


  if (!name) return NULL;
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
  } else {
    gd.swapbytes = 0;
  }
  printf("swapbytes = %d\n", gd.swapbytes);

  if (!(strncmp(head.id,"Incub8r3/Pro4d  ",16)) && head.cps == 6)
    gd.cubetype = 0;
  else if (!(strncmp(head.id,"Foldout         ",16)) && head.cps == 2)
    gd.cubetype = 1;
  else if (!(strncmp(head.id,"INCUB8R2        ",16)) && head.cps == 6)
    gd.cubetype = 2;
  else if (!(strncmp(head.id,"FOLDOUT         ",16)) && head.cps == 2)
    gd.cubetype = 3;
  else {
    printf("\007  ERROR -- Invalid header in file %s... aborting.\n",name);
    fclose(file);
    return NULL;
  }

  length = head.numch;
  gd.length = length;
  nummc = (length+7)/8;  /* minicubes per side */
  if (gd.cubetype==0 || gd.cubetype==2)
    nummc = nummc*(nummc+1)*(nummc+2)/3; /* minicubes in cube */
  else if (gd.cubetype==1 || gd.cubetype==3)
    nummc = (nummc*(nummc+1)/2) * ((length+3)/4); /* minicubes in cube */

  printf("Axis length of cube is %d.\n", length);
  if (gd.cubetype > 1) return file;

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
    if (gd.swapbytes) swap4(&gd.mcnum[gd.numsr]);
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
 
  return file;
}

int
main (int argc, char **argv)
{
  int length;  /* number of channels per side of cube */
  char c3nam[256], lunam[256], outnam[256], cbuf[256];
  unsigned int mc[512], *data2d;
  unsigned short mc2[512];
  float data1d[RW_MAXCH];
  int i, j, k, x, y, z, ix, iy, iz;
  int ax[512], ay[512], az[512];
  FILE *file;
  unsigned char lobyte[RW_MAXCH+6];
  unsigned short overflows[RW_MAXCH][2], numoverflows;
  int overflowaddr;
  short lugate[16384];


  printf("\n\n"
	 "  Pro3d  --  a 3D-to-2D and 3D-to-1D projection program for\n"
	 "             compressed-format RadWare triple coincidence cubes.\n\n"
	 "             It generates .2dp and .spe files, optionally\n"
	 "             gated by a .tab look-up gate file (see gf3 help)\n\n"
	 "  Author     D.C Radford    August 2004\n\n"
	 "  Version    "VERSION"\n\n");

  while (1) {
    if (argc > 1 && *argv[1]) {
      strncpy(c3nam, argv[1], 70);
      *argv[1] = 0;
    } else {
      printf("Enter input 3D cube file name (%s): ",RW_3DFILE);
      fgets(c3nam, 250, stdin);
      c3nam[strlen(c3nam)-1] = 0;  /* remove \n */
      if (c3nam[0] == 0) strncpy(c3nam, RW_3DFILE, 256);
    }

    /* open 3d cube */
    setfnamext(c3nam,".cub",0);
    if ((gd.CubeFile3d = open_3cube(c3nam))) break;
  }
  length = gd.length;

  while (1) {
    memset(lugate, 1, 4*length);
    if (argc > 2 && *argv[2]) {
      strncpy(lunam, argv[2], 70);
      *argv[2] = 0;
    } else {
      printf("Enter input gate lookup table file name\n"
	     "                  (return for no gating) ");
      fgets(lunam, 250, stdin);
      lunam[strlen(lunam)-1] = 0;  /* remove \n */
      if (strlen(lunam) < 1) break;
    }
    /* read gate lookup table */
    setfnamext(lunam,".tab",0);
    if (!read_tab(lunam, &i, &j, &k, lugate)) break;
  }

  /* malloc the 2D projection space */
  while (!(data2d=malloc(4*(length+8)*length))) {
    printf("Ooops, data2d malloc failed...\n"
	   "  ... please free up some memory and press return.\n");
    fgets(cbuf,256,stdin);
  }

  /* calculate the mappings from 3d channel space to 2d channel space */
  i = 0;
  for (z=0; z<8; z++) {
    for (y=0; y<8; y++) {
      for (x=0; x<8; x++) {
	ax[i] = x;
	ay[i] = y;
	az[i] = z;
	i++;
      }
    }
  }

  memset(data2d, 0, 4*(length+8)*length);
  printf("Projecting to 2D...\n");

  if (gd.cubetype==0) {  /* compressed 1/6 cube */
    for (iz=0; iz<length; iz+=8) {
      for (iy=0; iy<=iz; iy+=8) {
	for (ix=0; ix<=iy; ix+=8) {
	  j = LUMC(ix,iy,iz);
	  getmc(j,mc);
	  getmc(j+1,&mc[256]);
	  if ((j&511)==0) {
	    printf("\rx y z: %4d %4d %4d ",ix,iy,iz);
	    fflush(stdout);
	  }
	  for (i=0; i<512; i++) {
	    if (lugate[iz+az[i]]) *(data2d + (iy+ay[i])*length + ix+ax[i]) += mc[i];
	    if (lugate[iy+ay[i]]) *(data2d + (iz+az[i])*length + ix+ax[i]) += mc[i];
	    if (lugate[ix+ax[i]]) *(data2d + (iz+az[i])*length + iy+ay[i]) += mc[i];
	  }
	}
      }
    }
  }

  else if (gd.cubetype==1) {  /* compressed 1/2 cube */
    j = 0;
    for (iz=0; iz<length; iz+=8) {
      for (iy=0; iy<=iz; iy+=8) {
	for (ix=0; ix<length; ix+=8) {
	  getmc(j++,mc);
	  if ((ix+4) < length) {
	    getmc(j++,&mc[256]);
	  } else {
	    memset(&mc[256], 0, 1024);
	  }
	  if ((j&511)==0) {
	    printf("\rx y z: %4d %4d %4d ",ix,iy,iz);
	    fflush(stdout);
	  }
	  for (i=0; i<512; i++)
	    if (lugate[ix+az[i]]) *(data2d + (iz+ay[i])*length + iy+ax[i]) += mc[i];
	}
      }
    }
    for (iz=0; iz<length; iz++)
      *(data2d + iz*length + iz) = *(data2d + iz*length + iz)/2;
  }

  else if (gd.cubetype==2) {  /* uncompressed 1/6 cube */
    j = 0;
    for (iz=0; iz<length; iz+=8) {
      for (iy=0; iy<=iz; iy+=8) {
	for (ix=0; ix<=iy; ix+=8) {
	  if (!fread(mc2,1024,1,gd.CubeFile3d)) {
	    printf("\007  ERROR -- Could not read record %d... "
		   "aborting.\n", j);
	    exit(-1);
	  }
	  if (gd.swapbytes) {
	    for (i=0; i<512; i++)
	      swap2(&mc2[i]);
	  }
	  if (((j++)&255)==0) {
	    printf("\rx y z: %4d %4d %4d ",ix,iy,iz);
	    fflush(stdout);
	  }
	  for (i=0; i<512; i++) {
	    if (lugate[iz+az[i]]) *(data2d + (iy+ay[i])*length + ix+ax[i]) += mc2[i];
	    if (lugate[iy+ay[i]]) *(data2d + (iz+az[i])*length + ix+ax[i]) += mc2[i];
	    if (lugate[ix+ax[i]]) *(data2d + (iz+az[i])*length + iy+ay[i]) += mc2[i];
	  }
	}
      }
    }
  }

  else if (gd.cubetype==3) {  /* uncompressed 1/2 cube */
    j = 1024;
    k = 1 + ((length*(length+1)/2 * (length+6) + 1023) / 1024);
    k = k * 1024;
    for (iz=0; iz<length; iz++) {
      printf("\r z: %4d ",iz);
      fflush(stdout);
      for (iy=0; iy<=iz; iy++) {
	fseek(gd.CubeFile3d, ((long)j), SEEK_SET);
	if (!fread(lobyte,length+6,1,gd.CubeFile3d)) {
	  printf("\007  ERROR -- Could not read cube file... aborting.\n");
	  exit(-1);
	}
	j += length+6;
	for (ix=0; ix<length; ix++)
	  if (lugate[ix]) *(data2d + iz*length + iy) += lobyte[ix];

	memcpy(&numoverflows, &lobyte[length], 2);
	if (numoverflows == 0)
	  continue;
	memcpy(&overflowaddr, &lobyte[length+2], 4);
	if (gd.swapbytes) {
	  swap2(&numoverflows);
	  swap4(&overflowaddr);
	}
	fseek(gd.CubeFile3d, (long)k+(long)(overflowaddr*4), SEEK_SET);
	if (!fread(overflows,4*numoverflows,1,gd.CubeFile3d)) {
	  printf("\007  ERROR -- Could not read cube file... aborting.\n");
	  exit(-1);
	}
	for (ix=0; ix<numoverflows; ix++) {
	  if (lugate[ix]) {
	    if (gd.swapbytes) swap2(&overflows[ix][1]);
	    *(data2d + iz*length + iy) += (int)overflows[ix][1];
	  }
	}
      }
      *(data2d + iz*length + iz) = *(data2d + iz*length + iz)/2;
    }
  }

  printf("\n ...done.\n");

  /* write out 2d projection */
  while (1) {
    setfnamext(c3nam,".2dp",1);
    printf("Enter output 2D file name (%s): ",c3nam);
    fgets(outnam, 250, stdin);
    outnam[strlen(outnam)-1] = 0;  /* remove \n */
    if (outnam[0] == 0) strncpy(outnam, c3nam, 256);

    /* open 2d file */
    setfnamext(outnam,".2dp",0);
    if ((file=fopen(outnam,"r+"))) {
      printf("\007  WARNING -- File %s exists. Overwrite it? (N/Y) ", outnam);
      fgets(cbuf,200,stdin);
      if (cbuf[0] != 'y' && cbuf[0] != 'Y') {
	fclose(file);
	continue;
      }
    }
    else if (!(file=fopen(outnam,"w+"))) {
      printf("\007  ERROR -- Cannot open file %s for writing.\n", outnam);
      continue;
    }
    break;
  }

  for (y=0; y<length; y++) {
    for (x=0; x<=y; x++) {
      data1d[x] = (float)(*(data2d + y*length + x));
    }
    data1d[y] = 2.0*data1d[y];
    for (x=y+1; x<length; x++) {
      data1d[x] = (float)(*(data2d + x*length + y));
    }
    if (!(fwrite(data1d, 4*length, 1, file))) {
      printf("\007  ERROR -- Cannot write 2d file, aborting...\n");
      exit(-1);
    }
  }
  fclose(file);

  printf("Projecting to 1D...\n");
  for (y=0; y<length; y++) {
    data1d[y] = 0.0;
    for (x=0; x<=y; x++) {
      data1d[x] += (float)(*(data2d + y*length + x));
      data1d[y] += (float)(*(data2d + y*length + x));
    }
  }

  /* write out 1d projection */
  while (1) {
    setfnamext(c3nam,".spe",1);
    printf("Enter output 1D file name (%s): ",c3nam);
    fgets(outnam, 250, stdin);
    outnam[strlen(outnam)-1] = 0;  /* remove \n */
    if (outnam[0] == 0) strncpy(outnam, c3nam, 256);
    if (!(wspec(outnam, data1d, length))) break;
  }

  printf("\n"
	 "Congratulations! All seems to have finished okay.\n"
	 "  If you haven't already done so, you should probably now run\n"
	 "    foldout on the 3d cube\n\n");
  return 0;
}
