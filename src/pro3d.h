
#define RW_3DFILE "4play.cub"  /* Default filename for 3D cube */

#define RW_MAXCH 1400          /* Maximum channels on cube axis */
                               /*    (must be multiple of 8)    */

/************************************************
 *  COMPRESSED 3D CUBE FILE FORMAT
 *  1024byte file header
 *  4096byte data records
 *
 *      each record contains:
 *      compressed 8x8x4 mini-cubes, originally 4bytes/channel
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
  int minmc;               /* start minicube number, starts at 0 */
  unsigned short nummc;    /* number of minicubes stored in here */
  unsigned short offset;   /* offset in bytes to first full minicube */
} RHead3D;

typedef struct {
  RHead3D h;
  unsigned char d[4088];  /* the bit compressed data */
} Record3D;               /* see the compression alg for details */
