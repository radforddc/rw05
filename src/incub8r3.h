
#define RW_MAXFILES 9999         /* Max files per tape to read */
#define RW_MAXRECORDS 1000000000 /* Max number of records per tape to read */
#define RW_DATFILE "incub8r.inc" /* Defaults file for hypercube */

#define RW_MAXCH 1400            /* Maximum channels on cube axis */
                                 /*    (must be multiple of 8)    */

/* a = RW_MAXCH/8                 = 175
   b = a*(a+1)*(a+2)/6            = 908600
   c = ceil(b/128)                = 7099
   RW_LB2 = ceil(c/RW_CHNK_S)     = 222
   RW_LB1 = RW_LB2*RW_CHNK_S      = 7104

   RW_DB1 = floor((RW_SCR_RECL-4)/(2*(RW_DB2+1)))
   */

#define RW_LB1 7104        /* length required for buf1, depends on RW_MAXCH */
                           /* RW_LB1 > x(x+1)(x+2)/(6*128),                 */
		           /* where x = RW_MAXCH/8                          */
                           /* must also be a multiple of RW_CHNK_S          */
#define RW_DB1 90          /* depth of buf1 */

#define RW_LB2 222         /* length required for buf2, depends on RW_LB1   */
                           /* RW_LB2 >= RW_LB1/RW_CHNK_S                    */
#define RW_DB2 180         /* depth of buf2 not to exceed RW_SCR_RECL       */
                           /* byte records in scr file                      */

#define RW_SCR_RECL 32768  /* Record length of scr file                     */
                           /* must be >= RW_DB2*2*(RW_DB1+1) + 4            */
#define RW_CHNK_S  32      /* size of update chunk, in units of 1/4 MB      */

#define RW_MAXMULT 40

/************************************************
 *  3D CUBE FILE FORMAT
 *  1024byte file header
 *  4096byte data records
 *
 *      each data record contains:
 *      variable number of bit-compressed 8x8x4 mini-cubes
 *      8bytes header, 4088 bytes of data
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
