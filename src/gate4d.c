/* hyper-cube gating C routines for 4dg8r, to be linked with 4dg8r.f */
/* D.C. Radford */

/* 16 June 1999: DCR: found and fixed bug in calculating gates for
   w=x, w=y and w=z that was adding counts just above gate upper limits */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "4play.h"
#include "rwutils.c"
#include "util.h"

int lumw[RW_MAXCH];   /* look-up table, maps cube channel to linear minicube */
int lumx[RW_MAXCH];
int lumy[RW_MAXCH];
int lumz[RW_MAXCH];
  /* look up minicube addr */
#define LUMC(w,x,y,z) lumw[w] + lumx[x] + lumy[y] + lumz[z]
  /* look up addr in minicube */
#define LUIMC(w,x,y,z) (w&3) + ((x&3)<<2) + ((y&3)<<4) + ((z&3)<<6)

typedef struct {
  short filenum;
  short srloc;
  int mcnum[17];
} SRlus;

/* Some global data */
struct GlobalData {
  FILE *CubeDirFile;
  FILE *CubeFile[RW_MAX_FILES];
  char CubeDirFileName[256];
  char CubeFileName[RW_MAX_FILES][256];
  int  numCubeFiles;
  int  mclo[RW_MAX_FILES], mchi[RW_MAX_FILES];
  int  length;               /* length of axis on cube */
  int  nummc;                /* number of minicubes */
  SRlus *srlut;              /* SR table */
  int  numsr[RW_MAX_FILES];  /* current number of SuperRecords */
  int  totnumsr;             /* current total number of SuperRecords */
  int  swapbytes;
} gd;

#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))

#include "compress4d.c"

int
luimc2(int w, int x, int y, int z)
{
  int ch[4], i, j, tmp;

  if (w>x || x>y || y>z) {
    ch[0]=w; ch[1]=x; ch[2]=y; ch[3]=z;
    for (i=3; i>0; i--) {
      if(ch[i] < ch[i-1]) {
	tmp=ch[i]; ch[i]=ch[i-1]; ch[i-1]=tmp;
	j=i;
	while (j<3 && ch[j] > ch[j+1]) {
	  tmp=ch[j]; ch[j]=ch[j+1]; ch[j+1]=tmp;
	  j++;
	}
      }
    }
    return LUIMC(ch[0],ch[1],ch[2],ch[3]);
  }
  else
    return LUIMC(w,x,y,z);
}

void swap4(int *in);
void swap2(unsigned short *in);

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
	warn1("\007  ERROR -- SR search failed!! ...aborting.\n"
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
	  (ssrnum<15 &&num >= gd.srlut[srnum].mcnum[ssrnum+1]) ||
	  (ssrnum==15 &&num > gd.srlut[srnum].mcnum[ssrnum+1])) {
	warn1("\007  ERROR -- SubSR search failed!! ...aborting.\n"
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
	  60*(num - gd.srlut[srnum].mcnum[ssrnum])/
	    (gd.srlut[srnum].mcnum[ssrnum+1]-gd.srlut[srnum].mcnum[ssrnum]+1);
      }
      else {
	recnum = 65 + 1024*gd.srlut[srnum].srloc + 64*ssrnum -
	  68*(gd.srlut[srnum].mcnum[ssrnum+1] - num)/
	    (gd.srlut[srnum].mcnum[ssrnum+1]-gd.srlut[srnum].mcnum[ssrnum]+1);
	if (ssrnum==15) recnum--;
      }

      while (1) {
	fseek(gd.CubeFile[gd.srlut[srnum].filenum],
	      ((long)recnum)*1024, SEEK_SET);
	if (!fread(&rec,1024,1,gd.CubeFile[gd.srlut[srnum].filenum])) {
	  warn1("\007  ERROR -- Could not read record %d... "
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
	/* printf(" Ooops, seeking backwards, record number %d...\n",
	   recnum); */
      }
    }

    /* if we're not quite there yet, read forward in the file */
    while (num > rec.h.minmc+rec.h.nummc-1) {
      recnum++;
      if (!fread(&rec,1024,1,gd.CubeFile[gd.srlut[srnum].filenum])) {
	warn1("\007  ERROR -- Could not read record %d... "
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
    warn1("\007  ERROR -- mcptr outside record!! ...aborting.\n"
	 "  mc SRnum minmc nummc recnum mcptr(-rec): %d %d %d %d %d %d\n",
	 num, srnum, rec.h.minmc, rec.h.nummc, recnum,
	 mcptr-(unsigned char *)(&rec) );
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
      warn1("\007  ERROR -- Could not read record %d... "
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

int close4dfile(void)
{
  int i;

  fclose(gd.CubeDirFile);
  for (i=0;i<gd.numCubeFiles;i++) fclose(gd.CubeFile[i]);
  return 0;
}

int open4dfile(char *name, int *numchs_ret)
{
  FILE *srtfile;
  int length=0;
  int i,j,totnumsr,fnum,sr;
  unsigned int nummc=0;
  unsigned int nummcc, nummcc2, nummcc4;
  FHead head;
  SRHead shead;
  Record rec;
  char cdname[256], srtname[256], filnam[256], *c;
  int buildlist = 1;


  strcpy(cdname, name);
  if (!cdname[0] || !(gd.CubeDirFile=fopen(cdname,"r"))) {
    warn1("\007  ERROR -- Cannot open file %s for reading.\n", cdname);
    return 1;
  }

#define QUIT close4dfile(); return 1;
  totnumsr = 0;
  gd.numCubeFiles = 0;
  while (gd.numCubeFiles<RW_MAX_FILES) {
    fnum = gd.numCubeFiles;
    if (!fgets(gd.CubeFileName[fnum], 256, gd.CubeDirFile) ||
	gd.CubeFileName[fnum][0] == 0)
      break;

    /* remove trailing \n and leading and trailing spaces */
    c = gd.CubeFileName[fnum];
    while (*c == ' ') memmove(c, c+1, strlen(c));
    for (c += strlen(c) - 1; *c == ' ' || *c == '\n'; c--) *c = 0;
    if (gd.CubeFileName[fnum][0] == 0)
      break;

    strcpy(filnam, gd.CubeFileName[fnum]);
    if (!inq_file(filnam, &j)) {
      /* file does not exist? - try pre-pending path to .4d file */
      strcpy(filnam, cdname);
      if ((c = rindex(filnam, '/'))) {
	*(c+1) = '\0';
      } else {
	filnam[0] = '\0';
      }
      strcat(filnam, gd.CubeFileName[fnum]);
    }
    if (!(gd.CubeFile[fnum]=fopen(filnam,"r"))) {
      warn1("\007  ERROR -- Cannot open file %s for reading.\n",
	   gd.CubeFileName[fnum]);
      QUIT;
    }
    gd.numCubeFiles++;

    if (!fread(&head,1024,1,gd.CubeFile[fnum])) {
      warn1("\007  ERROR -- Cannot read file %s... aborting.\n",
	   gd.CubeFileName[fnum]);
      QUIT;
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
      warn1("\007  ERROR -- Invalid header in file %s... aborting.\n",
	   gd.CubeFileName[fnum]);
      QUIT;
    }

    gd.mclo[fnum] = head.mclo;
    gd.mchi[fnum] = head.mchi;
    gd.numsr[fnum] = head.snum;
    totnumsr += head.snum;

    if (fnum==0) {
      length = head.numch;
      gd.length = length;
      if (length > RW_MAXCH) {
	warn1("ERROR: Number of channels in cube (%d)\n"
	     "         is greater than RW_MAXCH (%d) in 4play.h\n",
	     length, RW_MAXCH);
	exit(-1);
      }
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
	warn1("\007  ERROR -- Files missing or in wrong order?... aborting.\n");
	QUIT;
      }
    }
    else {
      if (length!=head.numch) {
	warn1("\007  ERROR -- Different cube sizes... aborting.\n");
	QUIT;
      }
      if (head.mclo!=gd.mchi[fnum-1]+1) {
	warn1("\007  ERROR -- Files missing or in wrong order?... aborting.\n");
	QUIT;
      }
    }
  }

  if (gd.numCubeFiles<1) {
    warn1("\007  ERROR -- No cube files opened...\n");
    QUIT;
  }
  if (head.mchi!=nummc-1) {
    warn1("\007  ERROR -- Files missing?... aborting.\n"
	 "  ...This program was compiled with RW_MAX_FILES = %d.\n",
	 RW_MAX_FILES);
    QUIT;
  }

  gd.totnumsr = totnumsr;
  tell("Axis length of 4d cube is %d.\n"
       "4d cube has %d minicubes, %d SuperRecords.\n",
       length, nummc, totnumsr);

  /* read or build SuperRecord physical # and minicube # lookup table */
  if (!(gd.srlut = malloc(sizeof(SRlus)*totnumsr))) {
    tell("whoops\n");
    exit(1);
  }

  strcpy(srtname,cdname);
  setfnamext(srtname,".srt",1);
  if ((srtfile=fopen(srtname,"r"))) {
    buildlist = 0;
    tell("  ...Reading SuperRecord list from file %s...\n",srtname);
    if ((!fread(gd.srlut,sizeof(SRlus)*totnumsr,1,srtfile)) ||
	gd.srlut[0].mcnum[1]<0) {
      tell("\007  ERROR -- Could not read list, will rebuild it.\n");
      buildlist = 1;
    }
    fclose(srtfile);
    tell("  ...Done reading SuperRecord list.\n");
  }

  if (buildlist) {
    tell("  Building SuperRecord list...\n"
	 "  Expect it to take at least %d minutes...\n", totnumsr/250);

    sr = 0;
    for (fnum=0; fnum<gd.numCubeFiles; fnum++) {
      for (i=0; i<gd.numsr[fnum]; i++) {
	fseek(gd.CubeFile[fnum], (((long) i)*1024+1)*1024, SEEK_SET);
	if (!fread(&shead,1024,1,gd.CubeFile[fnum])) {
	  warn1("\007  ERROR -- Could not read SuperRecord header %d,"
	       " file #%d... aborting.\n", i, fnum+1);
	  QUIT;
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
	      warn1("\007  ERROR -- Could not read Record %d-%d, file #%d... "
		   "aborting.\n", i, j, fnum+1);
	      QUIT;
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
    tell("  ...Done building SuperRecord list.\n");

    strcpy(srtname,cdname);
    setfnamext(srtname,".srt",1);
    tell("  ...Saving SuperRecord list in file %s...\n",srtname);
    if (!(srtfile=fopen(srtname,"w+"))) {
      warn1("\007  ERROR -- Cannot open file for writing.\n");
      exit(-1);
    }
    if (!fwrite(gd.srlut,sizeof(SRlus)*totnumsr,1,srtfile)) {
      warn1("\007  ERROR -- Could not write list... aborting.\n");
      exit(-1);
    }
    fclose(srtfile);
    tell("  ...Done saving SuperRecord list.\n");
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

  *numchs_ret = gd.length;
  return 0;
}

int read4dcube(int lo1, int hi1, int lo2, int hi2, int lo3, int hi3,
	       int *spec_ret)
{
  int lim[3][2];
  int i, j, tmp, w, x, y, z;
  int xmc, ymc, zmc, xlo, ylo, zlo, xhi, yhi, zhi;
  int w3, w4, x3, x4, y1, y2, y3, y4, z1, z2, z3, z4;
  unsigned short mc[256];
  int mcnum, time0, time1;


  memset(spec_ret, 0, 4*gd.length);

  lim[0][0] = lo1; lim[0][1] = hi1;
  lim[1][0] = lo2; lim[1][1] = hi2;
  lim[2][0] = lo3; lim[2][1] = hi3;
  for (i=0; i<3; i++) {
    if (lim[i][0]>lim[i][1]) {
      tmp = lim[i][0];
      lim[i][0] = lim[i][1];
      lim[i][1] = tmp;
    }
    if (lim[i][0]<0 || lim[i][1] >= gd.length) {
      warn1(" OVERFLOW bud.  The valid range is 0 to %d.\n",gd.length-1);
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
  if (lim[1][0]>lim[2][0]) {
    tmp = lim[2][0];
    lim[2][0] = lim[1][0];
    lim[1][0] = tmp;
    tmp = lim[2][1];
    lim[2][1] = lim[1][1];
    lim[1][1] = tmp;
    if (lim[0][0]>lim[1][0]) {
      tmp = lim[1][0];
      lim[1][0] = lim[0][0];
      lim[0][0] = tmp;
      tmp = lim[1][1];
      lim[1][1] = lim[0][1];
      lim[0][1] = tmp;
    }
  }

  /* now go read the cube to get the gate */
  tell("Accessing gate (%d-%d), (%d-%d), (%d-%d)...",
       lim[0][0],lim[0][1], lim[1][0],lim[1][1], lim[2][0],lim[2][1]);
  fflush(stdout);
  time0 = time(NULL);

  /* remember the data are organised as minicubes, 4chs on a side */ 
  for (zmc = lim[2][0]>>2; zmc<=lim[2][1]>>2; zmc++) {
    zlo = zmc*4;
    zhi = zlo+3;
    if (zlo<lim[2][0])
      zlo = lim[2][0];
    if (zhi>lim[2][1])
      zhi = lim[2][1];

    for (ymc = lim[1][0]>>2; ymc<=lim[1][1]>>2; ymc++) {
      ylo = ymc*4;
      yhi = ylo+3;
      if (ylo<lim[1][0])
	ylo = lim[1][0];
      if (yhi>lim[1][1])
	yhi = lim[1][1];

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

      for (xmc = lim[0][0]>>2; xmc<=lim[0][1]>>2; xmc++) {
	xlo = xmc*4;
	xhi = xlo+3;
	if (xlo<lim[0][0])
	  xlo = lim[0][0];
	if (xhi>lim[0][1])
	  xhi = lim[0][1];

	if (xlo>y1) {
	  x3 = y1;
	  x4 = y2;
	  if (xlo>z1) {
	    y3 = z1;
	    y4 = z2;
	    z3 = xlo;
	    z4 = xhi;
	  }
	  else {
	    y3 = xlo;
	    y4 = xhi;
	    z3 = z1;
	    z4 = z2;
	  }
	}
	else {
	  x3 = xlo;
	  x4 = xhi;
	  y3 = y1;
	  y4 = y2;
	  z3 = z1;
	  z4 = z2;
	}
	/* (x,y,z) gate is now ((x3,x4),(y3,y4),(z3,z4)) */

	/* read gate for w <= x */
	for (w3=0; w3<=x3-4; w3+=4) {
	  mcnum = LUMC(w3,x3,y3,z3);
	  getmc(mcnum, mc);
	  for (z=z3; z<=z4; z++) {
	    for (y=y3; y<=y4; y++) {
	      for (x=x3; x<=x4; x++) {
		i = luimc2(w3,x,y,z);
		for (j=0; j<4; j++)
		  spec_ret[w3+j] += mc[i+j];
	      }
	    }
	  }
	}

	w3 = (x3>>2)<<2;
	mcnum = LUMC(w3,x3,y3,z3);
	getmc(mcnum, mc);
	for (z=z3; z<=z4; z++) {
	  for (y=y3; y<=y4; y++) {
	    for (x=x3; x<=x4; x++) {
	      for (w=w3; w<=x; w++)
		spec_ret[w] += mc[luimc2(w,x,y,z)];
	    }
	  }
	}

	/* check for cases where x=y */
	if (x4>=y3) {
	  w4 = (x4<y4 ? x4 : y4);
	  for (z=z3; z<=z4; z++) {
	    for (y=y3; y<=w4; y++) {
	      spec_ret[y] += 3*mc[luimc2(y,y,y,z)];
	      if (y==z)
		spec_ret[y] += 14*mc[LUIMC(y,y,y,y)];
	    }
	  }
	}

	/* ok, finished reading gate for w <= x */
	/* read gate for x <= w <= y */
	/* first use the current minicube channels */
	w4 = w3+3;
	if (w4>=y3) { /* y-values are in this same mc range as x and w */
	  for (z=z3; z<=z4; z++) {
	    for (y=y3; y<=y4; y++) {
	      for (w=x3; w<=y; w++) {
		for (x=x3; (x<=w)&&(x<=x4); x++)
		  spec_ret[w] += mc[luimc2(x,y,w,z)];
	      }
	    }
	  }
	}

	else { /* y-values are not in same mc range as x and w */
	  for (z=z3; z<=z4; z++) {
	    for (y=y3; y<=y4; y++) {
	      i = luimc2(x3,x3,y,z);
	      for (w=x3; w<=w4; w++) {
		for (j=0; (j<=w-x3)&&(j<=x4-x3); j++)
		  spec_ret[w] += mc[i+j];
		i += 4;
	      }
	    }
	  }

	  /* now go ahead and read more minicubes, still for x <= w <= y */
	  for (w3=((x3>>2)+1)<<2; w3<=y3-4; w3+=4) {
	    mcnum = LUMC(x3,w3,y3,z3);
	    getmc(mcnum, mc);
	    w4 = w3 + 3;
	    for (z=z3; z<=z4; z++) {
	      for (y=y3; y<=y4; y++) {
		i = luimc2(x3,w3,y,z); /* no need to worry x>=w or w>=y */
		for (w=w3; w<=w4; w++) {
		  for (j=0; j<=x4-x3; j++)
		    spec_ret[w] += mc[i+j];
		  i += 4;
		}
	      }
	    }
	  }
	  w3 = (y3>>2)<<2;
	  mcnum = LUMC(x3,w3,y3,z3);
	  getmc(mcnum, mc);
	  for (z=z3; z<=z4; z++) {
	    for (y=y3; y<=y4; y++) {
	      for (w=w3; w<=y; w++) {
		for (x=x3; x<=x4; x++)
		  spec_ret[w] += mc[luimc2(x,w,y,z)];
	      }
	    }
	  }
	}

	/* check for cases where y=z */
	w3 = (y3>>2)<<2;
	if (y4>=z3) {
	  w4 = (y4<z4 ? y4 : z4);
	  for (z=z3; z<=w4; z++) {
	    for (x=x3; x<=x4; x++)
	      spec_ret[z] += 3*mc[luimc2(x,z,z,z)];
	  }
	}

	/* ok, finished reading gate for x <= w <= y */
	/* read gate for y <= w <= z */
	/* first use the current minicube channels */
	w4 = w3+3;
	if (w4>=z3) { /* z-values are in this same mc range as y and w */
	  for (z=z3; z<=z4; z++) {
	    for (w=y3; w<=z; w++) {
	      for (y=y3; (y<=w)&&(y<=y4); y++) {
		for (x=x3; x<=x4; x++)
		  spec_ret[w] += mc[luimc2(x,y,w,z)];
	      }
	    }
	  }
	}

	else { /* z-values are not in same mc range as y and w */
	  for (w=y3; w<=w4; w++) {
	    for (y=y3; (y<=w)&&(y<=y4); y++) {
	      for (x=x3; x<=x4; x++) {
		i = luimc2(x,y,w,z3);
		for (j=0; j<=z4-z3; j++)
		  spec_ret[w] += mc[i+(j<<6)];
	      }
	    }
	  }

	  /* now go ahead and read more minicubes, still for y <= w <= z */
	  for (w3=((y3>>2)+1)<<2; w3<=z3-4; w3+=4) {
	    mcnum = LUMC(x3,y3,w3,z3);
	    getmc(mcnum, mc);
	    for (y=y3; y<=y4; y++) {
	      for (x=x3; x<=x4; x++) {
		i = luimc2(x,y,w3,z3); /* no need to worry y>=w or w>=z */
		for (z=z3; z<=z4; z++) {
		  for (j=0; j<4; j++)
		    spec_ret[w3+j] += mc[i+(j<<4)];
		  i += 64;
		}
	      }
	    }
	  }
	  w3 = (z3>>2)<<2;
	  mcnum = LUMC(x3,y3,w3,z3);
	  getmc(mcnum, mc);
	  for (z=z3; z<=z4; z++) {
	    for (w=w3; w<=z; w++) {
	      for (y=y3; y<=y4; y++) {
		for (x=x3; x<=x4; x++)
		  spec_ret[w] += mc[luimc2(x,y,w,z)];
	      }
	    }
	  }
	}

	/* read gate for z <= w */
	/* first use the current minicube channels */
	w3 = (z3>>2)<<2;
	w4 = w3+3;
	for (w=z3; w<=w4; w++) {
	  for (z=z3; (z<=w)&&(z<=z4); z++) {
	    for (y=y3; y<=y4; y++) {
	      for (x=x3; x<=x4; x++)
		spec_ret[w] += mc[luimc2(x,y,z,w)];
	    }
	  }
	}

	/* now go ahead and read more minicubes, still for z < w */
	for (w3=((z3>>2)+1)<<2; w3<=gd.length-1; w3+=4) {
	  mcnum = LUMC(x3,y3,z3,w3);
	  getmc(mcnum, mc);
	  w4 = w3 + 3;
	  if (w4>gd.length-1)
	    w4 = gd.length-1;

	  for (z=z3; z<=z4; z++) {
	    for (y=y3; y<=y4; y++) {
	      for (x=x3; x<=x4; x++) {
		i = luimc2(x,y,z,w3);
		for (j=0; j<=w4-w3; j++)
		  spec_ret[w3+j] += mc[i+(j<<6)];
	      }
	    }
	  }
	}

      }
    }
  }

  time1 = time(NULL);
  tmp = time1-time0;
  tell(" took %d sec.\n",tmp);

  return 0;
}

int sum4gate(int *lo, int *hi)
{
  int lim[4][2], order[4], sum_ret = 0;
  int i, j, mcnum, w, x, y, z;
  int wmc, xmc, ymc, zmc, wlo, xlo, ylo, zlo, whi, xhi, yhi, zhi;
  unsigned short mc[256];
  int spec[RW_MAXCH];
  /* int tmp, time0, time1; */

  /* order the lower limits of the gates */
  order[0] = 0;
  for (i=1; i<4; i++) {
    j = i;
    while (j>0 && lo[i] < lo[order[j-1]]) {
      order[j] = order[j-1];
      j--;
    }
    order[j] = i;
  }
  for (i=0; i<4; i++) {
    lim[i][0] = lo[order[i]];
    lim[i][1] = hi[order[i]];
  }

  if (lim[0][1] >= lim[1][0] ||
      lim[1][1] >= lim[2][0] ||
      lim[2][1] >= lim[3][0]) {
    /* gates overlap; treat as special case... */
    read4dcube(lim[1][0], lim[1][1], lim[2][0], lim[2][1], 
	       lim[3][0], lim[3][1], spec);
    for (w=lim[0][0]; w<=lim[0][1]; w++)
      sum_ret += spec[w];
    return sum_ret;
  }

  /* gates don't overlap; read the cube to get the gate */
  /* tell("Summing gate (%d-%d), (%d-%d), (%d-%d, (%d-%d)...",
     lim[0][0],lim[0][1], lim[1][0],lim[1][1],
     lim[2][0],lim[2][1], lim[3][0],lim[3][1]);
     fflush(stdout);
     time0 = time(NULL);
  */

  /* remember the data are organised as minicubes, 4chs on a side */ 
  for (zmc = lim[3][0]>>2; zmc<=lim[3][1]>>2; zmc++) {
    zlo = zmc*4;
    zhi = zlo+3;
    if (zlo<lim[3][0])
      zlo = lim[3][0];
    if (zhi>lim[3][1])
      zhi = lim[3][1];

    for (ymc = lim[2][0]>>2; ymc<=lim[2][1]>>2; ymc++) {
      ylo = ymc*4;
      yhi = ylo+3;
      if (ylo<lim[2][0])
	ylo = lim[2][0];
      if (yhi>lim[2][1])
	yhi = lim[2][1];

      for (xmc = lim[1][0]>>2; xmc<=lim[1][1]>>2; xmc++) {
	xlo = xmc*4;
	xhi = xlo+3;
	if (xlo<lim[1][0])
	  xlo = lim[1][0];
	if (xhi>lim[1][1])
	  xhi = lim[1][1];

	for (wmc = lim[0][0]>>2; wmc<=lim[0][1]>>2; wmc++) {
	  wlo = wmc*4;
	  whi = wlo+3;
	  if (wlo<lim[0][0])
	    wlo = lim[0][0];
	  if (whi>lim[0][1])
	    whi = lim[0][1];

	  mcnum = LUMC(wlo,xlo,ylo,zlo);
	  getmc(mcnum, mc);
	  for (z=zlo; z<=zhi; z++) {
	    for (y=ylo; y<=yhi; y++) {
	      for (x=xlo; x<=xhi; x++) {
		i = luimc2(wlo,x,y,z);
		for (j=0; j<=whi-wlo; j++)
		  sum_ret += mc[i+j];
	      }
	    }
	  }
	}
      }
    }
  }

  /* time1 = time(NULL);
     tmp = time1-time0;
     tell(" took %d sec.\n",tmp);
  */
  return sum_ret;
}
