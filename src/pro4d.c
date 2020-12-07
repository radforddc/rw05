/***************************************************************************
   PRO4D: 4D-to-3D projection program for quadruple coincidence hybercubes.
   This version for 1/24 hybercubes.
   D.C Radford,  C.J. Beyer    Aug 1997
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "pro4d.h"
#include "rwutils.c"

#define VERSION "29 Aug 97"

int lumw[RW_MAXCH];   /* look-up table, maps 4d cube ch to linear minicube */
int lumx[RW_MAXCH];
int lumy[RW_MAXCH];
int lumz[RW_MAXCH];
int lumx3d[RW_MAXCH+16]; /* look-up table, maps 3d ch to linear minicube */
int lumy3d[RW_MAXCH+16];
int lumz3d[RW_MAXCH+16];

  /* look up minicube addr */
#define LUMC(w,x,y,z) lumw[w] + lumx[x] + lumy[y] + lumz[z]
  /* look up addr in minicube */
#define LUIMC(w,x,y,z) (w&3) + ((x&3)<<2) + ((y&3)<<4) + ((z&3)<<6)
  /* look up 3d addr */
#define ADDR3D(x,y,z)\
      (((lumx3d[x]+lumy3d[y]+lumz3d[z])<<8) + (x&7) + ((y&7)<<3) + ((z&3)<<6))

#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))

typedef struct {
  short filenum;
  short srloc;
  int mcnum[17];
} SRlus;

/* Some global data */
struct GlobalData {
  FILE *CubeDirFile;
  FILE *CubeFile[RW_MAX_FILES];
  char CubeFileName[RW_MAX_FILES][256];
  int  numCubeFiles;
  int  mclo[RW_MAX_FILES], mchi[RW_MAX_FILES];
  int  length;     /* length of axis on cube */
  int  nummc;      /* number of minicubes */
  SRlus *srlut;              /* SR table */
  int  numsr[RW_MAX_FILES];  /* current number of SuperRecords */
  int  totnumsr;             /* current total number of SuperRecords */
  FILE *CubeFile3d;
  int  swapbytes;
} gd;

#include "compress4d.c"
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
getmc(int num, unsigned short mc[256])
{
  int i, skip, mcl, nbytes;
  static int recloaded = 0;
  static int recnum;
  static Record rec;
  static int oldnum=0;      /* number of last minicube decompressed, if any */
  static int srnum=0;       /* number of current SuperRecord */
  static int ssrnum=0;      /* number of current 1/16 SuperRecord */
  static unsigned char *mcptr;
  unsigned char cbuf[1024];

  if ((!recloaded) ||
      num < rec.h.minmc ||
      num > rec.h.minmc+rec.h.nummc-1) {

    /* no record is loaded, or it does not have the required minicube */

    /* first check to see that the required minicube does not */
    /* lie just ahead of any currently loaded record */
    /* if it does, we can just read ahead in the file, rather than fseeking */
    if ((!recloaded) ||
	num < rec.h.minmc ||
	num > rec.h.minmc+4*rec.h.nummc-1 ||
	num > gd.srlut[srnum].mcnum[16]) {

      /* next look to see if it lies outside any current SuperRecord */
      if ((!recloaded) ||
	  num < gd.srlut[srnum].mcnum[0] ||
	  num > gd.srlut[srnum].mcnum[16]) {

	/* search for the record that contains minicube # num */
	/* first search in units of 64MB */
	for (srnum=64; srnum<gd.totnumsr; srnum+=64) {
	  if (gd.srlut[srnum].mcnum[0] > num)
	    break;
	}
	
	/* now search in units of 1MB */
	for (srnum-=64; srnum<gd.totnumsr; srnum++) {
	  if (gd.srlut[srnum].mcnum[0] > num)
	    break;
	}
	srnum--;
      }
      if (num < gd.srlut[srnum].mcnum[0] ||
	  num > gd.srlut[srnum].mcnum[16]) {
	printf("\007  ERROR -- SR search failed!! ...aborting.\n"
	       "  mc minmc maxmc SRnum: %d %d %d %d\n",
	       num, gd.srlut[srnum].mcnum[0],
	       gd.srlut[srnum].mcnum[16], srnum);
	exit(-1);
      }

      /* we know that the minicube lies in SuperRecord srnum */
      /* now search in units of 1/16 MB (64 records) */
      for (ssrnum=1; ssrnum<16; ssrnum++) {
	if (gd.srlut[srnum].mcnum[ssrnum] > num)
	  break;
      }
      ssrnum--;
      if (num < gd.srlut[srnum].mcnum[ssrnum] ||
	  num >= gd.srlut[srnum].mcnum[ssrnum+1]) {
	printf("\007  ERROR -- SubSR search failed!! ...aborting.\n"
	       "  mc minmc nummc SRnum SubSRnum: %d %d %d %d %d\n",
	       num, gd.srlut[srnum].mcnum[ssrnum],
	       gd.srlut[srnum].mcnum[ssrnum+1], srnum, ssrnum);
	exit(-1);
      }
      /* printf(" srnum.ssrnum: %d.%d\n",srnum,ssrnum); */

      /* take a conservative guess at the correct record number */
      if ((num-gd.srlut[srnum].mcnum[ssrnum]) <
	  (gd.srlut[srnum].mcnum[ssrnum+1]-num)) {
	recnum = 2 + 1024*gd.srlut[srnum].srloc + 64*ssrnum +
	  64*(num - gd.srlut[srnum].mcnum[ssrnum])/
	    (gd.srlut[srnum].mcnum[ssrnum+1]-gd.srlut[srnum].mcnum[ssrnum]+1);
      }
      else {
	recnum = 65 + 1024*gd.srlut[srnum].srloc + 64*ssrnum -
	  64*(gd.srlut[srnum].mcnum[ssrnum+1] - num)/
	    (gd.srlut[srnum].mcnum[ssrnum+1]-gd.srlut[srnum].mcnum[ssrnum]+1);
	if (ssrnum==15) recnum--;
      }

      while (1) {
	fseek(gd.CubeFile[gd.srlut[srnum].filenum],
	      ((long)recnum)*1024, SEEK_SET);
	if (!fread(&rec,1024,1,gd.CubeFile[gd.srlut[srnum].filenum])) {
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

	if (rec.h.minmc==-1) {
	  /* we are in the last SR of the file, past the last good data */
	  /* so go to the beginning of this subSR */
	  recnum = 2 + 1024*gd.srlut[srnum].srloc + 64*ssrnum;
	  continue;
	}

	if (num >= rec.h.minmc)
	  break;
	recnum--;
/* printf(" Ooops, seeking backwards, record number %d...\n",recnum); */
      }
    }

    /* if we're not quite there yet, read forward in the file */
    while (num > rec.h.minmc+rec.h.nummc-1) {
      recnum++;
      if (!fread(&rec,1024,1,gd.CubeFile[gd.srlut[srnum].filenum])) {
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
  if (mcptr > (unsigned char *)(&rec)+1024 ) {
    printf("\007  ERROR -- mcptr outside record!! ...aborting.\n"
	   "  mc SRnum minmc nummc recnum mcptr(-rec): %d %d %d %d %d %ld\n",
	   num, srnum, rec.h.minmc, rec.h.nummc, recnum,
	   (long int) (mcptr-(unsigned char *)(&rec)) );
    exit(-1);
  }

  oldnum = num;
  mcl=MCLEN(mcptr);

  if (mcptr + mcl > (unsigned char *)(&rec)+1024 ) {
    /* minicube spills over to next record */
    /* copy first part of minicube into cbuf */
    nbytes = (unsigned char *)(&rec) + 1024 - mcptr;
    memcpy (cbuf, mcptr, nbytes);

    /* now read in next record */
    /* note that we don't have to worry about the next record being in a new
       SuperRecord, since we do not allow minicubes to span SuperRecords */
    recnum++;
    if (!fread(&rec,1024,1,gd.CubeFile[gd.srlut[srnum].filenum])) {
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
    decompress (cbuf, mc);

    /* lastly fix up mcptr in case next call is for a mc in the same record */
    mcptr = (unsigned char *)(&rec) + rec.h.offset;
    oldnum = rec.h.minmc;
  }
  else
    decompress (mcptr,mc);

}

FILE * 
open_4cube (char *name)
{
  FILE *file, *srtfile;
  int length=0;
  int i,j,totnumsr,fnum,sr;
  unsigned int nummc=0;
  unsigned int nummcc, nummcc2, nummcc4;
  FHead head;
  SRHead shead;
  Record rec;
  char srtname[256], *c;
  int buildlist = 1;


  if (!name)
    return NULL;

  if (!(file=fopen(name,"r"))) {
    printf("\007  ERROR -- Cannot open file %s for reading.\n", name);
    return NULL;
  }

#define QUIT() \
        fclose(file);\
	for (i=0;i<gd.numCubeFiles;i++) fclose(gd.CubeFile[i]);\
        return NULL;

  totnumsr = 0;
  gd.numCubeFiles = 0;
  while (gd.numCubeFiles<RW_MAX_FILES) {
    fnum = gd.numCubeFiles;
    if (!fgets(gd.CubeFileName[fnum], 256, file) ||
	gd.CubeFileName[fnum][0] == 0)
      break;

    /* remove trailing \n and leading and trailing spaces */
    c = gd.CubeFileName[fnum];
    while (*c == ' ') memmove(c, c+1, strlen(c));
    for (c += strlen(c) - 1; *c == ' ' || *c == '\n'; c--) *c = 0;
    if (gd.CubeFileName[fnum][0] == 0) break;

    if (!(gd.CubeFile[fnum]=fopen(gd.CubeFileName[fnum],"r"))) {
      printf("\007  ERROR -- Cannot open file %s for reading.\n",
	     gd.CubeFileName[fnum]);
      QUIT();
    }
    gd.numCubeFiles++;
    if (!fread(&head,1024,1,gd.CubeFile[fnum])) {
      printf("\007  ERROR -- Cannot read file %s... aborting.\n",
	     gd.CubeFileName[fnum]);
      QUIT();
    }
    if (head.cps > 24) {
      gd.swapbytes = 1;
      swap4(&head.numch);
      swap4(&head.cps);
      swap4(&head.snum);
      swap4(&head.mclo);
      swap4(&head.mchi);
    }
    else
      gd.swapbytes = 0;

    if (strncmp(head.id,"4play hypercube ",16) ||  head.cps != 24) {
      printf("\007  ERROR -- Invalid header in file %s... aborting.\n",
	     gd.CubeFileName[fnum]);
      QUIT();
    }

    gd.mclo[fnum] = head.mclo;
    gd.mchi[fnum] = head.mchi;
    gd.numsr[fnum] = head.snum;
    totnumsr += head.snum;

    if (fnum==0) {
      length = head.numch;
      gd.length = length;
      nummc = (length +3)/4;  /* minicubes per side */
      /* nummc = nummc*(nummc+1)*(nummc+2)/6*(nummc+3)/4; */
      /* minicubes in cube */
      nummcc =  nummc*(nummc+1)*(nummc+2)/6;
      nummcc2 = nummcc/2;
      nummcc4 = nummcc/4;
      if (4*nummcc4 == nummcc) {
	nummc = (nummc+3)   * nummcc4;
      }
      else if (2*nummcc2 == nummcc) { 
	nummc = (nummc+3)/2 * nummcc2;
      }
      else {
	nummc = (nummc+3)/4 * nummcc;
      }
      gd.nummc = nummc;
      if (head.mclo!=0) {
	printf("\007  ERROR -- Files missing or in wrong order?..."
	       " aborting.\n");
	QUIT();
      }
    }
    else {
      if (length!=head.numch) {
	printf("\007  ERROR -- Different cube sizes... aborting.\n");
	QUIT();
      }
      if (head.mclo!=gd.mchi[fnum-1]+1) {
	printf("\007  ERROR -- Files missing or in wrong order?..."
	       " aborting.\n");
	QUIT();
      }
    }
  }

  if (gd.numCubeFiles<1) {
    printf("\007  ERROR -- No cube files opened...\n");
    QUIT();
  }
  if (head.mchi!=nummc-1) {
    printf("\007  ERROR -- Files missing?... aborting.\n"
	   "  ...This program was compiled with RW_MAX_FILES = %d.\n",
	   RW_MAX_FILES);
    QUIT();
  }

  gd.totnumsr = totnumsr;
  printf("Axis length of 4d cube is %d.\n"
	 "4d cube has %d minicubes, %d SuperRecords.\n",
	 length, nummc, totnumsr);

  /* read or build SuperRecord physical # and minicube # lookup table */
  if (!(gd.srlut = malloc(sizeof(SRlus)*totnumsr))) {
    printf("whoops\n");
    exit(1);
  }

  strcpy(srtname,name);
  setfnamext(srtname,".srt",1);
  if ((srtfile=fopen(srtname,"r"))) {
    buildlist = 0;
    printf("  ...Reading SuperRecord list from file %s...\n",srtname);
    if ((!fread(gd.srlut,sizeof(SRlus)*totnumsr,1,srtfile)) ||
	gd.srlut[0].mcnum[1]<0) {
      printf("\007  ERROR -- Could not read list, will rebuild it.\n");
      buildlist = 1;
    }
    fclose(srtfile);
    printf("  ...Done reading SuperRecord list.\n");
  }

  if (buildlist) {
    printf("  Building SuperRecord list...\n"
	   "  Expect it to take at least %d minutes...\n", totnumsr/250);

    sr = 0;
    for (fnum=0; fnum<gd.numCubeFiles; fnum++) {
      for (i=0; i<gd.numsr[fnum]; i++) {
	fseek(gd.CubeFile[fnum], (((long) i)*1024+1)*1024, SEEK_SET);
	if (!fread(&shead,1024,1,gd.CubeFile[fnum])) {
	  printf("\007  ERROR -- Could not read SuperRecord header %d,"
		 " file #%d... aborting.\n", i, fnum+1);
	  QUIT();
	}
	if (gd.swapbytes) {
	  swap4(&shead.snum);
	  swap4(&shead.minmc);
	  swap4(&shead.maxmc);
	}
	if (shead.snum > -1) {
	  gd.srlut[sr+shead.snum].filenum = fnum;
	  gd.srlut[sr+shead.snum].srloc = i;
	  gd.srlut[sr+shead.snum].mcnum[0] = shead.minmc;
	  gd.srlut[sr+shead.snum].mcnum[16] = shead.maxmc;

	  for (j=1; j<16; j++) {
	    fseek(gd.CubeFile[fnum], (long)64*1024 , SEEK_CUR);
	    if (!fread(&rec, 1024, 1, gd.CubeFile[fnum])) {
	      printf("\007  ERROR -- Could not read Record %d-%d, file #%d... "
		     "aborting.\n", i, j, fnum+1);
	      QUIT();
	    }
	    if (gd.swapbytes)
	      swap4(&rec.h.minmc);
	    if (rec.h.minmc>=0)
	      gd.srlut[sr+shead.snum].mcnum[j] = rec.h.minmc;
	    else
	      gd.srlut[sr+shead.snum].mcnum[j] = shead.maxmc;
	  }
	}
      }
      sr += gd.numsr[fnum];
    }
    printf("  ...Done building SuperRecord list.\n");

    strcpy(srtname,name);
    setfnamext(srtname,".srt",1);
    printf("  ...Saving SuperRecord list in file %s...\n",srtname);
    if (!(srtfile=fopen(srtname,"w+"))) {
      printf("\007  ERROR -- Cannot open file for writing.\n");
      exit(-1);
    }
    if (!fwrite(gd.srlut,sizeof(SRlus)*totnumsr,1,srtfile)) {
      printf("\007  ERROR -- Could not write list... aborting.\n");
      exit(-1);
    }
    fclose(srtfile);
    printf("  ...Done saving SuperRecord list.\n");
  }
#undef QUIT

  /* build our minicube lookup table */
  lumw[0] = lumw[1] = lumw[2] = lumw[3] = 0;
  lumx[0] = lumx[1] = lumx[2] = lumx[3] = 0;
  lumy[0] = lumy[1] = lumy[2] = lumy[3] = 0;
  lumz[0] = lumz[1] = lumz[2] = lumz[3] = 0;
  for (i=4;i<gd.length;i++) {
    lumw[i] = (i/4);
    lumx[i] = lumx[i-4] + lumw[i];
    lumy[i] = lumy[i-4] + lumx[i];
    lumz[i] = lumz[i-4] + lumy[i];
  }

  return file;
}

FILE * 
open_3cube (char *name)
{
  FILE *file;
  int i;
  char cbuf[256];
  FHead3D head;


  if (!name)
    return NULL;
  if ((file=fopen(name,"r+"))) {
    printf("\007  WARNING -- File %s exists. Overwrite it? (N/Y) ", name);
    fgets(cbuf,256,stdin);
    if (cbuf[0] != 'y' && cbuf[0] != 'Y') {
      fclose(file);
      return NULL;
    }
  }
  else if (!(file=fopen(name,"w+"))) {
    printf("\007  ERROR -- Cannot open file %s for writing.\n", name);
    return NULL;
  }

  strncpy(head.id, "Incub8r3/Pro4d  ", 16);
  head.numch = gd.length;
  head.bpc = 4;
  head.cps = 6;
  head.numrecs = 0;
  memset (head.resv, 0, 992);
  if (!fwrite(&head,1024,1,file)) {
    printf("\007  ERROR -- Could not write 3D cube header... aborting.\n");
    fclose(file);
    return NULL;
  }

  /* calculate look-up tables to convert (x,y,z) 3d cube address 
        to linearized 3d minicube address.  x<y<z
     lum{x,y,z}3d[ch] returns the number of subcubes to skip over
        to get to the subcube with ch in it.
     note that a 3d subcube is 8x8x4 channels.
     */
  for (i=0;i<8;i++) {
    lumx3d[i] = 0;
    lumy3d[i] = 0;
    lumz3d[i] = i/4;
  }
  for (i=8;i<gd.length+16;i++) {
    lumx3d[i] = (i/8)*2;
    lumy3d[i] = lumy3d[i-8] + lumx3d[i];
    lumz3d[i] = lumz3d[i-8] + lumy3d[i];
  }
 
  return file;
}

int
main (int argc, char **argv)
{
  int length;  /* number of channels per side of cube */
  char cbuf[1024];
  char c4nam[256], c3nam[256];
  unsigned short mc[256];
  unsigned int *cube3d;
  int w, x, y, z, iw, ix, iy, iz;
  int i, zlo, zhi, nummc3d, sizeMB, lo3daddr, minsize;
  int owxy, owxz, owyz, oxyz;
  int awxy[256], awxz[256], awyz[256], axyz[256];
  Record3D *rec3d;
  int mcnum3d = 0, mc3d, numrecs = 0;
  unsigned char *outptr;
  FHead3D head;


  printf("\n\n"
	 "  Pro4d  --  a 4D-to-3D projection program for\n"
	 "             RadWare quadruple coincidence hybercubes.\n\n"
	 "  Author     D.C Radford    Aug 1997\n\n"
	 "  Version    "VERSION"\n\n");

  while (1) {
    if (argc > 1) {
      strncpy(c4nam, argv[1], 70);
      argc = 0;
    } else {
      printf("Enter input 4D cube directory file name (%s): ",RW_4DFILE);
      if (!fgets(c4nam, 250, stdin)) continue;
      c4nam[strlen(c4nam)-1] = 0;  /* remove \n */
      if (c4nam[0] == 0) {
	strncpy(c4nam, RW_4DFILE, 256);
      }
    }

    /* open hypercube */
    setfnamext(c4nam,".4d",0);
    if ((gd.CubeDirFile = open_4cube(c4nam)))
      break;
  }

  while (1) {
    printf("Enter output 3D cube file name (%s): ",RW_3DFILE);
    if (!fgets(c3nam, 250, stdin)) continue;
    c3nam[strlen(c3nam)-1] = 0;  /* remove \n */
    if (c3nam[0] == 0)
      strncpy(c3nam, RW_3DFILE, 256);

    /* open 3D cube */
    setfnamext(c3nam,".cub",0);
    if ((gd.CubeFile3d = open_3cube(c3nam)))
      break;
  }
  length = gd.length;

  /* calculate the mappings from 4d channel space to 3d channel space */
  i = 0;
  for (z=0; z<4; z++) {
    for (y=0; y<4; y++) {
      for (x=0; x<4; x++) {
	for (w=0; w<4; w++) {
	  awxy[i] = ADDR3D(w,x,y);
	  awxz[i] = ADDR3D(w,x,z);
	  awyz[i] = ADDR3D(w,y,z);
	  axyz[i] = ADDR3D(x,y,z);
	  i++;
	}
      }
    }
  }

  /* malloc the space for the 3d cube */
  nummc3d = (length+7)/8;
  minsize = (nummc3d*(nummc3d+1)+1023)/1024;
  nummc3d = nummc3d*(nummc3d+1)*(nummc3d+2)/3;
  while (1) {
    printf("How many MB of RAM can I use? (suggest at least 32) : ");
    if (!fgets(cbuf, 512, stdin)) continue;
    if (sscanf(cbuf,"%d", &sizeMB) == 1) {
      if (sizeMB<minsize) {
	printf("Bah humbug, are you kidding? I need at least %d!\n\n",
	       minsize);
	continue;
      }
      if (sizeMB>256)
	printf("Wow! You must be rich.\n");
      if (sizeMB>(nummc3d+1023)/1024) {
	sizeMB = (nummc3d+1023)/1024;
	printf("Actually, I only need %dMB.\n",sizeMB);
      }
      if ((cube3d = malloc(sizeMB*1024*1024)))
	break;
      printf("\007 Nope, could not malloc %dMB!\n",sizeMB);
    }
  }

  /* malloc and prepare the 3d cube output record */
  rec3d = malloc(5*1024);
  outptr = rec3d->d;
  rec3d->h.minmc = 0;
  rec3d->h.offset=8;

  zlo = 0;
  while (zlo<length) {
    lo3daddr = ADDR3D(0,0,zlo);
    /* figure out how many z-values we can fit into the malloc'ed space */
    for (zhi=zlo+7; zhi<length-1; zhi+=8) {
      if ((ADDR3D(0,0,(zhi+9))-lo3daddr) > sizeMB*1024*256)
	break;
    }
    if (zhi>length-1)
      zhi = length - 1;

    memset(cube3d, 0, sizeMB*1024*1024);

    printf("Projecting for z = %d to %d...  %s\n", zlo, zhi, datim());
    for (iz=zlo; iz<=zhi; iz+=4) {
      for (iy=0; iy<=iz; iy+=4) {
	for (ix=0; ix<=iy; ix+=4) {
	  for (iw=0; iw<=ix; iw+=4) {
	    i = LUMC(iw,ix,iy,iz);
	    if ((i&4095)==0) {
	      printf("\rw x y z: %4d %4d %4d %4d ",iw,ix,iy,iz);
	      fflush(stdout);
	    }
	    getmc(i,mc);

	    owxy = ADDR3D(iw,ix,iy)-lo3daddr;
	    owxz = ADDR3D(iw,ix,iz)-lo3daddr;
	    owyz = ADDR3D(iw,iy,iz)-lo3daddr;
	    oxyz = ADDR3D(ix,iy,iz)-lo3daddr;
	    for (i=0; i<256; i++) {
	      if (mc[i]>0) {
		cube3d[awxz[i]+owxz] += mc[i];
		cube3d[awyz[i]+owyz] += mc[i];
		cube3d[axyz[i]+oxyz] += mc[i];
		if (iy>=zlo)
		  cube3d[awxy[i]+owxy] += mc[i];
	      }
	    }
	  }
	}
      }
    }
/*    printf("\n%s\n",datim()); */

    for (iz=zhi+1; iz<length; iz+=4) {
      for (iy=zlo; iy<=zhi; iy+=4) {
	for (ix=0; ix<=iy; ix+=4) {
	  for (iw=0; iw<=ix; iw+=4) {
	    i = LUMC(iw,ix,iy,iz);
	    if ((i&4095)==0) {
	      printf("\rw x y z: %4d %4d %4d %4d ",iw,ix,iy,iz);
	      fflush(stdout);
	    }
	    getmc(i,mc);

	    owxy = ADDR3D(iw,ix,iy)-lo3daddr;
	    for (i=0; i<256; i++)
	      if (mc[i]>0)
		cube3d[awxy[i]+owxy] += mc[i];
	  }
	}
      }
    }
    printf("\n");

    /* compress and write out 3d projection */
    mc3d = 0;
    for (z=zlo; z<=zhi; z+=8) {
      for (y=0; y<=z; y+=8) {
	for (x=0; x<=y; x+=8) {
	  for (i=0; i<2; i++) {
	    outptr += compress3d(&cube3d[mc3d],outptr);
	    mcnum3d++;
	    if (outptr > &(rec3d->d[4086])) {
	      /* record is full, write it out */
	      rec3d->h.nummc = mcnum3d - rec3d->h.minmc;
	      if (!(fwrite(rec3d, 4096, 1, gd.CubeFile3d))) {
		printf("\007  ERROR -- Cannot write 3d cube, aborting...\n");
		exit(-1);
	      }
	      numrecs++;
	      rec3d->h.minmc = mcnum3d;
	      rec3d->h.offset = 8 + outptr - &(rec3d->d[4088]);
	      if (rec3d->h.offset<8)
		rec3d->h.offset = 8;
	      outptr = (unsigned char *)rec3d + rec3d->h.offset;
	      if (rec3d->h.offset>8)
		memcpy(rec3d->d, &rec3d->d[4088], rec3d->h.offset-8);
	    }
	    mc3d += 256;
	  }
	}
      }
    }

    zlo = zhi + 1;
  }
  printf("\nDone... %s\n",datim());

  /* write out the last record */
  rec3d->h.nummc = mcnum3d - rec3d->h.minmc;
  if (!(fwrite(rec3d, 4096, 1, gd.CubeFile3d))) {
    printf("\007  ERROR -- Cannot write 3d cube, aborting...\n");
    exit(-1);
  }
  numrecs++;

  rewind(gd.CubeFile3d);
  if (!(fread(&head, 1024, 1, gd.CubeFile3d))) {
    printf("\007  ERROR -- Cannot reread 3d cube header, aborting...\n");
    exit(-1);
  }
  head.numrecs = numrecs;
  rewind(gd.CubeFile3d);
  if (!(fwrite(&head, 1024, 1, gd.CubeFile3d))) {
    printf("\007  ERROR -- Cannot rewrite 3d cube header, aborting...\n");
    exit(-1);
  }

  printf("\n"
	 "Congratulations! All seems to have finished okay.\n"
	 "  You should now run pro3d on the 3d cube\n"
	 "    ...and maybe foldout too.\n"
	 "  Come back again sometime.\n\n");
  return 0;
}


