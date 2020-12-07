/*  4 bytes-per-channel (.spn) symmetric matrix replay program */
/*  D.C. Radford     Nov 1997                                  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
/* #include <sys/mtio.h> */
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#define VERSION "12 November 1997   Beta"
#define RW_DATFILE "matrix.inc"
#define RW_MAXFILES 9999         /* Max files per tape to read */
#define RW_MAXRECORDS 1000000000 /* Max number of records per tape to read */
#define RW_MAXMULT 40

/* Some global data */
struct GlobalData {
  FILE *LogFile;
  FILE *MatFile;
  char  TapeDev[256];
  int   SwapTapeBytes;
  int   adcmax;
  int   RecordSize;
  int   gotsignal;             /* a signal was caught by signal handler */
} gd;

char *tbuf;                     /* tape buffer */
unsigned int *mat, spec[4096];  /* matrix and 1D spectrum */
int luych[4096];

#include "rwutils.c"

void setup_replay(void)
{
  int  i, j, ovrtprompt = 1;
  char cbuf[256], dnam[256], mnam[256], title[256], tnam[256];
  char recs[256], stb[256];
  char *c;
  FILE *file;

  printf("\n\n"
	 "   Matrix...\n\n"
	 "   Creates 4 bytes-per-channel (.spn) symmetric RadWare matrix\n"
	 "   D.C. Radford\n"
	 "   Version "VERSION"\n\n");

  luych[0] = 0;
  for (i=1; i<4096; i++)
    luych[i] = luych[i-1] + i;

  if ((c=getenv("RADWARE_OVERWRITE_FILE")) &&
      (*c == 'y' || *c == 'Y')) ovrtprompt = 0;

  while (1) { /* while data(inc) file is valid */

    /* First read in the data file */
    printf("Enter replay data file name (%s): ",RW_DATFILE);
    fgets(dnam, 250, stdin);
    dnam[strlen(dnam)-1] = 0;  /* remove \n */
    if (dnam[0] == 0) strncpy(dnam, RW_DATFILE, 256);
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
    GETDATA(mnam, "matrix file name");
    GETDATA(recs, "record blocksize");
    GETDATA(stb,  "swap bytes from tape");
#undef GETDATA

    fclose(file);
    if (stb[0] == 'y' || stb[0] == 'Y') {
      strcpy(stb,"y");
      gd.SwapTapeBytes = 1;
    } else {
      strcpy(stb,"n");
      gd.SwapTapeBytes = 0;
    }

    printf("\n           Replay data file name = %s\n"
           "                      Tape drive = %s\n"
           "         Output matrix file name = %s\n"
           "        Record BlockSize (Bytes) = %s\n"
           "      Swap Bytes from Tape (y/n) = %s\n\n",
	   dnam,tnam,mnam,recs,stb);

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

    strcpy(tnam,"matrix.log");
    while (1) {
      if ((gd.LogFile=fopen(tnam,"r"))) {
	fclose(gd.LogFile);
        if (ovrtprompt) {
	  printf("  WARNING: Log file (%s) already exists\n",tnam);
	  printf("  Enter new name or <Return> to overwrite: ");
	  fgets(cbuf,256,stdin);
	  if (cbuf[0] != '\n') {
	    cbuf[strlen(cbuf)-1]=0;
	    strcpy(tnam,cbuf);
	    if (!(c=strrchr(tnam,'.'))) strcat(tnam,".log");
	    continue;
	  }
	} else {
	  printf("  Overwriting Log file (%s).\n",tnam);
	}
      }
      if (!(gd.LogFile=fopen(tnam,"w"))) {
	printf("\007  ERROR -- Could not open %s for writing.\n",tnam);
    	printf("  Enter new log file name: ");
	fgets(cbuf,256,stdin);
	if (cbuf[0] != '\n') {
	  strncpy(tnam,cbuf,strlen(cbuf)-1);
	  continue;
	}
      }
      break;
    }

    setfnamext(mnam,".spn",0);
    if (!(gd.MatFile = fopen(mnam,"r+"))) {
      printf("\n Matrix file %s does not exist, will create it...\n",mnam);
      if (!(gd.MatFile = fopen(mnam,"w+"))) {
        printf("\007 ERROR -- Could not open %s for writing.\n",mnam);
	fclose(gd.LogFile);
	continue;
      }
    } else {
      printf ("\n Reading matrix from %s...\n",mnam);
      rewind(gd.MatFile);
      for (i=0; i<4096; i++) {
	if (fread(spec,4096*4,1,gd.MatFile)<1) {
          printf("\007 ERROR -- Could not read row %d from %s.\n",i,mnam);
	  printf("      ...terminating...\n\n");
	  exit(-1);
	}
	spec[i] = spec[i]/2;
	for (j=0; j<=i; j++)
	  mat[luych[i]+j] = spec[j];
      }
    }

    /* Inform the log file */
    fprintf(gd.LogFile,"Replay log for program Matrix,  %s\n\n",datim());
    fprintf(gd.LogFile,"\n           Replay data file name = %s\n"
           "                      Tape drive = %s\n"
           "         Output matrix file name = %s\n"
           "        Record BlockSize (Bytes) = %s\n"
           "      Swap Bytes from Tape (y/n) = %s\n\n",
	   dnam,gd.TapeDev,mnam,recs,stb);
    fprintf(gd.LogFile,"-------------------------------------------\n\n");

    /* malloc tape buffer(s) */
    if (!(tbuf = malloc(gd.RecordSize))) {
      fprintf(stderr,"\007  ERROR: Could not malloc tape buffer (%d bytes).\n",
	      gd.RecordSize);
      fprintf(stdout,"terminating...\n");
      exit(-1);
    }

    fflush(gd.LogFile);
    break;
  }
}

int read_tape(int fd)
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
     memcpy(data, tbuf, 80);
     for (i=0; i<4; i++) {
     printf("  %9x %9x %9x %9x %9x\n",
     data[5*i+0], data[5*i+1], data[5*i+2], data[5*i+3], data[5*i+4]);
     }
     for (i=0; i<4; i++) {
     printf("%6d %6d %6d %6d %6d %6d %6d %6d %6d %6d\n",
     data[10*i+0], data[10*i+1], data[10*i+2], data[10*i+3], data[10*i+4],
     data[10*i+5], data[10*i+6], data[10*i+7], data[10*i+8], data[10*i+9]);
     } */
  return ret;
}

/*--------------------------------------------------------*/
void breakhandler(int dummy)
{
   gd.gotsignal = 1;
}


/*--------------------------------------------------------*/
void sighandler(void)
{

   /* See signal(2) for details: 2 = SIGINT */
   /* Maybe this'll be different on different OSs so watch out. */
   signal(SIGINT, breakhandler);
   gd.gotsignal = 0;
}
/*--------------------------------------------------------*/

#include "get_event.c"

int main (int argc, char **argv)
{
  long numevents = 0;
  int  maxrecords = RW_MAXRECORDS, numrecords, totnumrecords=0;
  int  maxfiles = RW_MAXFILES, numfiles, totnumfiles=0, numtapes;
  int  scanning = 1, prompt_tape = 1, skipforward = 0;
  int  i, j, tfd=0;
  int  elist[RW_MAXMULT], gemult;
  char cbuf[512];

  while (!(mat=malloc(4096*4097*2))) {
    printf("Ooops, matrix malloc failed...\n"
	   "  ... please free up some memory and press return.\n");
    fgets(cbuf,256,stdin);
  }

  memset(mat,0,4096*4097*2);
  setup_replay();

  numtapes = 0;
  while (scanning) { /* loop through tapes */
    sighandler();   /* set up signal handling */
    maxfiles = 0;
    numfiles = 0;
    maxrecords = 0;
    numrecords = 0;
    skipforward = 0;

    if (totnumrecords>0) {
      printf ("\nWriting matrix....\n");
      rewind(gd.MatFile);
      for (i=0; i<4096; i++) {
	for (j=0; j<=i; j++) {
	  spec[j] = mat[luych[i]+j];
	}
	spec[i] *= 2;
	for (j=i+1; j<4096; j++) {
	  spec[j] = mat[luych[j]+i];
	}
	fwrite(spec,4096*4,1,gd.MatFile);
      }
      fflush(gd.MatFile);
    }

    while (prompt_tape) {
      /* Prompt to enter new tape */
      printf ("\nInsert tape and press Return,\n"
	      "   S to skip forward over some data,\n"
	      "            C to change tape drives,\n"
	      "                        or E to end. ");
      fgets(cbuf,512,stdin);
      if (cbuf[0] == 'e' || cbuf[0] == 'E') {
	scanning = 0;
	break;

      } else if (cbuf[0] == 's' || cbuf[0] == 'S') {
	skipforward = 1;

      } else if (cbuf[0] == 'c' || cbuf[0] == 'C') {
	printf ("  ...Enter new tape device: ");
	fgets(cbuf,256,stdin);
        if (strlen(cbuf)<2) continue;
	cbuf[strlen(cbuf)-1] = 0;    /* remove trailing \n */
	while (cbuf[strlen(cbuf)-1]==' ') {
	  cbuf[strlen(cbuf)-1] = 0;  /* remove trailing spaces */
	}
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
    if (!scanning) break;
 
    /* Get the maximum number of files on tape to be scanned */
    while (1) {
      if (skipforward)
	printf("\nNumber of files to skip (Return for all, or E to end): ");
      else
	printf("\nNumber of files to scan (Return for all, or E to end): ");
      fgets(cbuf,512,stdin);
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
      } else if (maxfiles == 0 && cbuf[0] != 0) {
	printf("\007  ERROR -- Invalid response.\n");
      } else if (maxfiles == 0) {
	scanning = 0;
      } else if (maxfiles > RW_MAXFILES) {
	maxfiles = 0;
	printf("\007  ERROR -- Value too large (max = %d).\n",RW_MAXFILES);
      } else {
	break;
      }
    }
    if (!scanning) break;

    /* Get the max number of records on tape to be scanned */
    while (1) {
      if (skipforward)
	printf("\nNumber of records to skip (Return for all):  ");
      else
	printf("\nNumber of records to scan (Return for all):  ");
      fgets(cbuf,512,stdin);
      if (cbuf[0] == '\n') {
	maxrecords = RW_MAXRECORDS;
	break;
      }
      maxrecords = strtol(cbuf, NULL, 0);
      if (maxrecords < 0) {
	printf("\007  ERROR -- Cannot be less than 0.\n");
      } else if (maxrecords == 0 && cbuf[0] != 0) {
	printf("\007  ERROR -- Invalid response.\n");
      } else if (maxrecords == 0) {
	scanning = 0;
      } else if (maxrecords > RW_MAXRECORDS) {
	maxrecords = 0;
	printf("\007  ERROR -- Value too large (max = %d).\n",RW_MAXRECORDS);
      } else {
	break;
      }
    }
    if (!scanning) break;

    sprintf(cbuf,"Scan started:  %s\n"
	    "  Hit control-C to interrupt at any time...\n\n", datim());
    printf("%s",cbuf);
    fflush(stdin);
    fprintf(gd.LogFile,"%s",cbuf);
    fflush(gd.LogFile);

    /* process events */
    while (numrecords < maxrecords && numfiles < maxfiles) {
      if (gd.gotsignal) break;

      /* read in buffer */
      i = read_tape(tfd);
      if (i <= 0) {
	if (i < 0) {
	  /* read returned unknown error */
	  sprintf(cbuf,"ERROR -- Could not read from %s.\n"
		  "End-of-Tape reached??  %s\n\n", gd.TapeDev, datim());
	} else {
	  /* end of file */
	  sprintf(cbuf,"End-of-File reached  %s\n\n", datim());
	}
	numfiles++; totnumfiles++;
	printf("\007\n%s",cbuf);
	fprintf(gd.LogFile,"%s",cbuf);
	prompt_tape = 1;
	printf("Closing tape device %s...\n",gd.TapeDev);
	fprintf(gd.LogFile,"Closing tape device %s...\n",gd.TapeDev);
	if (close(tfd)) {
	  printf("\007 Ooops, could not close tape device!\n");
	  fprintf(gd.LogFile," Ooops, could not close tape device!\n");
	}
	break;
      } else if (i < gd.RecordSize) {
	/* full buffer not read, throw it away. */
	printf("\n  Discarding tape buffer with %d bytes!\n", i);
	fflush(stdin);
	continue;
      }

      /* no worries */
      numrecords++; totnumrecords++;
      if (skipforward) {
	if ((numrecords&31) == 0) {
	  printf("\r %d records skipped...", numrecords);
	  fflush(stdout);
	}
	continue;
      }

      while ((gemult = get_event(elist)) >= 0) {
	numevents++;
        if (gemult < 2) continue;

	/* increment matrix */
	for (i=0; i<gemult-1; i++) {
	  if (elist[i]<4095 && elist[i]>0) {
	    for (j=i+1; j<gemult; j++) {
	      if (elist[j]<4095 && elist[j]>0) {
	        if (elist[i]<elist[j]) {
		  mat[luych[elist[j]] + elist[i]]++;
		} else {
		  mat[luych[elist[i]] + elist[j]]++;
		}
	      }
	    }
	  }
	}
      }

      if ((numrecords&7) == 0) {
	printf("\r %d recs, %ld evnts...", numrecords, numevents);
	fflush(stdout);
      }
    }
  }

  sprintf(cbuf,"Scan completed:  %s\n\n"
	       "      Number of tapes scanned = %d\n"
	       "   Total number of files read = %d\n"
	       " Total number of records read = %d\n"
	       "       Total number of events = %ld\n",
	  datim(),numtapes,totnumfiles,totnumrecords,numevents);
  printf("%s",cbuf);
  fprintf(gd.LogFile,"%s",cbuf);

  fflush(stdout);
  fclose(gd.LogFile);
  exit(0);
}
