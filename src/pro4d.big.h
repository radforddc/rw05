
#define RW_4DFILE "4play.4d"   /* Default filename for hypercube */
#define RW_3DFILE "4play.cub"  /* Default filename for 3D cube */

/*#define RW_MAXCH 1024 */
#define RW_MAXCH 1400          /* Maximum channels on cube axis */
                               /*    (must be multiple of 8)    */

#define RW_MAX_FILES 32        /* max number of sub-cube-files        */
#define RW_MAX_FILE_SIZE 2047  /* max 4D file size in SuperRecords (~MB) */

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

/************************************************
 *  3D CUBE FILE FORMAT
 *  1024byte file header
 *  1024byte data records
 *
 *      each record contains:
 *      uncompressed 8x8x8 mini-cubes, 2bytes/channel
 */

  /* 3d cube file header */
typedef struct {
  char id[16];            /* "Incub8r3/Pro4d  " */
  int  numch;             /* number of channels on axis */
  int  bpc;               /* bytes per channel, = 4 */
  int  cps;               /* 1/cps symmetry compression, = 6 */
  int  numrecs;           /* number of 4kB data records in the file */
  char resv[992];         /* FUTURE flags */
} FHead3D;
  /* 3Drecord header */
typedef struct {
  int minmc;          /* start minicube number, starts at 0 */
  short nummc;        /* number of minicubes stored in here */
  short offset;       /* offset in bytes to first full minicube */
} RHead3D;

typedef struct {
  RHead3D h;
  unsigned char d[4088];  /* the bit compressed data */
} Record3D;               /* see the compression alg for details */
