/***************************************************************************
   incub8r3: Replay program for compressed triple coincidence cubes.
   Allows use of non-linear gains through look-up tables
     to convert gain-matched ADC outputs to cube channels.
   Uses disk buffering to .scr "scratch" file.
   This version for 1/6 cubes.
   D.C Radford     Sept  1997
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

#include "incub8r3.h"
#include "rwutils.c"

#define VERSION "23 Sept 1997   Beta"

/* Some global data */
struct GlobalData {
  FILE *CubeFile;
  FILE *ScrFile;
  FILE *LogFile;
  char CubeFileName[256];
  int  ScrSize, RecordSize;
  char TapeDev[256];
  int  SwapTapeBytes;
  int  length;     /* length of axis on cube */
  int  nummc;      /* number of minicubes */
  int  numscr;     /* number of records written to scrfile */
  int  adcmax;
  int  numrecs;    /* current number of records in cube file */
  int  gotsignal;  /* a signal was caught by signal handler */
} gd;

int dbinfo[10];

/* lookup table stuff */
char *tbuf;           /* tape buffer */
short luch[16384];    /* look-up table, maps ADC channels to cube channels */
int lumx[RW_MAXCH];   /* look-up table, maps 3d ch to linear minicube */
int lumy[RW_MAXCH];
int lumz[RW_MAXCH];
  /* look up minicube addr */
#define LUMC(x,y,z) lumx[x] + lumy[y] + lumz[z]
  /* look up addr in minicube */
#define LUIMC(x,y,z) (x&7) + ((y&7)<<3) + ((z&3)<<6)

/* increment buffers */

unsigned short buf1[RW_LB1][RW_DB1];
unsigned short buf2[RW_LB2][RW_DB2][RW_DB1+1];

int nbuf1[RW_LB1];       /* number in each row of buf1 */
int nbuf2[RW_LB2];       /* number in each row of buf2 */
int scrptr[RW_LB2];      /* last record # written to scr file */
                         /* for each row of buf2              */

#define MCLEN(mcptr) (mcptr[0] + (mcptr[0] >= 7*32 ? mcptr[1]*32+2 : 1))


FILE *open_3cube (char *name, int length, FILE *mesg);
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

  printf("\n\n"
	 " Incub8r3: A replay program for compressed-format RadWare\n"
	 "    triple-coincidence cubes.\n"
	 " Allows use of non-linear gains through look-up tables\n"
	 "    to convert gain-matched ADC outputs to cube channels.\n"
	 " Uses disk buffering to .scr scratch file.\n"
	 " This version for 1/6 cubes.\n\n"
	 "  Author     D.C Radford    Sept 1997\n\n"
	 "  Version "VERSION"\n\n");

  if ((c=getenv("RADWARE_OVERWRITE_FILE")) &&
      (*c == 'y' || *c == 'Y')) ovrtprompt = 0;

  while (1) { /* while data(inc) file is valid */

    /* First read in the data file */
    printf("Enter replay data file name (%s): ",RW_DATFILE);
    if (!fgets(dnam, 250, stdin)) continue;
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
    for(c=a+strlen(a)-1;*c==' ';c--) *c=0; /* remove trailing spaces */

    GETDATA(title,"title");
    GETDATA(tnam, "tape drive");
    GETDATA(snam, "scratch file name");
    GETDATA(cnam, "cube file name");
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
           "                  Cube file name = %s\n"
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
	  if (fgets(cbuf,256,stdin) && cbuf[0] != '\n') {
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
	if (fgets(cbuf,256,stdin) && cbuf[0] != '\n') {
	  strncpy(tnam,cbuf,strlen(cbuf)-1);
	  setfnamext(tnam,".log",0);
	  continue;
	}
      }
      break;
    }

    /* read in lookup table */
    if (read_tab(lnam,&nclook,&lmin,&lmax,luch)) {
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

    /* Inform the log file */
    fprintf(gd.LogFile,"Replay log for program Incub8r3,  %s\n\n",datim());
    fprintf(gd.LogFile,"\n           Replay data file name = %s\n"
           "                      Tape drive = %s\n"
           "               Scratch file name = %s\n"
           "                  Cube file name = %s\n"
           "         Look-up table file name = %s\n"
           "        Record BlockSize (Bytes) = %s\n"
           "      Scratch File Size (MBytes) = %s\n"
           "      Swap Bytes from Tape (y/n) = %s\n\n",
	   dnam,gd.TapeDev,snam,cnam,lnam,recs,scrs,stb);
    fprintf(gd.LogFile," No. of values in lookup table = %d\n\n",lmax);
    fprintf(gd.LogFile,"-------------------------------------------\n\n");

    /* calculate look-up tables to convert (x,y,z) cube address 
          to linearized minicube address.  x<y<z
       lum{x,y,z}[ch] returns the number of subcubes to skip over
          to get to the subcube with ch in it.
    */
    for (i=0;i<8;i++) {
      lumx[i] = 0;
      lumy[i] = 0;
      lumz[i] = i/4;
    }
    for (i=8;i<gd.length;i++) {
      lumx[i] = (i/8)*2;
      lumy[i] = lumy[i-8] + lumx[i];
      lumz[i] = lumz[i-8] + lumy[i];
    }
 
    /* open cube and scratch */
    if (!(gd.CubeFile=open_3cube(cnam, gd.length, gd.LogFile))) {
      fclose(gd.LogFile);
      continue;
    }
    if (!(gd.ScrFile = open_scr(snam, gd.ScrSize, gd.LogFile))) {
      fclose(gd.CubeFile);
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
open_3cube (char *name, int length, FILE *mesg)
{
  FILE     *file;
  int      i, nummc;
  FHead3D  head;
  Record3D rec;


  if (!name)
    return NULL;
  if (!(file=fopen(name,"r+"))) {
    if (!(file=fopen(name,"w+"))) {
      printf("\007  ERROR -- Cannot open file %s.\n", name);
      return NULL;
    }
  }
  rewind(file);

  nummc = (length + 7)/8;
  nummc = nummc*(nummc+1)*(nummc+2)/3; /* minicubes in cube */
  gd.nummc = nummc;

  switch (fread(&head,1,1024,file)) {
  default:
    fprintf(stderr,"  WARNING -- partial header on cube. Junking it.\n");
  case 0:
    gd.numrecs = (nummc + 4087)/4088; /* note, an empty minicube is 1 byte */
    strncpy(head.id, "Incub8r3/Pro4d  ", 16);
    head.numch = length;
    head.bpc = 4;
    head.cps = 6;
    head.numrecs = gd.numrecs;
    memset (head.resv, 0, 992);

    fprintf(mesg,"  Creating new cube: %d channels per side...\n",length);
    printf("  Creating new cube: %d channels per side...\n",length);
    rewind(file);
    if (!fwrite(&head,1024,1,file)) {
      fprintf(stderr, "\007  ERROR -- Could not write cube header... "
	      "aborting.\n");
      fclose(file);
      return NULL;
    }
    /* initialize cube */
    for (i=0;i<4088;i++)
      rec.d[i] = 0;

    for (i=0; i<gd.numrecs; i++) { /* loop through records */
      rec.h.minmc = i*4088;
      rec.h.nummc = 4088;
      rec.h.offset = 8;
      if(!fwrite(&rec,4096,1,file)) {
	fprintf(stderr, "\007  ERROR -- Could not write record %d... "
		"aborting.\n",i);
	fclose(file);
	return NULL;
      }
    }
    fflush(file);
    printf("  ...Done creating new cube.\n");
    break;

  case 1024:
    fprintf(mesg,"  Checking existing cube file...\n");
    printf("  Checking existing cube file...\n");
    if (strncmp(head.id,"Incub8r3/Pro4d  ",16) ||
	head.bpc != 4 ||
	head.cps != 6) {
      fprintf(stderr,"\007  ERROR -- Invalid header... aborting.\n");
      fclose(file);
      return NULL;
    }
    else if (head.numch != length) {
      fprintf(stderr,"\007  ERROR -- Different axis length in cube (%d)..\n",
	      head.numch);
      fclose(file);
      return NULL;
    }
    gd.numrecs = head.numrecs;
    printf("  ...Okay, %d records.\n", gd.numrecs);
  }

  fprintf(mesg,
	  "Axis length of 3d cube is %d.\n"
	  "3d cube has %d minicubes and %d records.\n",
	  length, nummc, head.numrecs);

  printf("Axis length of 3d cube is %d.\n"
	  "3d cube has %d minicubes and %d records.\n",
	  length, nummc, head.numrecs);

  strcpy(gd.CubeFileName, name);
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
  for (i=0;i<length*1024;i++) /* writing to make sure we have space */
    if (!fwrite(b, 1024, 1, file)) {
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

  ret = read(fd, tbuf, gd.RecordSize);

  if (ret < 0)
    return -1;
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

#include "compress3d.c"
void
dscr(void)
{
  int            numchunks, chunknum;
  Record3D       recIn, recOut;
  int            j, k, mc, mcl;
  unsigned char  *mcptrIn, *mcptrOut, cbuf[2048];
  int            minmc, maxmc;
  int            recnumIn, recnumOut, nbytes;
  int            addr13b, addr8b, addr21b;
  int            nmcperc=RW_CHNK_S*256;
  unsigned short inbuf[RW_DB2+1][RW_DB1+1];
  FHead3D        filehead;
  unsigned int   *chunk;
  FILE           *OutFile;
  char           filname[512];

  while (!(chunk=malloc(RW_CHNK_S*256*1024))) {
    printf("Ooops, chunk malloc failed...\n"
	   "  ... please free up some memory and press return.\n");
    fgets(filname,256,stdin);
  }

  /* open new output file for incremented copy of cube */
  strcpy(filname, gd.CubeFileName);
  strcat(filname, ".tmp-increment-copy");
  if (!(OutFile=fopen(filname,"w+"))) {
    printf("\007  ERROR -- Cannot open new output cube file.\n");
    exit (-1);
  }
  fseek (gd.CubeFile, 0, SEEK_SET);
  if (!fread (&filehead, 1024, 1, gd.CubeFile)) {
    printf("\007  ERROR -- Cannot read file header, aborting...\n");
    exit(-1);
  }
  fseek (OutFile, 0, SEEK_SET);
  if (!fwrite (&filehead, 1024, 1, OutFile)) {
    printf("\007  ERROR -- Cannot write file header, aborting...\n");
    exit(-1);
  }

  /* a chunk is RW_CHNK_S/16 Mchannels, nmcperc minicubes */
  numchunks = (gd.nummc+nmcperc-1)/nmcperc;

  fprintf(gd.LogFile," ...Updating cube from scratch file:  %s\n", datim());
  fflush(gd.LogFile);
  printf( " ...Updating cube from scratch file:  %s\n", datim());
  printf("There are %d chunks to increment...\n",numchunks);

  /* read in first record */
  recnumIn = 0;
  fseek(gd.CubeFile, 1024, SEEK_SET);
  if (!fread(&recIn, 4096, 1, gd.CubeFile)) {
    printf("\007  ERROR -- Corrupted cube, aborting...\n");
    exit(-1);
  }
  mcptrIn = recIn.d;

  /* init the first output record */
  recnumOut = 0;
  recOut.h.minmc = recIn.h.minmc;
  recOut.h.offset = 8;
  mcptrOut = recOut.d;
  
  /* loop through all the chunks in the file */
  for (chunknum=0; chunknum<numchunks; chunknum++) {
    minmc = chunknum*nmcperc;
    maxmc = minmc+nmcperc-1;
    if (maxmc>gd.nummc-1)
      maxmc = gd.nummc - 1;
    dbinfo[0]=11;
    dbinfo[2]=0;
    dbinfo[3]=0;
    printf("\r  ...chunk %d, recs %d %d   ",chunknum,recnumIn,recnumOut);
    fflush(stdout);

    /* loop through all the minicubes in the chunk */
    for (mc=minmc; mc<=maxmc; mc++) {
      dbinfo[2]=mc;

      if (mc > recIn.h.minmc + recIn.h.nummc - 1) {
	/* next compressed minicube starts in the next input record */
	if (!fread (&recIn, 4096, 1, gd.CubeFile)) {
	  printf("\007  ERROR -- Corrupted cube, aborting...\n");
	  exit(-1);
	}
	recnumIn++;
	mcptrIn = recIn.d + recIn.h.offset - 8;
	/* at this point our minicube should be at the start of the record */
	if (recIn.h.minmc != mc) {
	  printf("Severe ERROR 1 - fatal!\n PLEASE send a bug report"
		 " to radfordd@mail.phy,ornl.gov with this information\n"
		 "  and a copy of your get_event.c and incub8r3.h.\n");
	  printf("rec: %d  mc: %d  should be %d\n",
		 recnumIn, mc, recIn.h.minmc);
	  exit(-1);
	}
      }
      else if (mcptrIn > recIn.d + 4088 ) {
	printf("Severe ERROR 2 - fatal!\n PLEASE send a bug report"
	       " to radfordd@mail.phy,ornl.gov with this information\n"
	       "  and a copy of your get_event.c and incub8r3.h.\n");
	printf("rec: %d  mc: %d  should be > %d\n",
	       recnumIn, mc, recIn.h.minmc + recIn.h.nummc - 1);
	exit(-1);
      }

      mcl=MCLEN(mcptrIn);
      if (mcptrIn + mcl > recIn.d + 4088 ) {
	/* compressed minicube spills over into the next input record */
	nbytes = mcptrIn + mcl - (recIn.d + 4088);
	memcpy (cbuf, mcptrIn, mcl-nbytes);
	if (!fread (&recIn, 4096, 1, gd.CubeFile)) {
	  printf("\007  ERROR -- Corrupted cube, aborting...\n");
	  exit(-1);
	}
	if (nbytes != (recIn.h.offset-8)) {
	  printf("Severe ERROR 3 - fatal!\n PLEASE send a bug report"
		 " to radfordd@mail.phy,ornl.gov with this information\n"
		 "  and a copy of your get_event.c and incub8r3.h.\n");
	  printf("rec, offset, nbytes, mcl, mcptr: %d %d %d %d %ld\n"
		 "mc minmc nummc: %d %d %d\n",
		 recnumIn+1, recIn.h.offset, nbytes ,mcl,
		 (long int) (mcptrIn-recIn.d+8),
                 mc, recIn.h.minmc, recIn.h.nummc);
	  exit(-1);
	}
	if (recIn.h.minmc != mc+1) {
	  printf("Severe ERROR 4 - fatal!\n PLEASE send a bug report"
		 " to radfordd@mail.phy,ornl.gov with this information\n"
		 "  and a copy of your get_event.c and incub8r3.h.\n");
	  printf("rec: %d  mc: %d  should be %d\n",
		 recnumIn+1, mc+1, recIn.h.minmc);
	  exit(-1);
	}
	recnumIn++;
	memcpy (&cbuf[mcl-nbytes], recIn.d, nbytes);
	decompress3d (cbuf, &chunk[(mc-minmc)<<8]);
	mcptrIn = recIn.d + recIn.h.offset - 8;
      }
      else {
	decompress3d (mcptrIn, &chunk[(mc-minmc)<<8]);
	mcptrIn += mcl;
      }
    }
    
    /* increment the chunk from the buffers */
    addr8b = chunknum;

    /* first empty the corresponding parts of buf1 */
    dbinfo[0]=12;
    dbinfo[2]=0;
    dbinfo[3]=0;
    for (addr13b=addr8b*RW_CHNK_S;
	 addr13b<(addr8b+1)*RW_CHNK_S; addr13b++) {
      addr21b = (addr13b - addr8b*RW_CHNK_S)<<16;
      for (j=0; j<nbuf1[addr13b]; j++)
	  chunk[addr21b+buf1[addr13b][j]]++;
      nbuf1[addr13b] = 0;
    }

    /* next empty the corresponding parts of buf2 */
    dbinfo[0]=13;
    for (j=0; j<nbuf2[addr8b]; j++) {
      addr21b = buf2[addr8b][j][RW_DB1]<<16;
      for (k=0; k<RW_DB1; k++)
	chunk[addr21b+buf2[addr8b][j][k]]++;
    }
    nbuf2[addr8b] = 0;

    /* increment the chunk from the scratch file */
    dbinfo[0]=14;
    while (scrptr[addr8b]>0) {
      fseek(gd.ScrFile, ((long)scrptr[addr8b]-1)*RW_SCR_RECL, SEEK_SET);
      if (!fread(inbuf, RW_SCR_RECL, 1, gd.ScrFile)) {
	fprintf(stderr,"\007  ERROR -- Could not read scr file.. fatal.\n");
	exit(-1);
      }
      for (j=0; j<RW_DB2; j++) {
	addr21b = inbuf[j][RW_DB1]<<16;
	for (k=0; k<RW_DB1; k++)
	  chunk[addr21b+inbuf[j][k]]++;
      }
      memcpy(&(scrptr[addr8b]), &inbuf[RW_DB2][0], 4);
    }

    /* recompress and rewrite the chunk */
    /* loop through all the minicubes in the chunk */
    dbinfo[0]=15;
    for (mc=minmc; mc<=maxmc; mc++) {
      dbinfo[2]=mc;
      mcl = compress3d (&chunk[(mc-minmc)<<8], cbuf);
      if (mcptrOut + mcl > recOut.d + 4088) {
	/* the minicube spills over the end of the output record */
	if (mcptrOut + 2 > recOut.d + 4088) {
	  /* need at least first 2 bytes of minicube in current record */
	  /* so move whole minicube to next record */
	  recOut.h.nummc = mc - recOut.h.minmc;
	  if (!fwrite(&recOut, 4096, 1, OutFile)) {
	    printf("\007  ERROR -- Cannot write cube, aborting...\n");
	    exit(-1);
	  }
	  recOut.h.minmc = mc;
	  recOut.h.offset = 8;
	  memcpy (recOut.d, cbuf, mcl);
	  mcptrOut = recOut.d + recOut.h.offset - 8 + mcl;
	}
	else {
	  /* move only part of minicube to next record */
	  nbytes = mcptrOut + mcl - (recOut.d + 4088);
	  memcpy (mcptrOut, cbuf, mcl-nbytes);
	  recOut.h.nummc = mc - recOut.h.minmc + 1;
	  if (!fwrite(&recOut, 4096, 1, OutFile)) {
	    printf("\007  ERROR -- Cannot write cube, aborting...\n");
	    exit(-1);
	  }
	  recOut.h.minmc = mc + 1;
	  recOut.h.offset = nbytes + 8;
	  memcpy (recOut.d, cbuf+mcl-nbytes, nbytes);
	  mcptrOut = recOut.d + recOut.h.offset - 8;
	}
	recnumOut++;
      }
      else {
	memcpy (mcptrOut, cbuf, mcl);
	mcptrOut += mcl;
      }
    }
  } /* end of loop through chunks in the file */

  /* write out the last record */
  dbinfo[0]=16;
  recOut.h.nummc = gd.nummc - recOut.h.minmc;
  if (!fwrite(&recOut, 4096, 1, OutFile)) {
    printf("\007  ERROR -- Cannot write cube, aborting...\n");
    exit(-1);
  }
  recnumOut++;

  fseek (OutFile, 0, SEEK_SET);
  if (!fread (&filehead, 1024, 1, OutFile)) {
    printf("\007  ERROR -- Corrupted file header, aborting...\n");
    exit(-1);
  }
  filehead.numrecs = recnumOut;
  fseek (OutFile, 0, SEEK_SET);
  if (!fwrite (&filehead, 1024, 1, OutFile)) {
    printf("\007  ERROR -- Cannot write file header, aborting...\n");
    exit(-1);
  }
  fflush(OutFile);

  free(chunk);
  fclose(gd.CubeFile);
  gd.CubeFile = OutFile;
  if (rename(filname, gd.CubeFileName)) {
    printf("\007  ERROR -- Cannot rename file, aborting...\n");
    exit(-1);
  }

  fprintf(gd.LogFile,"   ...Done updating cube:  %s\n"
	  "  There are now %d records.\n", datim(), recnumOut);
  fflush(gd.LogFile);
  printf("\n   ...Done updating cube:  %s\n"
	 "  There are now %d records.\n", datim(), recnumOut);
  dbinfo[0]=17;
}

void
dbuf2(int addr8b)
{
  char outbuf[RW_SCR_RECL];

  dbinfo[0]=8;
  dbinfo[4]=scrptr[addr8b];
  dbinfo[5]=gd.numscr;
  memcpy(outbuf, &(buf2[addr8b][0][0]), RW_DB2*2*(RW_DB1+1));
  memcpy(outbuf+RW_DB2*2*(RW_DB1+1), &(scrptr[addr8b]), 4);

  if (!fwrite(outbuf,RW_SCR_RECL,1,gd.ScrFile)) {
    fprintf(stderr,"\007  ERROR -- Could not write to scr file.. fatal.\n");
    exit(-1);
  }
  gd.numscr++;
  scrptr[addr8b] = gd.numscr;
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
    if (!fgets(cbuf,256,stdin)) continue;
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
  int mc_addr, inmc_addr, addr13b, addr16b, addr8b;
  int ex, ey, ez;
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

    if ((file=fopen("incub8r3.spn","w+"))) {
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

    sprintf(cbuf,"Scan started:  %s\n\n", datim());
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
	  printf("\r %d records skipped...",
		 numrecords);
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
        if (gemult < 3) continue;

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
        if (gemult < 3) continue;
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
	for(l=2;l<gemult;l++) {
	  ez = elist[l];
	  
	  for(k=1;k<l;k++) {
	    ey = elist[k];

	    for(j=0;j<k;j++) {
	      ex = elist[j];
	      numincr++;

	      /* linear address (LUMC first 21bits, LUIMC last 8bits) */
	      dbinfo[0]=6;
	      dbinfo[2]=0;
	      dbinfo[3]=0;
	      dbinfo[4]=0;
	      dbinfo[5]=0;
	      mc_addr = LUMC (ex,ey,ez);
	      inmc_addr = LUIMC (ex,ey,ez);
	      dbinfo[2]=mc_addr;
	      dbinfo[3]=inmc_addr;
	      addr13b = mc_addr / 256;
	      addr16b = (mc_addr&255)*256 + inmc_addr;
	      dbinfo[4]=addr13b;
	      dbinfo[5]=nbuf1[addr13b];
	      buf1[addr13b][nbuf1[addr13b]++] = addr16b;

	      if (nbuf1[addr13b] == RW_DB1) {  /* dump buf1 to buf2 */
		dbinfo[0]=7;
		dbinfo[6]=0;
		dbinfo[7]=0;
		addr8b = addr13b / RW_CHNK_S;
		dbinfo[6]=addr8b;
		dbinfo[7]=nbuf2[addr8b];
		memcpy(&(buf2[addr8b][nbuf2[addr8b]][0]),
		       &(buf1[addr13b][0]),
		       RW_DB1*2); /* copy whole row into buf2 */
		buf2[addr8b][nbuf2[addr8b]++][RW_DB1] =
		  (addr13b-(addr8b*RW_CHNK_S));
		nbuf1[addr13b] = 0;

		if (nbuf2[addr8b] == RW_DB2) {
		  dbuf2(addr8b);  /* dump buf2 to scratch file */
		  nbuf2[addr8b] = 0;
		}
	      }
	      dbinfo[0]=5;
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
