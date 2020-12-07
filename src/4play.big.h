
#define RW_MAXFILES 9999         /* Max files per tape to read */
#define RW_MAXRECORDS 1000000000 /* Max number of records per tape to read */
#define RW_DATFILE "4play.inc"   /* Defaults file for hypercube */

/*#define RW_MAXCH 1024 */
#define RW_MAXCH 1400            /* Maximum channels on cube axis */
                                 /*    (must be multiple of 4)    */

/* a = RW_MAXCH/4                 = 350
   b = a*(a+1)*(a+2)*(a+3)/24     = 636035400
   c = ceil(b/256)                = 2484514
   RW_LB2 = ceil(c/RW_CHNK_S)     = 12941
   RW_LB1 = RW_LB2*RW_CHNK_S      = 2484672

   RW_DB1 = floor((RW_SCR_RECL-4)/(2*(RW_DB2+1)))
   */

#define RW_LB1 2484672     /* length required for buf1, depends on RW_MAXCH */
                           /* RW_LB1 > x(x+1)(x+2)(x+3)/(24*256),           */
		           /* where x = RW_MAXCH/4                          */
                           /* must also be a multiple of RW_CHNK_S          */
#define RW_DB1 6           /* depth of buf1 */

#define RW_LB2 12941       /* length required for buf2, depends on RW_LB1   */
                           /* RW_LB2 >= RW_LB1/RW_CHNK_S                    */
#define RW_DB2 292         /* depth of buf2 not to exceed RW_SCR_RECL       */
                           /* byte records in scr file                      */

#define RW_SCR_RECL 4096   /* Record length of scr file                     */
                           /* must be >= RW_DB2*2*(RW_DB1+1) + 4            */
#define RW_CHNK_S  192     /* size of update chunk, in units of 1/8 MB      */

#define RW_MAXMULT 40

#define RW_MAX_FILES 32        /* max number of sub-cube-files        */
#define RW_MAX_FILE_SIZE 2047  /* max file size in SuperRecords (~MB) */

/************************************************
 *  HYPERCUBE FILE FORMAT
 *  1024byte file header
 *  1Mbyte SuperRecords
 *
 *    each SuperRecord contains:
 *    1024byte header
 *    1023 1024byte records
 *
 *      each record contains:
 *      variable number of bit-compressed 4x4x4x4 mini-cubes
 *      8bytes header, 1016bytes of data
 */

  /* hypercube file header */
typedef struct {
  char id[16];            /* "4play hypercube " */
  int  numch;             /* number of channels on axis */
  int  cps;               /* 1/cps symmetry compression */
  int  version;           /* 0 */
  int  snum;              /* number of SuperRecords in this file */
  int  mclo;              /* number of first minicube in this subfile */
  int  mchi;              /* number of last minicube in this subfile */
  char resv[984];         /* FUTURE flags */
} FHead;

  /* SuperRecord header */
typedef struct {
  int snum;           /* virtual SRec number (for ordering), starts at 0*/
  int minmc;          /* start minicube number, starts at 0 */
  int maxmc;          /* number of last minicube stored in here */
  char resv[1012];
} SRHead;

  /* record header */
typedef struct {
  int minmc;               /* start minicube number, starts at 0 */
  unsigned short nummc;    /* number of minicubes stored in here */
  unsigned short offset;   /* offset in bytes to first full minicube */
} RHead;

typedef struct {
  RHead h;
  unsigned char d[1016];  /* the bit compressed data */
} Record;                 /* see the compression alg for details */
