/***************************************************************************
   4PLAY: Replay program for quadruple coincidence hybercubes.
   Allows use of non-linear gains through look-up tables
     to convert gain-matched ADC outputs to cube channels.
   Uses disk buffering to .scr "scratch" file.
   This version for 1/24 hybercubes.
   D.C Radford, C.J. Beyer    Aug 1997
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifdef VMS
#include <unixio.h>
#else
/* #include <sys/mtio.h> */
#endif
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include "4play.h"
#include "rwutils.c"

#define VERSION "19 August 1997   Beta"

/* Some global data */
struct GlobalData {
  FILE *CubeDirFile;
  FILE *CubeFile[RW_MAX_FILES];
  FILE *ScrFile;
  FILE *LogFile;
  char  CubeFileName[RW_MAX_FILES][256];
  int   numCubeFiles;
  int   mclo[RW_MAX_FILES], mchi[RW_MAX_FILES];
  int   ScrSize, RecordSize;
  char  TapeDev[256];
  int   SwapTapeBytes;
  int   length;            /* length of axis on cube */
  int   nummc;             /* number of minicubes */
  int   numscr;            /* number of records written to scrfile */
  int   adcmax;
  short srvlut[RW_MAX_FILES][RW_MAX_FILE_SIZE]; /* SR virtual # lookup table */
  int   numsr[RW_MAX_FILES];   /* current number of SuperRecords */
  int   gotsignal;             /* a signal was caught by signal handler */
} gd;

int dbinfo[10];

/* lookup table stuff */
char *tbuf;           /* tape buffer */
short luch[16384];    /* look-up table, maps ADC channels to cube channels */
int lumw[RW_MAXCH];   /* look-up table, maps cube channel to linear minicube */
int lumx[RW_MAXCH];
int lumy[RW_MAXCH];
int lumz[RW_MAXCH];
  /* look up minicube addr */
#define LUMC(w,x,y,z) lumw[w] + lumx[x] + lumy[y] + lumz[z]
  /* look up addr in minicube */
#define LUIMC(w,x,y,z) (w&3) + ((x&3)<<2) + ((y&3)<<4) + ((z&3)<<6)

/* increment buffers
   buf1 holds short(16bit) thus must have RW_LB1 (20bit) rows to keep ~36 bits
   buf2 holds RW_LB2 (12bit) rows of 24bit(16+8) increments
   */

unsigned short buf1[RW_LB1][RW_DB1];
unsigned short buf2[RW_LB2][RW_DB2][RW_DB1+1];

unsigned char nbuf1[RW_LB1];       /* number in each row of buf1 */
unsigned short nbuf2[RW_LB2];      /* number in each row of buf2 */
unsigned int scrptr[RW_LB2];       /* last record # written to scr file */
                                   /* for each row of buf2              */


#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))


FILE *open_4cube (char *name, int length, FILE *mesg);
FILE *open_scr (char *name, int length, FILE *mesg);

/***************************************************************************
   setup_replay: reads in data file that describes cube, opens and
                 initializes cube, scr and log
*/
void
setup_replay(void)
{
  FILE *file;
  char cbuf[256], dnam[256], title[256], tnam[256], snam[256], cnam[256];
  char lnam[256], recs[256], scrs[256], stb[256];
  char *c;
  int  nclook, lmin, lmax, i, ovrtprompt = 1;

  printf(
".    .   :           *     .       . .            .     .      .      \n"
"   .          .  .               .          *    :         .    *    .\n"
"      @<--@<--@<--@<--@<--@<--@____@-->@-->@-->@-->@-->@-->@    .     \n"
". .   @  ..      .   :         *     .    .        .  . .  @ :        \n"
"    . @ . 44. 44   PPPP     LL         A     .  YY    YY . @      *   \n"
" .    @   44  44   PP  P  . LL   .    AAA        YY  YY.   @  .       \n"
"      @   4444444  PPPP     LL    .  A   A   .*   YYYY    .@     ..   \n"
"   :  @   .   44   PP   .   LL      AAAAAAA        YY      @.         \n"
"*     @ .     44   PP       LLLLL  AA     AA    .  YY   :  @   *      \n"
"     .@    .    .   ..     *          :      .       .   * @  .   :   \n"
"  .   @<--@<--@<--@<--@<--@<--@____@-->@-->@-->@-->@-->@-->@ .        \n"
"..         .  *          .    :     *       .       .  .         .    \n"
"   .          .     Version "VERSION"       .    \n\n");

  if ((c=getenv("RADWARE_OVERWRITE_FILE")) &&
      (*c == 'y' || *c == 'Y')) ovrtprompt = 0;

  while (1) { /* while data(inc) file is valid */

    /* First read in the data file */
    printf("Enter replay data file name (%s): ",RW_DATFILE);
    fgets(dnam, 250, stdin);
    dnam[strlen(dnam)-1] = 0;  /* remove \n */

    if (dnam[0] == 0) {
      strncpy(dnam, RW_DATFILE, 256);
    }

    setfnamext(dnam,".inc",0);
    if (!(file = fopen(dnam,"r"))) {
      printf("\007  ERROR: cannot open file %s\n",dnam);
      continue;
    }

#define GETDATA(a,b) \
    fgets(a, 256, file);\
    if (a[0] == 0) {\
      printf("\007  ERROR -- reading %s.\n",b);\
      fclose(file);\
      continue;\
    }\
    a[strlen(a)-1] = 0; /* remove trailing \n */ \
    while (a[0] == ' ') memmove(a, a+1, strlen(a)); /* remove leading spaces */ \
    for (c=a+strlen(a)-1; *c==' '; c--) *c=0; /* remove trailing spaces */

    GETDATA(title,"title");
    GETDATA(tnam, "tape drive");
    GETDATA(snam, "scratch file name");
    GETDATA(cnam, "hypercube dirfile name");
    GETDATA(lnam, "look-up table file name");
    GETDATA(recs, "record blocksize");
    GETDATA(scrs, "scratch file size");
    GETDATA(stb,  "swap bytes from tape");
#undef GETDATA

    fclose(file);
    if (stb[0] == 'y' || stb[0] == 'Y') {
      strcpy(stb,"y");
      gd.SwapTapeBytes = 1;
    }
    else {
      strcpy(stb,"n");
      gd.SwapTapeBytes = 0;
    }

    printf("\n           Replay data file name = %s\n"
           "                      Tape drive = %s\n"
           "               Scratch file name = %s\n"
           "               Cube dirfile name = %s\n"
           "         Look-up table file name = %s\n"
           "        Record BlockSize (Bytes) = %s\n"
           "      Scratch File Size (MBytes) = %s\n"
           "      Swap Bytes from Tape (y/n) = %s\n\n",
	   dnam,tnam,snam,cnam,lnam,recs,scrs,stb);

    printf("Are these values correct? (N/Y) ");
    fgets(cbuf,256,stdin);
    if (cbuf[0] != 'y' && cbuf[0] != 'Y')
      continue;

    strcpy(gd.TapeDev, tnam);

    /* test values */
    gd.RecordSize = strtol(recs, NULL, 0);
    if (gd.RecordSize < 512) {
      printf("\007  ERROR -- record blocksize too small.\n");
      continue;
    }
    gd.ScrSize = strtol(scrs, NULL, 0);
    if (gd.ScrSize < 1) {
      printf("\007  ERROR -- scratch file size too small.\n");
      continue;
    }

    strcpy(tnam,cnam);
    setfnamext(tnam,".log",1);
    while (1) {
      if ((gd.LogFile=fopen(tnam,"r"))) {
	fclose(gd.LogFile);
        if (ovrtprompt) {
	  printf("  WARNING: Log file (%s) already exists\n",tnam);
	  printf("  Enter new name or <Return> to overwrite: ");
	  if (!fgets(cbuf,256,stdin)) continue;
	  if (cbuf[0] != '\n') {
	    cbuf[strlen(cbuf)-1]=0;
	    strcpy(tnam,cbuf);
	    setfnamext(tnam,".log",0);
	    continue;
	  }
	}
	else
	  printf("  Overwriting Log file (%s).\n",tnam);
      }
      if (!(gd.LogFile=fopen(tnam,"w"))) {
	printf("\007  ERROR -- Could not open %s for writing.\n",tnam);
    	printf("  Enter new log file name: ");
	if (!fgets(cbuf,256,stdin)) continue;
	if (cbuf[0] != '\n') {
	  strncpy(tnam,cbuf,strlen(cbuf)-1);
	  setfnamext(tnam,".log",0);
	  continue;
	}
      }
      break;
    }

    /* read in lookup table */
    if (read_tab(lnam, &nclook, &lmin, &lmax, luch)) {
      fclose(gd.LogFile);
      continue;
    }
    gd.adcmax = nclook;
    printf("\nNo. of values in lookup table = %d\n",lmax);
    if (lmax > RW_MAXCH) {
      printf("\007  ERROR -- number of values in lookup file is too"
	     "large (max = %d).\n",RW_MAXCH);
      fclose(gd.LogFile);
      continue;
    }
    gd.length = lmax;
    /* check contents of lookup table */
    for (i=0; i<nclook; i++) {
      if (luch[i] > lmax) {
	printf("\007  ERROR -- lookup table is corrupted?\n");
	fclose(gd.LogFile);
	continue;
      }
    }

    /* Inform the log file */
    fprintf(gd.LogFile,"Replay log for program 4PLAY,  %s\n\n",datim());
    fprintf(gd.LogFile,"\n           Replay data file name = %s\n"
           "                      Tape drive = %s\n"
           "               Scratch file name = %s\n"
           "               Cube dirfile name = %s\n"
           "         Look-up table file name = %s\n"
           "        Record BlockSize (Bytes) = %s\n"
           "      Scratch File Size (MBytes) = %s\n"
           "      Swap Bytes from Tape (y/n) = %s\n\n",
	   dnam,gd.TapeDev,snam,cnam,lnam,recs,scrs,stb);
     fprintf(gd.LogFile," No. of values in lookup table = %d\n\n",lmax);
    fprintf(gd.LogFile,"-------------------------------------------\n\n");

    /* calculate look-up tables to convert (w,x,y,z) cube address 
          to linearized minicube address.  w<x<y<z
       lum{w,x,y,z}[ch] returns the number of subcubes to skip over
          to get to the subcube with ch in it.
       note that there are 4 channels on a subcube side (thus i = ch/4)
       thus, number of subcubes to skip:
        lumw[ch] = sum[j=0..i](1) = i
        lumx[ch] = sum[j=0..i](sum[k=0..j](1)) = i(i+1)/2
        lumy[ch] = sum[j=0..i](sum[k=0..j](sum[l=0..k](1))) = i(i+1)(i+2)/6
	lumz[ch] = ... = i(i+1)(i+2)(i+3)/24
          where i = ch/4,
       or, alternatively:
     */
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

    /* open hypercube and scratch */
    if (!(gd.CubeDirFile=open_4cube(cnam, gd.length, gd.LogFile))) {
      fclose(gd.LogFile);
      continue;
    }
    if (!(gd.ScrFile = open_scr(snam, gd.ScrSize, gd.LogFile))) {
      fclose(gd.CubeDirFile);
      for (i=0; i<gd.numCubeFiles; i++)
	fclose(gd.CubeFile[i]);
      fclose(gd.LogFile);
      continue;
    }

    /* malloc tape buffer(s) */
    if (!(tbuf = malloc(gd.RecordSize))) {
      fprintf(stderr,"\007  ERROR: Could not malloc tape buffer (%d bytes).\n",
	      gd.RecordSize);
      fprintf(stdout,"terminating...\n");
      exit(-1);
    }

    /* clear buf1 and buf2 */
    for(i=0;i<RW_LB1;i++)
      nbuf1[i] = 0;

    for(i=0;i<RW_LB2;i++) {
      nbuf2[i] = 0;
      scrptr[i] = 0;
    }
    fflush(gd.LogFile);
    break;
  }
}

/***************************************************************************
   open_4cube: returns file descriptor on success or NULL on failure
     name - file name on disk
     length - number of channels on an axis
     mesg - stream to print error messages (in addition to stdout)
*/
FILE * 
open_4cube (char *name, int length, FILE *mesg)
{
  FILE   *file;
  int    i, totnumsr, fnum;
  unsigned int  nummc;
  FHead  head;
  SRHead shead;
  char   *c;
  int    nmcperc=RW_CHNK_S*256;
  unsigned int nummcc, nummcc2, nummcc4;


  if (!name)
    return NULL;

  if (!(file=fopen(name,"r"))) {
    printf("\007  ERROR -- Cannot open file %s for reading.\n"
	   "  Maybe you need to create the 4D cube files with make4cub.\n\n",
	   name);
    return NULL;
  }

  nummc = (length +3)/4;  /* minicubes per side */
  /* nummc = nummc*(nummc+1)*(nummc+2)/6*(nummc+3)/4; */
  /* minicubes in cube */
  nummcc =  nummc*(nummc+1)*(nummc+2)/6;
  nummcc2 = nummcc/2;
  nummcc4 = nummcc/4;
  if (4*nummcc4 == nummcc) { 
    nummc = (nummc+3) * nummcc4;
  }
  else if (2*nummcc2 == nummcc) {
    nummc = (nummc+3)/2 * nummcc2;
  }
  else {
    nummc = (nummc+3)/4 * nummcc;
  }
  gd.nummc = nummc;

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

    if (!(gd.CubeFile[fnum]=fopen(gd.CubeFileName[fnum],"r+"))) {
      fprintf(mesg,"  ERROR -- Cannot open file %s for reading.\n",
	      gd.CubeFileName[fnum]);
      printf("\007  ERROR -- Cannot open file %s for reading.\n",
	     gd.CubeFileName[fnum]);
      QUIT();
    }
    gd.numCubeFiles++;
    if (!fread(&head,1024,1,gd.CubeFile[fnum])) {
      fprintf(mesg,"  ERROR -- Cannot read file %s... aborting.\n",
	      gd.CubeFileName[fnum]);
      printf("\007  ERROR -- Cannot read file %s... aborting.\n",
	     gd.CubeFileName[fnum]);
      QUIT();
    }
    if (strncmp(head.id,"4play hypercube ",16) ||  head.cps != 24) {
      fprintf(mesg,"  ERROR -- Invalid header in file %s... aborting.\n",
	      gd.CubeFileName[fnum]);
      printf("\007  ERROR -- Invalid header in file %s... aborting.\n",
	     gd.CubeFileName[fnum]);
      QUIT();
    }
    if (length!=head.numch) {
      fprintf(mesg,"  ERROR -- Different cube sizes... aborting.\n");
      printf("\007  ERROR -- Different cube sizes... aborting.\n");
      QUIT();
    }

    gd.mclo[fnum] = head.mclo;
    gd.mchi[fnum] = head.mchi;
    gd.numsr[fnum] = head.snum;
    totnumsr += head.snum;
    if (head.snum>=RW_MAX_FILE_SIZE) {
      fprintf(mesg,"  ERROR -- File %s too big, max. = %dMB... aborting.\n",
	      gd.CubeFileName[fnum], RW_MAX_FILE_SIZE);
      printf("\007  ERROR -- File %s too big, max. = %dMB... aborting.\n",
	     gd.CubeFileName[fnum], RW_MAX_FILE_SIZE);
      QUIT();
    }
    if (head.snum>=RW_MAX_FILE_SIZE*3/4) {
      printf("\007  WARNING -- File %s is very big, max. = %dMB.\n",
	     gd.CubeFileName[fnum], RW_MAX_FILE_SIZE);
    }

    if ((fnum==0 && head.mclo!=0) ||
	(fnum>0  && head.mclo!=gd.mchi[fnum-1]+1)) {
      fprintf(mesg,"  ERROR -- Files missing or in wrong order?..."
	      " aborting.\n");
      printf("\007  ERROR -- Files missing or in wrong order?..."
	     " aborting.\n");
      QUIT();
    }
    if (head.mclo!=(head.mclo/nmcperc)*nmcperc) {
      fprintf(mesg,"  ERROR -- File size is not integer multiple"
	      "of chunk size... aborting.\n");
      printf("\007  ERROR -- File size is not integer multiple"
	     "of chunk size... aborting.\n");
      QUIT();
    }
  }

  if (gd.numCubeFiles<1) {
    fprintf(mesg,"  ERROR -- No cube files opened...\n");
    printf("\007  ERROR -- No cube files opened...\n");
    QUIT();
  }
  if (head.mchi!=nummc-1) {
    fprintf(mesg,"  ERROR -- Files missing?... aborting.\n"
	    "  ...This program was compiled with RW_MAX_FILES = %d.\n",
	    RW_MAX_FILES);
    printf("\007  ERROR -- Files missing?... aborting.\n"
	   "  ...This program was compiled with RW_MAX_FILES = %d.\n",
	   RW_MAX_FILES);
    QUIT();
  }

  fprintf(mesg,
	  "Axis length of 4d cube is %d.\n"
	  "4d cube is in %d files.\n"
	  "It has %d minicubes and %d SuperRecords.\n",
	  length, gd.numCubeFiles, nummc, totnumsr);

  printf("Axis length of 4d cube is %d.\n"
	 "4d cube is in %d files.\n"
	 "It has %d minicubes and %d SuperRecords.\n",
	 length, gd.numCubeFiles, nummc, totnumsr);

  /* build SuperRecord virtual number lookup table */
  printf("  Building SuperRecord list...\n");
  memset(gd.srvlut, -1, RW_MAX_FILES*RW_MAX_FILE_SIZE*2);

  for (fnum=0; fnum<gd.numCubeFiles; fnum++) {
    for (i=0; i<gd.numsr[fnum]; i++) {
      fseek(gd.CubeFile[fnum], ((((long)i)*1024)+1)*1024, SEEK_SET);
      if (!fread(&shead,1024,1,gd.CubeFile[fnum])) {
	fprintf(mesg,"  ERROR -- Could not read SuperRecord header %d!!\n", i);
	printf("\007  ERROR -- Could not read SuperRecord header %d!!\n", i);
	QUIT();
      }
      gd.srvlut[fnum][i] = shead.snum;
    }
  }
  printf("  ...Done building SuperRecord list.\n");

#undef QUIT
  return file;
}

/***************************************************************************
   open_scr: returns file descriptor on success or NULL on failure
     name - file name on disk
     length - size on Megabytes of scr file
     mesg - stream to print error messages (in addition to stdout)
*/
FILE *
open_scr (char *name, int length, FILE *mesg)
{
  FILE *file;
  int b[256], i;

  if(!name || length < 1 || !(file=fopen(name,"w+")))
    return NULL;
  rewind(file);

  fprintf(mesg,"  Making sure we have enough scratch space...\n");
  printf("  Making sure we have enough scratch space...\n");
  for (i=0;i<length*256;i++) /* writing to make sure we have space */
    if (!fwrite(b, 4096, 1, file)) {
      fprintf(stderr,"\007  ERROR -- Could not allocate scratch space on disk"
	      "(%dM).\n",length);
      fclose(file);
      return NULL;
    }
  
  gd.numscr = 0;
  rewind(file);
  fprintf(mesg,"  ...Done.\n");
  printf("  ...Done.\n");

  return file;
}

int
read_tape(int fd)
{
  int ret;
  char tmp;
  int i;
  /* unsigned short data[40]; */

  if ((ret = read(fd, tbuf, gd.RecordSize)) < 0) return -1;
  if (gd.SwapTapeBytes) {
    for (i=0; i<ret; i+=2) {
      tmp = *(tbuf+i);
      *(tbuf+i) = *(tbuf+i+1);
      *(tbuf+i+1) = tmp;
    }
  }
  /* printf("%d bytes read from tape...\n", ret);
   * memcpy(data, tbuf, 80);
   * for (i=0; i<4; i++) {
   *    printf("  %9x %9x %9x %9x %9x\n",
   *        data[5*i+0], data[5*i+1], data[5*i+2], data[5*i+3], data[5*i+4]);
   * }
   * for (i=0; i<4; i++) {
   *    printf("%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d\n",
   *      data[10*i+0], data[10*i+1], data[10*i+2], data[10*i+3], data[10*i+4],
   *      data[10*i+5], data[10*i+6], data[10*i+7], data[10*i+8], data[10*i+9]);
   * }
   */
  return ret;
}

#include "compress4d.c"
void
dscr(void)
{
  int            numchunks, fnum, chunknum, totnumsr;
  char           *srecIn;
  char           *srecOut;
  SRHead         *srheadIn;
  SRHead         *srheadOut;
  Record         *recIn, *recOut;
  int            j, k, mc, mcl;
  unsigned char  *mcptrIn, *mcptrOut, cbuf[1024];
  int            minmc, maxmc;
  int            srecnumIn, srecnumOut, oldsrnum;
  int            recnumIn, recnumOut, nbytes;
  int            addr20b, addr12b, addr24b;
  int            nmcperc=RW_CHNK_S*256;
  unsigned short inbuf[((RW_SCR_RECL/2)/(RW_DB1+1))+1][RW_DB1+1];
  FHead          filehead;
  unsigned short *chunk;
  FILE           *OutFile;
  char           filname[512];


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
  while (!(chunk=malloc(RW_CHNK_S*128*1024))) {
    printf("Ooops, chunk malloc failed...\n"
	   "  ... please free up some memory and press return.\n");
    fgets((char *)cbuf,256,stdin);
  }

  srheadIn = (void*)srecIn;
  srheadOut = (void*)srecOut;

  /* a chunk is RW_CHNK_S/16 Mchannels, nmcperc minicubes */
  numchunks = (gd.nummc+nmcperc-1)/nmcperc;

  fprintf(gd.LogFile," ...Updating cube from scratch file:  %s\n", datim());
  fflush(gd.LogFile);
  printf( " ...Updating cube from scratch file:  %s\n", datim());
  printf("There are %d chunks to increment...\n",numchunks);

  /* loop through the cube files */
  totnumsr = 0;
  chunknum = 0;
  for (fnum=0; fnum<gd.numCubeFiles; fnum++) {

    /* open new output file for incremented copy of cube */
    strcpy(filname, gd.CubeFileName[fnum]);
    strcat(filname, ".tmp-increment-copy");
    if (!(OutFile=fopen(filname,"w+"))) {
      printf("\007  ERROR -- Cannot open new output cube file.\n");
      exit (-1);
    }
    fseek (gd.CubeFile[fnum], 0, SEEK_SET);
    if (!fread (&filehead, 1024, 1, gd.CubeFile[fnum])) {
      printf("\007  ERROR -- Cannot read file header, aborting...\n");
      exit(-1);
    }
    fseek (OutFile, 0, SEEK_SET);
    if (!fwrite (&filehead, 1024, 1, OutFile)) {
      printf("\007  ERROR -- Cannot write file header, aborting...\n");
      exit(-1);
    }
    srecnumIn = 0;      /* virtual SuperRecord for first chunk */
    oldsrnum = 0;

    /* read in first SuperRecord */
    for (j=0; gd.srvlut[fnum][j]!=srecnumIn; j++);
    fseek(gd.CubeFile[fnum], ((((long)j)*1024)+1)*1024, SEEK_SET);
    if (!fread(srecIn, 1024*1024, 1, gd.CubeFile[fnum])) {
      printf("\007  ERROR -- Corrupted cube, aborting...\n");
      exit(-1);
    }
    gd.srvlut[fnum][j] = -1;
    recIn = (Record *)&srecIn[1024];
    recnumIn = 0;
    mcptrIn = (unsigned char *)recIn + recIn->h.offset;

    /* init the first output SuperRecord */
    srecnumOut = 0;
    srheadOut->snum = 0;
    srheadOut->minmc = srheadIn->minmc;
    memcpy (srheadOut->resv, srheadIn->resv, 1012);
    recnumOut = 0;
    recOut = (Record *)&srecOut[1024];
    recOut->h.minmc = recIn->h.minmc;
    recOut->h.offset = 8;
    mcptrOut = (unsigned char *)recOut + recOut->h.offset;
  
    /* loop through all the chunks in the file */
    for (minmc=gd.mclo[fnum]; minmc<gd.mchi[fnum]; minmc+=nmcperc) {
      maxmc = minmc+nmcperc-1;
      if (maxmc>gd.mchi[fnum])
	maxmc = gd.mchi[fnum];
      dbinfo[0]=11;
      dbinfo[2]=0;
      dbinfo[3]=0;
      printf("\r  ...chunk %d, file %d, SRs %d %d, recs %d %d   ",
	     chunknum, fnum+1, oldsrnum,srecnumOut, recnumIn,recnumOut);
      fflush(stdout);

      /* loop through all the minicubes in the chunk */
      for (mc=minmc; mc<=maxmc; mc++) {
	dbinfo[2]=mc;

	if (mc > recIn->h.minmc + recIn->h.nummc - 1) {
	  /* next compressed minicube starts in the next input record */
	  if (++recnumIn == 1023) {
	    /* ooops, we need a new SuperRecord */
	    srecnumIn++;
	    oldsrnum++;
	    for (j=0; gd.srvlut[fnum][j]!=srecnumIn; j++);
	    fseek (gd.CubeFile[fnum], ((((long)j)*1024)+1)*1024, SEEK_SET);
	    if (!fread (srecIn, 1024*1024, 1, gd.CubeFile[fnum])) {
	      printf("\007  ERROR -- Corrupted cube, aborting...\n");
	      exit(-1);
	    }
	    gd.srvlut[fnum][j] = -1;
	    recIn = (Record *)&srecIn[1024];
	    recnumIn = 0;
	  }
	  else {
	    recIn = (Record *)((unsigned char *)recIn + 1024);
	  }

	  mcptrIn = (unsigned char *)recIn + recIn->h.offset;
	  /* at this point our minicube should be at the start of the record */
	  if (recIn->h.minmc != mc) {
	    printf("Severe ERROR 1 - fatal!\n PLEASE send a bug report"
		   " to radfordd@mail.phy,ornl.gov with this information\n"
		   "  and a copy of your get_event.c and 4play.h.\n");
	    printf("SR in,out: %d,%d  CurrentSR: %d\n"
		   "      rec: %d  mc: %d  should be %d\n",
		   oldsrnum,srecnumOut,srecnumIn, recnumIn, mc, recIn->h.minmc);
	    exit(-1);
	  }
	}
	else if (mcptrIn > (unsigned char *)recIn+1024 ) {
	  printf("Severe ERROR 2 - fatal!\n PLEASE send a bug report"
		 " to radfordd@mail.phy,ornl.gov with this information\n"
		 "  and a copy of your get_event.c and 4play.h.\n");
	  printf("SR in,out: %d,%d  CurrentSR: %d\n"
		 "      rec: %d  mc: %d  should be %d\n",
		 oldsrnum,srecnumOut,srecnumIn, recnumIn, mc,
		 recIn->h.minmc + recIn->h.nummc - 1);
	  exit(-1);
	}

	mcl=MCLEN(mcptrIn);
	if (mcptrIn + mcl > (unsigned char *)recIn+1024 ) {
	  memcpy (cbuf, mcptrIn, mcl);
	  memcpy (cbuf+(int)((unsigned char *)recIn + 1024 - mcptrIn),
		  (char*)recIn+1024+8, mcl);
	  decompress (cbuf, &chunk[(mc-minmc)<<8]);
	}
	else {
	  decompress (mcptrIn, &chunk[(mc-minmc)<<8]);
	  mcptrIn += mcl;
	}
      }

      /* increment the chunk from the buffers */
      addr12b = chunknum++;

      /* first empty the corresponding parts of buf1 */
      dbinfo[0]=12;
      dbinfo[2]=0;
      dbinfo[3]=0;
      for (addr20b=addr12b*RW_CHNK_S;
	   addr20b<(addr12b+1)*RW_CHNK_S; addr20b++) {
	addr24b = (addr20b - addr12b*RW_CHNK_S)<<16;
	for (j=0; j<nbuf1[addr20b]; j++)
	  chunk[addr24b+buf1[addr20b][j]]++;
	nbuf1[addr20b] = 0;
      }

      /* next empty the corresponding parts of buf2 */
      dbinfo[0]=13;
      for (j=0; j<nbuf2[addr12b]; j++) {
	addr24b = buf2[addr12b][j][RW_DB1]<<16;
	for (k=0; k<RW_DB1; k++)
	  chunk[addr24b+buf2[addr12b][j][k]]++;
      }
      nbuf2[addr12b] = 0;

      /* increment the chunk from the scratch file */
      dbinfo[0]=14;
      while (scrptr[addr12b]>0) {
	fseek(gd.ScrFile, ((long)scrptr[addr12b]-1)*RW_SCR_RECL, SEEK_SET);
	if (!fread(inbuf, RW_SCR_RECL, 1, gd.ScrFile)) {
	  fprintf(stderr,"\007  ERROR -- Could not read scr file.. fatal.\n");
	  exit(-1);
	}
	for (j=0; j<RW_DB2; j++) {
	  addr24b = inbuf[j][RW_DB1]<<16;
	  for (k=0; k<RW_DB1; k++)
	    chunk[addr24b+inbuf[j][k]]++;
	}
	memcpy(&(scrptr[addr12b]), &inbuf[RW_DB2][0], 4);
      }

      /* recompress and rewrite the chunk */
      /* loop through all the minicubes in the chunk */
      dbinfo[0]=15;
      for (mc=minmc; mc<=maxmc; mc++) {
	dbinfo[2]=mc;
	compress (&chunk[(mc-minmc)<<8], mcptrOut);
	mcl=MCLEN(mcptrOut);
	if (mcptrOut + mcl > (unsigned char *)recOut+1024 ) {
	  /* the minicube spills over the end of the output record */
	  memcpy (cbuf, mcptrOut, mcl);

	  if (recnumOut == 1022) {
	    /* ooops, we are at the end of our output SuperRecord */
	    if (gd.numsr[fnum]>=RW_MAX_FILE_SIZE-2) {
	      printf("\007  ERROR -- File is too big, max. = %dMB..."
		     " aborting.\n", RW_MAX_FILE_SIZE);
	      exit(-1);
	    }

	    recOut->h.nummc = mc - recOut->h.minmc;
	    srheadOut->maxmc = mc-1;
	    /* find the first empty SuperRecord on disk and write into it */
	    if (!fwrite(srecOut, 1024*1024, 1, OutFile)) {
	      printf("\007  ERROR -- Cannot write cube, aborting...\n");
	      exit(-1);
	    }
	    if (j >= gd.numsr[fnum]) {
	      gd.numsr[fnum]++;
	    }

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
    } /* end of loop through chunks in the file */

    /* write out the last output SuperRecord of this file */
    dbinfo[0]=16;
    recOut->h.nummc = gd.mchi[fnum] - recOut->h.minmc + 1;
    srheadOut->maxmc = gd.mchi[fnum];
    for (j=0; j<1022-recnumOut; j++) {
      recOut = (Record *)((unsigned char *)recOut + 1024);
      recOut->h.minmc = -1;
    }

    if (!fwrite(srecOut, 1024*1024, 1, OutFile)) {
      printf("\007  ERROR -- Cannot write cube, aborting...\n");
      exit(-1);
    }
    fflush(OutFile);

    for (j=0; j<=srecnumOut; j++){
      gd.srvlut[fnum][j] = j;
    }
    for (j=srecnumOut+1; j<RW_MAX_FILE_SIZE; j++){
      gd.srvlut[fnum][j] = -1;
    }

    gd.numsr[fnum] = srecnumOut + 1;

    fseek (OutFile, 0, SEEK_SET);
    if (!fread (&filehead, 1024, 1, OutFile)) {
      printf("\007  ERROR -- Corrupted file header, aborting...\n");
      exit(-1);
    }
    filehead.snum = gd.numsr[fnum];
    fseek (OutFile, 0, SEEK_SET);
    if (!fwrite (&filehead, 1024, 1, OutFile)) {
      printf("\007  ERROR -- Cannot write file header, aborting...\n");
      exit(-1);
    }

    if (gd.numsr[fnum]>=RW_MAX_FILE_SIZE*3/4) {
      printf("\007  WARNING -- File #%d is very big, max. = %dMB.\n",
	     fnum+1, RW_MAX_FILE_SIZE);
      printf("   ...Please consider running split4cub on it.\n");
    }
  
    fflush(OutFile);
    totnumsr += gd.numsr[fnum];

    fclose(gd.CubeFile[fnum]);
    gd.CubeFile[fnum] = OutFile;
    if (rename(filname, gd.CubeFileName[fnum])) {
      printf("\007  ERROR -- Cannot rename file, aborting...\n");
      exit(-1);
    }
  } /* end of loop through cube files */

  free(srecIn);
  free(srecOut);
  free(chunk);

  fprintf(gd.LogFile,"   ...Done updating cube:  %s\n"
	  "  There are now %d SuperRecords.\n", datim(), totnumsr);
  fflush(gd.LogFile);
  printf("\n   ...Done updating cube:  %s\n"
	 "  There are now %d SuperRecords.\n", datim(), totnumsr);
  dbinfo[0]=17;
}

void
dbuf2(int addr12b)
{
  char outbuf[RW_SCR_RECL];

  dbinfo[0]=8;
  dbinfo[4]=scrptr[addr12b];
  dbinfo[5]=gd.numscr;
  memcpy(outbuf, &(buf2[addr12b][0][0]), RW_DB2*2*(RW_DB1+1));
  memcpy(outbuf+RW_DB2*2*(RW_DB1+1), &(scrptr[addr12b]), 4);

  if (!fwrite(outbuf,RW_SCR_RECL,1,gd.ScrFile)) {
    fprintf(stderr,"\007  ERROR -- Could not write to scr file.. fatal.\n");
    exit(-1);
  }
  gd.numscr++;
  scrptr[addr12b] = gd.numscr;
  dbinfo[0]=9;

  if (gd.numscr*RW_SCR_RECL >= gd.ScrSize*1024*1024) {
    dbinfo[0]=10;
    /* scr full, inc cub */
    fprintf(gd.LogFile,"  Scratch File full...\n");
    printf("\n  Scratch File full...\n");
    dscr();
    gd.numscr = 0;
    rewind(gd.ScrFile);
  }
}

/*--------------------------------------------------------*/
void
breakhandler(int dummy)
{
   gd.gotsignal = 1;
}


/*--------------------------------------------------------*/
void
segvhandler(int dummy)
{
  char cbuf[256];

  gd.gotsignal = 1;
  printf("\007\n ACKKK!!!  Segmentation Violation!!!  Debug info follows.\n"
	 "%d %d %d %d %d %d %d %d %d %d\n",
	 dbinfo[0],dbinfo[1],dbinfo[2],dbinfo[3],dbinfo[4],
	 dbinfo[5],dbinfo[6],dbinfo[7],dbinfo[8],dbinfo[9]);
  while (1) {
    printf("  ... Kill? (N/Y)\n");
    if (!fgets(cbuf,256,stdin)) continue;;
    if (cbuf[0] == 'y' || cbuf[0] == 'Y')
      exit (1);
    if (cbuf[0] == 'n' || cbuf[0] == 'N')
      break;
  }
}

/*--------------------------------------------------------*/
void
sighandler()
{

   /* See signal(2) for details: 2 = SIGINT, 11 = SIGSEGV */
   /* Maybe this'll be different on different OSs so watch out. */
   signal(SIGINT, breakhandler);
   signal(SIGSEGV, segvhandler);
   gd.gotsignal = 0;
}
/*--------------------------------------------------------*/

#include "get_event.c"

int
main (int argc, char **argv)
{
  char cbuf[512];
  int maxrecords = RW_MAXRECORDS, numrecords, totnumrecords=0;
  int maxfiles = RW_MAXFILES, numfiles, totnumfiles=0;
  int numtapes;
  int scanning = 1;
  int prompt_tape = 1;
  int tfd=0;
  int mc_addr, inmc_addr, addr20b, addr16b, addr12b;
  int ew, ex, ey, ez;
  int i,j,k,l,tmp;
  int elist[RW_MAXMULT], gemult;
  unsigned long numincr = 0, numevents = 0;
  int skipforward = 0;
  int spec[16384];
  FILE *file;

  memset(spec,0,4096*4*4);
  setup_replay();

  numtapes = 0;
  while (scanning) { /* loop through tapes */
    sighandler();   /* set up signal handling */
    maxfiles = 0;
    numfiles = 0;
    maxrecords = 0;
    numrecords = 0;
    skipforward = 0;

    if ((file=fopen("4play.spn","w+"))) {
      fwrite(spec,4096*4,4,file);
      fclose(file);
    }

    while  (prompt_tape) {
      /* Prompt to enter new tape */
      printf ("\nInsert tape and press Return,\n"
	        "   S to skip forward over some data,\n"
	        "            C to change tape drives,\n"
	        "                        or E to end. ");
      if (!fgets(cbuf,512,stdin)) continue;
      if (cbuf[0] == 'e' || cbuf[0] == 'E') {
	scanning = 0;
	break;
      }

      else if (cbuf[0] == 's' || cbuf[0] == 'S')
	skipforward = 1;

      else if (cbuf[0] == 'c' || cbuf[0] == 'C') {
	printf ("  ...Enter new tape device: ");
	if (!fgets(cbuf,256,stdin)) continue;
        if (strlen(cbuf)<2)
	  continue;
	cbuf[strlen(cbuf)-1] = 0;    /* remove trailing \n */
	while (cbuf[strlen(cbuf)-1]==' ')
	  cbuf[strlen(cbuf)-1] = 0;  /* remove trailing spaces */
        if (strlen(cbuf)>2) {
	  strcpy(gd.TapeDev, cbuf);
	  printf ("  New tape device is %s.\n", gd.TapeDev);
	}
	continue;
      }

      /* open tape device */
      if ((tfd = open(gd.TapeDev, O_RDONLY, 0)) <= 0) {
	printf("\007  ERROR -- Could not open %s for reading.\n",gd.TapeDev);
	continue;
      }
      numtapes++;
      prompt_tape = 0;
    }
    if (!scanning)
      break;
 
    /* Get the maximum number of files on tape to be scanned */
    while (1) {
      if (skipforward)
	printf("\nNumber of files to skip (Return for all, or E to end): ");
      else
	printf("\nNumber of files to scan (Return for all, or E to end): ");
      if (!fgets(cbuf,512,stdin)) continue;
      if (cbuf[0] == 'e' || cbuf[0] == 'E') {
	scanning = 0;
	break;
      }
      if (cbuf[0] == '\n') {
	maxfiles = RW_MAXFILES;
	break;
      }
      maxfiles = strtol(cbuf, NULL, 0);
      if (maxfiles < 0) {
	printf("\007  ERROR -- Cannot be less than 0.\n");
      }
      else if (maxfiles == 0 && cbuf[0] != 0) {
	printf("\007  ERROR -- Invalid response.\n");
      }
      else if (maxfiles == 0) {
	scanning = 0;
      }
      else if (maxfiles > RW_MAXFILES) {
	maxfiles = 0;
	printf("\007  ERROR -- Value too large (max = %d).\n",RW_MAXFILES);
      }
      else {
	break;
      }
    }
    if (!scanning)
      break;

    /* Get the max number of records on tape to be scanned */
    while (1) {
      if (skipforward)
	printf("\nNumber of records to skip (Return for all):  ");
      else
	printf("\nNumber of records to scan (Return for all):  ");
      if (!fgets(cbuf,512,stdin)) continue;
      if (cbuf[0] == '\n') {
	maxrecords = RW_MAXRECORDS;
	break;
      }
      maxrecords = strtol(cbuf, NULL, 0);
      if (maxrecords < 0) {
	printf("\007  ERROR -- Cannot be less than 0.\n");
      }
      else if (maxrecords == 0 && cbuf[0] != 0) {
	printf("\007  ERROR -- Invalid response.\n");
      }
      else if (maxrecords == 0) {
	scanning = 0;
      }
      else if (maxrecords > RW_MAXRECORDS) {
	maxrecords = 0;
	printf("\007  ERROR -- Value too large (max = %d).\n",RW_MAXRECORDS);
      }
      else {
	break;
      }
    }
    if (!scanning)
      break;

    sprintf(cbuf,"Scan started:  %s\n"
	    "  Hit control-C to interrupt at any time...\n\n", datim());
    printf("%s",cbuf);
    fflush(stdin);
    fprintf(gd.LogFile,"%s",cbuf);
    fflush(gd.LogFile);

    /* process events */
    while (numrecords < maxrecords && numfiles < maxfiles) {
      if (gd.gotsignal)
	break;

      /* read in buffer */
      dbinfo[0]=0;
      i = read_tape(tfd);
      if (i<0) {
	/* read returned unknown error */
	printf("\007\n  ERROR -- Could not read from %s.\n", gd.TapeDev);
	sprintf(cbuf,"End-of-Tape reached??  %s\n\n", datim());
	printf("%s",cbuf);
	fprintf(gd.LogFile,"%s",cbuf);
	prompt_tape = 1;

	printf("Closing tape device %s...\n",gd.TapeDev);
	fprintf(gd.LogFile,"Closing tape device %s...\n",gd.TapeDev);
	if (close(tfd)) {
	  printf("\007 Ooops, could not close tape device!\n");
	  fprintf(gd.LogFile," Ooops, could not close tape device!\n");
	}

	break;
      }
      else if (i==0) {
	/* end of file */
	/* first check to see if we are testing the program */
	if (strcmp(gd.TapeDev,"/dev/null")) {
	  /* no, we are not using a null device */
	  sprintf(cbuf,"End-of-File reached  %s\n\n", datim());
	  printf("\n%s",cbuf);
	  fprintf(gd.LogFile,"%s",cbuf);
	  numfiles++; totnumfiles++;
	  continue;
	}
      }
      else if (i<gd.RecordSize) {
	/* full buffer not read, throw it away. */
	printf("\n  Discarding tape buffer with %d bytes!\n", i);
	fflush(stdin);
	continue;
      }

      /* no worries */
      numrecords++; totnumrecords++;
      dbinfo[9]=numrecords;

      if (skipforward) {
	if ((numrecords&31) == 0) {
	  printf("\r %d records skipped...", numrecords);
	  fflush(stdout);
	}
	continue;
      }

      dbinfo[0]=1;
      while ((gemult = get_event(elist)) >= 0) {
	dbinfo[0]=2;
	dbinfo[1]=gemult;
        spec[8192+gemult]++;
	numevents++;
	dbinfo[8]=numevents;
        if (gemult < 4) continue;

	for (i=0; i<gemult; i++) {
	  if (elist[i]<16384 && elist[i]>=0)
	    spec[elist[i]]++;
	}

	/* convert from ADC to cube channel numbers */
	dbinfo[0]=3;
	j = 0;
	for (i=0; i<gemult; i++) {
	  if (elist[i] >= 0 && elist[i] < gd.adcmax && luch[elist[i]]>0)
	    elist[j++] = luch[elist[i]]-1;
	}
	gemult = j;
        if (gemult < 4) continue;
        spec[12288+gemult]++;

	dbinfo[0]=4;
	dbinfo[1]=gemult;

	/* order elist */
	for (i=gemult-1; i>0; i--) {
	  if(elist[i] < elist[i-1]) {
	    tmp=elist[i]; elist[i]=elist[i-1]; elist[i-1]=tmp;
	    j=i;
	    while (j<gemult-1 && elist[j] > elist[j+1]) {
	      tmp=elist[j]; elist[j]=elist[j+1]; elist[j+1]=tmp;
	      j++;
	    }
	  }
	}
	dbinfo[0]=5;

	/* loop through all possible combinations */
	for(l=3;l<gemult;l++) {
	  ez = elist[l];
	  
	  for(k=2;k<l;k++) {
	    ey = elist[k];

	    for(j=1;j<k;j++) {
	      ex = elist[j];

	      for(i=0;i<j;i++) {
		ew = elist[i];
		numincr++;

		/* linear address (LUMC first 28bits, LUIMC last 8bits) */
		dbinfo[0]=6;
		dbinfo[2]=0;
		dbinfo[3]=0;
		dbinfo[4]=0;
		dbinfo[5]=0;
		mc_addr = LUMC (ew,ex,ey,ez);
		inmc_addr = LUIMC (ew,ex,ey,ez);
		dbinfo[2]=mc_addr;
		dbinfo[3]=inmc_addr;
		addr20b = mc_addr / 256;
		addr16b = (mc_addr&255)*256 + inmc_addr;
		dbinfo[4]=addr20b;
		dbinfo[5]=nbuf1[addr20b];
		buf1[addr20b][nbuf1[addr20b]++] = addr16b;

		if (nbuf1[addr20b] == RW_DB1) {  /* dump buf1 to buf2 */
		  dbinfo[0]=7;
		  dbinfo[6]=0;
		  dbinfo[7]=0;
		  addr12b = addr20b / RW_CHNK_S;
		  dbinfo[6]=addr12b;
		  dbinfo[7]=nbuf2[addr12b];
		  memcpy(&(buf2[addr12b][nbuf2[addr12b]][0]),
			 &(buf1[addr20b][0]),
			 RW_DB1*2); /* copy whole row into buf2 */
		  buf2[addr12b][nbuf2[addr12b]++][RW_DB1] =
		    (addr20b-(addr12b*RW_CHNK_S));
		  nbuf1[addr20b] = 0;

		  if (nbuf2[addr12b] == RW_DB2) {
		    dbuf2(addr12b);  /* dump buf2 to scratch file */
		    nbuf2[addr12b] = 0;
		  }
		}
		dbinfo[0]=5;
	      }
	    }
	  }
	}
	dbinfo[0]=1;
      }

      if ((numrecords&7) == 0) {
	printf("\r %d recs, %lu evnts, %lu incrs, %d scr bufs...",
	       numrecords, numevents, numincr, gd.numscr);
	fflush(stdout);
      }
    }
  }

  dscr();

  sprintf(cbuf,"Scan completed:  %s\n\n"
	       "      Number of tapes scanned = %d\n"
	       "   Total number of files read = %d\n"
	       " Total number of records read = %d\n"
	       "       Total number of events = %lu\n"
	       "   Total number of increments = %lu\n",
	  datim(),numtapes,totnumfiles,totnumrecords,numevents,numincr);
  printf("%s",cbuf);
  fprintf(gd.LogFile,"%s",cbuf);


  fflush(stdout);
  fclose(gd.LogFile);
  exit(0);
}
